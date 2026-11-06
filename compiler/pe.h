// ============================================================================
//  StarC 编译器 · PE32+ 可执行文件生成器
//  ---------------------------------------------------------------------------
//  作用：把「已生成的 x86-64 机器码 + 数据 + 导入表」直接拼成一个 Windows
//        PE32+ (.exe) 文件。不调用 gcc / link.exe / 任何外部工具。
//
//  设计要点（决定了「零依赖」成立）：
//    1. 固定 ImageBase = 0x140000000，关闭 DYNAMIC_BASE → 不需要 .reloc 段，
//       所有代码一律 RIP-relative 寻址，绝对地址只出现在导入表（由 loader 填）。
//    2. 段布局：.text / .rdata / .data / .idata，SectionAlignment=0x1000，
//       FileAlignment=0x200。
//    3. 导入表只导 kernel32 的少量 API，运行时全部由 starcc 自己生成机器码实现。
// ============================================================================
#ifndef STARC_PE_C
#define STARC_PE_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ---------------------------------------------------------------- 字节缓冲
typedef struct {
    uint8_t *p;
    size_t   len;
    size_t   cap;
} Buf;

static void buf_init(Buf *b, size_t cap) {
    b->p = (uint8_t *)calloc(cap ? cap : 256, 1);
    b->len = 0;
    b->cap = cap ? cap : 256;
}
static void buf_need(Buf *b, size_t n) {
    if (b->len + n <= b->cap) return;
    size_t nc = b->cap * 2;
    while (nc < b->len + n) nc *= 2;
    b->p = (uint8_t *)realloc(b->p, nc);
    memset(b->p + b->cap, 0, nc - b->cap);
    b->cap = nc;
}
static void buf_u8(Buf *b, uint8_t v)      { buf_need(b,1); b->p[b->len++] = v; }
static void buf_u16(Buf *b, uint16_t v)    { buf_need(b,2); memcpy(b->p+b->len,&v,2); b->len+=2; }
static void buf_u32(Buf *b, uint32_t v)    { buf_need(b,4); memcpy(b->p+b->len,&v,4); b->len+=4; }
static void buf_u64(Buf *b, uint64_t v)    { buf_need(b,8); memcpy(b->p+b->len,&v,8); b->len+=8; }
static void buf_bytes(Buf *b, const void *d, size_t n) { buf_need(b,n); memcpy(b->p+b->len,d,n); b->len+=n; }
static void buf_zeros(Buf *b, size_t n)    { buf_need(b,n); memset(b->p+b->len,0,n); b->len+=n; }
// 在指定位置回填（不改变长度）
static void buf_patch_u32(Buf *b, size_t off, uint32_t v) { memcpy(b->p+off,&v,4); }
static void buf_patch_u64(Buf *b, size_t off, uint64_t v) { memcpy(b->p+off,&v,8); }
static void buf_align(Buf *b, size_t a)    { while (b->len % a) buf_u8(b, 0); }
static void buf_free(Buf *b) { free(b->p); b->p=NULL; b->len=b->cap=0; }

// ---------------------------------------------------------------- PE 常量
#define IMAGE_BASE          0x140000000ULL
#define SECTION_ALIGN       0x1000
#define FILE_ALIGN          0x200
#define PE_MACHINE_AMD64    0x8664
#define SCN_CODE            0x00000020u                 // CNT_CODE
#define SCN_IDATA           0x00000040u                 // CNT_INITIALIZED_DATA
#define SCN_RDATA           0x40000000u                 // MEM_READ
#define SCN_DATA            (0x00000040u|0x80000000u)    // CNT_INITIALIZED_DATA|MEM_WRITE
#define SCN_RW              (0x00000040u|0xC0000000u)    // READ|WRITE
#define SCN_RX              (0x00000020u|0x60000000u)    // CODE|EXECUTE|READ

// 段描述
typedef struct {
    char     name[8];
    Buf      body;        // 段内容
    uint32_t rva;         // 装载后 RVA
    uint32_t raw_off;     // 文件偏移
    uint32_t raw_size;    // 文件大小（对齐后）
    uint32_t vsize;       // 虚拟大小（未对齐）
    uint32_t chars;       // 段属性
} Section;

#define MAX_SECTIONS 8
// 标准库 v1.0.0：单模块会导入几十个 Win32 API（kernel32 + msvcrt + ws2_32 ...），
// 64 太紧。提到 256，配合 pe_import_add 的去重。
#define MAX_IMPORTS  256

