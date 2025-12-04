#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/parser.h"
#include "../include/token.h"
#include "../include/symbol.h"
#include "../include/error.h"

static void ensure_output_capacity(OutputBuffer *buffer, size_t additional){
    if(buffer == NULL){
        return;
    }

    size_t required = buffer->length + additional + 1;
    if(required <= buffer->capacity){
        return;
    }

    size_t new_capacity = buffer->capacity == 0 ? 128 : buffer->capacity;
    while(new_capacity < required){
        new_capacity *= 2;
    }

    buffer->data = realloc(buffer->data, new_capacity);
    buffer->capacity = new_capacity;
}

void init_output_buffer(OutputBuffer *buffer){
    if(buffer == NULL){
        return;
    }
    buffer->data = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
}

void free_output_buffer(OutputBuffer *buffer){
    if(buffer == NULL){
        return;
    }
    free(buffer->data);
    buffer->data = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
}

char *detach_output_buffer(OutputBuffer *buffer){
    if(buffer == NULL){
        return NULL;
    }
    char *data = buffer->data;
    buffer->data = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
    return data;
}

static void output_buffer_append(OutputBuffer *buffer, const char *text){
    if(buffer == NULL || text == NULL){
        return;
    }
    size_t len = strlen(text);
    if(len == 0){
        return;
    }
    ensure_output_capacity(buffer, len);
    memcpy(buffer->data + buffer->length, text, len);
    buffer->length += len;
    buffer->data[buffer->length] = '\0';
}

typedef struct {
    SymbolType inferred_type;
    int token_count;
    int has_value;
    int is_string;
    double numeric_value;
    char string_value[512];
    int last_line;
} ExpressionResult;

typedef struct {
    Parser *parser;
    const TokenType *terminators;
    size_t term_count;
} ExpressionContext;

void init_parser(Parser *parser, TokenList *tokens, SymbolTable *symbolTable, ErrorList *errors, OutputBuffer *output){
    parser->tokens = tokens;
    parser->position = 0;
    parser->symbolTable = symbolTable;
    parser->errors = errors;
    parser->output = output;
}

static void append_expression_to_output(Parser *parser, const ExpressionResult *expr, int is_first){
    if(parser == NULL || parser->output == NULL || expr == NULL){
        return;
    }

    if(!is_first){
        output_buffer_append(parser->output, " ");
    }

    if(!expr->has_value){
        output_buffer_append(parser->output, "<undef>");
        return;
    }

    if(expr->inferred_type == KEY_STRING){
        output_buffer_append(parser->output, expr->string_value);
        return;
    }

    char buffer[64];
    if(expr->inferred_type == KEY_REAL){
        snprintf(buffer, sizeof(buffer), "%.6g", expr->numeric_value);
    } else {
        snprintf(buffer, sizeof(buffer), "%.0f", expr->numeric_value);
    }
    output_buffer_append(parser->output, buffer);
}

static Token* current_token(Parser *parser){
    if(parser->tokens == NULL || parser->tokens->count == 0){
        return NULL;
    }
    if(parser->position >= parser->tokens->count){
        return NULL;
    }
    return &parser->tokens->tokens[parser->position];
}

static Token* previous_token(Parser *parser){
    if(parser->tokens == NULL || parser->tokens->count == 0 || parser->position == 0){
        return NULL;
    }
    int index = parser->position - 1;
    if(index >= parser->tokens->count){
        index = parser->tokens->count - 1;
    }
    return &parser->tokens->tokens[index];
}

static void advance(Parser *parser){
    if(parser->tokens == NULL){
        return;
    }
    if(parser->position < parser->tokens->count){
        parser->position++;
    }
}

static int match(Parser *parser, TokenType type){
    Token *token = current_token(parser);
    if(token != NULL && token->type == type){
        advance(parser);
        return 1;
    }
    return 0;
}

static void add_syntax_error(Parser *parser, const char *message, int line){
    Error err = create_error(SYNTAX_ERR, message, line);
    add_error(parser->errors, err);
}

static void add_semantic_error(Parser *parser, const char *message, int line){
    Error err = create_error(SEMANTIC_ERR, message, line);
    add_error(parser->errors, err);
}

static void expect(Parser *parser, TokenType type, const char *message){
    Token *token = current_token(parser);
    if(token == NULL){
        Token *prev = previous_token(parser);
        int line = prev ? prev->line : 0;
        add_syntax_error(parser, message, line);
        return;
    }
    if(token->type != type){
        add_syntax_error(parser, message, token->line);
        advance(parser);
    } else {
        advance(parser);
    }
}

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

