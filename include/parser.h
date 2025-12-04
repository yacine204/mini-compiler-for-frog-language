#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include "token.h"
#include "symbol.h"
#include "error.h"

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} OutputBuffer;

typedef struct {
    TokenList *tokens;
    int position;
    SymbolTable *symbolTable;
    ErrorList *errors;
    OutputBuffer *output;
} Parser;

void init_output_buffer(OutputBuffer *buffer);
void free_output_buffer(OutputBuffer *buffer);
char *detach_output_buffer(OutputBuffer *buffer);

void init_parser(Parser *parser, TokenList *tokens, SymbolTable *symbolTable, ErrorList *errors, OutputBuffer *output);
void parse(Parser *parser);

#endif