#ifndef DB_OPERATIONS_H
#define DB_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jansson.h>

// Função para obter o próximo ID disponível
int get_next_id();

// Funções para as operações de escrita
int add_movie(const char *title, const char *genres, const char *director, int year);
int add_genre_to_movie(int id, const char *genre);
int remove_movie(int id);

// Funções para as operações de leitura
char* list_all_titles();
char* list_all_movies();
char* get_movie_by_id(int id);
char* list_movies_by_genre(const char *genre);

// Funções auxiliares
void db_lock();
void db_unlock();

#endif