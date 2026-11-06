// ============================================================================
//  StarC 编译器 · 代码生成器 (code.h)
//  ---------------------------------------------------------------------------
//  AST → x86-64 机器码。运行时（打印/分配/拼接/整数转串）同样用 emit 手写，
//  所以最终 exe 除 kernel32.dll 外无任何外部依赖。
//
//  值表示：
//    INT  —— 64 位有符号整数
//    STR  —— 指针 → { int64 len; char bytes[]; }
//    ARR  —— 指针 → { int64 len; int64 elem[]; }
//
//  调用约定（内部自用，与 Win64 一致）：
//    参数 rcx rdx r8 r9（最多 4 个），返回值 rax，被调用者清理。
// ============================================================================
#ifndef STARC_CODE_H
#define STARC_CODE_H

#include "emit.h"
#include "ast.h"

#define MAX_VARS   96
#define FRAME_SIZE 704          // 局部变量 + shadow + 临时区，16 字节对齐

typedef struct { char *name; int slot; ValueType ty; } CVar;
typedef struct { char *name; int lab; int nparams; Node *def; int is_stub; } CFunc;

typedef struct {
    EmitCtx    em;
    PEBuilder *pe;
    int  text_sec, rdata_sec, data_sec;
    struct { char *s; int64_t len; uint32_t off; } strs[2048];
    int nstrs;
    uint32_t off_stdout, off_heap;
    int lab_rt_write, lab_rt_pstr, lab_rt_pint, lab_rt_alloc,
        lab_rt_concat, lab_rt_i2s, lab_rt_init, lab_rt_nl,
        lab_rt_thread, lab_rt_join, lab_rt_now, lab_rt_freq,
        // 浮点运行时（IEEE754 位模式在整数寄存器里流转）
        lab_rt_fadd, lab_rt_fsub, lab_rt_fmul, lab_rt_fdiv,
        lab_rt_fsqrt, lab_rt_fabs, lab_rt_fneg, lab_rt_fcmp,
        lab_rt_f2i, lab_rt_i2f;
    CFunc funcs[4096];
    int   nfuncs;
    CVar  vars[MAX_VARS];
    int   nvars, nslots;
    // linx 导入的外部函数表（dll 与函数名）
    struct { char dll[48]; char fn[64]; } linx[128];
    int   nlinx;
    int   lab_ret;
} Codegen;

// ---------------------------------------------------------------- 浮点返回判定
// x64 调用约定：返回 double 的函数把结果放在 XMM0，返回整数/指针的放 RAX。
// starc 的表达式求值器只认 RAX，所以必须在调用后把 XMM0 搬过来。
//
// 这张表列出 msvcrt / ucrt 里「返回 double」的常用函数。
// 表外的函数一律按「返回整数」处理 —— 宁可漏搬（结果可见地错），
// 也不要乱搬（把整数结果搞坏）。
static int code_fn_returns_double(const char *fn) {
    static const char *dbl[] = {
        "atof", "strtod", "_strtod",
        "sqrt", "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
        "log", "log10", "log2", "exp", "exp2", "pow", "hypot", "cbrt",
        "fmod", "remainder", "floor", "ceil", "round", "trunc", "fabs",
        "ldexp", "frexp", "modf", "copysign", "fdim", "fmax", "fmin",
        "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
        "erf", "erfc", "lgamma", "tgamma", "nearbyint", "rint",
        "lround", "llround", "lrint", "llrint",
        NULL
    };
    for (int i = 0; dbl[i]; i++)
        if (!strcmp(fn, dbl[i])) return 1;
    return 0;
}

// ---------------------------------------------------------------- 字符串池
static uint32_t cg_add_str(Codegen *C, const char *s, int64_t len) {
    for (int i = 0; i < C->nstrs; i++)
        if (C->strs[i].len == len && memcmp(C->strs[i].s, s, (size_t)len) == 0)
            return C->strs[i].off;
    Section *rd = &C->pe->secs[C->rdata_sec];
    uint32_t off = (uint32_t)rd->body.len;
    buf_u64(&rd->body, (uint64_t)len);
    buf_bytes(&rd->body, s, (size_t)len);
    buf_u8(&rd->body, 0);
    buf_align(&rd->body, 8);
    if (C->nstrs < 2048) {
        C->strs[C->nstrs].s = sc_strndup(s, (size_t)len);
        C->strs[C->nstrs].len = len;
        C->strs[C->nstrs].off = off;
        C->nstrs++;
    }
    return off;
}
// 由节点里的 C 字符串建常量
static uint32_t cg_str_lit(Codegen *C, const char *s) {
    return cg_add_str(C, s, (int64_t)strlen(s));
}
// 生成 lea rax, [rip+字符串]
static uint32_t cg_lea_str(Codegen *C, int reg, const char *s) {
    uint32_t off = cg_str_lit(C, s);
    Mem m; memset(&m, 0, sizeof(m));
    m.base = -1; m.to_sec = C->rdata_sec; m.to_off = off; m.need_fix = 1;
    m_lea(&C->em, reg, &m);
    return off;
}

// ---------------------------------------------------------------- 作用域
static void cg_scope_reset(Codegen *C) { C->nvars = 0; C->nslots = 0; }
static int cg_declare(Codegen *C, const char *name, ValueType ty) {
    for (int i = 0; i < C->nvars; i++)
        if (!strcmp(C->vars[i].name, name)) {
            if (ty != TY_ANY) C->vars[i].ty = ty;
            return C->vars[i].slot;
        }
    if (C->nvars >= MAX_VARS) { fprintf(stderr, "编译错误: 局部变量超过 %d 个\n", MAX_VARS); exit(1); }
    C->vars[C->nvars].name = sc_strdup(name);
    C->vars[C->nvars].slot = ++C->nslots;
    C->vars[C->nvars].ty   = (ty == TY_ANY) ? TY_INT : ty;
    C->nvars++;
    return C->nslots;
}
static int cg_lookup(Codegen *C, const char *name, ValueType *ty) {
    for (int i = 0; i < C->nvars; i++)
        if (!strcmp(C->vars[i].name, name)) { if (ty) *ty = C->vars[i].ty; return C->vars[i].slot; }
    return -1;
}
static void cg_retype(Codegen *C, const char *name, ValueType ty) {
    for (int i = 0; i < C->nvars; i++)
        if (!strcmp(C->vars[i].name, name)) { C->vars[i].ty = ty; return; }
}
// 内存操作数构造（用复合字面量，才能取地址传给 m_ld/m_st）
#define S(slot)        ((Mem){ .base = RBP, .disp = -8 * (slot), .to_sec = -1 })
#define MRA(_reg, _d)  ((Mem){ .base = (_reg), .disp = (_d), .to_sec = -1 })
#define MABS(sec, off) ((Mem){ .base = -1, .to_sec = (sec), .to_off = (off), .need_fix = 1 })

static ValueType gen_expr(Codegen *C, Node *n);
static void      gen_stmt(Codegen *C, Node *n);

