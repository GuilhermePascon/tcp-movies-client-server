#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

void display_menu()
{
    printf("\n===== SISTEMA DE STREAMING DE FILMES USANDO TCP =====\n");
    printf("1. Cadastrar um novo filme\n");
    printf("2. Adicionar um novo gênero a um filme\n");
    printf("3. Remover um filme pelo identificador\n");
    printf("4. Listar todos os títulos de filmes\n");
    printf("5. Listar informações de todos os filmes\n");
    printf("6. Listar informações de um filme específico\n");
    printf("7. Listar todos os filmes de um determinado gênero\n");
    printf("0. Sair\n");
    printf("Escolha uma opção: ");
}

int main(int argc, char *argv[])
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char server_ip[16] = "127.0.0.1"; // Endereço IP padrão (localhost)
    int port = 49153;                 // Porta não reservada

    // Verifica se foi fornecido um endereço IP
    if (argc >= 2)
    {
        strcpy(server_ip, argv[1]);
    }

    // Verifica se foi fornecida uma porta
    if (argc >= 3)
    {
        port = atoi(argv[2]);
    }

    // Cria o socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nErro ao criar socket\n");
        return -1;
    }

    // Configura o endereço do servidor
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Converte o endereço IP de texto para binário
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
    {
        printf("\nEndereço inválido / Endereço não suportado\n");
        return -1;
    }

    // Conecta ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConexão falhou\n");
        return -1;
    }

    printf("Conectado ao servidor %s:%d\n", server_ip, port);

    int option;
    char message[BUFFER_SIZE];

    // Loop principal do cliente
    while (1)
    {
        display_menu();
        scanf("%d", &option);
        getchar(); // Consome o caractere de nova linha

        if (option == 0)
        {
            // Sair
            strcpy(message, "exit");
            send(sock, message, strlen(message), 0);
            break;
        }

        switch (option)
        {
        case 1:
        {
            // Cadastrar um novo filme
            char title[256], genres[512], director[256];
            int year;

            printf("Título do filme: ");
            fgets(title, sizeof(title), stdin);
            title[strcspn(title, "\n")] = 0; // Remove a quebra de linha

            printf("Gêneros (separados por vírgula): ");
            fgets(genres, sizeof(genres), stdin);
            genres[strcspn(genres, "\n")] = 0;

            printf("Diretor: ");
            fgets(director, sizeof(director), stdin);
            director[strcspn(director, "\n")] = 0;

            printf("Ano de lançamento: ");
            scanf("%d", &year);
            getchar(); // Consome o caractere de nova linha

            // Formata a mensagem
            sprintf(message, "1;%s;%s;%s;%d", title, genres, director, year);
            break;
        }
        case 2:
        {
            // Adicionar um novo gênero a um filme
            int id;
            char genre[100];

            printf("ID do filme: ");
            scanf("%d", &id);
            getchar(); // Consome o caractere de nova linha

            printf("Gênero a adicionar: ");
            fgets(genre, sizeof(genre), stdin);
            genre[strcspn(genre, "\n")] = 0;

            // Formata a mensagem
            sprintf(message, "2;%d;%s", id, genre);
            break;
        }
        case 3:
        {
            // Remover um filme pelo identificador
            int id;

            printf("ID do filme a ser removido: ");
            scanf("%d", &id);
            getchar(); // Consome o caractere de nova linha

            // Formata a mensagem
            sprintf(message, "3;%d", id);
            break;
        }
        case 4:
            // Listar todos os títulos de filmes
            strcpy(message, "4");
            break;
        case 5:
            // Listar informações de todos os filmes
            strcpy(message, "5");
            break;
        case 6:
        {
            // Listar informações de um filme específico
            int id;

            printf("ID do filme: ");
            scanf("%d", &id);
            getchar(); // Consome o caractere de nova linha

            // Formata a mensagem
            sprintf(message, "6;%d", id);
            break;
        }
        case 7:
        {
            // Listar todos os filmes de um determinado gênero
            char genre[100];

            printf("Gênero: ");
            fgets(genre, sizeof(genre), stdin);
            genre[strcspn(genre, "\n")] = 0;

            // Formata a mensagem
            sprintf(message, "7;%s", genre);
            break;
        }
        default:
            printf("Opção inválida!\n");
            continue;
        }

        // Envia a mensagem ao servidor
        send(sock, message, strlen(message), 0);

        // Limpa o buffer
        memset(buffer, 0, BUFFER_SIZE);

        // Recebe a resposta do servidor
        read(sock, buffer, BUFFER_SIZE);
        printf("\n%s\n", buffer);

        // Aguarda o usuário pressionar Enter para continuar
        printf("\nPressione Enter para continuar...");
        getchar();
    }

    // Fecha o socket
    close(sock);
    printf("Conexão encerrada\n");

    return 0;
}