// ============================================================================
//  StarC 编译器 · 语法树与递归下降语法分析 (ast.h)
// ============================================================================
#ifndef STARC_AST_H
#define STARC_AST_H

#include "lex.h"

// ---------------------------------------------------------------- 值类型
typedef enum { TY_INT = 0, TY_STR, TY_ARR, TY_ANY } ValueType;

// ---------------------------------------------------------------- 节点
typedef enum {
    N_PROGRAM, N_FUNC, N_BLOCK,
    N_ASSIGN, N_PRINT, N_RETURN, N_IF, N_WHILE, N_LINX, N_LINXO, N_PARALLEL,
    N_BIN, N_UN, N_NUM, N_STR, N_VAR, N_CALL, N_INDEX, N_ARRAY,
    N_EXPORT           // export "函数名"  —— DLL 导出声明
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind  kind;
    int       line;
    char     *name;        // 函数名 / 变量名 / 运算符
    int64_t   ival;        // N_NUM
    char     *sval;        // N_STR
    Node     *a, *b, *c, *d;
    Node    **list;        // 子节点列表（params / args / block stmts / array elems）
    char    **names;       // 参数名列表
    int       n, cap;
};

static Node *node_new(NodeKind k, int line) {
    Node *x = (Node *)calloc(1, sizeof(Node));
    x->kind = k; x->line = line;
    return x;
}
static void node_push(Node *p, Node *c) {
    if (p->n >= p->cap) {
        p->cap = p->cap ? p->cap * 2 : 4;
        p->list = (Node **)realloc(p->list, p->cap * sizeof(Node *));
    }
    p->list[p->n++] = c;
}
static void params_push(Node *p, const char *nm) {
    if (p->n >= p->cap - 1) {
        p->cap = p->cap ? p->cap * 2 : 4;
        p->names = (char **)realloc(p->names, p->cap * sizeof(char *));
    }
    p->names[p->n++] = sc_strdup(nm);
}

// ---------------------------------------------------------------- parser
typedef struct { Lexer L; } Parser;

// 表达式
static Node *p_expr(Parser *P);

static Node *p_primary(Parser *P) {
    Lexer *L = &P->L;
    Token t = L->cur;
    if (t.kind == TK_NUM) {
        lx_next(L);
        Node *x = node_new(N_NUM, t.line); x->ival = t.ival; return x;
    }
    if (t.kind == TK_STR) {
        lx_next(L);
        Node *x = node_new(N_STR, t.line); x->sval = t.sval; return x;
    }
    if (t.kind == TK_WAHR)  { lx_next(L); Node *x = node_new(N_NUM, t.line); x->ival = 1; return x; }
    if (t.kind == TK_FALSCH){ lx_next(L); Node *x = node_new(N_NUM, t.line); x->ival = 0; return x; }
    if (t.kind == TK_LPAREN) {
        lx_next(L);
        Node *e = p_expr(P);
        lx_expect(L, TK_RPAREN, ")");
        return e;
    }
    if (t.kind == TK_LBRACK) {
        // 数组字面量 [a, b, c]
        lx_next(L);
        Node *arr = node_new(N_ARRAY, t.line);
        if (!lx_at(L, TK_RBRACK)) {
            node_push(arr, p_expr(P));
            while (lx_accept(L, TK_COMMA)) node_push(arr, p_expr(P));
        }
        lx_expect(L, TK_RBRACK, "]");
        return arr;
    }
    if (t.kind == TK_ID) {
        char *nm = t.sval;
        lx_next(L);
        if (lx_at(L, TK_LPAREN)) {
            lx_next(L);
            Node *call = node_new(N_CALL, t.line);
            call->name = nm;
            if (!lx_at(L, TK_RPAREN)) {
                node_push(call, p_expr(P));
                while (lx_accept(L, TK_COMMA)) node_push(call, p_expr(P));
            }
            lx_expect(L, TK_RPAREN, ")");
            return call;
        }
        Node *v = node_new(N_VAR, t.line);
        v->name = nm;
        return v;
    }
    fprintf(stderr, "语法错误 第%d行: 意外的 %s\n", t.line, tk_name(t.kind));
    exit(1);
}

// 后缀：下标 a[i]
static Node *p_postfix(Parser *P) {
    Node *e = p_primary(P);
    while (lx_at(&P->L, TK_LBRACK)) {
        int ln = P->L.cur.line;
        lx_next(&P->L);
        Node *idx = p_expr(P);
        lx_expect(&P->L, TK_RBRACK, "]");
        Node *n = node_new(N_INDEX, ln);
        n->a = e; n->b = idx;
        e = n;
    }
    return e;
}

static Node *p_unary(Parser *P) {
    Lexer *L = &P->L;
    if (lx_accept(L, TK_MINUS)) {
        Node *n = node_new(N_UN, L->cur.line);
        n->name = sc_strdup("-"); n->a = p_unary(P); return n;
    }
    if (lx_accept(L, TK_NICHT)) {
        Node *n = node_new(N_UN, L->cur.line);
        n->name = sc_strdup("nicht"); n->a = p_unary(P); return n;
    }
    return p_postfix(P);
}

