// Full name: Fuad Salahli
// Student ID: 241ADB065
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Claude Ai for tokenizer position tracking, file/directory handling logic. Geek for Geeks for general information.

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define STUDENT_NAME "Fuad"
#define STUDENT_LAST "Salahli"
#define STUDENT_ID   "241ADB065"

#define PATH_LEN 4096

/* ======================================================================== */
/* Tokenizer                                                                */
/* ======================================================================== */

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
    char *src;        
    size_t line_end;
    size_t pos;
} Lexer;

static int is_digit(char c) { return c >= '0' && c <= '9'; }

static Token make_tok(TokenType type, size_t pos)
{
    Token t;
    t.type = type;
    t.int_val = 0;
    t.float_val = 0.0;
    t.is_float = 0;
    t.pos = pos;
    return t;
}

void lexer_init(Lexer *l, char *src, size_t line_start, size_t line_end)
{
    l->src = src;
    l->pos = line_start;
    l->line_end = line_end;
}

Token lexer_next(Lexer *l)
{
    
    while (l->pos < l->line_end && (l->src[l->pos] == ' ' || l->src[l->pos] == '\t'))
        l->pos++;

    size_t at = l->pos + 1;
    if (l->pos >= l->line_end)
        return make_tok(TOK_EOF, at);

    char c = l->src[l->pos];

    if (is_digit(c)) {
        size_t start = l->pos;
        long long acc = 0;
        while (l->pos < l->line_end && is_digit(l->src[l->pos])) {
            acc = acc * 10 + (l->src[l->pos] - '0');
            l->pos++;
        }

     
        if (l->pos + 1 < l->line_end && l->src[l->pos] == '.' && is_digit(l->src[l->pos + 1])) {
            l->pos++;
            while (l->pos < l->line_end && is_digit(l->src[l->pos]))
                l->pos++;
            
            char saved = l->src[l->pos];
            l->src[l->pos] = '\0';
            Token t = make_tok(TOK_NUM, at);
            t.is_float = 1;
            t.float_val = strtod(l->src + start, NULL);
            l->src[l->pos] = saved;
            return t;
        }

        Token t = make_tok(TOK_NUM, at);
        t.int_val = acc;
        return t;
    }

    l->pos++;
    switch (c) {
    case '+': return make_tok(TOK_PLUS, at);
    case '-': return make_tok(TOK_MINUS, at);
    case '/': return make_tok(TOK_DIV, at);
    case '(': return make_tok(TOK_LPAREN, at);
    case ')': return make_tok(TOK_RPAREN, at);
    case '*':
        
        if (l->pos < l->line_end && l->src[l->pos] == '*') {
            l->pos++;
            return make_tok(TOK_POW, at);
        }
        return make_tok(TOK_MUL, at);
    default:
        
        return make_tok(TOK_INVALID, at);
    }
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
} Parser;


void parser_error(Parser *p, size_t pos)
{
    if (!p->error_occurred) {
        p->error_occurred = 1;
        p->error_pos = pos;
    }
}

static double as_double(Value v) { return v.is_float ? v.f_val : (double)v.i_val; }

/*
 * Applies + - * / to two values. Evaluation happens right after the right
 * operand is parsed (no syntax tree), so runtime errors and syntax errors are
 * naturally ordered by their place in the line.
 *
 *  - Division by zero is reported at the START OF THE RIGHT OPERAND
 *    (right_pos): the spec says so, and it is checked before dividing so an
 *    integer division can never trap.
 *  - A float result that is infinite or NaN is reported at the OPERATOR
 *    (op_pos): the operands were fine, the operation is what overflowed.
 */
static void apply_binary(Parser *p, TokenType op, Value *left, Value right,
                         size_t op_pos, size_t right_pos)
{
    if (op == TOK_DIV) {
        int zero = right.is_float ? right.f_val == 0.0 : right.i_val == 0;
        if (zero) {
            parser_error(p, right_pos);
            return;
        }
    }

    if (!left->is_float && !right.is_float) {
        
        unsigned long long a = (unsigned long long)left->i_val;
        unsigned long long b = (unsigned long long)right.i_val;
        switch (op) {
        case TOK_PLUS:  left->i_val = (long long)(a + b); break;
        case TOK_MINUS: left->i_val = (long long)(a - b); break;
        case TOK_MUL:   left->i_val = (long long)(a * b); break;
        default:        left->i_val = left->i_val / right.i_val; break; /* truncates toward zero */
        }
        return;
    }

    double x = as_double(*left), y = as_double(right), z;
    switch (op) {
    case TOK_PLUS:  z = x + y; break;
    case TOK_MINUS: z = x - y; break;
    case TOK_MUL:   z = x * y; break;
    default:        z = x / y; break;
    }
    if (isinf(z) || isnan(z)) {
        parser_error(p, op_pos);
        return;
    }
    left->is_float = 1;
    left->f_val = z;
}

