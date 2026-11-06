// ============================================================================
//  StarC 编译器 · x86-64 机器码编码器 (emit.h)
//  ---------------------------------------------------------------------------
//  作用：直接发 x86-64 机器码字节，不生成汇编文本、不调用 as/ld/gcc。
//        这一层是「零依赖」的第二个支点：编译器和它的运行时全部由它拼出来。
//
//  约定：
//    - 一律 64 位操作（REX.W），整数是 int64。
//    - 内存操作数统一走 [base + disp32]，需要 SIB 时自动补。
//    - 跨段引用（.rdata 字符串、IAT 导入）先登记，等 PE 布局定了再回填。
// ============================================================================
#ifndef STARC_EMIT_H
#define STARC_EMIT_H

#include "pe.h"

// ---------------------------------------------------------------- 寄存器
enum {
    RAX=0, RCX=1, RDX=2, RBX=3, RSP=4, RBP=5, RSI=6, RDI=7,
    R8=8, R9=9, R10=10, R11=11, R12=12, R13=13, R14=14, R15=15
};

// 条件码
enum { CC_E=4, CC_NE=5, CC_L=12, CC_GE=13, CC_LE=14, CC_G=15, CC_B=2, CC_AE=3, CC_A=7, CC_BE=6 };

// 内存操作数
typedef struct {
    int      base;        // 基址寄存器；-1 表示 [rip+disp]
    int32_t  disp;
    // 跨段符号（base==-1 且需要 PE 回填时使用）
    int      to_sec;      // >=0：目标段索引
    uint32_t to_off;      // 目标段内偏移
    char     sym[96];     // to_sec<0 且非空：导入符号 "dll!Func"
    int      need_fix;
} Mem;

typedef struct { uint32_t pos; int lab; } JmpFix;
typedef struct { uint32_t pos; int to_sec; uint32_t to_off; char sym[96]; } RipFix;

// 编码上下文：负责记住当前在哪个段、以及按标签回填
typedef struct {
    PEBuilder *pe;
    Buf       *cur;        // 当前输出段
    int        cur_sec;    // 当前段索引
    // 标签表
    uint32_t   lab_off[8192];
    int        lab_defined[8192];
    int        nlabels;
    JmpFix     jmpfix[16384];
    int        njmpfix;
    JmpFix     lealab[4096];      // lea r64, [rip+段内标签]
    int        nlealab;
    RipFix     ripfix[8192];
    int        nripfix;
} EmitCtx;

static void em_init(EmitCtx *e, PEBuilder *pe, int sec_index) {
    e->pe = pe;
    e->cur_sec = sec_index;
    e->cur = &pe->secs[sec_index].body;
    e->nlabels = 0;
    e->njmpfix = 0;
    e->nripfix = 0;
    e->nlealab = 0;
    memset(e->lab_defined, 0, sizeof(e->lab_defined));
}

// ---------------------------------------------------------------- 基础
static void em_b(EmitCtx *e, uint8_t v) { buf_u8(e->cur, v); }
static void em_d(EmitCtx *e, uint32_t v) { buf_u32(e->cur, v); }
static uint32_t em_here(EmitCtx *e) { return (uint32_t)e->cur->len; }

// REX 前缀：op64 一般 true
static void em_rex(EmitCtx *e, int w, int reg, int index, int base) {
    uint8_t r = 0x40;
    if (w) r |= 8;
    if (reg & 8) r |= 4;
    if (index & 8) r |= 2;
    if (base & 8) r |= 1;
    if (r != 0x40) em_b(e, r);
}