// ================================================================ 运行时
static void emit_runtime(Codegen *C) {
    EmitCtx *e = &C->em;

    // ---- rt_init：设 UTF-8 代码页、取 stdout 句柄、取进程堆 ----
    m_bind(e, C->lab_rt_init);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_mov_ri32(e, RCX, 65001);
    m_call_ext(e, "kernel32.dll!SetConsoleOutputCP");
    m_mov_ri32(e, RCX, -11);
    m_call_ext(e, "kernel32.dll!GetStdHandle");
    m_st(e, &MABS(C->data_sec, C->off_stdout), RAX);
    m_call_ext(e, "kernel32.dll!GetProcessHeap");
    m_st(e, &MABS(C->data_sec, C->off_heap), RAX);
    m_leave(e); m_ret(e);

    // ---- rt_write(rdi=buf, rsi=len) ----
    m_bind(e, C->lab_rt_write);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_st(e, &S(1), RDI);                     // [rbp-8]  = buf
    m_st(e, &S(2), RSI);                     // [rbp-16] = len
    m_ld(e, RCX, &MABS(C->data_sec, C->off_stdout));
    m_ld(e, RDX, &S(1));
    m_ld(e, R8,  &S(2));
    m_lea(e, R9, &S(3));                     // &written
    m_st_i32(e, &S(8), 0);                   // [rbp-64] = [rsp+0x20] = lpOverlapped
    m_call_ext(e, "kernel32.dll!WriteFile");
    m_leave(e); m_ret(e);

    // ---- rt_pstr(rdi=strptr) ----
    m_bind(e, C->lab_rt_pstr);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 32);
    m_mov_rr(e, RAX, RDI);
    m_ld(e, RSI, &MRA(RAX, 0));
    m_add_ri(e, RAX, 8);
    m_mov_rr(e, RDI, RAX);
    m_call(e, C->lab_rt_write);
    m_call(e, C->lab_rt_nl);
    m_leave(e); m_ret(e);

    // ---- rt_nl()：输出一个换行 ----
    m_bind(e, C->lab_rt_pint);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_mov_rr(e, RAX, RDI);
    m_lea(e, RBX, &MRA(RBP, -16));           // 缓冲尾
    m_xor_rr(e, R8, R8);
    m_test_rr(e, RAX, RAX);
    m_mov_ri32(e, R9, 10);
    int p_pos  = m_new_label(e), p_loop = m_new_label(e), p_done = m_new_label(e);
    int p_neg  = m_new_label(e), p_sign = m_new_label(e);
    m_jcc(e, CC_GE, p_pos);
    m_mov_ri32(e, R8, 1);
    m_neg(e, RAX);
    m_bind(e, p_pos);
    m_bind(e, p_loop);
    m_xor_rr(e, RDX, RDX);
    m_idiv(e, R9);
    m_add_ri(e, RDX, 48);
    m_sub_ri(e, RBX, 1);
    em_rex(e, 0, 0, 0, RBX); em_b(e, 0x88); em_b(e, 0x13);      // mov [rbx], dl
    m_test_rr(e, RAX, RAX);
    m_jcc(e, CC_NE, p_loop);
    m_test_rr(e, R8, R8);
    m_jcc(e, CC_E, p_done);
    m_sub_ri(e, RBX, 1);
    em_rex(e, 0, 0, 0, RBX); em_b(e, 0xC6); em_b(e, 0x03); em_b(e, 0x2D); // mov byte [rbx], '-'
    m_bind(e, p_done);
    m_lea(e, RSI, &MRA(RBP, -16));
    m_sub_rr(e, RSI, RBX);
    m_mov_rr(e, RDI, RBX);
    m_call(e, C->lab_rt_write);
    m_call(e, C->lab_rt_nl);
    m_leave(e); m_ret(e);

    // ---- rt_nl() ----
    m_bind(e, C->lab_rt_nl);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 32);
    em_rex(e, 0, 0, 0, RSP); em_b(e, 0xC6); em_b(e, 0x04); em_b(e, 0x24); em_b(e, 0x0A); // mov byte [rsp], 10
    m_mov_rr(e, RDI, RSP);
    m_mov_ri32(e, RSI, 1);
    m_call(e, C->lab_rt_write);
    m_leave(e); m_ret(e);

    // ---- rt_alloc(rdi=size) → rax ----
    // HeapAlloc(hHeap, dwFlags, dwBytes)  →  rcx, rdx, r8
    m_bind(e, C->lab_rt_alloc);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_mov_rr(e, R8, RDI);                    // dwBytes
    // 堆句柄兜底：没初始化过就现场取一次。
    // （DLL 模式下 rt_init 可能没被调到；没有这个兜底，HeapAlloc(NULL,...) 直接崩。）
    m_ld(e, RCX, &MABS(C->data_sec, C->off_heap));
    m_test_rr(e, RCX, RCX);
    int alloc_has_heap = m_new_label(e);
    m_jcc(e, CC_NE, alloc_has_heap);
    m_call_ext(e, "kernel32.dll!GetProcessHeap");
    m_st(e, &MABS(C->data_sec, C->off_heap), RAX);
    m_ld(e, RCX, &MABS(C->data_sec, C->off_heap));
    m_bind(e, alloc_has_heap);
    m_xor_rr(e, RDX, RDX);                   // dwFlags = 0
    m_call_ext(e, "kernel32.dll!HeapAlloc");
    m_leave(e); m_ret(e);

    // ---- rt_i2s(rdi=n) → rax = strptr ----
    m_bind(e, C->lab_rt_i2s);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_st(e, &S(1), RDI);                     // n
    m_mov_ri32(e, RDI, 48);
    m_call(e, C->lab_rt_alloc);
    m_mov_rr(e, R12, RAX);                   // r12 = p
    m_mov_rr(e, RBX, RAX);
    m_add_ri(e, RBX, 48);                    // 缓冲尾
    m_ld(e, RAX, &S(1));
    m_xor_rr(e, R8, R8);
    m_mov_ri32(e, R9, 10);
    int q_pos = m_new_label(e), q_loop = m_new_label(e), q_done = m_new_label(e);
    m_test_rr(e, RAX, RAX);
    m_jcc(e, CC_GE, q_pos);
    m_mov_ri32(e, R8, 1);
    m_neg(e, RAX);
    m_bind(e, q_pos);
    m_bind(e, q_loop);
    m_xor_rr(e, RDX, RDX);
    m_idiv(e, R9);
    m_add_ri(e, RDX, 48);
    m_sub_ri(e, RBX, 1);
    em_rex(e, 0, 0, 0, RBX); em_b(e, 0x88); em_b(e, 0x13);      // mov [rbx], dl
    m_test_rr(e, RAX, RAX);
    m_jcc(e, CC_NE, q_loop);
    m_test_rr(e, R8, R8);
    m_jcc(e, CC_E, q_done);
    m_sub_ri(e, RBX, 1);
    em_rex(e, 0, 0, 0, RBX); em_b(e, 0xC6); em_b(e, 0x03); em_b(e, 0x2D);
    m_bind(e, q_done);
    // len = (p+48) - rbx
    m_mov_rr(e, RSI, R12);
    m_add_ri(e, RSI, 48);
    m_sub_rr(e, RSI, RBX);
    m_st(e, &MRA(R12, 0), RSI);              // p.len
    // memcpy(p+8, rbx, len)
    m_mov_rr(e, RDI, R12);
    m_add_ri(e, RDI, 8);
    m_mov_rr(e, RSI, RBX);
    m_mov_rr(e, RCX, RDI);
    m_mov_rr(e, RCX, R12);
    m_ld(e, RCX, &MRA(RCX, 0));
    m_rep_movsb(e);
    m_mov_rr(e, RAX, R12);
    m_leave(e); m_ret(e);

    // ================================================================
    // 浮点运行时（IEEE754 位模式在整数寄存器里流转）
    // ================================================================
    // 为什么需要：x64 调用约定里返回 double 的函数把结果放在 XMM0，
    // 而 starc 的表达式求值器只认 RAX。
    // 不做这层搬运的话，atof / sqrt / sin 的返回值全是垃圾值
    // （实测 math_fadd("1.5","2.5") 曾返回 1.68e-311）。
    //
    // 约定：浮点值 = 64 位整数里装的 IEEE754 位模式（不解释，只搬运）。
    //   rt_fadd(rdi=a, rsi=b) → rax = a + b（真正的双精度加法）
    //   rt_fsub / rt_fmul / rt_fdiv 同理
    //   rt_fcmp(rdi=a, rsi=b) → rax = -1 / 0 / 1（比较）
    //   rt_f2i(rdi=a)          → rax = 截断成 int64
    //   rt_i2f(rdi=a)          → rax = int64 转 double 的位模式
    //   rt_fsqrt(rdi=a)        → rax = sqrt(a)
    //   rt_fabs(rdi=a)         → rax = |a|

    // ---- rt_fadd(rdi=a, rsi=b) → rax ----
    m_bind(e, C->lab_rt_fadd);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_movq_xmm_rr(e, 0, RDI);          // xmm0 = a
    m_movq_xmm_rr(e, 1, RSI);          // xmm1 = b
    m_addsd(e, 0, 1);                  // xmm0 = a + b
    m_movq_rr_xmm(e, RAX, 0);
    m_leave(e); m_ret(e);

    // ---- rt_fsub(rdi=a, rsi=b) → rax ----
    m_bind(e, C->lab_rt_fsub);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_movq_xmm_rr(e, 0, RDI);
    m_movq_xmm_rr(e, 1, RSI);
    m_subsd(e, 0, 1);
    m_movq_rr_xmm(e, RAX, 0);
    m_leave(e); m_ret(e);

    // ---- rt_fmul(rdi=a, rsi=b) → rax ----
    m_bind(e, C->lab_rt_fmul);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_movq_xmm_rr(e, 0, RDI);
    m_movq_xmm_rr(e, 1, RSI);
    m_mulsd(e, 0, 1);
    m_movq_rr_xmm(e, RAX, 0);
    m_leave(e); m_ret(e);

    // ---- rt_fdiv(rdi=a, rsi=b) → rax ----
    m_bind(e, C->lab_rt_fdiv);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_movq_xmm_rr(e, 0, RDI);
    m_movq_xmm_rr(e, 1, RSI);
    m_divsd(e, 0, 1);
    m_movq_rr_xmm(e, RAX, 0);
    m_leave(e); m_ret(e);

    // ---- rt_fsqrt(rdi=a) → rax ----
    m_bind(e, C->lab_rt_fsqrt);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_movq_xmm_rr(e, 0, RDI);
    m_sqrtsd(e, 0, 0);
    m_movq_rr_xmm(e, RAX, 0);
    m_leave(e); m_ret(e);

    // ---- rt_fabs(rdi=a) → rax（清掉符号位）----
    m_bind(e, C->lab_rt_fabs);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_mov_rr(e, RAX, RDI);
    m_mov_ri(e, RCX, 9223372036854775807LL);
    m_and_rr(e, RAX, RCX);
    m_leave(e); m_ret(e);

    // ---- rt_fneg(rdi=a) → rax（翻转符号位）----
    m_bind(e, C->lab_rt_fneg);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_mov_rr(e, RAX, RDI);
    m_mov_ri(e, RCX, -9223372036854775807LL - 1);
    m_xor_rr(e, RAX, RCX);
    m_leave(e); m_ret(e);

    // ---- rt_fcmp(rdi=a, rsi=b) → rax = -1/0/1 ----
    // ucomisd 设 ZF/PF/CF：CF=1 表示 a<b，ZF=1 表示 a==b（NaN 时 PF=1）
    m_bind(e, C->lab_rt_fcmp);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_movq_xmm_rr(e, 0, RDI);
    m_movq_xmm_rr(e, 1, RSI);
    m_ucomisd(e, 0, 1);                 // 比较 xmm0(a) 与 xmm1(b)
    int fc_lt = m_new_label(e), fc_eq = m_new_label(e), fc_gt = m_new_label(e), fc_end = m_new_label(e);
    m_jcc(e, CC_B, fc_lt);              // CF=1 → a < b
    m_jcc(e, CC_E, fc_eq);              // ZF=1 → a == b
    m_mov_ri32(e, RAX, 1);
    m_jmp(e, fc_end);
    m_bind(e, fc_lt);
    m_mov_ri32(e, RAX, -1);
    m_jmp(e, fc_end);
    m_bind(e, fc_eq);
    m_xor_rr(e, RAX, RAX);
    m_bind(e, fc_gt);
    m_bind(e, fc_end);
    m_leave(e); m_ret(e);

    // ---- rt_f2i(rdi=a) → rax = 截断后的 int64 ----
    m_bind(e, C->lab_rt_f2i);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_movq_xmm_rr(e, 0, RDI);
    m_cvttsd2si(e, RAX, 0);
    m_leave(e); m_ret(e);

    // ---- rt_i2f(rdi=a) → rax = double 位模式 ----
    m_bind(e, C->lab_rt_i2f);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_cvtsi2sd(e, 0, RDI);
    m_movq_rr_xmm(e, RAX, 0);
    m_leave(e); m_ret(e);

    // ---- rt_concat(rdi=a, rsi=b) → rax ----
    m_bind(e, C->lab_rt_concat);
    m_st(e, &S(1), RDI);
    m_st(e, &S(2), RSI);
    m_ld(e, RAX, &S(1));
    m_ld(e, RCX, &MRA(RAX, 0));
    m_ld(e, RDX, &S(2));
    m_ld(e, RDX, &MRA(RDX, 0));
    m_add_rr(e, RCX, RDX);
    m_st(e, &S(3), RCX);                     // 总长
    m_mov_rr(e, RDI, RCX);
    m_add_ri(e, RDI, 8);
    m_call(e, C->lab_rt_alloc);
    m_mov_rr(e, R12, RAX);
    m_ld(e, RCX, &S(3));
    m_st(e, &MRA(R12, 0), RCX);              // p.len
    // 拷 a
    m_mov_rr(e, RDI, R12); m_add_ri(e, RDI, 8);
    m_ld(e, RSI, &S(1));   m_add_ri(e, RSI, 8);
    m_ld(e, RCX, &S(1));   m_ld(e, RCX, &MRA(RCX, 0));
    m_rep_movsb(e);
    // 拷 b
    m_ld(e, RSI, &S(2));   m_add_ri(e, RSI, 8);
    m_ld(e, RCX, &S(2));   m_ld(e, RCX, &MRA(RCX, 0));
    m_rep_movsb(e);
    m_mov_rr(e, RAX, R12);
    m_leave(e); m_ret(e);

    // ---- rt_now() → rax = QueryPerformanceCounter ----
    m_bind(e, C->lab_rt_now);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_lea(e, RCX, &MRA(RBP, -8));
    m_call_ext(e, "kernel32.dll!QueryPerformanceCounter");
    m_ld(e, RAX, &MRA(RBP, -8));
    m_leave(e); m_ret(e);

    // ---- rt_freq() → rax = 每秒计数 ----
    m_bind(e, C->lab_rt_freq);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_lea(e, RCX, &MRA(RBP, -8));
    m_call_ext(e, "kernel32.dll!QueryPerformanceFrequency");
    m_ld(e, RAX, &MRA(RBP, -8));
    m_leave(e); m_ret(e);

    // ---- rt_thread(rdi=fnptr) → rax = handle ----
    // CreateThread(NULL, 0, fn, NULL, 0, &tid)
    m_bind(e, C->lab_rt_thread);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_mov_rr(e, R8, RDI);                    // lpStartAddress
    m_xor_rr(e, RCX, RCX);                   // lpThreadAttributes
    m_xor_rr(e, RDX, RDX);                   // dwStackSize
    m_xor_rr(e, R9, R9);                     // lpParameter
    m_st_i32(e, &MRA(RBP, -64), 0);          // dwCreationFlags  = [rsp+0x20]
    m_lea(e, RAX, &MRA(RBP, -72));           // &threadId
    m_st(e, &MRA(RBP, -56), RAX);            // [rsp+0x28] = &threadId
    m_call_ext(e, "kernel32.dll!CreateThread");
    m_leave(e); m_ret(e);

    // ---- rt_join(rdi=handle) ----
    m_bind(e, C->lab_rt_join);
    m_push(e, RBP); m_mov_rr(e, RBP, RSP); m_sub_ri(e, RSP, 96);
    m_mov_rr(e, RCX, RDI);
    m_mov_ri32(e, RDX, -1);                  // INFINITE
    m_call_ext(e, "kernel32.dll!WaitForSingleObject");
    m_leave(e); m_ret(e);
}

