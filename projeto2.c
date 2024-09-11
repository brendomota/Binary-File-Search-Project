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
void inserir_registro(FILE *in, FILE *in_aux, FILE *out)
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
        fseek(out, 0, 2);
        fwrite(&tam_reg, sizeof(int), 1, out);
        fwrite(registro, sizeof(char), tam_reg, out);
    }

    // Atualiza o arquivo in_aux
    rewind(in_aux);
    byte_in_aux += 116;
    fwrite(&byte_in_aux, sizeof(int), 1, in_aux);
    rewind(in_aux);
}

/*--------FUNÇÃO PARA BUSCA PRIMÁRIA UM REGISTRO---------*/
void busca_p_registro(FILE *re, FILE *re_aux, FILE *out)
{
    busca_p remove;
    int byte_re_aux;
    int tam_reg;
    int cabecalho;
    char registro[120];

    rewind(out);
    fread(&cabecalho, sizeof(int), 1, out);

    char pegar_chave[20];
    fread(&byte_re_aux, sizeof(int), 1, re_aux);
    fseek(re, byte_re_aux, 0);
    fread(&remove, sizeof(remove), 1, re);

    sprintf(pegar_chave, "%s%s", remove.id_aluno, remove.sigla_disc);

    int offset_aux = ftell(out);
    tam_reg = pegar_tamanho_reg(out, registro);
    char *ptrchar;
    int offset_byte = offset_aux;

    int chave_encontrada = 0; // Flag para ver se a chave foi encontrada ou não

    while (tam_reg > 0)
    {
        char reg_aux[120];
        char registro_copy[120];         // Cria uma cópia para o strtok
        strcpy(registro_copy, registro); // Copia o conteúdo de registro

        reg_aux[0] = '\0';
        ptrchar = strtok(registro_copy, "#");

        while (ptrchar != NULL)
        {
            strcat(reg_aux, ptrchar);
            ptrchar = strtok(NULL, "#");
        }

        if (strstr(reg_aux, pegar_chave) != NULL)
        {
            printf("\nRegistro que sera removido: %s", reg_aux);
            int tamanho_bytes_registro = tam_reg;
            char *estrela = "*";
            int offset_proximo_registro = cabecalho;

            fseek(out, offset_byte, 0);
            cabecalho = offset_byte;

            fwrite(&tamanho_bytes_registro, sizeof(int), 1, out);
            fwrite(estrela, sizeof(char), 1, out);
            fwrite(&offset_proximo_registro, sizeof(int), 1, out);

            // Volta para o início para escrever o cabeçalho
            fseek(out, 0, SEEK_SET);
            fwrite(&offset_byte, sizeof(int), 1, out);
            chave_encontrada = 1;
            break;
        }

        // Percorrer o lixo caso tenha
        char buffer[200];
        int offset_aux_ini = ftell(out);
        offset_aux = ftell(out);
        fread(&tam_reg, sizeof(int), 1, out); // Lê o tamanho do registro como int
        int flagwhile = 0;

        while (tam_reg <= 0 || tam_reg > sizeof(buffer))
        {
            flagwhile = 1;
            printf("\nTamanho do registro em lixo: %d", tam_reg);

            // Avança 1 byte, pois o tam_reg pode estar lendo lixo
            fseek(out, ftell(out) - 3, SEEK_SET);
            offset_aux = ftell(out);

            // Lê o próximo byte e tenta interpretar como tamanho de registro
            fread(&tam_reg, sizeof(int), 1, out); // Ler como int para manter consistência
        }

        // Se não encontramos lixo, voltamos ao ponto de leitura original
        if (flagwhile == 0)
            fseek(out, offset_aux_ini, SEEK_SET);
        else
            fseek(out, offset_aux, SEEK_SET);

        tam_reg = pegar_tamanho_reg(out, registro);
        offset_byte = ftell(out) - tam_reg - sizeof(int);
    }

    if (chave_encontrada == 0)
    {
        printf("\nA chave nao foi encontrada e a proxima remocao acontecera com a chave seguinte a esta no arquivo remove.bin.");
    }

    // Atualiza o arquivo re_aux
    rewind(re_aux);
    byte_re_aux += 8;
    fwrite(&byte_re_aux, sizeof(int), 1, re_aux);
    rewind(re_aux);
}

int main()
{
    int cabecalho;

    /*--------CRIAÇÃO DOS PONTEIROS DOS ARQUIVOS---------*/

    // Ponteiros para obter informações dos arquivos insere.bin, busca_primaria.bin e busca_secundaria.bin
    FILE *in;
    if (!(in = fopen("insere.bin", "r+b")))
    {
        printf("\nNao foi possivel abrir o arquivo de insersao");
        return 0;
    }

    FILE *busca_p;
    if (!(busca_p = fopen("busca_primaria.bin", "r+b")))
    {
        printf("\nNao foi possivel  abrir o arquivo busca_primaria.bin");
        return 0;
    }

    FILE *busca_s;
    if (!(busca_s = fopen("busca_secundaria.bin", "r+b")))
    {
        printf("\nNao foi possivel  abrir o arquivo busca_secundaria.bin");
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
            printf("\nNao foi possivel criar o arquivo auxiliar de remocao");
            return 0;
        }
        // Como o arquivo é novo, consideramos que é a primeira inserção
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
            printf("\nNao foi possivel criar o arquivo auxiliar de insersao");
            return 0;
        }
        cabecalho = -1;
        fwrite(&cabecalho, sizeof(int), 1, out);
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
            inserir_registro(in, in_aux, out);
            printf("\nINSERCAO REALIZADA COM SUCESSO");
        }

        /*---------OPERAÇÃO DE busca_p NO ARQUIVO OUT.BIN-----------*/
        if (opcao == 2)
        {
            printf("\nREMOCAO REALIZADA COM SUCESSO");
        }

        /*---------OPERAÇÃO DE COMPACTAR O ARQUIVO OUT.BIN---------*/
        if (opcao == 3)
        {
            printf("\nCOMPACTACAO REALIZADA COM SUCESSO");
        }

        if (opcao == 4)
        {
            break;
        }

        opcao = -1;
    }

    printf("\n----------PROGRAMA FINALIZADO----------");
    fclose(in);
    fclose(out);
    fclose(in_aux);

    return 0;
}
