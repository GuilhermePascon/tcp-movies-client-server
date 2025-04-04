#include "json_operations.h"

#define DB_FILE "movies.json"

// Mutex para sincronização de acesso ao banco de dados
pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

// Carrega o banco de dados de um arquivo JSON
json_t* load_database() {
    json_error_t error;
    json_t *root = json_load_file(DB_FILE, 0, &error);
    if (!root) {
        // Se o arquivo não existe ou está vazio, cria um novo banco de dados
        root = json_pack("{s:[], s:i}", "movies", "last_id", 0);
        json_dump_file(root, DB_FILE, JSON_INDENT(2));
    }
    return root;
}

// Salva o banco de dados em um arquivo JSON
void save_database(json_t *root) {
    if (json_dump_file(root, DB_FILE, JSON_INDENT(2)) != 0) {
        fprintf(stderr, "Erro ao salvar o banco de dados\n");
    }
}

// Bloqueia o acesso ao banco de dados
void db_lock() {
    pthread_mutex_lock(&db_mutex);
}

// Desbloqueia o acesso ao banco de dados
void db_unlock() {
    pthread_mutex_unlock(&db_mutex);
}

// Obtém o próximo ID disponível
int get_next_id() {
    int next_id = 0;
    json_t *root = load_database();
    json_t *last_id = json_object_get(root, "last_id");
    
    if (last_id) {
        next_id = json_integer_value(last_id) + 1;
    }
    
    json_decref(root);
    return next_id;
}

