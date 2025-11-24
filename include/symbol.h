#ifndef SYMBOL_H
#define SYMBOL_H

#include "token.h"  
#include <stdlib.h>
#include <string.h>

typedef  enum{
    KEY_INT,
    KEY_REAL,
    KEY_STRING,
    KEY_UNKNOWN 
}SymbolType;


typedef struct{
    SymbolType type;
    char *id;
    char *value;
    int line_declared;
}Symbol;


typedef struct{
    Symbol *symbols;
    int count;
    int capacity;
}SymbolTable;

Symbol create_symbol(const char *name, SymbolType type, int line_declared);
void add_symbol(SymbolTable *table, Symbol symbol);
Symbol* findSymbol(SymbolTable *table, const char *id);
void free_symbol_table(SymbolTable *table);

#endif