// modrm + sib + disp
static void em_modrm_mem(EmitCtx *e, int reg, Mem *m) {
    if (m->base < 0) {
        // [rip + disp32]
        em_b(e, (uint8_t)(0x00 | ((reg & 7) << 3) | 0x05));
        if (m->need_fix) {
            // 登记回填：disp 相对「下一条指令」
            RipFix *f = &e->ripfix[e->nripfix++];
            f->pos = em_here(e);
            f->to_sec = m->to_sec; f->to_off = m->to_off;
            memcpy(f->sym, m->sym, sizeof(f->sym));
        }
        em_d(e, (uint32_t)m->disp);
        return;
    }
    int mod = (m->disp == 0) ? 0x00 : 0x02;      // 统一用 disp32，省事且永远正确
    mod = 0x02;
    int rm = m->base & 7;
    em_b(e, (uint8_t)((mod << 6) | ((reg & 7) << 3) | (rm == 4 ? 4 : rm)));
    if (rm == 4) em_b(e, 0x24);                  // SIB：base=rsp/r12
    em_d(e, (uint32_t)m->disp);
}

// ---------------------------------------------------------------- 指令
static void m_push(EmitCtx *e, int r) { em_rex(e,0,0,0,r); em_b(e,(uint8_t)(0x50+(r&7))); }
static void m_pop (EmitCtx *e, int r) { em_rex(e,0,0,0,r); em_b(e,(uint8_t)(0x58+(r&7))); }
static void m_ret (EmitCtx *e) { em_b(e,0xC3); }
static void m_nop (EmitCtx *e) { em_b(e,0x90); }
static void m_cqo (EmitCtx *e) { em_b(e,0x48); em_b(e,0x99); }
static void m_int3(EmitCtx *e) { em_b(e,0xCC); }
static void m_leave(EmitCtx *e){ em_b(e,0xC9); }

// mov r64, r64
static void m_mov_rr(EmitCtx *e, int dst, int src) {
    em_rex(e,1,src,0,dst); em_b(e,0x89);
    em_b(e,(uint8_t)(0xC0 | ((src&7)<<3) | (dst&7)));
}
static void m_mov_ri(EmitCtx *e, int r, int64_t v) {
    // MOVABS r64, imm64：REX.W + B8 + imm64（共 10 字节）
    // 注意：这是「绝对值」语义，不做符号扩展 —— 对 0xFFFFFFFF80000000 这类
    // 「64 位负数」是唯一正确的表达方式（用 B8+imm32 的符号扩展形式会得到 0x80000000）。
    em_rex(e,1,0,0,r); em_b(e,(uint8_t)(0xB8+(r&7)));
    buf_u64(e->cur,(uint64_t)v);
}

// mov r64, imm32（符号扩展，短）
static void m_mov_ri32(EmitCtx *e, int r, int32_t v) {
    em_rex(e,1,0,0,r); em_b(e,0xC7);
    em_b(e,(uint8_t)(0xC0|(r&7)));
    em_d(e,(uint32_t)v);
}
// mov r64, [mem]
static void m_ld(EmitCtx *e, int dst, Mem *m) {
    em_rex(e,1,dst,0, m->base<0?0:m->base);
    em_b(e,0x8B);
    em_modrm_mem(e,dst,m);
}
// mov [mem], r64
static void m_st(EmitCtx *e, Mem *m, int src) {
    em_rex(e,1,src,0, m->base<0?0:m->base);
    em_b(e,0x89);
    em_modrm_mem(e,src,m);
}
// mov qword [mem], imm32
static void m_st_i32(EmitCtx *e, Mem *m, int32_t v) {
    em_rex(e,1,0,0, m->base<0?0:m->base);
    em_b(e,0xC7);
    em_modrm_mem(e,0,m);
    em_d(e,(uint32_t)v);
}
// lea r64, [mem]
static void m_lea(EmitCtx *e, int dst, Mem *m) {
    em_rex(e,1,dst,0, m->base<0?0:m->base);
    em_b(e,0x8D);
    em_modrm_mem(e,dst,m);
}
// mov byte [mem], imm8
static void m_st_i8(EmitCtx *e, Mem *m, int8_t v) {
    // 正确编码：C6 /0 ib = mov byte ptr [m], imm8
    // （原先写成 0x88，那是 mov r/m8, r8 —— 会把 AL 的低字节写进去，
    //   而且少发一个立即数字节，导致后面整条指令流错位、程序直接崩）
    em_rex(e,0,0,0, m->base<0?0:m->base);
    em_b(e,0xC6);
    em_modrm_mem(e,0,m);
    em_b(e,(uint8_t)v);
}
// rep movsb  （rcx 计数，rsi 源，rdi 目的）
static void m_rep_movsb(EmitCtx *e) { em_b(e,0xF3); em_b(e,0xA4); }

