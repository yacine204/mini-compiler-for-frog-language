#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "include/token.h"
#include "include/error.h"
#include "include/symbol.h"
#include "include/lexer.h"
#include "include/parser.h"

// GUI Widgets
typedef struct {
    GtkWidget *window;
    GtkWidget *file_chooser_button;
    GtkWidget *file_path_label;
    GtkWidget *lexical_button;
    GtkWidget *syntax_button;
    GtkWidget *semantic_button;
    GtkWidget *source_text_view;
    GtkWidget *result_text_view;
    GtkWidget *variables_text_view;  // New: for displaying variables
    GtkTextBuffer *source_buffer;
    GtkTextBuffer *result_buffer;
    GtkTextBuffer *variables_buffer;  // New: buffer for variables
    GtkWidget *final_result_label;  
    char *current_file_path;
    TokenList tokenList;
    ErrorList errorList;
    SymbolTable symbolTable;
    char *program_output;
} AppWidgets;

static void set_buffer_text_utf8(GtkTextBuffer *buffer, const char *text) {
    if (!buffer) {
        return;
    }
    const char *src = text ? text : "";
    char *safe = g_utf8_make_valid(src, -1);
    gtk_text_buffer_set_text(buffer, safe, -1);
    g_free(safe);
}

static void set_label_text_utf8(GtkLabel *label, const char *text) {
    if (!label) {
        return;
    }
    const char *src = text ? text : "";
    char *safe = g_utf8_make_valid(src, -1);
    gtk_label_set_text(label, safe);
    g_free(safe);
}

static void replace_program_output(AppWidgets *widgets, char *owned_text) {
    if (!widgets) {
        if (owned_text) {
            free(owned_text);
        }
        return;
    }

    if (widgets->program_output) {
        free(widgets->program_output);
    }

    const char *src = owned_text ? owned_text : "";
    widgets->program_output = g_utf8_make_valid(src, -1);
    if (owned_text) {
        free(owned_text);
    }
}

// Update variables display
void update_variables_display(AppWidgets *widgets) {
    if (!widgets) {
        return;
    }

    GString *text = g_string_new(NULL);
    const char *type_names[] = {"Integer", "Real", "String", "Unknown"};

    g_string_append(text, "========================================\n");
    g_string_append(text, "        DECLARED VARIABLES\n");
    g_string_append(text, "========================================\n\n");

    if (widgets->symbolTable.count == 0) {
        g_string_append(text, "No variables declared yet.\n");
    } else {
        g_string_append(text, "Variable Name        Type            Value\n");
        g_string_append(text, "-------------------  --------------  ----------------\n");

        for (int i = 0; i < widgets->symbolTable.count; i++) {
            Symbol *sym = &widgets->symbolTable.symbols[i];
            g_string_append_printf(text, "%-20s %-15s %s\n",
                                   sym->id,
                                   type_names[sym->type],
                                   sym->value ? sym->value : "uninitialized");
        }

        g_string_append_printf(text, "\nTotal Variables: %d\n", widgets->symbolTable.count);
    }

    g_string_append(text, "\n========================================\n");
    g_string_append(text, "          PROGRAM OUTPUT\n");
    g_string_append(text, "========================================\n\n");

    const char *output_text = (widgets->program_output && widgets->program_output[0] != '\0')
        ? widgets->program_output
        : "No FRG_Print output generated.\n";

    g_string_append(text, output_text);
    if(text->len == 0 || text->str[text->len - 1] != '\n') {
        g_string_append_c(text, '\n');
    }

    gtk_text_buffer_set_text(widgets->variables_buffer, text->str, -1);
    g_string_free(text, TRUE);
}

// Load file content into source text view
void load_file_content(AppWidgets *widgets, const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        set_buffer_text_utf8(widgets->result_buffer, "Error: Cannot open file");
        return;
    }
    
    // Read file content
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);
    
    GError *err = NULL;
    char *utf8 = g_locale_to_utf8(content, -1, NULL, NULL, &err);
    if (!utf8) {
        if (err) {
            g_error_free(err);
            err = NULL;
        }
        utf8 = g_utf8_make_valid(content, -1);
    }
    set_buffer_text_utf8(widgets->source_buffer, utf8);
    g_free(utf8);
    free(content);
}

