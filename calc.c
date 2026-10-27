// Nisa Belen Gun 241ADB181
// https://github.com/nisabelen/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: course instructions, ChatGPT for understanding parser structure and code review.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define STUDENT_NAME "nisa_belen_gun_241adb181"

// AI (ChatGPT) prompt:
// "How can I add simple error position tracking to my + - * / parser?"

void skip_spaces(char line[], int *i) {
    while (line[*i] == ' ' || line[*i] == '\t') {
        (*i)++;
    }
}

int read_number(char line[], int *i, long long line_start,
                long long *error_pos, long long *number,
                int *number_start) {

    skip_spaces(line, i);

    *number_start = *i;

    // burada sayı başlamıyorsa hata
    if (line[*i] < '0' || line[*i] > '9') {
        *error_pos = line_start + *i;
        return -1;
    }

    *number = 0;

    while (line[*i] >= '0' && line[*i] <= '9') {
        *number = (*number * 10) + (line[*i] - '0');
        (*i)++;
    }

    return 0;
}

int calculate_term(char line[], int *i, long long line_start,
                   long long *error_pos, long long *result) {

    long long number;
    int number_start;

    if (read_number(line, i, line_start,
                    error_pos, &number, &number_start) != 0) {
        return -1;
    }

    *result = number;

    while (1) {
        skip_spaces(line, i);

        if (line[*i] != '*' && line[*i] != '/') {
            break;
        }

        char operation = line[*i];
        (*i)++;

        if (read_number(line, i, line_start,
                        error_pos, &number, &number_start) != 0) {
            return -1;
        }

        // sıfıra bölme
        if (operation == '/' && number == 0) {
            *error_pos = line_start + number_start;
            return -1;
        }

        if (operation == '*') {
            *result = *result * number;
        } else {
            *result = *result / number;
        }
    }

    return 0;
}

int calculate_line(char line[], long long line_start,
                   long long *error_pos, long long *answer) {

    int i = 0;
    long long result;

    if (calculate_term(line, &i, line_start,
                       error_pos, &result) != 0) {
        return -1;
    }

    while (1) {
        skip_spaces(line, &i);

        if (line[i] == '\n' || line[i] == '\0') {
            break;
        }

        // burada artık sadece + veya - olabilir
        if (line[i] != '+' && line[i] != '-') {
            *error_pos = line_start + i;
            return -1;
        }

        char operation = line[i];
        i++;

        long long number;

        if (calculate_term(line, &i, line_start,
                           error_pos, &number) != 0) {
            return -1;
        }

        if (operation == '+') {
            result = result + number;
        } else {
            result = result - number;
        }
    }

    *answer = result;

    return 0;
}

int is_blank_line(char line[]) {
    int i = 0;

    while (line[i] == ' ' ||
           line[i] == '\t' ||
           line[i] == '\r') {
        i++;
    }

    if (line[i] == '\n' || line[i] == '\0') {
        return 1;
    }

    return 0;
}

void get_file_name(char input[], char output[]) {
    int start = 0;
    int i = 0;
    int j = 0;

    while (input[i] != '\0') {
        if (input[i] == '/') {
            start = i + 1;
        }
        i++;
    }

    i = start;

    while (input[i] != '\0') {

        if (input[i] == '.' &&
            input[i + 1] == 't' &&
            input[i + 2] == 'x' &&
            input[i + 3] == 't') {
            break;
        }

        output[j] = input[i];

        i++;
        j++;
    }

    output[j] = '\0';
}

int main(int argc, char *argv[]) {
    FILE *input;
    FILE *output;

    char line[4096];
    char file_name[200];
    char output_path[500];

    if (argc != 4 || strcmp(argv[1], "-o") != 0) {
        fprintf(stderr, "Invalid arguments.\n");
        return 1;
    }

    char *output_folder = argv[2];
    char *input_file = argv[3];

    input = fopen(input_file, "r");

    if (input == NULL) {
        fprintf(stderr, "Cannot open input file.\n");
        return 1;
    }

    if (mkdir(output_folder, 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "Cannot create output folder.\n");
        fclose(input);
        return 1;
    }

    get_file_name(input_file, file_name);

    snprintf(
        output_path,
        sizeof(output_path),
        "%s/%s_%s.txt",
        output_folder,
        file_name,
        STUDENT_NAME
    );

    long long position = 1;
    long long error_pos = 0;
    int has_error = 0;

    /*
     * AI (ChatGPT) prompt:
     * "How can I make sure an error file contains only ERROR:position
     * and no results from previous lines?"
     *
     * Önce dosyayı kontrol ediyoruz.
     */
    while (fgets(line, sizeof(line), input) != NULL) {

        if (!is_blank_line(line)) {
            long long answer;

            if (calculate_line(
                    line,
                    position,
                    &error_pos,
                    &answer) != 0) {

                has_error = 1;
                break;
            }
        }

        // newline dahil bütün karakterleri say
        position = position + (long long)strlen(line);
    }

    output = fopen(output_path, "w");

    if (output == NULL) {
        fprintf(stderr, "Cannot open output file.\n");
        fclose(input);
        return 1;
    }

    if (has_error) {

        // hata varsa sadece bunu yaz
        fprintf(output, "ERROR:%lld\n", error_pos);

    } else {

        // hata yoksa dosyanın başına dön
        rewind(input);

        position = 1;

        while (fgets(line, sizeof(line), input) != NULL) {

            if (!is_blank_line(line)) {

                long long answer;

                if (calculate_line(
                        line,
                        position,
                        &error_pos,
                        &answer) == 0) {

                    fprintf(output, "%lld\n", answer);
                }
            }

            position = position + (long long)strlen(line);
        }
    }

    fclose(input);
    fclose(output);

    return 0;
}