typedef struct {
    const char *dll;                    // 如 "kernel32.dll"
    const char *funcs[MAX_IMPORTS];     // 函数名列表
    int         nfuncs;
    uint32_t    ilt_rva;                // Import Lookup Table RVA
    uint32_t    iat_rva;                // Import Address Table RVA
    uint32_t    name_rva;               // DLL 名字 RVA
} ImportDll;

// ---------------------------------------------------------------- 延迟重定位
// 代码生成时还不知道各段最终 RVA（段大小会变），所以所有 RIP-relative 的
// disp32 先写 0 并登记到 fix 表，等布局算完再统一回填。
#define MAX_FIXES 8192
typedef struct {
    int      from_sec;      // disp32 所在的段
    uint32_t from_off;      // disp32 在段内的字节偏移
    int      to_sec;        // 目标段；-1 表示导入符号
    uint32_t to_off;        // 目标段内偏移（to_sec>=0 时有效）
    char     sym[96];       // to_sec<0 时用 "dll!Func"
} PEFix;

// ---------------------------------------------------------------- 构建器
typedef struct {
    Section   secs[MAX_SECTIONS];
    int       nsecs;
    ImportDll dlls[8];
    int       ndlls;
    uint32_t  entry_rva;                // 入口点 RVA
    PEFix     fixes[MAX_FIXES];
    int       nfixes;
    // ---- DLL 导出表 ----
    int       is_dll;                   // 1 = 产出 DLL（带导出表）
    char      export_name[64];          // DLL 主导出名（形如 "starc_std"）
    char     *exports[512];             // 导出函数名列表
    int       nexports;
    uint32_t  export_lab_offs[512];   // 导出函数在 .text 段内的偏移（回填 RVA 用）
    uint16_t  export_ord_map[512];  // 排序后：名字下标 → 函数地址表下标
    uint32_t  export_dir_rva;           // 导出目录 RVA（回填用）
    uint32_t  export_addr_table_rva;    // 函数地址表 RVA
    uint32_t  export_name_table_rva;    // 名字指针表 RVA
    uint32_t  export_ord_table_rva;     // 名字序数表 RVA
} PEBuilder;

static void pe_init(PEBuilder *pe) {
    memset(pe, 0, sizeof(*pe));
}

// 登记一条待回填的 RIP-relative disp32
static void pe_fix_sec(PEBuilder *pe, int from_sec, uint32_t from_off, int to_sec, uint32_t to_off) {
    if (pe->nfixes >= MAX_FIXES) { fprintf(stderr, "PE: fix table overflow\n"); exit(1); }
    PEFix *f = &pe->fixes[pe->nfixes++];
    f->from_sec = from_sec; f->from_off = from_off;
    f->to_sec = to_sec; f->to_off = to_off; f->sym[0] = 0;
}
static void pe_fix_sym(PEBuilder *pe, int from_sec, uint32_t from_off, const char *sym) {
    if (pe->nfixes >= MAX_FIXES) { fprintf(stderr, "PE: fix table overflow\n"); exit(1); }
    PEFix *f = &pe->fixes[pe->nfixes++];
    f->from_sec = from_sec; f->from_off = from_off;
    f->to_sec = -1; f->to_off = 0;
    strncpy(f->sym, sym, sizeof(f->sym) - 1);
}
// to_sec >= 0 走段内偏移，否则按导入符号处理（sym 为空时忽略）
static void pe_fix_sec_or_sym(PEBuilder *pe, int from_sec, uint32_t from_off,
                              int to_sec, uint32_t to_off, const char *sym) {
    if (to_sec >= 0) pe_fix_sec(pe, from_sec, from_off, to_sec, to_off);
    else pe_fix_sym(pe, from_sec, from_off, sym);
}

static Section *pe_section(PEBuilder *pe, const char *name, uint32_t chars) {
    if (pe->nsecs >= MAX_SECTIONS) { fprintf(stderr,"PE: too many sections\n"); exit(1); }
    Section *s = &pe->secs[pe->nsecs++];
    memset(s, 0, sizeof(*s));
    strncpy(s->name, name, 7);
    s->chars   = chars;
    buf_init(&s->body, 65536);
    return s;
}

static Section *pe_find_section(PEBuilder *pe, const char *name) {
    for (int i = 0; i < pe->nsecs; i++)
        if (strcmp(pe->secs[i].name, name) == 0) return &pe->secs[i];
    return NULL;
}
static int pe_section_index(PEBuilder *pe, const char *name) {
    for (int i = 0; i < pe->nsecs; i++)
        if (strcmp(pe->secs[i].name, name) == 0) return i;
    return -1;
}

