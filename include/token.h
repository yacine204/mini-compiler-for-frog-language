#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>

typedef enum{
    KEYWORD_BEGIN,
    KEYWORD_END,
    KEYWORD_INT,
    KEYWORD_REAL,
    KEYWORD_STRING,
    IDENTIFIER,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    ASSIGN_OP,
    END_INSTRUCTION,
    COMMENT,
    CONDITION,
    ELSE,
    REPEAT,
    UNTIL
    
}TokenType;

typedef struct{
    TokenType type; //type of token
    char *value; //actuall text of the token
    int line; //located at number line in source code
}Token;


typedef struct 
{
    Token *tokens; //pointer to array of tokens]
    int count; //number of tokens
    int capacity; // capacity of the array  
} TokenList;

Token create_token(TokenType type, const char *value, int line);
void add_token(TokenList *list, Token token);
void free_token_list(TokenList *list);

#endif