/* ======================================================================== */
/* Parser                                                                   */
/* ======================================================================== */


Value parse_expr(Parser *p);
Value parse_term(Parser *p);
Value parse_power(Parser *p);
Value parse_primary(Parser *p);

Value parse_primary(Parser *p)
{
    Value val = {0, 0, 0.0};
    if (p->error_occurred)
        return val;

    if (p->current.type == TOK_NUM) {
        val.is_float = p->current.is_float;
        val.i_val = p->current.int_val;
        val.f_val = p->current.float_val;
        p->current = lexer_next(&p->lexer);
        return val;
    }

    if (p->current.type == TOK_LPAREN) {
        p->current = lexer_next(&p->lexer);
        val = parse_expr(p);
        if (p->error_occurred)
            return val;
       
        if (p->current.type != TOK_RPAREN) {
            parser_error(p, p->current.pos);
            return val;
        }
        p->current = lexer_next(&p->lexer);
        return val;
    }

    
    parser_error(p, p->current.pos);
    return val;
}


Value parse_power(Parser *p)
{
    Value left = parse_primary(p);
    if (p->error_occurred)
        return left;

    if (p->current.type == TOK_POW) {
        size_t pow_pos = p->current.pos;
        p->current = lexer_next(&p->lexer);
        size_t exp_pos = p->current.pos;
        Value right = parse_power(p);
        if (p->error_occurred)
            return left;

        if (!left.is_float && !right.is_float) {
            
            if (right.i_val < 0) {
                parser_error(p, exp_pos);
                return left;
            }
            
            unsigned long long result = 1;
            unsigned long long base = (unsigned long long)left.i_val;
            unsigned long long e = (unsigned long long)right.i_val;
            while (e) {
                if (e & 1u)
                    result *= base;
                e >>= 1;
                if (e)
                    base *= base;
            }
            left.i_val = (long long)result;
            return left;
        }

        double res = pow(as_double(left), as_double(right));
        
        if (isinf(res) || isnan(res)) {
            parser_error(p, pow_pos);
            return left;
        }
        left.is_float = 1;
        left.f_val = res;
    }
    return left;
}

Value parse_term(Parser *p)
{
    Value left = parse_power(p);
    if (p->error_occurred)
        return left;

    while (p->current.type == TOK_MUL || p->current.type == TOK_DIV) {
        TokenType op = p->current.type;
        size_t op_pos = p->current.pos;
        p->current = lexer_next(&p->lexer);
        size_t right_pos = p->current.pos;
        Value right = parse_power(p);
        if (p->error_occurred)
            return left;
        apply_binary(p, op, &left, right, op_pos, right_pos);
        if (p->error_occurred)
            return left;
    }
    return left;
}

Value parse_expr(Parser *p)
{
    Value left = parse_term(p);
    if (p->error_occurred)
        return left;

    while (p->current.type == TOK_PLUS || p->current.type == TOK_MINUS) {
        TokenType op = p->current.type;
        size_t op_pos = p->current.pos;
        p->current = lexer_next(&p->lexer);
        size_t right_pos = p->current.pos;
        Value right = parse_term(p);
        if (p->error_occurred)
            return left;
        apply_binary(p, op, &left, right, op_pos, right_pos);
        if (p->error_occurred)
            return left;
    }
    return left;
}

/* ======================================================================== */
/* File processing                                                          */
/* ======================================================================== */

typedef struct {
    char *data;
    size_t len, cap;
} OutBuf;


static void ob_append(OutBuf *ob, const char *s, size_t n)
{
    if (ob->len + n > ob->cap) {
        size_t ncap = ob->cap ? ob->cap : 4096;
        while (ncap < ob->len + n)
            ncap *= 2;
        char *nd = realloc(ob->data, ncap);
        if (!nd) {
            fprintf(stderr, "calc: out of memory\n");
            exit(1);
        }
        ob->data = nd;
        ob->cap = ncap;
    }
    memcpy(ob->data + ob->len, s, n);
    ob->len += n;
}


