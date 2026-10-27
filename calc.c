// Nisa Belen Gun 241ADB181
// https://github.com/nisabelen/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: https://github.com/WarpRules/recursive_descent_parser_tutorial/blob/main/recursive_descent_parser_tutorial.c
// https://en.cppreference.com/c/numeric/math/pow

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define FIRST_NAME "nisa_belen"
#define LAST_NAME  "gun"
#define STUDENT_ID "241adb181"

typedef struct {
    int is_float;
    long long i;
    double d;
} Value;

typedef struct {
    const char *text;
    size_t pos;
    size_t end;
    size_t file_len;
    int has_newline;
    long long error;
} Parser;

Value make_int(long long x) {
    Value v = {0, x, (double)x};
    return v;
}

Value make_float(double x) {
    Value v = {1, 0, x};
    return v;
}

double value_as_double(Value v) {
    return v.is_float ? v.d : (double)v.i;
}

void skip_spaces(Parser *p) {
    while (p->pos < p->end &&
           (p->text[p->pos] == ' ' ||
            p->text[p->pos] == '\t' ||
            p->text[p->pos] == '\r')) {

        p->pos++;
    }
}

long long current_error_position(Parser *p) {
    if (p->pos < p->end) {
        return (long long)p->pos + 1;
    }

    // EOL hatası newline karakterinde EOF hatası file length + 1de
    if (p->has_newline) {
        return (long long)p->end + 1;
    }

    return (long long)p->file_len + 1;
}

void set_error(Parser *p) {
    if (p->error == 0) {
        p->error = current_error_position(p);
    }
}

int parse_expression(Parser *p, Value *result);

int parse_number(Parser *p, Value *result) {
    skip_spaces(p);

    if (p->pos >= p->end ||
        !isdigit((unsigned char)p->text[p->pos])) {

        set_error(p);
        return -1;
    }

    long long integer_part = 0;

    while (p->pos < p->end &&
           isdigit((unsigned char)p->text[p->pos])) {

        integer_part =
            integer_part * 10 + (p->text[p->pos] - '0');

        p->pos++;
    }

    if (p->pos < p->end &&
        p->text[p->pos] == '.') {

        size_t dot_pos = p->pos;
        p->pos++;

        if (p->pos >= p->end ||
            !isdigit((unsigned char)p->text[p->pos])) {

            p->error = (long long)dot_pos + 1;
            return -1;
        }

        double number = (double)integer_part;
        double place = 0.1;

        while (p->pos < p->end &&
               isdigit((unsigned char)p->text[p->pos])) {

            number +=
                (p->text[p->pos] - '0') * place;

            place /= 10.0;
            p->pos++;
        }

        *result = make_float(number);
        return 0;
    }

    *result = make_int(integer_part);

    return 0;
}

int parse_primary(Parser *p,
                  Value *result,
                  size_t *start_pos) {

    skip_spaces(p);

    *start_pos = p->pos;

    // parantez
    if (p->pos < p->end &&
        p->text[p->pos] == '(') {

        p->pos++;

        if (parse_expression(p, result) != 0) {
            return -1;
        }

        skip_spaces(p);

        if (p->pos >= p->end ||
            p->text[p->pos] != ')') {

            set_error(p);
            return -1;
        }

        p->pos++;

        return 0;
    }

    return parse_number(p, result);
}

long long integer_power(long long base,
                        long long exponent) {
    long long result = 1;

    while (exponent > 0) {
        result = result * base;
        exponent--;
    }
    return result;
}

int parse_power(Parser *p,
                Value *result,
                size_t *start_pos) {

    if (parse_primary(
            p,
            result,
            start_pos) != 0) {

        return -1;
    }
    skip_spaces(p);

    if (p->pos + 1 < p->end &&
        p->text[p->pos] == '*' &&
        p->text[p->pos + 1] == '*') {

        size_t operator_pos = p->pos;
        p->pos += 2;
        skip_spaces(p);
        size_t exponent_pos = p->pos;

        Value exponent;
        size_t exponent_start;

        //sağdan
        if (parse_power(
                p,
                &exponent,
                &exponent_start) != 0) {

            return -1;
        }

        if (!result->is_float &&
            !exponent.is_float) {

            if (exponent.i < 0) {
                p->error =
                    (long long)exponent_pos + 1;

                return -1;
            }

            result->i =
                integer_power(
                    result->i,
                    exponent.i
                );
            result->d =
                (double)result->i;
        }

        // herhangi biri float ise pow(
        else {
            double answer =
                pow(
                    value_as_double(*result),
                    value_as_double(exponent)
                );

            if (!isfinite(answer)) {
                p->error =
                    (long long)operator_pos + 1;

                return -1;
            }

            *result = make_float(answer);
        }
    }

    return 0;
}