static ExpressionResult make_unknown_expression(void){
    ExpressionResult res;
    memset(&res, 0, sizeof(res));
    res.inferred_type = KEY_UNKNOWN;
    return res;
}

static int is_expr_terminator(ExpressionContext *ctx, Token *token){
    if(token == NULL){
        return 1;
    }
    for(size_t i = 0; i < ctx->term_count; i++){
        if(token->type == ctx->terminators[i]){
            return 1;
        }
    }
    return 0;
}

static ExpressionResult parse_add_sub(ExpressionContext *ctx);
static ExpressionResult parse_expression(Parser *parser, const TokenType *terminators, size_t term_count);

static ExpressionResult parse_primary(ExpressionContext *ctx){
    Parser *parser = ctx->parser;
    Token *token = current_token(parser);
    ExpressionResult result = make_unknown_expression();

    if(token == NULL){
        return result;
    }

    if(is_expr_terminator(ctx, token)){
        return result;
    }

    switch(token->type){
        case INTEGER_LITERAL:
            result.inferred_type = KEY_INT;
            result.token_count = 1;
            result.has_value = 1;
            result.is_string = 0;
            result.numeric_value = strtod(token->value, NULL);
            result.last_line = token->line;
            advance(parser);
            break;
        case FLOAT_LITERAL:
            result.inferred_type = KEY_REAL;
            result.token_count = 1;
            result.has_value = 1;
            result.is_string = 0;
            result.numeric_value = strtod(token->value, NULL);
            result.last_line = token->line;
            advance(parser);
            break;
        case STRING_LITERAL:
            result.inferred_type = KEY_STRING;
            result.token_count = 1;
            result.has_value = 1;
            result.is_string = 1;
            strncpy(result.string_value, token->value, sizeof(result.string_value) - 1);
            result.string_value[sizeof(result.string_value) - 1] = '\0';
            result.last_line = token->line;
            advance(parser);
            break;
        case IDENTIFIER: {
            result.token_count = 1;
            result.last_line = token->line;
            Symbol *sym = findSymbol(parser->symbolTable, token->value);
            if(sym == NULL){
                char msg[256];
                sprintf(msg, "Variable '%s' not declared", token->value);
                add_semantic_error(parser, msg, token->line);
                advance(parser);
                break;
            }

            result.inferred_type = sym->type;
            if(sym->value == NULL){
                char msg[256];
                sprintf(msg, "Variable '%s' used before assignment", sym->id);
                add_semantic_error(parser, msg, token->line);
                result.has_value = 0;
            } else if(sym->type == KEY_STRING){
                result.has_value = 1;
                result.is_string = 1;
                strncpy(result.string_value, sym->value, sizeof(result.string_value) - 1);
                result.string_value[sizeof(result.string_value) - 1] = '\0';
            } else {
                result.has_value = 1;
                result.is_string = 0;
                result.numeric_value = strtod(sym->value, NULL);
                if(sym->type == KEY_REAL){
                    result.inferred_type = KEY_REAL;
                } else {
                    result.inferred_type = KEY_INT;
                }
            }
            advance(parser);
            break;
        }
        case OPEN_PAREN: {
            int start_line = token->line;
            advance(parser); // consume '('
            TokenType close = CLOSE_PAREN;
            ExpressionResult inner = parse_expression(parser, &close, 1);
            expect(parser, CLOSE_PAREN, "Expected ')' to close expression");
            inner.token_count += 2;
            if(inner.last_line == 0){
                inner.last_line = start_line;
            }
            return inner;
        }
        default: {
            char msg[256];
            sprintf(msg, "Unexpected token '%s' in expression", token->value);
            add_syntax_error(parser, msg, token->line);
            advance(parser);
            break;
        }
    }

    return result;
}

static ExpressionResult parse_unary(ExpressionContext *ctx){
    Token *token = current_token(ctx->parser);
    if(token != NULL && token->type == OPERATOR_MINUS){
        advance(ctx->parser);
        ExpressionResult operand = parse_unary(ctx);
        operand.token_count += 1;
        operand.last_line = operand.last_line ? operand.last_line : (token ? token->line : 0);

        if(operand.inferred_type == KEY_STRING){
            add_semantic_error(ctx->parser, "Cannot apply unary '-' to a string", token->line);
            return make_unknown_expression();
        }

        if(operand.has_value){
            operand.numeric_value = -operand.numeric_value;
        }
        if(operand.inferred_type == KEY_UNKNOWN){
            operand.inferred_type = KEY_INT;
        }
        operand.is_string = 0;
        return operand;
    }
    return parse_primary(ctx);
}

