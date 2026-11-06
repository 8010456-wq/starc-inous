// ============================================================================
//  starcc —— StarC 语言编译器（自研，C 实现，零外部工具链）
//  ---------------------------------------------------------------------------
//  用法：
//      starcc <源文件.starc> [-o 输出.exe] [-S] [-v]
//
//  编译流水线：
//      .starc 源码
//        → 词法分析 (lex.h)
//        → 语法分析 (ast.h)  → AST
//        → 代码生成 (code.h) → x86-64 机器码
//        → PE 封装 (pe.h)    → .exe
//
//  特点：不调用 gcc / as / ld / link.exe。产物只依赖系统自带的 kernel32.dll。
// ============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "pe.h"
#include "emit.h"
#include "lex.h"
#include "ast.h"
#include "code.h"

static char *read_file(const char *path, long *out_len) {
    FILE *fp = fopen(path, "rb");
    if (!fp) { fprintf(stderr, "错误: 打不开源文件 %s\n", path); return NULL; }
    fseek(fp, 0, SEEK_END);
    long n = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)n + 4);
    size_t got = fread(buf, 1, (size_t)n, fp);
    buf[got] = 0;
    fclose(fp);
    if (out_len) *out_len = (long)got;
    // 去掉 UTF-8 BOM
    if (got >= 3 && (unsigned char)buf[0] == 0xEF && (unsigned char)buf[1] == 0xBB && (unsigned char)buf[2] == 0xBF)
        memmove(buf, buf + 3, got - 2);
    return buf;
}

static void usage(const char *prog) {
    printf("StarC 编译器 starcc v1.1 —— 自研 · 零外部工具链 · 直出 PE32+\n");
    printf("用法: %s <源文件1.starc> [源文件2.starc ...] [选项]\n", prog);
    printf("  可一次传多个 .starc，按命令行顺序合并成一个程序编译\n");
    printf("  -o <文件>   指定输出可执行文件（默认与第一个源文件同名 .exe）\n");
    printf("  -v          显示编译统计\n");
    printf("  -h          帮助\n");
}

