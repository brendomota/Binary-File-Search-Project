#include<stdio.h>
#include<conio.h>

int main() {
    FILE *fd;
    
    //////////////////////////////
    struct hist {
        char id_aluno[4];
        char sigla_disc[4];
        char nome_aluno[50];
        char nome_disc[50];
        float media;
        float freq;
    } vet[10] = {{"001", "001", "Joao da Silva", "Disc-111", 7.5, 72.1},
                {"001", "002", "Joao da Silva", "Disc-222", 8.2, 80.2},
                {"001", "003", "Paulo Souza", "Disc-333", 5.4, 79.3},                
                {"002", "004", "Paulo Souza", "Disc-444", 6.8, 91.4},
            
                {"002", "005", "Pedro", "Disc-555", 6.3, 72.5},
                {"002", "006", "Joao da Silva", "Disc-666", 8.3, 77.6},
                {"003", "007", "Lucas Silva e Silva", "Disc-777", 9.7, 89.7},
                
                {"004", "008", "Pedro", "Disc-888-888", 9.5, 92.8},
                {"005", "009", "Lucas Silva e Silva", "Disc-888-888", 9.5, 92.9},
                {"001", "010", "Joao da Silva", "Disc-888-888", 9.5, 92.0}};
       
    fd = fopen("insere.bin", "w+b");
    fwrite(vet, sizeof(vet), 1, fd);
    fclose(fd);
    
    //////////////////////////////
    struct busca_p {
        char id_aluno[4];
        char sigla_disc[4];
    } vet_bp[5] = {{"001","003"},
                  {"002","004"},
                  
                  {"003","007"},
                  {"001","001"},
                  {"001","010"}};
    
    fd = fopen("busca_p.bin", "w+b");
    fwrite(vet_bp, sizeof(vet_bp), 1, fd);
    fclose(fd);
    
    //////////////////////////////
    char nomes[5][50] = {"Paulo Souza", "Joao da Silva", "Lucas Silva e Silva", "Pedro", "Joao da Silva"};
       
    fd = fopen("busca_s.bin", "w+b");
    for (int i=0; i<5; i++)
       fwrite(nomes[i], sizeof(char), 50, fd);
    fclose(fd);
    
    /*char buffer[50];
    fd = fopen("busca_s.bin", "r+b");
    for (int i=0; i<quantidade; i++)
       {
         fread(buffer, sizeof(buffer), 1, fd);
         printf("\n%s",buffer);
       }
    fclose(fd);
    getch();*/
}

