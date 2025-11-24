#include "../include/symbol.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

Symbol create_symbol(const char *name, SymbolType type, int line_declared){
    Symbol symbol;
    symbol.id = malloc(strlen(name)+1);
    strcpy(symbol.id, name);
    symbol.type = type;
    symbol.line_declared = line_declared;
    return symbol;
};


void add_symbol(SymbolTable *table, Symbol symbol){
    if(table->capacity == 0){
        table->capacity = 10;
        table->count = 0;
        table->symbols = malloc(sizeof(Symbol) * table->capacity);
    }
    if(table->count >= table->capacity){
        table->capacity *= 2;
        table->symbols = realloc(table->symbols, sizeof(Symbol) * table->capacity);
    }
    table->symbols[table->count++] = symbol;
};

Symbol* findSymbol(SymbolTable *table, const char *id){

    for(int i=0 ; i < table->count; i++){
        if(strcmp(table->symbols[i].id, id) == 0)
        return &table->symbols[i];
    }
    return NULL;
};

void free_symbol_table(SymbolTable *table){
    for(int i=0 ; i< table->count; i++)
        free(table->symbols[i].id);
    free(table->symbols);
    free(table);
};