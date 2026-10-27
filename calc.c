// Full name: Fuad Salahli
// Student ID: 241ADB065
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Claude Ai for tokenizer position tracking, file/directory handling logic. Geek for Geeks for general information.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define STUDENT_NAME "Fuad"
#define STUDENT_LAST "Salahli"
#define STUDENT_ID   "241ADB065"

typedef enum {
    TOK_NUM,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_POW,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_EOF,
    TOK_INVALID
} TokenType;

typedef struct {
    TokenType type;
    long long int_val;
    double float_val;
    int is_float;
    size_t pos; 
} Token;

typedef struct {
    const char *src;
    size_t len;
    size_t pos; 
} Lexer;

void lexer_init(Lexer *l, const char *src, size_t len, size_t start_pos) {
    l->src = src;
    l->len = len;
    l->pos = start_pos;
}

Token lexer_next(Lexer *l) {
    while (l->pos < l->len) {
        char c = l->src[l->pos];
        size_t current_pos = l->pos + 1;

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            l->pos++;
            continue;
        }

        if (isdigit(c) || c == '.') {
            size_t start_idx = l->pos;
            int has_dot = 0;
            int has_digits_before_dot = 0;
            int has_digits_after_dot = 0;
            int invalid_float = 0;
            size_t invalid_pos = current_pos;

            if (isdigit(c)) {
                has_digits_before_dot = 1;
                while (l->pos < l->len && isdigit(l->src[l->pos])) {
                    l->pos++;
                }
            }

            if (l->pos < l->len && l->src[l->pos] == '.') {
                has_dot = 1;
                invalid_pos = l->pos + 1;
                l->pos++;
                if (l->pos < l->len && isdigit(l->src[l->pos])) {
                    has_digits_after_dot = 1;
                    while (l->pos < l->len && isdigit(l->src[l->pos])) {
                        l->pos++;
                    }
                }
            }

            if (l->pos < l->len && (l->src[l->pos] == 'e' || l->src[l->pos] == 'E')) {
                invalid_pos = l->pos + 1;
                invalid_float = 1;
                while (l->pos < l->len && !isspace((unsigned char)l->src[l->pos]) && 
                       l->src[l->pos] != '+' && l->src[l->pos] != '-' && 
                       l->src[l->pos] != '*' && l->src[l->pos] != '/' && 
                       l->src[l->pos] != '(' && l->src[l->pos] != ')') {
                    l->pos++;
                }
            }

            if (!has_digits_before_dot && has_dot && !has_digits_after_dot) {
                Token t = {TOK_INVALID, 0, 0.0, 0, current_pos};
                return t;
            }

            if (has_dot && (!has_digits_before_dot || !has_digits_after_dot)) {
                Token t = {TOK_INVALID, 0, 0.0, 0, invalid_pos};
                return t;
            }

            if (invalid_float) {
                Token t = {TOK_INVALID, 0, 0.0, 0, invalid_pos};
                return t;
            }

            size_t token_len = l->pos - start_idx;
            char *num_str = (char *)malloc(token_len + 1);
            memcpy(num_str, l->src + start_idx, token_len);
            num_str[token_len] = '\0';

            Token t;
            t.pos = current_pos;
            if (has_dot) {
                t.type = TOK_NUM;
                t.is_float = 1;
                t.float_val = atof(num_str);
            } else {
                t.type = TOK_NUM;
                t.is_float = 0;
                t.int_val = atoll(num_str);
            }
            free(num_str);
            return t;
        }

        if (c == '+') { l->pos++; Token t = {TOK_PLUS, 0, 0.0, 0, current_pos}; return t; }
        if (c == '-') { l->pos++; Token t = {TOK_MINUS, 0, 0.0, 0, current_pos}; return t; }
        if (c == '/') { l->pos++; Token t = {TOK_DIV, 0, 0.0, 0, current_pos}; return t; }
        if (c == '(') { l->pos++; Token t = {TOK_LPAREN, 0, 0.0, 0, current_pos}; return t; }
        if (c == ')') { l->pos++; Token t = {TOK_RPAREN, 0, 0.0, 0, current_pos}; return t; }

        if (c == '*') {
            l->pos++;
            if (l->pos < l->len && l->src[l->pos] == '*') {
                l->pos++;
                Token t = {TOK_POW, 0, 0.0, 0, current_pos};
                return t;
            }
            Token t = {TOK_MUL, 0, 0.0, 0, current_pos};
            return t;
        }

        l->pos++;
        Token t = {TOK_INVALID, 0, 0.0, 0, current_pos};
        return t;
    }

    Token t = {TOK_EOF, 0, 0.0, 0, l->len + 1};
    return t;
}

typedef struct {
    int is_float;
    long long i_val;
    double f_val;
} Value;

typedef struct {
    Lexer lexer;
    Token current;
    int error_occurred;
    size_t error_pos;
    size_t line_end_pos;
    size_t file_len;
} Parser;

void parser_error(Parser *p, size_t pos) {
    if (!p->error_occurred) {
        p->error_occurred = 1;
        p->error_pos = pos;
    }
}

