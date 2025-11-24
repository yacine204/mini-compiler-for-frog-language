#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "error.h"

void lexer(char *filePath, TokenList *tokenList, ErrorList *errorList);

#endif