// 算术：add/sub/and/or/xor/cmp
static void m_alu_rr(EmitCtx *e, uint8_t op, int dst, int src) {
    em_rex(e,1,src,0,dst); em_b(e,op);
    em_b(e,(uint8_t)(0xC0|((src&7)<<3)|(dst&7)));
}
static void m_add_rr(EmitCtx *e,int d,int s){ m_alu_rr(e,0x01,d,s); }
static void m_sub_rr(EmitCtx *e,int d,int s){ m_alu_rr(e,0x29,d,s); }
static void m_and_rr(EmitCtx *e,int d,int s){ m_alu_rr(e,0x21,d,s); }
static void m_or_rr (EmitCtx *e,int d,int s){ m_alu_rr(e,0x09,d,s); }
static void m_xor_rr(EmitCtx *e,int d,int s){ m_alu_rr(e,0x31,d,s); }
static void m_cmp_rr(EmitCtx *e,int d,int s){ m_alu_rr(e,0x39,d,s); }

// add/sub/cmp r64, imm32
static void m_alu_ri(EmitCtx *e, uint8_t ext, int r, int32_t v) {
    em_rex(e,1,0,0,r); em_b(e,0x81);
    em_b(e,(uint8_t)(0xC0|((ext&7)<<3)|(r&7)));
    em_d(e,(uint32_t)v);
}
static void m_add_ri(EmitCtx *e,int r,int32_t v){ m_alu_ri(e,0,r,v); }
static void m_sub_ri(EmitCtx *e,int r,int32_t v){ m_alu_ri(e,5,r,v); }
static void m_cmp_ri(EmitCtx *e,int r,int32_t v){ m_alu_ri(e,7,r,v); }

// add/sub/cmp r64, r/m64  （第二操作数直接在内存里，省掉一次 load）
static void m_alu_rm(EmitCtx *e, uint8_t op, int dst, Mem *m) {
    em_rex(e,1,dst,0, m->base<0?0:m->base); em_b(e,op);
    em_modrm_mem(e,dst,m);
}
static void m_add_rm(EmitCtx *e,int d,Mem*m){ m_alu_rm(e,0x03,d,m); }
static void m_sub_rm(EmitCtx *e,int d,Mem*m){ m_alu_rm(e,0x2B,d,m); }
static void m_cmp_rm(EmitCtx *e,int d,Mem*m){ m_alu_rm(e,0x3B,d,m); }
static void m_imul_rm(EmitCtx *e,int d,Mem*m){
    em_rex(e,1,d,0, m->base<0?0:m->base); em_b(e,0x0F); em_b(e,0xAF);
    em_modrm_mem(e,d,m);
}
// add/sub/cmp/and/or/xor r64, imm32  （短形式）
static void m_add_ri32(EmitCtx *e,int r,int32_t v){ m_alu_ri(e,0,r,v); }
static void m_cmp_ri32(EmitCtx *e,int r,int32_t v){ m_alu_ri(e,7,r,v); }

// reg = reg * 常量，用 LEA 实现（3/5/9 一步搞定，比 imul 快）
static void m_mul_lea(EmitCtx *e, int reg, int factor) {
    int sc;
    if (factor == 3) sc = 1;
    else if (factor == 5) sc = 2;
    else if (factor == 9) sc = 3;
    else return;
    em_rex(e, 1, reg, reg, reg);
    em_b(e, 0x8D);
    em_b(e, (uint8_t)(0x00 | ((reg & 7) << 3) | 0x04));   // mod=00, rm=100 (SIB)
    em_b(e, (uint8_t)((sc << 6) | ((reg & 7) << 3) | (reg & 7)));
}
// reg = reg * 2^k，用 SHL
static void m_shl_imm(EmitCtx *e, int reg, int k) {
    em_rex(e, 1, 0, 0, reg);
    em_b(e, 0xC1); em_b(e, (uint8_t)(0xE0 | (reg & 7))); em_b(e, (uint8_t)k);
}