Value parse_expr(Parser *p);
Value parse_term(Parser *p);
Value parse_power(Parser *p);
Value parse_primary(Parser *p);

Value parse_primary(Parser *p) {
    Value val = {0, 0, 0.0};
    if (p->error_occurred) return val;

    if (p->current.type == TOK_NUM) {
        val.is_float = p->current.is_float;
        if (val.is_float) val.f_val = p->current.float_val;
        else val.i_val = p->current.int_val;
        p->current = lexer_next(&p->lexer);
        return val;
    } else if (p->current.type == TOK_LPAREN) {
        p->current = lexer_next(&p->lexer);
        val = parse_expr(p);
        if (p->error_occurred) return val;
        if (p->current.type == TOK_RPAREN) {
            p->current = lexer_next(&p->lexer);
            return val;
        } else {
            size_t err_pos = (p->current.type == TOK_EOF) ? p->file_len + 1 : p->current.pos;
            parser_error(p, err_pos);
            return val;
        }
    } else {
        size_t err_pos = (p->current.type == TOK_EOF) ? p->file_len + 1 : p->current.pos;
        parser_error(p, err_pos);
        return val;
    }
}

Value parse_power(Parser *p) {
    Value left = parse_primary(p);
    if (p->error_occurred) return left;

    if (p->current.type == TOK_POW) {
        size_t pow_pos = p->current.pos;
        p->current = lexer_next(&p->lexer);
        size_t right_pos = (p->current.type == TOK_EOF) ? p->file_len + 1 : p->current.pos;
        Value right = parse_power(p);
        if (p->error_occurred) return left;

        if (!left.is_float && !right.is_float && right.i_val < 0) {
            parser_error(p, right_pos);
            return left;
        }

        int res_float = left.is_float || right.is_float;
        if (res_float) {
            double b = left.is_float ? left.f_val : (double)left.i_val;
            double e = right.is_float ? right.f_val : (double)right.i_val;
            double res = pow(b, e);
            if (isinf(res) || isnan(res)) {
                parser_error(p, pow_pos);
                return left;
            }
            left.is_float = 1;
            left.f_val = res;
        } else {
            long long b = left.i_val;
            long long e = right.i_val;
            if (e == 0) {
                left.is_float = 0;
                left.i_val = 1;
            } else {
                long long res = 1;
                for (long long i = 0; i < e; i++) res *= b;
                left.is_float = 0;
                left.i_val = res;
            }
        }
    }
    return left;
}

Value parse_term(Parser *p) {
    Value left = parse_power(p);
    if (p->error_occurred) return left;

    while (p->current.type == TOK_MUL || p->current.type == TOK_DIV) {
        TokenType op = p->current.type;
        p->current = lexer_next(&p->lexer);
        size_t right_pos = (p->current.type == TOK_EOF) ? p->file_len + 1 : p->current.pos;
        Value right = parse_power(p);
        if (p->error_occurred) return left;

        if (op == TOK_MUL) {
            if (left.is_float || right.is_float) {
                double l_val = left.is_float ? left.f_val : (double)left.i_val;
                double r_val = right.is_float ? right.f_val : (double)right.i_val;
                left.is_float = 1;
                left.f_val = l_val * r_val;
            } else {
                left.i_val = left.i_val * right.i_val;
            }
        } else {
            if (right.is_float) {
                if (right.f_val == 0.0) {
                    parser_error(p, right_pos);
                    return left;
                }
                double l_val = left.is_float ? left.f_val : (double)left.i_val;
                left.is_float = 1;
                left.f_val = l_val / right.f_val;
            } else {
                if (right.i_val == 0) {
                    parser_error(p, right_pos);
                    return left;
                }
                if (left.is_float) {
                    left.f_val = left.f_val / (double)right.i_val;
                } else {
                    left.i_val = left.i_val / right.i_val;
                }
            }
        }
    }
    return left;
}

Value parse_expr(Parser *p) {
    Value left = parse_term(p);
    if (p->error_occurred) return left;

    while (p->current.type == TOK_PLUS || p->current.type == TOK_MINUS) {
        TokenType op = p->current.type;
        p->current = lexer_next(&p->lexer);
        Value right = parse_term(p);
        if (p->error_occurred) return left;

        if (op == TOK_PLUS) {
            if (left.is_float || right.is_float) {
                double lb = left.is_float ? left.f_val : (double)left.i_val;
                double rb = right.is_float ? right.f_val : (double)right.i_val;
                left.is_float = 1;
                left.f_val = lb + rb;
            } else {
                left.i_val = left.i_val + right.i_val;
            }
        } else {
            if (left.is_float || right.is_float) {
                double lb = left.is_float ? left.f_val : (double)left.i_val;
                double rb = right.is_float ? right.f_val : (double)right.i_val;
                left.is_float = 1;
                left.f_val = lb - rb;
            } else {
                left.i_val = left.i_val - right.i_val;
            }
        }
    }
    return left;
}