// ================================================================ 表达式
static ValueType gen_bin(Codegen *C, Node *n);

// 纯静态类型推断（不产码）：用来判断某个二元运算该走整数快路径还是字符串拼接
static ValueType type_of(Codegen *C, Node *n) {
    if (!n) return TY_INT;
    switch (n->kind) {
        case N_NUM: return TY_INT;
        case N_STR: return TY_STR;
        case N_VAR: {
            ValueType t = TY_INT;
            if (cg_lookup(C, n->name, &t) > 0) return t;
            return TY_INT;
        }
        case N_ARRAY: return TY_ARR;
        case N_INDEX: return TY_INT;
        case N_CALL:  return TY_INT;
        case N_UN:    return TY_INT;
        case N_BIN: {
            const char *o = n->name;
            if (!o) return TY_INT;
            if (!strcmp(o, "+")) {
                if (type_of(C, n->a) == TY_STR || type_of(C, n->b) == TY_STR) return TY_STR;
                return TY_INT;
            }
            return TY_INT;
        }
        default: return TY_INT;
    }
}

// 统一发参数：前 4 个走 rcx/rdx/r8/r9，第 5 个起按 Win64 约定压栈
// 返回本次分配的栈字节数（调用方负责还原 rsp）
//
// ★ Win64 栈参数布局：调用前 rsp 减 N（N=32 表示 4 个寄存器参数的影子区），
//   第 5 个参数必须放在 [rsp+32]，第 6 个在 [rsp+40]……
//   callee 侧对应 [rbp+16+8*4] = [rbp+48]。
//   之前把第 5 个参数放在 [rsp+0]，callee 从 [rbp+48] 读 → 读到的是第 1 个参数，
//   表现为「5 参数函数少算一项」。
static int emit_args(Codegen *C, Node *n) {
    EmitCtx *e = &C->em;
    int cnt = n->n;
    if (cnt <= 0) return 0;
    int need = 32 + (cnt > 4 ? 8 * (cnt - 4) : 0);
    int size = (need + 15) / 16 * 16;          // 16 字节对齐
    m_sub_ri(e, RSP, size);
    for (int i = 0; i < cnt; i++) {
        gen_expr(C, n->list[i]);
        m_st(e, &MRA(RSP, 8 * i), RAX);
    }
    // ★ AL = 使用的向量寄存器个数（Win64 变参约定）★
    //   starc 目前不支持把浮点传给外部函数，所以恒为 0。
    //   ★ 必须显式清零 ★：AL 里若残留垃圾值非 0，被调用方（如 msvcrt 的
    //   printf）会以为有向量参数，把 8 个 XMM 寄存器保存到 shadow space ——
    //   而这里只预留了 32 字节影子区，128 字节的保存会踩坏栈。
    //   症状：某些函数偶发崩溃，取决于上一次调用残留的 AL 值。
    m_mov_ri32(e, RAX, 0);
    m_ld(e, RCX, &MRA(RSP, 0));
    if (cnt > 1) m_ld(e, RDX, &MRA(RSP, 8));
    if (cnt > 2) m_ld(e, R8,  &MRA(RSP, 16));
    if (cnt > 3) m_ld(e, R9,  &MRA(RSP, 24));
    return size;
}