// 加入一个 DLL 导入组
static ImportDll *pe_import(PEBuilder *pe, const char *dll) {
    if (pe->ndlls >= 8) { fprintf(stderr,"PE: too many dlls\n"); exit(1); }
    ImportDll *d = &pe->dlls[pe->ndlls++];
    memset(d, 0, sizeof(*d));
    d->dll = dll;
    return d;
}
static void pe_import_add(ImportDll *d, const char *fn) {
    // ★ 去重：同一 DLL 的同名函数只导入一次 ★
    //   原因：common.starc 与各功能模块都会 linx 同一批 kernel32/msvcrt 函数，
    //   不去重会重复占位导入表，既撑爆 MAX_IMPORTS，
    //   也会让 PE 导入表出现重复项（GetProcAddress 索引错位）。
    for (int i = 0; i < d->nfuncs; i++)
        if (!strcmp(d->funcs[i], fn)) return;
    if (d->nfuncs >= MAX_IMPORTS) { fprintf(stderr,"PE: too many imports\n"); exit(1); }
    d->funcs[d->nfuncs++] = fn;
}

// ---------------------------------------------------------------- 写文件
// 1) 先计算所有段 RVA（按 SECTION_ALIGN 对齐）
// 2) 构造 .idata 内容
// 3) 拼接文件
static int pe_write(PEBuilder *pe, const char *path) {
    // ---- 0. 确保 .idata 段存在且排在最后（必须在算 RVA 之前）----
    // 同时给空段补 1 字节：空段会导致两个段拿到同一个 RVA、文件偏移重叠，
    // 表现为「程序能加载但读写全局变量时拿到垃圾值」——极难查。
    for (int i = 0; i < pe->nsecs; i++)
        if (pe->secs[i].body.len == 0) buf_u8(&pe->secs[i].body, 0);

    Section *idata = pe_find_section(pe, ".idata");
    if (!idata) idata = pe_section(pe, ".idata", SCN_IDATA | SCN_RDATA | SCN_RW);
    else {
        // 已有则挪到末尾，保证 RVA 递增
        int idx = pe_section_index(pe, ".idata");
        if (idx != pe->nsecs - 1) {
            Section tmp = pe->secs[idx];
            for (int i = idx; i < pe->nsecs - 1; i++) pe->secs[i] = pe->secs[i + 1];
            pe->secs[pe->nsecs - 1] = tmp;
        }
    }

    // ---- 0.5 DLL：建 .edata 导出表段（必须排在 .idata 之前，RVA 要递增）----
    Section *edata = NULL;
    if (pe->is_dll) {
        edata = pe_find_section(pe, ".edata");
        if (!edata) edata = pe_section(pe, ".edata", SCN_IDATA | SCN_RDATA);
        // 无论新建还是已有，都保证它排在 .idata 前面：
        // 段顺序 = RVA 顺序，.edata 必须在 .idata 之前，否则两者会拿到同一个 RVA。
        int eidx = pe_section_index(pe, ".edata");
        int iidx = pe_section_index(pe, ".idata");
        if (eidx > iidx) {
            Section tmp = pe->secs[eidx];
            for (int i = eidx; i > iidx; i--) pe->secs[i] = pe->secs[i - 1];
            pe->secs[iidx + 1] = tmp;
        }
    }

    // ---- 0.6 DLL：建 .reloc 重定位段 ----
    // DLL 会被加载到任意基址，没有重定位表 loader 会拒绝加载（ERROR_BAD_EXE_FORMAT）。
    // 我们的代码全部用 RIP-relative 寻址，本身不需要重定位，
    // 但 PE 规范要求 DLL 带一个合法的重定位目录 —— 写一个空的块（VirtualAddress=0, SizeOfBlock=8）。
    Section *reloc = NULL;
    if (pe->is_dll) {
        reloc = pe_find_section(pe, ".reloc");
        if (!reloc) reloc = pe_section(pe, ".reloc", SCN_IDATA | SCN_RDATA);
        reloc->body.len = 0;
        buf_u32(&reloc->body, 0);      // VirtualAddress = 0
        buf_u32(&reloc->body, 8);      // SizeOfBlock = 8
        // 把它放到 .idata 之前、.edata 之后
        int ridx = pe_section_index(pe, ".reloc");
        int eidx = pe_section_index(pe, ".edata");
        if (ridx > eidx) {
            Section tmp = pe->secs[ridx];
            for (int i = ridx; i > eidx; i--) pe->secs[i] = pe->secs[i - 1];
            pe->secs[eidx + 1] = tmp;
        }
    }

    // ★ 顺序很关键：DLL 的 .edata 段内容必须先填好（拿到真实长度），
    //   否则算 RVA 时 .edata 还是空的，会和 .idata 拿到同一个 RVA，
    //   表现为 loader 报 ERROR_BAD_EXE_FORMAT(193)。
    // 所以先算 .edata 的内容长度（用相对偏移），再统一算 RVA，最后回填绝对 RVA。
    uint32_t edata_rel_af = 0, edata_rel_an = 0, edata_rel_ao = 0, edata_rel_str = 0;
    uint32_t edata_size = 0;
    if (pe->is_dll) {
        int n = pe->nexports;
        edata_rel_af  = 40;                          // 导出目录固定 40 字节
        edata_rel_an  = edata_rel_af + 4 * (uint32_t)n;
        edata_rel_ao  = edata_rel_an + 4 * (uint32_t)n;
        edata_rel_str = edata_rel_ao + 2 * (uint32_t)n;
        uint32_t sz = edata_rel_str;
        for (int i = 0; i < n; i++) sz += (uint32_t)strlen(pe->exports[i]) + 1;
        sz += (uint32_t)strlen(pe->export_name) + 1;
        edata_size = sz;
        // 先把段体撑到正确长度（内容后面再填），这样算 RVA 时长度是对的
        edata->body.len = 0;
        buf_zeros(&edata->body, sz);
    }

    // .idata 的长度也要先算准（它在最后一段，但长度算错会让 SizeOfImage 偏小）
    {
        uint32_t sz = (uint32_t)((pe->ndlls + 1) * 20);
        for (int d = 0; d < pe->ndlls; d++) {
            sz += (uint32_t)((pe->dlls[d].nfuncs + 1) * 8) * 2;   // ILT + IAT
            for (int f = 0; f < pe->dlls[d].nfuncs; f++)
                sz += 2 + (uint32_t)strlen(pe->dlls[d].funcs[f]) + 1 + 1;  // Hint/Name（含对齐）
            sz += 1;                                              // DLL 名结尾对齐
        }
        idata->body.len = 0;
        buf_zeros(&idata->body, sz + 64);
    }

    uint32_t rva = 0x1000;
    for (int i = 0; i < pe->nsecs; i++) {
        if (getenv("SCDBG")) fprintf(stderr, "[dbg] sec[%d] %s len=%zu rva=0x%x\n", i, pe->secs[i].name, pe->secs[i].body.len, rva);
        pe->secs[i].rva  = rva;
        pe->secs[i].vsize = (uint32_t)pe->secs[i].body.len;
        rva += (uint32_t)((pe->secs[i].body.len + SECTION_ALIGN - 1) / SECTION_ALIGN * SECTION_ALIGN);
    }
    if (pe->entry_rva == 0) pe->entry_rva = pe->secs[0].rva;   // 默认入口 = 第一段开头

    // ---- 1.5 DLL：填充导出表内容（.edata）----
    // 结构（IMAGE_EXPORT_DIRECTORY）：
    //   [0]  Characteristics        [4] TimeDateStamp     [8] MajorVersion [12] MinorVersion
    //   [16] Name RVA              [20] Base              [24] NumberOfFunctions
    //   [28] NumberOfNames         [32] AddressOfFunctions[36] AddressOfNames
    //   [40] AddressOfNameOrdinals
    // 之后依次：函数地址表(uint32×n)、名字指针表(uint32×n)、名字序数表(uint16×n)、名字串
    if (pe->is_dll) {
        Buf *b = &edata->body;
        b->len = 0;
        int n = pe->nexports;

        // ★ 导出名必须按 ASCII 升序排列 ★
        //   Windows 的 GetProcAddress 用二分查找导出名字符串表，
        //   名字没排序就只有一部分函数能被查到（实测：前 3 个查不到，第 4 个起能查到）。
        //   排序时要同步重排「函数地址表」和「名字序数表」，否则地址会错位。
        {
            int order[512];
            for (int i = 0; i < n; i++) order[i] = i;
            // 插入排序（n 通常几十，够快）
            for (int i = 1; i < n; i++) {
                int key = order[i];
                int j = i - 1;
                while (j >= 0 && strcmp(pe->exports[order[j]], pe->exports[key]) > 0) {
                    order[j + 1] = order[j];
                    j--;
                }
                order[j + 1] = key;
            }
            // 按排序结果重建三张表的索引
            uint32_t tmp_addr[512];
            uint16_t tmp_ord[512];
            for (int i = 0; i < n; i++) {
                tmp_addr[i] = 0;               // 地址表稍后回填
                tmp_ord[i] = (uint16_t)order[i];
            }
            // 名字指针表按排序后的顺序写；序数表指向原函数下标
            // （地址表下标 = 原下标，所以序数表用 order[i]）
            // 这里把排序结果暂存到 pe->exports 的重排副本
            static char *sorted_names[512];
            static uint32_t sorted_off[512];
            for (int i = 0; i < n; i++) {
                sorted_names[i] = pe->exports[order[i]];
                sorted_off[i] = (uint32_t)i;
            }
            // 覆盖原数组（保持 exports[] 是排序后的顺序）
            for (int i = 0; i < n; i++) pe->exports[i] = sorted_names[i];
            // 序数表改为记录「函数地址表下标」= order[i]
            for (int i = 0; i < n; i++) pe->export_ord_map[i] = (uint16_t)order[i];
        }

        uint32_t af_rva  = edata->rva + edata_rel_af;             // 函数地址表
        uint32_t an_rva  = edata->rva + edata_rel_an;             // 名字指针表
        uint32_t ao_rva  = edata->rva + edata_rel_ao;             // 名字序数表
        uint32_t str_rva = edata->rva + edata_rel_str;            // 名字串区

        // 先算名字串偏移
        uint32_t str_off[512];
        uint32_t cur = str_rva;
        for (int i = 0; i < n; i++) {
            str_off[i] = cur;
            cur += (uint32_t)strlen(pe->exports[i]) + 1;
        }
        uint32_t dllname_rva = cur;

        // 写导出目录（40 字节）
        buf_u32(b, 0);                 // Characteristics
        buf_u32(b, 0);                 // TimeDateStamp
        buf_u16(b, 0); buf_u16(b, 0);  // Major/MinorVersion
        buf_u32(b, dllname_rva);       // Name RVA
        buf_u32(b, 1);                 // Base（导出序号从 1 开始）
        buf_u32(b, (uint32_t)n);       // NumberOfFunctions
        buf_u32(b, (uint32_t)n);       // NumberOfNames
        buf_u32(b, af_rva);            // AddressOfFunctions
        buf_u32(b, an_rva);            // AddressOfNames
        buf_u32(b, ao_rva);            // AddressOfNameOrdinals

        // 函数地址表：先占位，稍后回填真实 RVA
        for (int i = 0; i < n; i++) buf_u32(b, 0);
        // 名字指针表（按排序后的名字顺序）
        for (int i = 0; i < n; i++) buf_u32(b, str_off[i]);
        // 名字序数表：第 i 个名字 → 函数地址表里的第 export_ord_map[i] 项
        for (int i = 0; i < n; i++) buf_u16(b, pe->export_ord_map[i]);
        // 名字串
        for (int i = 0; i < n; i++) { buf_bytes(b, pe->exports[i], strlen(pe->exports[i])); buf_u8(b, 0); }
        // DLL 名
        buf_bytes(b, pe->export_name, strlen(pe->export_name));
        buf_u8(b, 0);

        pe->export_dir_rva = edata->rva;
        pe->export_addr_table_rva = af_rva;
        pe->export_name_table_rva = an_rva;
        pe->export_ord_table_rva = ao_rva;
    }
    // ---- 2. 构造导入表（.idata）----
    {
        Buf *b = &idata->body;
        b->len = 0;

        // 先预留导入目录表空间
        size_t dir_off = 0;
        buf_zeros(b, (pe->ndlls + 1) * 20);

        // 计算各块 RVA（顺序排布：ILT -> IAT -> Hint/Name -> DLL名）
        uint32_t cur = idata->rva + (uint32_t)((pe->ndlls + 1) * 20);
        uint32_t ilt_rvas[8], iat_rvas[8], hn_rva[8], nm_rva[8];
        for (int d = 0; d < pe->ndlls; d++) {
            ilt_rvas[d] = cur; cur += (uint32_t)((pe->dlls[d].nfuncs + 1) * 8);
        }
        for (int d = 0; d < pe->ndlls; d++) {
            iat_rvas[d] = cur; cur += (uint32_t)((pe->dlls[d].nfuncs + 1) * 8);
        }
        for (int d = 0; d < pe->ndlls; d++) {
            hn_rva[d] = cur;
            for (int f = 0; f < pe->dlls[d].nfuncs; f++) {
                cur += 2 + (uint32_t)strlen(pe->dlls[d].funcs[f]) + 1;
                cur = (cur + 1) & ~1u;      // ★ 逐项对齐，与写入路径保持一致
            }
            cur = (cur + 1) & ~1u;
        }
        for (int d = 0; d < pe->ndlls; d++) {
            nm_rva[d] = cur; cur += (uint32_t)strlen(pe->dlls[d].dll) + 1;
            cur = (cur + 1) & ~1u;
        }

        // 写 ILT
        for (int d = 0; d < pe->ndlls; d++) {
            ImportDll *dl = &pe->dlls[d];
            dl->ilt_rva = ilt_rvas[d];
            dl->iat_rva = iat_rvas[d];
            dl->name_rva = nm_rva[d];
            uint32_t hn = hn_rva[d];
            for (int f = 0; f < dl->nfuncs; f++) {
                buf_u64(b, (uint64_t)hn);
                hn += 2 + (uint32_t)strlen(dl->funcs[f]) + 1;
                hn = (hn + 1) & ~1u;      // ★ 必须与写入路径的 buf_align 一致
            }
            buf_u64(b, 0);
        }
        // 写 IAT（loader 会用真实函数地址覆盖）
        for (int d = 0; d < pe->ndlls; d++) {
            ImportDll *dl = &pe->dlls[d];
            uint32_t hn = hn_rva[d];
            for (int f = 0; f < dl->nfuncs; f++) {
                buf_u64(b, (uint64_t)hn);
                hn += 2 + (uint32_t)strlen(dl->funcs[f]) + 1;
                hn = (hn + 1) & ~1u;      // ★ 同上
            }
            buf_u64(b, 0);
        }
        // 写 Hint/Name
        for (int d = 0; d < pe->ndlls; d++) {
            ImportDll *dl = &pe->dlls[d];
            for (int f = 0; f < dl->nfuncs; f++) {
                buf_u16(b, 0);
                buf_bytes(b, dl->funcs[f], strlen(dl->funcs[f]));
                buf_u8(b, 0);
                buf_align(b, 2);
            }
        }
        // 写 DLL 名
        for (int d = 0; d < pe->ndlls; d++) {
            buf_bytes(b, pe->dlls[d].dll, strlen(pe->dlls[d].dll));
            buf_u8(b, 0);
            buf_align(b, 2);
        }
        // 回填导入目录表
        for (int d = 0; d < pe->ndlls; d++) {
            size_t o = dir_off + (size_t)d * 20;
            buf_patch_u32(b, o + 0,  pe->dlls[d].ilt_rva);
            buf_patch_u32(b, o + 4,  0);
            buf_patch_u32(b, o + 8,  0);
            buf_patch_u32(b, o + 12, pe->dlls[d].name_rva);
            buf_patch_u32(b, o + 16, pe->dlls[d].iat_rva);
        }
        idata->vsize = (uint32_t)b->len;
    }

    // ---- 2.4 DLL：回填导出函数地址表 ----
    // 导出函数的真实 RVA = .text 段 RVA + 该函数标签在段内的偏移
    if (pe->is_dll && edata) {
        uint32_t text_rva_now = 0;
        for (int i = 0; i < pe->nsecs; i++)
            if (strcmp(pe->secs[i].name, ".text") == 0) { text_rva_now = pe->secs[i].rva; break; }
        Buf *eb = &edata->body;
        uint32_t af_off = edata_rel_af;                    // 地址表在 .edata 内的偏移
        // ★ 地址表下标 = 原始（未排序）导出顺序；名字序数表通过 export_ord_map 映射过来。
        //   所以这里按「原始下标」写地址，序数表负责把排序后的名字指到正确的地址项。
        for (int i = 0; i < pe->nexports; i++) {
            uint32_t fn_off = pe->export_lab_offs[i];
            uint32_t fn_rva = text_rva_now + fn_off;
            buf_patch_u32(eb, af_off + 4 * (uint32_t)i, fn_rva);
        }
    }

    // ---- 2.5 重算镜像大小 ----
    // .idata 的内容是在 RVA 算完之后才填的（算 RVA 时它还是空的），所以必须重算，
    // 否则 SizeOfImage 会漏掉最后一段 → loader 报 "Exec format error"。
    uint32_t image_size = rva;    {
        uint32_t end = 0;
        for (int i = 0; i < pe->nsecs; i++) {
            uint32_t sz = (uint32_t)pe->secs[i].body.len;
            pe->secs[i].vsize = sz;
            uint32_t e = pe->secs[i].rva + (uint32_t)((sz + SECTION_ALIGN - 1) / SECTION_ALIGN * SECTION_ALIGN);
            if (e > end) end = e;
        }
        image_size = end;
    }

    // ---- 3. 回填所有延迟重定位 ----
    for (int i = 0; i < pe->nfixes; i++) {
        PEFix *f = &pe->fixes[i];
        Section *from = &pe->secs[f->from_sec];
        int64_t here = (int64_t)from->rva + f->from_off + 4;   // disp 相对的下一条指令
        int64_t target = 0;
        if (f->to_sec >= 0) {
            target = (int64_t)pe->secs[f->to_sec].rva + f->to_off;
        } else {
            // 导入符号：在 IAT 里查
            char dll[64]; const char *fn = NULL;
            const char *bang = strchr(f->sym, '!');
            if (!bang) { fprintf(stderr, "PE: bad symbol %s\n", f->sym); return 0; }
            size_t dl = (size_t)(bang - f->sym);
            if (dl >= sizeof(dll)) dl = sizeof(dll) - 1;
            memcpy(dll, f->sym, dl); dll[dl] = 0;
            fn = bang + 1;
            for (int d = 0; d < pe->ndlls; d++) {
                if (strcmp(pe->dlls[d].dll, dll) != 0) continue;
                for (int k = 0; k < pe->dlls[d].nfuncs; k++) {
                    if (strcmp(pe->dlls[d].funcs[k], fn) == 0) {
                        target = (int64_t)pe->dlls[d].iat_rva + k * 8;
                        break;
                    }
                }
                break;
            }
            if (target == 0) { fprintf(stderr, "PE: unresolved import %s\n", f->sym); return 0; }
        }
        int64_t disp = target - here;
        if (disp < -0x80000000LL || disp > 0x7FFFFFFFLL) {
            fprintf(stderr, "PE: rel32 out of range for %s\n", f->sym);
            return 0;
        }
        buf_patch_u32(&from->body, f->from_off, (uint32_t)(int32_t)disp);
    }

    // ---- 3. 计算文件布局 ----
    uint32_t nsec = (uint32_t)pe->nsecs;
    uint32_t optional_size = 240;                  // PE32+ 固定 240
    uint32_t headers_size  = 0x40 /*DOS*/ + 4 /*PE sig*/ + 20 /*COFF*/ + optional_size
                           + nsec * 40;
    // 文件头按 FILE_ALIGN 对齐
    uint32_t headers_aligned = (headers_size + FILE_ALIGN - 1) / FILE_ALIGN * FILE_ALIGN;

    uint32_t off = headers_aligned;
    for (int i = 0; i < pe->nsecs; i++) {
        pe->secs[i].raw_off  = off;
        pe->secs[i].raw_size = (uint32_t)((pe->secs[i].body.len + FILE_ALIGN - 1) / FILE_ALIGN * FILE_ALIGN);
        off += pe->secs[i].raw_size;
    }
    uint32_t file_size = off;

    // 找出 .text 段 RVA / 大小
    uint32_t text_rva = 0, text_size = 0, data_size = 0;
    for (int i = 0; i < pe->nsecs; i++) {
        if (strcmp(pe->secs[i].name, ".text") == 0) { text_rva = pe->secs[i].rva; text_size = pe->secs[i].raw_size; }
        else data_size += pe->secs[i].raw_size;
    }

    // ---- 4. 组装整个文件 ----
    Buf f; buf_init(&f, file_size + 4096);
    memset(f.p, 0, f.cap);

    // --- DOS Header (严格 64 字节) ---
    // 直接铺 64 字节再定点打补丁，避免逐字段数错（PE 手工构造的经典坑）
    {
        buf_zeros(&f, 64);
        f.p[0] = 'M'; f.p[1] = 'Z';             // e_magic
        f.p[2] = 0x90; f.p[3] = 0x00;           // e_cblp
        f.p[4] = 0x03; f.p[5] = 0x00;           // e_cp
        f.p[0x18] = 0x04; f.p[0x19] = 0x00;     // e_lfarlc
        buf_patch_u32(&f, 0x3C, 0x40);          // e_lfanew → PE 头在 0x40
    }

    // --- PE Signature ---
    buf_u32(&f, 0x00004550);          // 'PE\0\0'

    // --- COFF File Header ---
    buf_u16(&f, PE_MACHINE_AMD64);
    buf_u16(&f, (uint16_t)nsec);
    buf_u32(&f, 0);                   // TimeDateStamp
    buf_u32(&f, 0);                   // PointerToSymbolTable
    buf_u32(&f, 0);                   // NumberOfSymbols
    buf_u16(&f, (uint16_t)optional_size);
    // DLL: 0x2022 = IMAGE_FILE_DLL | EXECUTABLE_IMAGE | LARGE_ADDRESS_AWARE
    // （系统 DLL 都是 0x2022：少了 0x2000 不给 DLL 标志，少了 0x0022 loader 也拒绝）
    buf_u16(&f, pe->is_dll ? 0x2022 : 0x0022);

    // --- Optional Header (PE32+) ---
    buf_u16(&f, 0x20B);               // Magic = PE32+
    buf_u8(&f, 1); buf_u8(&f, 0);     // LinkerVersion
    buf_u32(&f, text_size);           // SizeOfCode
    buf_u32(&f, data_size);           // SizeOfInitializedData
    buf_u32(&f, 0);                   // SizeOfUninitializedData
    buf_u32(&f, pe->entry_rva);       // AddressOfEntryPoint
    buf_u32(&f, text_rva);            // BaseOfCode
    buf_u64(&f, IMAGE_BASE);          // ImageBase
    buf_u32(&f, SECTION_ALIGN);
    buf_u32(&f, FILE_ALIGN);
    buf_u16(&f, 6); buf_u16(&f, 0);   // OS version
    buf_u16(&f, 0); buf_u16(&f, 0);   // Image version
    buf_u16(&f, 6); buf_u16(&f, 0);   // Subsystem version
    buf_u32(&f, 0);                   // Win32VersionValue
    buf_u32(&f, image_size);          // SizeOfImage
    buf_u32(&f, headers_aligned);     // SizeOfHeaders
    buf_u32(&f, 0);                   // CheckSum
    buf_u16(&f, 3);                   // Subsystem = CONSOLE
    buf_u16(&f, 0x8100);              // NX_COMPAT | TERMINAL_SERVER_AWARE（不含 DYNAMIC_BASE）
    buf_u64(&f, 0x800000);            // SizeOfStackReserve = 8MB（深递归/多线程用）
    buf_u64(&f, 0x1000);              // SizeOfStackCommit
    buf_u64(&f, 0x100000);            // SizeOfHeapReserve
    buf_u64(&f, 0x1000);              // SizeOfHeapCommit
    buf_u32(&f, 0);                   // LoaderFlags
    buf_u32(&f, 16);                  // NumberOfRvaAndSizes

    // --- Data Directories (16 * 8) ---
    for (int i = 0; i < 16; i++) { buf_u32(&f, 0); buf_u32(&f, 0); }
    // 导入表目录 = 索引 1
    {
        size_t o = 0x40 + 4 + 20 + 112 + 8 * 1;
        buf_patch_u32(&f, o,     idata->rva);
        buf_patch_u32(&f, o + 4, (pe->ndlls + 1) * 20);
    }
    // 导出表目录 = 索引 0（DLL 专用）
    if (pe->is_dll && edata) {
        size_t o = 0x40 + 4 + 20 + 112 + 8 * 0;
        buf_patch_u32(&f, o,     edata->rva);
        buf_patch_u32(&f, o + 4, (uint32_t)edata->body.len);
    }
    // 重定位表目录 = 索引 5（DLL 必需）
    if (pe->is_dll && reloc) {
        size_t o = 0x40 + 4 + 20 + 112 + 8 * 5;
        buf_patch_u32(&f, o,     reloc->rva);
        buf_patch_u32(&f, o + 4, (uint32_t)reloc->body.len);
    }

    // --- Section Headers ---
    for (int i = 0; i < pe->nsecs; i++) {
        Section *s = &pe->secs[i];
        char nm[8]; memset(nm, 0, 8); strncpy(nm, s->name, 8);
        buf_bytes(&f, nm, 8);
        buf_u32(&f, s->vsize);
        buf_u32(&f, s->rva);
        buf_u32(&f, s->raw_size);
        buf_u32(&f, s->raw_off);
        buf_u32(&f, 0); buf_u32(&f, 0);
        buf_u16(&f, 0); buf_u16(&f, 0);
        buf_u32(&f, s->chars);
    }

    // --- 补齐到 headers_aligned ---
    buf_align(&f, FILE_ALIGN);

    // --- 段内容 ---
    for (int i = 0; i < pe->nsecs; i++) {
        Section *s = &pe->secs[i];
        buf_bytes(&f, s->body.p, s->body.len);
        buf_align(&f, FILE_ALIGN);
    }

    // ---- 5. 写盘 ----
    FILE *fp = fopen(path, "wb");
    if (!fp) { fprintf(stderr, "PE: cannot open %s\n", path); buf_free(&f); return 0; }
    fwrite(f.p, 1, f.len, fp);
    fclose(fp);
    buf_free(&f);
    return 1;
}

#endif // STARC_PE_C
