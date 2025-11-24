#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/parser.h"
#include "../include/token.h"
#include "../include/symbol.h"
#include "../include/error.h"


void init_parser(Parser *parser, TokenList *tokens, SymbolTable *symbolTable, ErrorList *errors){
    parser->tokens = tokens;
    parser->position = 0;
    parser->symbolTable = symbolTable;
    parser->errors = errors;
}


static Token* current_token(Parser *parser){
    if(parser->position < parser->tokens->count){
        return &parser->tokens->tokens[parser->position];
    }
    // Return last token if we're at the end
    return &parser->tokens->tokens[parser->tokens->count - 1];
}

// Move to next token
static void advance(Parser *parser){
    if(parser->position < parser->tokens->count - 1){
        parser->position++;
    }
}

// Check if current token matches type, if yes advance
static int match(Parser *parser, TokenType type){
    if(current_token(parser)->type == type){
        advance(parser);
        return 1;
    }
    return 0;
}

// Expect a specific token type, error if not found
static void expect(Parser *parser, TokenType type, const char *message){
    if(current_token(parser)->type != type){
        Error err = create_error(SYNTAX_ERR, message, current_token(parser)->line);
        add_error(parser->errors, err);
    } else {
        advance(parser);
    }
}

// Convert TokenType to SymbolType
static SymbolType token_to_symbol_type(TokenType type){
    switch(type){
        case KEYWORD_INT:
            return KEY_INT;
        case KEYWORD_REAL:
            return KEY_REAL;
        case KEYWORD_STRING:
            return KEY_STRING;
        default:
            return KEY_UNKNOWN;
    }
}

// Parse variable declaration
static void parse_declaration(Parser *parser){
    Token *type_token = current_token(parser);
    
    // Check if it's a valid type keyword
    if(type_token->type != KEYWORD_INT && 
       type_token->type != KEYWORD_REAL && 
       type_token->type != KEYWORD_STRING){
        char msg[256];
        sprintf(msg, "Expected type declaration (FRG_Int, FRG_Real, or FRG_Strg)");
        Error err = create_error(SYNTAX_ERR, msg, type_token->line);
        add_error(parser->errors, err);
        return;
    }
    
    SymbolType sym_type = token_to_symbol_type(type_token->type);
    advance(parser); // Move past type keyword
    
    // Expect identifier
    Token *id_token = current_token(parser);
    if(id_token->type != IDENTIFIER){
        Error err = create_error(SYNTAX_ERR, 
            "Expected identifier after type declaration", id_token->line);
        add_error(parser->errors, err);
        return;
    }
    
    char *var_name = id_token->value;
    int line = id_token->line;
    advance(parser); // Move past identifier
    
    // Check if variable already declared (SEMANTIC ERROR)
    Symbol *existing = findSymbol(parser->symbolTable, var_name);
    if(existing != NULL){
        char msg[256];
        sprintf(msg, "Variable '%s' already declared at line %d", 
                var_name, existing->line_declared);
        Error err = create_error(SEMANTIC_ERR, msg, line);
        add_error(parser->errors, err);
    } else {
        // Add to symbol table
        Symbol sym = create_symbol(var_name, sym_type, line);
        add_symbol(parser->symbolTable, sym);
    }
    
    // Check for optional assignment
    if(current_token(parser)->type == ASSIGN_OP){
        advance(parser); // Move past :=
        
        Token *value_token = current_token(parser);
        
        // Type checking (SEMANTIC ERROR)
        if(sym_type == KEY_INT && value_token->type != INTEGER_LITERAL){
            char msg[256];
            sprintf(msg, "Type mismatch: Cannot assign %s to integer variable", 
                    value_token->value);
            Error err = create_error(SEMANTIC_ERR, msg, value_token->line);
            add_error(parser->errors, err);
        } else if(sym_type == KEY_REAL && 
                  value_token->type != FLOAT_LITERAL && 
                  value_token->type != INTEGER_LITERAL){
            char msg[256];
            sprintf(msg, "Type mismatch: Cannot assign %s to real variable", 
                    value_token->value);
            Error err = create_error(SEMANTIC_ERR, msg, value_token->line);
            add_error(parser->errors, err);
        } else if(sym_type == KEY_STRING && value_token->type != STRING_LITERAL){
            char msg[256];
            sprintf(msg, "Type mismatch: Cannot assign %s to string variable", 
                    value_token->value);
            Error err = create_error(SEMANTIC_ERR, msg, value_token->line);
            add_error(parser->errors, err);
        }
        
        // Update symbol with value
        Symbol *sym = findSymbol(parser->symbolTable, var_name);
        if(sym != NULL){
            sym->value = malloc(strlen(value_token->value) + 1);
            strcpy(sym->value, value_token->value);
        }
        
        advance(parser); // Move past value
    }
    
    // Expect # at end of instruction
    expect(parser, END_INSTRUCTION, "Expected '#' at end of instruction");
}