static void m_imul_rr(EmitCtx *e,int d,int s){ em_rex(e,1,d,0,s); em_b(e,0x0F); em_b(e,0xAF); em_b(e,(uint8_t)(0xC0|((d&7)<<3)|(s&7))); }
static void m_idiv(EmitCtx *e,int r)         { em_rex(e,1,0,0,r); em_b(e,0xF7); em_b(e,(uint8_t)(0xF8|(r&7))); }
static void m_neg (EmitCtx *e,int r)         { em_rex(e,1,0,0,r); em_b(e,0xF7); em_b(e,(uint8_t)(0xD8|(r&7))); }
static void m_not (EmitCtx *e,int r)         { em_rex(e,1,0,0,r); em_b(e,0xF7); em_b(e,(uint8_t)(0xD0|(r&7))); }
static void m_test_rr(EmitCtx *e,int d,int s){ em_rex(e,1,s,0,d); em_b(e,0x85); em_b(e,(uint8_t)(0xC0|((s&7)<<3)|(d&7))); }
static void m_shl_cl(EmitCtx *e,int r)       { em_rex(e,1,0,0,r); em_b(e,0xD3); em_b(e,(uint8_t)(0xE0|(r&7))); }
static void m_sar_cl(EmitCtx *e,int r)       { em_rex(e,1,0,0,r); em_b(e,0xD3); em_b(e,(uint8_t)(0xF8|(r&7))); }

// movzx r64, r/m8
static void m_movzx8(EmitCtx *e,int dst,int src) {
    em_rex(e,1,dst,0,src);
    em_b(e,0x0F); em_b(e,0xB6);
    em_b(e,(uint8_t)(0xC0|((dst&7)<<3)|(src&7)));
}
// movzx r64, byte [mem]   —— 字节级读取（URL 编码、base64、二进制解析的基础）
static void m_ld_u8(EmitCtx *e,int dst,Mem *m) {
    em_rex(e,1,dst,0, m->base<0?0:m->base);
    em_b(e,0x0F); em_b(e,0xB6);
    em_modrm_mem(e,dst,m);
}
// setcc r8
static void m_setcc(EmitCtx *e,int cc,int r) {
    em_rex(e,0,0,0,r);
    em_b(e,0x0F); em_b(e,(uint8_t)(0x90|cc));
    em_b(e,(uint8_t)(0xC0|(r&7)));
}

// ---------------------------------------------------------------- 跳转与标签
static int  m_new_label(EmitCtx *e) { return e->nlabels++; }
static void m_bind(EmitCtx *e, int lab) {
    e->lab_off[lab] = em_here(e);
    e->lab_defined[lab] = 1;
}
static void m_jmp(EmitCtx *e, int lab) {
    em_b(e,0xE9);
    JmpFix *f = &e->jmpfix[e->njmpfix++];
    f->pos = em_here(e); f->lab = lab;
    em_d(e,0);
}
static void m_jcc(EmitCtx *e, int cc, int lab) {
    em_b(e,0x0F); em_b(e,(uint8_t)(0x80|cc));
    JmpFix *f = &e->jmpfix[e->njmpfix++];
    f->pos = em_here(e); f->lab = lab;
    em_d(e,0);
}
// call 内部标签
static void m_call(EmitCtx *e, int lab) {
    em_b(e,0xE8);
    JmpFix *f = &e->jmpfix[e->njmpfix++];
    f->pos = em_here(e); f->lab = lab;
    em_d(e,0);
}
// call [rip + 导入符号 IAT]  /  call [rip + 段内偏移]
static void m_call_ext(EmitCtx *e, const char *sym) {
    em_b(e,0xFF); em_b(e,0x15);
    RipFix *f = &e->ripfix[e->nripfix++];
    f->pos = em_here(e);
    f->to_sec = -1; f->to_off = 0;
    strncpy(f->sym, sym, 95); f->sym[95] = 0;
    em_d(e,0);
}