static Node *p_mul(Parser *P) {
    Node *l = p_unary(P);
    for (;;) {
        TokenKind k = P->L.cur.kind;
        if (k != TK_STAR && k != TK_SLASH && k != TK_PERCENT) break;
        int ln = P->L.cur.line;
        const char *op = (k == TK_STAR) ? "*" : (k == TK_SLASH) ? "/" : "%";
        lx_next(&P->L);
        Node *r = p_unary(P);
        Node *n = node_new(N_BIN, ln); n->name = sc_strdup(op); n->a = l; n->b = r; l = n;
    }
    return l;
}
static Node *p_add(Parser *P) {
    Node *l = p_mul(P);
    for (;;) {
        TokenKind k = P->L.cur.kind;
        if (k != TK_PLUS && k != TK_MINUS) break;
        int ln = P->L.cur.line;
        const char *op = (k == TK_PLUS) ? "+" : "-";
        lx_next(&P->L);
        Node *r = p_mul(P);
        Node *n = node_new(N_BIN, ln); n->name = sc_strdup(op); n->a = l; n->b = r; l = n;
    }
    return l;
}
static Node *p_cmp(Parser *P) {
    Node *l = p_add(P);
    TokenKind k = P->L.cur.kind;
    const char *op = NULL;
    switch (k) {
        case TK_EQ: op = "=="; break;
        case TK_NE: op = "!="; break;
        case TK_LT: op = "<";  break;
        case TK_GT: op = ">";  break;
        case TK_LE: op = "<="; break;
        case TK_GE: op = ">="; break;
        default: break;
    }
    if (op) {
        int ln = P->L.cur.line;
        lx_next(&P->L);
        Node *r = p_add(P);
        Node *n = node_new(N_BIN, ln); n->name = sc_strdup(op); n->a = l; n->b = r; return n;
    }
    return l;
}
static Node *p_and(Parser *P) {
    Node *l = p_cmp(P);
    while (P->L.cur.kind == TK_UND) {
        int ln = P->L.cur.line;
        lx_next(&P->L);
        Node *r = p_cmp(P);
        Node *n = node_new(N_BIN, ln); n->name = sc_strdup("und"); n->a = l; n->b = r; l = n;
    }
    return l;
}
static Node *p_or(Parser *P) {
    Node *l = p_and(P);
    while (P->L.cur.kind == TK_ODER) {
        int ln = P->L.cur.line;
        lx_next(&P->L);
        Node *r = p_and(P);
        Node *n = node_new(N_BIN, ln); n->name = sc_strdup("oder"); n->a = l; n->b = r; l = n;
    }
    return l;
}
static Node *p_expr(Parser *P) { return p_or(P); }

// 语句块：直到 Ende / EOF
static Node *p_block(Parser *P);
static Node *parse_stmt(Parser *P);

static Node *p_block(Parser *P) {
    Node *b = node_new(N_BLOCK, P->L.cur.line);
    while (!lx_at(&P->L, TK_ENDE) && !lx_at(&P->L, TK_EOF) && !lx_at(&P->L, TK_SONST)) {
        Node *s = parse_stmt(P);
        if (s) node_push(b, s);
    }
    return b;
}