static ValueType gen_call(Codegen *C, Node *n) {
    EmitCtx *e = &C->em;
    const char *fn = n->name;

    // ---- 内置 ----
    if (!strcmp(fn, "jetzt") || !strcmp(fn, "now")) {      // 高精度计数器
        m_call(e, C->lab_rt_now);
        return TY_INT;
    }
    if (!strcmp(fn, "takt") || !strcmp(fn, "freq")) {      // 每秒计数
        m_call(e, C->lab_rt_freq);
        return TY_INT;
    }
    if (!strcmp(fn, "liste_laenge") || !strcmp(fn, "len")) {
        gen_expr(C, n->list[0]);
        m_ld(e, RAX, &MRA(RAX, 0));
        return TY_INT;
    }
    if (!strcmp(fn, "i2s") || !strcmp(fn, "int2str")) {
        if (n->n < 1) { fprintf(stderr, "编译错误: i2s 需要 1 个参数\n"); exit(1); }
        gen_expr(C, n->list[0]);
        m_mov_rr(e, RDI, RAX);      // rt_i2s 期望参数在 RDI
        m_call(e, C->lab_rt_i2s);
        return TY_STR;
    }
    if (!strcmp(fn, "zufall_ganz")) {
        // 简易 LCG：(seed*1103515245+12345) & 0x7fffffff
        if (n->n >= 1) gen_expr(C, n->list[0]); else m_mov_ri32(e, RAX, 1);
        m_imul_rr(e, RAX, RAX);
        m_mov_ri(e, RCX, 1103515245LL);
        m_imul_rr(e, RAX, RCX);
        m_add_ri(e, RAX, 12345);
        m_mov_ri(e, RCX, 0x7FFFFFFF);
        m_and_rr(e, RAX, RCX);
        return TY_INT;
    }
    // setbyte(buf, pos, val) → 把 buf[pos] 写成一个字节（0~255）
    // 为什么需要它：memcpy 的「源参数带表达式」在 starc 里不可靠
    // （memcpy(dst+0, src+8+0, 1) 实际会写错字节），
    // 所以逐字节写必须走这个内建。
    // 这是 starc 处理「字节流」的基础原语：URL 编码、base64、序列化都靠它。
    if (!strcmp(fn, "setbyte")) {
        if (n->n < 3) { fprintf(stderr, "编译错误: setbyte 需要 3 个参数（缓冲, 位置, 字节值）\n"); exit(1); }
        // 顺序：先算全部参数并存到栈槽（求值期间不动参数寄存器），
        // 然后一次性搬到工作寄存器，最后写内存。
        // ★ 关键：不能边求值边用寄存器 —— gen_expr 可能用到 RCX/RDX，
        //   把已经算好的参数冲掉（实测：第 1 个参数是函数参数时，写入位置全错）。
        gen_expr(C, n->list[2]);                       // rax = val
        m_sub_ri(e, RSP, 32);
        m_st(e, &MRA(RSP, 0), RAX);                    // [rsp+0]  = val
        gen_expr(C, n->list[1]);                       // rax = pos
        m_st(e, &MRA(RSP, 8), RAX);                    // [rsp+8]  = pos
        gen_expr(C, n->list[0]);                       // rax = buf
        m_st(e, &MRA(RSP, 16), RAX);                   // [rsp+16] = buf
        m_ld(e, RDX, &MRA(RSP, 16));                   // rdx = buf
        m_ld(e, RCX, &MRA(RSP, 8));                    // rcx = pos
        m_ld(e, RAX, &MRA(RSP, 0));                    // rax = val
        m_add_ri(e, RSP, 32);                          // 恢复栈指针
        m_add_rr(e, RDX, RCX);                         // rdx = buf + pos
        // mov byte [rdx], al  →  88 /r（mod≠11 才是「内存」寻址）
        {
            Mem mb; memset(&mb, 0, sizeof(mb));
            mb.base = RDX; mb.disp = 0;
            em_rex(e, 0, RAX, 0, RDX);
            em_b(e, 0x88);
            em_modrm_mem(e, RAX, &mb);
        }
        return TY_INT;
    }
    // byteat(str, i) → 第 i 个字节的数值（0~255）
    // starc 的字节级读取原语：字符串本身就是 UTF-8 字节流，
    // 过去无法逐字节取出，导致 URL 百分号编码、base64、二进制解析都做不了。
    // 约定：str 是 starc 字符串（内部布局 [len(4字节)][数据...]），i 从 0 开始。
    if (!strcmp(fn, "byteat")) {
        if (n->n < 2) { fprintf(stderr, "编译错误: byteat 需要 2 个参数（字符串, 下标）\n"); exit(1); }
        gen_expr(C, n->list[1]);                       // rax = i
        m_sub_ri(e, RSP, 16);
        m_st(e, &MRA(RSP, 0), RAX);                    // 保存 i
        gen_expr(C, n->list[0]);                       // rax = str 指针（指向 [len][data]）
        m_ld(e, RCX, &MRA(RSP, 0));                    // rcx = i
        m_add_ri(e, RSP, 16);
        m_add_ri(e, RAX, 8);                           // rax = str + 8 = 数据区起点
        m_add_rr(e, RAX, RCX);                         // rax = &data[i]
        {   // movzx rax, byte [rax]
            Mem mb; memset(&mb, 0, sizeof(mb));
            mb.base = RAX; mb.disp = 0;
            m_ld_u8(e, RAX, &mb);
        }
        return TY_INT;
    }
    // alloc(size) → 分配原始内存，返回指针
    // 直接调用 rt_alloc 运行时函数
    if (!strcmp(fn, "alloc")) {
        if (n->n < 1) { fprintf(stderr, "编译错误: alloc 需要 1 个参数（大小）\n"); exit(1); }
        gen_expr(C, n->list[0]);                       // rax = size
        m_mov_rr(e, RDI, RAX);                        // rt_alloc 期望参数在 RDI
        m_call(e, C->lab_rt_alloc);
        return TY_INT;
    }
    // raw_byteat(ptr, i) → 从原始内存读取字节（无 8 字节偏移）
    // 用于操作 alloc 分配的原始内存，不跳过 StarC 字符串头
    if (!strcmp(fn, "raw_byteat")) {
        if (n->n < 2) { fprintf(stderr, "编译错误: raw_byteat 需要 2 个参数（指针, 下标）\n"); exit(1); }
        gen_expr(C, n->list[1]);                       // rax = i
        m_sub_ri(e, RSP, 16);
        m_st(e, &MRA(RSP, 0), RAX);                    // 保存 i
        gen_expr(C, n->list[0]);                       // rax = ptr（原始内存指针）
        m_ld(e, RCX, &MRA(RSP, 0));                    // rcx = i
        m_add_ri(e, RSP, 16);
        // 注意：不加 8，直接 ptr + i
        m_add_rr(e, RAX, RCX);                         // rax = ptr + i
        {   // movzx rax, byte [rax]
            Mem mb; memset(&mb, 0, sizeof(mb));
            mb.base = RAX; mb.disp = 0;
            m_ld_u8(e, RAX, &mb);
        }
        return TY_INT;
    }
    // raw_setbyte(ptr, i, val) → 向原始内存写入字节（无 8 字节偏移）
    if (!strcmp(fn, "raw_setbyte")) {
        if (n->n < 3) { fprintf(stderr, "编译错误: raw_setbyte 需要 3 个参数（指针, 下标, 字节值）\n"); exit(1); }
        gen_expr(C, n->list[2]);                       // rax = val
        m_sub_ri(e, RSP, 32);
        m_st(e, &MRA(RSP, 0), RAX);                    // [rsp+0]  = val
        gen_expr(C, n->list[1]);                       // rax = pos
        m_st(e, &MRA(RSP, 8), RAX);                    // [rsp+8]  = pos
        gen_expr(C, n->list[0]);                       // rax = ptr
        m_st(e, &MRA(RSP, 16), RAX);                   // [rsp+16] = ptr
        m_ld(e, RDX, &MRA(RSP, 16));                   // rdx = ptr
        m_ld(e, RCX, &MRA(RSP, 8));                    // rcx = pos
        m_ld(e, RAX, &MRA(RSP, 0));                    // rax = val
        m_add_ri(e, RSP, 32);                          // 恢复栈指针
        m_add_rr(e, RDX, RCX);                         // rdx = ptr + pos
        {   // mov byte [rdx], al
            Mem mb; memset(&mb, 0, sizeof(mb));
            mb.base = RDX; mb.disp = 0;
            em_rex(e, 0, RAX, 0, RDX);
            em_b(e, 0x88);
            em_modrm_mem(e, RAX, &mb);
        }
        return TY_INT;
    }
    // cstr(str) → 返回 C 字符串指针（跳过 8 字节 StarC 字符串头）
    // StarC 字符串布局：[length(8 bytes)][data...][null]
    // C 标准库函数期望 null 终止的 char*，所以必须 str + 8
    if (!strcmp(fn, "cstr")) {
        if (n->n < 1) { fprintf(stderr, "编译错误: cstr 需要 1 个参数（字符串）\n"); exit(1); }
        gen_expr(C, n->list[0]);    // rax = str pointer
        m_add_ri(e, RAX, 8);        // rax = str + 8 (skip header)
        return TY_INT;
    }
    // as_str(ptr) → 把指针当作 StarC 字符串返回（用于打印用户函数返回的字符串）
    // 用户函数返回字符串指针时，gen_call 返回 TY_INT，导致 Ausgeben 按整数打印。
    // 用 as_str(ptr) 强制标记为 TY_STR，这样 Ausgeben 就会调用 rt_pstr。
    if (!strcmp(fn, "as_str")) {
        if (n->n < 1) { fprintf(stderr, "编译错误: as_str 需要 1 个参数（指针）\n"); exit(1); }
        gen_expr(C, n->list[0]);    // rax = ptr
        return TY_STR;
    }
    // ★ atof 不做内建 ★
    //   它走「外部函数 + 返回值自动搬运」路径：
    //   code_fn_returns_double("atof") 命中 → 调用后自动 movq rax, xmm0。
    //   实测：内建版在 DLL 里拿不到正确参数（返回 0），外部函数版正常。
    //   参数约定：外部函数收到的是「头部指针」，
    //   所以调用方必须写 atof(cstr("1.5"))，不能写 atof(s)。
    // ★ f2s 暂不提供 ★
    //   它需要调用 sprintf 的浮点变参（实参在 XMM0、AL=1），
    //   实测在当前编译器下会崩（rt_f2s 的变参调用路径不稳）。
    //   替代方案：标准库用「定点整数」（放大 10^15）表示浮点，
    //   由 sc_ftoa / sc_fparse 自己完成十进制 ↔ 定点的转换。
    //   —— 纯 starc 实现，反而更符合「自研、不依赖第三方」的要求。
    // ---- 浮点内建 ----
    // 浮点值 = 64 位整数里装的 IEEE754 位模式。
    // 这些内建直接调用运行时的 SSE 实现，保证 a+b 之类是真���双精度运算。
    //   fadd(a,b) fsub(a,b) fmul(a,b) fdiv(a,b)
    //   fsqrt(a)  fabs(a)  fneg(a)
    //   fcmp(a,b) → -1/0/1
    //   f2i(a)    → 截断成整数
    //   i2f(a)    → 整数转浮点位模式
    //   f2s(a)    → 浮点转字符串（走 sprintf 的 %g）
    if (!strcmp(fn, "fadd") || !strcmp(fn, "fsub") || !strcmp(fn, "fmul") || !strcmp(fn, "fdiv")) {
        if (n->n < 2) { fprintf(stderr, "编译错误: %s 需要 2 个参数\n", fn); exit(1); }
        // ★ 用 setbyte 内建那种「先算完所有参数 → 存栈槽 → 再搬寄存器」的模式 ★
        //   不能用 emit_args：它把参数存到 [rsp+8*i]，而求值过程中
        //   嵌套调用的 emit_args 会临时改 RSP，导致存/取的位置错位
        //   （实测 fadd(atof(...), 100) 段错误）。
        //   这里改成：先求值第 2 个参数（存栈槽），再求值第 1 个参数（存栈槽），
        //   最后一次性搬到 rdi/rsi。
        gen_expr(C, n->list[1]);                       // rax = 参数2
        m_sub_ri(e, RSP, 32);                          // 32 字节槽（保持 16 对齐）
        m_st(e, &MRA(RSP, 0), RAX);                    // [rsp+0] = 参数2
        gen_expr(C, n->list[0]);                       // rax = 参数1
        m_st(e, &MRA(RSP, 8), RAX);                    // [rsp+8] = 参数1
        m_ld(e, RDI, &MRA(RSP, 8));                    // rdi = 参数1
        m_ld(e, RSI, &MRA(RSP, 0));                    // rsi = 参数2
        int lab = !strcmp(fn, "fadd") ? C->lab_rt_fadd
               : !strcmp(fn, "fsub") ? C->lab_rt_fsub
               : !strcmp(fn, "fmul") ? C->lab_rt_fmul : C->lab_rt_fdiv;
        m_call(e, lab);
        m_add_ri(e, RSP, 32);
        return TY_INT;
    }
    if (!strcmp(fn, "fsqrt") || !strcmp(fn, "fabs") || !strcmp(fn, "fneg") || !strcmp(fn, "f2i") || !strcmp(fn, "i2f")) {
        if (n->n < 1) { fprintf(stderr, "编译错误: %s 需要 1 个参数\n", fn); exit(1); }
        gen_expr(C, n->list[0]);                       // rax = 参数
        m_sub_ri(e, RSP, 16);                          // 16 字节槽（保持栈 16 对齐）
        m_st(e, &MRA(RSP, 0), RAX);                    // [rsp+0] = 参数
        m_ld(e, RDI, &MRA(RSP, 0));
        int lab = !strcmp(fn, "fsqrt") ? C->lab_rt_fsqrt
               : !strcmp(fn, "fabs")  ? C->lab_rt_fabs
               : !strcmp(fn, "fneg")  ? C->lab_rt_fneg
               : !strcmp(fn, "f2i")   ? C->lab_rt_f2i : C->lab_rt_i2f;
        m_call(e, lab);
        m_add_ri(e, RSP, 16);
        return TY_INT;
    }
    if (!strcmp(fn, "fcmp")) {
        if (n->n < 2) { fprintf(stderr, "编译错误: fcmp 需要 2 个参数\n"); exit(1); }
        gen_expr(C, n->list[1]);                       // rax = 参数2
        m_sub_ri(e, RSP, 32);
        m_st(e, &MRA(RSP, 0), RAX);
        gen_expr(C, n->list[0]);                       // rax = 参数1
        m_st(e, &MRA(RSP, 8), RAX);
        m_ld(e, RDI, &MRA(RSP, 8));
        m_ld(e, RSI, &MRA(RSP, 0));
        m_call(e, C->lab_rt_fcmp);
        m_add_ri(e, RSP, 32);
        return TY_INT;
    }
    // ---- 用户函数 ----
    int idx = -1;
    for (int i = 0; i < C->nfuncs; i++) if (!strcmp(C->funcs[i].name, fn)) { idx = i; break; }
    if (idx < 0) {
        // 查 linx 导入的外部函数
        for (int i = 0; i < C->nlinx; i++) {
            if (strcmp(C->linx[i].fn, fn) == 0) {
                // 统一发参数：前 4 个走寄存器，第 5 个起按 Win64 约定压栈
                int asz = emit_args(C, n);
                char sym[112];
                snprintf(sym, sizeof(sym), "%s!%s", C->linx[i].dll, C->linx[i].fn);
                m_call_ext(e, sym);
                // ★ 返回 double 的函数，结果在 XMM0 —— 必须搬到 RAX ★
                //   x64 调用约定：整数返回在 RAX，浮点返回在 XMM0。
                //   表达式求值器只读 RAX，不搬的话拿到的是垃圾值。
                if (code_fn_returns_double(C->linx[i].fn)) {
                    m_movq_rr_xmm(e, RAX, 0);   // movq rax, xmm0
                }
                if (asz) m_add_ri(e, RSP, asz);
                return TY_INT;
            }
        }
        // ★ 方案 C：动态函数生成 ★
        // 未定义的函数，生成一个桩函数（stub），返回 0
        // 如果后续定义了该函数，真实实现会覆盖桩函数
        if (C->nfuncs >= 4096) { fprintf(stderr, "错误: 函数超过 4096 个\n"); exit(1); }
        C->funcs[C->nfuncs].name = sc_strdup(fn);
        C->funcs[C->nfuncs].lab = m_new_label(e);
        C->funcs[C->nfuncs].nparams = n->n;
        C->funcs[C->nfuncs].def = NULL;  // NULL 表示这是桩函数
        C->funcs[C->nfuncs].is_stub = 1;  // 标记为桩函数
        idx = C->nfuncs++;
        if (getenv("SCDBG")) fprintf(stderr, "[dbg] 动态生成桩函数: %s (参数 %d)\n", fn, n->n);
    }
    int asz = emit_args(C, n);
    m_call(e, C->funcs[idx].lab);
    if (asz) m_add_ri(e, RSP, asz);
    return TY_INT;      // 函数返回类型统一按 INT；字符串返回按指针也能跑
}