int parse_term(Parser *p,
               Value *result) {

    size_t left_start;

    if (parse_power(
            p,
            result,
            &left_start) != 0) {

        return -1;
    }

    while (1) {
        skip_spaces(p);

        if (p->pos >= p->end) {
            break;
        }

        if (p->text[p->pos] == '*' &&
            p->pos + 1 < p->end &&
            p->text[p->pos + 1] == '*') {

            break;
        }

        if (p->text[p->pos] != '*' &&
            p->text[p->pos] != '/') {

            break;
        }

        char operation =
            p->text[p->pos];

        p->pos++;

        skip_spaces(p);

        size_t right_start =
            p->pos;

        Value right;
        size_t dummy;

        if (parse_power(
                p,
                &right,
                &dummy) != 0) {

            return -1;
        }

        if (operation == '/') {
            int zero;

            if (right.is_float) {
                zero = (right.d == 0.0);
            } else {
                zero = (right.i == 0);
            }

            if (zero) {
                p->error =
                    (long long)right_start + 1;

                return -1;
            }
        }

        if (!result->is_float &&
            !right.is_float) {

            if (operation == '*') {
                result->i =
                    result->i * right.i;
            } else {
                result->i =
                    result->i / right.i;
            }

            result->d =
                (double)result->i;
        }

        // float varsa doubl
        else {
            double answer;

            if (operation == '*') {
                answer =
                    value_as_double(*result) *
                    value_as_double(right);
            } else {
                answer =
                    value_as_double(*result) /
                    value_as_double(right);
            }

            if (!isfinite(answer)) {
                p->error =
                    (long long)right_start + 1;

                return -1;
            }

            *result =
                make_float(answer);
        }
    }

    return 0;
}

int parse_expression(Parser *p,
                     Value *result) {

    if (parse_term(p, result) != 0) {
        return -1;
    }

    while (1) {
        skip_spaces(p);

        if (p->pos >= p->end ||
            p->text[p->pos] == ')') {

            break;
        }

        if (p->text[p->pos] != '+' &&
            p->text[p->pos] != '-') {

            set_error(p);
            return -1;
        }

        char operation =
            p->text[p->pos];

        p->pos++;

        Value right;

        if (parse_term(
                p,
                &right) != 0) {

            return -1;
        }

        // integers
        if (!result->is_float &&
            !right.is_float) {

            if (operation == '+') {
                result->i =
                    result->i + right.i;
            } else {
                result->i =
                    result->i - right.i;
            }

            result->d =
                (double)result->i;
        }

        // float varsa
        else {
            double answer;

            if (operation == '+') {
                answer =
                    value_as_double(*result) +
                    value_as_double(right);
            } else {
                answer =
                    value_as_double(*result) -
                    value_as_double(right);
            }

            if (!isfinite(answer)) {
                set_error(p);
                return -1;
            }

            *result =
                make_float(answer);
        }
    }

    return 0;
}

int parse_line(const char *text,
               size_t start,
               size_t end,
               size_t file_len,
               int has_newline,
               Value *result,
               long long *error) {

    Parser p;

    p.text = text;
    p.pos = start;
    p.end = end;
    p.file_len = file_len;
    p.has_newline = has_newline;
    p.error = 0;

    if (parse_expression(
            &p,
            result) != 0) {

        *error = p.error;
        return -1;
    }

    skip_spaces(&p);

    if (p.pos != p.end) {
        *error =
            (long long)p.pos + 1;

        return -1;
    }

    return 0;
}