// Adiciona um novo filme ao banco de dados
int add_movie(const char *title, const char *genres, const char *director, int year) {
    db_lock();
    
    json_t *root = load_database();
    json_t *movies = json_object_get(root, "movies");
    json_t *last_id_json = json_object_get(root, "last_id");
    
    int last_id = 0;
    if (json_is_integer(last_id_json)) {
        last_id = json_integer_value(last_id_json);
    }
    
    int new_id = last_id + 1;
    
    // Cria o novo filme
    json_t *new_movie = json_object();
    json_object_set_new(new_movie, "id", json_integer(new_id));
    json_object_set_new(new_movie, "title", json_string(title));
    
    // Processa os gêneros
    json_t *genres_array = json_array();
    char *genres_copy = strdup(genres);
    char *token = strtok(genres_copy, ",");
    while (token) {
        // Remove espaços extras
        while (*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') *end-- = '\0';
        
        json_array_append_new(genres_array, json_string(token));
        token = strtok(NULL, ",");
    }
    free(genres_copy);
    
    json_object_set_new(new_movie, "genres", genres_array);
    json_object_set_new(new_movie, "director", json_string(director));
    json_object_set_new(new_movie, "year", json_integer(year));
    
    // Adiciona o filme ao array de filmes
    json_array_append_new(movies, new_movie);
    
    // Atualiza o último ID
    json_object_set_new(root, "last_id", json_integer(new_id));
    
    // Salva o banco de dados
    save_database(root);
    json_decref(root);
    
    db_unlock();
    return new_id;
}

// Adiciona um novo gênero a um filme existente
int add_genre_to_movie(int id, const char *genre) {
    db_lock();
    int success = 0;
    
    json_t *root = load_database();
    json_t *movies = json_object_get(root, "movies");
    
    // Procura o filme pelo ID
    size_t index;
    json_t *movie;
    json_array_foreach(movies, index, movie) {
        json_t *movie_id = json_object_get(movie, "id");
        if (json_integer_value(movie_id) == id) {
            // Encontrou o filme, adiciona o gênero
            json_t *genres = json_object_get(movie, "genres");
            
            // Verifica se o gênero já existe
            int genre_exists = 0;
            size_t i;
            json_t *existing_genre;
            json_array_foreach(genres, i, existing_genre) {
                if (strcmp(json_string_value(existing_genre), genre) == 0) {
                    genre_exists = 1;
                    break;
                }
            }
            
            if (!genre_exists) {
                json_array_append_new(genres, json_string(genre));
                success = 1;
            }
            break;
        }
    }
    
    // Salva o banco de dados se houve alteração
    if (success) {
        save_database(root);
    }
    
    json_decref(root);
    db_unlock();
    return success;
}

// Remove um filme pelo ID
int remove_movie(int id) {
    db_lock();
    int success = 0;
    
    json_t *root = load_database();
    json_t *movies = json_object_get(root, "movies");
    json_t *new_movies = json_array();
    
    // Copia todos os filmes, exceto o que será removido
    size_t index;
    json_t *movie;
    json_array_foreach(movies, index, movie) {
        json_t *movie_id = json_object_get(movie, "id");
        if (json_integer_value(movie_id) != id) {
            json_array_append(new_movies, movie);
        } else {
            success = 1;
        }
    }
    
    // Substitui o array de filmes
    json_object_set_new(root, "movies", new_movies);
    
    // Salva o banco de dados
    if (success) {
        save_database(root);
    }
    
    json_decref(root);
    db_unlock();
    return success;
}

// Lista todos os títulos de filmes com seus identificadores
char* list_all_titles() {
    db_lock();
    
    json_t *root = load_database();
    json_t *movies = json_object_get(root, "movies");
    
    // Aloca espaço para a resposta
    char *response = malloc(10240);
    if (!response) {
        json_decref(root);
        db_unlock();
        return NULL;
    }
    
    sprintf(response, "ID | Título\n-------------------\n");
    
    // Itera sobre os filmes e adiciona os títulos à resposta
    size_t index;
    json_t *movie;
    json_array_foreach(movies, index, movie) {
        json_t *id = json_object_get(movie, "id");
        json_t *title = json_object_get(movie, "title");
        
        char line[512];
        sprintf(line, "%d | %s\n", (int)json_integer_value(id), json_string_value(title));
        strcat(response, line);
    }
    
    json_decref(root);
    db_unlock();
    return response;
}

// Lista informações de todos os filmes
char* list_all_movies() {
    db_lock();
    
    json_t *root = load_database();
    json_t *movies = json_object_get(root, "movies");
    
    // Aloca espaço para a resposta
    char *response = malloc(51200);
    if (!response) {
        json_decref(root);
        db_unlock();
        return NULL;
    }
    
    strcpy(response, "Lista de Filmes:\n================\n");
    
    // Itera sobre os filmes e adiciona as informações à resposta
    size_t index;
    json_t *movie;
    json_array_foreach(movies, index, movie) {
        json_t *id = json_object_get(movie, "id");
        json_t *title = json_object_get(movie, "title");
        json_t *genres = json_object_get(movie, "genres");
        json_t *director = json_object_get(movie, "director");
        json_t *year = json_object_get(movie, "year");
        
        char movie_info[2048];
        sprintf(movie_info, "\nID: %d\nTítulo: %s\nDiretor: %s\nAno: %d\nGêneros: ", 
                (int)json_integer_value(id), 
                json_string_value(title),
                json_string_value(director),
                (int)json_integer_value(year));
        
        // Processa os gêneros
        size_t i;
        json_t *genre;
        json_array_foreach(genres, i, genre) {
            if (i > 0) strcat(movie_info, ", ");
            strcat(movie_info, json_string_value(genre));
        }
        
        strcat(movie_info, "\n");
        strcat(response, movie_info);
    }
    
    json_decref(root);
    db_unlock();
    return response;
}

// Busca um filme pelo ID
char* get_movie_by_id(int id) {
    db_lock();
    
    json_t *root = load_database();
    json_t *movies = json_object_get(root, "movies");
    
    char *response = malloc(2048);
    if (!response) {
        json_decref(root);
        db_unlock();
        return NULL;
    }
    
    strcpy(response, "");
    
    // Procura o filme pelo ID
    size_t index;
    json_t *movie;
    json_array_foreach(movies, index, movie) {
        json_t *movie_id = json_object_get(movie, "id");
        if (json_integer_value(movie_id) == id) {
            json_t *title = json_object_get(movie, "title");
            json_t *genres = json_object_get(movie, "genres");
            json_t *director = json_object_get(movie, "director");
            json_t *year = json_object_get(movie, "year");
            
            sprintf(response, "ID: %d\nTítulo: %s\nDiretor: %s\nAno: %d\nGêneros: ", 
                    id, 
                    json_string_value(title),
                    json_string_value(director),
                    (int)json_integer_value(year));
            
            // Processa os gêneros
            size_t i;
            json_t *genre;
            json_array_foreach(genres, i, genre) {
                if (i > 0) strcat(response, ", ");
                strcat(response, json_string_value(genre));
            }
            
            break;
        }
    }
    
    if (strlen(response) == 0) {
        strcpy(response, "Filme não encontrado");
    }
    
    json_decref(root);
    db_unlock();
    return response;
}

// Lista todos os filmes de um determinado gênero
char* list_movies_by_genre(const char *genre) {
    db_lock();
    
    json_t *root = load_database();
    json_t *movies = json_object_get(root, "movies");
    
    char *response = malloc(10240);
    if (!response) {
        json_decref(root);
        db_unlock();
        return NULL;
    }
    
    sprintf(response, "Filmes do gênero '%s':\n===================\n", genre);
    
    // Itera sobre os filmes e procura pelo gênero
    size_t index;
    json_t *movie;
    int found = 0;
    
    json_array_foreach(movies, index, movie) {
        json_t *genres = json_object_get(movie, "genres");
        
        // Verifica se o gênero está presente
        size_t i;
        json_t *g;
        int genre_found = 0;
        
        json_array_foreach(genres, i, g) {
            if (strcasecmp(json_string_value(g), genre) == 0) {
                genre_found = 1;
                break;
            }
        }
        
        if (genre_found) {
            found = 1;
            json_t *id = json_object_get(movie, "id");
            json_t *title = json_object_get(movie, "title");
            json_t *director = json_object_get(movie, "director");
            json_t *year = json_object_get(movie, "year");
            
            char movie_info[1024];
            sprintf(movie_info, "\nID: %d\nTítulo: %s\nDiretor: %s\nAno: %d\n", 
                    (int)json_integer_value(id), 
                    json_string_value(title),
                    json_string_value(director),
                    (int)json_integer_value(year));
            
            strcat(response, movie_info);
        }
    }
    
    if (!found) {
        strcat(response, "\nNenhum filme encontrado com esse gênero.\n");
    }
    
    json_decref(root);
    db_unlock();
    return response;
}