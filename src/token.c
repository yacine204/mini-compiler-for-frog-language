#include "../include/token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Token create_token(TokenType type, const char *value, int line){
    Token token;
    token.type = type;
    token.line = line;
    //allocate just enough space for the value 
    token.value = malloc(strlen(value)+1);
    strcpy(token.value, value);

    return token;
}

void add_token(TokenList *list, Token token){
    //if list is empty initialize space in memory
    if(list->tokens == NULL){
        list->capacity = 10;
        list->count = 0;
        list->tokens = malloc(sizeof(Token)* list->capacity);
    }

    if(list->count >= list->capacity){
        list->capacity *= 2;
        list->tokens = realloc(list->tokens, sizeof(Token)* list->capacity);
    }
    list->tokens[list->count++] = token;
    
}

void free_token_list(TokenList *list){
    free(list);
}