// lea r64, [rip+本段标签]  —— 用来取函数首地址（并发起线程要用）
static void m_lea_lab(EmitCtx *e, int reg, int lab) {
    em_rex(e, 1, reg, 0, 0);
    em_b(e, 0x8D);
    em_b(e, (uint8_t)(0x00 | ((reg & 7) << 3) | 0x05));
    JmpFix *f = &e->lealab[e->nlealab++];
    f->pos = em_here(e); f->lab = lab;
    em_d(e, 0);
}

// ---------------------------------------------------------------- SSE2 浮点
// 为什么需要：x64 的调用约定里，返回 double 的函数把结果放在 XMM0，
// 而整数返回在 RAX。starc 的表达式求值器只认 RAX，
// 所以「atof / sqrt / sin ...」这类调用拿到的会是垃圾值。
//
// 解决办法：把 double 的 IEEE754 位模式当作普通 int64 在寄存器里流转，
// 需要真正做浮点运算时由运行时辅助函数用 SSE 处理。
//
// 指令编码：
//   movq xmm, r64      66 REX.W 0F 6E /r      （把 64 位整数搬进 xmm 的低 64 位）
//   movq r64, xmm      66 REX.W 0F 7E /r      （把 xmm 的低 64 位搬回整数寄存器）
//   addsd xmm, xmm     F2 0F 58 /r
//   subsd xmm, xmm     F2 0F 5C /r
//   mulsd xmm, xmm     F2 0F 59 /r
//   divsd xmm, xmm     F2 0F 5E /r
//   sqrtsd xmm, xmm    F2 0F 51 /r
//   cvtsi2sd xmm, r64  F2 REX.W 0F 2A /r
//   cvttsd2si r64, xmm F2 REX.W 0F 2C /r
//   ucomisd xmm, xmm   66 0F 2E /r
//   movsd xmm, [mem]   F2 0F 10 /r
//   movsd [mem], xmm   F2 0F 11 /r