static ExpressionResult parse_mul_div(ExpressionContext *ctx){
    ExpressionResult left = parse_unary(ctx);
    while(1){
        Token *token = current_token(ctx->parser);
        if(token == NULL || is_expr_terminator(ctx, token)){
            break;
        }
        if(token->type != OPERATOR_MULTIPLY && token->type != OPERATOR_DIVIDE){
            break;
        }

        TokenType op = token->type;
        advance(ctx->parser);
        ExpressionResult right = parse_unary(ctx);

        ExpressionResult combined = make_unknown_expression();
        combined.token_count = left.token_count + right.token_count + 1;
        combined.last_line = right.token_count ? right.last_line : token->line;

        if(left.inferred_type == KEY_STRING || right.inferred_type == KEY_STRING){
            add_semantic_error(ctx->parser, "String values are not allowed in arithmetic expressions", token->line);
        } else {
            combined.inferred_type = KEY_INT;
            if(left.inferred_type == KEY_REAL || right.inferred_type == KEY_REAL || op == OPERATOR_DIVIDE){
                combined.inferred_type = KEY_REAL;
            }

            combined.has_value = left.has_value && right.has_value;
            combined.is_string = 0;

            if(combined.has_value){
                double lhs = left.numeric_value;
                double rhs = right.numeric_value;
                if(op == OPERATOR_MULTIPLY){
                    combined.numeric_value = lhs * rhs;
                } else {
                    if(rhs == 0.0){
                        add_semantic_error(ctx->parser, "Division by zero", token->line);
                        combined.has_value = 0;
                    } else {
                        combined.numeric_value = lhs / rhs;
                    }
                }
            }
        }

        left = combined;
    }
    return left;
}

static ExpressionResult parse_add_sub(ExpressionContext *ctx){
    ExpressionResult left = parse_mul_div(ctx);
    while(1){
        Token *token = current_token(ctx->parser);
        if(token == NULL || is_expr_terminator(ctx, token)){
            break;
        }
        if(token->type != OPERATOR_PLUS && token->type != OPERATOR_MINUS){
            break;
        }

        TokenType op = token->type;
        advance(ctx->parser);
        ExpressionResult right = parse_mul_div(ctx);

        ExpressionResult combined = make_unknown_expression();
        combined.token_count = left.token_count + right.token_count + 1;
        combined.last_line = right.token_count ? right.last_line : token->line;

        if(left.inferred_type == KEY_STRING || right.inferred_type == KEY_STRING){
            add_semantic_error(ctx->parser, "String values are not allowed in arithmetic expressions", token->line);
        } else {
            combined.inferred_type = KEY_INT;
            if(left.inferred_type == KEY_REAL || right.inferred_type == KEY_REAL){
                combined.inferred_type = KEY_REAL;
            }

            if(left.inferred_type == KEY_UNKNOWN && right.inferred_type != KEY_UNKNOWN){
                combined.inferred_type = right.inferred_type;
            }
            if(right.inferred_type == KEY_UNKNOWN && left.inferred_type != KEY_UNKNOWN){
                combined.inferred_type = left.inferred_type;
            }

            combined.has_value = left.has_value && right.has_value;
            combined.is_string = 0;

            if(combined.has_value){
                double lhs = left.numeric_value;
                double rhs = right.numeric_value;
                if(op == OPERATOR_PLUS){
                    combined.numeric_value = lhs + rhs;
                } else {
                    combined.numeric_value = lhs - rhs;
                }
            }
        }

        left = combined;
    }
    return left;
}

static ExpressionResult parse_expression(Parser *parser, const TokenType *terminators, size_t term_count){
    ExpressionContext ctx = {parser, terminators, term_count};
    ExpressionResult result = parse_add_sub(&ctx);
    if(result.token_count == 0){
        Token *token = current_token(parser);
        int line = token ? token->line : (previous_token(parser) ? previous_token(parser)->line : 0);
        add_syntax_error(parser, "Expected expression", line);
    }
    return result;
}

static int is_assignment_compatible(SymbolType target, SymbolType source){
    if(source == KEY_UNKNOWN){
        return 1;
    }
    if(target == KEY_STRING){
        return source == KEY_STRING;
    }
    if(target == KEY_REAL){
        return (source == KEY_REAL || source == KEY_INT);
    }
    if(target == KEY_INT){
        return source == KEY_INT;
    }
    return 1;
}

