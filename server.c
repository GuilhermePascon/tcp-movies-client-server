#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "json_operations.h"
#include <asm-generic/socket.h>

#define PORT 49153
#define BUFFER_SIZE 4096

// Função para tratar as requisições do cliente
void process_request(char *request, char *response);

// Função executada por cada thread para atender um cliente
void *handle_client(void *client_socket);

int main()
{
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Cria o socket do servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configura o socket para reutilizar endereço e porta
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("Falha na configuração do socket");
        exit(EXIT_FAILURE);
    }

    // Configura o endereço do servidor
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Associa o socket ao endereço e porta
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Falha ao fazer bind");
        exit(EXIT_FAILURE);
    }

    // Configura o socket para escutar conexões
    if (listen(server_fd, 10) < 0)
    {
        perror("Falha ao escutar");
        exit(EXIT_FAILURE);
    }

    printf("Servidor iniciado. Aguardando conexões na porta %d...\n", PORT);

    // Loop principal para aceitar conexões
    while (1)
    {
        // Aceita uma nova conexão
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Falha ao aceitar conexão");
            continue;
        }

        // Cria uma nova thread para lidar com o cliente
        pthread_t thread_id;
        int *pclient = malloc(sizeof(int));
        *pclient = client_socket;

        if (pthread_create(&thread_id, NULL, handle_client, pclient) != 0)
        {
            perror("Falha ao criar thread");
            close(client_socket);
            free(pclient);
        }
        else
        {
            // Desvincula a thread para que ela seja automaticamente limpa ao terminar
            pthread_detach(thread_id);
        }
    }

    return 0;
}

void *handle_client(void *arg)
{
    int client_socket = *((int *)arg);
    free(arg);

    char buffer[BUFFER_SIZE] = {0};
    char response[BUFFER_SIZE] = {0};

    // Loop para receber e processar mensagens do cliente
    while (1)
    {
        // Limpa o buffer
        memset(buffer, 0, BUFFER_SIZE);

        // Recebe dados do cliente
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (bytes_read <= 0)
        {
            // Cliente desconectou ou erro de leitura
            break;
        }

        // Processa a requisição
        process_request(buffer, response);

        // Envia a resposta ao cliente
        send(client_socket, response, strlen(response), 0);

        // Se o cliente enviar "exit", encerra a conexão
        if (strncmp(buffer, "exit", 4) == 0)
        {
            break;
        }
    }

    // Fecha o socket do cliente
    close(client_socket);
    printf("Cliente desconectado\n");

    return NULL;
}

void process_request(char *request, char *response)
{
    // Inicializa a resposta
    memset(response, 0, BUFFER_SIZE);

    // Tokeniza a requisição para obter o comando e os parâmetros
    char *command = strtok(request, ";");
    if (!command)
    {
        strcpy(response, "Erro: comando inválido");
        return;
    }

    // Processa o comando
    if (strcmp(command, "1") == 0)
    {
        // Cadastrar um novo filme
        char *title = strtok(NULL, ";");
        char *genres = strtok(NULL, ";");
        char *director = strtok(NULL, ";");
        char *year_str = strtok(NULL, ";");

        if (!title || !genres || !director || !year_str)
        {
            strcpy(response, "Erro: parâmetros insuficientes");
            return;
        }

        int year = atoi(year_str);
        if (year <= 0)
        {
            strcpy(response, "Erro: ano inválido");
            return;
        }

        int id = add_movie(title, genres, director, year);
        sprintf(response, "Filme cadastrado com sucesso. ID: %d", id);
    }
    else if (strcmp(command, "2") == 0)
    {
        // Adicionar um novo gênero a um filme
        char *id_str = strtok(NULL, ";");
        char *genre = strtok(NULL, ";");

        if (!id_str || !genre)
        {
            strcpy(response, "Erro: parâmetros insuficientes");
            return;
        }

        int id = atoi(id_str);
        if (id <= 0)
        {
            strcpy(response, "Erro: ID inválido");
            return;
        }

        int success = add_genre_to_movie(id, genre);
        if (success)
        {
            sprintf(response, "Gênero '%s' adicionado ao filme ID %d", genre, id);
        }
        else
        {
            sprintf(response, "Erro: filme ID %d não encontrado ou gênero já existente", id);
        }
    }
    else if (strcmp(command, "3") == 0)
    {
        // Remover um filme pelo identificador
        char *id_str = strtok(NULL, ";");

        if (!id_str)
        {
            strcpy(response, "Erro: ID não fornecido");
            return;
        }

        int id = atoi(id_str);
        if (id <= 0)
        {
            strcpy(response, "Erro: ID inválido");
            return;
        }

        int success = remove_movie(id);
        if (success)
        {
            sprintf(response, "Filme ID %d removido com sucesso", id);
        }
        else
        {
            sprintf(response, "Erro: filme ID %d não encontrado", id);
        }
    }
    else if (strcmp(command, "4") == 0)
    {
        // Listar todos os títulos de filmes com seus identificadores
        char *titles = list_all_titles();
        if (titles)
        {
            strcpy(response, titles);
            free(titles);
        }
        else
        {
            strcpy(response, "Erro ao listar títulos");
        }
    }
    else if (strcmp(command, "5") == 0)
    {
        // Listar informações de todos os filmes
        char *movies = list_all_movies();
        if (movies)
        {
            strcpy(response, movies);
            free(movies);
        }
        else
        {
            strcpy(response, "Erro ao listar filmes");
        }
    }
    else if (strcmp(command, "6") == 0)
    {
        // Listar informações de um filme específico
        char *id_str = strtok(NULL, ";");

        if (!id_str)
        {
            strcpy(response, "Erro: ID não fornecido");
            return;
        }

        int id = atoi(id_str);
        if (id <= 0)
        {
            strcpy(response, "Erro: ID inválido");
            return;
        }

        char *movie = get_movie_by_id(id);
        if (movie)
        {
            strcpy(response, movie);
            free(movie);
        }
        else
        {
            strcpy(response, "Erro ao buscar filme");
        }
    }
    else if (strcmp(command, "7") == 0)
    {
        // Listar todos os filmes de um determinado gênero
        char *genre = strtok(NULL, ";");

        if (!genre)
        {
            strcpy(response, "Erro: gênero não fornecido");
            return;
        }

        char *movies = list_movies_by_genre(genre);
        if (movies)
        {
            strcpy(response, movies);
            free(movies);
        }
        else
        {
            strcpy(response, "Erro ao listar filmes por gênero");
        }
    }
    else if (strcmp(command, "help") == 0)
    {
        // Exibe ajuda com os comandos disponíveis
        strcpy(response, "Comandos disponíveis:\n\n"
                         "1;título;gêneros;diretor;ano - Cadastrar novo filme\n"
                         "2;id;gênero - Adicionar gênero a um filme\n"
                         "3;id - Remover filme\n"
                         "4 - Listar todos os títulos de filmes\n"
                         "5 - Listar informações de todos os filmes\n"
                         "6;id - Listar informações de um filme específico\n"
                         "7;gênero - Listar todos os filmes de um gênero\n"
                         "exit - Encerrar conexão\n");
    }
    else
    {
        strcpy(response, "Comando não reconhecido. Digite 'help' para ver os comandos disponíveis.");
    }
}