static int process_file(const char *in_path, const char *out_path)
{
    FILE *fin = fopen(in_path, "rb");
    if (!fin) {
        fprintf(stderr, "calc: cannot open input file '%s': %s\n", in_path, strerror(errno));
        return 0;
    }

    
    if (fseek(fin, 0, SEEK_END) != 0) {
        fprintf(stderr, "calc: cannot read '%s'\n", in_path);
        fclose(fin);
        return 0;
    }
    long fsize = ftell(fin);
    if (fsize < 0 || fseek(fin, 0, SEEK_SET) != 0) {
        fprintf(stderr, "calc: cannot read '%s'\n", in_path);
        fclose(fin);
        return 0;
    }
    char *buf = (char *)malloc((size_t)fsize + 1);
    if (!buf) {
        fprintf(stderr, "calc: out of memory\n");
        fclose(fin);
        return 0;
    }
    size_t len = fread(buf, 1, (size_t)fsize, fin);
    int read_failed = ferror(fin);
    fclose(fin);
    if (read_failed) {
        fprintf(stderr, "calc: cannot read '%s'\n", in_path);
        free(buf);
        return 0;
    }
    buf[len] = '\0'; 

    OutBuf ob = {NULL, 0, 0};
    int failed = 0;
    size_t err_pos = 0;
    char tmp[64];

    size_t line_start = 0;
    while (line_start < len) {
        char *nl = (char *)memchr(buf + line_start, '\n', len - line_start);
        size_t line_end = nl ? (size_t)(nl - buf) : len;

        
        size_t i = line_start;
        while (i < line_end && (buf[i] == ' ' || buf[i] == '\t'))
            i++;
        if (i == line_end || buf[i] == '#') {
            line_start = line_end + 1;
            continue;
        }

        Parser p;
        lexer_init(&p.lexer, buf, line_start, line_end);
        p.error_occurred = 0;
        p.error_pos = 0;
        p.current = lexer_next(&p.lexer);

        Value res = parse_expr(&p);

        
        if (!p.error_occurred && p.current.type != TOK_EOF)
            parser_error(&p, p.current.pos);

        if (p.error_occurred) {
            
            failed = 1;
            err_pos = p.error_pos;
            break;
        }

        int n = res.is_float ? snprintf(tmp, sizeof tmp, "%.15g\n", res.f_val)
                             : snprintf(tmp, sizeof tmp, "%lld\n", res.i_val);
        ob_append(&ob, tmp, (size_t)n);

        line_start = line_end + 1;
    }

    free(buf);

    if (failed) {
        
        int n = snprintf(tmp, sizeof tmp, "ERROR:%zu\n", err_pos);
        ob.len = 0;
        ob_append(&ob, tmp, (size_t)n);
    }

    FILE *fout = fopen(out_path, "wb");
    int ok = 0;
    if (fout) {
        ok = (ob.len == 0 || fwrite(ob.data, 1, ob.len, fout) == ob.len);
        if (fclose(fout) != 0)
            ok = 0;
    }
    free(ob.data);
    if (!ok)
        fprintf(stderr, "calc: cannot write output file '%s': %s\n", out_path, strerror(errno));
    return ok;
}

/* ======================================================================== */
/* Paths and command line                                                   */
/* ======================================================================== */

static int last_component(const char *path, char *out, size_t cap)
{
    size_t end = strlen(path);
    while (end > 0 && path[end - 1] == '/')
        end--;
    size_t start = end;
    while (start > 0 && path[start - 1] != '/')
        start--;
    size_t n = end - start;
    if (n == 0 || n >= cap)
        return 0;
    memcpy(out, path + start, n);
    out[n] = '\0';
    return 1;
}

static void strip_txt(char *name)
{
    size_t n = strlen(name);
    if (n > 4 && strcmp(name + n - 4, ".txt") == 0)
        name[n - 4] = '\0';
}


static int ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode))
            return 1;
        fprintf(stderr, "calc: '%s' exists but is not a folder\n", path);
        return 0;
    }
    if (mkdir(path, 0777) != 0) {
        fprintf(stderr, "calc: cannot create folder '%s': %s\n", path, strerror(errno));
        return 0;
    }
    return 1;
}


static int run_one(const char *in_path, const char *outdir, const char *stem)
{
    char out_path[PATH_LEN];
    int n = snprintf(out_path, sizeof out_path, "%s/%s_%s_%s_%s.txt",
                     outdir, stem, STUDENT_NAME, STUDENT_LAST, STUDENT_ID);
    if (n < 0 || (size_t)n >= sizeof out_path) {
        fprintf(stderr, "calc: output path too long\n");
        return 0;
    }
    return process_file(in_path, out_path);
}


static int default_outdir(char *outdir, size_t cap, const char *base)
{
    int n = snprintf(outdir, cap, "%s_%s_%s", base, STUDENT_NAME, STUDENT_ID);
    if (n < 0 || (size_t)n >= cap) {
        fprintf(stderr, "calc: output folder path too long\n");
        return 0;
    }
    return 1;
}

