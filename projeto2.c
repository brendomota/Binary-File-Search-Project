#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*--------ESTRUTURA PARA INSERIR E busca_p---------*/
typedef struct
{
    char id_aluno[4];
    char sigla_disc[4];
    char nome_aluno[50];
    char nome_disc[50];
    float media;
    float freq;
} historico;

typedef struct
{
    char id_aluno[4];
    char sigla_disc[4];
} busca_p;

typedef struct
{
    char elem_id_aluno[4];
    char elem_sigla_disc[4];
    int elem_deslocamento;
} elem_vetor;

typedef struct
{
    char nome[50];
    int byte_lista_invertida;
} secundaria_nomes;

typedef struct
{
    char chave[6];
    int prox;
} secundaria_chaves;

/*----------FUNÇÃO QUE RETORNA O TAMANHO DO REGISTRO A SER LIDO----------*/
int pegar_tamanho_reg(FILE *fd, char *registro)
{
    char byte;

    if (!fread(&byte, sizeof(int), 1, fd))
    {
        return 0;
    }
    else
    {
        fread(registro, byte, 1, fd);
        registro[byte] = '\0';
        return byte;
    }
}

/*---------FUNÇÃO PARA INSERIR UM REGISTRO NO ARQUIVO----------*/
void inserir_registro(FILE *in, FILE *in_aux, FILE *out, FILE *indice_p, FILE *indice_s_nome)
{
    // Variáveis que serão utilizadas na inserção
    historico hist;
    int byte_in_aux;
    int tam_reg;
    int cabecalho;
    char registro[120];

    // Obtem o byte a ser lido no arquivo insere.bin
    fread(&byte_in_aux, sizeof(int), 1, in_aux);

    // Faz a leitura do arquivo insere.bin e formata o conteúdo para depois inserir no arquivo out.bin
    fseek(in, byte_in_aux, 0);
    fread(&hist, sizeof(hist), 1, in);
    sprintf(registro, "%s#%s#%s#%s#%.2f#%.2f", hist.id_aluno, hist.sigla_disc, hist.nome_aluno, hist.nome_disc, hist.media, hist.freq);
    printf("\n%s", registro);
    tam_reg = strlen(registro);
    tam_reg++;
    registro[tam_reg] = '\0';
    printf("\n%d", tam_reg);

    rewind(out);
    fread(&cabecalho, sizeof(int), 1, out);
    // Verifica se o cabeçalho é igual a -1, para adicionar no último byte do arquivo out.bin
    if (cabecalho == -1)
    {
        fseek(out, 0, SEEK_END);
        fwrite(&tam_reg, sizeof(int), 1, out);
        fwrite(registro, sizeof(char), tam_reg, out);
    }

    // Atualiza o arquivo in_aux
    rewind(in_aux);
    byte_in_aux += 116;
    fwrite(&byte_in_aux, sizeof(int), 1, in_aux);
    rewind(in_aux);

    int cabecalho_indice = 0;

    rewind(indice_p);
    fwrite(&cabecalho_indice, sizeof(int), 1, indice_p);

    int cabecalho_s = 0;

    rewind(indice_s_nome);
    fwrite(&cabecalho_s, sizeof(int), 1, indice_s_nome);
}

