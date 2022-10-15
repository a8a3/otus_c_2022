#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CD stands for Central Directory
#define CD_END_SIGNATURE 0x06054b50
#define CD_ENTRY_SIGNATURE 0x02014b50

#define CD_START_OFFSET 16
#define FILE_NAME_LEN_OFFSET 28
#define EXTRA_FIELD_LEN_OFFSET 30
#define FILE_COMMENT_LEN_OFFSET 32
#define FILE_NAME_OFFSET 46

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: jpeg_zip <file_name>\n");
        exit(EXIT_FAILURE);
    }
    const char* file_name = argv[1];
    FILE* fp = fopen(file_name, "rb");

    if (NULL == fp) {
        printf("unable to open file: %s\n", file_name);
        exit(EXIT_FAILURE);
    }

    // find cd end signature
    uint32_t cd_end_signature;
    fseek(fp, -1 * (long)sizeof cd_end_signature, SEEK_END);
    do {
        fread(&cd_end_signature, sizeof cd_end_signature, 1, fp);
    } while (CD_END_SIGNATURE != cd_end_signature &&
             0 == fseek(fp, -1 * (long)(sizeof cd_end_signature + 1), SEEK_CUR));

    if (cd_end_signature == CD_END_SIGNATURE) { // cd end signature found
        printf("files list:\n");

        long cd_end_pos = ftell(fp) - (long)sizeof cd_end_signature;

        uint32_t cd_start_offset_val;
        fseek(fp, cd_end_pos + CD_START_OFFSET, SEEK_SET); // move to cd start offset value
        fread(&cd_start_offset_val, sizeof cd_start_offset_val, 1, fp);
        fseek(fp, cd_start_offset_val, SEEK_SET); // move to cd start

        uint16_t entry_idx = 0;
        uint32_t cd_entry_signature;
        uint16_t file_name_len;
        uint16_t extra_field_len;
        uint16_t file_comment_len;
        char file_name_buf[1024] = {0};

        // find first entry
        do {
            fread(&cd_entry_signature, sizeof cd_entry_signature, 1, fp);
        } while (CD_ENTRY_SIGNATURE != cd_entry_signature &&
                 0 == fseek(fp, -1 * (long)(sizeof cd_entry_signature - 1), SEEK_CUR));
        assert(cd_entry_signature == CD_ENTRY_SIGNATURE);
        fseek(fp, -1 * (long)sizeof cd_entry_signature, SEEK_CUR);

        do { // iterate over cd entries
            fread(&cd_entry_signature, sizeof cd_entry_signature, 1, fp);

            if (cd_entry_signature == CD_ENTRY_SIGNATURE) {
                fseek(fp, FILE_NAME_LEN_OFFSET - sizeof cd_entry_signature, SEEK_CUR); // move to file name len record
                fread(&file_name_len, sizeof file_name_len, 1, fp);                    // read file name len
                fread(&extra_field_len, sizeof extra_field_len, 1, fp);
                fread(&file_comment_len, sizeof file_comment_len, 1, fp);

                int file_name_current_offset = FILE_NAME_OFFSET - FILE_NAME_LEN_OFFSET - sizeof file_name_len -
                                               sizeof extra_field_len - sizeof file_comment_len;
                fseek(fp, file_name_current_offset, SEEK_CUR); // move to file name record

                fread(file_name_buf, file_name_len, 1, fp); // read file name
                file_name_buf[file_name_len] = '\0';
                fseek(fp, extra_field_len + file_comment_len, SEEK_CUR); // move to current entry tail

                if (file_name_buf[file_name_len - 1] != '/') { // skip directory
                    printf("file[%u]: %s\n", entry_idx, file_name_buf);
                    ++entry_idx;
                }
            }
        } while (cd_entry_signature != CD_END_SIGNATURE);
        printf("files in archive: %u\n", entry_idx);
    } else {
        printf("it is not a jpeg_zip file\n");
    }
    fclose(fp);
    exit(EXIT_SUCCESS);
}
