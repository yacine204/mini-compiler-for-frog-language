#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>

typedef enum {
    NONE,
    KEYWORD_BEGIN,       // FRG_Begin
    KEYWORD_END,         // FRG_End
    KEYWORD_INT,
    KEYWORD_REAL,
    KEYWORD_STRING,
    KEYWORD_PRINT,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_REPEAT,
    KEYWORD_UNTIL,
    BLOCK_BEGIN,         // Begin
    BLOCK_END,           // End
    IDENTIFIER,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    ASSIGN_OP,
    END_INSTRUCTION,
    COMMENT,
    COMMA,
    OPEN_BRACKET,
    CLOSE_BRACKET,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPERATOR_PLUS,
    OPERATOR_MINUS,
    OPERATOR_MULTIPLY,
    OPERATOR_DIVIDE,
    RELATIONAL_OP

} TokenType;

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