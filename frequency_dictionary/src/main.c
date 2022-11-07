#include <frequency_dictionary.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

void print_help() {
    printf("OPTIONS:\n"
           "\t-f, --file Input file name\n"
           "USAGE:\n"
           "\twords_count [-f file name]\n");
}

int main(int argc, char** argv) {
    int opt_char = 0;
    int opt_idx = 0;
    const char* file_name = NULL;
    struct option opts[] = {
        {"help", no_argument, 0, 'h'},
        {"file", required_argument, 0, 'f'},
        {0, 0, 0, 0},
    };

    while (-1 != (opt_char = getopt_long(argc, argv, ":hf:", opts, &opt_idx))) {
        switch (opt_char) {
            case 'h': {
                print_help();
                exit(EXIT_SUCCESS);
            }
            case 'f': {
                file_name = optarg;
                break;
            }
            case ':': {
                fprintf(stderr, "missing argument for '%c' option\n\n", optopt);
                print_help();
                exit(EXIT_FAILURE);
            }
            case '?': {
                fprintf(stderr, "unknown option: '%c'\n\n", optopt);
                print_help();
                exit(EXIT_FAILURE);
            }
        }
    }

    FILE* fp = fopen(file_name, "r");
    if (NULL == fp) {
        fprintf(stderr, "unable to open file: %s\n", file_name);
        exit(EXIT_FAILURE);
    }
    frequency_dictionary* fd = create_frequency_dictionary(fp);
    if (NULL == fd) {
        fprintf(stderr, "unable to create frequency dictionary\n");
        exit(EXIT_FAILURE);
    }
    frequency_dictionary_print(fd);
    remove_frequency_dictionary(&fd);
    fclose(fp);
    exit(EXIT_SUCCESS);
}