// File chooser callback
void on_file_chosen(GtkFileChooserButton *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));
    if (filename) {
        // Update file path label
        char label_text[512];
        snprintf(label_text, sizeof(label_text), "File: %s", filename);
        set_label_text_utf8(GTK_LABEL(widgets->file_path_label), label_text);
        
        // Store file path
        if (widgets->current_file_path) {
            free(widgets->current_file_path);
        }
        widgets->current_file_path = strdup(filename);
        
        // Load file content
        load_file_content(widgets, filename);
        
        // Clear result and variables
        set_buffer_text_utf8(widgets->result_buffer, "File loaded. Click analysis buttons to proceed.");
        set_buffer_text_utf8(widgets->variables_buffer, "No analysis performed yet.");
        replace_program_output(widgets, NULL);
        
        g_free(filename);
    }
}

// Lexical Analysis Button Callback
void on_lexical_analysis(GtkWidget *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;

    if (!widgets->current_file_path) {
        set_buffer_text_utf8(widgets->result_buffer, "Please select a file first!");
        return;
    }

    // Reset data structures
    widgets->tokenList = (TokenList){NULL, 0, 0};
    widgets->errorList = (ErrorList){NULL, 0, 0};

    // Run lexical analysis
    lexer(widgets->current_file_path, &widgets->tokenList, &widgets->errorList);

    // Build result string
    char result[8192] = {0};
    strcat(result, "========================================\n");
    strcat(result, "      LEXICAL ANALYSIS RESULTS\n");
    strcat(result, "========================================\n\n");

    // Token names
    const char *token_names[] = {
        "NONE",
        "KEYWORD_BEGIN",
        "KEYWORD_END",
        "KEYWORD_INT",
        "KEYWORD_REAL",
        "KEYWORD_STRING",
        "KEYWORD_PRINT",
        "KEYWORD_IF",
        "KEYWORD_ELSE",
        "KEYWORD_REPEAT",
        "KEYWORD_UNTIL",
        "BLOCK_BEGIN",
        "BLOCK_END",
        "IDENTIFIER",
        "INTEGER_LITERAL",
        "FLOAT_LITERAL",
        "STRING_LITERAL",
        "ASSIGN_OP",
        "END_INSTRUCTION",
        "COMMENT",
        "COMMA",
        "OPEN_BRACKET",
        "CLOSE_BRACKET",
        "OPEN_PAREN",
        "CLOSE_PAREN",
        "OPERATOR_PLUS",
        "OPERATOR_MINUS",
        "OPERATOR_MULTIPLY",
        "OPERATOR_DIVIDE",
        "RELATIONAL_OP"
    };

    // Add tokens
    char temp[256];
    sprintf(temp, "Total Tokens: %d\n\n", widgets->tokenList.count);
    strcat(result, temp);

    strcat(result, "Line  Type                 Value\n");
    strcat(result, "----  -------------------  --------------------\n");

    for (int i = 0; i < widgets->tokenList.count && i < 100; i++) {
        sprintf(temp, "%-4d  %-20s '%s'\n",
                widgets->tokenList.tokens[i].line,
                token_names[widgets->tokenList.tokens[i].type],
                widgets->tokenList.tokens[i].value);
        strcat(result, temp);
    }

    // Add errors
    strcat(result, "\n========================================\n");
    strcat(result, "           LEXICAL ERRORS\n");
    strcat(result, "========================================\n");

    int error_count = 0;
    for (int i = 0; i < widgets->errorList.count; i++) {
        if (widgets->errorList.errors[i].type == LEXICAL_ERR) {
            sprintf(temp, "Line %d: %s\n",
                    widgets->errorList.errors[i].line,
                    widgets->errorList.errors[i].err_message);
            strcat(result, temp);
            error_count++;
        }
    }

    if (error_count == 0) {
        strcat(result, "No lexical errors found! ✓\n");
    }

    sprintf(temp, "\nTotal Lexical Errors: %d\n", error_count);
    strcat(result, temp);

    // Display result
    set_buffer_text_utf8(widgets->result_buffer, result);
    replace_program_output(widgets, NULL);
}