// 把 "a/b/c.starc" → "a/b/c"；再拼 .exe
static void derive_exe_name(const char *src, char *out, size_t cap) {
    strncpy(out, src, cap - 1); out[cap - 1] = 0;
    char *dot = strrchr(out, '.');
    char *sep1 = strrchr(out, '/');
    char *sep2 = strrchr(out, '\\');
    char *sep = (sep1 > sep2) ? sep1 : sep2;
    if (dot && (!sep || dot > sep)) *dot = 0;
    strncat(out, ".exe", cap - strlen(out) - 1);
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(argv[0]); return 1; }

    const char *src_paths[32];
    int nsrc = 0;
    char exe_path[1024] = {0};
    int verbose = 0;
    int dll_mode = 0;                 // 1 = 产出 DLL（带导出表）
    char dll_name[64] = {0};

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o") && i + 1 < argc) { strncpy(exe_path, argv[++i], sizeof(exe_path) - 1); }
        else if (!strcmp(argv[i], "-v")) verbose = 1;
        else if (!strcmp(argv[i], "--dll")) dll_mode = 1;
        else if (!strcmp(argv[i], "--dll-name") && i + 1 < argc) { strncpy(dll_name, argv[++i], sizeof(dll_name) - 1); dll_mode = 1; }
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) { usage(argv[0]); return 0; }
        else if (argv[i][0] == '-') { fprintf(stderr, "未知选项 %s\n", argv[i]); return 1; }
        else if (nsrc < 32) src_paths[nsrc++] = argv[i];
    }
    if (nsrc == 0) { usage(argv[0]); return 1; }
    if (!exe_path[0]) derive_exe_name(src_paths[0], exe_path, sizeof(exe_path));
    if (dll_mode && !dll_name[0]) {
        // 默认 DLL 名 = 输出文件名（去扩展名）
        strncpy(dll_name, exe_path, sizeof(dll_name) - 1);
        char *dot = strrchr(dll_name, '.');
        if (dot) *dot = 0;
    }

    // ---------------- 前端 ----------------
    // 支持多文件：按命令行顺序解析，再把顶层节点顺序拼成一个程序。
    // 这样每个功能可以各占一个 .starc（老板的约束），不用手动拼源码。
    Node *prog = NULL;
    for (int f = 0; f < nsrc; f++) {
        long src_len = 0;
        char *src = read_file(src_paths[f], &src_len);
        if (!src) return 1;
        Parser P; memset(&P, 0, sizeof(P));
        lx_init(&P.L, src);
        Node *one = parse_program(&P);
        if (verbose) fprintf(stderr, "  解析 %s：%d 个顶层节点\n", src_paths[f], one->n);
        if (!prog) {
            prog = one;
        } else {
            for (int k = 0; k < one->n; k++) node_push(prog, one->list[k]);
        }
    }
    if (getenv("SCDBG")) fprintf(stderr, "[dbg] parse ok, top=%d\n", prog->n);

    // ---------------- 建 PE 骨架 ----------------
    // 注意：这两个结构里有上百 KB 的表（重定位/标签/导入），必须放静态区。
    // 放栈上会直接吃掉 1MB 默认栈，表现为「程序启动即 127，无任何输出」。
    static PEBuilder pe_storage;
    PEBuilder *pep = &pe_storage;
    pe_init(pep);
    Section *st = pe_section(pep, ".text",  SCN_RX);
    Section *rd = pe_section(pep, ".rdata", SCN_RDATA);
    Section *dt = pe_section(pep, ".data",  SCN_RW);
    int text_sec  = pe_section_index(pep, ".text");
    int rdata_sec = pe_section_index(pep, ".rdata");
    int data_sec  = pe_section_index(pep, ".data");

    ImportDll *k32 = pe_import(pep, "kernel32.dll");
    pe_import_add(k32, "GetStdHandle");
    pe_import_add(k32, "WriteFile");
    pe_import_add(k32, "SetConsoleOutputCP");
    pe_import_add(k32, "GetProcessHeap");
    pe_import_add(k32, "HeapAlloc");
    pe_import_add(k32, "ExitProcess");
    pe_import_add(k32, "CreateThread");
    pe_import_add(k32, "WaitForSingleObject");
    pe_import_add(k32, "QueryPerformanceCounter");
    pe_import_add(k32, "QueryPerformanceFrequency");

    static Codegen C;               // 放静态区：结构里有近百 KB 的表，别上栈
    memset(&C, 0, sizeof(C));
    C.pe = pep; C.text_sec = text_sec; C.rdata_sec = rdata_sec; C.data_sec = data_sec;
    em_init(&C.em, pep, text_sec);

    // ---- 收集 linx 声明：把外部库函数注册进 PE 导入表 ----
    // 写法：linx "user32.dll!MessageBoxA"  或  linx "user32:MessageBoxA"
    {
        // 注意：这里用一个小递归收集
        Node *stack[1024]; int sp = 0;
        stack[sp++] = prog;
        while (sp > 0) {
            Node *cur = stack[--sp];
            if (!cur) continue;
            if (cur->kind == N_LINX && cur->sval) {
                char spec[128]; strncpy(spec, cur->sval, sizeof(spec) - 1); spec[sizeof(spec)-1] = 0;
                char *sep = strpbrk(spec, "!:");
                if (sep) {
                    *sep = 0;
                    char *dllname = spec, *fname = sep + 1;
                    if (!strchr(dllname, '.')) { static char db[64]; snprintf(db, sizeof(db), "%s.dll", dllname); dllname = db; }
                    // 找/建 DLL 组
                    ImportDll *target = NULL;
                    for (int i = 0; i < pep->ndlls; i++)
                        if (!strcmp(pep->dlls[i].dll, dllname)) { target = &pep->dlls[i]; break; }
                    if (!target) target = pe_import(pep, sc_strdup(dllname));
                    pe_import_add(target, sc_strdup(fname));
                    // 记录到 codegen
                    if (C.nlinx < 128) {
                        strncpy(C.linx[C.nlinx].dll, dllname, 47);
                        strncpy(C.linx[C.nlinx].fn, fname, 63);
                        C.nlinx++;
                    }
                }
            }
            // 处理 linx-o 声明：linx-o <dll_name> <func1> <func2> ...
            if (cur->kind == N_LINXO && cur->sval) {
                char *dllname = cur->sval;
                char full_dll[64];
                if (!strchr(dllname, '.')) snprintf(full_dll, sizeof(full_dll), "%s.dll", dllname);
                else strcpy(full_dll, dllname);
                // 找/建 DLL 组
                ImportDll *target = NULL;
                for (int i = 0; i < pep->ndlls; i++)
                    if (!strcmp(pep->dlls[i].dll, full_dll)) { target = &pep->dlls[i]; break; }
                if (!target) target = pe_import(pep, sc_strdup(full_dll));
                // 导入所有函数名
                for (int i = 0; i < cur->n; i++) {
                    if (cur->list[i] && cur->list[i]->sval) {
                        pe_import_add(target, sc_strdup(cur->list[i]->sval));
                        if (C.nlinx < 128) {
                            strncpy(C.linx[C.nlinx].dll, full_dll, 47);
                            strncpy(C.linx[C.nlinx].fn, cur->list[i]->sval, 63);
                            C.nlinx++;
                        }
                    }
                }
            }
            // 注意：N_FUNC 的 n 是「参数个数」，list 可能为 NULL —— 必须先判 list
            if (cur->a && sp < 1023) stack[sp++] = cur->a;
            if (cur->b && sp < 1023) stack[sp++] = cur->b;
            if (cur->c && sp < 1023) stack[sp++] = cur->c;
            if (cur->d && sp < 1023) stack[sp++] = cur->d;
            if (cur->list)
                for (int i = 0; i < cur->n && sp < 1023; i++)
                    if (cur->list[i]) stack[sp++] = cur->list[i];
        }
    }

    // .data 全局槽
    C.off_stdout = (uint32_t)dt->body.len; buf_u64(&dt->body, 0);
    C.off_heap   = (uint32_t)dt->body.len; buf_u64(&dt->body, 0);

    if (getenv("SCDBG")) fprintf(stderr, "[dbg] prelude ok\n");
    // ---------------- 分配标签 ----------------
    int lab_main    = m_new_label(&C.em);
    C.lab_rt_init   = m_new_label(&C.em);
    C.lab_rt_write  = m_new_label(&C.em);
    C.lab_rt_pstr   = m_new_label(&C.em);
    C.lab_rt_pint   = m_new_label(&C.em);
    C.lab_rt_alloc  = m_new_label(&C.em);
    C.lab_rt_concat = m_new_label(&C.em);
    C.lab_rt_i2s    = m_new_label(&C.em);
    C.lab_rt_nl     = m_new_label(&C.em);
    C.lab_rt_thread = m_new_label(&C.em);
    C.lab_rt_join   = m_new_label(&C.em);
    C.lab_rt_now    = m_new_label(&C.em);
    C.lab_rt_freq   = m_new_label(&C.em);
    // 浮点运行时标签
    C.lab_rt_fadd   = m_new_label(&C.em);
    C.lab_rt_fsub   = m_new_label(&C.em);
    C.lab_rt_fmul   = m_new_label(&C.em);
    C.lab_rt_fdiv   = m_new_label(&C.em);
    C.lab_rt_fsqrt  = m_new_label(&C.em);
    C.lab_rt_fabs   = m_new_label(&C.em);
    C.lab_rt_fneg   = m_new_label(&C.em);
    C.lab_rt_fcmp   = m_new_label(&C.em);
    C.lab_rt_f2i    = m_new_label(&C.em);
    C.lab_rt_i2f    = m_new_label(&C.em);


    // 收集函数
    for (int i = 0; i < prog->n; i++) {
        Node *f = prog->list[i];
        if (!f || f->kind != N_FUNC) continue;
        if (C.nfuncs >= 4096) { fprintf(stderr, "错误: 函数超过 4096 个\n"); return 1; }
        C.funcs[C.nfuncs].name = f->name;
        C.funcs[C.nfuncs].lab = m_new_label(&C.em);
        C.funcs[C.nfuncs].nparams = f->n;
        C.funcs[C.nfuncs].def = f;
        C.nfuncs++;
    }

    if (getenv("SCDBG")) fprintf(stderr, "[dbg] funcs=%d linx=%d\n", C.nfuncs, C.nlinx);

    // ---------------- DLL：收集导出函数 ----------------
    // 源码里写 export "函数名" 即可把该函数导出。
    // 这里把导出名映射到对应函数的代码标签，供导出表回填真实 RVA。
    int export_lab[512];
    if (dll_mode) {
        pep->is_dll = 1;
        strncpy(pep->export_name, dll_name, sizeof(pep->export_name) - 1);
        for (int i = 0; i < prog->n; i++) {
            Node *s = prog->list[i];
            if (!s || s->kind != N_EXPORT) continue;
            if (pep->nexports >= 512) { fprintf(stderr, "错误: 导出函数超过 512 个\n"); return 1; }
            int found = -1;
            for (int k = 0; k < C.nfuncs; k++)
                if (!strcmp(C.funcs[k].name, s->name)) { found = k; break; }
            if (found < 0) {
                fprintf(stderr, "错误: export \"%s\" —— 找不到同名函数\n", s->name);
                return 1;
            }
            pep->exports[pep->nexports] = s->name;
            export_lab[pep->nexports] = C.funcs[found].lab;
            pep->nexports++;
        }
        if (pep->nexports == 0) {
            fprintf(stderr, "错误: --dll 模式但没有任何 export 声明\n");
            return 1;
        }
    }
    // ---------------- 发射入口 main ----------------
    m_bind(&C.em, lab_main);
    m_push(&C.em, RBP); m_mov_rr(&C.em, RBP, RSP); m_sub_ri(&C.em, RSP, FRAME_SIZE);
    m_call(&C.em, C.lab_rt_init);
    cg_scope_reset(&C);
    if (dll_mode) {
        // DLL 模式：顶层语句不执行（加载 DLL 时不该跑逻辑），
        // 入口返回 TRUE（=1）表示加载成功。
        // 真正的 DllMain 会在下面单独发射。
        m_mov_ri32(&C.em, RAX, 1);
        m_leave(&C.em); m_ret(&C.em);
    } else {
    // 顶层语句
    for (int i = 0; i < prog->n; i++) {
        Node *s = prog->list[i];
        if (!s || s->kind == N_FUNC || s->kind == N_EXPORT) continue;
        gen_stmt(&C, s);
    }
    m_xor_rr(&C.em, RAX, RAX);
    m_mov_rr(&C.em, RCX, RAX);
    m_call_ext(&C.em, "kernel32.dll!ExitProcess");
    m_int3(&C.em);
    }

    if (getenv("SCDBG")) fprintf(stderr, "[dbg] main emitted\n");
    // ---------------- 发射运行时 ----------------
    emit_runtime(&C);

    // ---------------- 发射用户函数 ----------------
    for (int i = 0; i < C.nfuncs; i++)
        emit_function(&C, &C.funcs[i]);

    // ---------------- DLL：发射 DllMain ----------------
    // DllMain(hinstDLL, fdwReason, lpvReserved) → BOOL
    // 签名必须是 stdcall 且返回值在 EAX。
    //
    // ★ 必须调用 rt_init ★
    //   rt_init 会把进程堆句柄写进 .data 段的全局变量。
    //   之前 DLL 模式只在 lab_main 里调 rt_init，而 DLL 的入口是 lab_dllmain，
    //   于是 .data 从未被初始化 —— rt_alloc 拿到 NULL 堆句柄直接崩。
    //   表现：LoadLibrary 成功，但一调用导出函数就段错误。
    int lab_dllmain = 0;
    if (dll_mode) {
        lab_dllmain = m_new_label(&C.em);
        m_bind(&C.em, lab_dllmain);
        m_push(&C.em, RBP); m_mov_rr(&C.em, RBP, RSP); m_sub_ri(&C.em, RSP, 96);
        // rdx = fdwReason（1 = DLL_PROCESS_ATTACH）
        m_mov_ri32(&C.em, RDX, 1);
        m_mov_ri32(&C.em, RCX, 0);
        m_mov_ri32(&C.em, R8, 0);
        m_call(&C.em, C.lab_rt_init);
        m_mov_ri32(&C.em, RAX, 1);            // 返回 TRUE
        m_leave(&C.em); m_ret(&C.em);
    }

    if (getenv("SCDBG")) fprintf(stderr, "[dbg] runtime+funcs emitted\n");
    em_finish(&C.em);
    if (getenv("SCDBG")) fprintf(stderr, "[dbg] em_finish ok\n");

    // ---------------- 写 PE ----------------
    C.pe->entry_rva = 0x1000 + C.em.lab_off[dll_mode ? lab_dllmain : lab_main];
    // 把导出函数的「段内偏移」交给 PE 层，导出表用真实 RVA
    // 注意：标签 ID 要先经 lab_off 换成 .text 段内的字节偏移
    if (dll_mode) {
        for (int i = 0; i < pep->nexports; i++)
            pep->export_lab_offs[i] = C.em.lab_off[export_lab[i]];
    }
    if (!pe_write(pep, exe_path)) {
        fprintf(stderr, "错误: 生成 %s 失败\n", exe_path);
        return 1;
    }

    if (verbose) {
        printf("源文件      : %d 个\n", nsrc);
        for (int i = 0; i < nsrc; i++) printf("  - %s\n", src_paths[i]);
        printf("函数        : %d 个\n", C.nfuncs);
        printf("字符串常量  : %d 个\n", C.nstrs);
        printf(".text       : %zu 字节机器码\n", st->body.len);
        printf(".rdata      : %zu 字节\n", rd->body.len);
        printf(".data       : %zu 字节\n", dt->body.len);
        printf("重定位      : %d 条 IRP / %d 条 PE\n", C.em.nripfix, pep->nfixes);
        if (dll_mode) {
            printf("产物类型    : DLL\n");
            printf("导出函数    : %d 个\n", pep->nexports);
            for (int i = 0; i < pep->nexports; i++) printf("  + %s\n", pep->exports[i]);
        }
    }
    printf("编译成功 -> %s\n", exe_path);
    return 0;
}
