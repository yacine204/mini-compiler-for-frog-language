#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "symbol.h"
#include "error.h"

typedef struct {
    TokenList *tokens;
    int position;
    SymbolTable *symbolTable;
    ErrorList *errors;
} Parser;

void init_parser(Parser *parser, TokenList *tokens, SymbolTable *symbolTable, ErrorList *errors);
void parse(Parser *parser);

#endif