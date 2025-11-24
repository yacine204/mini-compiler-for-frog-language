#include <stdio.h>
#include "../include/error.h"
#include <string.h>
#include <stdlib.h>

Error create_error(ErrorType type, const char *message, int line){
    Error error;
    error.type = type;
    error.err_message = malloc(strlen(message)+1);
    strcpy(error.err_message, message);
    error.line = line;
    return error;
};

void add_error(ErrorList *list, Error error){
    if(list->capacity == 0) {
        list->capacity = 10;
        list-> count = 0;
        list->errors = malloc(sizeof(Error) * list->capacity);
    }

    if(list->count >= list->capacity){
        list -> capacity *= 2;
        list->errors = realloc(list->errors, sizeof(Error) * list->capacity);
    }
    list->errors[list->count++] = error;
};
void free_error_list(ErrorList *list){
    for(int i=0 ; i < list->capacity ; i++){
        free(list->errors[i].err_message);
    }

    free(list->errors);
    free(list);
};


void print_errors(ErrorList *list, ErrorType type){
    for(int i = 0; i < list->count; i++){
        if(list->errors[i].type == type){
            const char *type_str = "";
            switch(type){
                case SYNTAX_ERR: type_str = "Syntax"; break;
                case LEXICAL_ERR: type_str = "Lexical"; break;
                case SEMANTIC_ERR: type_str = "Semantic"; break;
            }
            printf("Error (%s) [Line %d]: %s\n", type_str, list->errors[i].line, list->errors[i].err_message);
        }
    }
}