// Syntax Analysis Button Callback
void on_syntax_analysis(GtkWidget *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;

    if (!widgets->current_file_path) {
        set_buffer_text_utf8(widgets->result_buffer, "Please select a file first!");
        return;
    }

    // Reset and run lexical first
    widgets->tokenList = (TokenList){NULL, 0, 0};
    widgets->errorList = (ErrorList){NULL, 0, 0};
    widgets->symbolTable = (SymbolTable){NULL, 0, 0};

    lexer(widgets->current_file_path, &widgets->tokenList, &widgets->errorList);

    // Run parser (syntax analysis)
    OutputBuffer exec_output;
    init_output_buffer(&exec_output);

    Parser parser;
    init_parser(&parser, &widgets->tokenList, &widgets->symbolTable, &widgets->errorList, &exec_output);
    parse(&parser);

    char *program_output = detach_output_buffer(&exec_output);
    replace_program_output(widgets, program_output);

    // Build result string
    char result[8192] = {0};
    strcat(result, "========================================\n");
    strcat(result, "       SYNTAX ANALYSIS RESULTS\n");
    strcat(result, "========================================\n\n");

    // Add syntax errors
    char temp[256];
    int error_count = 0;

    for (int i = 0; i < widgets->errorList.count; i++) {
        if (widgets->errorList.errors[i].type == SYNTAX_ERR) {
            sprintf(temp, "Line %d: %s\n",
                    widgets->errorList.errors[i].line,
                    widgets->errorList.errors[i].err_message);
            strcat(result, temp);
            error_count++;
        }
    }

    if (error_count == 0) {
        strcat(result, "No syntax errors found! ✓\n\n");
        strcat(result, "Program structure is correct:\n");
        strcat(result, "- Starts with FRG_Begin\n");
        strcat(result, "- Ends with FRG_End\n");
        strcat(result, "- All instructions properly terminated with #\n");
    } else {
        sprintf(temp, "\nTotal Syntax Errors: %d\n", error_count);
        strcat(result, temp);
    }

    // Display result
    set_buffer_text_utf8(widgets->result_buffer, result);

    // Update variables display
    update_variables_display(widgets);
}

// Semantic Analysis Button Callback
void on_semantic_analysis(GtkWidget *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    if (!widgets->current_file_path) {
        set_buffer_text_utf8(widgets->result_buffer, "Please select a file first!");
        return;
    }
    
    // Reset and run full analysis
    widgets->tokenList = (TokenList){NULL, 0, 0};
    widgets->errorList = (ErrorList){NULL, 0, 0};
    widgets->symbolTable = (SymbolTable){NULL, 0, 0};
    
    lexer(widgets->current_file_path, &widgets->tokenList, &widgets->errorList);
    
    OutputBuffer exec_output;
    init_output_buffer(&exec_output);

    Parser parser;
    init_parser(&parser, &widgets->tokenList, &widgets->symbolTable, &widgets->errorList, &exec_output);
    parse(&parser);

    char *program_output = detach_output_buffer(&exec_output);
    replace_program_output(widgets, program_output);
    
    // Build result string
    char result[8192] = {0};
    strcat(result, "========================================\n");
    strcat(result, "      SEMANTIC ANALYSIS RESULTS\n");
    strcat(result, "========================================\n\n");
    
    // Add semantic errors
    char temp[256];
    int error_count = 0;
    
    for (int i = 0; i < widgets->errorList.count; i++) {
        if (widgets->errorList.errors[i].type == SEMANTIC_ERR) {
            sprintf(temp, "Line %d: %s\n",
                    widgets->errorList.errors[i].line,
                    widgets->errorList.errors[i].err_message);
            strcat(result, temp);
            error_count++;
        }
    }
    
    if (error_count == 0) {
        strcat(result, "No semantic errors found! ✓\n\n");
    } else {
        sprintf(temp, "\nTotal Semantic Errors: %d\n\n", error_count);
        strcat(result, temp);
    }
    
    // Add symbol table
    strcat(result, "========================================\n");
    strcat(result, "          SYMBOL TABLE\n");
    strcat(result, "========================================\n\n");
    
    const char *type_names[] = {"Integer", "Real", "String", "Unknown"};
    
    strcat(result, "Name                Type            Value\n");
    strcat(result, "-------------------  --------------  --------------\n");
    
    for (int i = 0; i < widgets->symbolTable.count; i++) {
        Symbol *sym = &widgets->symbolTable.symbols[i];
        sprintf(temp, "%-20s %-15s %s\n",
                sym->id,
                type_names[sym->type],
                sym->value ? sym->value : "uninitialized");
        strcat(result, temp);
    }
    
    if (widgets->symbolTable.count == 0) {
        strcat(result, "(No variables declared)\n");
    }
    
    // Display result
    set_buffer_text_utf8(widgets->result_buffer, result);
    
    // Update variables display
    update_variables_display(widgets);
}

