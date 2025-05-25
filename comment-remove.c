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
        .multi_start = "'''",
        .multi_end = "'''",
        .extensions = {".py", NULL}
    },
    {  
        .single_line = "#",
        .multi_start = "",
        .multi_end = "",
        .extensions = {".sh", ".pl", ".rb", ".bash", NULL}
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
    long len = ftell(in);
    fseek(in, 0, SEEK_SET);

    char *buffer = malloc(len + 1);
    if (!buffer) {
        perror("Memory allocation failed");
        fclose(in);
        return 1;
    }

    fread(buffer, 1, len, in);
    buffer[len] = '\0';
    fclose(in);

    char *output = malloc(len + 1);
    int output_len = 0;
    bool comments_found = false;
    bool in_comment = false;
    bool in_string = false;
    bool in_char = false;
    bool escape = false;

    for (int i = 0; i < len; i++) {
        if (!in_comment && !in_string && !in_char) {
            if (rules->multi_start[0] != '\0' &&
                strncmp(&buffer[i], rules->multi_start, strlen(rules->multi_start)) == 0) {
                in_comment = true;
                comments_found = true;
                i += strlen(rules->multi_start) - 1;
                continue;
            }

            if (rules->single_line[0] != '\0' &&
                strncmp(&buffer[i], rules->single_line, strlen(rules->single_line)) == 0) {
                comments_found = true;
                while (i < len && buffer[i] != '\n') i++;
                if (i < len) output[output_len++] = buffer[i];
                continue;
            }
        }

        if (!in_comment && !escape) {
            if (buffer[i] == '"' && !in_char) {
                in_string = !in_string;
            }
            else if (buffer[i] == '\'' && !in_string) {
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

        if (in_comment && rules->multi_end[0] != '\0' &&
            strncmp(&buffer[i], rules->multi_end, strlen(rules->multi_end)) == 0) {
            in_comment = false;
            i += strlen(rules->multi_end) - 1;
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
    system(diff_cmd);

    printf("\nReplace original file? [Y/n] ");
    char choice;
    scanf(" %c", &choice);

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