// Parse assignment: x := 20#
static void parse_assignment(Parser *parser){
    Token *id_token = current_token(parser);
    
    if(id_token->type != IDENTIFIER){
        Error err = create_error(SYNTAX_ERR, "Expected identifier", id_token->line);
        add_error(parser->errors, err);
        return;
    }
    
    char *var_name = id_token->value;
    int line = id_token->line;
    
    // Check if variable is declared (SEMANTIC ERROR)
    Symbol *sym = findSymbol(parser->symbolTable, var_name);
    if(sym == NULL){
        char msg[256];
        sprintf(msg, "Variable '%s' not declared", var_name);
        Error err = create_error(SEMANTIC_ERR, msg, line);
        add_error(parser->errors, err);
    }
    
    advance(parser); // Move past identifier
    
    // Expect :=
    expect(parser, ASSIGN_OP, "Expected ':=' operator");
    
    // Get value
    Token *value_token = current_token(parser);
    
    // Type checking if variable exists (SEMANTIC ERROR)
    if(sym != NULL){
        if(sym->type == KEY_INT && value_token->type != INTEGER_LITERAL){
            char msg[256];
            sprintf(msg, "Type mismatch: Cannot assign %s to integer variable '%s'", 
                    value_token->value, var_name);
            Error err = create_error(SEMANTIC_ERR, msg, value_token->line);
            add_error(parser->errors, err);
        } else if(sym->type == KEY_REAL && 
                  value_token->type != FLOAT_LITERAL && 
                  value_token->type != INTEGER_LITERAL){
            char msg[256];
            sprintf(msg, "Type mismatch: Cannot assign %s to real variable '%s'", 
                    value_token->value, var_name);
            Error err = create_error(SEMANTIC_ERR, msg, value_token->line);
            add_error(parser->errors, err);
        } else if(sym->type == KEY_STRING && value_token->type != STRING_LITERAL){
            char msg[256];
            sprintf(msg, "Type mismatch: Cannot assign %s to string variable '%s'", 
                    value_token->value, var_name);
            Error err = create_error(SEMANTIC_ERR, msg, value_token->line);
            add_error(parser->errors, err);
        }
        
        // Update value
        if(sym->value != NULL){
            free(sym->value);
        }
        sym->value = malloc(strlen(value_token->value) + 1);
        strcpy(sym->value, value_token->value);
    }
    
    advance(parser); // Move past value
    
    // Expect # at end
    expect(parser, END_INSTRUCTION, "Expected '#' at end of instruction");
}

// Main parse function
void parse(Parser *parser){
    // Expect FRG_Begin
    if(!match(parser, KEYWORD_BEGIN)){
        Error err = create_error(SYNTAX_ERR, 
            "Program must start with FRG_Begin", 
            current_token(parser)->line);
        add_error(parser->errors, err);
        return;
    }
    
    // Expect # after FRG_Begin
    expect(parser, END_INSTRUCTION, "Expected '#' after FRG_Begin");
    
    // Skip comments
    while(current_token(parser)->type == COMMENT){
        advance(parser);
    }
    
    // Parse statements until FRG_End
    while(current_token(parser)->type != KEYWORD_END){
        TokenType type = current_token(parser)->type;
        
        // Skip comments
        if(type == COMMENT){
            advance(parser);
            continue;
        }
        
        // Declaration
        if(type == KEYWORD_INT || type == KEYWORD_REAL || type == KEYWORD_STRING){
            parse_declaration(parser);
        }
        // Assignment
        else if(type == IDENTIFIER){
            parse_assignment(parser);
        }
        // Unknown token
        else {
            char msg[256];
            sprintf(msg, "Unexpected token '%s'", current_token(parser)->value);
            Error err = create_error(SYNTAX_ERR, msg, current_token(parser)->line);
            add_error(parser->errors, err);
            advance(parser); // Skip bad token
        }
    }
    
    // Expect FRG_End
    if(!match(parser, KEYWORD_END)){
        Error err = create_error(SYNTAX_ERR, 
            "Program must end with FRG_End", 
            current_token(parser)->line);
        add_error(parser->errors, err);
    } else {
        // Expect # after FRG_End
        expect(parser, END_INSTRUCTION, "Expected '#' after FRG_End");
    }
}