#ifndef ERROR_H
#define ERROR_H

typedef enum {
    SYNTAX_ERR,
    LEXICAL_ERR,
    SEMANTIC_ERR
} ErrorType;

typedef struct {
    ErrorType type;
    char *err_message;
    int line;
} Error;

typedef struct {
    Error *errors;
    int count;
    int capacity;
} ErrorList;

Error create_error(ErrorType type, const char *message, int line);
void add_error(ErrorList *list, Error error);
void free_error_list(ErrorList *list);
void print_errors(ErrorList *list, ErrorType type);

#endif