// Create GUI
void create_gui(AppWidgets *widgets) {
    // Main window
    widgets->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(widgets->window), "FROG Compiler");
    gtk_window_set_default_size(GTK_WINDOW(widgets->window), 1000, 800);
    gtk_container_set_border_width(GTK_CONTAINER(widgets->window), 10);
    g_signal_connect(widgets->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Main vertical box
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(widgets->window), main_vbox);
    
    // Top section: File chooser
    GtkWidget *file_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), file_hbox, FALSE, FALSE, 0);
    
    GtkWidget *file_label = gtk_label_new("Select File:");
    gtk_box_pack_start(GTK_BOX(file_hbox), file_label, FALSE, FALSE, 0);
    
    widgets->file_chooser_button = gtk_file_chooser_button_new("Select FROG File", 
                                                                GTK_FILE_CHOOSER_ACTION_OPEN);
    
    // Add file filter for .frg files
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "FROG Files (*.frg)");
    gtk_file_filter_add_pattern(filter, "*.frg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widgets->file_chooser_button), filter);
    
    gtk_box_pack_start(GTK_BOX(file_hbox), widgets->file_chooser_button, TRUE, TRUE, 0);
    g_signal_connect(widgets->file_chooser_button, "file-set", 
                     G_CALLBACK(on_file_chosen), widgets);
    
    // File path label
    widgets->file_path_label = gtk_label_new("No file selected");
    gtk_label_set_xalign(GTK_LABEL(widgets->file_path_label), 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), widgets->file_path_label, FALSE, FALSE, 0);
    
    // Analysis buttons
    GtkWidget *button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), button_hbox, FALSE, FALSE, 0);
    
    widgets->lexical_button = gtk_button_new_with_label("Lexical Analysis");
    gtk_box_pack_start(GTK_BOX(button_hbox), widgets->lexical_button, TRUE, TRUE, 0);
    g_signal_connect(widgets->lexical_button, "clicked", 
                     G_CALLBACK(on_lexical_analysis), widgets);
    
    widgets->syntax_button = gtk_button_new_with_label("Syntax Analysis");
    gtk_box_pack_start(GTK_BOX(button_hbox), widgets->syntax_button, TRUE, TRUE, 0);
    g_signal_connect(widgets->syntax_button, "clicked", 
                     G_CALLBACK(on_syntax_analysis), widgets);
    
    widgets->semantic_button = gtk_button_new_with_label("Semantic Analysis");
    gtk_box_pack_start(GTK_BOX(button_hbox), widgets->semantic_button, TRUE, TRUE, 0);
    g_signal_connect(widgets->semantic_button, "clicked", 
                     G_CALLBACK(on_semantic_analysis), widgets);
    
    // Text views section - horizontal split
    GtkWidget *text_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), text_hbox, TRUE, TRUE, 0);
    
    // Source code frame
    GtkWidget *source_frame = gtk_frame_new("Source Code");
    gtk_box_pack_start(GTK_BOX(text_hbox), source_frame, TRUE, TRUE, 0);
    
    GtkWidget *source_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(source_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(source_frame), source_scroll);
    
    widgets->source_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->source_text_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(widgets->source_text_view), TRUE);
    widgets->source_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->source_text_view));
    gtk_container_add(GTK_CONTAINER(source_scroll), widgets->source_text_view);
    
    // Result frame
    GtkWidget *result_frame = gtk_frame_new("Analysis Results");
    gtk_box_pack_start(GTK_BOX(text_hbox), result_frame, TRUE, TRUE, 0);
    
    GtkWidget *result_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(result_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(result_frame), result_scroll);
    
    widgets->result_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->result_text_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(widgets->result_text_view), TRUE);
    widgets->result_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->result_text_view));
    gtk_container_add(GTK_CONTAINER(result_scroll), widgets->result_text_view);
    
    // NEW: Variables display section at bottom (50% width)
    GtkWidget *bottom_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), bottom_hbox, FALSE, FALSE, 0);
    gtk_widget_set_size_request(bottom_hbox, -1, 200);  // Fixed height
    
    // Variables frame (takes 50% width)
    GtkWidget *variables_frame = gtk_frame_new("Output");
    gtk_box_pack_start(GTK_BOX(bottom_hbox), variables_frame, TRUE, TRUE, 0);
    
    GtkWidget *variables_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(variables_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(variables_frame), variables_scroll);
    
    widgets->variables_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->variables_text_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(widgets->variables_text_view), TRUE);
    widgets->variables_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->variables_text_view));
    gtk_container_add(GTK_CONTAINER(variables_scroll), widgets->variables_text_view);
    
    // Initial message
    set_buffer_text_utf8(widgets->result_buffer,
        "Welcome to FROG Compiler!\n\nPlease select a .frg file to begin analysis.");

    set_buffer_text_utf8(widgets->variables_buffer,
        "No analysis performed yet.");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    AppWidgets widgets = {0};
    widgets.current_file_path = NULL;
    widgets.program_output = NULL;
    
    create_gui(&widgets);
    gtk_widget_show_all(widgets.window);
    gtk_main();
    
    if (widgets.current_file_path) {
        free(widgets.current_file_path);
    }
    if (widgets.program_output) {
        free(widgets.program_output);
    }
    
    return 0;

    }