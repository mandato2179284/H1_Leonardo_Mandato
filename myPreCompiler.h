#ifndef MYPRECOMPILER_H
#define MYPRECOMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_VAR_LEN 100
#define MAX_LINE_LEN 512
#define MAX_ERR_MSG 256

typedef struct {
    char name[MAX_VAR_LEN];
    char type[MAX_VAR_LEN];
    int line_declared;
    bool used;
} Variable;

typedef struct {
    int line_number;
    char filename[MAX_VAR_LEN];
    char message[MAX_ERR_MSG];
} ErrorLog;

typedef struct {
    int total_vars_checked;
    int total_errors;
    int unused_vars;
    int invalid_var_names;
    int invalid_types;

    Variable *vars;
    int var_count;
    int var_capacity;

    ErrorLog *errors;
    int err_count;
    int err_capacity;

    char **typedefs_names;
    int typedefs_count;
    int typedefs_capacity;
} Stats;

void init_stats(Stats *stats);
void free_stats(Stats *stats);
void add_variable(Stats *stats, const char *type, const char *name, int line);
void add_error(Stats *stats, int line, const char *filename, const char *msg, int err_type);
bool is_valid_identifier(const char *str);
void process_file(FILE *in_fp, const char *filename, Stats *stats);
void print_statistics(FILE *out_fp, Stats *stats, bool verbose);

#endif