static int run_single(const char *input, const char *out_arg)
{
    struct stat st;
    if (stat(input, &st) != 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "calc: input file '%s' not found\n", input);
        return 0;
    }

    char stem[PATH_LEN];
    if (!last_component(input, stem, sizeof stem)) {
        fprintf(stderr, "calc: invalid input path '%s'\n", input);
        return 0;
    }
    strip_txt(stem);

    char outdir[PATH_LEN];
    if (out_arg) {
        if (strlen(out_arg) >= sizeof outdir) {
            fprintf(stderr, "calc: output folder path too long\n");
            return 0;
        }
        strcpy(outdir, out_arg);
    } else if (!default_outdir(outdir, sizeof outdir, stem)) {
        return 0;
    }

    if (!ensure_dir(outdir))
        return 0;
    return run_one(input, outdir, stem);
}

static int cmp_names(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

static int run_directory(const char *dir_arg, const char *out_arg)
{
    struct stat st;
    if (stat(dir_arg, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "calc: folder '%s' not found\n", dir_arg);
        return 0;
    }

    char outdir[PATH_LEN];
    if (out_arg) {
        if (strlen(out_arg) >= sizeof outdir) {
            fprintf(stderr, "calc: output folder path too long\n");
            return 0;
        }
        strcpy(outdir, out_arg);
    } else {
        char base[PATH_LEN];
        if (!last_component(dir_arg, base, sizeof base)) {
            fprintf(stderr, "calc: invalid folder path '%s'\n", dir_arg);
            return 0;
        }
        if (!default_outdir(outdir, sizeof outdir, base))
            return 0;
    }

    DIR *d = opendir(dir_arg);
    if (!d) {
        fprintf(stderr, "calc: cannot open folder '%s': %s\n", dir_arg, strerror(errno));
        return 0;
    }

    
    char **names = NULL;
    size_t nnames = 0, ncap = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        size_t n = strlen(e->d_name);
        if (n <= 4 || strcmp(e->d_name + n - 4, ".txt") != 0)
            continue;
        char full[PATH_LEN];
        int w = snprintf(full, sizeof full, "%s/%s", dir_arg, e->d_name);
        if (w < 0 || (size_t)w >= sizeof full)
            continue;
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode))
            continue;
        if (nnames == ncap) {
            size_t nc = ncap ? ncap * 2 : 16;
            char **nn = (char **)realloc(names, nc * sizeof *nn);
            if (!nn) {
                fprintf(stderr, "calc: out of memory\n");
                exit(1);
            }
            names = nn;
            ncap = nc;
        }
        char *copy = (char *)malloc(n + 1);
        if (!copy) {
            fprintf(stderr, "calc: out of memory\n");
            exit(1);
        }
        memcpy(copy, e->d_name, n + 1);
        names[nnames++] = copy;
    }
    closedir(d);

    
    if (nnames > 1)
        qsort(names, nnames, sizeof *names, cmp_names);

    int ok = ensure_dir(outdir);
    for (size_t k = 0; ok && k < nnames; k++) {
        char full[PATH_LEN], stem[PATH_LEN];
        int w = snprintf(full, sizeof full, "%s/%s", dir_arg, names[k]);
        if (w < 0 || (size_t)w >= sizeof full || strlen(names[k]) >= sizeof stem)
            continue;
        strcpy(stem, names[k]);
        strip_txt(stem);
        
        if (!run_one(full, outdir, stem))
            ok = 0;
    }

    for (size_t k = 0; k < nnames; k++)
        free(names[k]);
    free(names);
    return ok;
}

static void usage(void)
{
    fprintf(stderr, "usage: calc [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n");
}

int main(int argc, char *argv[])
{
    const char *dir_arg = NULL;
    const char *out_arg = NULL;
    const char *input_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (i + 1 >= argc || dir_arg) {
                fprintf(stderr, "calc: option '%s' needs exactly one folder\n", argv[i]);
                usage();
                return 1;
            }
            dir_arg = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (i + 1 >= argc || out_arg) {
                fprintf(stderr, "calc: option '%s' needs exactly one folder\n", argv[i]);
                usage();
                return 1;
            }
            out_arg = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "calc: unknown option '%s'\n", argv[i]);
            usage();
            return 1;
        } else if (input_file) {
            fprintf(stderr, "calc: more than one input file given\n");
            usage();
            return 1;
        } else {
            input_file = argv[i];
        }
    }

    if (!dir_arg && !input_file) {
        fprintf(stderr, "calc: no input file or folder given\n");
        usage();
        return 1;
    }
    if (dir_arg && input_file) {
        fprintf(stderr, "calc: give either -d DIR or an input file, not both\n");
        usage();
        return 1;
    }

    int ok = dir_arg ? run_directory(dir_arg, out_arg) : run_single(input_file, out_arg);
    return ok ? 0 : 1;
}