static void update_symbol_value(Symbol *sym, const ExpressionResult *expr){
    if(sym == NULL || expr == NULL){
        return;
    }
    if(sym->value != NULL){
        free(sym->value);
        sym->value = NULL;
    }
    if(expr->token_count == 0 || !expr->has_value){
        return;
    }

    if(expr->inferred_type == KEY_STRING){
        const char *stored = expr->string_value;
        sym->value = malloc(strlen(stored) + 1);
        strcpy(sym->value, stored);
        return;
    }

    char buffer[64];
    if(sym->type == KEY_REAL || expr->inferred_type == KEY_REAL){
        snprintf(buffer, sizeof(buffer), "%.6g", expr->numeric_value);
    } else {
        snprintf(buffer, sizeof(buffer), "%.0f", expr->numeric_value);
    }
    sym->value = malloc(strlen(buffer) + 1);
    strcpy(sym->value, buffer);
}

static void parse_declaration(Parser *parser, TokenType decl_type){
    SymbolType sym_type = token_to_symbol_type(decl_type);
    Token *type_token = current_token(parser);
    if(type_token == NULL){
        return;
    }

    advance(parser); // consume type keyword

    while(1){
        Token *token = current_token(parser);
        if(token == NULL){
            add_syntax_error(parser, "Unexpected end of declaration", type_token->line);
            return;
        }

        if(token->type != IDENTIFIER){
            add_syntax_error(parser, "Expected identifier in declaration", token->line);
            if(token->type == END_INSTRUCTION){
                break;
            }
            advance(parser);
            continue;
        }

        Symbol *existing = findSymbol(parser->symbolTable, token->value);
        if(existing != NULL){
            char msg[256];
            sprintf(msg, "Variable '%s' already declared at line %d", token->value, existing->line_declared);
            add_semantic_error(parser, msg, token->line);
        } else {
            Symbol sym = create_symbol(token->value, sym_type, token->line);
            add_symbol(parser->symbolTable, sym);
        }

        char var_name[256];
        strncpy(var_name, token->value, sizeof(var_name) - 1);
        var_name[sizeof(var_name) - 1] = '\0';
        advance(parser);

        if(match(parser, ASSIGN_OP)){
            TokenType stops[] = {COMMA, END_INSTRUCTION};
            ExpressionResult expr = parse_expression(parser, stops, 2);
            Symbol *sym = findSymbol(parser->symbolTable, var_name);
            if(sym != NULL){
                if(!is_assignment_compatible(sym_type, expr.inferred_type)){
                    char msg[256];
                    sprintf(msg, "Type mismatch in declaration of '%s'", var_name);
                    add_semantic_error(parser, msg, expr.last_line ? expr.last_line : sym->line_declared);
                }
                update_symbol_value(sym, &expr);
            }
        }

        if(match(parser, COMMA)){
            continue;
        }
        break;
    }

    expect(parser, END_INSTRUCTION, "Expected '#' at end of declaration");
}

static void parse_assignment(Parser *parser){
    Token *id_token = current_token(parser);
    if(id_token == NULL){
        return;
    }

    if(id_token->type != IDENTIFIER){
        add_syntax_error(parser, "Expected identifier", id_token->line);
        advance(parser);
        return;
    }

    Symbol *sym = findSymbol(parser->symbolTable, id_token->value);
    if(sym == NULL){
        char msg[256];
        sprintf(msg, "Variable '%s' not declared", id_token->value);
        add_semantic_error(parser, msg, id_token->line);
    }

    advance(parser); // consume identifier
    expect(parser, ASSIGN_OP, "Expected ':=' operator");

    TokenType stops[] = {END_INSTRUCTION};
    ExpressionResult expr = parse_expression(parser, stops, 1);

    if(sym != NULL){
        if(!is_assignment_compatible(sym->type, expr.inferred_type)){
            char msg[256];
            sprintf(msg, "Type mismatch while assigning to '%s'", sym->id);
            add_semantic_error(parser, msg, expr.last_line ? expr.last_line : id_token->line);
        }
        update_symbol_value(sym, &expr);
    }

    expect(parser, END_INSTRUCTION, "Expected '#' at end of instruction");
}

static void parse_print(Parser *parser){
    int argument_count = 0;
    int first_value = 1;
    while(1){
        TokenType stops[] = {COMMA, END_INSTRUCTION};
        ExpressionResult expr = parse_expression(parser, stops, 2);
        if(expr.token_count == 0){
            break;
        }
        append_expression_to_output(parser, &expr, first_value);
        first_value = 0;
        argument_count++;
        if(match(parser, COMMA)){
            continue;
        }
        break;
    }

    expect(parser, END_INSTRUCTION, "Expected '#' after FRG_Print");

    if(argument_count == 0){
        Token *token = previous_token(parser);
        int line = token ? token->line : 0;
        add_syntax_error(parser, "FRG_Print requires at least one argument", line);
    } else if(parser->output != NULL){
        output_buffer_append(parser->output, "\n");
    }
}