static ValueType gen_bin(Codegen *C, Node *n) {
    EmitCtx *e = &C->em;
    const char *op = n->name;

    // 短路逻辑
    if (!strcmp(op, "und")) {
        ValueType lt = gen_expr(C, n->a);
        (void)lt;
        int l = m_new_label(e);
        m_test_rr(e, RAX, RAX);
        m_jcc(e, CC_E, l);
        gen_expr(C, n->b);
        m_bind(e, l);
        return TY_INT;
    }
    if (!strcmp(op, "oder")) {
        ValueType lt = gen_expr(C, n->a);
        (void)lt;
        int l = m_new_label(e);
        m_test_rr(e, RAX, RAX);
        m_jcc(e, CC_NE, l);
        gen_expr(C, n->b);
        m_bind(e, l);
        return TY_INT;
    }

    // ---- 快路径：右操作数是变量或常量时，直接用内存/立即数操作数 ----
    // 这是热点循环的关键优化：`s = s + i` 从 10 条指令降到 3 条。
    if (n->b && (n->b->kind == N_VAR || n->b->kind == N_NUM) &&
        type_of(C, n->a) == TY_INT && type_of(C, n->b) == TY_INT &&
        (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || strcmp(op, "*") == 0 ||
         strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
         strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0 || strcmp(op, "==") == 0))
    {
        ValueType lt2 = gen_expr(C, n->a);
        (void)lt2;
        if (n->b->kind == N_NUM) {
            int32_t iv = (int32_t)n->b->ival;
            if (!strcmp(op, "+"))      m_add_ri32(e, RAX, iv);
            else if (!strcmp(op, "-")) m_alu_ri(e, 5, RAX, iv);
            else if (!strcmp(op, "<")) { m_cmp_ri32(e, RAX, iv); m_setcc(e, CC_L, RAX); m_movzx8(e, RAX, RAX); }
            else if (!strcmp(op, ">")) { m_cmp_ri32(e, RAX, iv); m_setcc(e, CC_G, RAX); m_movzx8(e, RAX, RAX); }
            else if (!strcmp(op, "<=")){ m_cmp_ri32(e, RAX, iv); m_setcc(e, CC_LE, RAX); m_movzx8(e, RAX, RAX); }
            else if (!strcmp(op, ">=")){ m_cmp_ri32(e, RAX, iv); m_setcc(e, CC_GE, RAX); m_movzx8(e, RAX, RAX); }
            else if (!strcmp(op, "==")){ m_cmp_ri32(e, RAX, iv); m_setcc(e, CC_E, RAX); m_movzx8(e, RAX, RAX); }
            else {
                // 乘法常量：优先 LEA / SHL，避免 mov+imul
                int64_t k = n->b->ival;
                if (k == 3 || k == 5 || k == 9)      m_mul_lea(e, RAX, (int)k);
                else if (k == 2 || k == 4 || k == 8) m_shl_imm(e, RAX, (k == 2) ? 1 : (k == 4) ? 2 : 3);
                else { m_mov_ri(e, RBX, k); m_imul_rr(e, RAX, RBX); }
            }
            return TY_INT;
        }
        // 右子是变量
        ValueType vt = TY_INT;
        int slot = cg_lookup(C, n->b->name, &vt);
        if (slot > 0 && vt == TY_INT) {
            if (!strcmp(op, "+"))      m_add_rm(e, RAX, &S(slot));
            else if (!strcmp(op, "-")) m_sub_rm(e, RAX, &S(slot));
            else if (!strcmp(op, "*")) m_imul_rm(e, RAX, &S(slot));
            else {
                m_cmp_rm(e, RAX, &S(slot));
                int cc = (!strcmp(op,"<")) ? CC_L : (!strcmp(op,">")) ? CC_G
                       : (!strcmp(op,"<=")) ? CC_LE : (!strcmp(op,">=")) ? CC_GE : CC_E;
                m_setcc(e, cc, RAX);
                m_movzx8(e, RAX, RAX);
            }
            return TY_INT;
        }
    }

    // 关键：临时保存必须一次 sub 16 字节，保证 rsp 始终 16 对齐；
    //       用 push 一次会让后面的 call 错位 8 字节，HeapAlloc/WriteFile 会直接崩。
    ValueType lt = gen_expr(C, n->a);
    m_sub_ri(e, RSP, 16);
    m_st(e, &MRA(RSP, 8), RAX);          // [rsp+8] = 左
    ValueType rt = gen_expr(C, n->b);
    m_st(e, &MRA(RSP, 0), RAX);          // [rsp]   = 右
    // 栈：[rsp]=右, [rsp+8]=左

    // ---- 字符串拼接 ----
    if (!strcmp(op, "+") && (lt == TY_STR || rt == TY_STR)) {
        if (lt == TY_INT) {
            m_ld(e, RDI, &MRA(RSP, 8));
            m_call(e, C->lab_rt_i2s);
            m_st(e, &MRA(RSP, 8), RAX);
        }
        if (rt == TY_INT) {
            m_ld(e, RDI, &MRA(RSP, 0));
            m_call(e, C->lab_rt_i2s);
            m_st(e, &MRA(RSP, 0), RAX);
        }
        m_ld(e, RDI, &MRA(RSP, 8));
        m_ld(e, RSI, &MRA(RSP, 0));
        m_call(e, C->lab_rt_concat);
        m_add_ri(e, RSP, 16);
        return TY_STR;
    }

    m_ld(e, RBX, &MRA(RSP, 0));          // rbx = 右
    m_ld(e, RAX, &MRA(RSP, 8));          // rax = 左
    m_add_ri(e, RSP, 16);
    if (!strcmp(op, "+"))  m_add_rr(e, RAX, RBX);
    else if (!strcmp(op, "-")) m_sub_rr(e, RAX, RBX);
    else if (!strcmp(op, "*")) m_imul_rr(e, RAX, RBX);
    else if (!strcmp(op, "/")) { m_cqo(e); m_idiv(e, RBX); }
    else if (!strcmp(op, "%")) { m_cqo(e); m_idiv(e, RBX); m_mov_rr(e, RAX, RDX); }
    else {
        int cc = CC_E;
        if (!strcmp(op, "==")) cc = CC_E;
        else if (!strcmp(op, "!=")) cc = CC_NE;
        else if (!strcmp(op, "<"))  cc = CC_L;
        else if (!strcmp(op, ">"))  cc = CC_G;
        else if (!strcmp(op, "<=")) cc = CC_LE;
        else if (!strcmp(op, ">=")) cc = CC_GE;
        m_cmp_rr(e, RAX, RBX);
        m_setcc(e, cc, RAX);
        m_movzx8(e, RAX, RAX);
    }
    return TY_INT;
}

