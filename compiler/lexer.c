#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/token.h"
#include "../include/error.h"
#include "../include/lexer.h"

static int equals_ignore_case(const char *a, const char *b) {
    while(*a && *b) {
        if(tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static void emit_token(TokenList *tokenList, TokenType type, const char *lexeme, int line, TokenType *last_type) {
    Token token = create_token(type, lexeme, line);
    add_token(tokenList, token);
    if(last_type != NULL) {
        *last_type = type;
    }
}

static int is_comment_line(const char *line) {
    int idx = 0;
    while(line[idx] != '\0' && (line[idx] == ' ' || line[idx] == '\t')) {
        idx++;
    }

    if(line[idx] == '#' && line[idx + 1] == '#') {
        return 1;
    }
    return 0;
}

void lexer(char *filePath, TokenList *tokenList, ErrorList *errorList){
    FILE *f = fopen(filePath, "r");
    if(f == NULL){
        printf("Error: Cannot open file %s\n", filePath);
        return;
    }
    
    char line[512];
    int line_number = 1;
    TokenType last_type = NONE;
    
    while(fgets(line, sizeof(line), f) != NULL){
        int len = strlen(line);
        while(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')){
            line[--len] = '\0';
        }

        if(len == 0){
            line_number++;
            continue;
        }

        if(is_comment_line(line)){
            // Store the comment text starting at the first '#'
            const char *comment_start = strchr(line, '#');
            emit_token(tokenList, COMMENT, comment_start ? comment_start : line, line_number, &last_type);
            line_number++;
            continue;
        }

        int i = 0;
        while(i < len){
            char c = line[i];

            if(c == ' ' || c == '\t'){
                i++;
                continue;
            }

            if(isalpha((unsigned char)c) || c == '_'){
                char buffer[256] = {0};
                int j = 0;

                while(i < len && (isalnum((unsigned char)line[i]) || line[i] == '_')){
                    if(j < (int)sizeof(buffer) - 1){
                        buffer[j++] = line[i];
                    }
                    i++;
                }
                buffer[j] = '\0';

                if(equals_ignore_case(buffer, "FRG_Begin")){
                    emit_token(tokenList, KEYWORD_BEGIN, buffer, line_number, &last_type);
                } else if(equals_ignore_case(buffer, "FRG_End")){
                    emit_token(tokenList, KEYWORD_END, buffer, line_number, &last_type);
                } else if(equals_ignore_case(buffer, "FRG_Int")){
                    emit_token(tokenList, KEYWORD_INT, buffer, line_number, &last_type);
                } else if(equals_ignore_case(buffer, "FRG_Real")){
                    emit_token(tokenList, KEYWORD_REAL, buffer, line_number, &last_type);
                } else if(equals_ignore_case(buffer, "FRG_Strg")){
                    emit_token(tokenList, KEYWORD_STRING, buffer, line_number, &last_type);
                } else if(equals_ignore_case(buffer, "FRG_Print")){
                    emit_token(tokenList, KEYWORD_PRINT, buffer, line_number, &last_type);
                } else if(equals_ignore_case(buffer, "If")){
                    emit_token(tokenList, KEYWORD_IF, buffer, line_number, &last_type);
                } else if(equals_ignore_case(buffer, "Else")){
                    emit_token(tokenList, KEYWORD_ELSE, buffer, line_number, &last_type);
                } else if(equals_ignore_case(buffer, "Repeat")){
                    emit_token(tokenList, KEYWORD_REPEAT, buffer, line_number, &last_type);
                } else if(equals_ignore_case(buffer, "Until")){
                    emit_token(tokenList, KEYWORD_UNTIL, buffer, line_number, &last_type);
                } else if(equals_ignore_case(buffer, "Begin")){
                    emit_token(tokenList, BLOCK_BEGIN, buffer, line_number, &last_type);
                } else if(equals_ignore_case(buffer, "End")){
                    emit_token(tokenList, BLOCK_END, buffer, line_number, &last_type);
                } else {
                    emit_token(tokenList, IDENTIFIER, buffer, line_number, &last_type);
                }
                continue;
            }

            if(isdigit((unsigned char)c)){
                char buffer[256] = {0};
                int j = 0;
                int has_dot = 0;

                while(i < len && (isdigit((unsigned char)line[i]) || line[i] == '.')){
                    if(line[i] == '.'){
                        if(has_dot){
                            Error err = create_error(LEXICAL_ERR, "Multiple decimal points in number", line_number);
                            add_error(errorList, err);
                            break;
                        }
                        has_dot = 1;
                    }
                    if(j < (int)sizeof(buffer) - 1){
                        buffer[j++] = line[i];
                    }
                    i++;
                }
                buffer[j] = '\0';

                if(has_dot){
                    emit_token(tokenList, FLOAT_LITERAL, buffer, line_number, &last_type);
                } else {
                    emit_token(tokenList, INTEGER_LITERAL, buffer, line_number, &last_type);
                }
                continue;
            }

            if(c == '"'){
                char buffer[512] = {0};
                int j = 0;
                i++;
                while(i < len && line[i] != '"'){
                    if(j < (int)sizeof(buffer) - 1){
                        buffer[j++] = line[i];
                    }
                    i++;
                }

                if(i >= len || line[i] != '"'){
                    Error err = create_error(LEXICAL_ERR, "Unterminated string literal", line_number);
                    add_error(errorList, err);
                } else {
                    i++; // skip closing quote
                    emit_token(tokenList, STRING_LITERAL, buffer, line_number, &last_type);
                }
                continue;
            }

            if(c == ':' && i + 1 < len && line[i + 1] == '='){
                emit_token(tokenList, ASSIGN_OP, ":=", line_number, &last_type);
                i += 2;
                continue;
            }

            if(c == ','){
                emit_token(tokenList, COMMA, ",", line_number, &last_type);
                i++;
                continue;
            }

            if(c == '['){
                emit_token(tokenList, OPEN_BRACKET, "[", line_number, &last_type);
                i++;
                continue;
            }

            if(c == ']'){
                emit_token(tokenList, CLOSE_BRACKET, "]", line_number, &last_type);
                i++;
                continue;
            }

            if(c == '('){
                emit_token(tokenList, OPEN_PAREN, "(", line_number, &last_type);
                i++;
                continue;
            }

            if(c == ')'){
                emit_token(tokenList, CLOSE_PAREN, ")", line_number, &last_type);
                i++;
                continue;
            }

            if(c == '+'){
                emit_token(tokenList, OPERATOR_PLUS, "+", line_number, &last_type);
                i++;
                continue;
            }

            if(c == '-'){
                emit_token(tokenList, OPERATOR_MINUS, "-", line_number, &last_type);
                i++;
                continue;
            }

            if(c == '*'){
                emit_token(tokenList, OPERATOR_MULTIPLY, "*", line_number, &last_type);
                i++;
                continue;
            }

            if(c == '/'){
                emit_token(tokenList, OPERATOR_DIVIDE, "/", line_number, &last_type);
                i++;
                continue;
            }

            if(c == '<' || c == '>' || c == '=' || c == '!'){
                char op[3] = {c, '\0', '\0'};
                if(i + 1 < len && line[i + 1] == '='){
                    op[1] = '=';
                    op[2] = '\0';
                    i += 2;
                } else {
                    if(c == '!'){
                        char msg[128];
                        sprintf(msg, "Unknown operator '!'");
                        Error err = create_error(LEXICAL_ERR, msg, line_number);
                        add_error(errorList, err);
                        i++;
                        continue;
                    }
                    i++;
                }
                emit_token(tokenList, RELATIONAL_OP, op, line_number, &last_type);
                continue;
            }

            if(c == '#'){
                if(last_type != KEYWORD_BEGIN){
                    emit_token(tokenList, END_INSTRUCTION, "#", line_number, &last_type);
                }
                i++;
                continue;
            }

            char error_msg[256];
            sprintf(error_msg, "Unknown character '%c'", c);
            Error err = create_error(LEXICAL_ERR, error_msg, line_number);
            add_error(errorList, err);
            i++;
        }

        line_number++;
    }

    fclose(f);
}