static void parse_condition(Parser *parser, const char *context){
    char message[256];
    snprintf(message, sizeof(message), "Expected '[' to start %s condition", context);
    expect(parser, OPEN_BRACKET, message);

    TokenType left_terms[] = {RELATIONAL_OP, CLOSE_BRACKET};
    ExpressionResult left = parse_expression(parser, left_terms, 2);

    Token *rel = current_token(parser);
    if(rel == NULL || rel->type != RELATIONAL_OP){
        snprintf(message, sizeof(message), "Expected relational operator in %s condition", context);
        add_syntax_error(parser, message, rel ? rel->line : (previous_token(parser) ? previous_token(parser)->line : 0));
    } else {
        advance(parser);
    }

    TokenType right_terms[] = {CLOSE_BRACKET};
    ExpressionResult right = parse_expression(parser, right_terms, 1);

    snprintf(message, sizeof(message), "Expected ']' to close %s condition", context);
    expect(parser, CLOSE_BRACKET, message);

    if(left.inferred_type == KEY_STRING || right.inferred_type == KEY_STRING){
        if(left.inferred_type != KEY_STRING || right.inferred_type != KEY_STRING){
            snprintf(message, sizeof(message), "Cannot compare string with non-string in %s", context);
            add_semantic_error(parser, message, right.last_line ? right.last_line : left.last_line);
        }
    }
}

static void parse_statement(Parser *parser);

static void parse_block(Parser *parser){
    while(1){
        Token *token = current_token(parser);
        if(token == NULL || token->type == BLOCK_END){
            break;
        }
        parse_statement(parser);
    }
    expect(parser, BLOCK_END, "Expected 'End' to close block");
}

static void parse_if(Parser *parser){
    parse_condition(parser, "If");
    parse_statement(parser);
    if(match(parser, KEYWORD_ELSE)){
        parse_statement(parser);
    }
}

static void parse_repeat(Parser *parser){
    while(1){
        Token *token = current_token(parser);
        if(token == NULL || token->type == KEYWORD_UNTIL){
            break;
        }
        parse_statement(parser);
    }

    if(!match(parser, KEYWORD_UNTIL)){
        add_syntax_error(parser, "Expected 'until' to close Repeat block", previous_token(parser) ? previous_token(parser)->line : 0);
        return;
    }

    parse_condition(parser, "until");
}

static void parse_statement(Parser *parser){
    Token *token = current_token(parser);
    if(token == NULL){
        return;
    }

    switch(token->type){
        case COMMENT:
            advance(parser);
            break;
        case END_INSTRUCTION:
            advance(parser);
            break;
        case KEYWORD_INT:
        case KEYWORD_REAL:
        case KEYWORD_STRING:
            parse_declaration(parser, token->type);
            break;
        case IDENTIFIER:
            parse_assignment(parser);
            break;
        case KEYWORD_PRINT:
            advance(parser);
            parse_print(parser);
            break;
        case KEYWORD_IF:
            advance(parser);
            parse_if(parser);
            break;
        case KEYWORD_ELSE:
            add_syntax_error(parser, "Else without matching If", token->line);
            advance(parser);
            break;
        case BLOCK_BEGIN:
            advance(parser);
            parse_block(parser);
            break;
        case BLOCK_END:
            add_syntax_error(parser, "Unexpected 'End'", token->line);
            advance(parser);
            break;
        case KEYWORD_REPEAT:
            advance(parser);
            parse_repeat(parser);
            break;
        case KEYWORD_UNTIL:
            add_syntax_error(parser, "Unexpected 'until' without Repeat", token->line);
            advance(parser);
            break;
        default: {
            char msg[256];
            sprintf(msg, "Unexpected token '%s'", token->value);
            add_syntax_error(parser, msg, token->line);
            advance(parser);
            break;
        }
    }
}

void parse(Parser *parser){
    if(parser->tokens == NULL || parser->tokens->count == 0){
        add_syntax_error(parser, "Source is empty", 0);
        return;
    }

    if(!match(parser, KEYWORD_BEGIN)){
        Token *token = current_token(parser);
        int line = token ? token->line : 0;
        add_syntax_error(parser, "Program must start with FRG_Begin", line);
    }

    while(1){
        Token *token = current_token(parser);
        if(token == NULL || token->type == KEYWORD_END){
            break;
        }
        parse_statement(parser);
    }

    if(!match(parser, KEYWORD_END)){
        Token *token = previous_token(parser);
        int line = token ? token->line : 0;
        add_syntax_error(parser, "Program must end with FRG_End", line);
    }
}

