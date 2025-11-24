#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    GtkTextBuffer *source_buffer;
    GtkTextBuffer *result_buffer;
    GtkWidget *final_result_label;  
    char *current_file_path;
    TokenList tokenList;
    ErrorList errorList;
    SymbolTable symbolTable;
} AppWidgets;

// Load file content into source text view
void load_file_content(AppWidgets *widgets, const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        gtk_text_buffer_set_text(widgets->result_buffer, "Error: Cannot open file", -1);
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
    
    // Display in source text view
    gtk_text_buffer_set_text(widgets->source_buffer, content, -1);
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
        gtk_label_set_text(GTK_LABEL(widgets->file_path_label), label_text);
        
        // Store file path
        if (widgets->current_file_path) {
            free(widgets->current_file_path);
        }
        widgets->current_file_path = strdup(filename);
        
        // Load file content
        load_file_content(widgets, filename);
        
        // Clear result
        gtk_text_buffer_set_text(widgets->result_buffer, "File loaded. Click analysis buttons to proceed.", -1);
        
        g_free(filename);
    }
}

// Lexical Analysis Button Callback
void on_lexical_analysis(GtkWidget *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    if (!widgets->current_file_path) {
        gtk_text_buffer_set_text(widgets->result_buffer, "Please select a file first!", -1);
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
        "KEYWORD_BEGIN", "KEYWORD_END", "KEYWORD_INT", "KEYWORD_REAL",
        "KEYWORD_STRING", "IDENTIFIER", "INTEGER_LITERAL", "FLOAT_LITERAL",
        "STRING_LITERAL", "ASSIGN_OP", "END_INSTRUCTION", "COMMENT",
        "CONDITION", "ELSE", "REPEAT", "UNTIL"
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
    gtk_text_buffer_set_text(widgets->result_buffer, result, -1);
}

// Syntax Analysis Button Callback
void on_syntax_analysis(GtkWidget *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    if (!widgets->current_file_path) {
        gtk_text_buffer_set_text(widgets->result_buffer, "Please select a file first!", -1);
        return;
    }
    
    // Reset and run lexical first
    widgets->tokenList = (TokenList){NULL, 0, 0};
    widgets->errorList = (ErrorList){NULL, 0, 0};
    widgets->symbolTable = (SymbolTable){NULL, 0, 0};
    
    lexer(widgets->current_file_path, &widgets->tokenList, &widgets->errorList);
    
    // Run parser (syntax analysis)
    Parser parser;
    init_parser(&parser, &widgets->tokenList, &widgets->symbolTable, &widgets->errorList);
    parse(&parser);
    
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
        strcat(result, "- Starts with FRG_Begin#\n");
        strcat(result, "- Ends with FRG_End#\n");
        strcat(result, "- All instructions properly terminated with #\n");
    } else {
        sprintf(temp, "\nTotal Syntax Errors: %d\n", error_count);
        strcat(result, temp);
    }
    
    // Display result
    gtk_text_buffer_set_text(widgets->result_buffer, result, -1);
}

// Semantic Analysis Button Callback
void on_semantic_analysis(GtkWidget *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    if (!widgets->current_file_path) {
        gtk_text_buffer_set_text(widgets->result_buffer, "Please select a file first!", -1);
        return;
    }
    
    // Reset and run full analysis
    widgets->tokenList = (TokenList){NULL, 0, 0};
    widgets->errorList = (ErrorList){NULL, 0, 0};
    widgets->symbolTable = (SymbolTable){NULL, 0, 0};
    
    lexer(widgets->current_file_path, &widgets->tokenList, &widgets->errorList);
    
    Parser parser;
    init_parser(&parser, &widgets->tokenList, &widgets->symbolTable, &widgets->errorList);
    parse(&parser);
    
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
    gtk_text_buffer_set_text(widgets->result_buffer, result, -1);
}

// Create GUI
void create_gui(AppWidgets *widgets) {
    // Main window
    widgets->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(widgets->window), "FROG Compiler");
    gtk_window_set_default_size(GTK_WINDOW(widgets->window), 1000, 700);
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
    
    // Text views section
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
    
    // Initial message
    gtk_text_buffer_set_text(widgets->result_buffer, 
                            "Welcome to FROG Compiler!\n\n"
                            "Please select a .frg file to begin analysis.", -1);

    widgets->final_result_label = gtk_label_new("No analysis performed yet.");
    gtk_label_set_xalign(GTK_LABEL(widgets->final_result_label), 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), widgets->final_result_label, FALSE, FALSE, 0);
    }

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    AppWidgets widgets = {0};
    widgets.current_file_path = NULL;
    
    create_gui(&widgets);
    gtk_widget_show_all(widgets.window);
    gtk_main();
    
    if (widgets.current_file_path) {
        free(widgets.current_file_path);
    }
    
    return 0;
}