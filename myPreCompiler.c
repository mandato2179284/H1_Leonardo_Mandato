#include "myPreCompiler.h"

typedef enum { GLOBAL_DECLS, LOCAL_DECLS, BODY } Phase;

void init_stats(Stats *stats) {
    memset(stats, 0, sizeof(Stats));
    stats->var_capacity = 10;
    stats->vars = malloc(stats->var_capacity * sizeof(Variable));
    stats->err_capacity = 10;
    stats->errors = malloc(stats->err_capacity * sizeof(ErrorLog));
    stats->typedefs_capacity = 10;
    stats->typedefs_names = malloc(stats->typedefs_capacity * sizeof(char *));
}

void free_stats(Stats *stats) {
    free(stats->vars);
    free(stats->errors);
    for (int i = 0; i < stats->typedefs_count; i++)
        free(stats->typedefs_names[i]);
    free(stats->typedefs_names);
}

void add_error(Stats *stats, int line, const char *filename, const char *msg, int err_type) {
    if (stats->err_count >= stats->err_capacity) {
        stats->err_capacity *= 2;
        stats->errors = realloc(stats->errors, stats->err_capacity * sizeof(ErrorLog));
    }
    stats->errors[stats->err_count].line_number = line;
    strncpy(stats->errors[stats->err_count].filename, filename, MAX_VAR_LEN - 1);
    stats->errors[stats->err_count].filename[MAX_VAR_LEN - 1] = '\0';
    strncpy(stats->errors[stats->err_count].message, msg, MAX_ERR_MSG - 1);
    stats->errors[stats->err_count].message[MAX_ERR_MSG - 1] = '\0';
    stats->err_count++;
    stats->total_errors++;

    if (err_type == 1) stats->invalid_types++;
    else if (err_type == 2) stats->invalid_var_names++;
    else if (err_type == 3) stats->unused_vars++;
}

void add_variable(Stats *stats, const char *type, const char *name, int line) {
    if (stats->var_count >= stats->var_capacity) {
        stats->var_capacity *= 2;
        stats->vars = realloc(stats->vars, stats->var_capacity * sizeof(Variable));
    }
    strncpy(stats->vars[stats->var_count].type, type, MAX_VAR_LEN - 1);
    stats->vars[stats->var_count].type[MAX_VAR_LEN - 1] = '\0';
    strncpy(stats->vars[stats->var_count].name, name, MAX_VAR_LEN - 1);
    stats->vars[stats->var_count].name[MAX_VAR_LEN - 1] = '\0';
    stats->vars[stats->var_count].line_declared = line;
    stats->vars[stats->var_count].used = false;

    stats->var_count++;
    stats->total_vars_checked++;
}

static void add_typedef(Stats *stats, const char *name) {
    if (stats->typedefs_count >= stats->typedefs_capacity) {
        stats->typedefs_capacity *= 2;
        stats->typedefs_names = realloc(stats->typedefs_names,
                                        stats->typedefs_capacity * sizeof(char *));
    }
    stats->typedefs_names[stats->typedefs_count] = malloc(strlen(name) + 1);
    strcpy(stats->typedefs_names[stats->typedefs_count], name);
    stats->typedefs_count++;
}

static bool is_typedef_name(Stats *stats, const char *name) {
    for (int i = 0; i < stats->typedefs_count; i++)
        if (strcmp(stats->typedefs_names[i], name) == 0) return true;
    return false;
}

/* Riconosce le keyword di tipo C standard, incluso "struct" come prefisso di tipo */
static bool is_type_keyword(const char *tok) {
    return strcmp(tok, "int")      == 0 || strcmp(tok, "char")     == 0 ||
           strcmp(tok, "float")    == 0 || strcmp(tok, "double")   == 0 ||
           strcmp(tok, "short")    == 0 || strcmp(tok, "long")     == 0 ||
           strcmp(tok, "signed")   == 0 || strcmp(tok, "unsigned") == 0 ||
           strcmp(tok, "void")     == 0 || strcmp(tok, "struct")   == 0;
}

bool is_valid_identifier(const char *str) {
    if (!str || !*str) return false;
    if (!isalpha((unsigned char)str[0]) && str[0] != '_') return false;
    for (int i = 1; str[i]; i++)
        if (!isalnum((unsigned char)str[i]) && str[i] != '_') return false;
    return true;
}