// movq xmm, r64
static void m_movq_xmm_rr(EmitCtx *e, int xmm, int src) {
    em_b(e, 0x66);
    em_rex(e, 1, xmm, 0, src);
    em_b(e, 0x0F); em_b(e, 0x6E);
    em_b(e, (uint8_t)(0xC0 | ((xmm & 7) << 3) | (src & 7)));
}
// movq r64, xmm
// 编码：66 REX.W 0F 7E /r
//   这条指令是「MOVQ r/m64, xmm」—— ModRM 的 reg 字段放 xmm，rm 字段放整数寄存器。
//   （注意：0F 6E 是 xmm,r/m64；0F 7E 是 r/m64,xmm —— reg/rm 的角色会互换，
//     写反了会变成把整数寄存器当 xmm 用，编译能过但结果是垃圾值。）
static void m_movq_rr_xmm(EmitCtx *e, int dst, int xmm) {
    em_b(e, 0x66);
    em_rex(e, 1, xmm, 0, dst);
    em_b(e, 0x0F); em_b(e, 0x7E);
    em_b(e, (uint8_t)(0xC0 | ((xmm & 7) << 3) | (dst & 7)));
}
// 通用：opcode 形式的 SSE 标量双精度运算（addsd/subsd/mulsd/divsd/sqrtsd）
static void m_sse_sd_op(EmitCtx *e, uint8_t op, int dst, int src) {
    em_b(e, 0xF2);
    em_rex(e, 1, dst, 0, src);
    em_b(e, 0x0F); em_b(e, op);
    em_b(e, (uint8_t)(0xC0 | ((dst & 7) << 3) | (src & 7)));
}
static void m_addsd(EmitCtx *e, int d, int s){ m_sse_sd_op(e, 0x58, d, s); }
static void m_subsd(EmitCtx *e, int d, int s){ m_sse_sd_op(e, 0x5C, d, s); }
static void m_mulsd(EmitCtx *e, int d, int s){ m_sse_sd_op(e, 0x59, d, s); }
static void m_divsd(EmitCtx *e, int d, int s){ m_sse_sd_op(e, 0x5E, d, s); }
static void m_sqrtsd(EmitCtx *e, int d, int s){ m_sse_sd_op(e, 0x51, d, s); }
// cvtsi2sd xmm, r64
static void m_cvtsi2sd(EmitCtx *e, int xmm, int src) {
    em_b(e, 0xF2);
    em_rex(e, 1, xmm, 0, src);
    em_b(e, 0x0F); em_b(e, 0x2A);
    em_b(e, (uint8_t)(0xC0 | ((xmm & 7) << 3) | (src & 7)));
}
// cvttsd2si r64, xmm
static void m_cvttsd2si(EmitCtx *e, int dst, int xmm) {
    em_b(e, 0xF2);
    em_rex(e, 1, dst, 0, xmm);
    em_b(e, 0x0F); em_b(e, 0x2C);
    em_b(e, (uint8_t)(0xC0 | ((dst & 7) << 3) | (xmm & 7)));
}
// ucomisd xmm, xmm  → ZF/PF/CF 反映大小关系
static void m_ucomisd(EmitCtx *e, int dst, int src) {
    em_b(e, 0x66);
    em_rex(e, 1, dst, 0, src);
    em_b(e, 0x0F); em_b(e, 0x2E);
    em_b(e, (uint8_t)(0xC0 | ((dst & 7) << 3) | (src & 7)));
}
// movsd xmm, [mem]
static void m_movsd_xmm_mem(EmitCtx *e, int xmm, Mem *m) {
    em_b(e, 0xF2);
    em_rex(e, 1, xmm, 0, 0);
    em_b(e, 0x0F); em_b(e, 0x10);
    em_modrm_mem(e, xmm, m);
}
// movsd [mem], xmm
static void m_movsd_mem_xmm(EmitCtx *e, Mem *m, int xmm) {
    em_b(e, 0xF2);
    em_rex(e, 1, xmm, 0, 0);
    em_b(e, 0x0F); em_b(e, 0x11);
    em_modrm_mem(e, xmm, m);
}

// 收尾：把标签位移回填进跳转指令
static void em_finish(EmitCtx *e) {
    for (int i = 0; i < e->njmpfix; i++) {
        int lab = e->jmpfix[i].lab;
        if (!e->lab_defined[lab]) {
            fprintf(stderr, "emit: undefined label L%d\n", lab);
            exit(1);
        }
        // 相对位移 = 目标 - (该 disp 字段之后的下一条指令)
        int64_t here = (int64_t)e->jmpfix[i].pos + 4;
        int64_t disp = (int64_t)e->lab_off[lab] - here;
        buf_patch_u32(e->cur, e->jmpfix[i].pos, (uint32_t)(int32_t)disp);
    }
    // lea 取标签地址（同段内相对位移）
    for (int i = 0; i < e->nlealab; i++) {
        int lab = e->lealab[i].lab;
        if (!e->lab_defined[lab]) { fprintf(stderr, "emit: 未定义标签 L%d (lea)\n", lab); exit(1); }
        int64_t here = (int64_t)e->lealab[i].pos + 4;
        int64_t disp = (int64_t)e->lab_off[lab] - here;
        buf_patch_u32(e->cur, e->lealab[i].pos, (uint32_t)(int32_t)disp);
    }
    // RIP 引用的跨段回填：需要 PE 层的段 RVA，交给 pe_write
    for (int i = 0; i < e->nripfix; i++) {
        pe_fix_sec_or_sym(e->pe, e->cur_sec, e->ripfix[i].pos,
                          e->ripfix[i].to_sec, e->ripfix[i].to_off, e->ripfix[i].sym);
    }
}

#endif // STARC_EMIT_H