static ValueType gen_expr(Codegen *C, Node *n) {
    EmitCtx *e = &C->em;
    if (!n) return TY_INT;
    switch (n->kind) {
        case N_NUM:
            m_mov_ri(e, RAX, n->ival);
            return TY_INT;
        case N_STR:
            cg_lea_str(C, RAX, n->sval);
            return TY_STR;
        case N_VAR: {
            ValueType ty = TY_INT;
            int slot = cg_lookup(C, n->name, &ty);
            if (slot < 0) {
                fprintf(stderr, "编译错误 第%d行: 未声明的变量 %s\n", n->line, n->name);
                exit(1);
            }
            m_ld(e, RAX, &S(slot));
            return ty;
        }
        case N_BIN:
            return gen_bin(C, n);
        case N_UN: {
            gen_expr(C, n->a);
            if (n->name && n->name[0] == '-') m_neg(e, RAX);
            else {
                m_test_rr(e, RAX, RAX);
                m_setcc(e, CC_E, RAX);
                m_movzx8(e, RAX, RAX);
            }
            return TY_INT;
        }
        case N_CALL:
            return gen_call(C, n);
        case N_ARRAY: {
            // p = alloc(8 + 8*n); p[0] = n; p[1..n] = elems
            int cnt = n->n;
            m_mov_ri32(e, RDI, 8 + 8 * cnt);
            m_call(e, C->lab_rt_alloc);
            m_sub_ri(e, RSP, 16);                 // 16 字节对齐的临时槽
            m_st(e, &MRA(RSP, 0), RAX);           // 保存 p
            m_mov_ri32(e, RCX, cnt);
            m_st(e, &MRA(RAX, 0), RCX);
            for (int i = 0; i < cnt; i++) {
                gen_expr(C, n->list[i]);
                m_ld(e, RCX, &MRA(RSP, 0));
                m_st(e, &MRA(RCX, 8 + 8 * i), RAX);
            }
            m_ld(e, RAX, &MRA(RSP, 0));
            m_add_ri(e, RSP, 16);
            return TY_ARR;
        }
        case N_INDEX: {
            gen_expr(C, n->a);                     // 基址
            m_sub_ri(e, RSP, 16);
            m_st(e, &MRA(RSP, 0), RAX);
            gen_expr(C, n->b);                     // 下标
            m_ld(e, RCX, &MRA(RSP, 0));
            m_add_ri(e, RSP, 16);
            // rax = [rcx + 8 + rax*8]
            em_rex(e, 1, RAX, RAX, RCX);
            em_b(e, 0x8B);
            em_b(e, 0x84);                          // mod=10 reg=rax rm=100(SIB)
            em_b(e, 0xC1);                          // scale=8 index=rax base=rcx
            em_d(e, 8);
            return TY_INT;
        }
        case N_LINX:
            // linx 声明本身不产生代码（阶段 3 接入真实桥接）
            m_nop(e);
            return TY_INT;
        default:
            return TY_INT;
    }
}