char *read_file(const char *filename,
                size_t *length) {

    FILE *fp =
        fopen(filename, "rb");

    if (fp == NULL) {
        return NULL;
    }

    size_t capacity = 4096;
    size_t used = 0;

    char *data =
        malloc(capacity + 1);

    if (data == NULL) {
        fclose(fp);
        return NULL;
    }

    while (1) {

        if (used == capacity) {

            capacity =
                capacity * 2;

            char *new_data =
                realloc(
                    data,
                    capacity + 1
                );

            if (new_data == NULL) {
                free(data);
                fclose(fp);
                return NULL;
            }

            data = new_data;
        }

        size_t got =
            fread(
                data + used,
                1,
                capacity - used,
                fp
            );

        used += got;

        if (got == 0) {

            if (ferror(fp)) {
                free(data);
                fclose(fp);
                return NULL;
            }

            break;
        }
    }

    data[used] = '\0';

    fclose(fp);

    *length = used;

    return data;
}

int blank_or_comment(const char *text,
                     size_t start,
                     size_t end) {

    size_t i = start;

    while (i < end &&
           (text[i] == ' ' ||
            text[i] == '\t' ||
            text[i] == '\r')) {

        i++;
    }

    if (i == end) {
        return 1;
    }
    if (text[i] == '#') {
        return 1;
    }

    return 0;
}

void print_value(FILE *fp,
                 Value value) {

        if (value.is_float) {
        fprintf(
            fp,
            "%.15g\n",
            value.d
        );
    } else {
        fprintf(
            fp,
            "%lld\n",
            value.i
        );
    }
}

void get_base_name(const char *path,
                   char result[],
                   size_t size) {

    const char *name =
        strrchr(path, '/');

    if (name == NULL) {
        name = path;
    } else {
        name++;
    }

    snprintf(
        result,
        size,
        "%s",
        name
    );

    size_t len =
        strlen(result);

    if (len >= 4 &&
        strcmp(
            result + len - 4,
            ".txt") == 0) {

        result[len - 4] = '\0';
    }
}

void get_dir_name(const char *path,
                  char result[],
                  size_t size) {

    char temp[1024];

    snprintf(
        temp,
        sizeof(temp),
        "%s",
        path
    );

    size_t len =
        strlen(temp);

    // folder sonunda / varsa kaldır gitainn
    while (len > 0 &&
           temp[len - 1] == '/') {

        temp[len - 1] = '\0';
        len--;
    }
    const char *name =
        strrchr(temp, '/');

    if (name == NULL) {
        name = temp;
    } else {
        name++;
    }

    snprintf(
        result,
        size,
        "%s",
        name
    );
}

int make_folder(const char *folder) {

    if (mkdir(folder, 0777) == 0) {
        return 0;
    }

    if (errno == EEXIST) {

        struct stat info;

        if (stat(folder, &info) == 0 &&
            S_ISDIR(info.st_mode)) {

            return 0;
        }
    }
    return -1;
}
int evaluate_file(const char *input_file,
                  const char *output_dir) {

    size_t length = 0;

    char *data =
        read_file(
            input_file,
            &length
        );

    if (data == NULL) {
        fprintf(
            stderr,
            "Cannot read input file: %s\n",
            input_file
        );

        return -1;
    }

    long long first_error = 0;

    size_t start = 0;

    while (start < length) {

        size_t end = start;

        while (end < length &&
               data[end] != '\n') {

            end++;
        }

        int has_newline =
            (end < length &&
             data[end] == '\n');

        if (!blank_or_comment(
                data,
                start,
                end)) {

            Value value;
            long long error = 0;

            if (parse_line(
                    data,
                    start,
                    end,
                    length,
                    has_newline,
                    &value,
                    &error) != 0) {

                first_error = error;
                break;
            }
        }

        if (has_newline) {
            start = end + 1;
        } else {
            start = end;
        }
    }

    char base[256];
    char output_path[1400];

    get_base_name(
        input_file,
        base,
        sizeof(base)
    );

    snprintf(
        output_path,
        sizeof(output_path),
        "%s/%s_%s_%s_%s.txt",
        output_dir,
        base,
        FIRST_NAME,
        LAST_NAME,
        STUDENT_ID
    );

    FILE *out =
        fopen(
            output_path,
            "w"
        );

    if (out == NULL) {
        fprintf(
            stderr,
            "Cannot write output file.\n"
        );

        free(data);

        return -1;
    }

    // hata ERROR işte
    if (first_error != 0) {

        fprintf(
            out,
            "ERROR:%lld\n",
            first_error
        );

        fclose(out);
        free(data);

        return 0;
    }

    start = 0;

    while (start < length) {

        size_t end = start;

        while (end < length &&
               data[end] != '\n') {

            end++;
        }

        int has_newline =
            (end < length &&
             data[end] == '\n');

        if (!blank_or_comment(
                data,
                start,
                end)) {

            Value value;
            long long error = 0;

            if (parse_line(
                    data,
                    start,
                    end,
                    length,
                    has_newline,
                    &value,
                    &error) == 0) {

                print_value(
                    out,
                    value
                );
            }
        }

        if (has_newline) {
            start = end + 1;
        } else {
            start = end;
        }
    }

    fclose(out);
    free(data);

    return 0;
}