/*--------FUNÇÃO PARA RECRIAR ARQUIVO DE ÍNDICE---------*/
void recriar_indice(FILE *indice_p, FILE *out)
{
    int size = 0;
    elem_vetor vetor[100]; // Inicializa um vetor com capacidade para 100 elementos (ajuste conforme necessário)

    int cabecalho_indice;

    int deslocamento;
    elem_vetor element;
    int tam;
    char buffer_hashtag[2];

    fseek(out, sizeof(int), SEEK_SET); // Pular o cabeçalho do arquivo

    while (fread(&tam, sizeof(int), 1, out)) // Lê o tamanho do registro
    {
        // Atualizar o deslocamento
        deslocamento = ftell(out) - sizeof(int);

        // Ler a chave primária (Código de aluno e Sigla da disciplina)
        fread(element.elem_id_aluno, sizeof(char), 3, out); // Lê 3 caracteres do código do aluno
        element.elem_id_aluno[3] = '\0';                    // Adiciona o terminador de string
        printf("\nElemento.id: %s", element.elem_id_aluno);

        fread(buffer_hashtag, sizeof(char), 1, out); // Ignorar o '#'

        fread(element.elem_sigla_disc, sizeof(char), 3, out); // Lê 3 caracteres da sigla da disciplina
        element.elem_sigla_disc[3] = '\0';                    // Adiciona o terminador de string
        printf("\nElemento.sigla: %s", element.elem_sigla_disc);

        // Armazena o deslocamento do registro
        element.elem_deslocamento = deslocamento;

        // Inserir o elemento no vetor de forma ordenada
        int i = size - 1;
        while (i >= 0 && (strcmp(vetor[i].elem_id_aluno, element.elem_id_aluno) > 0 ||
                          (strcmp(vetor[i].elem_id_aluno, element.elem_id_aluno) == 0 &&
                           strcmp(vetor[i].elem_sigla_disc, element.elem_sigla_disc) > 0)))
        {
            vetor[i + 1] = vetor[i]; // Desloca os elementos para a direita
            i--;
        }
        vetor[i + 1] = element; // Insere o elemento na posição correta
        size++;

        // Pular o restante do registro (tam - (tamanho da chave + delimitadores))
        fseek(out, tam - (3 + 1 + 3), SEEK_CUR); // 3 (ID aluno) + 1 (#) + 3 (Sigla)
    }

    // Escrever o vetor ordenado no arquivo de índice
    fseek(indice_p, sizeof(int), SEEK_SET);
    for (int pos = 0; pos < size; pos++)
    {
        fwrite(vetor[pos].elem_id_aluno, sizeof(char), 3, indice_p);       // Escreve o ID do aluno
        fwrite(vetor[pos].elem_sigla_disc, sizeof(char), 3, indice_p);     // Escreve a sigla da disciplina
        fwrite(&(vetor[pos].elem_deslocamento), sizeof(int), 1, indice_p); // Escreve o deslocamento
    }

    // Escrever o cabeçalho do índice
    cabecalho_indice = 1;
    rewind(indice_p);
    fwrite(&cabecalho_indice, sizeof(int), 1, indice_p);
    rewind(indice_p);
}

/*--------FUNÇÃO PARA BUSCA PRIMÁRIA UM REGISTRO---------*/
void busca_p_registro(FILE *indice_p, FILE *busca_primaria, FILE *busca_p_aux, FILE *out)
{
    rewind(indice_p);

    // Estamos pegando a chave para ser lida
    char pegar_chave[20];
    int byte_busca_p_aux;
    busca_p busca;
    fread(&byte_busca_p_aux, sizeof(int), 1, busca_p_aux);
    fseek(busca_primaria, byte_busca_p_aux, 0);
    fread(&busca, sizeof(busca), 1, busca_primaria);
    sprintf(pegar_chave, "%s%s", busca.id_aluno, busca.sigla_disc);

    printf("\nChave que iremos buscar: %s", pegar_chave);

    int deslocamento;
    int cabecalho_indice;
    char chave[6];

    fread(&cabecalho_indice, sizeof(int), 1, indice_p);

    printf("\nCabecalho: %d", cabecalho_indice);

    int flag_encontrou = 0;
    if (cabecalho_indice == 1)
    {
        fseek(indice_p, sizeof(int), SEEK_SET);
        while (fread(&chave, sizeof(char), 6, indice_p))
        {
            printf("\nChave que estamos vendo: %s", chave);
            chave[6] = '\0';
            if (strcmp(pegar_chave, chave) == 0)
            {
                fread(&deslocamento, sizeof(int), 1, indice_p);
                flag_encontrou = 1;
                printf("\nChave encontrada: %s", chave);
                break;
            }
            fread(&deslocamento, sizeof(int), 1, indice_p);
        }

        if (flag_encontrou == 0)
        {
            printf("\nA chave nao foi encontrada");
        }

        else
        {
            fseek(out, deslocamento, SEEK_SET);

            char registro[120];
            int tam_reg = pegar_tamanho_reg(out, registro);
            char *pch;
            pch = strtok(registro, "#");

            printf("\nRegistro encontrado:\n");
            while (pch != NULL)
            {
                printf("%s\n", pch);
                // strcpy(teste,pch);
                // printf("%s\n",teste);
                pch = strtok(NULL, "#");
            }
            printf("\n");
        }
    }
    else
    {
        printf("\nVamos recriar o indice");
        recriar_indice(indice_p, out);

        fseek(indice_p, sizeof(int), SEEK_SET);
        while (fread(&chave, sizeof(char), 6, indice_p))
        {
            printf("\nChave que estamos vendo: %s", chave);
            chave[6] = '\0';
            if (strcmp(pegar_chave, chave) == 0)
            {
                fread(&deslocamento, sizeof(int), 1, indice_p);
                flag_encontrou = 1;
                printf("\nChave encontrada: %s", chave);
                break;
            }
            fread(&deslocamento, sizeof(int), 1, indice_p);
        }

        if (flag_encontrou == 0)
        {
            printf("\nA chave nao foi encontrada");
        }

        else
        {
            fseek(out, deslocamento, SEEK_SET);

            char registro[120];
            int tam_reg = pegar_tamanho_reg(out, registro);
            char *pch;
            pch = strtok(registro, "#");

            printf("\nRegistro encontrado:\n");
            while (pch != NULL)
            {
                printf("%s#", pch);
                // strcpy(teste,pch);
                // printf("%s\n",teste);
                pch = strtok(NULL, "#");
            }
            printf("\n");
        }
    }
    rewind(busca_p_aux);
    byte_busca_p_aux += 8;
    fwrite(&byte_busca_p_aux, sizeof(int), 1, busca_p_aux);
    rewind(busca_p_aux);
}