/*
 * Determina se la riga "cleaned" e' una dichiarazione di variabile.
 * Le righe con '{' sono definizioni di struct/blocchi, non dichiarazioni.
 */
static bool is_declaration_line(Stats *stats, const char *cleaned) {
    char temp[MAX_LINE_LEN];
    strcpy(temp, cleaned);
    char *tok1 = strtok(temp, " \t;");
    if (!tok1) return false;

    if (strcmp(tok1, "typedef") == 0 || is_type_keyword(tok1) ||
        is_typedef_name(stats, tok1))
        return true;

    if (strcmp(tok1, "return") == 0 || strcmp(tok1, "goto")   == 0 ||
        strcmp(tok1, "if")     == 0 || strcmp(tok1, "while")  == 0 ||
        strcmp(tok1, "for")    == 0 || strcmp(tok1, "printf") == 0 ||
        strcmp(tok1, "switch") == 0 || strcmp(tok1, "do")     == 0)
        return false;

    /* Assegnazioni, chiamate a funzione o aperture di blocco non sono dichiarazioni */
    if (strchr(cleaned, '=') || strchr(cleaned, '(') || strchr(cleaned, '{'))
        return false;

    char *tok2 = strtok(NULL, " \t;=,");
    if (!tok2) return false;

    return (isalpha((unsigned char)tok2[0]) || tok2[0] == '_' || tok2[0] == '*');
}

/*
 * Estrae il nome del typedef dalla riga di chiusura di un typedef struct.
 * Esempio: "} NomeTipo;"  ->  aggiunge "NomeTipo" come typedef.
 */
static void extract_typedef_from_closing(Stats *stats, const char *cleaned,
                                         int line_number, const char *filename) {
    char after[MAX_LINE_LEN];
    strcpy(after, cleaned + 1); /* salta il '}' */

    char *p = after;
    while (*p == ' ' || *p == '\t') p++;

    char *semi = strchr(p, ';');
    if (semi) *semi = '\0';

    char *end = p + strlen(p) - 1;
    while (end > p && (*end == ' ' || *end == '\t')) *end-- = '\0';

    if (strlen(p) == 0) return;

    if (is_valid_identifier(p)) {
        add_typedef(stats, p);
    } else {
        char msg[MAX_ERR_MSG];
        snprintf(msg, MAX_ERR_MSG, "Nome typedef struct non valido: '%s'", p);
        add_error(stats, line_number, filename, msg, 2);
    }
}