static Node *parse_stmt(Parser *P) {
    Lexer *L = &P->L;
    int ln = L->cur.line;

    if (lx_accept(L, TK_WENN)) {
        // 条件直接交给表达式解析器：带不带括号都行，
        // 因为 `(a-b) >= x` 里括号只包住左操作数，不能当成整个条件的外壳。
        Node *cond = p_expr(P);
        lx_accept(L, TK_DANN);
        Node *th = p_block(P);
        Node *el = NULL;
        if (lx_accept(L, TK_SONST)) el = p_block(P);
        lx_expect(L, TK_ENDE, "Ende");
        // `Ende Wenn` 收尾：若 Wenn 后面跟着条件，那是新语句，不能吃
        if (lx_at(L, TK_WENN)) {
            TokenKind k2 = lx_peek(L)->kind;
            if (!(k2 == TK_ID || k2 == TK_NUM || k2 == TK_LPAREN || k2 == TK_STR ||
                  k2 == TK_WAHR || k2 == TK_FALSCH || k2 == TK_NICHT || k2 == TK_MINUS))
                lx_next(L);
        }
        Node *n = node_new(N_IF, ln); n->a = cond; n->b = th; n->c = el;
        return n;
    }
    if (lx_accept(L, TK_SOLANGE)) {
        Node *cond = p_expr(P);
        Node *body = p_block(P);
        lx_expect(L, TK_ENDE, "Ende");
        if (lx_at(L, TK_SOLANGE)) {
            TokenKind k2 = lx_peek(L)->kind;
            if (!(k2 == TK_ID || k2 == TK_NUM || k2 == TK_LPAREN || k2 == TK_STR ||
                  k2 == TK_WAHR || k2 == TK_FALSCH || k2 == TK_NICHT || k2 == TK_MINUS))
                lx_next(L);
        }
        Node *n = node_new(N_WHILE, ln); n->a = cond; n->b = body;
        return n;
    }
    if (lx_accept(L, TK_AUSGEBEN)) {
        Node *n = node_new(N_PRINT, ln);
        n->a = p_expr(P);
        return n;
    }
    if (lx_accept(L, TK_ZURUECK)) {
        Node *n = node_new(N_RETURN, ln);
        // 允许裸 return
        if (!lx_at(L, TK_ENDE) && !lx_at(L, TK_EOF) && !lx_at(L, TK_SONST))
            n->a = p_expr(P);
        return n;
    }
    if (lx_accept(L, TK_PARALLEL)) {
        // Parallel ... Ende ：块内每条语句（要求是函数调用）各起一个线程，块末全部 join
        Node *n = node_new(N_PARALLEL, ln);
        while (!lx_at(L, TK_ENDE) && !lx_at(L, TK_EOF))
            node_push(n, parse_stmt(P));
        lx_expect(L, TK_ENDE, "Ende");
        if (lx_at(L, TK_PARALLEL)) lx_next(L);
        return n;
    }
    if (lx_accept(L, TK_LINX)) {
        Node *n = node_new(N_LINX, ln);
        if (L->cur.kind == TK_STR) { n->sval = L->cur.sval; lx_next(L); }
        else if (L->cur.kind == TK_ID) { n->sval = L->cur.sval; lx_next(L); }
        else { fprintf(stderr, "语法错误 第%d行: linx 后面要跟库名\n", ln); exit(1); }
        return n;
    }
    if (lx_accept(L, TK_LINXO)) {
        // linx-o <dll_name> <func1> <func2> ...
        Node *n = node_new(N_LINXO, ln);
        if (L->cur.kind == TK_ID) { n->sval = L->cur.sval; lx_next(L); }
        else { fprintf(stderr, "语法错误 第%d行: linx-o 后面要跟库名\n", ln); exit(1); }
        // 收集后续所有函数名
        while (L->cur.kind == TK_ID) {
            Node *fn = node_new(N_STR, ln);
            fn->sval = L->cur.sval;
            node_push(n, fn);
            lx_next(L);
        }
        return n;
    }
    // 赋值： IDENT '=' expr   （支持 a[i] = expr 留给后续）
    if (L->cur.kind == TK_ID && lx_peek(L)->kind == TK_ASSIGN) {
        char *nm = L->cur.sval;
        lx_next(L); lx_next(L);
        Node *n = node_new(N_ASSIGN, ln);
        n->name = nm;
        n->a = p_expr(P);
        return n;
    }
    // 其余按表达式语句处理
    Node *e = p_expr(P);
    return e;
}

// 全文
static Node *parse_program(Parser *P) {
    Node *prog = node_new(N_PROGRAM, 1);
    lx_next(&P->L);
    while (!lx_at(&P->L, TK_EOF)) {
        // export "函数名" —— 声明该函数从 DLL 导出
        if (lx_at(&P->L, TK_EXPORT)) {
            int ln = P->L.cur.line;
            lx_next(&P->L);
            if (!lx_at(&P->L, TK_STR)) {
                fprintf(stderr, "语法错误 第%d行: export 后面要跟字符串形式的函数名\n", ln);
                exit(1);
            }
            Node *ex = node_new(N_EXPORT, ln);
            ex->name = P->L.cur.sval;
            lx_next(&P->L);
            node_push(prog, ex);
            continue;
        }
        if (lx_at(&P->L, TK_FUNKTION)) {
            int ln = P->L.cur.line;
            lx_next(&P->L);
            if (!lx_at(&P->L, TK_ID)) {
                fprintf(stderr, "语法错误 第%d行: Funktion 后面要跟函数名\n", ln);
                exit(1);
            }
            Node *f = node_new(N_FUNC, ln);
            f->name = P->L.cur.sval;
            lx_next(&P->L);
            lx_expect(&P->L, TK_LPAREN, "(");
            if (!lx_at(&P->L, TK_RPAREN)) {
                if (!lx_at(&P->L, TK_ID)) { fprintf(stderr, "语法错误: 参数名无效\n"); exit(1); }
                params_push(f, P->L.cur.sval); lx_next(&P->L);
                while (lx_accept(&P->L, TK_COMMA)) {
                    if (!lx_at(&P->L, TK_ID)) { fprintf(stderr, "语法错误: 参数名无效\n"); exit(1); }
                    params_push(f, P->L.cur.sval); lx_next(&P->L);
                }
            }
            lx_expect(&P->L, TK_RPAREN, ")");
            f->a = p_block(P);
            lx_expect(&P->L, TK_ENDE, "Ende");
            // `Ende Funktion` 的收尾词：只有后面不跟函数名时才是收尾，
            // 否则那是下一个函数的开头，吃掉会把下一个函数整个弄丢。
            if (lx_at(&P->L, TK_FUNKTION) && lx_peek(&P->L)->kind != TK_ID)
                lx_next(&P->L);
            node_push(prog, f);
            continue;
        }
        // 顶层语句（允许脚本式写法）
        Node *s = parse_stmt(P);
        if (s) node_push(prog, s);
    }
    return prog;
}

#endif // STARC_AST_H
