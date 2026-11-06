// ============================================================================
//  StarC 编译器 · 词法分析器 (lex.h)
//  ---------------------------------------------------------------------------
//  语言关键字（德语派生命名，取自 E:\yuyao\vendor 下的三份示例库与真实词库）：
//      Funktion  Ende  Solange  Wenn  dann  Sonst  Ausgeben  gebe_zurueck
//      wahr  falsch  und  oder  nicht  linx
// ============================================================================
#ifndef STARC_LEX_H
#define STARC_LEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef enum {
    TK_EOF = 0,
    TK_NUM,          // 123
    TK_STR,          // "..."
    TK_ID,           // identifier
    // 关键字
    TK_FUNKTION, TK_ENDE, TK_SOLANGE, TK_WENN, TK_DANN, TK_SONST,
    TK_AUSGEBEN, TK_ZURUECK, TK_WAHR, TK_FALSCH, TK_UND, TK_ODER,
    TK_NICHT, TK_LINX, TK_LINXO, TK_PARALLEL,
    TK_EXPORT,       // export —— DLL 导出声明
    // 运算符 / 标点
    TK_ASSIGN,       // =
    TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_PERCENT,
    TK_EQ, TK_NE, TK_LT, TK_GT, TK_LE, TK_GE,
    TK_LPAREN, TK_RPAREN, TK_LBRACK, TK_RBRACK, TK_LBRACE, TK_RBRACE,
    TK_COMMA, TK_DOT, TK_SEMI,
    TK_NEWLINE,      // 仅在需要时用；目前视为空白
    TK_ERROR
} TokenKind;

typedef struct {
    TokenKind kind;
    int64_t   ival;
    char     *sval;      // 字符串字面量 / 标识符名（堆分配）
    int       line;
    int       col;
} Token;

typedef struct {
    const char *src;
    size_t      pos;
    int         line;
    int         col;
    Token       cur;
    Token       ahead;
    int         has_ahead;
} Lexer;

static const char *tk_name(TokenKind k) {
    switch (k) {
        case TK_EOF: return "EOF";
        case TK_NUM: return "数字";
        case TK_STR: return "字符串";
        case TK_ID:  return "标识符";
        case TK_FUNKTION: return "Funktion";
        case TK_ENDE: return "Ende";
        case TK_SOLANGE: return "Solange";
        case TK_WENN: return "Wenn";
        case TK_DANN: return "dann";
        case TK_SONST: return "Sonst";
        case TK_AUSGEBEN: return "Ausgeben";
        case TK_ZURUECK: return "gebe_zurueck";
        case TK_WAHR: return "wahr";
        case TK_FALSCH: return "falsch";
        case TK_UND: return "und";
        case TK_ODER: return "oder";
        case TK_NICHT: return "nicht";
        case TK_LINX: return "linx";
        case TK_LINXO: return "linx-o";
        case TK_PARALLEL: return "Parallel";
        case TK_ASSIGN: return "=";
        case TK_PLUS: return "+";
        case TK_MINUS: return "-";
        case TK_STAR: return "*";
        case TK_SLASH: return "/";
        case TK_PERCENT: return "%";
        case TK_EQ: return "==";
        case TK_NE: return "!=";
        case TK_LT: return "<";
        case TK_GT: return ">";
        case TK_LE: return "<=";
        case TK_GE: return ">=";
        case TK_LPAREN: return "(";
        case TK_RPAREN: return ")";
        case TK_LBRACK: return "[";
        case TK_RBRACK: return "]";
        case TK_LBRACE: return "{";
        case TK_RBRACE: return "}";
        case TK_COMMA: return ",";
        case TK_DOT: return ".";
        case TK_SEMI: return ";";
        default: return "?";
    }
}