void process_file(FILE *in_fp, const char *filename, Stats *stats) {
    char line[MAX_LINE_LEN], cleaned[MAX_LINE_LEN];
    int line_number = 0;
    bool in_block_comment  = false;
    bool seen_main         = false;
    bool in_struct_block   = false;
    bool is_typedef_struct = false;

    Phase phase = GLOBAL_DECLS;
    int start_var_index = stats->var_count;

    while (fgets(line, sizeof(line), in_fp)) {
        line_number++;
        int w = 0;

        /* 1. Pulizia commenti */
        for (int r = 0; line[r]; r++) {
            if (line[r] == '\n' || line[r] == '\r') continue;
            if (!in_block_comment && line[r] == '/' && line[r+1] == '*') {
                in_block_comment = true; r++; continue;
            }
            if (in_block_comment && line[r] == '*' && line[r+1] == '/') {
                in_block_comment = false; r++; continue;
            }
            if (!in_block_comment && line[r] == '/' && line[r+1] == '/') break;
            if (!in_block_comment) cleaned[w++] = line[r];
        }
        cleaned[w] = '\0';

        /* 2. Trim spazi */
        int shift = 0;
        while (cleaned[shift] == ' ' || cleaned[shift] == '\t') shift++;
        if (shift > 0) memmove(cleaned, cleaned + shift, strlen(cleaned) - shift + 1);

        int len = strlen(cleaned);
        while (len > 0 && (cleaned[len-1] == ' ' || cleaned[len-1] == '\t')) {
            cleaned[len-1] = '\0'; len--;
        }

        /* 3. Righe vuote, direttive, graffe isolate */
        if (cleaned[0] == '\0' || cleaned[0] == '#') continue;

        if (cleaned[0] == '{' && cleaned[1] == '\0') continue;

        if (cleaned[0] == '}' && cleaned[1] == '\0') {
            if (in_struct_block) {
                in_struct_block   = false;
                is_typedef_struct = false;
            }
            continue;
        }

        /* 4. Corpo della struct: saltiamo i membri */
        if (in_struct_block) {
            if (cleaned[0] == '}') {
                if (is_typedef_struct)
                    extract_typedef_from_closing(stats, cleaned, line_number, filename);
                in_struct_block   = false;
                is_typedef_struct = false;
            }
            continue;
        }

        /* 5. Rilevamento del main */
        if (!seen_main && strstr(cleaned, "main")) {
            seen_main = true;
            phase = LOCAL_DECLS;
            continue;
        }

        /* 6. Definizione di struct pura (non typedef): "struct Tag {" */
        if (strncmp(cleaned, "struct", 6) == 0 &&
            (cleaned[6] == ' ' || cleaned[6] == '\t') &&
            strchr(cleaned, '{')) {
            in_struct_block   = true;
            is_typedef_struct = false;
            continue;
        }

        /* 7. Gestione typedef */
        if (strncmp(cleaned, "typedef", 7) == 0) {

            /* typedef struct con corpo { */
            if (strstr(cleaned + 7, "struct") && strchr(cleaned, '{')) {
                if (strchr(cleaned, '}')) {
                    /* Tutto su una riga: typedef struct { int x; } Nome; */
                    char temp_td[MAX_LINE_LEN];
                    strcpy(temp_td, cleaned);
                    char *semi = strrchr(temp_td, ';');
                    if (semi) *semi = '\0';
                    char *last = NULL, *t_tok = strtok(temp_td, " \t{}");
                    while (t_tok) { last = t_tok; t_tok = strtok(NULL, " \t{}"); }
                    if (last) {
                        if (is_valid_identifier(last))
                            add_typedef(stats, last);
                        else {
                            char msg[MAX_ERR_MSG];
                            snprintf(msg, MAX_ERR_MSG, "Nome typedef struct non valido: '%s'", last);
                            add_error(stats, line_number, filename, msg, 2);
                        }
                    }
                } else {
                    /* Multi-riga: il corpo della struct segue nelle righe successive */
                    in_struct_block   = true;
                    is_typedef_struct = true;
                }
                continue;
            }

            /* typedef semplice (incluso "typedef struct Tag Alias;") */
            {
                char temp_td[MAX_LINE_LEN];
                strcpy(temp_td, cleaned);
                char *p    = temp_td + 7;
                char *last = NULL;
                char *t_tok = strtok(p, " \t;");
                while (t_tok) { last = t_tok; t_tok = strtok(NULL, " \t;"); }
                if (last) {
                    if (is_valid_identifier(last))
                        add_typedef(stats, last);
                    else {
                        char msg[MAX_ERR_MSG];
                        sprintf(msg, "Nome typedef non valido: '%s'", last);
                        add_error(stats, line_number, filename, msg, 2);
                    }
                }
            }
            continue;
        }

        /* 8. Fase dichiarazioni */
        if (phase == GLOBAL_DECLS || phase == LOCAL_DECLS) {
            if (!is_declaration_line(stats, cleaned)) {
                if (phase == LOCAL_DECLS) phase = BODY;
                else continue;
            }

            if (phase != BODY) {
                char temp[MAX_LINE_LEN];
                strcpy(temp, cleaned);
                char *ptr = temp;
                char type[MAX_VAR_LEN] = "";
                bool type_ok = true;

                /* Parsing del tipo */
                while (*ptr) {
                    while (*ptr == ' ' || *ptr == '\t') ptr++;
                    if (!*ptr) break;

                    char word[MAX_VAR_LEN];
                    int i = 0;
                    while (ptr[i] && (isalnum((unsigned char)ptr[i]) || ptr[i] == '_')) {
                        word[i] = ptr[i]; i++;
                    }
                    word[i] = '\0';

                    if (i == 0) {
                        if (*ptr == '*') {
                            if (type[0]) strcat(type, " *");
                            else         strcpy(type, "*");
                            ptr++; continue;
                        } else break;
                    }

                    if (is_type_keyword(word) || is_typedef_name(stats, word)) {
                        if (type[0]) strcat(type, " ");
                        strcat(type, word);
                        ptr += i;

                        /*
                         * "struct" e' seguito dal tag name: lo incorporiamo nel tipo.
                         * Es. "struct Point p;" -> type = "struct Point"
                         */
                        if (strcmp(word, "struct") == 0) {
                            while (*ptr == ' ' || *ptr == '\t') ptr++;
                            char tag[MAX_VAR_LEN];
                            int ti = 0;
                            while (ptr[ti] &&
                                   (isalnum((unsigned char)ptr[ti]) || ptr[ti] == '_')) {
                                tag[ti] = ptr[ti]; ti++;
                            }
                            tag[ti] = '\0';
                            if (ti > 0) {
                                strcat(type, " ");
                                strcat(type, tag);
                                ptr += ti;
                            }
                        }
                    } else {
                        if (type[0] == '\0') {
                            strcpy(type, word);
                            type_ok = false;
                            ptr += i;
                        }
                        break;
                    }
                }

                if (!type_ok) {
                    char msg[MAX_ERR_MSG];
                    sprintf(msg, "Tipo di dato non valido: '%s'", type);
                    add_error(stats, line_number, filename, msg, 1);
                }

                char *v_tok = strtok(ptr, ",;");
                while (v_tok) {
                    char *start = v_tok;
                    while (*start == ' ' || *start == '\t' || *start == '*') start++;

                    for (int k = 0; start[k]; k++) {
                        if (start[k] == '=' || start[k] == '[') {
                            start[k] = '\0'; break;
                        }
                    }

                    char *end = start + strlen(start) - 1;
                    while (end > start && (*end == ' ' || *end == '\t')) *end-- = '\0';

                    if (strlen(start) > 0) {
                        if (!is_valid_identifier(start)) {
                            char msg[MAX_ERR_MSG];
                            sprintf(msg, "Nome variabile non valido: '%s'", start);
                            add_error(stats, line_number, filename, msg, 2);
                        }
                        add_variable(stats, type, start, line_number);
                    }
                    v_tok = strtok(NULL, ",;");
                }
                continue;
            }
        }

        /* 9. Tracciamento utilizzo variabili nel BODY */
        if (phase == BODY) {
            for (int i = start_var_index; i < stats->var_count; i++) {
                if (!stats->vars[i].used) {
                    char *pos = cleaned;
                    while ((pos = strstr(pos, stats->vars[i].name))) {
                        char before = (pos == cleaned) ? ' ' : *(pos - 1);
                        char after  = *(pos + strlen(stats->vars[i].name));
                        if (!isalnum((unsigned char)before) && before != '_' &&
                            !isalnum((unsigned char)after)  && after  != '_') {
                            stats->vars[i].used = true;
                            break;
                        }
                        pos++;
                    }
                }
            }
        }
    }

    /* 10. Segnalazione variabili non usate */
    for (int i = start_var_index; i < stats->var_count; i++) {
        if (!stats->vars[i].used) {
            char msg[MAX_ERR_MSG];
            sprintf(msg, "Variabile non utilizzata: '%s'", stats->vars[i].name);
            add_error(stats, stats->vars[i].line_declared, filename, msg, 3);
        }
    }
}

void print_statistics(FILE *out_fp, Stats *stats, bool verbose) {
    FILE *streams[2];
    int n = 0;

    streams[n++] = out_fp;
    if (verbose && out_fp != stdout)
        streams[n++] = stdout;

    for (int i = 0; i < n; i++) {
        fprintf(streams[i], "\n===== STATISTICHE DI ELABORAZIONE =====\n");
        fprintf(streams[i], "Variabili controllate:        %d\n", stats->total_vars_checked);
        fprintf(streams[i], "Errori totali:                %d\n", stats->total_errors);
        fprintf(streams[i], "  - Variabili non usate:      %d\n", stats->unused_vars);
        fprintf(streams[i], "  - Nomi non validi:          %d\n", stats->invalid_var_names);
        fprintf(streams[i], "  - Tipi non validi:          %d\n", stats->invalid_types);

        if (stats->total_errors > 0) {
            fprintf(streams[i], "\nDETTAGLIO ERRORI:\n");
            for (int j = 0; j < stats->err_count; j++)
                fprintf(streams[i], "[%s | riga %d] %s\n",
                        stats->errors[j].filename,
                        stats->errors[j].line_number,
                        stats->errors[j].message);
        }
        fprintf(streams[i], "========================================\n");
    }
}