int is_txt_file(const char *name) {

    size_t len =
        strlen(name);

    if (len >= 4 &&
        strcmp(
            name + len - 4,
            ".txt") == 0) {

        return 1;
    }

    return 0;
}

int process_directory(const char *input_dir,
                      const char *output_dir) {

    DIR *dir =
        opendir(input_dir);

    if (dir == NULL) {
        fprintf(
            stderr,
            "Cannot open input directory: %s\n",
            input_dir
        );

        return -1;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        if (!is_txt_file(entry->d_name)) {
            continue;
        }

        char input_path[1400];

        snprintf(
            input_path,
            sizeof(input_path),
            "%s/%s",
            input_dir,
            entry->d_name
        );

        struct stat info;

        if (stat(input_path, &info) != 0 ||
            !S_ISREG(info.st_mode)) {

            continue;
        }

        if (evaluate_file(
                input_path,
                output_dir) != 0) {

            closedir(dir);
            return -1;
        }
    }

    closedir(dir);

    return 0;
}

void usage(const char *program) {

    fprintf(
        stderr,
        "Usage: %s [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n",
        program
    );
}

int main(int argc,
         char *argv[]) {

    const char *input_file = NULL;
    const char *input_dir = NULL;
    const char *output_dir = NULL;

    for (int i = 1;
         i < argc;
         i++) {

        if (strcmp(argv[i], "-d") == 0 ||
            strcmp(argv[i], "--dir") == 0) {

            if (i + 1 >= argc ||
                input_dir != NULL) {

                usage(argv[0]);
                return 1;
            }

            input_dir =
                argv[++i];
        }

        else if (strcmp(argv[i], "-o") == 0 ||
                 strcmp(argv[i], "--output-dir") == 0) {

            if (i + 1 >= argc ||
                output_dir != NULL) {

                usage(argv[0]);
                return 1;
            }

            output_dir =
                argv[++i];
        }

        else if (argv[i][0] == '-') {

            fprintf(
                stderr,
                "Unknown option: %s\n",
                argv[i]
            );

            return 1;
        }

        else {

            if (input_file != NULL) {
                usage(argv[0]);
                return 1;
            }

            input_file =
                argv[i];
        }
    }

    // Ya file ya da directory 
    if ((input_file == NULL &&
         input_dir == NULL) ||
        (input_file != NULL &&
         input_dir != NULL)) {

        usage(argv[0]);
        return 1;
    }

    char default_output[1024];

    if (output_dir == NULL) {

        char base[256];

        if (input_file != NULL) {

            get_base_name(
                input_file,
                base,
                sizeof(base)
            );
        }

        else {

            get_dir_name(
                input_dir,
                base,
                sizeof(base)
            );
        }

        snprintf(
            default_output,
            sizeof(default_output),
            "%s_%s_%s",
            base,
            FIRST_NAME,
            STUDENT_ID
        );

        output_dir =
            default_output;
    }

    if (make_folder(output_dir) != 0) {

        fprintf(
            stderr,
            "Cannot create output directory: %s\n",
            output_dir
        );

        return 1;
    }

    if (input_file != NULL) {

        if (evaluate_file(
                input_file,
                output_dir) != 0) {

            return 1;
        }

        return 0;
    }

    if (process_directory(
            input_dir,
            output_dir) != 0) {

        return 1;
    }

    return 0;
}
