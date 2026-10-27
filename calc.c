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

long long read_number(char line[], int *i) {
    long long number = 0;

    while (line[*i] == ' ' || line[*i] == '\t') {
        (*i)++;
    }

    while (line[*i] >= '0' && line[*i] <= '9') {
        number = number * 10 + (line[*i] - '0');
        (*i)++;
    }

    return number;
}

long long calculate_term(char line[], int *i) {
    long long result = read_number(line, i);

    while (1) {
        while (line[*i] == ' ' || line[*i] == '\t') {
            (*i)++;
        }

        if (line[*i] != '*' && line[*i] != '/') {
            break;
        }

        char operation = line[*i];
        (*i)++;

        long long number = read_number(line, i);

        if (operation == '*') {
            result = result * number;
        } else {
            result = result / number;
        }
    }

    return result;
}

long long calculate_line(char line[]) {
    int i = 0;

    long long result = calculate_term(line, &i);

    while (1) {
        while (line[i] == ' ' || line[i] == '\t') {
            i++;
        }

        if (line[i] == '\n' || line[i] == '\0') {
            break;
        }

        char operation = line[i];
        i++;

        long long number = calculate_term(line, &i);

        if (operation == '+') {
            result = result + number;
        } else if (operation == '-') {
            result = result - number;
        }
    }

    return result;
}

void get_file_name(char input[], char output[]) {
    int start = 0;
    int i = 0;
    int j = 0;

    // klasör varsa sadece dosya adını bul
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

    char line[1000];
    char file_name[200];
    char output_path[500];

    // Şimdilik sadece: ./calc -o results input.txt
    if (argc != 4) {
        fprintf(stderr, "Invalid arguments.\n");
        return 1;
    }

    if (strcmp(argv[1], "-o") != 0) {
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

    // output klasörü yoksa
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

    output = fopen(output_path, "w");

    if (output == NULL) {
        fprintf(stderr, "Cannot open output file.\n");
        fclose(input);
        return 1;
    }

    while (fgets(line, sizeof(line), input) != NULL) {

        if (line[0] == '\n' || line[0] == '\0') {
            continue;
        }

        long long answer = calculate_line(line);

        fprintf(output, "%lld\n", answer);
    }

    fclose(input);
    fclose(output);

    return 0;
}