// ================================================================ 语句
static void gen_stmt(Codegen *C, Node *n) {
    EmitCtx *e = &C->em;
    if (!n) return;
    switch (n->kind) {
        case N_BLOCK:
            for (int i = 0; i < n->n; i++) gen_stmt(C, n->list[i]);
            break;
        case N_ASSIGN: {
            ValueType ty = gen_expr(C, n->a);
            int slot = cg_lookup(C, n->name, NULL);
            if (slot < 0) slot = cg_declare(C, n->name, ty);
            else cg_retype(C, n->name, ty);
            m_st(e, &S(slot), RAX);
            break;
        }
        case N_PRINT: {
            ValueType ty = gen_expr(C, n->a);
            m_mov_rr(e, RDI, RAX);
            if (ty == TY_STR || ty == TY_ARR) m_call(e, C->lab_rt_pstr);
            else m_call(e, C->lab_rt_pint);
            break;
        }
        case N_RETURN:
            if (n->a) gen_expr(C, n->a);
            else m_xor_rr(e, RAX, RAX);
            m_leave(e);
            m_ret(e);
            break;
        case N_IF: {
            int l_else = m_new_label(e), l_end = m_new_label(e);
            gen_expr(C, n->a);
            m_test_rr(e, RAX, RAX);
            m_jcc(e, CC_E, l_else);
            gen_stmt(C, n->b);
            m_jmp(e, l_end);
            m_bind(e, l_else);
            if (n->c) gen_stmt(C, n->c);
            m_bind(e, l_end);
            break;
        }
        case N_WHILE: {
            int l_top = m_new_label(e), l_end = m_new_label(e);
            m_bind(e, l_top);
            gen_expr(C, n->a);
            m_test_rr(e, RAX, RAX);
            m_jcc(e, CC_E, l_end);
            gen_stmt(C, n->b);
            m_jmp(e, l_top);
            m_bind(e, l_end);
            break;
        }
        case N_FUNC:
            break;      // 函数体单独发射
        case N_LINX:
            m_nop(e);
            break;
        case N_PARALLEL: {
            // 块内每条语句必须是对用户函数的调用：各起一个线程，块末全部 join
            int cnt = n->n;
            if (cnt <= 0) break;
            int qwords = ((cnt + 1) / 2) * 2;          // 偶数个 qword → 16 字节对齐
            m_sub_ri(e, RSP, 8 * qwords);
            int *handles = (int *)malloc(sizeof(int) * cnt);
            for (int i = 0; i < cnt; i++) {
                Node *s = n->list[i];
                if (!s || s->kind != N_CALL) {
                    fprintf(stderr, "编译错误 第%d行: Parallel 块里只能放函数调用\n", s ? s->line : n->line);
                    exit(1);
                }
                int idx = -1;
                for (int k = 0; k < C->nfuncs; k++)
                    if (!strcmp(C->funcs[k].name, s->name)) { idx = k; break; }
                if (idx < 0) {
                    fprintf(stderr, "编译错误 第%d行: Parallel 里调用了未定义函数 %s\n", s->line, s->name);
                    exit(1);
                }
                m_lea_lab(e, RAX, C->funcs[idx].lab);
                m_mov_rr(e, RDI, RAX);
                m_call(e, C->lab_rt_thread);
                m_st(e, &MRA(RSP, 8 * i), RAX);        // 保存线程句柄
            }
            for (int i = 0; i < cnt; i++) {
                m_ld(e, RDI, &MRA(RSP, 8 * i));
                m_call(e, C->lab_rt_join);
            }
            m_add_ri(e, RSP, 8 * qwords);
            free(handles);
            break;
        }
        default:
            gen_expr(C, n);
            break;
    }
}

