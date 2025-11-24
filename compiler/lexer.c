#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/token.h"
#include "../include/error.h"
#include "../include/lexer.h"

void lexer(char *filePath, TokenList *tokenList, ErrorList *errorList){
    FILE *f = fopen(filePath, "r");
    if(f == NULL){
        printf("Error: Cannot open file %s\n", filePath);
        return;
    }
    
    char line[512];
    int line_number = 1;
    
    while(fgets(line, sizeof(line), f) != NULL){
        int i = 0;
        int len = strlen(line);
        
        // Skip empty lines
        if(len == 0){
            line_number++;
            continue;
        }
        
        // Check for comment (# at start of line or after only whitespace)
        int is_comment = 0;
        for(int j = 0; j < len; j++){
            if(line[j] == '#'){
                // Check if this # is at the start or only whitespace before it
                int only_whitespace_before = 1;
                for(int k = 0; k < j; k++){
                    if(line[k] != ' ' && line[k] != '\t'){
                        only_whitespace_before = 0;
                        break;
                    }
                }
                if(only_whitespace_before){
                    Token comment = create_token(COMMENT, line, line_number);
                    add_token(tokenList, comment);
                    is_comment = 1;
                    break;
                }
            }
        }
        
        if(is_comment){
            line_number++;
            continue;
        }
        
        // Process the line character by character
        while(i < len){
            char c = line[i];
            
            // Skip whitespace
            if(c == ' ' || c == '\t' || c == '\r' || c == '\n'){
                i++;
                continue;
            }
            

            if(isalpha(c) || c == '_'){
                char buffer[256] = {0};
                int j = 0;
                
                // Read identifier: starts with letter/underscore, can contain letters, digits, underscores
                while(i < len && (isalnum(line[i]) || line[i] == '_')){
                    buffer[j++] = line[i++];
                }
                buffer[j] = '\0';
                
                // Check for keywords
                Token token;
                if(strcmp(buffer, "FRG_Begin") == 0){
                    token = create_token(KEYWORD_BEGIN, buffer, line_number);
                } else if(strcmp(buffer, "FRG_End") == 0){
                    token = create_token(KEYWORD_END, buffer, line_number);
                } else if(strcmp(buffer, "FRG_Int") == 0){
                    token = create_token(KEYWORD_INT, buffer, line_number);
                } else if(strcmp(buffer, "FRG_Real") == 0){
                    token = create_token(KEYWORD_REAL, buffer, line_number);
                } else if(strcmp(buffer, "FRG_Strg") == 0){
                    token = create_token(KEYWORD_STRING, buffer, line_number);
                } else if(strcmp(buffer, "If") == 0){
                    token = create_token(CONDITION, buffer, line_number);
                } else if(strcmp(buffer, "FRG_Else") == 0){
                    token = create_token(ELSE, buffer, line_number);
                } else if(strcmp(buffer, "Repeat") == 0){
                    token = create_token(REPEAT, buffer, line_number);
                } else if(strcmp(buffer, "Until") == 0){
                    token = create_token(UNTIL, buffer, line_number);
                } else {
                    // It's an identifier (variable name)
                    token = create_token(IDENTIFIER, buffer, line_number);
                }
                add_token(tokenList, token);
                continue;
            }
            
            // Check for numbers (integer or float)
            if(isdigit(c)){
                char buffer[256] = {0};
                int j = 0;
                int has_dot = 0;
                
                while(i < len && (isdigit(line[i]) || line[i] == '.')){
                    if(line[i] == '.'){
                        if(has_dot){
                            // Error: multiple dots in number
                            Error err = create_error(LEXICAL_ERR, 
                                "Multiple decimal points in number", line_number);
                            add_error(errorList, err);
                            break;
                        }
                        has_dot = 1;
                    }
                    buffer[j++] = line[i++];
                }
                buffer[j] = '\0';
                
                Token token;
                if(has_dot){
                    token = create_token(FLOAT_LITERAL, buffer, line_number);
                } else {
                    token = create_token(INTEGER_LITERAL, buffer, line_number);
                }
                add_token(tokenList, token);
                continue;
            }
            
            // Check for string literal 
            if(c == '"'){
                char buffer[512] = {0};
                int j = 0;
                i++; // Skip opening quote
                
                // Read until closing quote or end of line
                while(i < len && line[i] != '"' && line[i] != '\n'){
                    buffer[j++] = line[i++];
                }
                
                if(i >= len || line[i] != '"'){
                    Error err = create_error(LEXICAL_ERR, 
                        "Unterminated string literal", line_number);
                    add_error(errorList, err);
                } else {
                    i++; 
                    buffer[j] = '\0';
                    Token token = create_token(STRING_LITERAL, buffer, line_number);
                    add_token(tokenList, token);
                }
                continue;
            }
            
            
            if(c == ':' && i + 1 < len && line[i + 1] == '='){
                Token token = create_token(ASSIGN_OP, ":=", line_number);
                add_token(tokenList, token);
                i += 2;
                continue;
            }
            
            
            if(c == '#'){
                Token token = create_token(END_INSTRUCTION, "#", line_number);
                add_token(tokenList, token);
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