void recriar_indice_secundario(FILE *indice_s_nome, FILE *indice_s_chave, FILE *indice_p, FILE *out)
{
    secundaria_nomes vetor_nomes[50]; // Vetor para armazenar nomes de forma ordenada
    secundaria_nomes elementos_nome;
    secundaria_chaves vetor_chaves[50];
    secundaria_chaves elementos_chave;

    fseek(indice_s_nome, sizeof(int), SEEK_SET); // Movendo ponteiro no arquivo de nomes (se houver um cabeçalho)
    fseek(out, sizeof(int), SEEK_SET);           // Movendo ponteiro no arquivo de registros

    char registro[120];
    char nome[256];

    int byte_out_atual = ftell(out);            // Pega a posição atual no arquivo 'out'
    int tam = pegar_tamanho_reg(out, registro); // Função que lê o tamanho do registro e retorna o registro lido

    int num_nomes = 0; // Contador de nomes no vetor

    // Processar cada registro no arquivo 'out'
    while (tam > 0)
    {
        printf("\nENTROU NO WHILE");
        // Volta o ponteiro para o início do registro para processar
        fseek(out, byte_out_atual, SEEK_SET);

        // Copiar o registro para uma string temporária para manipulação
        char temp_registro[120];
        strcpy(temp_registro, registro);

        // Extraindo o nome do registro (3 campos antes do nome)
        char *token = strtok(temp_registro, "#"); // ID aluno
        token = strtok(NULL, "#");                // Sigla da disciplina
        token = strtok(NULL, "#");                // Nome do aluno

        printf("\n TOKEN: %s", token);

        if (token != NULL)
        {
            strcpy(nome, token); // Copiando o nome extraído para a variável 'nome'
        }
        else
        {
            break; // Se não houver nome, encerra o laço
        }

        

        // Insere o nome no vetor na posição correta (permitindo duplicatas)
        int i = 0, j;
        while (i < num_nomes && strcmp(vetor_nomes[i].nome, nome) < 0)
        {
            i++;
        }

        // Move os elementos para a direita para abrir espaço para o novo nome
        for (j = num_nomes; j > i; j--)
        {
            vetor_nomes[j] = vetor_nomes[j - 1];
        }

        // Insere o novo nome na posição correta
        strcpy(vetor_nomes[i].nome, nome);
        vetor_nomes[i].byte_lista_invertida = -1; // O byte da lista invertida será atualizado posteriormente
        num_nomes++;

        // Avança o ponteiro para o próximo registro no arquivo 'out'
        byte_out_atual += sizeof(int) + tam;    // Atualiza o byte offset
        tam = pegar_tamanho_reg(out, registro); // Lê o próximo registro
    }
    printf("\nnum_nomes: %d", num_nomes);

    // Imprimindo os nomes do vetor em ordem crescente (incluindo repetidos)
    printf("Nomes no vetor (em ordem alfabética):\n");
    for (int i = 0; i < num_nomes; i++)
    {
        printf("%s\n", vetor_nomes[i].nome); // Imprime cada nome no vetor
    }
}

void busca_s_registro(FILE *indice_s_nome, FILE *indice_s_chave, FILE *busca_secundaria, FILE *indice_p, FILE *busca_s_aux, FILE *out)
{
    int cabecalho_s;
    rewind(indice_s_nome);
    fread(&cabecalho_s, sizeof(int), 1, indice_s_nome);
    rewind(indice_s_nome);
    if (cabecalho_s == 0)
    {
        recriar_indice_secundario(indice_s_nome, indice_s_chave, indice_p, out);
    }
}