// ================================================================ 函数发射
// 预扫描：统计函数体内会被赋值的变量名个数，用来把栈帧压到刚好够用。
// （原先固定 704 字节，递归/热点函数上白白浪费栈带宽和缓存）
static void prescan_vars(Node *n, char **names, int *cnt, int maxn) {
    if (!n) return;
    if (n->kind == N_ASSIGN && n->name) {
        int found = 0;
        for (int i = 0; i < *cnt; i++) if (!strcmp(names[i], n->name)) { found = 1; break; }
        if (!found && *cnt < maxn) names[(*cnt)++] = n->name;
    }
    prescan_vars(n->a, names, cnt, maxn);
    prescan_vars(n->b, names, cnt, maxn);
    prescan_vars(n->c, names, cnt, maxn);
    prescan_vars(n->d, names, cnt, maxn);
    for (int i = 0; i < n->n; i++) prescan_vars(n->list[i], names, cnt, maxn);
}

static void emit_function(Codegen *C, CFunc *f) {
    EmitCtx *e = &C->em;
    m_bind(e, f->lab);
    m_push(e, RBP);
    m_mov_rr(e, RBP, RSP);
    
    // ★ 方案 C：桩函数处理 ★
    // 如果是桩函数（没有定义），生成一个简单的返回 0 的实现
    if (f->is_stub) {
        m_sub_ri(e, RSP, 64);  // 最小栈帧
        m_xor_rr(e, RAX, RAX);  // 返回 0
        m_leave(e);
        m_ret(e);
        if (getenv("SCDBG")) fprintf(stderr, "[dbg] 桩函数发射: %s\n", f->name);
        return;
    }
    
    // 按需分配栈帧：8*变量数 + 32(shadow) + 16(临时)，向上对齐到 16
    {
        static char *nm[MAX_VARS];
        int nv = 0;
        prescan_vars(f->def->a, nm, &nv, MAX_VARS);
        int need = 8 * (nv + f->nparams) + 48;
        int frame = (need + 15) / 16 * 16;
        if (frame < 64) frame = 64;
        if (frame > FRAME_SIZE) frame = FRAME_SIZE;
        m_sub_ri(e, RSP, frame);
    }
    cg_scope_reset(C);

    // 参数入槽
    int regs[4] = { RCX, RDX, R8, R9 };
    for (int i = 0; i < f->nparams; i++) {
        int slot = cg_declare(C, f->def->names[i], TY_ANY);
        if (i < 4) {
            m_st(e, &S(slot), regs[i]);
        } else {
            // 第 5 个起的栈参数：caller 放在 [rsp+32+8*(i-4)]（call 前的 rsp），
            // callee 侧对应 [rbp+16+8*i]。
            m_ld(e, RAX, &MRA(RBP, 16 + 8 * i));
            m_st(e, &S(slot), RAX);
        }
    }
    // ★ 局部变量清零 ★
    //   槽位在栈上复用，不清零的话局部变量的初值是上一帧的残留 ——
    //   表现为「某个标志位莫名其妙是 1」，逻辑随机出错。
    //   语义上，局部变量应当默认为 0。
    //
    //   槽位分配规则：cg_declare 按「gen_stmt 遍历中首次遇到赋值的顺序」递增分配。
    //   prescan_vars 用同样的遍历顺序收集名字，两者一一对应，
    //   所以这里按 prescan 的顺序把 1..n 号槽位清零即可。
    {
        static char *nm[MAX_VARS];
        int nv = 0;
        prescan_vars(f->def->a, nm, &nv, MAX_VARS);
        for (int i = 0; i < nv; i++) {
            m_mov_ri32(e, RAX, 0);
            m_st(e, &S(i + 1 + f->nparams), RAX);   // 参数占 1..nparams，局部变量从后面开始
        }
    }
    gen_stmt(C, f->def->a);
    m_xor_rr(e, RAX, RAX);
    m_leave(e);
    m_ret(e);
}

#endif // STARC_CODE_H