static char *sc_strdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    memcpy(p, s, n);
    return p;
}
static char *sc_strndup(const char *s, size_t n) {
    char *p = (char *)malloc(n + 1);
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

static void lx_init(Lexer *L, const char *src) {
    L->src = src; L->pos = 0; L->line = 1; L->col = 1; L->has_ahead = 0;
    memset(&L->cur, 0, sizeof(L->cur));
}

static int lx_peekc(Lexer *L) { return (unsigned char)L->src[L->pos]; }
static int lx_getc(Lexer *L) {
    int c = (unsigned char)L->src[L->pos];
    if (!c) return 0;
    L->pos++;
    if (c == '\n') { L->line++; L->col = 1; } else L->col++;
    return c;
}

static TokenKind lx_keyword(const char *w) {
    if (!strcmp(w, "Funktion"))     return TK_FUNKTION;
    if (!strcmp(w, "Ende"))         return TK_ENDE;
    if (!strcmp(w, "Solange"))      return TK_SOLANGE;
    if (!strcmp(w, "Wenn"))         return TK_WENN;
    if (!strcmp(w, "dann"))         return TK_DANN;
    if (!strcmp(w, "Sonst"))        return TK_SONST;
    if (!strcmp(w, "Ausgeben"))     return TK_AUSGEBEN;
    if (!strcmp(w, "gebe_zurueck")) return TK_ZURUECK;
    if (!strcmp(w, "wahr"))         return TK_WAHR;
    if (!strcmp(w, "falsch"))       return TK_FALSCH;
    if (!strcmp(w, "und"))          return TK_UND;
    if (!strcmp(w, "oder"))         return TK_ODER;
    if (!strcmp(w, "nicht"))        return TK_NICHT;
    if (!strcmp(w, "linx"))         return TK_LINX;
    if (!strcmp(w, "linx-o"))       return TK_LINXO;
    if (!strcmp(w, "Parallel"))     return TK_PARALLEL;
    if (!strcmp(w, "export"))      return TK_EXPORT;
    return TK_ID;
}

static Token lx_scan(Lexer *L) {
    Token t; memset(&t, 0, sizeof(t));
    // 跳过空白与注释（// 与 #）
    for (;;) {
        int c = lx_peekc(L);
        if (c == 0) break;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { lx_getc(L); continue; }
        if (c == '/' && L->src[L->pos + 1] == '/') {
            while (lx_peekc(L) && lx_peekc(L) != '\n') lx_getc(L);
            continue;
        }
        if (c == '#') {
            while (lx_peekc(L) && lx_peekc(L) != '\n') lx_getc(L);
            continue;
        }
        break;
    }
    t.line = L->line; t.col = L->col;
    int c = lx_peekc(L);
    if (c == 0) { t.kind = TK_EOF; return t; }

    // 数字
    if (isdigit(c)) {
        int64_t v = 0;
        while (isdigit(lx_peekc(L))) v = v * 10 + (lx_getc(L) - '0');
        t.kind = TK_NUM; t.ival = v;
        return t;
    }
    // 标识符 / 关键字
    if (isalpha(c) || c == '_') {
        size_t st = L->pos;
        while (isalnum(lx_peekc(L)) || lx_peekc(L) == '_' || lx_peekc(L) == '-') lx_getc(L);
        // 允许 linx-o 这样的带连字符关键字
        char *w = sc_strndup(L->src + st, L->pos - st);
        t.kind = lx_keyword(w);
        t.sval = w;
        return t;
    }
    // 字符串
    if (c == '"') {
        lx_getc(L);
        size_t cap = 64, n = 0;
        char *b = (char *)malloc(cap);
        while (lx_peekc(L) && lx_peekc(L) != '"') {
            int ch = lx_getc(L);
            if (ch == '\\') {
                int e = lx_getc(L);
                switch (e) {
                    case 'n': ch = '\n'; break;
                    case 't': ch = '\t'; break;
                    case 'r': ch = '\r'; break;
                    case '0': ch = 0; break;
                    case '\\': ch = '\\'; break;
                    case '"': ch = '"'; break;
                    default: ch = e; break;
                }
            }
            if (n + 2 >= cap) { cap *= 2; b = (char *)realloc(b, cap); }
            b[n++] = (char)ch;
        }
        if (lx_peekc(L) == '"') lx_getc(L);
        b[n] = 0;
        t.kind = TK_STR; t.sval = b;
        return t;
    }
    // 运算符
    lx_getc(L);
    switch (c) {
        case '=':
            if (lx_peekc(L) == '=') { lx_getc(L); t.kind = TK_EQ; }
            else t.kind = TK_ASSIGN;
            return t;
        case '!':
            if (lx_peekc(L) == '=') { lx_getc(L); t.kind = TK_NE; }
            else t.kind = TK_NICHT;
            return t;
        case '<':
            if (lx_peekc(L) == '=') { lx_getc(L); t.kind = TK_LE; }
            else if (lx_peekc(L) == '>') { lx_getc(L); t.kind = TK_NE; }
            else t.kind = TK_LT;
            return t;
        case '>':
            if (lx_peekc(L) == '=') { lx_getc(L); t.kind = TK_GE; }
            else t.kind = TK_GT;
            return t;
        case '+': t.kind = TK_PLUS;    return t;
        case '-': t.kind = TK_MINUS;   return t;
        case '*': t.kind = TK_STAR;    return t;
        case '/': t.kind = TK_SLASH;   return t;
        case '%': t.kind = TK_PERCENT; return t;
        case '(': t.kind = TK_LPAREN;  return t;
        case ')': t.kind = TK_RPAREN;  return t;
        case '[': t.kind = TK_LBRACK;  return t;
        case ']': t.kind = TK_RBRACK;  return t;
        case '{': t.kind = TK_LBRACE;  return t;
        case '}': t.kind = TK_RBRACE;  return t;
        case ',': t.kind = TK_COMMA;   return t;
        case '.': t.kind = TK_DOT;     return t;
        case ';': t.kind = TK_SEMI;    return t;
        default:  t.kind = TK_ERROR;   return t;
    }
}

static void lx_next(Lexer *L) {
    if (L->has_ahead) { L->cur = L->ahead; L->has_ahead = 0; }
    else L->cur = lx_scan(L);
}
static Token *lx_peek(Lexer *L) {
    if (!L->has_ahead) { L->ahead = lx_scan(L); L->has_ahead = 1; }
    return &L->ahead;
}
static int lx_at(Lexer *L, TokenKind k) { return L->cur.kind == k; }
static int lx_accept(Lexer *L, TokenKind k) { if (L->cur.kind == k) { lx_next(L); return 1; } return 0; }
static void lx_expect(Lexer *L, TokenKind k, const char *what) {
    if (L->cur.kind != k) {
        fprintf(stderr, "语法错误 第%d行 第%d列: 期望 %s，实际是 %s\n",
                L->cur.line, L->cur.col, what, tk_name(L->cur.kind));
        exit(1);
    }
    lx_next(L);
}

#endif // STARC_LEX_H
