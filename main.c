#include "myPreCompiler.h"
#include <getopt.h>

#define MAX_FILES 50

static void print_usage(const char *prog) {
    printf("Uso: %s -i <file_input> [-o <file_output>] [-v]\n", prog);
}

int main(int argc, char *argv[]) {
    char *input_files[MAX_FILES];
    int file_count = 0;
    char *output_file = NULL;
    bool verbose = false;

    static struct option long_opts[] = {
        {"in", required_argument, 0, 'i'},
        {"out", required_argument, 0, 'o'},
        {"verbose", no_argument, 0, 'v'},
        {0,0,0,0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i:o:v", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'i':
                input_files[file_count++] = optarg;
                while (optind < argc && argv[optind][0] != '-' && file_count < MAX_FILES)
                    input_files[file_count++] = argv[optind++];
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (file_count == 0) {
        fprintf(stderr, "ERRORE: nessun file di input.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    FILE *out_fp = stdout;
    if (output_file) {
        out_fp = fopen(output_file, "w");
        if (!out_fp) {
            perror("Errore apertura file output");
            return EXIT_FAILURE;
        }
    } else {
        verbose = true;
    }

    Stats stats;
    init_stats(&stats);

    for (int i = 0; i < file_count; i++) {
        FILE *in_fp = fopen(input_files[i], "r");
        if (!in_fp) {
            fprintf(stderr, "Errore apertura file '%s'\n", input_files[i]);
            continue;
        }

        fseek(in_fp, 0, SEEK_END);
        long size = ftell(in_fp);
        if (size <= 0) {
            fprintf(stderr, "File '%s' vuoto o non leggibile\n", input_files[i]);
            fclose(in_fp);
            continue;
        }
        rewind(in_fp);

        process_file(in_fp, input_files[i], &stats);
        // Controllo errore di lettura da file
        if (ferror(in_fp)) {
            fprintf(stderr, "ERRORE: durante la lettura dal file '%s'\n", input_files[i]);
        }

        // Controllo errore chiusura file
        if (fclose(in_fp) != 0) {
            fprintf(stderr, "ERRORE: chiusura file '%s' fallita\n", input_files[i]);
        }
    }

    print_statistics(out_fp, &stats, verbose);

    free_stats(&stats);

    if (out_fp != stdout) {
        if (fclose(out_fp) != 0) {
            perror("ERRORE: chiusura file di output fallita");
        }
    }

    return EXIT_SUCCESS;
}
