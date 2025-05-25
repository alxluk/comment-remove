#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <strings.h>

#define _POSIX_C_SOURCE 200809L
#define DIFF_CMD_SIZE 8192

typedef struct {
    const char *single_line;
    const char *multi_start;
    const char *multi_end;
    const char *extensions[10];
} LangRules;

const LangRules languages[] = {
    {
        .single_line = "//",
        .multi_start = "/*",
        .multi_end = "*/",
        .extensions = {".c", ".h", ".cpp", ".hpp", ".cs", ".java", ".js", NULL}
    },
    {
        .single_line = "#",
        .multi_start = "",
        .multi_end = "",
        .extensions = {".py", NULL}
    },
    {
        .single_line = "#",
        .multi_start = "",
        .multi_end = "",
        .extensions = {".sh", ".bash", NULL}
    },
    {
        .single_line = "#",
        .multi_start = "=begin",
        .multi_end = "=end",
        .extensions = {".rb", NULL}
    },
    {
        .single_line = "#",
        .multi_start = "=pod",
        .multi_end = "=cut",
        .extensions = {".pl", NULL}
    },
    {
        .single_line = "",
        .multi_start = "<!--",
        .multi_end = "-->",
        .extensions = {".html", ".htm", ".xml", ".xhtml", NULL}
    },
    {
        .single_line = "",
        .multi_start = "/*",
        .multi_end = "*/",
        .extensions = {".css", NULL}
    },
    {
        .single_line = "//",
        .multi_start = "/*",
        .multi_end = "*/",
        .extensions = {".php", NULL}
    }
};

const LangRules* detect_language(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return NULL;

    for (size_t i = 0; i < sizeof(languages)/sizeof(languages[0]); i++) {
        for (int j = 0; languages[i].extensions[j]; j++) {
            if (strcasecmp(dot, languages[i].extensions[j]) == 0) {
                return &languages[i];
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const LangRules *rules = detect_language(input_file);
    if (!rules) {
        fprintf(stderr, "Unsupported file type\n");
        return 1;
    }

    char output_file[FILENAME_MAX];
    const char *dot = strrchr(input_file, '.');
    if (dot) {
        snprintf(output_file, sizeof(output_file), "%.*s_1%s",
                 (int)(dot - input_file), input_file, dot);
    } else {
        snprintf(output_file, sizeof(output_file), "%s_1", input_file);
    }

    FILE *in = fopen(input_file, "r");
    if (!in) {
        perror("Error opening input file");
        return 1;
    }

    fseek(in, 0, SEEK_END);
    long file_len = ftell(in);
    if (file_len == -1) {
        perror("Error determining file length");
        fclose(in);
        return 1;
    }
    fseek(in, 0, SEEK_SET);

    size_t buffer_size = (size_t)file_len;
    char *buffer = malloc(buffer_size + 1);
    if (!buffer) {
        perror("Memory allocation failed");
        fclose(in);
        return 1;
    }

    size_t bytes_read = fread(buffer, 1, buffer_size, in);
    if (bytes_read != buffer_size) {
        if (ferror(in)) {
            perror("Error reading input file");
        } else {
            fprintf(stderr, "Error: Could not read the entire file\n");
        }
        free(buffer);
        fclose(in);
        return 1;
    }
    buffer[bytes_read] = '\0';
    fclose(in);

    char *output = malloc(buffer_size + 1);
    size_t output_len = 0;
    bool comments_found = false;
    bool in_comment = false;
    bool in_string = false;
    bool in_char = false;
    bool escape = false;

    for (size_t i = 0; i < bytes_read; i++) {
        if (!in_comment && !in_string && !in_char) {
            size_t multi_start_len = strlen(rules->multi_start);
            if (multi_start_len > 0 &&
                i + multi_start_len <= bytes_read &&
                strncmp(&buffer[i], rules->multi_start, multi_start_len) == 0) {
                in_comment = true;
                comments_found = true;
                i += multi_start_len - 1; 
                continue;
            }

            size_t single_line_len = strlen(rules->single_line);
            if (single_line_len > 0 &&
                i + single_line_len <= bytes_read &&
                strncmp(&buffer[i], rules->single_line, single_line_len) == 0) {
                comments_found = true;
                
                while (i < bytes_read && buffer[i] != '\n') {
                    i++;
                }
                if (i < bytes_read) {
                    output[output_len++] = buffer[i];
                }
                continue;
            }
        }

        if (!in_comment && !escape) {
            if (buffer[i] == '"' && !in_char) {
                in_string = !in_string;
            } else if (buffer[i] == '\'' && !in_string) {
                in_char = !in_char;
            }
        }

        if ((in_string || in_char) && buffer[i] == '\\' && !escape) {
            escape = true;
        } else {
            escape = false;
        }

        if (!in_comment) {
            output[output_len++] = buffer[i];
        }

        if (in_comment) {
            size_t multi_end_len = strlen(rules->multi_end);
            if (multi_end_len > 0 &&
                i + multi_end_len <= bytes_read &&
                strncmp(&buffer[i], rules->multi_end, multi_end_len) == 0) {
                in_comment = false;
                i += multi_end_len - 1; 
            }
        }
    }

    if (!comments_found) {
        printf("No comments found in the code.\n");
        remove(output_file);
        free(buffer);
        free(output);
        return 0;
    }

    FILE *out = fopen(output_file, "w");
    if (!out) {
        perror("Error opening output file");
        free(buffer);
        free(output);
        return 1;
    }

    fwrite(output, 1, output_len, out);
    fclose(out);

    char diff_cmd[DIFF_CMD_SIZE];
        snprintf(diff_cmd, sizeof(diff_cmd),
                "diff -u --color=always \"%s\" \"%s\"", input_file, output_file);
        int unused = system(diff_cmd);
        (void)unused;

    printf("\nReplace original file? [Y/n] ");
    char choice = 'n';
    int result = scanf(" %c", &choice);
    if (result != 1) {
        choice = 'n';
    }

    if (tolower(choice) == 'y' || choice == '\n') {
        if (remove(input_file) != 0) {
            perror("Error removing original file");
        } else if (rename(output_file, input_file) != 0) {
            perror("Error renaming file");
        } else {
            printf("File successfully replaced.\n");
        }
    } else {
        if (remove(output_file) == 0) {
            printf("Temporary file removed: %s\n", output_file);
        } else {
            perror("Error removing temporary file");
        }
    }

    free(buffer);
    free(output);
    return 0;
}
