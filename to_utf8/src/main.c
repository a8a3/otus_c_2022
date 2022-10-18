#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum encodings { undefined = -1, cp1251 = 1, koi8r, iso88595 } encodings_e;

const size_t k_encoded_buf_sz = 1024;
const size_t k_utf8_buf_sz = k_encoded_buf_sz << 1;

void print_help() {
    printf("OPTIONS:\n"
           "\t-i, --input    Input encoded file name\n"
           "\t-e, --encoding Input file encoding(1- cp1251, 2- koi8r, 3- iso88595)\n"
           "\t-o, --output   Output file name in utf8\n"
           "USAGE:\n"
           "\tto_utf8 [-i input file name] [-e input file encoding] [-o output file in utf8]\n");
}

const short cp1251_utf8[128] = {
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0x410, 0x411, 0x412, 0x413, 0x414, 0x415, 0x416, 0x417, 0x418, 0x419, 0x41a, 0x41b, 0x41c, 0x41d, 0x41e, 0x41f,
    0x420, 0x421, 0x422, 0x423, 0x424, 0x425, 0x426, 0x427, 0x428, 0x429, 0x42a, 0x42b, 0x42c, 0x42d, 0x42e, 0x42f,
    0x430, 0x431, 0x432, 0x433, 0x434, 0x435, 0x436, 0x437, 0x438, 0x439, 0x43a, 0x43b, 0x43c, 0x43d, 0x43e, 0x43f,
    0x440, 0x441, 0x442, 0x443, 0x444, 0x445, 0x446, 0x447, 0x448, 0x449, 0x44a, 0x44b, 0x44c, 0x44d, 0x44e, 0x44f};

const short koi8r_utf8[128] = {
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0x44e, 0x430, 0x431, 0x446, 0x434, 0x435, 0x444, 0x433, 0x445, 0x438, 0x439, 0x43a, 0x43b, 0x43c, 0x43d, 0x43e,
    0x43f, 0x44f, 0x440, 0x441, 0x442, 0x443, 0x436, 0x432, 0x44c, 0x44b, 0x437, 0x448, 0x44d, 0x449, 0x447, 0x44a,
    0x42e, 0x410, 0x411, 0x426, 0x414, 0x415, 0x424, 0x413, 0x425, 0x418, 0x419, 0x41a, 0x41b, 0x41c, 0x41d, 0x41e,
    0x41f, 0x42f, 0x420, 0x421, 0x422, 0x423, 0x416, 0x412, 0x42c, 0x42b, 0x417, 0x428, 0x42d, 0x429, 0x427, 0x42a};

const short iso88595_utf8[128] = {
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0x410, 0x411, 0x412, 0x413, 0x414, 0x415, 0x416, 0x417, 0x418, 0x419, 0x41A, 0x41B, 0x41C, 0x41D, 0x41E, 0x41F,
    0x420, 0x421, 0x422, 0x423, 0x424, 0x425, 0x426, 0x427, 0x428, 0x429, 0x42A, 0x42B, 0x42C, 0x42D, 0x42E, 0x42F,
    0x430, 0x431, 0x432, 0x433, 0x434, 0x435, 0x436, 0x437, 0x438, 0x439, 0x43A, 0x43B, 0x43C, 0x43D, 0x43E, 0x43F,
    0x440, 0x441, 0x442, 0x443, 0x444, 0x445, 0x446, 0x447, 0x448, 0x449, 0x44A, 0x44B, 0x44C, 0x44D, 0x44E, 0x44F,
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0};

int main(int argc, char** argv) {
    int opt_char = 0;
    int opt_idx = 0;
    const char* input_file_name = NULL;
    const char* output_file_name = NULL;
    encodings_e encoding = undefined;

    struct option opts[] = {
        {"help", no_argument, 0, 'h'},
        {"input", required_argument, 0, 'i'},
        {"encoding", required_argument, 0, 'e'},
        {"output", required_argument, 0, 'o'},
        {0, 0, 0, 0},
    };

    while (-1 != (opt_char = getopt_long(argc, argv, ":hi:e:o:", opts, &opt_idx))) {
        switch (opt_char) {
            case 'h': {
                print_help();
                exit(EXIT_SUCCESS);
            }
            case 'i': {
                input_file_name = optarg;
                break;
            }
            case 'e': {
                encoding = atoi(optarg);
                if (encoding < 1 || encoding > 3) {
                    fprintf(stderr, "unsupported encoding: %d\n\n", encoding);
                    print_help();
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 'o': {
                output_file_name = optarg;
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

    FILE* in_fp = fopen(input_file_name, "r");
    if (NULL == in_fp) {
        fprintf(stderr, "unable to open file: %s\n", input_file_name);
        exit(EXIT_FAILURE);
    }

    FILE* out_fp = fopen(output_file_name, "w+");

    char encoded_buf[k_encoded_buf_sz];
    char utf8_buf[k_utf8_buf_sz];

    while (NULL != fgets(encoded_buf, sizeof encoded_buf, in_fp)) {
        size_t i = 0, j = 0;
        for (; encoded_buf[i] != '\0'; ++i) {
            unsigned char encoded_symbol = encoded_buf[i];
            assert(encoded_symbol <= 0xFF);

            if (encoded_symbol < 0x80) { // ascii symbol
                utf8_buf[j++] = encoded_symbol;
            } else {
                encoded_symbol -= 128;
                short utf8_code = 0;
                switch (encoding) {
                    case cp1251:
                        utf8_code = cp1251_utf8[encoded_symbol];
                        break;
                    case koi8r:
                        utf8_code = koi8r_utf8[encoded_symbol];
                        break;
                    case iso88595:
                        utf8_code = iso88595_utf8[encoded_symbol];
                        break;
                    case undefined:
                        break;
                }
                utf8_buf[j++] = 0xC0 | (utf8_code >> 6);
                utf8_buf[j++] = 0x80 | (utf8_code & 0x3f);
            }
        }
        utf8_buf[j] = '\0';
        fputs(utf8_buf, out_fp);
    }
    fclose(in_fp);
    fclose(out_fp);
    exit(EXIT_SUCCESS);
}