int main()
{
    int cabecalho;
    int cabecalho_p;
    int cabecalho_s;

    /*--------CRIAÇÃO DOS PONTEIROS DOS ARQUIVOS---------*/

    // Ponteiros para obter informações dos arquivos insere.bin, busca_primaria.bin e busca_secundaria.bin
    FILE *in;
    if (!(in = fopen("insere.bin", "r+b")))
    {
        printf("\nNao foi possivel abrir o arquivo de insersao");
        return 0;
    }

    FILE *busca_primaria;
    if (!(busca_primaria = fopen("busca_p.bin", "r+b")))
    {
        printf("\nNao foi possivel  abrir o arquivo busca_p.bin");
        return 0;
    }

    FILE *busca_secundaria;
    if (!(busca_secundaria = fopen("busca_s.bin", "r+b")))
    {
        printf("\nNao foi possivel  abrir o arquivo busca_s.bin");
        return 0;
    }

    /*----------CRIAÇÃO DO ARQUIVO QUE ARMAZENA O BYTE A SER LIDO NO ARQUIVO INSERE.BIN----------*/
    FILE *in_aux;
    int byte_in_aux;

    in_aux = fopen("in_aux.bin", "r+b");

    // Se o arquivo não existir, cria-o com "w+b"
    if (in_aux == NULL)
    {
        // Arquivo ainda não existe, tem que ser criado com w+b
        in_aux = fopen("in_aux.bin", "w+b");
        if (in_aux == NULL)
        {
            printf("\nNao foi possivel criar o arquivo auxiliar de insersao");
            return 0;
        }
        // Como o arquivo é novo, consideramos que é a primeira inserção
        byte_in_aux = 0;
        fwrite(&byte_in_aux, sizeof(int), 1, in_aux);
    }
    else
    {
        // Verifica se o arquivo está vazio
        fseek(in_aux, 0, SEEK_END);
        long tam_in_aux = ftell(in_aux);

        if (tam_in_aux == 0)
        {
            byte_in_aux = 0;
            fwrite(&byte_in_aux, sizeof(int), 1, in_aux);
        }
    }
    rewind(in_aux);

    /*----------CRIAÇÃO DO ARQUIVO QUE ARMAZENA O BYTE A SER LIDO NO ARQUIVO BUSCA_P.BIN----------*/
    FILE *busca_p_aux;
    int byte_busca_p_aux;

    busca_p_aux = fopen("busca_p_aux.bin", "r+b");

    // Se o arquivo não existir, cria-o com "w+b"
    if (busca_p_aux == NULL)
    {
        // Arquivo ainda não existe, tem que ser criado com w+b
        busca_p_aux = fopen("busca_p_aux.bin", "w+b");
        if (busca_p_aux == NULL)
        {
            printf("\nNao foi possivel criar o arquivo auxiliar de busca primaria");
            return 0;
        }
        // Como o arquivo é novo, consideramos que é a primeira busca
        byte_busca_p_aux = 0;
        fwrite(&byte_busca_p_aux, sizeof(int), 1, busca_p_aux);
    }
    else
    {
        // Verifica se o arquivo está vazio
        fseek(busca_p_aux, 0, SEEK_END);
        long tam_busca_p_aux = ftell(busca_p_aux);

        if (tam_busca_p_aux == 0)
        {
            byte_busca_p_aux = 0;
            fwrite(&byte_busca_p_aux, sizeof(int), 1, busca_p_aux);
        }
    }
    rewind(busca_p_aux);

    /*----------CRIAÇÃO DO ARQUIVO QUE ARMAZENA O BYTE A SER LIDO NO ARQUIVO BUSCA_S.BIN----------*/
    FILE *busca_s_aux;
    int byte_busca_s_aux;

    busca_s_aux = fopen("busca_s_aux.bin", "r+b");

    // Se o arquivo não existir, cria-o com "w+b"
    if (busca_s_aux == NULL)
    {
        // Arquivo ainda não existe, tem que ser criado com w+b
        busca_s_aux = fopen("busca_s_aux.bin", "w+b");
        if (busca_s_aux == NULL)
        {
            printf("\nNao foi possivel criar o arquivo auxiliar de busca primaria");
            return 0;
        }
        // Como o arquivo é novo, consideramos que é a primeira busca
        byte_busca_s_aux = 0;
        fwrite(&byte_busca_s_aux, sizeof(int), 1, busca_s_aux);
    }
    else
    {
        // Verifica se o arquivo está vazio
        fseek(busca_s_aux, 0, SEEK_END);
        long tam_busca_s_aux = ftell(busca_s_aux);

        if (tam_busca_s_aux == 0)
        {
            byte_busca_s_aux = 0;
            fwrite(&byte_busca_s_aux, sizeof(int), 1, busca_s_aux);
        }
    }
    rewind(busca_s_aux);

    // Arquivo de saída
    FILE *out = fopen("out.bin", "r+b"); // Estamos usando r+b pois ela não permite truncamento (não zera o tamanho)
    if (out == NULL)
    {
        // Arquivo ainda não existe, tem que ser criado com w+b
        out = fopen("out.bin", "w+b");
        if (out == NULL)
        {
            printf("\nNao foi possivel criar o arquivo de saida");
            return 0;
        }
        cabecalho = -1;
        fwrite(&cabecalho, sizeof(int), 1, out);
    }

    // Arquivo de índice primário
    FILE *indice_p = fopen("indice_p.bin", "r+b"); // Estamos usando r+b pois ela não permite truncamento (não zera o tamanho)
    if (indice_p == NULL)
    {
        // Arquivo ainda não existe, tem que ser criado com w+b
        indice_p = fopen("indice_p.bin", "w+b");
        if (indice_p == NULL)
        {
            printf("\nNao foi possivel criar o arquivo de indice primario");
            return 0;
        }
        cabecalho_p = 0;
        fwrite(&cabecalho_p, sizeof(int), 1, indice_p);
    }

    // Arquivo de índice secundário para o nome
    FILE *indice_s_nome = fopen("indice_s_nome.bin", "r+b"); // Estamos usando r+b pois ela não permite truncamento (não zera o tamanho)
    if (indice_s_nome == NULL)
    {
        // Arquivo ainda não existe, tem que ser criado com w+b
        indice_s_nome = fopen("indice_s_nome.bin", "w+b");
        if (indice_s_nome == NULL)
        {
            printf("\nNao foi possivel criar o arquivo de indice secundario de nome");
            return 0;
        }
        cabecalho_s = 0;
        fwrite(&cabecalho_s, sizeof(int), 1, indice_s_nome);
    }

    // Arquivo de índice secundário para as chaves
    FILE *indice_s_chave = fopen("indice_s_chave.bin", "r+b"); // Estamos usando r+b pois ela não permite truncamento (não zera o tamanho)
    if (indice_s_chave == NULL)
    {
        // Arquivo ainda não existe, tem que ser criado com w+b
        indice_s_chave = fopen("indice_s_chave.bin", "w+b");
        if (indice_s_chave == NULL)
        {
            printf("\nNao foi possivel criar o arquivo de indice secundario de chave");
            return 0;
        }
    }

    /*----------MENU PARA INTERAÇÃO COM O PROGRAMA----------*/
    printf("\n----------MENU----------");
    printf("\n1. Inserir");
    printf("\n2. Busca Primaria");
    printf("\n3. Busca Secundaria");
    printf("\n4. Sair do Programa");
    int opcao = -1;

    while (opcao != 4)
    {
        printf("\nINFORME SUA OPCAO: ");
        scanf("%d", &opcao);
        while (opcao < 1 || opcao > 4)
        {
            printf("\nOPCAO INFORMADA NAO EXISTE, INFORME SUA OPCAO NOVAMENTE: ");
            scanf("%d", &opcao);
        }

        /*----------OPERAÇÃO DE INSERIR NO ARQUIVO OUT.BIN----------*/
        if (opcao == 1)
        {
            inserir_registro(in, in_aux, out, indice_p, indice_s_nome);
            printf("\nINSERCAO REALIZADA COM SUCESSO");
        }

        /*---------OPERAÇÃO DE busca_p NO ARQUIVO OUT.BIN-----------*/
        if (opcao == 2)
        {
            busca_p_registro(indice_p, busca_primaria, busca_p_aux, out);
            printf("\nBUSCA PRIMARIA REALIZADA COM SUCESSO");
        }

        /*---------OPERAÇÃO DE busca_s NO ARQUIVO OUT.BIN-----------*/
        if (opcao == 3)
        {
            busca_s_registro(indice_s_nome, indice_s_chave, busca_secundaria, indice_p, busca_s_aux, out);
            printf("\nBUSCA SECUNDARIA REALIZADA COM SUCESSO");
        }

        if (opcao == 4)
        {
            break;
        }

        opcao = -1;
    }

    printf("\n----------PROGRAMA FINALIZADO----------");

    fclose(in);
    fclose(in_aux);
    fclose(busca_primaria);
    fclose(busca_p_aux);
    fclose(out);
    fclose(indice_p);

    return 0;
}