void process_file(const char *in_path, const char *out_path) {
    FILE *fin = fopen(in_path, "rb");
    if (!fin) {
        fprintf(stderr, "Error: cannot open input file %s\n", in_path);
        exit(1);
    }

    fseek(fin, 0, SEEK_END);
    long fsize = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    char *buf = (char *)malloc(fsize + 1);
    size_t read_bytes = fread(buf, 1, fsize, fin);
    (void)read_bytes;
    buf[fsize] = '\0';
    fclose(fin);

    FILE *fout = fopen(out_path, "w");
    if (!fout) {
        fprintf(stderr, "Error: cannot open output file %s\n", out_path);
        free(buf);
        exit(1);
    }

    size_t line_start = 0;
    while ((long)line_start < fsize) {
        size_t line_end = line_start;
        while ((long)line_end < fsize && buf[line_end] != '\n') {
            line_end++;
        }

        int is_blank = 1;
        int is_comment = 0;
        size_t first_non_space = line_start;

        for (size_t i = line_start; i < line_end; i++) {
            if (!isspace((unsigned char)buf[i])) {
                is_blank = 0;
                first_non_space = i;
                break;
            }
        }

        if (!is_blank && buf[first_non_space] == '#') {
            is_comment = 1;
        }

        if (is_blank || is_comment) {
            line_start = line_end + 1;
            continue;
        }

        int has_invalid_hash = 0;
        size_t hash_pos = 0;
        for (size_t i = line_start; i < line_end; i++) {
            if (buf[i] == '#') {
                has_invalid_hash = 1;
                hash_pos = i + 1;
                break;
            }
        }

        if (has_invalid_hash) {
            fprintf(fout, "ERROR:%zu\n", hash_pos);
            line_start = line_end + 1;
            continue;
        }

        Parser p;
        lexer_init(&p.lexer, buf, fsize, line_start);
        p.error_occurred = 0;
        p.error_pos = 0;
        p.line_end_pos = line_end + 1;
        p.file_len = fsize;
        p.current = lexer_next(&p.lexer);

        Value res = parse_expr(&p);

        if (!p.error_occurred && p.current.type != TOK_EOF && p.current.pos <= line_end + 1) {
            p.error_occurred = 1;
            p.error_pos = p.current.pos;
        }

        if (p.error_occurred) {
            fprintf(fout, "ERROR:%zu\n", p.error_pos);
        } else {
            if (res.is_float) {
                fprintf(fout, "%.15g\n", res.f_val);
            } else {
                fprintf(fout, "%lld\n", res.i_val);
            }
        }

        line_start = line_end + 1;
    }

    free(buf);
    fclose(fout);
}

int main(int argc, char *argv[]) {
    char *dir_arg = NULL;
    char *out_arg = NULL;
    char *input_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (i + 1 < argc) dir_arg = argv[++i];
            else { fprintf(stderr, "Error: missing argument for dir\n"); return 1; }
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (i + 1 < argc) out_arg = argv[++i];
            else { fprintf(stderr, "Error: missing argument for output-dir\n"); return 1; }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: unknown option %s\n", argv[i]);
            return 1;
        } else {
            input_file = argv[i];
        }
    }

    if (!dir_arg && !input_file) {
        fprintf(stderr, "Error: no input file or directory specified\n");
        return 1;
    }

    if (dir_arg) {
        DIR *d = opendir(dir_arg);
        if (!d) {
            fprintf(stderr, "Error: cannot open directory %s\n", dir_arg);
            return 1;
        }

        const char *last_slash = strrchr(dir_arg, '/');
        const char *dir_base = last_slash ? last_slash + 1 : dir_arg;
        if (*dir_base == '\0') dir_base = dir_arg;

        char outdir_buf[512];
        if (out_arg) {
            strncpy(outdir_buf, out_arg, sizeof(outdir_buf) - 1);
            outdir_buf[sizeof(outdir_buf) - 1] = '\0';
        } else {
            snprintf(outdir_buf, sizeof(outdir_buf), "%s_%s", dir_base, STUDENT_NAME);
        }

        struct stat st = {0};
        if (stat(outdir_buf, &st) == -1) {
            mkdir(outdir_buf, 0777);
        }

        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            char *name = dir->d_name;
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

            char in_path[1024];
            snprintf(in_path, sizeof(in_path), "%s/%s", dir_arg, name);

            struct stat st_file;
            if (stat(in_path, &st_file) == 0 && S_ISREG(st_file.st_mode)) {
                size_t len = strlen(name);
                if (len > 4 && strcmp(name + len - 4, ".txt") == 0) {
                    char out_path[1024];
                    char base_name[256];
                    size_t copy_len = len - 4;
                    if (copy_len >= sizeof(base_name)) copy_len = sizeof(base_name) - 1;
                    memcpy(base_name, name, copy_len);
                    base_name[copy_len] = '\0';

                    snprintf(out_path, sizeof(out_path), "%s/%s_%s.txt", outdir_buf, base_name, STUDENT_NAME);
                    process_file(in_path, out_path);
                }
            }
        }
        closedir(d);
    } else {
        char base_name[256];
        const char *last_slash = strrchr(input_file, '/');
        const char *fname = last_slash ? last_slash + 1 : input_file;}
