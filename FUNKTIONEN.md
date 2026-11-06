# StarC 语言函数清单（FUNKTIONEN）

> 本文档**完整列举** StarC 编程语言当前（2026-11-06）出现过的**全部函数**，共 **813 个唯一函数**。
> 数据来源：`D:\starc\inous`（整合自 `D:\starc` 全部 187 个 `.staro` 文件）。
> 每个函数条目：**函数名（参数）** + **20 字左右功能说明** + **调用示例（1-5 行）**。
> 同名函数按函数名去重（不同项目/备份中的同名实现只列一次）。

**说明**：StarC 是德语式关键字的编程语言——函数定义用 `Funktion 名(参数) ... Ende`，返回用 `gebe_zurueck 值`。库文件后缀为 `.staro`，通过 `linx-o` 可直接调用 WinAPI/CUDA/PTX 底层能力。编译用 `D:\starc\starcc_v13\starcc.exe`。

---

## 目录

- [底层内存/字节操作](#底层内存字节操作)
- [字符串处理](#字符串处理)
- [文件系统](#文件系统)
- [数学/浮点](#数学浮点)
- [PTX/CUDA 内核发射](#ptxcuda-内核发射)
- [x86 汇编发射](#x86-汇编发射)
- [解析器/词法器](#解析器词法器)
- [其他](#其他)

---

## 底层内存/字节操作

<!-- FUNC_LIST_START -->
### `accum_scale_ptx(out, off)`
**功能**：发射 PTX 内核 accum_scale，实现 y[e] += w*in[e] 的累乘加。
**示例**：
```starc
// 发射 accum_scale 内核 PTX 到 out，返回新偏移
off = accum_scale_ptx(out, 0)
```

### `adamw_step(param, m, v, grad, n, step, opts)`
**功能**：一步 AdamW 优化：累积动量 + 偏差修正后更新参数。
**示例**：
```starc
// param/m/v/grad 为缓冲，opts 存超参(lr,beta1,beta2,eps,wd)
adamw_step(param, m, v, grad, n, step, opts)
```

### `add(a, b)`
**功能**：整数/浮点加法，返回 a + b。
**示例**：
```starc
r = add(3, 4)      // r = 7
```

### `alloc_raw(size)`
**功能**：分配 size 字节原始内存，返回起始指针。
**示例**：
```starc
buf = alloc_raw(1024)   // 分配 1024 字节
```

### `argument_n(c, pos, n)`
**功能**：从 C 字符串 pos 起取第 n 个参数（1 基），返回 StarC 字符串。
**示例**：
```starc
// 从 "cmd arg1 arg2" 的 pos 起取第 2 个参数
s = argument_n(c, pos, 2)
```

### `asm_add_r64_rm64(buf, off, rm, src)`
**功能**：发射 x86 `add r/m64, r64`（48 01 /r）指令，返回新偏移。
**示例**：
```starc
off = asm_add_r64_rm64(buf, off, 3, 0)  // add [rbx], rax
```

### `asm_add_rm64_r64(buf, off, dest, rm)`
**功能**：发射 x86 `add r64, r/m64`（48 03 /r）指令，返回新偏移。
**示例**：
```starc
off = asm_add_rm64_r64(buf, off, 0, 3)  // add rax, [rbx]
```

### `asm_add_rsp(buf, off, n)`
**功能**：发射 `add rsp, imm32`（48 81 C4）调整栈指针，返回新偏移。
**示例**：
```starc
off = asm_add_rsp(buf, off, 16)   // 栈上移 16 字节
```

### `asm_byte(buf, off, b)`
**功能**：向缓冲区 buf+off 写 1 字节 b，返回新偏移 off+1。
**示例**：
```starc
off = asm_byte(buf, off, 0x48)
```

### `asm_call_rax(buf, off)`
**功能**：发射 `call rax`（FF D0），通过 rax 间接调用，返回新偏移。
**示例**：
```starc
off = asm_call_rax(buf, off)
```

### `asm_call_rel32(buf, off, disp)`
**功能**：发射 `call rel32`（E8 disp32）相对调用，返回新偏移。
**示例**：
```starc
off = asm_call_rel32(buf, off, 0x100)
```

### `asm_cqo(buf, off)`
**功能**：发射 `cqo`（48 99）符号扩展 rax 到 rdx:rax，返回新偏移。
**示例**：
```starc
off = asm_cqo(buf, off)
```

### `asm_epilogue(buf, off)`
**功能**：发射函数收尾 `leave`(C9) + `ret`(C3)，返回新偏移。
**示例**：
```starc
off = asm_epilogue(buf, off)
```

### `asm_idiv_rm64(buf, off, rm)`
**功能**：发射 `idiv r/m64`（48 F7 /7）有符号除法，返回新偏移。
**示例**：
```starc
off = asm_idiv_rm64(buf, off, 3)
```

### `asm_imul_r64_rm64(buf, off, dest, rm)`
**功能**：发射 `imul r64, r/m64`（48 0F AF /r）有符号乘法，返回新偏移。
**示例**：
```starc
off = asm_imul_r64_rm64(buf, off, 0, 3)  // imul rax,[rbx]
```

### `asm_init(bufcap)`
**功能**：初始化全局代码缓冲区（可动态增长），返回容量。
**示例**：
```starc
cap = asm_init(4096)
```

### `asm_int3(buf, off)`
**功能**：发射 `int3`（CC）软件断点指令，返回新偏移。
**示例**：
```starc
off = asm_int3(buf, off)
```

### `asm_jcc_rel32(buf, off, cc, disp)`
**功能**：发射条件跳转 `jcc rel32`（0F 8x disp32），cc 为条件码。
**示例**：
```starc
off = asm_jcc_rel32(buf, off, 4, 16)  // je +16
```

### `asm_jmp_rel32(buf, off, disp)`
**功能**：发射无条件跳转 `jmp rel32`（E9 disp32），返回新偏移。
**示例**：
```starc
off = asm_jmp_rel32(buf, off, -8)
```

### `asm_mov_r64_imm32(buf, off, rm, v)`
**功能**：发射 `mov r/m64, imm32`（48 C7 /0）符号扩展立即数，返回新偏移。
**示例**：
```starc
off = asm_mov_r64_imm32(buf, off, 0, 5)  // mov rax,5
```

### `asm_mov_r64_rm64(buf, off, rm, src)`
**功能**：发射 `mov r/m64, r64`（48 89 /r），rm 为目的地，返回新偏移。
**示例**：
```starc
off = asm_mov_r64_rm64(buf, off, 3, 0)  // mov [rbx],rax
```

### `asm_mov_rax_imm64(buf, off, v)`
**功能**：发射 `mov rax, imm64`（48 B8）载入 64 位立即数，返回新偏移。
**示例**：
```starc
off = asm_mov_rax_imm64(buf, off, 0x123456789)
```

### `asm_mov_rax_rip(buf, off, disp)`
**功能**：发射 `mov rax, [rip+disp32]`（48 8B 05）RIP 相对读取，返回新偏移。
**示例**：
```starc
off = asm_mov_rax_rip(buf, off, 0x10)
```

### `asm_mov_rax_rspdisp(buf, off, disp)`
**功能**：发射 `mov rax, [rsp+disp32]`（48 8B 44 24）栈偏移读取，返回新偏移。
**示例**：
```starc
off = asm_mov_rax_rspdisp(buf, off, 8)
```

### `asm_mov_rbpdisp_reg(buf, off, reg, disp)`
**功能**：发射 `mov [rbp+disp32], reg`（48 89 45）写栈槽，返回新偏移。
**示例**：
```starc
off = asm_mov_rbpdisp_reg(buf, off, 0, -16)  // mov [rbp-16],rax
```

### `asm_mov_reg_rbpdisp(buf, off, reg, disp)`
**功能**：发射 `mov reg, [rbp+disp32]`（48 8B 45）读栈槽，disp 可为负。
**示例**：
```starc
off = asm_mov_reg_rbpdisp(buf, off, 0, -16)
```

### `asm_mov_rip_rax(buf, off, disp)`
**功能**：发射 `mov [rip+disp32], rax`（48 89 05）RIP 相对写入，返回新偏移。
**示例**：
```starc
off = asm_mov_rip_rax(buf, off, 0x10)
```

### `asm_mov_rm64_r64(buf, off, dest, rm)`
**功能**：发射 `mov r64, r/m64`（48 8B /r）寄存器读取，返回新偏移。
**示例**：
```starc
off = asm_mov_rm64_r64(buf, off, 0, 3)  // mov rax,[rbx]
```

### `asm_mov_rspdisp_rax(buf, off, disp)`
**功能**：发射 `mov [rsp+disp32], rax`（48 89 44 24）栈偏移写入，返回新偏移。
**示例**：
```starc
off = asm_mov_rspdisp_rax(buf, off, 8)
```

### `asm_pop(buf, off, reg)`
**功能**：发射 `pop r64`（58+rd）弹栈到寄存器，返回新偏移。
**示例**：
```starc
off = asm_pop(buf, off, 0)  // pop rax
```

### `asm_prologue(buf, off)`
**功能**：发射函数序言 `push rbp; mov rbp,rsp`（55 48 89 E5）。
**示例**：
```starc
off = asm_prologue(buf, off)
```

### `asm_push(buf, off, reg)`
**功能**：发射 `push r64`（50+rd）压栈寄存器，返回新偏移。
**示例**：
```starc
off = asm_push(buf, off, 0)  // push rax
```

### `asm_ret(buf, off)`
**功能**：发射 `ret`（C3）函数返回指令，返回新偏移。
**示例**：
```starc
off = asm_ret(buf, off)
```

### `asm_sub_r64_rm64(buf, off, rm, src)`
**功能**：发射 `sub r/m64, r64`（48 29 /r）减法，返回新偏移。
**示例**：
```starc
off = asm_sub_r64_rm64(buf, off, 3, 0)  // sub [rbx],rax
```

### `asm_sub_rm64_r64(buf, off, dest, rm)`
**功能**：发射 `sub r64, r/m64`（48 2B /r）减法，返回新偏移。
**示例**：
```starc
off = asm_sub_rm64_r64(buf, off, 0, 3)  // sub rax,[rbx]
```

### `asm_sub_rsp(buf, off, n)`
**功能**：发射 `sub rsp, imm32`（48 81 EC）栈分配，返回新偏移。
**示例**：
```starc
off = asm_sub_rsp(buf, off, 32)  // 栈下移 32 字节
```

### `asm_word32(buf, off, v)`
**功能**：向缓冲区写 4 字节小端整数 v，返回新偏移 off+4。
**示例**：
```starc
off = asm_word32(buf, off, 0xDEADBEEF)
```

### `asm_word64(buf, off, v)`
**功能**：向缓冲区写 8 字节小端整数 v，返回新偏移 off+8。
**示例**：
```starc
off = asm_word64(buf, off, 0x1234567890)
```

### `attn_dot_ptx(out, off)`
**功能**：发射 PTX 内核 attn_dot：每线程算 q 与 kv 点积得分。
**示例**：
```starc
off = attn_dot_ptx(out, 0)
```

### `attn_forward(x, kv_cache, t, ctx, scratch, cache, cap)`
**功能**：单 token 注意力前向：Q/K/V 变换、点积、softmax、输出。
**示例**：
```starc
attn_forward(x, kv_cache, t, ctx, scratch, cache, cap)
```

### `attn_gpu(cache, kv_cache, qbuf, sink, attn_buf, n, ring, scratch)`
**功能**：GPU 注意力三内核发射（dot+softmax+out）并回传结果。
**示例**：
```starc
attn_gpu(cache, kv_cache, qbuf, sink, attn_buf, n, ring, scratch)
```

### `attn_out_ptx(out, off)`
**功能**：发射 PTX 内核 attn_out：按权重合成每头输出。
**示例**：
```starc
off = attn_out_ptx(out, 0)
```

### `attn_softmax_ptx(out, off)`
**功能**：发射 PTX 内核 attn_softmax：每头 softmax 含 sink 归一。
**示例**：
```starc
off = attn_softmax_ptx(out, 0)
```

### `axpy_ptx(out, off)`
**功能**：发射 PTX 内核 axpy：out[i] += w * y[i]。
**示例**：
```starc
off = axpy_ptx(out, 0)
```

### `b16(out, off, v)`
**功能**：向原始缓冲写 2 字节小端整数 v，返回新偏移。
**示例**：
```starc
off = b16(out, off, 0x1234)
```

### `b32(out, off, v)`
**功能**：向原始缓冲写 4 字节小端整数 v，返回新偏移。
**示例**：
```starc
off = b32(out, off, 0x12345678)
```

### `b8(out, off, v)`
**功能**：向原始缓冲写 1 字节 v，返回新偏移 off+1。
**示例**：
```starc
off = b8(out, off, 0xFF)
```

### `basis_name(p)`
**功能**：取路径最后一段名（去 .exe/.com 扩展），如 arp.exe→arp。
**示例**：
```starc
n = basis_name("C:/Windows/arp.exe")  // "arp"
```

### `befehl_ausfuehren(name, c, rest)`
**功能**：按命令名分发执行 StarShell 内建命令。
**示例**：
```starc
befehl_ausfuehren("echo", c, rest)
```

### `befehl_beschreibung(tab, voll, token)`
**功能**：输出某命令的完整描述（含第一 token 回退）。
**示例**：
```starc
befehl_beschreibung(tab, "dir", "dir")
```

### `befehl_beschreibung_runde(tab, name, runde)`
**功能**：命令描述查询的一轮匹配（按全名或首 token）。
**示例**：
```starc
r = befehl_beschreibung_runde(tab, "dir", 1)
```

### `befehl_token(name)`
**功能**：取一行命令的首个单词（直到空格），返回子串。
**示例**：
```starc
t = befehl_token("dir /w")  // "dir"
```

### `befehl_vorhanden(tab, name)`
**功能**：判断命令表中是否含指定命令名，返回 1/0。
**示例**：
```starc
ok = befehl_vorhanden(tab, "dir")
```

### `befehl_zaehlen(tab)`
**功能**：统计命令表中可执行命令的条数。
**示例**：
```starc
n = befehl_zaehlen(tab)
```

### `benutzer_name()`
**功能**：通过 GetUserNameA 取当前登录用户名，返回 StarC 字符串。
**示例**：
```starc
u = benutzer_name()
```

### `bereich_auflisten(tab)`
**功能**：输出命令表中所有分组（[组名]）标题。
**示例**：
```starc
bereich_auflisten(tab)
```

### `bereich_name(tab, pos, ende)`
**功能**：取 [pos,ende) 分组名（方括号内文字）。
**示例**：
```starc
n = bereich_name(tab, pos, ende)
```

### `bereich_zaehlen(tab)`
**功能**：统计命令表中分组（[组名]）的数量。
**示例**：
```starc
n = bereich_zaehlen(tab)
```

### `bf16gemv_ptx(out, off)`
**功能**：发射 PTX BF16 GEMV 内核：out[m]=sum_k act[k]*w[m,k]。
**示例**：
```starc
off = bf16gemv_ptx(out, 0)
```

### `bild_leeren()`
**功能**：调用 system("cls") 清空控制台屏幕。
**示例**：
```starc
bild_leeren()
```

### `bild_leeren_nativ()`
**功能**：用 WinAPI 清屏（免 cmd），填空格并归位光标。
**示例**：
```starc
bild_leeren_nativ()
```

### `block_forward(ctx, hidden, kv_cache, t, token, scratch, cache, cap)`
**功能**：单层 Transformer Block 前向（注意力+FFN+残差）。
**示例**：
```starc
block_forward(ctx, hidden, kv_cache, t, token, scratch, cache, cap)
```

### `block_forward_student(ctx, hidden, kv_cache, t, token, scratch, cache, cap)`
**功能**：学生版 Block：无隐藏层，标准残差，单隐藏副本。
**示例**：
```starc
block_forward_student(ctx, hidden, kv_cache, t, token, scratch, cache, cap)
```

### `bt_check(n)`
**功能**：递归统计二叉树节点数（benchmark 校验）。
**示例**：
```starc
n = bt_check(root)
```

### `bt_run()`
**功能**：运行二叉树基准（造树+校验+输出计数）。
**示例**：
```starc
bt_run()
```

### `byte_to_str(byte)`
**功能**：把 0-9A-Z 字节值转成单字符 StarC 字符串。
**示例**：
```starc
s = byte_to_str(65)  // "A"
```

### `byteat_raw(p, i)`
**功能**：raw 缓冲取第 i 字节（用于帧头解析）。
**示例**：
```starc
b = byteat_raw(p, 8)
```

### `c_rest(c, pos)`
**功能**：取 C 字符串从 pos 到 NUL 的剩余部分为 StarC 串。
**示例**：
```starc
s = c_rest(c, 5)
```

### `c_teil(c, start, len)`
**功能**：取 C 字符串 [start, start+len) 子串为 StarC 串。
**示例**：
```starc
s = c_teil(c, 2, 5)
```

### `cache_clear(cache, cap)`
**功能**：回退缓存游标并作废条目，不释放 GPU 内存。
**示例**：
```starc
cache_clear(cache, cap)
```

### `cache_exists(path)`
**功能**：检查缓存文件是否存在，存在返回 1，否则 0。
**示例**：
```starc
ok = cache_exists("cache.bin")
```

### `cache_find(cache, cap, wbase)`
**功能**：按权重基址查找 GPU 常驻缓存槽，返回槽号或 -1。
**示例**：
```starc
slot = cache_find(cache, cap, wbase)
```

### `cache_flush(cache_path)`
**功能**：删除缓存文件，清空缓存，返回 0。
**示例**：
```starc
cache_flush("cache.bin")
```

### `cache_load_result(cache_path)`
**功能**：从缓存文件加载搜索结果路径，失败返回空串。
**示例**：
```starc
path = cache_load_result("cache.bin")
```

### `cache_pool(cache)`
**功能**：一次性分配 act/out/权重/尺度大池，后续 bump 分配。
**示例**：
```starc
cache_pool(cache)
```

### `cache_save_result(cache_path, result_path)`
**功能**：把搜索结果路径保存进缓存文件，成功返回 1。
**示例**：
```starc
cache_save_result("cache.bin", "C:/x/y.exe")
```

### `cache_slot(cache, cap)`
**功能**：找下一个空闲缓存槽号，返回 -1 表示已满。
**示例**：
```starc
slot = cache_slot(cache, cap)
```

### `close_dir(buffer)`
**功能**：从目录缓冲区末尾读句柄并关闭目录。
**示例**：
```starc
close_dir(buffer)
```

### `close_handle(handle)`
**功能**：关闭文件/设备句柄，返回 0。
**示例**：
```starc
close_handle(handle)
```

### `codegen_addr(irpn, start, len, arr_reg, out, off)`
**功能**：生成 PTX 数组元素地址计算代码，返回新偏移。
**示例**：
```starc
off = codegen_addr(irpn, 0, 4, 10, out, off)
```

### `codegen_float_rpn_at(frpn, irpn, start, len, out, off)`
**功能**：把浮点 RPN 片段生成为 PTX 指令序列。
**示例**：
```starc
off = codegen_float_rpn_at(frpn, irpn, 0, 6, out, off)
```

### `codegen_int_rpn_at(irpn, start, len, out, off)`
**功能**：把整数 RPN 片段生成为 PTX 指令序列。
**示例**：
```starc
off = codegen_int_rpn_at(irpn, 0, 6, out, off)
```

### `codegen_int_rpn_bare(irpn, start, len, out, off)`
**功能**：整数 RPN 生成为 x64 机器码（栈式，局部槽）。
**示例**：
```starc
off = codegen_int_rpn_bare(irpn, 0, 6, out, off)
```

### `codegen_prog(name, ast, ast_count, frpn, irpn, symtab, sym_count, out, off)`
**功能**：生成完整 PTX 程序（.entry 包装 + 代码），返回新偏移。
**示例**：
```starc
off = codegen_prog("main", ast, n, frpn, irpn, sym, sn, out, 0)
```

### `codegen_ptx_kernel(name, rpn, rn, out, off)`
**功能**：生成 PTX 内核骨架 .visible .entry name(...)，返回新偏移。
**示例**：
```starc
off = codegen_ptx_kernel("k1", rpn, rn, out, 0)
```

### `codegen_ptx_rpn(rpn, rn, out, off)`
**功能**：RPN 表达式生成为 PTX 求值（写 out[off..]）。
**示例**：
```starc
off = codegen_ptx_rpn(rpn, rn, out, 0)
```

### `codegen_rpn(rpn, rn, out, off)`
**功能**：RPN 生成为 x86 汇编指令文本，返回新偏移。
**示例**：
```starc
off = codegen_rpn(rpn, rn, out, 0)
```

### `codegen_stmt_bare(ast, node, irpn, out, off, fixbuf, cgst)`
**功能**：生成单个语句节点（赋值/循环/分支）的机器码。
**示例**：
```starc
off = codegen_stmt_bare(ast, node, irpn, out, off, fixbuf, cgst)
```

### `codegen_stmt_block_bare(ast, node_start, count, irpn, out, off, fixbuf, cgst)`
**功能**：生成连续语句块 [start, start+count) 的机器码。
**示例**：
```starc
off = codegen_stmt_block_bare(ast, s, 5, irpn, out, off, fixbuf, cgst)
```

### `codegen_stmts(ast, start, count, frpn, irpn, out, off)`
**功能**：生成语句列表为 PTX（浮点/整数赋值等），返回新偏移。
**示例**：
```starc
off = codegen_stmts(ast, 0, count, frpn, irpn, out, 0)
```

### `compare(a, b)`
**功能**：三路比较：a>b 返回 1，a<b 返回 -1，相等返回 0。
**示例**：
```starc
r = compare(3, 5)  // -1
```

### `compile_starc(src, ziel)`
**功能**：调用外部 starcc 编译 .starc→.sc，返回退出码。
**示例**：
```starc
code = compile_starc("prog.starc", "prog.sc")
```

### `concat(s1, s2)`
**功能**：拼接两个 StarC 字符串，返回新分配的字符串指针。
**示例**：
```starc
s = concat("ab", "cd")  // "abcd"
```

### `concat_cstr(s1, s2)`
**功能**：拼接两个 C 字符串，返回新 C 字符串指针。
**示例**：
```starc
p = concat_cstr("a", "b")
```

### `concat_starc_cstr(starc_str, cstr)`
**功能**：拼接 StarC 字符串与 C 字符串，返回 C 字符串指针。
**示例**：
```starc
p = concat_starc_cstr(s, cstr)
```

### `concat_str(s1, s2)`
**功能**：字符串连接，返回 s1+s2（StarC 字符串）。
**示例**：
```starc
s = concat_str("foo", "bar")
```

### `conv3d_bias_relu(in, w, bias, out, Ci, D, H, W, Co, KD, KH, KW)`
**功能**：3D 卷积+加偏置+ReLU 激活，in-place 输出。
**示例**：
```starc
conv3d_bias_relu(in, w, bias, out, 4, 8, 8, 8, 8, 3, 3, 3)
```

### `conv3d_forward(in, w, out, Ci, D, H, W, Co, KD, KH, KW)`
**功能**：3D 前向卷积 stride=1 无 padding，CPU 软浮点参考。
**示例**：
```starc
conv3d_forward(in, w, out, 4, 8, 8, 8, 8, 3, 3, 3)
```

### `conv3d_pad1(in, w, bias, out, Ci, D, H, W, Co)`
**功能**：3D 卷积 pad=1 核 3³ 尺寸不变，无 ReLU（解码末层）。
**示例**：
```starc
conv3d_pad1(in, w, bias, out, 8, 4, 4, 4, 1)
```

### `conv3d_pad1_backward(dout, fwd, in, w, din, dw, dbias, Ci, D, H, W, Co)`
**功能**：conv3d_pad1 反向传播，算输入/权重/偏置梯度。
**示例**：
```starc
conv3d_pad1_backward(dout, fwd, in, w, din, dw, dbias, 8, 4, 4, 4, 1)
```

### `conv3d_pad1_relu(in, w, bias, out, Ci, D, H, W, Co)`
**功能**：3D 卷积 pad=1 核 3³ + ReLU，尺寸不变（编码器）。
**示例**：
```starc
conv3d_pad1_relu(in, w, bias, out, 1, 16, 16, 16, 8)
```

### `conv3d_s2_backward(dout, fwd, in, w, din, dw, dbias, Ci, D, H, W, Co)`
**功能**：stride=2 pad=1 卷积反向，算各梯度。
**示例**：
```starc
conv3d_s2_backward(dout, fwd, in, w, din, dw, dbias, 8, 8, 8, 8, 16)
```

### `conv3d_s2_relu(in, w, bias, out, Ci, D, H, W, Co)`
**功能**：3D 卷积 stride=2 pad=1 核 3³ + ReLU，输出减半。
**示例**：
```starc
conv3d_s2_relu(in, w, bias, out, 1, 16, 16, 16, 8)
```

### `count_lines(str)`
**功能**：统计字符串中换行符(\n)的数量。
**示例**：
```starc
n = count_lines(s)
```

### `count_primes_serial(n)`
**功能**：单线程统计 <n 的素数个数（对照基准）。
**示例**：
```starc
c = count_primes_serial(100000)
```

### `cpu_kernzahl()`
**功能**：取系统处理器核数（GetSystemInfo dwNumberOfProcessors）。
**示例**：
```starc
n = cpu_kernzahl()
```

### `cpu_seitengroesse()`
**功能**：取系统内存页大小（GetSystemInfo dwPageSize）。
**示例**：
```starc
sz = cpu_seitengroesse()
```

### `cstr_append(dest, off, src)`
**功能**：把 C 串 src 追加到 dest 的 off 处，返回新长度。
**示例**：
```starc
n = cstr_append(dest, off, src)
```

### `cstr_copy(p)`
**功能**：分配并拷贝 C 字符串到新缓冲区，返回新指针。
**示例**：
```starc
q = cstr_copy(p)
```

### `cstr_eq(a, b)`
**功能**：逐字节比较两个 C 字符串，相等返回 1。
**示例**：
```starc
eq = cstr_eq("abc", "abc")  // 1
```

### `cstr_from_int(v)`
**功能**：把整数转成十进制 C 字符串（支持负数）。
**示例**：
```starc
s = cstr_from_int(-42)
```

### `cstr_len(p)`
**功能**：求 C 字符串长度（到 NUL 为止）。
**示例**：
```starc
n = cstr_len("hello")  // 5
```

### `cstr_starts_with(path, prefix)`
**功能**：判断 C 串 path 是否以 prefix 开头，返回 1/0。
**示例**：
```starc
ok = cstr_starts_with("a/b/c", "a/")  // 1
```

### `cstr_to_starc(c)`
**功能**：把 C 字符串转换为 StarC 字符串（带长度头）。
**示例**：
```starc
s = cstr_to_starc("hello")
```

### `cuda_count_side()`
**功能**：只读副作用诊断：忽略返回值直接读设备数。
**示例**：
```starc
n = cuda_count_side()
```

### `cuda_ctx_create(dev_idx)`
**功能**：创建 CUDA context（绑定设备），返回句柄，0 失败。
**示例**：
```starc
ctx = cuda_ctx_create(0)
```

### `cuda_ctx_destroy(ctx)`
**功能**：销毁 CUDA context，返回 0。
**示例**：
```starc
cuda_ctx_destroy(ctx)
```

### `cuda_device(ordinal)`
**功能**：取设备句柄（cuDeviceGet），失败返回 0。
**示例**：
```starc
dev = cuda_device(0)
```

### `cuda_device_count()`
**功能**：取 CUDA 设备数量，失败返回 -1。
**示例**：
```starc
n = cuda_device_count()
```

### `cuda_device_name(dev_idx, buf, buf_len)`
**功能**：把设备名写入原始缓冲 buf，返回 CUresult。
**示例**：
```starc
r = cuda_device_name(0, buf, 256)
```

### `cuda_diag_count()`
**功能**：诊断 cuDeviceGetCount 是否真实写 cnt，返回诊断缓冲。
**示例**：
```starc
res = cuda_diag_count()
```

### `cuda_func_get(mod, name_cstr, out_func)`
**功能**：从模块取内核函数句柄写入 out_func，返回 CUresult。
**示例**：
```starc
r = cuda_func_get(mod, "kernel", out_func)
```

### `cuda_init()`
**功能**：初始化 CUDA 驱动（cuInit），返回 CUresult。
**示例**：
```starc
r = cuda_init()
```

### `cuda_launch(func, gx, gy, gz, bx, by, bz, smem, stream, params)`
**功能**：以网格/块尺寸启动 CUDA 内核，返回 CUresult。
**示例**：
```starc
r = cuda_launch(func, 32,1,1, 256,1,1, 0, 0, params)
```


### `cuda_mem_alloc(size)`
**功能**：CUDA 显存分配（cuMemAlloc），失败返回 0。
**示例**：
```starc
ptr = cuda_mem_alloc(1024*1024)
```

### `cuda_mem_free(ptr)`
**功能**：释放 CUDA 显存，返回 0。
**示例**：
```starc
cuda_mem_free(ptr)
```

### `cuda_mem_info(res)`
**功能**：查询显存 free/total，写入 16 字节诊断缓冲。
**示例**：
```starc
cuda_mem_info(res)  // res[0..7]=free, res[8..15]=total
```

### `cuda_memcpy_dtoh(dst_host, src_dev, n)`
**功能**：设备→主机显存拷贝 n 字节，返回 CUresult。
**示例**：
```starc
r = cuda_memcpy_dtoh(host, dev, 4096)
```

### `cuda_memcpy_htod(dst_dev, src_host, n)`
**功能**：主机→设备显存拷贝 n 字节，返回 CUresult。
**示例**：
```starc
r = cuda_memcpy_htod(dev, host, 4096)
```

### `cuda_module_load(ptx_cstr, out_mod)`
**功能**：加载 PTX 字符串为 CUDA 模块，句柄写入 out_mod。
**示例**：
```starc
r = cuda_module_load(ptx, out_mod)
```

### `cur_thread_id()`
**功能**：取当前线程 ID（GetCurrentThreadId）。
**示例**：
```starc
tid = cur_thread_id()
```

### `custom_func_from_staro(x)`
**功能**：示例库函数：返回 x*2（mylib 测试）。
**示例**：
```starc
r = custom_func_from_staro(21)  // 42
```

### `datei_anlegen(args)`
**功能**：StarShell 命令 mkdir/md：创建目录。
**示例**：
```starc
datei_anlegen("D:/neu/ordner")
```

### `datei_attrib(args)`
**功能**：StarShell 命令 attrib：显示/设置文件属性。
**示例**：
```starc
datei_attrib("D:/x.txt")
```

### `datei_entfernen(args)`
**功能**：StarShell 命令 rmdir/rd：删除空目录。
**示例**：
```starc
datei_entfernen("D:/alt/ordner")
```

### `datei_existiert(args)`
**功能**：StarShell 命令 existiert/exists：判断路径存在。
**示例**：
```starc
datei_existiert("D:/x.txt")
```

### `datei_groesse(args)`
**功能**：StarShell 命令 groesse/size：取文件大小。
**示例**：
```starc
datei_groesse("D:/x.txt")
```

### `datei_kopieren(args)`
**功能**：StarShell 命令 copy/kopiere/cp：复制文件。
**示例**：
```starc
datei_kopieren("D:/a.txt D:/b.txt")
```

### `datei_lesen(args)`
**功能**：StarShell 命令 type/cat/zeige/more：显示文件内容。
**示例**：
```starc
datei_lesen("D:/x.txt")
```

### `datei_liste(args)`
**功能**：StarShell 命令 dir/ls/liste：列目录内容。
**示例**：
```starc
datei_liste("D:/ordner")
```

### `datei_liste_gross(args)`
**功能**：StarShell 命令 dirlist：列目录带大小。
**示例**：
```starc
datei_liste_gross("D:/ordner")
```

### `datei_loeschen(args)`
**功能**：StarShell 命令 del/rm/loesche/erase：删除文件。
**示例**：
```starc
datei_loeschen("D:/x.txt")
```

### `datei_touch(args)`
**功能**：StarShell 命令 touch：更新时间戳/建空文件。
**示例**：
```starc
datei_touch("D:/x.txt")
```

### `datei_verschieben(args)`
**功能**：StarShell 命令 move/verschiebe/ren/mv：移动或重命名。
**示例**：
```starc
datei_verschieben("D:/a.txt D:/b.txt")
```

### `datei_wechsle(args)`
**功能**：StarShell 命令 cd/wechsle：切换当前目录。
**示例**：
```starc
datei_wechsle("D:/ordner")
```

### `datei_wo()`
**功能**：StarShell 命令 pwd/wo：显示当前目录。
**示例**：
```starc
datei_wo()
```

### `decode_bf16(buf, off)`
**功能**：读 2 字节 BF16 并左移为 fp32 位模式。
**示例**：
```starc
f = decode_bf16(buf, off)
```

### `decode_f16(buf, off)`
**功能**：读 2 字节 F16 转 fp32 位模式（半精度转单精度）。
**示例**：
```starc
f = decode_f16(buf, off)
```

### `decode_fp8e4m3(buf, off)`
**功能**：读 1 字节 FP8_E4M3 转 fp32 位模式。
**示例**：
```starc
f = decode_fp8e4m3(buf, off)
```

### `deconv3d_forward(in, w, out, Ci, Di, Hi, Wi, Co, KD, KH, KW)`
**功能**：3D 转置卷积前向（stride=1 无 pad）。
**示例**：
```starc
deconv3d_forward(in, w, out, 4, 4,4,4, 8, 3,3,3)
```

### `deconv3d_s2_backward(dout, fwd, in, w, din, dw, dbias, Ci, Di, Hi, Wi, Co)`
**功能**：stride-2 转置卷积反向，算输入/权重/偏置梯度。
**示例**：
```starc
deconv3d_s2_backward(dout, fwd, in, w, din, dw, dbias, 8,4,4,4, 16)
```

### `deconv3d_s2_relu(in, w, bias, out, Ci, Di, Hi, Wi, Co)`
**功能**：3D 反卷积 stride=2 核 3³ pad=1 + ReLU，输出×2。
**示例**：
```starc
deconv3d_s2_relu(in, w, bias, out, 8, 4,4,4, 16)
```

### `delete_file(path)`
**功能**：删除指定文件（DeleteFileA），返回 0。
**示例**：
```starc
delete_file("D:/x.txt")
```

### `dir_create(pfad)`
**功能**：创建目录（CreateDirectoryA），成功返回非 0。
**示例**：
```starc
r = dir_create("D:/neu")
```

### `dir_cur()`
**功能**：返回当前工作目录（StarC 字符串）。
**示例**：
```starc
d = dir_cur()
```

### `dir_list(pattern)`
**功能**：列出 pattern 匹配项（如 "D:/dir/*"），逐行输出。
**示例**：
```starc
dir_list("D:/dir/*")
```

### `dir_remove(pfad)`
**功能**：删除空目录（RemoveDirectoryA），成功返回非 0。
**示例**：
```starc
r = dir_remove("D:/alt")
```

### `dir_set(dir)`
**功能**：切换当前目录（SetCurrentDirectoryA），成功返回非 0。
**示例**：
```starc
r = dir_set("D:/ordner")
```

### `dl_auth_header(token_c)`
**功能**：构造 "Authorization: Bearer <token>\r\n" 宽串，返回指针。
**示例**：
```starc
hdr = dl_auth_header(token_c)
```

### `dl_get(host, port, path, outbuf, cap, olen)`
**功能**：HTTPS GET 到 outbuf（cap 容量），字节数写 olen。
**示例**：
```starc
r = dl_get("huggingface.co", 443, "/p.bin", out, 65536, olen)
```

### `dl_get_file(host, port, path, name_c, auth_c)`
**功能**：HTTPS GET 流式写盘到文件，返回写入字节数。
**示例**：
```starc
n = dl_get_file("huggingface.co", 443, "/p.bin", "p.bin", 0)
```

### `dl_get_wi(host, port, path, outbuf, cap, olen)`
**功能**：WinINet 版 HTTPS/HTTP GET 到 outbuf，字节数写 olen。
**示例**：
```starc
r = dl_get_wi("example.com", 80, "/", out, 4096, olen)
```

### `dreifach(n)`
**功能**：示例数学函数：返回 n*3。
**示例**：
```starc
r = dreifach(7)  // 21
```

### `e4m3_bits(b)`
**功能**：E4M3 字节转 fp32 位模式（正确舍入版）。
**示例**：
```starc
f = e4m3_bits(b)
```

### `e4m3_build_lut(lutbuf)`
**功能**：构建 256 项 E4M3→fp32 LUT 到 lutbuf（每项 4 字节）。
**示例**：
```starc
e4m3_build_lut(lutbuf)
```

### `e4m3_lut_bits(b)`
**功能**：查 E4M3 LUT 得 fp32 位模式。
**示例**：
```starc
f = e4m3_lut_bits(b)
```

### `e8m0_exp(b)`
**功能**：E8M0 缩放字节 b 转 2 的指数（b-127）。
**示例**：
```starc
e = e8m0_exp(130)  // 3
```

### `edge_len(x1, y1, z1, x2, y2, z2)`
**功能**：计算两点间欧氏距离（f32）。
**示例**：
```starc
d = edge_len(0,0,0, 3,4,0)  // 5.0
```

### `emit_comment(out, off, s)`
**功能**：写注释行 "; ..." 到输出缓冲，返回新 off。
**示例**：
```starc
off = emit_comment(out, off, "start")
```

### `emit_instr(out, off, op, operand)`
**功能**：写 "指令 操作数" 一行，返回新 off。
**示例**：
```starc
off = emit_instr(out, off, "mov eax", 5)
```

### `emit_int(out, off, num)`
**功能**：写十进制数字到 out[off..]，返回新 off。
**示例**：
```starc
off = emit_int(out, off, 42)
```

### `emit_line(out, off, s)`
**功能**：写一行字符串加换行，返回新 off。
**示例**：
```starc
off = emit_line(out, off, "add eax, ebx")
```

### `emit_nl(out, off)`
**功能**：写换行 \r\n，返回新 off。
**示例**：
```starc
off = emit_nl(out, off)
```

### `emit_str(out, off, s)`
**功能**：写字符串数据到 out[off..]，返回新 off。
**示例**：
```starc
off = emit_str(out, off, "push eax")
```

### `eng_argmax(logits, vocab)`
**功能**：在 logits 上取最大值的下标（argmax）。
**示例**：
```starc
idx = eng_argmax(logits, 32000)
```

### `eng_attn(eng, q, pos, dim, n_heads, kv, out)`
**功能**：GQA 注意力单 token：q 已 RoPE，从 KV 缓存算输出。
**示例**：
```starc
eng_attn(eng, q, pos, 4096, 32, 8, out)
```

### `eng_disk(op, args)`
**功能**：StarShell 磁盘命令分发（liste 等）。
**示例**：
```starc
eng_disk("liste", "")
```

### `eng_disk_frei_mb(la)`
**功能**：取磁盘可用空间 MB（GetDiskFreeSpaceExA）。
**示例**：
```starc
f = eng_disk_frei_mb("D:/")
```

### `eng_disk_gesamt_mb(la)`
**功能**：取磁盘总空间 MB。
**示例**：
```starc
t = eng_disk_gesamt_mb("D:/")
```

### `eng_disk_liste()`
**功能**：枚举并输出所有逻辑盘符。
**示例**：
```starc
eng_disk_liste()
```

### `eng_disk_typname(t)`
**功能**：磁盘类型代码转名称文本（StarC 字符串）。
**示例**：
```starc
s = eng_disk_typname(2)
```

### `eng_forward(eng, token, pos, logits)`
**功能**：推理前向单 token：embedding+层+LM 头，写 logits。
**示例**：
```starc
eng_forward(eng, token_id, pos, logits)
```

### `eng_init(eng, path_c)`
**功能**：引擎初始化：mmap 模型+读头+转 FP64+预计算+分配 KV。
**示例**：
```starc
r = eng_init(eng, "model.bin")
```

### `eng_mathe(op, args)`
**功能**：StarShell 数学命令分发（add 等）。
**示例**：
```starc
eng_mathe("add", "3 4")
```

### `eng_matvec(w, M, N, x, y)`
**功能**：矩阵向量乘 y[m]=sum w[m*N+n]*x[n]（FP64）。
**示例**：
```starc
eng_matvec(w, M, N, x, y)
```

### `eng_matvec_b(w, b, M, N, x, y)`
**功能**：矩阵向量乘加偏置 y=w*x+b（FP64）。
**示例**：
```starc
eng_matvec_b(w, b, M, N, x, y)
```

### `eng_prozess(op, args)`
**功能**：StarShell 进程命令分发（liste 等）。
**示例**：
```starc
eng_prozess("liste", "")
```

### `eng_prozess_existiert(pid)`
**功能**：判断指定 PID 进程是否存在并输出 ja/nein。
**示例**：
```starc
eng_prozess_existiert(1234)
```

### `eng_prozess_kill(pid)`
**功能**：结束指定 PID 进程。
**示例**：
```starc
eng_prozess_kill(1234)
```

### `eng_prozess_liste()`
**功能**：枚举并输出所有进程（PID+名称）。
**示例**：
```starc
eng_prozess_liste()
```

### `eng_prozess_starte(args)`
**功能**：原生启动程序（CreateProcessA，不经 cmd）。
**示例**：
```starc
eng_prozess_starte("notepad.exe")
```

### `eng_prozess_zaehlen()`
**功能**：统计当前进程数量。
**示例**：
```starc
n = eng_prozess_zaehlen()
```

### `eng_rmsnorm(x, w, hidden, eps, out)`
**功能**：RMSNorm：out[i]=x[i]*r*w[i]，r=1/sqrt(mean+eps)。
**示例**：
```starc
eng_rmsnorm(x, w, hidden, eps, out)
```

### `eng_rope(q, k, cos_tab, sin_tab, dim, n_heads, kv)`
**功能**：对 q/k 应用 RoPE 旋转（用预计算 cos/sin 表）。
**示例**：
```starc
eng_rope(q, k, cos_tab, sin_tab, 4096, 32, 8)
```

### `eng_rope_tab(eng, pos, cos_tab, sin_tab)`
**功能**：预计算位置 pos 的 RoPE cos/sin 表（dim 长）。
**示例**：
```starc
eng_rope_tab(eng, pos, cos_tab, sin_tab)
```

### `eng_sys(op, args)`
**功能**：StarShell 系统命令分发（info 等）。
**示例**：
```starc
eng_sys("info", "")
```

### `eng_text(op, args)`
**功能**：StarShell 文本命令分发（laenge 等）。
**示例**：
```starc
eng_text("laenge", "hello")
```

### `eng_zeit(op, args)`
**功能**：StarShell 时间命令分发（jetzt 等）。
**示例**：
```starc
eng_zeit("jetzt", "")
```

### `eng_zeit_jahr()`
**功能**：取当前年份（GetLocalTime）。
**示例**：
```starc
y = eng_zeit_jahr()
```

### `eng_zeit_minute()`
**功能**：取当前分钟（0-59）。
**示例**：
```starc
m = eng_zeit_minute()
```

### `eng_zeit_monat()`
**功能**：取当前月份（1-12）。
**示例**：
```starc
mo = eng_zeit_monat()
```

### `eng_zeit_sekunde()`
**功能**：取当前秒（0-59）。
**示例**：
```starc
s = eng_zeit_sekunde()
```

### `eng_zeit_stunde()`
**功能**：取当前小时（0-23）。
**示例**：
```starc
h = eng_zeit_stunde()
```

### `eng_zeit_tag()`
**功能**：取当月日（1-31）。
**示例**：
```starc
d = eng_zeit_tag()
```

### `eng_zeit_wochentag()`
**功能**：取星期几（0=周日）。
**示例**：
```starc
wd = eng_zeit_wochentag()
```

### `exec_cmd(cmdline_str)`
**功能**：执行命令行并等待，返回退出码（CreateProcess）。
**示例**：
```starc
code = exec_cmd("dir")
```

### `exec_process(scmd)`
**功能**：执行命令（StarC 字符串），system()，返回退出码。
**示例**：
```starc
code = exec_process("D:/starc/starcc.exe a.starc")
```

### `exec_process_cstr(ccmd)`
**功能**：执行 C 字符串命令，system()，返回退出码。
**示例**：
```starc
code = exec_process_cstr("dir /w")
```

### `exec_process_nativ(cmd)`
**功能**：原生执行 C 串命令（CreateProcessA），返回退出码。
**示例**：
```starc
code = exec_process_nativ("notepad.exe")
```

### `expr_compile(tokens, n, out, off)`
**功能**：token 序列编译为 x86 汇编（mov/op 模式）。
**示例**：
```starc
off = expr_compile(tokens, n, out, 0)
```

### `extract_all_links(html)`
**功能**：提取 HTML 中所有 href 链接。
**示例**：
```starc
links = extract_all_links(html)
```

### `extract_arg(c, start)`
**功能**：从 C 字符串 start 取下一个参数（StarC 串）。
**示例**：
```starc
arg = extract_arg(c, 5)
```

### `extract_article_content(html, max_len)`
**功能**：提取页面正文文本（去 script/style），最多 max_len。
**示例**：
```starc
txt = extract_article_content(html, 5000)
```

### `extract_href(html)`
**功能**：提取第一个 href 链接（占位实现）。
**示例**：
```starc
h = extract_href(html)
```

### `extract_href_at(html, start)`
**功能**：从 start 处提取 href 链接（占位实现）。
**示例**：
```starc
h = extract_href_at(html, 10)
```

### `extract_images(html, max_count)`
**功能**：提取最多 max_count 个图片 src 链接。
**示例**：
```starc
imgs = extract_images(html, 10)
```

### `extract_time_info(html)`
**功能**：从页面提取日期/时间信息。
**示例**：
```starc
t = extract_time_info(html)
```

### `f32_add(a, b)`
**功能**：fp32 加法（IEEE754 round-to-nearest-even）。
**示例**：
```starc
r = f32_add(0x3F800000, 0x40000000)  // 1.0+2.0
```

### `f32_cos(a)`
**功能**：fp32 余弦（Taylor 展开，位模式输入输出）。
**示例**：
```starc
c = f32_cos(a)
```

### `f32_div(a, b)`
**功能**：fp32 除法（IEEE754）。
**示例**：
```starc
r = f32_div(a, b)
```

### `f32_exp(a)`
**功能**：fp32 自然指数 exp（纯 fp32 运算）。
**示例**：
```starc
e = f32_exp(a)
```

### `f32_gt(a, b)`
**功能**：有符号 fp32 大于比较（修正位模式无符号 bug）。
**示例**：
```starc
r = f32_gt(a, b)  // 1 若 a>b
```

### `f32_ln(a)`
**功能**：fp32 自然对数 ln（atanh 级数）。
**示例**：
```starc
l = f32_ln(a)
```

### `f32_lt(a, b)`
**功能**：有符号 fp32 小于比较，a<b 等价 f32_gt(b,a)。
**示例**：
```starc
r = f32_lt(a, b)
```

### `f32_mul(a, b)`
**功能**：fp32 乘法（IEEE754，符号异或修正）。
**示例**：
```starc
r = f32_mul(a, b)
```

### `f32_pow(b, n)`
**功能**：fp32 幂 b^n（n 整数，用 exp/ln）。
**示例**：
```starc
r = f32_pow(2.0, 3)  // 8.0
```

### `f32_round_halfaway(t)`
**功能**：fp32 向最近整数舍入（半值远离零）。
**示例**：
```starc
r = f32_round_halfaway(2.5)
```

### `f32_scale2(a, n)`
**功能**：fp32 乘 2^n（指数平移，快速）。
**示例**：
```starc
r = f32_scale2(a, 3)
```

### `f32_silu(a)`
**功能**：fp32 SiLU 激活 a/(1+exp(-a))。
**示例**：
```starc
r = f32_silu(a)
```

### `f32_sin(a)`
**功能**：fp32 正弦（Taylor 展开，位模式）。
**示例**：
```starc
s = f32_sin(a)
```

### `f32_softplus(a)`
**功能**：fp32 softplus ln(1+exp(a))。
**示例**：
```starc
r = f32_softplus(a)
```

### `f32_sqrt(a)`
**功能**：fp32 平方根（整数精确，IEEE 正确舍入）。
**示例**：
```starc
r = f32_sqrt(4.0)  // 2.0
```

### `f32_sqrtsoftplus(a)`
**功能**：fp32 sqrt(softplus(a)) 便捷包装。
**示例**：
```starc
r = f32_sqrtsoftplus(a)
```

### `f32_sub(a, b)`
**功能**：fp32 减法，等价 f32_add(a, -b)。
**示例**：
```starc
r = f32_sub(a, b)
```

### `f32_to_f64(src, soff, tmp)`
**功能**：fp32（src+soff）转 fp64 位模式（用 tmp 8 字节）。
**示例**：
```starc
f32_to_f64(src, soff, tmp)
```

### `f32_to_s32(a)`
**功能**：fp32 转有符号 32 位整数（向零截断）。
**示例**：
```starc
i = f32_to_s32(a)
```

### `fannkuch_run(n)`
**功能**：fannkuch 基准：n 元素排列翻转计数。
**示例**：
```starc
fannkuch_run(10)
```

### `file_append(path, data)`
**功能**：追加写 StarC 字符串到文件，成功返回非 0。
**示例**：
```starc
r = file_append("D:/x.txt", "text")
```

### `file_attrib(pfad)`
**功能**：取文件属性位掩码，0xFFFFFFFF 表示未找到。
**示例**：
```starc
attr = file_attrib("D:/x.txt")
```

### `file_close(h)`
**功能**：关闭文件句柄（CloseHandle），返回 0。
**示例**：
```starc
file_close(h)
```

### `file_copy(quelle, ziel)`
**功能**：复制文件（CopyFileA），成功返回非 0。
**示例**：
```starc
r = file_copy("D:/a.txt", "D:/b.txt")
```

### `file_delete(path)`
**功能**：删除文件（DeleteFileA），成功返回非 0。
**示例**：
```starc
r = file_delete("D:/x.txt")
```

### `file_exists(path)`
**功能**：检查文件是否存在，存在返回 1，否则 0。
**示例**：
```starc
ok = file_exists("D:/x.txt")
```

### `file_groesse(pfad)`
**功能**：取文件大小字节数，失败返回 -1。
**示例**：
```starc
sz = file_groesse("D:/x.txt")
```

### `file_mmap(path, out_map)`
**功能**：零拷贝只读 mmap 整个文件，结构写入 out_map。
**示例**：
```starc
r = file_mmap("D:/model.bin", out_map)
```

### `file_move(quelle, ziel)`
**功能**：移动或重命名文件（MoveFileA），成功返回非 0。
**示例**：
```starc
r = file_move("D:/a.txt", "D:/b.txt")
```

### `file_munmap(out_map)`
**功能**：解除 mmap 映射并关闭句柄，返回 0。
**示例**：
```starc
file_munmap(out_map)
```

### `file_open(path, access, creation)`
**功能**：打开文件返回句柄，失败返回 -1。
**示例**：
```starc
h = file_open("D:/x.txt", 0x80000000, 3)
```

### `file_open_sh(path, access, creation)`
**功能**：打开文件并允许共享读（写/读并行场景）。
**示例**：
```starc
h = file_open_sh("D:/x.bin", 0x80000000, 3)
```

### `file_read(h, buf, len)`
**功能**：从当前偏移读 len 字节到 buf，返回实际字节数。
**示例**：
```starc
n = file_read(h, buf, 4096)
```

### `file_read_at(h, buf, offset, len)`
**功能**：从指定偏移读 len 字节到 buf（定位+读）。
**示例**：
```starc
n = file_read_at(h, buf, 0, 4096)
```

### `file_run(buf, n)`
**功能**：文件处理基准：哈希词频统计主循环。
**示例**：
```starc
file_run(buf, n)
```

### `file_size(handle)`
**功能**：取文件句柄对应文件大小（GetFileSize）。
**示例**：
```starc
sz = file_size(handle)
```

### `file_touch(pfad)`
**功能**：创建空文件或更新时间戳（OPEN_ALWAYS）。
**示例**：
```starc
file_touch("D:/x.txt")
```

### `file_write(path, buf, len)`
**功能**：写 buf[0..len) 到文件，成功返回 1。
**示例**：
```starc
r = file_write("D:/x.bin", buf, 4096)
```

### `file_write_at(h, buf, offset, len)`
**功能**：向句柄指定偏移写数据（定位+写），成功返回 1。
**示例**：
```starc
r = file_write_at(h, buf, 0, 4096)
```

### `file_write_test()`
**功能**：测试写文件（写 "OK" 到 D:/starc_test.txt），返回字节数。
**示例**：
```starc
n = file_write_test()
```

### `fio_close(h)`
**功能**：关闭文件句柄（benchmark 封装），返回 0。
**示例**：
```starc
fio_close(h)
```

### `fio_open(path_c)`
**功能**：只读共享打开文件，返回句柄（>0 成功）。
**示例**：
```starc
h = fio_open("D:/x.bin")
```

### `fio_read_all(h, size)`
**功能**：读整个文件到新缓冲（补 NUL），失败返回 0。
**示例**：
```starc
buf = fio_read_all(h, size)
```

### `fio_read_f64(buf, off)`
**功能**：读 8 字节小端 double 位模式（返回 64 位整数）。
**示例**：
```starc
v = fio_read_f64(buf, off)
```

### `fio_read_i32(buf, off)`
**功能**：读 4 字节小端有符号 int32。
**示例**：
```starc
v = fio_read_i32(buf, off)
```

### `fio_read_u64(buf, off)`
**功能**：读 8 字节小端无符号 u64。
**示例**：
```starc
v = fio_read_u64(buf, off)
```

### `fio_size(h)`
**功能**：取文件大小（GetFileSizeEx），失败返回 -1。
**示例**：
```starc
sz = fio_size(h)
```

### `fp32_mul_pow2(bits, k)`
**功能**：fp32 位模式乘 2^k（只改指数字段）。
**示例**：
```starc
r = fp32_mul_pow2(0x3F800000, 3)  // 1.0*8
```

### `fp32gemv_ptx(out, off)`
**功能**：发射 PTX fp32gemv 内核：out[m]=sum_k act[k]*w[m,k]。
**示例**：
```starc
off = fp32gemv_ptx(out, 0)
```

### `fp4_dequant_byte(b, k, out, off)`
**功能**：反量化一个字节的 2 个 FP4 元素，各乘 2^k 写 fp32。
**示例**：
```starc
fp4_dequant_byte(b, k, out, off)
```

### `fp4_dequant_row(wbase, swbase, n, out, outoff)`
**功能**：反量化一行 n 个 FP4 元素（n 偶），写 fp32 输出。
**示例**：
```starc
fp4_dequant_row(wbase, swbase, n, out, outoff)
```

### `fp4_lut_bits(nib)`
**功能**：FP4 E2M1 LUT：索引转 fp32 位模式（0-7 正 8-15 负）。
**示例**：
```starc
f = fp4_lut_bits(nib)
```

### `fp4_nibble_high(b)`
**功能**：取字节高 4 位（0-15）。
**示例**：
```starc
nib = fp4_nibble_high(b)
```

### `fp4_nibble_low(b)`
**功能**：取字节低 4 位（0-15）。
**示例**：
```starc
nib = fp4_nibble_low(b)
```

### `fp4deq_ptx(out, off)`
**功能**：发射 PTX fp4deq 内核（FP4 反量化），返回新 off。
**示例**：
```starc
off = fp4deq_ptx(out, 0)
```

### `fp4gemv_ptx(out, off)`
**功能**：发射 PTX fp4gemv 内核（FP4 量化 GEMV），返回新 off。
**示例**：
```starc
off = fp4gemv_ptx(out, 0)
```

### `fp8_dequant_row(wbase, scbase, m, ncb, n, out, outoff)`
**功能**：反量化一行 n 个 FP8 元素（128 块缩放），写 fp32。
**示例**：
```starc
fp8_dequant_row(wbase, scbase, m, ncb, n, out, outoff)
```

### `fp8gemv_ptx(out, off)`
**功能**：发射 PTX fp8gemv 内核（FP8 量化 GEMV），返回新 off。
**示例**：
```starc
off = fp8gemv_ptx(out, 0)
```

### `fp_abs(a)`
**功能**：浮点绝对值（fabs 包装）。
**示例**：
```starc
r = fp_abs(a)
```

### `fp_add(a, b)`
**功能**：浮点加法（fadd 包装）。
**示例**：
```starc
r = fp_add(a, b)
```

### `fp_cmp(a, b)`
**功能**：浮点三路比较，返回 -1/0/1。
**示例**：
```starc
r = fp_cmp(a, b)
```

### `fp_div(a, b)`
**功能**：浮点除法（fdiv 包装）。
**示例**：
```starc
r = fp_div(a, b)
```

### `fp_floor(f)`
**功能**：浮点向下取整（转整再转浮）。
**示例**：
```starc
r = fp_floor(f)
```

### `fp_from_int(num)`
**功能**：整数转浮点（i2f 包装）。
**示例**：
```starc
f = fp_from_int(5)
```

### `fp_make(num, den)`
**功能**：由整数 num/den 构造浮点，den 默认 1。
**示例**：
```starc
f = fp_make(3, 4)  // 0.75
```

### `fp_mul(a, b)`
**功能**：浮点乘法（fmul 包装）。
**示例**：
```starc
r = fp_mul(a, b)
```

### `fp_neg(a)`
**功能**：浮点取负（fneg 包装）。
**示例**：
```starc
r = fp_neg(a)
```

### `fp_one()`
**功能**：返回浮点 1.0（位模式）。
**示例**：
```starc
one = fp_one()
```

### `fp_sqrt(a)`
**功能**：浮点平方根（fsqrt 包装）。
**示例**：
```starc
r = fp_sqrt(4.0)
```

### `fp_sub(a, b)`
**功能**：浮点减法（fsub 包装）。
**示例**：
```starc
r = fp_sub(a, b)
```

### `fp_to_int(f)`
**功能**：浮点转整数（f2i 包装）。
**示例**：
```starc
i = fp_to_int(3.7)  // 3
```

### `fp_zero()`
**功能**：返回浮点 0.0。
**示例**：
```starc
z = fp_zero()
```

### `fpp_hash(buf, ws, we)`
**功能**：对 [ws,we) 字节计算 31 哈希并取模 16384。
**示例**：
```starc
h = fpp_hash(buf, ws, we)
```

### `fpp_lex_less(tab, s1, s2)`
**功能**：字典序比较两个词槽，s1 小于 s2 返回 1。
**示例**：
```starc
r = fpp_lex_less(tab, s1, s2)
```

### `fpp_occupied(tab, slot)`
**功能**：判断哈希槽是否已占用（count!=0）。
**示例**：
```starc
oc = fpp_occupied(tab, slot)
```

### `fpp_word_eq(tab, slot, buf, ws, we)`
**功能**：比较槽内词与 [ws,we) 字节是否相等。
**示例**：
```starc
eq = fpp_word_eq(tab, slot, buf, ws, we)
```

### `from_cstr(cstr_ptr)`
**功能**：C 字符串转 StarC 字符串（拷贝）。
**示例**：
```starc
s = from_cstr(ptr)
```

### `fs_abs(b)`
**功能**：fp32 位模式取绝对值（清符号位）。
**示例**：
```starc
a = fs_abs(b)
```

### `fs_exp(b)`
**功能**：取 fp32 位模式的指数字段（0-255）。
**示例**：
```starc
e = fs_exp(b)
```

### `fs_iszero(b)`
**功能**：判断 fp32 位模式是否为 ±0。
**示例**：
```starc
z = fs_iszero(b)
```

### `fs_mant(b)`
**功能**：取 fp32 位模式的尾数字段。
**示例**：
```starc
m = fs_mant(b)
```

### `fs_neg(b)`
**功能**：fp32 位模式取负（翻转符号位）。
**示例**：
```starc
n = fs_neg(b)
```

### `fs_pack(s, e, m)`
**功能**：由符号/指数/尾数合成 fp32 位模式。
**示例**：
```starc
bits = fs_pack(0, 127, 0)  // 1.0
```

### `fs_sign(b)`
**功能**：取 fp32 位模式的符号位（0 或 1）。
**示例**：
```starc
s = fs_sign(b)
```

### `gbk_to_utf8(starc_str)`
**功能**：GBK 编码字符串转 UTF-8（MultiByteToWideChar）。
**示例**：
```starc
u = gbk_to_utf8(s)
```

### `ge64(a, b)`
**功能**：64 位非负比较 a>=b（分段规避 32 位比较 bug）。
**示例**：
```starc
r = ge64(a, b)
```

### `gemv32_ptx(out, off)`
**功能**：发射 PTX gemv32 内核（32 位 GEMV），返回新 off。
**示例**：
```starc
off = gemv32_ptx(out, 0)
```

### `gemv_bf16(wbase, act, out, M, K, mod, func)`
**功能**：BF16 GEMV：上传权重/激活并启动内核。
**示例**：
```starc
gemv_bf16(wbase, act, out, M, K, mod, func)
```

### `gemv_bf16_head(wbase, act, out, M, K, mod, func, cache, scratch)`
**功能**：头 GEMV：权重持久化缓存（仅首次上传）。
**示例**：
```starc
gemv_bf16_head(wbase, act, out, M, K, mod, func, cache, scratch)
```

### `gemv_bf16p(wbase, act, out, M, K, mod, func, cache, cap, scratch)`
**功能**：常驻版 BF16 GEMV，权重复用 act/out 池化。
**示例**：
```starc
gemv_bf16p(wbase, act, out, M, K, mod, func, cache, cap, scratch)
```

### `gemv_fp4(wbase, sbase, act, out, M, K, ncb32, gpu_lut, mod, func, scratch, cache, cap)`
**功能**：FP4 量化 GEMV 常驻缓存版（含权重/缩放上传）。
**示例**：
```starc
gemv_fp4(wbase, sbase, act, out, M, K, ncb32, gpu_lut, mod, func, scratch, cache, cap)
```

### `gemv_fp8(wbase, sbase, act, out, M, K, ncb, gpu_lut, mod, func, scratch, cache, cap)`
**功能**：FP8 量化 GEMV 常驻缓存版（含权重/缩放上传）。
**示例**：
```starc
gemv_fp8(wbase, sbase, act, out, M, K, ncb, gpu_lut, mod, func, scratch, cache, cap)
```

### `get_arg_at(cmdline, index)`
**功能**：从命令行提取第 index 个参数（StarC 串）。
**示例**：
```starc
arg = get_arg_at(cmdline, 2)
```

### `get_cmdline()`
**功能**：取完整命令行 C 字符串指针。
**示例**：
```starc
p = get_cmdline()
```

### `get_cmdline_cstr()`
**功能**：取命令行 C 字符串（GetCommandLineA）。
**示例**：
```starc
p = get_cmdline_cstr()
```

### `get_filename(buffer)`
**功能**：从 WIN32_FIND_DATAA 缓冲提取文件名（offset 44）。
**示例**：
```starc
name = get_filename(buffer)
```

### `get_stdout()`
**功能**：取标准输出句柄（GetStdHandle(-11)）。
**示例**：
```starc
h = get_stdout()
```

### `glb_acc_bbox(fb, js, js_end, acc_idx, out)`
**功能**：取 accessor 的 min/max 包围盒到 out，成功返回 1。
**示例**：
```starc
r = glb_acc_bbox(fb, js, js_end, acc_idx, out)
```

### `glb_get_accessor(fb, js, js_end, acc_idx, out)`
**功能**：提取 accessor 元数据（bufferView/type/count 等）。
**示例**：
```starc
r = glb_get_accessor(fb, js, js_end, acc_idx, out)
```

### `glb_get_bv(fb, js, js_end, bv_idx, out)`
**功能**：提取 bufferView 元数据（offset/length/stride）。
**示例**：
```starc
r = glb_get_bv(fb, js, js_end, bv_idx, out)
```

### `glb_global_bbox(fb, js, js_end, out)`
**功能**：累积全部 POSITION 的全局包围盒到 out。
**示例**：
```starc
r = glb_global_bbox(fb, js, js_end, out)
```

### `glb_mesh_prims(fb, js, js_end, mesh_idx, primarr_pos)`
**功能**：取 mesh 的 primitives 数组位置与数量。
**示例**：
```starc
n = glb_mesh_prims(fb, js, js_end, mesh_idx, primarr_pos)
```

### `glb_normalize_global(verts, nV, gb)`
**功能**：用包围盒把顶点归一化到 [-0.45,0.45]³。
**示例**：
```starc
glb_normalize_global(verts, nV, gb)
```

### `glb_num_meshes(fb, js, js_end)`
**功能**：取 meshes 数组数量，失败返回 -1。
**示例**：
```starc
n = glb_num_meshes(fb, js, js_end)
```

### `glb_parse_header(fb, out)`
**功能**：解析 GLB 头（JSON/bin 偏移长度等），成功返回 1。
**示例**：
```starc
r = glb_parse_header(fb, out)
```

### `glb_prim_pos(fb, el, js_end, out)`
**功能**：从 primitive 取 POSITION/indices accessor。
**示例**：
```starc
r = glb_prim_pos(fb, el, js_end, out)
```

### `glb_read_idx(fb, base, i, isz)`
**功能**：读索引（isz 为 1/2/4 字节）。
**示例**：
```starc
idx = glb_read_idx(fb, base, i, 4)
```

### `glb_to_voxel(path, vox, V, samples, fill, stats)`
**功能**：完整 GLB→体素化流程，写 stats 统计，成功返回 1。
**示例**：
```starc
r = glb_to_voxel("m.glb", vox, 64, 16, 1, stats)
```

### `glb_to_voxel_ws(path, vox, V, fill, stats, verts, maxV, visited, queue)`
**功能**：批处理版 GLB→体素化（复用预分配工作区）。
**示例**：
```starc
r = glb_to_voxel_ws(path, vox, V, fill, stats, verts, maxV, visited, queue)
```

### `glb_voxelize_prim(fb, js, js_end, bin_data, prim_el, vox, V, samples, stats)`
**功能**：体素化单个 primitive，成功返回 1。
**示例**：
```starc
r = glb_voxelize_prim(fb, js, js_end, bin, prim_el, vox, 64, 16, stats)
```

### `glb_voxelize_prim_ws(fb, js, js_end, bin_data, prim_el, vox, V, stats, verts, maxV, gb)`
**功能**：ws 版体素化单 primitive（预分配 verts 缓冲）。
**示例**：
```starc
r = glb_voxelize_prim_ws(fb, js, js_end, bin, prim_el, vox, V, stats, verts, maxV, gb)
```

### `glb_ws_alloc(V, maxV)`
**功能**：一次性分配体素化工作区（verts/visited/queue）。
**示例**：
```starc
ws = glb_ws_alloc(64, 100000)
```

### `has_str(str, sub)`
**功能**：判断字符串是否包含子串，返回 1/0。
**示例**：
```starc
ok = has_str("hello", "ell")  // 1
```

### `hash_words(buf, n, elem)`
**功能**：对 n 个 word（4/8 字节）算 CHECK 哈希打包。
**示例**：
```starc
h = hash_words(buf, n, 8)
```

### `hashmap_run(buf)`
**功能**：hashmap 基准：百万计数写入与查询。
**示例**：
```starc
hashmap_run(buf)
```

### `hc_head_apply(x4, hc_head_fn, hc_head_base, hc_head_scale, y, mod32, func32, scratch)`
**功能**：头部混合通道（4 通道→4 门→y[4096]）GPU 计算。
**示例**：
```starc
hc_head_apply(x4, fn, base, scale, y, mod, func, scratch)
```

### `hex_digit_val(ch)`
**功能**：十六进制字符转数值（0-15）。
**示例**：
```starc
v = hex_digit_val(65)  // 10
```

### `hi32(v)`
**功能**：取 64 位非负值的高 32 位。
**示例**：
```starc
h = hi32(v)
```

### `http_fetch(url, output_path)`
**功能**：用 curl 抓取 URL 存文件，成功返回 0。
**示例**：
```starc
r = http_fetch("https://x.com", "D:/o.html")
```

### `http_get(url)`
**功能**：用 curl 抓取 URL 返回响应体字符串。
**示例**：
```starc
body = http_get("https://x.com")
```

### `http_get_status(url)`
**功能**：curl GET 返回 HTTP 状态码。
**示例**：
```starc
code = http_get_status("https://x.com")
```

### `http_image_search(image_path, output_path)`
**功能**：百度识图图片搜索请求（curl POST）。
**示例**：
```starc
r = http_image_search("D:/a.jpg", "D:/res.html")
```

### `i2s(num)`
**功能**：整数转字符串（StarC 串）。
**示例**：
```starc
s = i2s(42)  // "42"
```

### `init_file_search()`
**功能**：分配 468 字节 WIN32_FIND_DATAA 缓冲并清零。
**示例**：
```starc
buf = init_file_search()
```

### `int_to_str(num)`
**功能**：整数转 StarC 字符串（支持负数）。
**示例**：
```starc
s = int_to_str(-7)  // "-7"
```

### `ipv4_set(addr, ip)`
**功能**：点分十进制 IP 解析进缓冲字节 4-7（网络序）。
**示例**：
```starc
r = ipv4_set(addr, "192.168.1.1")
```

### `is_alpha(ch)`
**功能**：判断字符是否为字母 A-Z/a-z。
**示例**：
```starc
r = is_alpha(65)  // 1
```

### `is_digit(ch)`
**功能**：判断字符是否为数字 0-9。
**示例**：
```starc
r = is_digit(53)  // 1
```

### `is_dir(buffer)`
**功能**：判断目录项是否为目录（属性位 16）。
**示例**：
```starc
d = is_dir(buffer)
```

### `is_dot(name)`
**功能**：判断文件名是否为 "." 或 ".."。
**示例**：
```starc
d = is_dot(name)
```

### `is_kw(s, kw)`
**功能**：字符串是否等于关键字（str_eq 包装）。
**示例**：
```starc
r = is_kw("if", "if")  // 1
```

### `is_prime(v)`
**功能**：素数判定（试除到 sqrt），返回 1/0。
**示例**：
```starc
p = is_prime(17)  // 1
```

### `isneg(v)`
**功能**：有符号判定：>1000000 或 <0 视为错误/负。
**示例**：
```starc
neg = isneg(sock_ret)
```

### `ist_powershell(name)`
**功能**：命令名含连字符（PS Cmdlet 形式）返回 1。
**示例**：
```starc
r = ist_powershell("Get-Process")  // 1
```

### `jp_is_digit(c)`
**功能**：判断字符是否数字（JSON 解析用）。
**示例**：
```starc
r = jp_is_digit(c)
```

### `js_arr_count(fb, ap, end)`
**功能**：取 JSON 数组元素个数，失败返回 -1。
**示例**：
```starc
n = js_arr_count(fb, ap, end)
```

### `js_arr_elem(fb, ap, end, idx, elempos)`
**功能**：取数组第 idx 元素起始位置（写 elempos）。
**示例**：
```starc
r = js_arr_elem(fb, ap, end, idx, elempos)
```

### `js_bool(s, key)`
**功能**：取 JSON 布尔值（1/0），未找到返回 0。
**示例**：
```starc
b = js_bool(s, "ok")
```

### `js_get_num(fb, objp, end, key, posbuf)`
**功能**：对象中取整数 key，缺省返回 0。
**示例**：
```starc
n = js_get_num(fb, objp, end, "count", posbuf)
```

### `js_key(s, key)`
**功能**：顶层查找 key 的值起点偏移，0 未找到。
**示例**：
```starc
p = js_key(s, "name")
```

### `js_key_from(s, start, key)`
**功能**：从 start 起查找 key，返回值起点偏移。
**示例**：
```starc
p = js_key_from(s, 0, "name")
```

### `js_num(s, key)`
**功能**：取数值 token（C 串），未找到返回 0。
**示例**：
```starc
p = js_num(s, "price")
```

### `js_obj_find(fb, objp, end, key)`
**功能**：对象中查找 key 值位置，未找到返回 -1。
**示例**：
```starc
p = js_obj_find(fb, objp, end, "name")
```

### `js_parse_float(fb, p, end, posbuf)`
**功能**：解析浮点数为位模式（写 posbuf）。
**示例**：
```starc
v = js_parse_float(fb, p, end, posbuf)
```

### `js_parse_int(fb, p, end, posbuf)`
**功能**：解析 JSON 整数，写新位置到 posbuf，返回数值。
**示例**：
```starc
n = js_parse_int(fb, p, end, posbuf)
```

### `js_raw(s, key)`
**功能**：取 value token 的 C 串（裸指针 NUL 结尾）。
**示例**：
```starc
p = js_raw(s, "name")
```

### `js_skip_comma(fb, p, end)`
**功能**：跳空白 + 可选逗号 + 空白（数组元素间）。
**示例**：
```starc
p = js_skip_comma(fb, p, end)
```

### `js_skip_number(fb, p, end)`
**功能**：跳过 JSON 数字（整数/小数/指数）。
**示例**：
```starc
p = js_skip_number(fb, p, end)
```

### `js_skip_string(fb, p, end)`
**功能**：跳过 JSON 字符串，返回闭引号后位置。
**示例**：
```starc
p = js_skip_string(fb, p, end)
```

### `js_skip_value(fb, p, end)`
**功能**：跳过任意 JSON 值（递归下降）。
**示例**：
```starc
p = js_skip_value(fb, p, end)
```

### `js_skip_ws(fb, p, end)`
**功能**：跳过 JSON 空白（空格/tab/换行/回车）。
**示例**：
```starc
p = js_skip_ws(fb, p, end)
```

### `js_str(s, key)`
**功能**：取字符串值（去引号）返回 C 串，未找到返回 0。
**示例**：
```starc
p = js_str(s, "name")
```

### `js_str_eq_at(fb, p, end, key)`
**功能**：JSON 字符串是否等于 StarC 串 key，返回 1/0。
**示例**：
```starc
eq = js_str_eq_at(fb, p, end, "type")
```

### `js_tensor_offsets(s, tname, o0, o1)`
**功能**：提取 safetensors 张量 data_offsets[0]/[1]。
**示例**：
```starc
r = js_tensor_offsets(s, "weight", o0, o1)
```

### `js_ws(s, i)`
**功能**：从 i 起跳过空白，返回首个非空白位置。
**示例**：
```starc
p = js_ws(s, i)
```

### `json_parse_run(buf, n)`
**功能**：JSON 解析基准：统计对象/整数/布尔/null。
**示例**：
```starc
json_parse_run(buf, n)
```

### `kernel_fattrs()`
**功能**：GetFileAttributesA("C:\") 实验（DLL 参数传递）。
**示例**：
```starc
v = kernel_fattrs()
```

### `kernel_fattrs_2()`
**功能**：绕开参数 bug：多传哑参数再读属性。
**示例**：
```starc
v = kernel_fattrs_2()
```

### `kernel_fattrs_o()`
**功能**：GetFileAttributesA("C:\") 对照实验。
**示例**：
```starc
v = kernel_fattrs_o()
```

### `kernel_halt_loop()`
**功能**：内核死循环：cli + hlt（星 OS 内核）。
**示例**：
```starc
kernel_halt_loop()
```

### `kernel_pid()`
**功能**：取当前进程 ID（GetCurrentProcessId）。
**示例**：
```starc
pid = kernel_pid()
```

### `kernel_stft()`
**功能**：带指针参数函数实验：预填再读回判断写入。
**示例**：
```starc
v = kernel_stft()
```

### `kernel_tick()`
**功能**：取系统时钟毫秒数（GetTickCount）。
**示例**：
```starc
t = kernel_tick()
```

### `kernel_tmp()`
**功能**：GetTempPathA 副作用检查，返回首字符。
**示例**：
```starc
c = kernel_tmp()
```

### `kstr_emit(out, off, s)`
**功能**：复制 StarC 字符串 s 到 out[off..]，返回新 off。
**示例**：
```starc
off = kstr_emit(out, off, ".version 7.0\n")
```

### `kstr_emit2(out, off, s)`
**功能**：字符串写入辅助（fp32 内核版）。
**示例**：
```starc
off = kstr_emit2(out, off, ".target sm_75\n")
```

### `kstr_emit3(out, off, s)`
**功能**：字符串写入辅助（attn 内核版）。
**示例**：
```starc
off = kstr_emit3(out, off, ".address_size 64\n")
```

### `kstr_emit4(out, off, s)`
**功能**：字符串写入辅助（routing 内核版）。
**示例**：
```starc
off = kstr_emit4(out, off, ".visible .entry softmax6\n")
```

### `kstr_emit5(out, off, s)`
**功能**：字符串写入辅助（ew 内核版）。
**示例**：
```starc
off = kstr_emit5(out, off, ".reg .f32 %f\n")
```

### `kw_type(s)`
**功能**：标识符转关键字类型码，非关键字返回 0。
**示例**：
```starc
t = kw_type("Wenn")  // 18
```

### `lex_all(src, toks, maxn)`
**功能**：词法分析全部 token 到预分配数组，返回个数。
**示例**：
```starc
n = lex_all(src, toks, 1000)
```

### `lex_eq_kw(src, start, len, kw)`
**功能**：比较源码片段与关键字是否相等，返回 1/0。
**示例**：
```starc
eq = lex_eq_kw(src, 0, 8, "Funktion")
```

### `lex_isidpart(c)`
**功能**：判断字符是否为标识符组成部分。
**示例**：
```starc
r = lex_isidpart(95)  // '_'
```

### `lex_isidstart(c)`
**功能**：判断字符是否为标识符起始字符。
**示例**：
```starc
r = lex_isidstart(65)  // 'A'
```

### `lex_isnum(c)`
**功能**：判断字符是否为数字 0-9。
**示例**：
```starc
r = lex_isnum(50)
```

### `lex_isspace(c)`
**功能**：判断字符是否为空白（空格/tab/LF/CR）。
**示例**：
```starc
r = lex_isspace(32)
```

### `lex_kw(src, start, len)`
**功能**：返回关键字编码（1=Funktion 等），非关键字 0。
**示例**：
```starc
k = lex_kw(src, 0, 8)
```

### `lex_tokenize(src, len, tokens, maxtok)`
**功能**：主词法：切分 token 到缓冲，返回总数（含 EOF）。
**示例**：
```starc
n = lex_tokenize(src, len, tokens, maxtok)
```

### `lib_add1(a)`
**功能**：测试库函数：返回 a+1。
**示例**：
```starc
r = lib_add1(41)  // 42
```

### `lib_mul2(a)`
**功能**：测试库函数：返回 a*2。
**示例**：
```starc
r = lib_mul2(21)  // 42
```

### `lo32(v)`
**功能**：取 64 位值的低 32 位。
**示例**：
```starc
l = lo32(v)
```

### `load_ckpt_u(path, W, B, mW, vW, mB, vB, total_w, total_b, stepbuf)`
**功能**：加载完整状态（权重+优化器）到 path.u，命中返回 1。
**示例**：
```starc
r = load_ckpt_u("m.u", W, B, mW, vW, mB, vB, tw, tb, stepbuf)
```

### `load_weights_u(path, W, B, total_w, total_b, stepbuf)`
**功能**：加载 W/B（无优化器）到 path.u，命中返回 1。
**示例**：
```starc
r = load_weights_u("m.u", W, B, tw, tb, stepbuf)
```

### `lsr(v, n)`
**功能**：整数逻辑右移 n 位（无内建 >>）。
**示例**：
```starc
r = lsr(v, 3)
```

### `m_cos(x)`
**功能**：fp64 余弦：约化到 [-pi,pi] 后泰勒。
**示例**：
```starc
c = m_cos(x)
```

### `m_exp(x)`
**功能**：fp64 指数：约化后泰勒再平方 k 次。
**示例**：
```starc
e = m_exp(x)
```

### `m_sigmoid(x)`
**功能**：fp64 sigmoid = 1/(1+exp(-x))。
**示例**：
```starc
s = m_sigmoid(x)
```

### `m_silu(x)`
**功能**：fp64 SiLU = x*sigmoid(x)。
**示例**：
```starc
s = m_silu(x)
```

### `m_sin(x)`
**功能**：fp64 正弦：约化到 [-pi,pi] 后泰勒。
**示例**：
```starc
s = m_sin(x)
```

### `m_sqrt(x)`
**功能**：fp64 平方根（fsqrt 包装）。
**示例**：
```starc
r = m_sqrt(4.0)
```

### `make_cstr(str)`
**功能**：StarC 字符串转 NUL 结尾 C 字符串（新分配）。
**示例**：
```starc
p = make_cstr("hello")
```

### `make_tree(d)`
**功能**：构造深度 d 的满二叉树（benchmark）。
**示例**：
```starc
t = make_tree(14)
```

### `mathe_add(a, b)`
**功能**：整数加法（数学库封装）。
**示例**：
```starc
r = mathe_add(3, 4)
```

### `mathe_div(a, b)`
**功能**：整数除法（数学库封装）。
**示例**：
```starc
r = mathe_div(7, 2)
```

### `mathe_eq(a, b)`
**功能**：整数相等判断，返回 1/0。
**示例**：
```starc
r = mathe_eq(3, 3)
```

### `mathe_fakultaet(n)`
**功能**：阶乘 n!（递归）。
**示例**：
```starc
r = mathe_fakultaet(5)  // 120
```

### `mathe_fib(n)`
**功能**：斐波那契第 n 项（递归）。
**示例**：
```starc
r = mathe_fib(10)
```

### `mathe_ge(a, b)`
**功能**：整数 >= 判断，返回 1/0。
**示例**：
```starc
r = mathe_ge(5, 3)
```

### `mathe_ggt(a, b)`
**功能**：最大公约数（欧几里得）。
**示例**：
```starc
r = mathe_ggt(12, 18)  // 6
```

### `mathe_gt(a, b)`
**功能**：整数 > 判断，返回 1/0。
**示例**：
```starc
r = mathe_gt(5, 3)
```

### `mathe_istprim(n)`
**功能**：素数判定（StarShell 数学）。
**示例**：
```starc
r = mathe_istprim(17)
```

### `mathe_kgv(a, b)`
**功能**：最小公倍数（用 ggt）。
**示例**：
```starc
r = mathe_kgv(4, 6)  // 12
```

### `mathe_le(a, b)`
**功能**：整数 <= 判断，返回 1/0。
**示例**：
```starc
r = mathe_le(3, 5)
```

### `mathe_lt(a, b)`
**功能**：整数 < 判断，返回 1/0。
**示例**：
```starc
r = mathe_lt(3, 5)
```

### `mathe_mul(a, b)`
**功能**：整数乘法（数学库封装）。
**示例**：
```starc
r = mathe_mul(3, 4)
```

### `mathe_ne(a, b)`
**功能**：整数不等判断，返回 1/0。
**示例**：
```starc
r = mathe_ne(3, 4)
```

### `mathe_neg(a)`
**功能**：整数取负（0-a）。
**示例**：
```starc
r = mathe_neg(5)  // -5
```

### `mathe_sub(a, b)`
**功能**：整数减法（数学库封装）。
**示例**：
```starc
r = mathe_sub(7, 3)
```

### `mathe_ziffernsumme(n)`
**功能**：整数各位数字之和。
**示例**：
```starc
r = mathe_ziffernsumme(123)  // 6
```

### `matmul_run(buf)`
**功能**：矩阵乘法基准：读入 A/B 算 C。
**示例**：
```starc
matmul_run(buf)
```

### `mem_copy(dst, src, n)`
**功能**：裸内存拷贝 n 字节。
**示例**：
```starc
mem_copy(dst, src, 4096)
```

### `mem_gesamt_mb()`
**功能**：取总物理内存 MB（GlobalMemoryStatusEx）。
**示例**：
```starc
mb = mem_gesamt_mb()
```

### `mem_verfuegbar_mb()`
**功能**：取可用物理内存 MB。
**示例**：
```starc
mb = mem_verfuegbar_mb()
```

### `mem_zero(ptr, n)`
**功能**：清零 n 字节内存。
**示例**：
```starc
mem_zero(ptr, 4096)
```

### `mf_append(path, data)`
**功能**：追加写数据到文件（FILE_APPEND_DATA）。
**示例**：
```starc
r = mf_append("D:/x.txt", "data")
```

### `mf_delete(path)`
**功能**：删除文件，成功返回非 0。
**示例**：
```starc
r = mf_delete("D:/x.txt")
```

### `mf_exists(path)`
**功能**：判断文件是否存在，返回 1/0。
**示例**：
```starc
ok = mf_exists("D:/x.txt")
```

### `mf_move(quelle, ziel)`
**功能**：移动/重命名文件，成功返回非 0。
**示例**：
```starc
r = mf_move("D:/a.txt", "D:/b.txt")
```

### `mf_read(path)`
**功能**：读整个文件为 StarC 字符串，失败返回 0。
**示例**：
```starc
s = mf_read("D:/x.txt")
```

### `mf_size(path)`
**功能**：取文件大小字节数，不存在返回 -1。
**示例**：
```starc
sz = mf_size("D:/x.txt")
```

### `mf_write(path, data)`
**功能**：覆盖写数据到文件，成功返回非 0。
**示例**：
```starc
r = mf_write("D:/x.txt", "data")
```

### `mg_accept(srv)`
**功能**：接受连接（accept），返回客户端 socket。
**示例**：
```starc
c = mg_accept(srv)
```

### `mg_cleanup()`
**功能**：WSACleanup 清理 Winsock，返回 0。
**示例**：
```starc
mg_cleanup()
```

### `mg_close(s)`
**功能**：关闭 socket（closesocket）。
**示例**：
```starc
mg_close(s)
```

### `mg_cstr(s)`
**功能**：StarC 字符串（offset 8 起）转新 C 字符串。
**示例**：
```starc
p = mg_cstr("hello")
```

### `mg_http_cl_key()`
**功能**：返回 "content-length" 常量 C 串（头比较）。
**示例**：
```starc
k = mg_http_cl_key()
```

### `mg_http_parse(buf, n, out)`
**功能**：解析 HTTP 请求行/头，写方法/路径/体偏移到 out。
**示例**：
```starc
r = mg_http_parse(buf, n, out)
```

### `mg_http_resp(code, ctype, body, out, cap)`
**功能**：组装 HTTP 响应到 out（cap 容量），返回总长度。
**示例**：
```starc
n = mg_http_resp(200, "application/json", body, out, cap)
```

### `mg_http_status_text(code)`
**功能**：HTTP 状态码转文本（200→"OK"），返回 C 串。
**示例**：
```starc
t = mg_http_status_text(200)
```

### `mg_json_quote(s)`
**功能**：C 串转 JSON 字符串字面量（含引号转义）。
**示例**：
```starc
q = mg_json_quote("a\"b")
```

### `mg_listen(port)`
**功能**：监听 0.0.0.0:port，返回 srv（<0 失败）。
**示例**：
```starc
srv = mg_listen(8080)
```

### `mg_newline()`
**功能**：输出换行符，返回 0。
**示例**：
```starc
mg_newline()
```

### `mg_path(a, b)`
**功能**：拼接两个 C 串为新 C 串（前缀+后缀）。
**示例**：
```starc
p = mg_path("D:/", "x.bin")
```

### `mg_read_file(name_c, out_size)`
**功能**：读整块文件（NUL 结尾），写大小到 out_size。
**示例**：
```starc
buf = mg_read_file("D:/x.bin", out_size)
```

### `mg_recv(s, buf, max)`
**功能**：从 socket 接收数据（recv），返回字节数。
**示例**：
```starc
n = mg_recv(s, buf, 4096)
```

### `mg_route(req, buf, out, cap)`
**功能**：HTTP 路由分发（/v1/models 等），写响应。
**示例**：
```starc
n = mg_route(req, buf, out, cap)
```

### `mg_send(s, ptr, len)`
**功能**：向 socket 发送数据（send），返回字节数。
**示例**：
```starc
n = mg_send(s, ptr, len)
```

### `mg_stdout()`
**功能**：取标准输出句柄（STD_OUTPUT_HANDLE）。
**示例**：
```starc
h = mg_stdout()
```

### `mg_utf16(s)`
**功能**：ANSI C 串转 UTF-16 宽串（含双 NUL）。
**示例**：
```starc
w = mg_utf16("hello")
```

### `mg_write_cstr(c)`
**功能**：写 C 字符串到标准输出（WriteFile）。
**示例**：
```starc
mg_write_cstr("hello")
```

### `mg_write_file(name_c, buf, len)`
**功能**：写整块数据到文件，返回 0 成功/负错误码。
**示例**：
```starc
r = mg_write_file("D:/x.bin", buf, 4096)
```

### `mg_write_int(v)`
**功能**：写整数到标准输出。
**示例**：
```starc
mg_write_int(42)
```

### `mg_write_str(s)`
**功能**：写 StarC 字符串到标准输出。
**示例**：
```starc
mg_write_str("hello")
```

### `mg_wsa()`
**功能**：WSAStartup 初始化 Winsock，返回 0 成功。
**示例**：
```starc
r = mg_wsa()
```

### `mhc_post(y, x, postw, comb, post_out, cache, scratch)`
**功能**：MHC post 投影：postw*y + comb·x 到 post_out。
**示例**：
```starc
mhc_post(y, x, postw, comb, post_out, cache, scratch)
```

### `mhc_post_ptx(out, off)`
**功能**：发射 PTX mhc_post 内核（GPU 化后处理）。
**示例**：
```starc
off = mhc_post_ptx(out, 0)
```

### `mhc_pre(x, hc_fn, hc_base, hc_scale, prew, postw, comb, y, gpu_lut, mod, func)`
**功能**：MHC pre 投影：归一化+门控+Sinkhorn，得 y。
**示例**：
```starc
mhc_pre(x, fn, base, scale, prew, postw, comb, y, lut, mod, func)
```

### `mhc_sinkhorn(comb)`
**功能**：Sinkhorn 双随机归一化（4x4，原地 fp32）。
**示例**：
```starc
mhc_sinkhorn(comb)
```

### `mhc_y_ptx(out, off)`
**功能**：发射 PTX mhc_y 内核（prew·x 输出投影）。
**示例**：
```starc
off = mhc_y_ptx(out, 0)
```

### `mlp_backward(X, H1, H2, Y_pred, Y_target, W, B, G, GB, dH2, dH1, in, hid, out)`
**功能**：MLP 反向：累加梯度到连续 G/GB 缓冲。
**示例**：
```starc
mlp_backward(X, H1, H2, Yp, Yt, W, B, G, GB, dH2, dH1, in, hid, out)
```

### `mlp_forward(X, W, B, H1, H2, Y, in, hid, out)`
**功能**：MLP 前向：3 层 relu 网络推理。
**示例**：
```starc
mlp_forward(X, W, B, H1, H2, Y, in, hid, out)
```

### `moe_forward(x, token, ctx, scratch, cache, cap)`
**功能**：MoE 前向：hash/topk 路由 + FP4 专家 + FP8 共享。
**示例**：
```starc
moe_forward(x, token, ctx, scratch, cache, cap)
```

### `moe_forward_student(x, token, ctx, scratch, cache, cap)`
**功能**：学生版 MoE 前向（同路由+专家结构）。
**示例**：
```starc
moe_forward_student(x, token, ctx, scratch, cache, cap)
```

### `multiply(a, b)`
**功能**：乘法函数（test_funcs）。
**示例**：
```starc
r = multiply(6, 7)  // 42
```

### `nb_energy(b)`
**功能**：nbody 总能量（严格左结合复现 C）。
**示例**：
```starc
e = nb_energy(b)
```

### `nb_get(b, i, k)`
**功能**：读 nbody 缓冲第 i 体第 k 分量（u64）。
**示例**：
```starc
v = nb_get(b, i, k)
```

### `nb_set(b, i, k, v)`
**功能**：写 nbody 缓冲第 i 体第 k 分量。
**示例**：
```starc
nb_set(b, i, k, v)
```

### `nbody_run(buf)`
**功能**：nbody 基准：读初值算能量。
**示例**：
```starc
nbody_run(buf)
```

### `nc_build_voxnet(points, nV, V, cellcount)`
**功能**：构建体网络空间哈希网格，返回占用单元数。
**示例**：
```starc
n = nc_build_voxnet(points, nV, 32, cellcount)
```

### `nc_count_points_faces(fb, js, js_end, out)`
**功能**：统计 GLB 点数/面数（out[0]=nV [8]=nF）。
**示例**：
```starc
r = nc_count_points_faces(fb, js, js_end, out)
```

### `nc_dedup(points, faces, nV, nF, dpoints, dfaces, remap, ht_slot, slots, out)`
**功能**：顶点去重（哈希），写新索引/面。
**示例**：
```starc
r = nc_dedup(points, faces, nV, nF, dp, df, remap, ht, slots, out)
```

### `nc_extract_points_faces(fb, js, js_end, bin_data, points, faces, maxV, maxF, stats)`
**功能**：提取点云+面云（跨 prim 顶点偏移累加）。
**示例**：
```starc
r = nc_extract_points_faces(fb, js, js_end, bin, points, faces, maxV, maxF, stats)
```

### `nc_feat(ci, occ, X, Y)`
**功能**：体网络占位转粗特征 X(64)+细目标 Y(512)。
**示例**：
```starc
nc_feat(ci, occ, X, Y)
```

### `nc_normalize_unit(verts, nV, gb)`
**功能**：网云点归一化到 [0,1)（用 bbox 线性映射）。
**示例**：
```starc
nc_normalize_unit(verts, nV, gb)
```

### `nc_sim_voxnet(ci_a, occ_a, ci_b, occ_b, V, va, vb, out)`
**功能**：网云体网络 Jaccard 相似度比较。
**示例**：
```starc
r = nc_sim_voxnet(ci_a, occ_a, ci_b, occ_b, V, va, vb, out)
```

### `net_connect(host_c, port)`
**功能**：连接 host_c:port（自研 DNS + 原始 TCP）；返回 socket（>=0）或负错误码。
**示例**：
```starc
ws = alloc(1024) ;     mem_zero(ws, 1024) ;     WSAStartup(514, ws) ;     ip = alloc(4) ;     mem_zero(ip, 4) ;     r = net_dns_query(host_c, ip) ;     Wenn r != 0 dann ;         gebe_zurueck r
```

### `net_dns_query(host_c, ipout)`
**功能**：自研 DNS：查询 host_c 的 A 记录 IPv4，写 4 字节到 ipout。
**示例**：
```starc
// UDP socket（AF_INET, SOCK_DGRAM, IPPROTO_UDP） ;     s = socket(2, 2, 17) ;     Wenn isneg(s) == 1 dann ;         gebe_zurueck 0 - 1
```

### `net_send_all(s, ptr, len)`
**功能**：发送全部字节；返回发送字节数（<0 失败）。
**示例**：
```starc
total = 0 ;     Solange total < len ;         n = send(s, ptr + total, len - total, 0) ;         Wenn isneg(n) == 1 oder n == 0 dann ;             gebe_zurueck 0 - 1
```

### `net_set_timeout(s, ms)`
**功能**：给 socket 设接收超时（毫秒）。SO_RCVTIMEO 在 Windows 为 DWORD 毫秒。
**示例**：
```starc
tv = alloc(4) ;     mem_zero(tv, 4) ;     raw_setbyte(tv, 0, ms - (ms / 256) * 256)   // 低字节 ;     raw_setbyte(tv, 1, (ms / 256) - ((ms / 65536) * 256))  // 次字节 ;     raw_setbyte(tv, 2, (ms / 65536) - ((ms / 16777216) * 256))  // 第三字节 ;     raw_setbyte(tv, 3, ms / 16777216)           // 高字节 ;     setsockopt(s, 65535, 4102, tv, 4) ;     gebe_zurueck 0
```

### `next_file(buffer)`
**功能**：next_file(buffer) - get next file, return 1 if exists。
**示例**：
```starc
h0 = raw_byteat(buffer, 460) ;     h1 = raw_byteat(buffer, 461) ;     h2 = raw_byteat(buffer, 462) ;     h3 = raw_byteat(buffer, 463) ;     h4 = raw_byteat(buffer, 464) ;     h5 = raw_byteat(buffer, 465) ;     h6 = raw_byteat(buffer, 466) ;     h7 = raw_byteat(buffer, 467) ;  ;     handle = h0 ;     handle = handle + h1 * 256 ;     handle = handle + h2 * 65536
```

### `off_next_float(buf, posbuf)`
**功能**：读取下一个 float (跳过空白, 返回 fp32 位模式, pos 写回) ----。
**示例**：
```starc
pos = raw_get64(posbuf, 0) ;     // 跳过空白 ;     Solange raw_byteat(buf, pos) == 32 oder raw_byteat(buf, pos) == 9 oder raw_byteat(buf, pos) == 10 oder raw_byteat(buf, pos) == 13 ;         pos = pos + 1
```

### `off_next_int(buf, posbuf)`
**功能**：读取下一个 int (跳过空白, pos 为 8 字节缓冲, 写回新位置) ----。
**示例**：
```starc
pos = raw_get64(posbuf, 0) ;     // 跳过空白 ;     Solange raw_byteat(buf, pos) == 32 oder raw_byteat(buf, pos) == 9 oder raw_byteat(buf, pos) == 10 oder raw_byteat(buf, pos) == 13 ;         pos = pos + 1
```

### `off_normalize(verts, nV, stbuf)`
**功能**：计算包围盒并归一化顶点到 [-0.45,0.45]^3 (留边距) ----。
**示例**：
```starc
minx = t_get(verts, 0) ;     maxx = minx ;     miny = t_get(verts, 1) ;     maxy = miny ;     minz = t_get(verts, 2) ;     maxz = minz ;     i = 1 ;     Solange i < nV ;         x = t_get(verts, i * 3 + 0) ;         y = t_get(verts, i * 3 + 1) ;         z = t_get(verts, i * 3 + 2) ;         Wenn f32_lt(x, minx) dann
```

### `off_parse_header(buf, len, posbuf, outbuf)`
**功能**：解析 OFF 头: 找 "OFF" 行, 读 nV nF nE ----。
**示例**：
```starc
pos = 0 ;     found = 0 ;     Solange pos < len und found == 0 ;         Wenn raw_byteat(buf, pos) == 79 und raw_byteat(buf, pos + 1) == 70 und raw_byteat(buf, pos + 2) == 70 dann ;             // 只前进 3 字节 (跳过 "OFF"), 让 off_next_int 从后面读 nV nF nE ;             // 兼容三种变体: "OFF 2938 ..." / "OFF\n2938 ..." / "OFF2938 ..." ;             pos = pos + 3 ;             found = 1 ;         Sonst ;             pos = pos + 1
```

### `off_read_verts(buf, posbuf, verts, nV)`
**功能**：读取顶点数组 (nV 个 [x,y,z] fp32), 存到 verts 缓冲 ----。
**示例**：
```starc
i = 0 ;     Solange i < nV ;         x = off_next_float(buf, posbuf) ;         y = off_next_float(buf, posbuf) ;         z = off_next_float(buf, posbuf) ;         t_set(verts, i * 3 + 0, x) ;         t_set(verts, i * 3 + 1, y) ;         t_set(verts, i * 3 + 2, z) ;         i = i + 1
```

### `off_to_voxel(path, vox, V, samples, fill)`
**功能**：从 OFF 文件体素化 (完整流程) ----。
**示例**：
```starc
m = alloc(32) ;     r = file_mmap(path, m) ;     Wenn r != 1 dann ;         gebe_zurueck 0
```

### `off_to_voxel_stride(path, vox, V, samples, fill, stride)`
**功能**：每 stride 面处理 1 个 (ModelNet40 原始 10 万面, V=16 体素化只需部分面) ----。
**示例**：
```starc
m = alloc(32) ;     r = file_mmap(path, m) ;     Wenn r != 1 dann ;         gebe_zurueck 0
```

### `op_prio(c)`
**功能**：运算符优先级。
**示例**：
```starc
Wenn c == 43 dann gebe_zurueck 1 Ende  // + ;     Wenn c == 45 dann gebe_zurueck 1 Ende  // - ;     Wenn c == 42 dann gebe_zurueck 2 Ende  // * ;     Wenn c == 47 dann gebe_zurueck 2 Ende  // / ;     gebe_zurueck 0
```

### `open_dir(path, buffer)`
**功能**：open_dir(path, buffer) - open directory, return 1 on success。
**示例**：
```starc
// Convert path to C string first ;     path_cstr = make_cstr(path) ;  ;     // Get length of path ;     path_len = liste_laenge(path) ;  ;     // Append "*" to the C string (ASCII 42) ;     raw_setbyte(path_cstr, path_len, 42) ;     raw_setbyte(path_cstr, path_len + 1, 0) ;  ;     handle = FindFirstFileA(path_cstr, buffer) ;
```

### `open_dir_pattern(pattern, buffer)`
**功能**：open_dir_pattern(pattern, buffer) - 直接搜索模式（如 "E:\\*foo.txt"）。
**示例**：
```starc
pattern_cstr = make_cstr(pattern) ;  ;     handle = FindFirstFileA(pattern_cstr, buffer) ;  ;     Wenn handle == -1 dann ;         raw_setbyte(buffer, 460, 0) ;         raw_setbyte(buffer, 461, 0) ;         raw_setbyte(buffer, 462, 0) ;         raw_setbyte(buffer, 463, 0) ;         raw_setbyte(buffer, 464, 0) ;         raw_setbyte(buffer, 465, 0) ;         raw_setbyte(buffer, 466, 0)
```

### `open_read(path)`
**功能**：open_read(path) - 打开文件用于读取。
**示例**：
```starc
path_cstr = make_cstr(path) ;  ;     // CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0) ;     fh = CreateFileA(path_cstr, 2147483648, 1, 0, 3, 0, 0) ;  ;     Wenn fh == 0 dann ;         gebe_zurueck 0
```

### `open_write(path)`
**功能**：open_write(path) - 创建/覆盖文件用于写入。
**示例**：
```starc
path_cstr = make_cstr(path) ;  ;     // CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0) ;     fh = CreateFileA(path_cstr, 1073741824, 2, 0, 2, 128, 0) ;  ;     Wenn fh == 0 dann ;         gebe_zurueck 0
```

### `opt_init(opts, lr, beta1, beta2, eps, wd)`
**功能**：存超参 (fp32 位模式)。
**示例**：
```starc
raw_set64(opts, 0, lr) ;     raw_set64(opts, 8, beta1) ;     raw_set64(opts, 16, beta2) ;     raw_set64(opts, 24, eps) ;     raw_set64(opts, 32, wd) ;     gebe_zurueck 0
```

### `opt_zero(buf, n)`
**功能**：零初始化 m/v 缓冲。
**示例**：
```starc
t_zero(buf, n) ;     gebe_zurueck 0
```

### `os_add_var(symtab, st, tokbuf, name_s, name_l, kind)`
**功能**：添加变量 (kind 由调用方定), 返回 sym 索引; aux 计数在 st[20](int) / st[28](array)。
**示例**：
```starc
vid = raw_get32(st, 0) ;     raw_set32(symtab, vid * 16, name_s) ;     raw_set32(symtab, vid * 16 + 4, name_l) ;     raw_set32(symtab, vid * 16 + 8, kind) ;     Wenn kind == 2 dann  // array ;         raw_set32(symtab, vid * 16 + 12, 16 + raw_get32(st, 28)) ;         raw_set32(st, 28, raw_get32(st, 28) + 1) ;     Sonst  // int (var 或 param) ;         raw_set32(symtab, vid * 16 + 12, 16 + raw_get32(st, 20)) ;         raw_set32(st, 20, raw_get32(st, 20) + 1)
```

### `os_builtin_id(tokbuf, s, l)`
**功能**：返回内置 id, 非内置 -1。
**示例**：
```starc
Wenn os_str_eq(tokbuf, s, l, "x86_cli") dann gebe_zurueck 0 Ende ;     Wenn os_str_eq(tokbuf, s, l, "x86_sti") dann gebe_zurueck 1 Ende ;     Wenn os_str_eq(tokbuf, s, l, "x86_hlt") dann gebe_zurueck 2 Ende ;     Wenn os_str_eq(tokbuf, s, l, "port_outb") dann gebe_zurueck 3 Ende ;     Wenn os_str_eq(tokbuf, s, l, "port_inb") dann gebe_zurueck 4 Ende ;     Wenn os_str_eq(tokbuf, s, l, "x86_read_cr2") dann gebe_zurueck 5 Ende ;     Wenn os_str_eq(tokbuf, s, l, "vga_putc") dann gebe_zurueck 6 Ende ;     Wenn os_str_eq(tokbuf, s, l, "vga_str") dann gebe_zurueck 7 Ende ;     Wenn os_str_eq(tokbuf, s, l, "vga_clear") dann gebe_zurueck 8 Ende ;     Wenn os_str_eq(tokbuf, s, l, "kernel_halt_loop") dann gebe_zurueck 9 Ende ;     gebe_zurueck 0 - 1
```

### `os_func_find(funcs, fcount, tokbuf, name_s, name_l)`
**功能**：返回函数 id 或 -1。
**示例**：
```starc
i = 0 ;     Solange i < fcount ;         ns = raw_get32(funcs, i * 28) ;         nl = raw_get32(funcs, i * 28 + 4) ;         Wenn nl == name_l dann ;             match = 1 ;             k = 0 ;             Solange k < name_l ;                 Wenn raw_byteat(tokbuf, ns + k) != raw_byteat(tokbuf, name_s + k) dann ;                     match = 0
```

### `os_int_op_prio(c)`
**功能**：* / = 3; + - = 2; < > <= >= = 1; und = 0。
**示例**：
```starc
Wenn c == 42 dann gebe_zurueck 3 Ende ;     Wenn c == 47 dann gebe_zurueck 3 Ende ;     Wenn c == 43 dann gebe_zurueck 2 Ende ;     Wenn c == 45 dann gebe_zurueck 2 Ende ;     Wenn c == 60 dann gebe_zurueck 1 Ende ;     Wenn c == 61 dann gebe_zurueck 1 Ende ;     Wenn c == 62 dann gebe_zurueck 1 Ende ;     Wenn c == 64 dann gebe_zurueck 1 Ende ;     Wenn c == 65 dann gebe_zurueck 1 Ende ;     gebe_zurueck 0
```

### `os_parse_block(tokens, i, tn, ast, irpn, funcs, fcount, symtab, st, stopsonst)`
**功能**：语句块解析 (递归), 遇 Ende(kw2)/EOF 结束; stopsonst=1 时也停 Sonst(kw5)。
**示例**：
```starc
stop = 0 ;     Solange i < tn und stop == 0 ;         base = i * 32 ;         t = raw_get32(tokens, base) ;         Wenn t == 6 dann ;             stop = 1 ;         Sonst ;             Wenn t == 3 und raw_get32(tokens, base + 4) == 2 dann  // Ende ;                 i = i + 1 ;                 stop = 1 ;             Sonst ;                 Wenn stopsonst == 1 und t == 3 und raw_get32(tokens, base + 4) == 5 dann
```

### `os_parse_int_expr(tokens, i, tn, irpn, symtab, st, funcs, fcount)`
**功能**：整数表达式 (Shunting-Yard), 写 irpn[st[4]..], 返回新 i; funcs/fcount 用于函数调用识别。
**示例**：
```starc
ostack = alloc(256) ;     osp = 0 ;     in = raw_get32(st, 4) ;     stop = 0 ;     Solange i < tn und stop == 0 ;         base = i * 32 ;         t = raw_get32(tokens, base) ;         Wenn t == 6 oder t == 7 dann ;             stop = 1 ;         Sonst ;             Wenn t == 3 dann ;                 Wenn raw_get32(tokens, base + 4) == 9 dann  // und
```

### `os_peek_int(tokens, i, tn, symtab, st)`
**功能**：从 i 起跳过前导 '(' 判断首个原子是否 int 表达式。
**示例**：
```starc
j = i ;     Solange j < tn und raw_byteat(tokens, j * 32 + 8) == 40 ;         j = j + 1
```

### `os_str_eq(tokbuf, s, l, kw)`
**功能**：tokbuf[s..s+l) 与 kw (StarC 字符串) 相等。
**示例**：
```starc
kl = str_len(kw) ;     Wenn kl != l dann gebe_zurueck 0 Ende ;     i = 0 ;     Solange i < l ;         Wenn raw_byteat(tokbuf, s + i) != raw_byteat(kw, 8 + i) dann ;             gebe_zurueck 0
```

### `os_var_find(symtab, symcount, tokbuf, name_s, name_l)`
**功能**：符号表查找。
**示例**：
```starc
i = 0 ;     Solange i < symcount ;         ns = raw_get32(symtab, i * 16) ;         nl = raw_get32(symtab, i * 16 + 4) ;         Wenn nl == name_l dann ;             match = 1 ;             k = 0 ;             Solange k < name_l ;                 Wenn raw_byteat(tokbuf, ns + k) != raw_byteat(tokbuf, name_s + k) dann ;                     match = 0
```

### `os_version_text()`
**功能**：------ Betriebssystemversion ----------。
**示例**：
```starc
osvi = alloc(148) ;     raw_setbyte(osvi, 0, 148) ;     raw_setbyte(osvi, 1, 0) ;     raw_setbyte(osvi, 2, 0) ;     raw_setbyte(osvi, 3, 0) ;     GetVersionExA(osvi) ;     major = raw_byteat(osvi, 4) + raw_byteat(osvi, 5) * 256 ;     minor = raw_byteat(osvi, 8) + raw_byteat(osvi, 9) * 256 ;     build = raw_byteat(osvi, 12) + raw_byteat(osvi, 13) * 256 ;     s = int_to_str(major) ;     s = str_concat(s, ".") ;     s = str_concat(s, int_to_str(minor))
```

### `p2(n)`
**功能**：（功能描述未提取）。
**示例**：
```starc
v = 1 ;     i = 0 ;     Solange i < n ;         v = v * 2 ;         i = i + 1
```

### `parse_os_program(srcbuf, slen, ast, irpn, symtab, funcs, spdata, out_info)`
**功能**：srcbuf -> AST + irpn + 函数表 + 字符串池。
**示例**：
```starc
tokens = alloc(8192 * 32) ;     tn = lex_tokenize(srcbuf, slen, tokens, 8192) ;     st = alloc(40) ;     raw_set32(st, 0, 0)   // sym_count ;     raw_set32(st, 4, 0)   // irpn_len ;     raw_set32(st, 12, 1)  // err ;     raw_set32(st, 16, 0)  // ast_count ;     raw_set32(st, 20, 0)  // int_var_count ;     raw_set32(st, 28, 0)  // array_count ;     raw_set32(st, 32, spdata) // 字符串池基址 (存入 st[32]) ;     raw_set32(st, 36, 0)  // 字符串池 off ;     raw_set32(out_info, 0, 0)
```

### `parse_program(srcbuf, slen, ast, frpn, irpn, symtab, out_info)`
**功能**：srcbuf -> AST + frpn + irpn + symtab。
**示例**：
```starc
tokens = alloc(8192 * 32) ;     tn = lex_tokenize(srcbuf, slen, tokens, 8192) ;     st = alloc(40) ;     raw_set32(st, 0, 0) ;     raw_set32(st, 4, 0) ;     raw_set32(st, 8, 0) ;     raw_set32(st, 12, 1) ;     raw_set32(st, 16, 0) ;     raw_set32(st, 20, 0) ;     raw_set32(st, 24, 0) ;     raw_set32(st, 28, 0) ;     raw_set32(out_info, 0, 0)
```

### `parse_rpn(tokens, n, rpn)`
**功能**：token 流 -> RPN, 返回 RPN 项数。
**示例**：
```starc
ostack = alloc(256) ;     osp = 0 ;     rn = 0 ;     i = 0 ;     Solange i < n ;         base = i * 32 ;         t = raw_get32(tokens, base) ;         Wenn t == 1 dann  // 数字 -> RPN 直接 ;             v = raw_get32(tokens, base + 4) ;             raw_set32(rpn, rn * 8, 1) ;             raw_set32(rpn, rn * 8 + 4, v) ;             rn = rn + 1
```

### `pfad_basename(args)`
**功能**：Basisname ohne Endung。
**示例**：
```starc
p = extract_arg(args, 0) ;     gebe_zurueck basis_name(p)
```

### `pfad_dirname(args)`
**功能**：Verzeichnisteil eines Pfades。
**示例**：
```starc
p = extract_arg(args, 0) ;     n = str_len(p) ;     i = n - 1 ;     Solange i >= 0 ;         ch = raw_byteat(p, 8 + i) ;         Wenn ch == 47 dann ;             gebe_zurueck str_slice(p, 0, i)
```

### `pfad_ext(args)`
**功能**：Endung eines Pfades (inkl. Punkt)。
**示例**：
```starc
p = extract_arg(args, 0) ;     n = str_len(p) ;     i = n - 1 ;     Solange i >= 0 ;         ch = raw_byteat(p, 8 + i) ;         Wenn ch == 47 dann ;             gebe_zurueck cstr_to_starc("")
```

### `pfad_join(args)`
**功能**：Pfade zusammenfuegen。
**示例**：
```starc
a = extract_arg(args, 0) ;     b = argument_n(args, 0, 2) ;     s = a ;     n = str_len(s) ;     Wenn n > 0 dann ;         letztes = raw_byteat(s, 8 + n - 1) ;         Wenn letztes != 47 dann ;             Wenn letztes != 92 dann ;                 s = str_concat(s, "/")
```

### `pfad_normal(args)`
**功能**：Normieren: \ -> /, doppelte trennen。
**示例**：
```starc
p = extract_arg(args, 0) ;     n = str_len(p) ;     buf = alloc(8 + n + 2) ;     pos = 0 ;     i = 0 ;     Solange i < n ;         ch = raw_byteat(p, 8 + i) ;         Wenn ch == 92 dann ;             ch = 47
```

### `pool3d_max(in, out, Ci, D, H, W)`
**功能**：3D 最大池化: 输入 (Ci,D,H,W) -> 输出 (Ci,D/2,H/2,W/2), 2x2x2 窗口 ----。
**示例**：
```starc
OD = D / 2 ;     OH = H / 2 ;     OW = W / 2 ;     ci = 0 ;     Solange ci < Ci ;         od = 0 ;         Solange od < OD ;             oh = 0 ;             Solange oh < OH ;                 ow = 0 ;                 Solange ow < OW ;                     mx = 0
```

### `port_inb(port)`
**功能**：端口输入 inb(port):  mov edx,port; in al,dx; 返回 al。
**示例**：
```starc
gebe_zurueck 0
```

### `port_outb(port, val)`
**功能**：端口输出 outb(port, val):  mov edx,port; mov al,val; out dx,al。
**示例**：
```starc
gebe_zurueck 0
```

### `pow2(e)`
**功能**：（功能描述未提取）。
**示例**：
```starc
v = 1 ;     i = 0 ;     Solange i < e ;         v = v * 2 ;         i = i + 1
```

### `powgen_ptx(out, off)`
**功能**：===== GPU pow 生成内核 (pow[i] = 2^(sbase[i]-127)) =====。
**示例**：
```starc
off = kstr_emit(out, off, ".version 7.0\n") ;     off = kstr_emit(out, off, ".target sm_75\n") ;     off = kstr_emit(out, off, ".address_size 64\n") ;     off = kstr_emit(out, off, ".visible .entry powgen(.param .u64 sb, .param .u64 pw, .param .u32 n) {\n") ;     off = kstr_emit(out, off, ".reg .u32 %e, %n, %nb, %t1, %t2;\n") ;     off = kstr_emit(out, off, ".reg .u64 %rsb, %rpw, %a;\n") ;     off = kstr_emit(out, off, ".reg .u8 %ub;\n") ;     off = kstr_emit(out, off, ".reg .f32 %f;\n") ;     off = kstr_emit(out, off, ".reg .pred %p;\n") ;     off = kstr_emit(out, off, "mov.u32 %t1, %ctaid.x;\n") ;     off = kstr_emit(out, off, "mov.u32 %t2, %ntid.x;\n") ;     off = kstr_emit(out, off, "mul.lo.u32 %e, %t1, %t2;\n")
```

### `pp_add_var(symtab, st, tokbuf, name_s, name_l, kind)`
**功能**：添加变量到符号表 (kind/aux 由调用方定), 返回 sym 索引。
**示例**：
```starc
vid = raw_get32(st, 0) ;     raw_set32(symtab, vid * 16, name_s) ;     raw_set32(symtab, vid * 16 + 4, name_l) ;     raw_set32(symtab, vid * 16 + 8, kind) ;     Wenn kind == 0 dann  // float ;         raw_set32(symtab, vid * 16 + 12, 16 + raw_get32(st, 24)) ;         raw_set32(st, 24, raw_get32(st, 24) + 1) ;     Sonst ;         Wenn kind == 1 dann  // int ;             raw_set32(symtab, vid * 16 + 12, 16 + raw_get32(st, 20)) ;             raw_set32(st, 20, raw_get32(st, 20) + 1) ;         Sonst  // array
```

### `pp_builtin_id(tokbuf, s, l)`
**功能**：线程索引内置 ID: 0=tid_x 1=tid_y 2=ctaid_x 3=ctaid_y 4=ntid_x 5=ntid_y。
**示例**：
```starc
Wenn pp_str_eq(tokbuf, s, l, "tid_x") dann gebe_zurueck 0 Ende ;     Wenn pp_str_eq(tokbuf, s, l, "tid_y") dann gebe_zurueck 1 Ende ;     Wenn pp_str_eq(tokbuf, s, l, "ctaid_x") dann gebe_zurueck 2 Ende ;     Wenn pp_str_eq(tokbuf, s, l, "ctaid_y") dann gebe_zurueck 3 Ende ;     Wenn pp_str_eq(tokbuf, s, l, "ntid_x") dann gebe_zurueck 4 Ende ;     Wenn pp_str_eq(tokbuf, s, l, "ntid_y") dann gebe_zurueck 5 Ende ;     Wenn pp_str_eq(tokbuf, s, l, "tid_z") dann gebe_zurueck 6 Ende ;     Wenn pp_str_eq(tokbuf, s, l, "ctaid_z") dann gebe_zurueck 7 Ende ;     Wenn pp_str_eq(tokbuf, s, l, "ntid_z") dann gebe_zurueck 8 Ende ;     gebe_zurueck 0 - 1
```

### `pp_int_op_prio(c)`
**功能**：* / = 3; + - = 2; < > <= >= = 1; und = 0 (最低)。
**示例**：
```starc
Wenn c == 42 dann gebe_zurueck 3 Ende  // * ;     Wenn c == 47 dann gebe_zurueck 3 Ende  // / ;     Wenn c == 43 dann gebe_zurueck 2 Ende  // + ;     Wenn c == 45 dann gebe_zurueck 2 Ende  // - ;     Wenn c == 60 dann gebe_zurueck 1 Ende  // < ;     Wenn c == 61 dann gebe_zurueck 1 Ende  // == ;     Wenn c == 62 dann gebe_zurueck 1 Ende  // > ;     Wenn c == 64 dann gebe_zurueck 1 Ende  // >= ;     Wenn c == 65 dann gebe_zurueck 1 Ende  // <= ;     gebe_zurueck 0                          // und
```

### `pp_parse_block(tokens, i, tn, ast, frpn, irpn, symtab, st, stopsonst)`
**功能**：语句块解析 (递归), tokens[i..] 起, 遇 Ende(kw2)/EOF 结束; stopsonst=1 时也在 Sonst(kw5) 停止 (then 分支)。
**示例**：
```starc
stop = 0 ;     Solange i < tn und stop == 0 ;         base = i * 32 ;         t = raw_get32(tokens, base) ;         Wenn t == 6 dann  // EOF ;             stop = 1 ;         Sonst ;             Wenn t == 3 und raw_get32(tokens, base + 4) == 2 dann  // Ende ;                 i = i + 1 ;                 stop = 1 ;             Sonst ;                 Wenn stopsonst == 1 und t == 3 und raw_get32(tokens, base + 4) == 5 dann  // Sonst (仅 then 分支, 不消费, 由 Wenn 处理器处理)
```

### `pp_parse_float_expr(tokens, i, tn, frpn, irpn, symtab, st)`
**功能**：浮点表达式解析, 支持数组装载 A[idx] (idx 为整数子RPN)。
**示例**：
```starc
ostack = alloc(256) ;     osp = 0 ;     fn = raw_get32(st, 8) ;     stop = 0 ;     Solange i < tn und stop == 0 ;         base = i * 32 ;         t = raw_get32(tokens, base) ;         Wenn t == 6 oder t == 7 dann  // EOF(6) 换行(7) ;             stop = 1 ;         Sonst ;             Wenn t == 3 dann ;                 stop = 1
```

### `pp_parse_int_expr(tokens, i, tn, irpn, symtab, st)`
**功能**：整数表达式解析 (Shunting-Yard), tokens[i..] 起, 遇 关键字/换行/EOF/]/)/, 结束。
**示例**：
```starc
ostack = alloc(256) ;     osp = 0 ;     in = raw_get32(st, 4) ;     stop = 0 ;     Solange i < tn und stop == 0 ;         base = i * 32 ;         t = raw_get32(tokens, base) ;         Wenn t == 6 oder t == 7 dann  // EOF(6) 换行(7) ;             stop = 1 ;         Sonst ;             Wenn t == 3 dann  // 关键字 ;                 Wenn raw_get32(tokens, base + 4) == 9 dann  // und: 逻辑与 (二元, prio 0)
```

### `pp_peek_int(tokens, i, tn, symtab, st)`
**功能**：从 i 起跳过前导 '(' , 判断第一个原子是否"整数表达式"。
**示例**：
```starc
j = i ;     Solange j < tn und raw_byteat(tokens, j * 32 + 8) == 40  // 跳过 ( ;         j = j + 1
```

### `pp_str_eq(tokbuf, s, l, kw)`
**功能**：tokbuf[s..s+l) 与 kw (StarC 字符串) 相等返回 1。
**示例**：
```starc
kl = str_len(kw) ;     Wenn kl != l dann gebe_zurueck 0 Ende ;     i = 0 ;     Solange i < l ;         Wenn raw_byteat(tokbuf, s + i) != raw_byteat(kw, 8 + i) dann ;             gebe_zurueck 0
```

### `pp_var_find(symtab, symcount, tokbuf, name_s, name_l)`
**功能**：符号表查找 (16B 项), 返回索引或 -1。
**示例**：
```starc
i = 0 ;     Solange i < symcount ;         ns = raw_get32(symtab, i * 16) ;         nl = raw_get32(symtab, i * 16 + 4) ;         Wenn nl == name_l dann ;             match = 1 ;             k = 0 ;             Solange k < name_l ;                 Wenn raw_byteat(tokbuf, ns + k) != raw_byteat(tokbuf, name_s + k) dann ;                     match = 0
```

### `pt_apply_split(cps, ncp, pst0, pen0, np0, iter_fn, pst1, pen1, spst, spen, cap)`
**功能**：应用一个 split 到码点区间列表 ----。
**示例**：
```starc
cnt = 0 ;     pi = 0 ;     Solange pi < np0 ;         lo = raw_get32(pst0, pi * 4) ;         hi = raw_get32(pen0, pi * 4) ;         // 找该 piece 内所有匹配 span ;         ns = 0 ;         Wenn iter_fn == 0 dann ;             ns = pt_s0_iter(cps, ncp, lo, hi, spst, spen) ;         Sonst ;             Wenn iter_fn == 1 dann ;                 ns = pt_s1_iter(cps, ncp, lo, hi, spst, spen)
```

### `pt_is_cjk(cp)`
**功能**：S1 CJK 范围。
**示例**：
```starc
Wenn (cp >= 19968 und cp <= 40365) dann ;         gebe_zurueck 1
```

### `pt_is_crlf(cp)`
**功能**：（功能描述未提取）。
**示例**：
```starc
Wenn cp == 13 dann ;         gebe_zurueck 1
```

### `pt_is_digit(cp)`
**功能**：码点类别 (返回 1/0) ----。
**示例**：
```starc
Wenn (cp >= 48 und cp <= 57) dann ;         gebe_zurueck 1
```

### `pt_is_letter(cp)`
**功能**：（功能描述未提取）。
**示例**：
```starc
// ASCII 字母 ;     Wenn (cp >= 65 und cp <= 90) dann ;         gebe_zurueck 1
```

### `pt_is_mark(cp)`
**功能**：（功能描述未提取）。
**示例**：
```starc
// 组合标记 (简化: 0x0300-0x036F 等) ;     Wenn (cp >= 768 und cp <= 879) dann ;         gebe_zurueck 1
```

### `pt_is_punct(cp)`
**功能**：（功能描述未提取）。
**示例**：
```starc
// ASCII 标点: 0x21-0x2F, 0x3A-0x40, 0x5B-0x60, 0x7B-0x7E ;     Wenn (cp >= 33 und cp <= 47) dann ;         gebe_zurueck 1
```

### `pt_is_space(cp)`
**功能**：（功能描述未提取）。
**示例**：
```starc
Wenn cp == 32 dann ;         gebe_zurueck 1
```

### `pt_is_symbol(cp)`
**功能**：（功能描述未提取）。
**示例**：
```starc
// 数学/货币/杂项符号 (常见) ;     Wenn (cp >= 162 und cp <= 191) dann ;         gebe_zurueck 1
```

### `pt_m_A(cps, n, i)`
**功能**：S2 分支匹配 (从码点索引 i 起, 返回匹配终点; -1 不匹配) ----。
**示例**：
```starc
Wenn i >= n dann ;         gebe_zurueck -1
```

### `pt_m_B(cps, n, i)`
**功能**：B: [^\r\n\p{L}\p{P}\p{S}]?[\p{L}\p{M}]+。
**示例**：
```starc
Wenn i >= n dann ;         gebe_zurueck -1
```

### `pt_m_C(cps, n, i)`
**功能**：C:  ?[\p{P}\p{S}]+[\r\n]*。
**示例**：
```starc
Wenn i >= n dann ;         gebe_zurueck -1
```

### `pt_m_D(cps, n, i)`
**功能**：D: \s*[\r\n]+。
**示例**：
```starc
Wenn i >= n dann ;         gebe_zurueck -1
```

### `pt_m_E(cps, n, i)`
**功能**：E: \s+(?!\S)。
**示例**：
```starc
Wenn i >= n dann ;         gebe_zurueck -1
```

### `pt_m_F(cps, n, i)`
**功能**：F: \s+。
**示例**：
```starc
Wenn i >= n dann ;         gebe_zurueck -1
```

### `pt_pretokenize(s, slen, cps, coffs, pst, pen, cap)`
**功能**：pre-tokenize ----。
**示例**：
```starc
ncp = pt_utf8_decode(s, slen, cps, coffs) ;     // 临时缓冲 (容量 cap) ;     pst0 = alloc(cap * 4) ;     pen0 = alloc(cap * 4) ;     pst1 = alloc(cap * 4) ;     pen1 = alloc(cap * 4) ;     spst = alloc(cap * 4) ;     spen = alloc(cap * 4) ;     // 初始: 1 个 piece [0, ncp) ;     raw_set32(pst0, 0, 0) ;     raw_set32(pen0, 0, ncp) ;     np = 1
```

### `pt_s0_iter(cps, n, lo, hi, spst, spen)`
**功能**：S0: \p{N}{1,3} 迭代 ----。
**示例**：
```starc
cnt = 0 ;     i = lo ;     Solange i < hi ;         p = raw_get32(cps, i * 4) ;         Wenn pt_is_digit(p) == 1 dann ;             j = i ;             stop = 0 ;             Solange j < hi und (j - i) < 3 und stop == 0 ;                 p = raw_get32(cps, j * 4) ;                 Wenn pt_is_digit(p) == 1 dann ;                     j = j + 1 ;                 Sonst
```

### `pt_s1_iter(cps, n, lo, hi, spst, spen)`
**功能**：S1: CJK+ 迭代 ----。
**示例**：
```starc
cnt = 0 ;     i = lo ;     Solange i < hi ;         p = raw_get32(cps, i * 4) ;         Wenn pt_is_cjk(p) == 1 dann ;             j = i ;             stop = 0 ;             Solange j < hi und stop == 0 ;                 p = raw_get32(cps, j * 4) ;                 Wenn pt_is_cjk(p) == 1 dann ;                     j = j + 1 ;                 Sonst
```

### `pt_s2_iter(cps, n, lo, hi, spst, spen)`
**功能**：S2 迭代: 在 [lo,hi) 码点区间找所有匹配 span, 输出到 spst/spen ----。
**示例**：
```starc
cnt = 0 ;     i = lo ;     Solange i < hi ;         // 尝试 A..F, 取第一个匹配的 (交替优先) ;         // 注意: 匹配上限是 hi (piece 终点), 不是 ncp (总长) - BPE 不跨 piece ;         end = -1 ;         e = pt_m_A(cps, hi, i) ;         Wenn e > i dann ;             end = e ;         Sonst ;             e = pt_m_B(cps, hi, i) ;             Wenn e > i dann
```

### `pt_utf8_decode(s, slen, cps, coffs)`
**功能**：UTF-8 解码: 字节串 -> 码点数组 + 每码点的字节偏移 ----。
**示例**：
```starc
n = 0 ;     i = 0 ;     Solange i < slen ;         b0 = raw_byteat(s, i) ;         cp = 0 ;         len = 1 ;         Wenn b0 < 128 dann ;             cp = b0 ;             len = 1 ;         Sonst ;             Wenn b0 < 224 dann ;                 // 2字节: 110xxxxx 10xxxxxx
```

### `ptr_get32(buf, off)`
**功能**：读 4 字节小端从 buf+off。
**示例**：
```starc
gebe_zurueck raw_byteat(buf, off) + raw_byteat(buf, off + 1) * 256 + raw_byteat(buf, off + 2) * 65536 + raw_byteat(buf, off + 3) * 16777216
```

### `ptr_get64(buf, off)`
**功能**：读 8 字节小端从 buf+off。
**示例**：
```starc
v = 0 ;     i = 7 ;     Solange i >= 0 ;         v = v * 256 ;         v = v + raw_byteat(buf, off + i) ;         i = i - 1
```

### `ptr_set32(buf, off, v)`
**功能**：写 4 字节小端到 buf+off。
**示例**：
```starc
raw_setbyte(buf, off, v % 256) ;     raw_setbyte(buf, off + 1, (v / 256) % 256) ;     raw_setbyte(buf, off + 2, (v / 65536) % 256) ;     raw_setbyte(buf, off + 3, (v / 16777216) % 256) ;     gebe_zurueck 0
```

### `ptr_set64(buf, off, v)`
**功能**：写 8 字节小端到 buf+off，v 为 64 位值。
**示例**：
```starc
i = 0 ;     Solange i < 8 ;         raw_setbyte(buf, off + i, v % 256) ;         v = v / 256 ;         i = i + 1
```

### `ptx_emit_f32hex(out, off, bits)`
**功能**：发射 PTX 十六进制浮点字面量 0f{8位hex} (fp32 位模式, 无符号除法安全)。
**示例**：
```starc
off = emit_str(out, off, "0f") ;     shift = 28 ;     k = 0 ;     Solange k < 8 ;         nb = bits / p2(shift) ;         nb = nb - (nb / 16) * 16 ;         Wenn nb < 10 dann ;             raw_setbyte(out, off, 48 + nb) ;         Sonst ;             raw_setbyte(out, off, 55 + nb)
```

### `ptx_instr(out, off, s)`
**功能**：写指令 + ";" + \n (内核内指令用)。
**示例**：
```starc
off = emit_str(out, off, s) ;     off = emit_str(out, off, ";") ;     off = ptx_nl(out, off) ;     gebe_zurueck off
```

### `ptx_line(out, off, s)`
**功能**：写一行 (字符串 + \n), 无分号 (顶层 directive 用)。
**示例**：
```starc
off = emit_str(out, off, s) ;     off = ptx_nl(out, off) ;     gebe_zurueck off
```

### `ptx_nl(out, off)`
**功能**：写纯 \n 换行。
**示例**：
```starc
raw_setbyte(out, off, 10) ;     gebe_zurueck off + 1
```

### `qpc()`
**功能**：（功能描述未提取）。
**示例**：
```starc
p = alloc(8) ;     QueryPerformanceCounter(p) ;     v = raw_byteat(p, 0) + raw_byteat(p, 1) * 256 + raw_byteat(p, 2) * 65536 + raw_byteat(p, 3) * 16777216 ;     v = v + raw_byteat(p, 4) * 4294967296 + raw_byteat(p, 5) * 1099511627776 + raw_byteat(p, 6) * 281474976710656 + raw_byteat(p, 7) * 72057594037927936 ;     gebe_zurueck v
```

### `qpf()`
**功能**：（功能描述未提取）。
**示例**：
```starc
p = alloc(8) ;     QueryPerformanceFrequency(p) ;     v = raw_byteat(p, 0) + raw_byteat(p, 1) * 256 + raw_byteat(p, 2) * 65536 + raw_byteat(p, 3) * 16777216 ;     v = v + raw_byteat(p, 4) * 4294967296 + raw_byteat(p, 5) * 1099511627776 + raw_byteat(p, 6) * 281474976710656 + raw_byteat(p, 7) * 72057594037927936 ;     gebe_zurueck v
```

### `r32(p, off)`
**功能**：读 4 字节小端从 p+off。
**示例**：
```starc
v = raw_byteat(p, off) + raw_byteat(p, off + 1) * 256 + raw_byteat(p, off + 2) * 65536 + raw_byteat(p, off + 3) * 16777216 ;     gebe_zurueck v
```

### `raw_get32(ptr, off)`
**功能**：小端 32 位读。
**示例**：
```starc
v = raw_byteat(ptr, off) ;     v = v + raw_byteat(ptr, off + 1) * 256 ;     v = v + raw_byteat(ptr, off + 2) * 65536 ;     v = v + raw_byteat(ptr, off + 3) * 16777216 ;     gebe_zurueck v
```

### `raw_get64(ptr, off)`
**功能**：小端 64 位读。
**示例**：
```starc
v = raw_byteat(ptr, off) ;     v = v + raw_byteat(ptr, off + 1) * 256 ;     v = v + raw_byteat(ptr, off + 2) * 65536 ;     v = v + raw_byteat(ptr, off + 3) * 16777216 ;     v = v + raw_byteat(ptr, off + 4) * 4294967296 ;     v = v + raw_byteat(ptr, off + 5) * 1099511627776 ;     v = v + raw_byteat(ptr, off + 6) * 274877906944 ;     v = v + raw_byteat(ptr, off + 7) * 70368744177664 ;     gebe_zurueck v
```

### `raw_set32(ptr, off, val)`
**功能**：小端 32 位写 (offset 起 4 字节)。
**示例**：
```starc
raw_setbyte(ptr, off, val % 256) ;     raw_setbyte(ptr, off + 1, (val / 256) % 256) ;     raw_setbyte(ptr, off + 2, (val / 65536) % 256) ;     raw_setbyte(ptr, off + 3, (val / 16777216) % 256) ;     gebe_zurueck 0
```

### `raw_set64(ptr, off, val)`
**功能**：小端 64 位写 (offset 起 8 字节)。
**示例**：
```starc
raw_setbyte(ptr, off, val % 256) ;     raw_setbyte(ptr, off + 1, (val / 256) % 256) ;     raw_setbyte(ptr, off + 2, (val / 65536) % 256) ;     raw_setbyte(ptr, off + 3, (val / 16777216) % 256) ;     raw_setbyte(ptr, off + 4, (val / 4294967296) % 256) ;     raw_setbyte(ptr, off + 5, (val / 1099511627776) % 256) ;     raw_setbyte(ptr, off + 6, (val / 274877906944) % 256) ;     raw_setbyte(ptr, off + 7, (val / 70368744177664) % 256) ;     gebe_zurueck 0
```

### `read_bytes(handle, buf, size)`
**功能**：read_bytes(handle, buf, size) - 从文件读取字节。
**示例**：
```starc
// 分配缓冲区接收读取字节数 ;     rn = alloc(4) ;     raw_setbyte(rn, 0, 0) ;     raw_setbyte(rn, 1, 0) ;     raw_setbyte(rn, 2, 0) ;     raw_setbyte(rn, 3, 0) ;  ;     ok = ReadFile(handle, buf, size, rn, 0) ;  ;     Wenn ok == 0 dann ;         gebe_zurueck 0
```

### `read_entire_file(path)`
**功能**：读取整个文件到 StarC 字符串。
**示例**：
```starc
path_cstr = to_cstr(path) ;     handle = CreateFileA(path_cstr, 2147483648, 0, 0, 3, 0, 0) ;  ;     Wenn handle == -1 dann ;         gebe_zurueck ""
```

### `read_i32(buf, off)`
**功能**：有符号 int32 小端读取。
**示例**：
```starc
v = raw_byteat(buf, off) + raw_byteat(buf, off + 1) * 256 + raw_byteat(buf, off + 2) * 65536 + raw_byteat(buf, off + 3) * 16777216 ;     Wenn v > 2147483647 dann ;         v = v - 4294967296
```

### `read_i64(buf, off)`
**功能**：有符号 int64 小端读取（用于 8 字节 word）。
**示例**：
```starc
v = read_u64(buf, off) ;     // 若符号位为 1（>2^63-1），转有符号 ;     Wenn v > 9223372036854775807 dann ;         v = v - 18446744073709551616
```

### `read_line()`
**功能**：读一行（去掉行尾 CR LF），返回 StarC 字符串；EOF 返回 0。
**示例**：
```starc
h = GetStdHandle(-10) ;     buf = alloc(1024) ;     i = 0 ;     fertig = 0 ;     Solange fertig == 0 ;         nb = alloc(4) ;         ok = ReadFile(h, buf + i, 1, nb, 0) ;         n = raw_byteat(nb, 0) + raw_byteat(nb, 1) * 256 + raw_byteat(nb, 2) * 65536 + raw_byteat(nb, 3) * 16777216 ;         Wenn ok == 0 dann ;             fertig = 1 ;         Sonst ;             Wenn n == 0 dann
```

### `read_s32(ptr, off)`
**功能**：读有符号 32 位 (补码 -> 负数)。
**示例**：
```starc
v = raw_get32(ptr, off) ;     Wenn v >= 2147483648 dann ;         gebe_zurueck v - 4294967296
```

### `read_u32(buf, offset)`
**功能**：read_u32(buf, offset) - 从缓冲区读取32位整数。
**示例**：
```starc
b0 = raw_byteat(buf, offset) ;     b1 = raw_byteat(buf, offset + 1) ;     b2 = raw_byteat(buf, offset + 2) ;     b3 = raw_byteat(buf, offset + 3) ;     value = b0 + b1 * 256 + b2 * 65536 + b3 * 16777216 ;     gebe_zurueck value
```

### `read_u32_le(buf, off)`
**功能**：（功能描述未提取）。
**示例**：
```starc
v = raw_byteat(buf, off + 0) + raw_byteat(buf, off + 1) * 256 ;     v = v + raw_byteat(buf, off + 2) * 65536 + raw_byteat(buf, off + 3) * 16777216 ;     gebe_zurueck v
```

### `read_u64(buf, off)`
**功能**：（功能描述未提取）。
**示例**：
```starc
v = raw_byteat(buf, off + 0) + raw_byteat(buf, off + 1) * 256 + raw_byteat(buf, off + 2) * 65536 + raw_byteat(buf, off + 3) * 16777216 ;     v = v + raw_byteat(buf, off + 4) * 4294967296 + raw_byteat(buf, off + 5) * 1099511627776 + raw_byteat(buf, off + 6) * 281474976710656 + raw_byteat(buf, off + 7) * 72057594037927936 ;     gebe_zurueck v
```

### `rechner_name()`
**功能**：------ Rechnername ----------。
**示例**：
```starc
buf = alloc(256) ;     nb = alloc(4) ;     raw_setbyte(nb, 0, 0) ;     raw_setbyte(nb, 1, 1) ;     raw_setbyte(nb, 2, 0) ;     raw_setbyte(nb, 3, 0) ;     n = GetComputerNameA(buf, nb) ;     gebe_zurueck cstr_to_starc(buf)
```

### `remove_extra_spaces(str)`
**功能**：（功能描述未提取）。
**示例**：
```starc
len = str_len(str) ;     result = alloc(8 + len + 1) ;     pos = 0 ;     i = 0 ;     last_space = 1 ;  ;     Solange i < len ;         ch = str_at(str, i) ;         Wenn ch == 32 ;             Wenn last_space == 0 ;                 raw_setbyte(result, 8 + pos, 32) ;                 pos = pos + 1
```

### `remove_script_style(html)`
**功能**：移除 <script> 和 <style> 块及其内容。
**示例**：
```starc
len = str_len(html) ;     result = alloc(8 + len + 1) ;     pos = 0 ;     i = 0 ;     skip_mode = 0  // 0=normal, 1=skip_script, 2=skip_style ;  ;     Solange i < len ;         ch = str_at(html, i) ;  ;         Wenn skip_mode == 0 ;             // 检查是否是 <script 或 <style 开始 ;             Wenn ch == 60
```

### `rmsnorm_u(x, roff, n)`
**功能**：无权重逐头 RMSNorm (q_b_norm)。
**示例**：
```starc
acc = 0 ;     i = 0 ;     Solange i < n ;         v = raw_get32(x, (roff + i) * 4) ;         acc = f32_add(acc, f32_mul(v, v)) ;         i = i + 1
```

### `rmsnorm_w(wb, x, n)`
**功能**：RMSNorm 带 BF16 weight。
**示例**：
```starc
acc = 0 ;     i = 0 ;     Solange i < n ;         v = raw_get32(x, i * 4) ;         acc = f32_add(acc, f32_mul(v, v)) ;         i = i + 1
```

### `rnd9(x)`
**功能**：floor(x*1e9+0.5)，复现 C rnd9（x 为负时需 floor 而非 trunc）。
**示例**：
```starc
t = fp_add(fp_mul(x, 4741671816366391296), 4602678819172646912)   // x*1e9 + 0.5 ;     v = f2i(t) ;     ti = i2f(v) ;     Wenn fp_cmp(t, 0) < 0 dann ;         Wenn fp_cmp(ti, t) != 0 dann ;             v = v - 1
```

### `rope_apply(x, roff, cossin, pos)`
**功能**：部分 RoPE (后 64 维)。
**示例**：
```starc
base = pos * 64 ;     i = 0 ;     Solange i < 32 ;         c = raw_get32(cossin, (base + i) * 4) ;         s = raw_get32(cossin, (base + 32 + i) * 4) ;         x0 = raw_get32(x, (roff + 2 * i) * 4) ;         x1 = raw_get32(x, (roff + 2 * i + 1) * 4) ;         raw_set32(x, (roff + 2 * i) * 4, f32_sub(f32_mul(x0, c), f32_mul(x1, s))) ;         raw_set32(x, (roff + 2 * i + 1) * 4, f32_add(f32_mul(x0, s), f32_mul(x1, c))) ;         i = i + 1
```

### `rope_reverse(x, roff, cossin, pos)`
**功能**：（功能描述未提取）。
**示例**：
```starc
base = pos * 64 ;     i = 0 ;     Solange i < 32 ;         c = raw_get32(cossin, (base + i) * 4) ;         s = raw_get32(cossin, (base + 32 + i) * 4) ;         x0 = raw_get32(x, (roff + 2 * i) * 4) ;         x1 = raw_get32(x, (roff + 2 * i + 1) * 4) ;         raw_set32(x, (roff + 2 * i) * 4, f32_add(f32_mul(x0, c), f32_mul(x1, s))) ;         raw_set32(x, (roff + 2 * i + 1) * 4, f32_sub(f32_mul(x1, c), f32_mul(x0, s))) ;         i = i + 1
```

### `run_cmd(cmd)`
**功能**：调用系统命令 (外部进程), 返回退出码 (老板令: linx 只能在 staro 内部实现)。
**示例**：
```starc
c = starc_to_cstr(cmd) ;     r = system(c) ;     gebe_zurueck r
```

### `s32_to_f32(n)`
**功能**：s32 -> fp32。
**示例**：
```starc
Wenn n == 0 dann ;         gebe_zurueck 0
```

### `s_adam_step(W, G, m, v, n, lr, beta1, beta2, eps, t)`
**功能**：S1: Adam 一阶单步 ----。
**示例**：
```starc
// bias correction: bc1 = 1 - b1^t, bc2 = 1 - b2^t ;     l1 = f32_ln(beta1) ;     l2 = f32_ln(beta2) ;     b1t = f32_exp(f32_mul(s32_to_f32(t), l1)) ;     b2t = f32_exp(f32_mul(s32_to_f32(t), l2)) ;     one = s32_to_f32(1) ;     bc1 = f32_sub(one, b1t) ;     bc2 = f32_sub(one, b2t) ;     omb1 = f32_sub(one, beta1) ;     omb2 = f32_sub(one, beta2) ;     i = 0 ;     Solange i < n
```

### `s_block_quant_fp4(buf, q, n, B, scale)`
**功能**：块级 fp4 量化 (e2m1, block 大小 B, 每块一个 scale) ----。
**示例**：
```starc
b = 0 ;     Solange b < n ;         amax = 0 ;         i = 0 ;         Solange i < B ;             v = raw_get32(buf, (b + i) * 4) ;             Wenn f32_gt(v, amax) dann amax = v Ende ;             Wenn f32_lt(v, f32_sub(0, amax)) dann amax = f32_sub(0, v) Ende ;             i = i + 1
```

### `s_block_quant_fp8(buf, q, n, B, scale)`
**功能**：块级 fp8 量化 (e4m3, block 大小 B, 每块一个 scale) ----。
**示例**：
```starc
b = 0 ;     Solange b < n ;         // 块内 amax ;         amax = 0 ;         i = 0 ;         Solange i < B ;             v = raw_get32(buf, (b + i) * 4) ;             Wenn f32_gt(v, amax) dann amax = v Ende ;             Wenn f32_lt(v, f32_sub(0, amax)) dann amax = f32_sub(0, v) Ende ;             i = i + 1
```

### `s_calib_amax(buf, n, amax_out)`
**功能**：收集整段激活的全局 amax (供动态量化 scale) ----。
**示例**：
```starc
amax = 0 ;     i = 0 ;     Solange i < n ;         v = raw_get32(buf, i * 4) ;         Wenn f32_gt(v, amax) dann amax = v Ende ;         Wenn f32_lt(v, f32_sub(0, amax)) dann amax = f32_sub(0, v) Ende ;         i = i + 1
```

### `s_grad_sparse(G, n, thr)`
**功能**：S3: 梯度阈值稀疏化 (置。
**示例**：
```starc
i = 0 ;     Solange i < n ;         g = raw_get32(G, i * 4) ;         Wenn f32_lt(g, thr) und f32_gt(g, f32_sub(s32_to_f32(0), thr)) dann ;             raw_set32(G, i * 4, s32_to_f32(0))
```

### `s_pack_f16(a)`
**功能**：S2: f32 <-> fp16 位模式转换 (混合精度, 权重/激活减半) ----。
**示例**：
```starc
// 简化为保留指数+截断尾数 (未做舍入, 精度足够 S2 混合训练) ;     // f32: s|e8|m23 -> f16: s|e5|m10 ;     sign = a / 2147483648 ;     expo = (a / 8388608) % 256 ;     mant = a % 8388608 ;     // 重偏置: e5 = e8 - 127 + 15 ;     e5 = expo - 127 + 15 ;     h = sign * 32768 + e5 * 1024 + mant / 8192 ;     gebe_zurueck h
```

### `s_quant_i8(buf, q, n, scale_out)`
**功能**：S2: f32 -> int8 量化 (含 scale), 输出 i8 字节 + scale f32 ----。
**示例**：
```starc
// 求 max|buf| ;     amax = s32_to_f32(0) ;     i = 0 ;     Solange i < n ;         v = raw_get32(buf, i * 4) ;         Wenn f32_gt(v, amax) dann amax = v Ende ;         Wenn f32_lt(v, f32_sub(s32_to_f32(0), amax)) dann amax = f32_sub(s32_to_f32(0), v) Ende ;         i = i + 1
```

### `s_round_half(x)`
**功能**：四舍五入到最近整数 (half away from zero), 返回 int ----。
**示例**：
```starc
half = f32_div(s32_to_f32(1), s32_to_f32(2))   // 0.5f ;     Wenn f32_lt(x, s32_to_f32(0)) dann ;         gebe_zurueck f32_to_s32(f32_sub(x, half)) ;     Sonst ;         gebe_zurueck f32_to_s32(f32_add(x, half))
```

### `s_unpack_f16(h)`
**功能**：fp16 位模式 -> f32 位模式。
**示例**：
```starc
sign = h / 32768 ;     e5 = (h / 1024) % 32 ;     mant = h % 1024 ;     expo = e5 - 15 + 127 ;     f = sign * 2147483648 + expo * 8388608 + mant * 8192 ;     gebe_zurueck f
```

### `save_ckpt_u(path, W, B, mW, vW, mB, vB, total_w, total_b, step)`
**功能**：保存完整状态 (W/B + 优化器 m/v) 到 path.u, 返回文件字节数, 0 失败 ----。
**示例**：
```starc
flags = 1 ;     sz = 32 + total_w * 4 * 3 + total_b * 4 * 3 ;     buf = alloc(sz) ;     raw_set32(buf, 0, u_magic()) ;     raw_set32(buf, 4, u_version()) ;     raw_set32(buf, 8, total_w) ;     raw_set32(buf, 12, total_b) ;     raw_set32(buf, 16, step) ;     raw_set32(buf, 20, flags) ;     raw_set32(buf, 24, 0) ;     raw_set32(buf, 28, 0) ;     off = 32
```

### `save_ckpt_u_buf(path, W, B, mW, vW, mB, vB, total_w, total_b, step, buf)`
**功能**：同 save_ckpt_u, 但序列化进调用方预分配 buf (断点周期存档用, 避免每步 re-alloc 152MB 撑爆 bump) ----。
**示例**：
```starc
flags = 1 ;     sz = 32 + total_w * 4 * 3 + total_b * 4 * 3 ;     raw_set32(buf, 0, u_magic()) ;     raw_set32(buf, 4, u_version()) ;     raw_set32(buf, 8, total_w) ;     raw_set32(buf, 12, total_b) ;     raw_set32(buf, 16, step) ;     raw_set32(buf, 20, flags) ;     raw_set32(buf, 24, 0) ;     raw_set32(buf, 28, 0) ;     off = 32 ;     i = 0
```

### `save_weights_u(path, W, B, total_w, total_b, step)`
**功能**：保存 W/B (无优化器状态) 到 path.u, 返回文件字节数, 0 失败 ----。
**示例**：
```starc
flags = 0 ;     // 组装: header(32) + W + B ;     sz = 32 + total_w * 4 + total_b * 4 ;     buf = alloc(sz) ;     raw_set32(buf, 0, u_magic()) ;     raw_set32(buf, 4, u_version()) ;     raw_set32(buf, 8, total_w) ;     raw_set32(buf, 12, total_b) ;     raw_set32(buf, 16, step) ;     raw_set32(buf, 20, flags) ;     raw_set32(buf, 24, 0) ;     raw_set32(buf, 28, 0)
```

### `sbuf(buf, cb, btype, ptr)`
**功能**：设置 SecBuffer 字段：buf[0]=cb, buf[4]=type, buf[8]=ptr。
**示例**：
```starc
w32(buf, 0, cb) ;     w32(buf, 4, btype) ;     raw_set64(buf, 8, ptr) ;     gebe_zurueck 0
```

### `sdesc(desc, ver, count, bufs)`
**功能**：设置 SecBufferDesc：desc[0]=version, desc[4]=count, desc[8]=buffers ptr。
**示例**：
```starc
w32(desc, 0, ver) ;     w32(desc, 4, count) ;     raw_set64(desc, 8, bufs) ;     gebe_zurueck 0
```

### `set_utf8_console()`
**功能**：设置控制台输出为 UTF-8 编码。
**示例**：
```starc
SetConsoleOutputCP(65001) ;     gebe_zurueck 1
```

### `shm_create()`
**功能**：（功能描述未提取）。
**示例**：
```starc
// CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 64, "LB_PAR_SHM") ;     h = CreateFileMappingA(-1, 0, 4, 0, 64, shm_name()) ;     gebe_zurueck h
```

### `shm_map(h)`
**功能**：（功能描述未提取）。
**示例**：
```starc
// MapViewOfFile(h, FILE_MAP_ALL_ACCESS=0xF001F=983071, 0, 0, 0) ;     p = MapViewOfFile(h, 983071, 0, 0, 0) ;     gebe_zurueck p
```

### `shm_name()`
**功能**：（功能描述未提取）。
**示例**：
```starc
// 返回 "LB_PAR_SHM\0" 的 C 字符串指针（静态，只建一次） ;     n = alloc(16) ;     raw_setbyte(n, 0, 76)   // L ;     raw_setbyte(n, 1, 66)   // B ;     raw_setbyte(n, 2, 95)   // _ ;     raw_setbyte(n, 3, 80)   // P ;     raw_setbyte(n, 4, 65)   // A ;     raw_setbyte(n, 5, 82)   // R ;     raw_setbyte(n, 6, 95)   // _ ;     raw_setbyte(n, 7, 83)   // S ;     raw_setbyte(n, 8, 72)   // H ;     raw_setbyte(n, 9, 77)   // M
```

### `sieve_run(n)`
**功能**：（功能描述未提取）。
**示例**：
```starc
buf = alloc(n + 1) ;     i = 0 ;     Solange i <= n ;         raw_setbyte(buf, i, 1) ;         i = i + 1
```

### `sigmoid_bits(v)`
**功能**：（功能描述未提取）。
**示例**：
```starc
nv = fs_neg(v) ;     e = f32_exp(nv) ;     o = f32_add(e, 1065353216) ;     gebe_zurueck f32_div(1065353216, o)
```

### `silu_clamp_ptx(out, off)`
**功能**：silu_clamp: 门激活合并, 复刻 moe_forward CPU 循环。
**示例**：
```starc
off = kstr_emit(out, off, ".version 7.0\n") ;     off = kstr_emit(out, off, ".target sm_75\n") ;     off = kstr_emit(out, off, ".address_size 64\n") ;     off = kstr_emit(out, off, ".visible .entry silu_clamp(.param .u64 g, .param .u64 u, .param .u64 o, .param .u32 n) {\n") ;     off = kstr_emit(out, off, ".reg .u32 %e, %n, %t1, %t2;\n") ;     off = kstr_emit(out, off, ".reg .u64 %rg, %ru, %ro, %a, %a2;\n") ;     off = kstr_emit(out, off, ".reg .f32 %f, %x, %uv, %s;\n") ;     off = kstr_emit(out, off, ".reg .pred %p;\n") ;     off = kstr_emit(out, off, "mov.u32 %t1, %ctaid.x;\n") ;     off = kstr_emit(out, off, "mov.u32 %t2, %ntid.x;\n") ;     off = kstr_emit(out, off, "mul.lo.u32 %e, %t1, %t2;\n") ;     off = kstr_emit(out, off, "mov.u32 %t1, %tid.x;\n")
```

### `silu_mul_ptx(out, off)`
**功能**：silu_mul(g, u, out, N)。
**示例**：
```starc
off = kstr_emit5(out, off, ".version 7.0\n") ;     off = kstr_emit5(out, off, ".target sm_75\n") ;     off = kstr_emit5(out, off, ".address_size 64\n") ;     off = kstr_emit5(out, off, ".visible .entry silu_mul(.param .u64 g, .param .u64 u, .param .u64 out, .param .u32 N) {\n") ;     off = kstr_emit5(out, off, ".reg .u32 %i, %N, %t1, %t2;\n") ;     off = kstr_emit5(out, off, ".reg .u64 %rg, %ru, %ro, %addr, %addr2;\n") ;     off = kstr_emit5(out, off, ".reg .f32 %g, %u, %ng, %e, %d, %s, %r;\n") ;     off = kstr_emit5(out, off, ".reg .pred %p;\n") ;     off = kstr_emit5(out, off, "mov.u32 %t1, %ctaid.x;\n") ;     off = kstr_emit5(out, off, "mov.u32 %t2, %ntid.x;\n") ;     off = kstr_emit5(out, off, "mul.lo.u32 %i, %t1, %t2;\n") ;     off = kstr_emit5(out, off, "mov.u32 %t1, %tid.x;\n")
```

### `sleep_ms(ms)`
**功能**：毫秒睡眠。
**示例**：
```starc
Sleep(ms) ;     gebe_zurueck 0
```

### `softmax4(row, out)`
**功能**：4 元素 softmax (dim=-1), row/out 是 4 个 fp32 位模式缓冲。
**示例**：
```starc
zmax = raw_get32(row, 0) ;     i = 1 ;     Solange i < 4 ;         zv = raw_get32(row, i * 4) ;         Wenn f32_gt(zv, zmax) dann ;             zmax = zv
```

### `softmax6_ptx(out, off)`
**功能**：（功能描述未提取）。
**示例**：
```starc
off = kstr_emit4(out, off, ".version 7.0\n") ;     off = kstr_emit4(out, off, ".target sm_75\n") ;     off = kstr_emit4(out, off, ".address_size 64\n") ;     off = kstr_emit4(out, off, ".visible .entry softmax6(.param .u64 logits, .param .u64 maxptr, .param .u64 out) {\n") ;     off = kstr_emit4(out, off, ".reg .u32 %t, %t1;\n") ;     off = kstr_emit4(out, off, ".reg .u64 %rl, %rm, %ro, %addr;\n") ;     off = kstr_emit4(out, off, ".reg .f32 %l, %mx, %d, %e;\n") ;     off = kstr_emit4(out, off, ".reg .pred %p;\n") ;     off = kstr_emit4(out, off, "mov.u32 %t, %tid.x;\n") ;     off = kstr_emit4(out, off, "setp.ge.u32 %p, %t, 6;\n") ;     off = kstr_emit4(out, off, "@%p bra EXIT;\n") ;     off = kstr_emit4(out, off, "ld.param.u64 %rl, [logits];\n")
```

### `sort_run(buf)`
**功能**：（功能描述未提取）。
**示例**：
```starc
n = 2000000 ;     a = alloc(n * 4) ;     b = alloc(n * 4) ;     count = alloc(256 * 8) ;     pos = alloc(256 * 8) ;  ;     // 加载输入 ;     i = 0 ;     Solange i < n ;         write_u32_le(a, i * 4, read_u32_le(buf, i * 4)) ;         i = i + 1
```

### `st_dtype_bits(buf, s, l)`
**功能**：dtype 解码表: dtype 字符串(buf[s..s+l)) -> 每元素位数 ----。
**示例**：
```starc
// BF16 (66 70 49 54) ;     Wenn l == 4 und raw_byteat(buf, s) == 66 und raw_byteat(buf, s+1) == 70 und raw_byteat(buf, s+2) == 49 und raw_byteat(buf, s+3) == 54 dann ;         gebe_zurueck 16
```

### `st_dtype_bytes(buf, s, l)`
**功能**：每元素字节数 (FP4/NF4 返回 0, 用 st_dtype_bits/8 或特殊处理)。
**示例**：
```starc
bits = st_dtype_bits(buf, s, l) ;     Wenn bits == 0 dann ;         gebe_zurueck 0
```

### `st_next_tensor(buf, n, pos, out)`
**功能**：从 pos 找下一个 tensor 条目, 填 out, 返回下个扫描位置 (0 = 结束)。
**示例**：
```starc
i = st_skipws(buf, pos, n) ;     Wenn i >= n dann ;         gebe_zurueck 0
```

### `st_parse_offs(buf, i, n, out)`
**功能**：解析 data_offsets [begin,end] 写入 out, 返回 ']' 后位置。
**示例**：
```starc
i = st_skipws(buf, i, n) ;     Wenn i < n und raw_byteat(buf, i) == 91 dann ;         i = i + 1 ;         p = alloc(4) ;         v0 = st_read_num(buf, i, n, p) ;         i = raw_get32(p, 0) ;         raw_set64(out, 72, v0) ;         i = st_skipws(buf, i, n) ;         Wenn i < n und raw_byteat(buf, i) == 44 dann ;             i = i + 1
```

### `st_parse_shape(buf, i, n, out)`
**功能**：解析 shape 数组 [a,b,c] 写入 out, 返回 ']' 后位置。
**示例**：
```starc
dim = 0 ;     raw_set64(out, 40, 0) ;     raw_set64(out, 48, 0) ;     raw_set64(out, 56, 0) ;     raw_set64(out, 64, 0) ;     i = st_skipws(buf, i, n) ;     Wenn i < n und raw_byteat(buf, i) == 91 dann ;         i = i + 1 ;         Solange i < n ;             i = st_skipws(buf, i, n) ;             Wenn i >= n dann ;                 gebe_zurueck i
```

### `st_parse_tensor_obj(buf, j, n, out)`
**功能**：解析一个 tensor 对象 (buf[j]=='{'), 填 out, 返回对象结束后的位置。
**示例**：
```starc
j = j + 1  // 跳过 '{' ;     Solange j < n ;         j = st_skipws(buf, j, n) ;         Wenn j >= n dann ;             gebe_zurueck j
```

### `st_read_num(buf, i, n, out_pos)`
**功能**：读整数 (可负), 返回数值, 新位置写 out_pos(4字节)。
**示例**：
```starc
i = st_skipws(buf, i, n) ;     neg = 0 ;     Wenn i < n und raw_byteat(buf, i) == 45 dann ;         neg = 1 ;         i = i + 1
```

### `st_read_str(buf, i, n, out_len)`
**功能**：读引号字符串 (buf[i]==34), 返回内容起始偏移(不含引号), 内容长度写 out_len(4字节)。
**示例**：
```starc
Wenn i >= n dann ;         raw_set32(out_len, 0, 0) ;         gebe_zurueck i
```

### `st_skip_value(buf, i, n)`
**功能**：跳过任意 JSON 值 (字符串/数字/数组/对象/字面量), 返回值后位置。
**示例**：
```starc
i = st_skipws(buf, i, n) ;     Wenn i >= n dann ;         gebe_zurueck i
```

### `st_skipws(buf, i, n)`
**功能**：跳过空白 (空格32 换行10 回车13 制表9), 返回新位置。
**示例**：
```starc
Solange i < n ;         c = raw_byteat(buf, i) ;         Wenn c != 32 und c != 10 und c != 13 und c != 9 dann ;             gebe_zurueck i
```

### `starc_to_cstr(s)`
**功能**：StarC 字符串 -> C 字符串 (裸指针, null 结尾, 仅临时用)。
**示例**：
```starc
n = str_len(s) ;     buf = alloc(n + 1) ;     i = 0 ;     Solange i < n ;         raw_setbyte(buf, i, raw_byteat(s, 8 + i)) ;         i = i + 1
```

### `str_at(str, pos)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck byteat(str, pos)
```

### `str_concat(s1, s2)`
**功能**：（功能描述未提取）。
**示例**：
```starc
l1 = str_len(s1) ;     l2 = str_len(s2) ;     total = l1 + l2 ;     result = alloc(8 + total + 1) ;     raw_setbyte(result, 0, total % 256) ;     raw_setbyte(result, 1, (total / 256) % 256) ;     raw_setbyte(result, 2, (total / 65536) % 256) ;     raw_setbyte(result, 3, (total / 16777216) % 256) ;     raw_setbyte(result, 4, 0) ;     raw_setbyte(result, 5, 0) ;     raw_setbyte(result, 6, 0) ;     raw_setbyte(result, 7, 0)
```

### `str_copy(s)`
**功能**：（功能描述未提取）。
**示例**：
```starc
n = str_len(s) ;     buf = alloc(8 + n + 1) ;     i = 0 ;     Solange i < n ;         raw_setbyte(buf, 8 + i, raw_byteat(s, 8 + i)) ;         i = i + 1
```

### `str_cut_tail(s, cut)`
**功能**：去掉末尾 cut 个字符（StarC 字符串）。
**示例**：
```starc
n = str_len(s) ;     m = n - cut ;     Wenn m < 0 dann ;         m = 0
```

### `str_eq(s1, s2)`
**功能**：比较两个 StarC 字符串, 相等返回 1。
**示例**：
```starc
l1 = str_len(s1) ;     l2 = str_len(s2) ;     Wenn l1 != l2 dann ;         gebe_zurueck 0
```

### `str_eq_at(c, pos, s)`
**功能**：C 字符串从 pos 起是否等于 StarC 字符串（边界为空格/NUL）。
**示例**：
```starc
n = str_len(s) ;     i = 0 ;     Solange i < n ;         Wenn raw_byteat(c, pos + i) != raw_byteat(s, 8 + i) dann ;             gebe_zurueck 0
```

### `str_eq_ci(s1, s2)`
**功能**：大小写不敏感字符串比较（ASCII）。
**示例**：
```starc
n1 = str_len(s1) ;     n2 = str_len(s2) ;     Wenn n1 != n2 dann ;         gebe_zurueck 0
```

### `str_find(str, substr)`
**功能**：（功能描述未提取）。
**示例**：
```starc
sl = str_len(str) ;     subl = str_len(substr) ;     Wenn subl == 0 dann ;         gebe_zurueck 0
```

### `str_ieq_at(str, pos, pattern)`
**功能**：大小写不敏感匹配：检查 str 在 pos 位置是否以 pattern 开头。
**示例**：
```starc
sl = str_len(str) ;     pl = str_len(pattern) ;     Wenn pos + pl > sl dann ;         gebe_zurueck 0
```

### `str_len(str)`
**功能**：StarC 字符串长度 (数据从 offset 8)。
**示例**：
```starc
i = 0 ;     Solange raw_byteat(str, 8 + i) != 0 ;         i = i + 1
```

### `str_lower(str)`
**功能**：（功能描述未提取）。
**示例**：
```starc
len = str_len(str) ;     result = alloc(8 + len + 1) ;     raw_setbyte(result, 0, len % 256) ;     raw_setbyte(result, 1, (len / 256) % 256) ;     raw_setbyte(result, 2, (len / 65536) % 256) ;     raw_setbyte(result, 3, (len / 16777216) % 256) ;     raw_setbyte(result, 4, 0) ;     raw_setbyte(result, 5, 0) ;     raw_setbyte(result, 6, 0) ;     raw_setbyte(result, 7, 0) ;     i = 0 ;     Solange i < len
```

### `str_reverse(s)`
**功能**：Zeichenkette umdrehen。
**示例**：
```starc
n = str_len(s) ;     buf = alloc(8 + n + 1) ;     i = 0 ;     Solange i < n ;         raw_setbyte(buf, 8 + i, raw_byteat(s, 8 + n - 1 - i)) ;         i = i + 1
```

### `str_slice(s, start, len)`
**功能**：（功能描述未提取）。
**示例**：
```starc
buf = alloc(8 + len + 1) ;     raw_setbyte(buf, 0, len % 256) ;     raw_setbyte(buf, 1, (len / 256) % 256) ;     raw_setbyte(buf, 2, (len / 65536) % 256) ;     raw_setbyte(buf, 3, (len / 16777216) % 256) ;     i = 0 ;     Solange i < len ;         raw_setbyte(buf, 8 + i, raw_byteat(s, 8 + start + i)) ;         i = i + 1
```

### `str_starts_with(str, prefix)`
**功能**：（功能描述未提取）。
**示例**：
```starc
sl = str_len(str) ;     pl = str_len(prefix) ;     Wenn pl > sl dann ;         gebe_zurueck 0
```

### `str_substr(str, start, len)`
**功能**：（功能描述未提取）。
**示例**：
```starc
result = alloc(8 + len + 1) ;     raw_setbyte(result, 0, len % 256) ;     raw_setbyte(result, 1, (len / 256) % 256) ;     raw_setbyte(result, 2, (len / 65536) % 256) ;     raw_setbyte(result, 3, (len / 16777216) % 256) ;     raw_setbyte(result, 4, 0) ;     raw_setbyte(result, 5, 0) ;     raw_setbyte(result, 6, 0) ;     raw_setbyte(result, 7, 0) ;     i = 0 ;     Solange i < len ;         raw_setbyte(result, 8 + i, str_at(str, start + i))
```

### `str_to_int(s)`
**功能**：StarC 字符串 -> 整数。
**示例**：
```starc
n = str_len(s) ;     i = 0 ;     v = 0 ;     neg = 0 ;     Wenn n > 0 dann ;         Wenn raw_byteat(s, 8) == 45 dann ;             neg = 1 ;             i = 1
```

### `str_trim(str)`
**功能**：（功能描述未提取）。
**示例**：
```starc
len = str_len(str) ;     start = 0 ;     end = len - 1 ;  ;     // 跳过前导空格 ;     skip_start = 0 ;     Solange start < len und skip_start == 0 ;         ch = str_at(str, start) ;         Wenn ch == 32 oder ch == 9 oder ch == 10 oder ch == 13 dann ;             start = start + 1 ;         Sonst ;             skip_start = 1
```

### `str_upper(s)`
**功能**：ASCII-Grossbuchstaben。
**示例**：
```starc
n = str_len(s) ;     buf = alloc(8 + n + 1) ;     i = 0 ;     Solange i < n ;         ch = raw_byteat(s, 8 + i) ;         Wenn ch >= 97 dann ;             Wenn ch <= 122 dann ;                 ch = ch - 32
```

### `string_run(buf, n)`
**功能**：（功能描述未提取）。
**示例**：
```starc
codepoints = 0 ;     spaces = 0 ;     checksum = 0 ;     i = 0 ;     Solange i < n ;         c = raw_byteat(buf, i) ;         // UTF-8 续字节：0x80..0xBF（等价 (c & 0xC0) == 0x80） ;         cont = 0 ;         Wenn c >= 128 dann ;             Wenn c <= 191 dann ;                 cont = 1
```

### `strip_html_tags(html)`
**功能**：（功能描述未提取）。
**示例**：
```starc
len = str_len(html) ;     result = alloc(8 + len + 1) ;     pos = 0 ;     i = 0 ;     in_tag = 0 ;  ;     Solange i < len ;         ch = str_at(html, i) ;         Wenn in_tag == 0 ;             Wenn ch == 60 ;                 in_tag = 1 ;                 i = i + 1
```

### `sut_accept(srv)`
**功能**：（功能描述未提取）。
**示例**：
```starc
c = accept(srv, 0, 0) ;     gebe_zurueck c
```

### `sut_cleanup()`
**功能**：（功能描述未提取）。
**示例**：
```starc
WSACleanup() ;     gebe_zurueck 0
```

### `sut_close(s)`
**功能**：（功能描述未提取）。
**示例**：
```starc
Wenn s >= 0 dann ;         closesocket(s)
```

### `sut_connect(ip, port)`
**功能**：连接 ip:port；ip 为点分十进制字符串（如 "125.25.71.69"）。返回 s（<0 失败）。
**示例**：
```starc
h = GetProcessHeap() ;     addr = HeapAlloc(h, 0, 16) ;     memset(addr, 0, 16) ;     memset(addr, 2, 1) ;     hi = port / 256 ;     lo = port - hi * 256 ;     memset(addr + 2, hi, 1) ;     memset(addr + 3, lo, 1) ;     ok = ipv4_set(addr, ip) ;     Wenn ok != 1 dann ;         HeapFree(h, 0, addr) ;         gebe_zurueck 0 - 1
```

### `sut_connect6(addr16, port)`
**功能**：连 IPv6：addr16 为 16 字节地址（inet_pton 解析结果）。
**示例**：
```starc
h = GetProcessHeap() ;     addr = HeapAlloc(h, 0, 32) ;     memset(addr, 0, 32) ;     memset(addr, 23, 1) ;     hi = port / 256 ;     lo = port - hi * 256 ;     memset(addr + 2, hi, 1) ;     memset(addr + 3, lo, 1) ;     memcpy(addr + 8, addr16, 16) ;     s = socket(23, 1, 6) ;     Wenn s < 0 dann ;         HeapFree(h, 0, addr)
```

### `sut_connect_auto(text, port)`
**功能**：按文本地址自动选协议连接（含冒号走 IPv6，否则 IPv4）。
**示例**：
```starc
Wenn sut_is_v6(text) == 1 dann ;         a16 = HeapAlloc(GetProcessHeap(), 0, 16) ;         memset(a16, 0, 16) ;         ok = sut_pton6(text, a16) ;         Wenn ok != 1 dann ;             HeapFree(GetProcessHeap(), 0, a16) ;             gebe_zurueck 0 - 4
```

### `sut_err()`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck WSAGetLastError()
```

### `sut_is_v6(text)`
**功能**：判断文本地址是否 IPv6（含冒号即认为是 IPv6）。
**示例**：
```starc
n = strlen(cstr(text)) ;     i = 0 ;     Solange i < n ;         Wenn byteat(text, i) == 58 dann ;             gebe_zurueck 1
```

### `sut_listen(port)`
**功能**：创建并绑定监听 socket；port 为十进制端口。返回 srv（<0 失败）。
**示例**：
```starc
h = GetProcessHeap() ;     srv = socket(2, 1, 6) ;     Wenn srv < 0 dann ;         gebe_zurueck 0 - 1
```

### `sut_listen_dual(port)`
**功能**：双栈监听：一个 socket 同时接受 IPv4 + IPv6。
**示例**：
```starc
h = GetProcessHeap() ;     s = socket(23, 1, 6) ;     Wenn s < 0 dann ;         gebe_zurueck sut_listen(port)
```

### `sut_pton6(text, out16)`
**功能**：文本 IPv6（如 "2409:8a3c:3d37:69e1::"）→ out16 的 16 字节。
**示例**：
```starc
memset(out16, 0, 16) ;     r = inet_pton(23, cstr(text), out16) ;     Wenn r != 1 dann ;         gebe_zurueck 0
```

### `sut_recv(s, buf, max)`
**功能**：（功能描述未提取）。
**示例**：
```starc
n = recv(s, buf, max, 0) ;     gebe_zurueck n
```

### `sut_recv_frame(s, buf, cap)`
**功能**：读完整一帧：[4字节大端长度][负载]。buf 为 raw 缓冲，cap 为容量。
**示例**：
```starc
n = recv(s, buf, 4, 0) ;     Wenn n == 0 dann ;         gebe_zurueck 0
```

### `sut_relay(c, t)`
**功能**：------------ 双向中继（SO_RCVTIMEO 交替轮询） ----------------。
**示例**：
```starc
h = GetProcessHeap() ;     cbuf = HeapAlloc(h, 0, 65536) ;     tbuf = HeapAlloc(h, 0, 65536) ;     sut_timeout(c, 30) ;     sut_timeout(t, 30) ;     dauer = 1 ;     Solange dauer == 1 ;         n = recv(c, cbuf, 65536, 0) ;         Wenn n > 0 dann ;             r = send(t, cbuf, n, 0) ;             Wenn r != n dann ;                 dauer = 0
```

### `sut_send(s, ptr, len)`
**功能**：（功能描述未提取）。
**示例**：
```starc
n = send(s, ptr, len, 0) ;     gebe_zurueck n
```

### `sut_send_frame(s, ptr, len)`
**功能**：写完整一帧：[4字节大端长度][负载]。返回 1 成功 / 0 失败。
**示例**：
```starc
hd = HeapAlloc(GetProcessHeap(), 0, 4) ;     memset(hd, 0, 4) ;     memset(hd, (len / 16777216) % 256, 1) ;     memset(hd + 1, (len / 65536) % 256, 1) ;     memset(hd + 2, (len / 256) % 256, 1) ;     memset(hd + 3, len % 256, 1) ;     r = send(s, hd, 4, 0) ;     Wenn r != 4 dann ;         HeapFree(GetProcessHeap(), 0, hd) ;         gebe_zurueck 0
```

### `sut_timeout(s, ms)`
**功能**：设置收发超时（毫秒）；timeout 为整数毫秒。
**示例**：
```starc
tv = HeapAlloc(GetProcessHeap(), 0, 8) ;     memset(tv, 0, 8) ;     lo = ms ;     hi = ms / 256 ;     memset(tv, lo % 256, 1) ;     memset(tv + 1, (lo / 256) % 256, 1) ;     memset(tv + 2, (lo / 65536) % 256, 1) ;     memset(tv + 3, (lo / 16777216) % 256, 1) ;     memset(tv + 4, hi % 256, 1) ;     memset(tv + 5, (hi / 256) % 256, 1) ;     memset(tv + 6, (hi / 65536) % 256, 1) ;     memset(tv + 7, (hi / 16777216) % 256, 1)
```

### `sut_wsa()`
**功能**：（功能描述未提取）。
**示例**：
```starc
wsa = HeapAlloc(GetProcessHeap(), 0, 512) ;     memset(wsa, 0, 512) ;     r = WSAStartup(514, wsa) ;     HeapFree(GetProcessHeap(), 0, wsa) ;     gebe_zurueck r
```

### `t_add(dst, b, n)`
**功能**：dst[i] += b[i] ----。
**示例**：
```starc
i = 0 ;     Solange i < n ;         raw_set32(dst, i * 4, f32_add(raw_get32(dst, i * 4), raw_get32(b, i * 4))) ;         i = i + 1
```

### `t_alloc(n)`
**功能**：分配 n 个 fp32 的张量缓冲 ----。
**示例**：
```starc
gebe_zurueck alloc(n * 4)
```

### `t_copy(dst, src, n)`
**功能**：拷贝 n 个元素 ----。
**示例**：
```starc
i = 0 ;     Solange i < n ;         raw_set32(dst, i * 4, raw_get32(src, i * 4)) ;         i = i + 1
```

### `t_fill(buf, n, v)`
**功能**：填充常量 ----。
**示例**：
```starc
i = 0 ;     Solange i < n ;         raw_set32(buf, i * 4, v) ;         i = i + 1
```

### `t_get(buf, i)`
**功能**：张量读/写 (fp32 位模式) ----。
**示例**：
```starc
gebe_zurueck raw_get32(buf, i * 4)
```

### `t_scale(buf, n, s)`
**功能**：dst[i] *= s ----。
**示例**：
```starc
i = 0 ;     Solange i < n ;         raw_set32(buf, i * 4, f32_mul(raw_get32(buf, i * 4), s)) ;         i = i + 1
```

### `t_set(buf, i, v)`
**功能**：（功能描述未提取）。
**示例**：
```starc
raw_set32(buf, i * 4, v) ;     gebe_zurueck 0
```

### `t_vadd(dst, b, c, n)`
**功能**：逐元素 a[i] = b[i] + c[i] ----。
**示例**：
```starc
i = 0 ;     Solange i < n ;         raw_set32(dst, i * 4, f32_add(raw_get32(b, i * 4), raw_get32(c, i * 4))) ;         i = i + 1
```

### `t_vmul(dst, b, c, n)`
**功能**：逐元素 a[i] = b[i] * c[i] ----。
**示例**：
```starc
i = 0 ;     Solange i < n ;         raw_set32(dst, i * 4, f32_mul(raw_get32(b, i * 4), raw_get32(c, i * 4))) ;         i = i + 1
```

### `t_vsub(dst, b, c, n)`
**功能**：逐元素 a[i] = b[i] - c[i] ----。
**示例**：
```starc
i = 0 ;     Solange i < n ;         raw_set32(dst, i * 4, f32_sub(raw_get32(b, i * 4), raw_get32(c, i * 4))) ;         i = i + 1
```

### `t_zero(buf, n)`
**功能**：清零 n 个元素 ----。
**示例**：
```starc
i = 0 ;     Solange i < n ;         raw_set32(buf, i * 4, 0) ;         i = i + 1
```

### `tabelle_ausgeben(tab)`
**功能**：------ Komplette Tabelle als Hilfe ausgeben ----------。
**示例**：
```starc
tl = str_len(tab) ;     pos = 0 ;     Solange pos < tl ;         ende = zeilen_ende(tab, pos) ;         ch = raw_byteat(tab, 8 + pos) ;         Wenn ch == 91 dann ;             write_str("  [") ;             write_str(bereich_name(tab, pos, ende)) ;             write_str("]") ;             write_newline() ;         Sonst ;             Wenn ch != 35 dann
```

### `tabelle_befehlpfad(tab, name)`
**功能**：------ Pfad oder Kommandoname des Tabellenbefehls ----------。
**示例**：
```starc
tl = str_len(tab) ;     pos = 0 ;     Solange pos < tl ;         ende = zeilen_ende(tab, pos) ;         b = zeile_befehl(tab, pos, ende) ;         Wenn str_len(b) != 0 dann ;             Wenn raw_byteat(b, 8) != 91 dann ;                 Wenn raw_byteat(b, 8) != 35 dann ;                     t = befehl_token(b) ;                     Wenn str_eq_ci(basis_name(t), name) == 1 dann ;                         gebe_zurueck t
```

### `tabelle_laden()`
**功能**：------ Tabelle laden ----------。
**示例**：
```starc
t = file_read("starshell_cmd.txt") ;     Wenn t != 0 dann ;         gebe_zurueck t
```

### `tick_ms()`
**功能**：系统运行毫秒数 (GetTickCount)。
**示例**：
```starc
t = GetTickCount() ;     gebe_zurueck t
```

### `tls_close(ctx, cred)`
**功能**：（功能描述未提取）。
**示例**：
```starc
DeleteSecurityContext(ctx) ;     FreeCredentialsHandle(cred) ;     gebe_zurueck 0
```

### `tls_connect(sock, host_c, cred, ctx)`
**功能**：TLS 握手：在已连接的 sock 上对 host_c 完成 TLS。返回 0 成功 / 负错误码。
**示例**：
```starc
ts = alloc(8) ;     mem_zero(ts, 8) ;     sch = alloc(80) ;     tls_cred(sch) ;     st = AcquireCredentialsHandleW(0, tls_utf16(mg_cstr("SCHANNEL")), 2, 0, sch, 0, 0, cred, ts) ;     Wenn st != 0 dann ;         gebe_zurueck 0 - 1
```

### `tls_cred(sch)`
**功能**：填充 SCHANNEL_CRED（x64 布局，总长 80 字节）：。
**示例**：
```starc
i = 0 ;     Solange i < 80 ;         raw_setbyte(sch, i, 0) ;         i = i + 1
```

### `tls_recv(sock, ctx, outbuf, cap)`
**功能**：TLS 接收：从 socket 读并解密，写明文到 outbuf（cap 容量）。
**示例**：
```starc
inbuf = alloc(65536) ;     mem_zero(inbuf, 65536) ;     rr = recv(sock, inbuf, 65536, 0) ;     Wenn isneg(rr) == 1 oder rr == 0 dann ;         gebe_zurueck 0 - 1
```

### `tls_send(sock, ctx, data, len)`
**功能**：TLS 发送：加密 data（len 字节）并写 socket。返回 0 成功 / 负错误码。
**示例**：
```starc
// 三缓冲：DATA, TOKEN(预分配), EXTRA —— ★ 必须连续（sdesc 按 count 连续读取） ;     b0 = alloc(48) ;     b1 = b0 + 16 ;     b2 = b0 + 32 ;     sbuf(b0, len, 1, data)              // SECBUFFER_DATA ;     sbuf(b1, 0, 2, 0)                   // SECBUFFER_TOKEN（输出） ;     sbuf(b2, 0, 0, 0)                   // SECBUFFER_EMPTY（EXTRA） ;     // 预分配 token 缓冲 ;     tk = alloc(65536) ;     mem_zero(tk, 65536) ;     raw_set64(b1, 8, tk)                // 提供 token 缓冲指针 ;     w32(b1, 0, 65536)                   // 容量（type 保持 TOKEN=2）
```

### `tls_utf16(s)`
**功能**：ANSI C 串 -> UTF-16（新分配，含双 NUL 结尾）。
**示例**：
```starc
n = cstr_len(s) ;     q = alloc((n + 1) * 2) ;     i = 0 ;     Solange i < n ;         raw_setbyte(q, i * 2, raw_byteat(s, i)) ;         raw_setbyte(q, i * 2 + 1, 0) ;         i = i + 1
```

### `to_cstr(str)`
**功能**：（功能描述未提取）。
**示例**：
```starc
len = str_len(str) ;     buf = alloc(len + 1) ;     i = 0 ;     Solange i < len ;         raw_setbyte(buf, i, str_at(str, i)) ;         i = i + 1
```

### `tok_bpe_word(word, n, vb, rk, ids, idcap)`
**功能**：BPE 合并一个字（u16 序列，n 字符）-> 写 token id 到 ids，返回 token 数 ----。
**示例**：
```starc
start = alloc(n * 4 + 8) ;     end = alloc(n * 4 + 8) ;     keybuf = alloc(2048) ;     tbuf = alloc(2048) ;     tcount = n ;     i = 0 ;     Solange i < n ;         raw_set32(start, i * 4, i) ;         raw_set32(end, i * 4, i + 1) ;         i = i + 1
```

### `tok_build_btu(btu, inv_btu)`
**功能**：构建 bytes_to_unicode 表 ----。
**示例**：
```starc
inbs = alloc(256) ;     mem_zero(inbs, 256) ;     b = 33 ;     Solange b <= 126 ;         raw_setbyte(inbs, b, 1) ;         b = b + 1
```

### `tok_decode(ids, n, vb, inv_btu)`
**功能**：解码 token ids -> UTF-8 C 串（新 C 串）----。
**示例**：
```starc
bytes_buf = alloc(32768) ;     bp = 0 ;     k = 0 ;     Solange k < n ;         idv = raw_get32(ids, k * 4) ;         cnt = raw_get64(vb, 8) ;         meta = raw_get64(vb, 40) ;         keypool = raw_get64(vb, 24) ;         e = 0 ;         Solange e < cnt ;             Wenn raw_get64(meta, e * 24 + 16) == idv dann ;                 eoff = raw_get64(meta, e * 24)
```

### `tok_encode(text_c, vb, rk, btu, ids, cap)`
**功能**：编码文本 -> token ids ----。
**示例**：
```starc
u = alloc(16384) ;     n = tok_utf8_u16(text_c, btu, u, 4096) ;     nids = 0 ;     Wenn n > 0 dann ;         nids = tok_bpe_word(u, n, vb, rk, ids, cap)
```

### `tok_hash16(seq, n)`
**功能**：哈希工具（FNV-1a 变体，u16 序列）----。
**示例**：
```starc
h = 5381 ;     i = 0 ;     Solange i < n ;         c = raw_byteat(seq, i * 2) + raw_byteat(seq, i * 2 + 1) * 8 * 8 * 4 ;         h = h * 8 * 4 + h + c ;         Solange h >= 262144 ;             h = h - 262144
```

### `tok_line(t)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck ptr_get32(t, 24)
```

### `tok_line_token(s, i, dst, out_n)`
**功能**：读一行中一个 token（UTF-8 -> 码点 u16 序列），写到 dst（cap 16 字符）。
**示例**：
```starc
nb = 0 ;     Solange raw_byteat(s, i) != 0 ;         c = raw_byteat(s, i) ;         Wenn c == 32 oder c == 9 oder c == 10 dann ;             raw_set64(out_n, 0, nb) ;             gebe_zurueck i
```

### `tok_new(ttype, text, num, line)`
**功能**：（功能描述未提取）。
**示例**：
```starc
t = alloc(32) ;     ptr_set32(t, 0, ttype) ;     ptr_set64(t, 8, text) ;     ptr_set32(t, 16, num) ;     ptr_set32(t, 24, line) ;     gebe_zurueck t
```

### `tok_num(t)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck ptr_get32(t, 16)
```

### `tok_parse_merges(m, rk)`
**功能**：解析 merges.txt（C 串）-> 哈希表 rk（rank = 行序，从 0 起）----。
**示例**：
```starc
i = 0 ;     rank = 0 ;     buf = alloc(512) ;     Solange raw_byteat(m, i) != 0 ;         a = alloc(256) ;         b = alloc(256) ;         out_n = alloc(8) ;         i = tok_line_token(m, i, a, out_n) ;         na = raw_get64(out_n, 0) ;         Solange raw_byteat(m, i) == 32 oder raw_byteat(m, i) == 9 ;             i = i + 1
```

### `tok_parse_vocab(v, vb)`
**功能**：解析 vocab.json（C 串）-> 哈希表 vb ----。
**示例**：
```starc
i = 0 ;     tokenbuf = alloc(512) ;     Solange raw_byteat(v, i) != 0 ;         c = raw_byteat(v, i) ;         Wenn c == 34 dann ;             j = i + 1 ;             nb = 0 ;             Solange raw_byteat(v, j) != 34 ;                 c = raw_byteat(v, j) ;                 Wenn c == 92 dann ;                     // 转义：\X -> X（JSON 里 token 只可能含 \" \\，近似按原字符） ;                     c2 = raw_byteat(v, j + 1)
```

### `tok_ranks_find(rk, key, klen)`
**功能**：查找 pair rank；未找到返回 1000000000。
**示例**：
```starc
buckets = raw_get64(rk, 0) ;     bucket = raw_get64(rk, 16) ;     keypool = raw_get64(rk, 24) ;     meta = raw_get64(rk, 40) ;     h = tok_hash16(key, klen) ;     idx = h - (h / buckets) * buckets ;     probes = 0 ;     Solange probes < buckets ;         cur = raw_get32(bucket, idx * 4) ;         Wenn cur == 0 dann ;             gebe_zurueck 1000000000 ;         Sonst
```

### `tok_ranks_init(rk, buckets)`
**功能**：ranks 哈希表：pair 拼接串(u16 序列) -> rank ----。
**示例**：
```starc
raw_set64(rk, 0, buckets) ;     raw_set64(rk, 8, 0) ;     b = alloc(buckets * 4) ;     mem_zero(b, buckets * 4) ;     raw_set64(rk, 16, b) ;     keypool = alloc(8000000) ;     raw_set64(rk, 24, keypool) ;     raw_set64(rk, 32, 0) ;     meta = alloc(200000 * 24) ;     raw_set64(rk, 40, meta) ;     gebe_zurueck 0
```

### `tok_ranks_insert(rk, key, klen, rank)`
**功能**：（功能描述未提取）。
**示例**：
```starc
buckets = raw_get64(rk, 0) ;     bucket = raw_get64(rk, 16) ;     keypool = raw_get64(rk, 24) ;     koff = raw_get64(rk, 32) ;     meta = raw_get64(rk, 40) ;     cnt = raw_get64(rk, 8) ;     raw_set64(keypool, koff, klen) ;     i = 0 ;     Solange i < klen ;         raw_setbyte(keypool, koff + 8 + i * 2, raw_byteat(key, i * 2)) ;         raw_setbyte(keypool, koff + 8 + i * 2 + 1, raw_byteat(key, i * 2 + 1)) ;         i = i + 1
```

### `tok_store(toks, idx, t)`
**功能**：把 token 指针存入 token 数组（每个槽 8 字节）。
**示例**：
```starc
ptr_set64(toks, idx * 8, t) ;     gebe_zurueck 0
```

### `tok_text(t)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck ptr_get64(t, 8)
```

### `tok_type(t)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck ptr_get32(t, 0)
```

### `tok_utf8_u16(s, btu, dst, cap)`
**功能**：文本(UTF-8 C 串) -> u16 序列（byte-level：逐字节经 btu 映射）----。
**示例**：
```starc
n = 0 ;     i = 0 ;     Solange raw_byteat(s, i) != 0 ;         c = raw_byteat(s, i) ;         nbytes = 1 ;         Wenn c >= 224 dann ;             nbytes = 3 ;         Sonst ;             Wenn c >= 192 dann ;                 nbytes = 2
```

### `tok_vocab_find(vb, seq, n, found)`
**功能**：查找 token -> id；found 指向 8 字节标志（1=找到）。
**示例**：
```starc
buckets = raw_get64(vb, 0) ;     bucket = raw_get64(vb, 16) ;     keypool = raw_get64(vb, 24) ;     meta = raw_get64(vb, 40) ;     h = tok_hash16(seq, n) ;     idx = h - (h / buckets) * buckets ;     probes = 0 ;     Solange probes < buckets ;         cur = raw_get32(bucket, idx * 4) ;         Wenn cur == 0 dann ;             raw_set64(found, 0, 0) ;             gebe_zurueck 0
```

### `tok_vocab_init(vb, buckets)`
**功能**：vocab 哈希表：token(u16 序列) -> id ----。
**示例**：
```starc
raw_set64(vb, 0, buckets) ;     raw_set64(vb, 8, 0) ;     b = alloc(buckets * 4) ;     mem_zero(b, buckets * 4) ;     raw_set64(vb, 16, b) ;     keypool = alloc(8000000) ;     raw_set64(vb, 24, keypool) ;     raw_set64(vb, 32, 0) ;     meta = alloc(200000 * 24) ;     raw_set64(vb, 40, meta) ;     gebe_zurueck 0
```

### `tok_vocab_insert(vb, seq, n, id)`
**功能**：（功能描述未提取）。
**示例**：
```starc
buckets = raw_get64(vb, 0) ;     bucket = raw_get64(vb, 16) ;     keypool = raw_get64(vb, 24) ;     koff = raw_get64(vb, 32) ;     meta = raw_get64(vb, 40) ;     cnt = raw_get64(vb, 8) ;     raw_set64(keypool, koff, n) ;     i = 0 ;     Solange i < n ;         raw_setbyte(keypool, koff + 8 + i * 2, raw_byteat(seq, i * 2)) ;         raw_setbyte(keypool, koff + 8 + i * 2 + 1, raw_byteat(seq, i * 2 + 1)) ;         i = i + 1
```

### `truncate_str(str, max_len)`
**功能**：截断字符串到最大长度（用于128K限制）。
**示例**：
```starc
len = str_len(str) ;     Wenn len <= max_len dann ;         gebe_zurueck str
```

### `tz_encode(s, slen, syms, btc)`
**功能**：输入 UTF-8 字节串 -> 符号流 (u32 数组 syms), 用 byte_to_char 表 (btc, 256 u16)。
**示例**：
```starc
n = 0 ;     i = 0 ;     Solange i < slen ;         b = raw_byteat(s, i) ;         raw_set32(syms, n * 4, tz_get16(btc, b * 2)) ;         n = n + 1 ;         i = i + 1
```

### `tz_get16(ptr, off)`
**功能**：小端 16 位读 (基于 raw_byteat, 支持 mmap) ----。
**示例**：
```starc
v = raw_byteat(ptr, off) ;     v = v + raw_byteat(ptr, off + 1) * 256 ;     gebe_zurueck v
```

### `tz_hash16(p, n)`
**功能**：哈希 (乘加取模, StarC 无位运算) ----。
**示例**：
```starc
h = 2166136261 ;     i = 0 ;     Solange i < n ;         h = h * 16777619 ;         h = (h + tz_get16(p, i * 2)) % 4294967296 ;         i = i + 1
```

### `tz_hash32(p, n)`
**功能**：u32 符号序列 (p 每符号 4 字节)。
**示例**：
```starc
h = 2166136261 ;     i = 0 ;     Solange i < n ;         h = h * 16777619 ;         h = (h + raw_get32(p, i * 4)) % 4294967296 ;         i = i + 1
```

### `tz_init(tbuf, rbuf, hashbuf, rrankbuf)`
**功能**：初始化 vocab 哈希表 + merges_rank 哈希表。
**示例**：
```starc
cap = 1048576 ;     base = raw_get64(tbuf, 0) ;     vocab_size = raw_get32(base, 0) ;     vb = raw_get32(base, 8) ;     mb = raw_get32(base, 12) ;     vstart = base + 16 ;     mstart = vstart + vb ;     btc = mstart + mb ;     // vocab 哈希表: key = u16 符号序列 ;     i = 0 ;     Solange i < cap ;         raw_set32(hashbuf, i * 12 + 8, 1)
```

### `tz_lookup(hashbuf, p, n)`
**功能**：查 vocab: p 是 u32 符号序列。
**示例**：
```starc
cap = 1048576 ;     h = tz_hash32(p, n) ;     slot = h % cap ;     i = 0 ;     Solange i < cap ;         Wenn raw_get32(hashbuf, slot * 12 + 8) == 1 dann ;             gebe_zurueck -1
```

### `tz_rank(rrankbuf, ap, la, bp, lb)`
**功能**：查 merges_rank: a,b 是 u32 符号序列 -> rank, 未命中 -1。
**示例**：
```starc
cap = 1048576 ;     // 拼接 key = a 符号 + b 符号 (u32) ;     kl = la + lb ;     kbuf = alloc(kl * 4) ;     j = 0 ;     Solange j < la ;         raw_set32(kbuf, j * 4, raw_get32(ap, j * 4)) ;         j = j + 1
```

### `tz_strdata(s, slen)`
**功能**：把 StarC 字符串 (8字节长度头+数据) 复制成 alloc 原始缓冲 (数据从 offset 0)。
**示例**：
```starc
buf = alloc(slen) ;     i = 0 ;     Solange i < slen ;         raw_setbyte(buf, i, raw_byteat(s, 8 + i)) ;         i = i + 1
```

### `tz_tokenize(hashbuf, rrankbuf, syms, nsym, ids)`
**功能**：BPE: 符号流 -> token id (标准算法, 每轮找最小 rank 对)。
**示例**：
```starc
st = alloc(nsym * 4) ;     en = alloc(nsym * 4) ;     i = 0 ;     Solange i < nsym ;         raw_set32(st, i * 4, i) ;         raw_set32(en, i * 4, i + 1) ;         i = i + 1
```

### `tz_tokenize_pt(s, slen, hashbuf, rrankbuf, btc, ids, cps, coffs, pst, pen, cap)`
**功能**：完整 tokenize (pre-tokenize + 逐piece BPE) ----。
**示例**：
```starc
ncp_tot = pt_utf8_decode(s, slen, cps, coffs) ;     npiece = pt_pretokenize(s, slen, cps, coffs, pst, pen, cap) ;     ntok = 0 ;     pi = 0 ;     Solange pi < npiece ;         a = raw_get32(pst, pi * 4) ;         b = raw_get32(pen, pi * 4) ;         ba = raw_get32(coffs, a * 4) ;         Wenn b == ncp_tot dann ;             bb = slen ;         Sonst ;             bb = raw_get32(coffs, b * 4)
```

### `u32r(buf, off)`
**功能**：读 u32（小端）----。
**示例**：
```starc
v = raw_byteat(buf, off + 0) ;     v = v + raw_byteat(buf, off + 1) * 256 ;     v = v + raw_byteat(buf, off + 2) * 65536 ;     v = v + raw_byteat(buf, off + 3) * 16777216 ;     gebe_zurueck v
```

### `u32w(buf, off, v)`
**功能**：写 u32 到 buf 偏移 off（小端）----。
**示例**：
```starc
raw_setbyte(buf, off + 0, v - (v / 256) * 256) ;     raw_setbyte(buf, off + 1, (v / 256) - (v / 65536) * 256) ;     raw_setbyte(buf, off + 2, (v / 65536) - (v / 16777216) * 256) ;     raw_setbyte(buf, off + 3, v / 16777216) ;     gebe_zurueck 0
```

### `u_bf16_to_f32(src, dst, n)`
**功能**：BF16 → FP32：src 裸指针（BF16 数组，n 个），写 FP32 到 dst ----。
**示例**：
```starc
i = 0 ;     Solange i < n ;         lo = raw_byteat(src, i * 2) ;         hi = raw_byteat(src, i * 2 + 1) ;         raw_setbyte(dst, i * 4, 0) ;         raw_setbyte(dst, i * 4 + 1, 0) ;         raw_setbyte(dst, i * 4 + 2, lo) ;         raw_setbyte(dst, i * 4 + 3, hi) ;         i = i + 1
```

### `u_down_el(inter, hidden)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck hidden * inter
```

### `u_el(vocab, hidden, kv, inter, n_heads)`
**功能**：每张量元素数（FP32 个数）----。
**示例**：
```starc
dim = hidden / n_heads ;     gebe_zurueck 0
```

### `u_emit(fh, stf, hdr, tname_c, n)`
**功能**：复制一个张量（BF16→FP32）并写入文件 ----。
**示例**：
```starc
o0 = alloc(8) ;     o1 = alloc(8) ;     r = js_tensor_offsets(hdr, tname_c, o0, o1) ;     Wenn r != 0 dann ;         mg_write_str("!! missing tensor: ") ;         mg_write_cstr(tname_c) ;         mg_newline() ;         gebe_zurueck 0 - 1
```

### `u_gate_el(inter, hidden)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck inter * hidden
```

### `u_header_ok(base, total_w, total_b)`
**功能**：返回 1 合法, 0 非法 ----。
**示例**：
```starc
Wenn raw_get32(base, 0) != u_magic() dann ;         gebe_zurueck 0
```

### `u_kb_el(kv, hidden, n_heads)`
**功能**：（功能描述未提取）。
**示例**：
```starc
dim = hidden / n_heads ;     gebe_zurueck kv * dim
```

### `u_kw_el(kv, hidden, n_heads)`
**功能**：（功能描述未提取）。
**示例**：
```starc
dim = hidden / n_heads ;     gebe_zurueck kv * dim * hidden
```

### `u_ln1_el(hidden)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck hidden
```

### `u_ln2_el(hidden)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck hidden
```

### `u_lnf_el(hidden)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck hidden
```

### `u_magic()`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck 826359381  // 0x31574655
```

### `u_ow_el(hidden)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck hidden * hidden
```

### `u_qb_el(hidden)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck hidden
```

### `u_qw_el(hidden)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck hidden * hidden
```

### `u_tensor_el(kind, vocab, hidden, kv, inter, n_heads)`
**功能**：单个权重张量的元素数（kind 编号：0=wte 1=ln1 2=qw 3=qb 4=kw 5=kb。
**示例**：
```starc
Wenn kind == 0 dann ;         gebe_zurueck vocab * hidden ;     Sonst ;         Wenn kind == 1 dann ;             gebe_zurueck hidden ;         Sonst ;             Wenn kind == 2 dann ;                 gebe_zurueck hidden * hidden ;             Sonst ;                 Wenn kind == 3 dann ;                     gebe_zurueck hidden ;                 Sonst
```

### `u_total_elems(vocab, hidden, kv, inter, n_heads, layers, tied)`
**功能**：总权重元素数（FP32 个数）----。
**示例**：
```starc
tot = vocab * hidden          // wte ;     l = 0 ;     Solange l < layers ;         tot = tot + hidden                    // ln1 ;         tot = tot + hidden * hidden           // qw ;         tot = tot + hidden                    // qb ;         tot = tot + u_kw_el(kv, hidden, n_heads)   // kw ;         tot = tot + u_kb_el(kv, hidden, n_heads)   // kb ;         tot = tot + u_vw_el(kv, hidden, n_heads)   // vw ;         tot = tot + u_vb_el(kv, hidden, n_heads)   // vb ;         tot = tot + hidden * hidden           // ow ;         tot = tot + hidden                    // ln2
```

### `u_up_el(inter, hidden)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck inter * hidden
```

### `u_vb_el(kv, hidden, n_heads)`
**功能**：（功能描述未提取）。
**示例**：
```starc
dim = hidden / n_heads ;     gebe_zurueck kv * dim
```

### `u_version()`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck 1
```

### `u_vw_el(kv, hidden, n_heads)`
**功能**：（功能描述未提取）。
**示例**：
```starc
dim = hidden / n_heads ;     gebe_zurueck kv * dim * hidden
```

### `u_write_f32(fh, src, n)`
**功能**：写一段 FP32 到文件（fh 已打开，位置自动前进）----。
**示例**：
```starc
nb = alloc(8) ;     mem_zero(nb, 8) ;     WriteFile(fh, src, n * 4, nb, 0) ;     gebe_zurueck 0
```

### `u_write_header(fh, vocab, hidden, layers, n_heads, kv, inter, max_pos, head_dim, eps_bits, theta_bits, tied, total_elems)`
**功能**：写 UWF1 头（32B Header + 56B Meta = 88B），fh 为已打开的文件句柄 ----。
**示例**：
```starc
h = alloc(88) ;     mem_zero(h, 88) ;     // Header 32B ;     raw_setbyte(h, 0, 77)        // 'M' ;     raw_setbyte(h, 1, 49)        // '1' ;     raw_setbyte(h, 2, 85)        // 'U' ;     raw_setbyte(h, 3, 70)        // 'F' ;     raw_setbyte(h, 4, 1)         // version=1 ;     // [8..12) 权重张量数（固定布局：wte + 13*L + lnf [+lm_head]） ;     n_t = 1 + 13 * layers + 1 ;     Wenn tied == 0 dann ;         n_t = n_t + 1
```

### `umgebung_alle()`
**功能**：Alle Umgebungsvariablen ausgeben。
**示例**：
```starc
block = GetEnvironmentStringsA() ;     pos = 0 ;     Solange raw_byteat(block, pos) != 0 ;         write_cstr(block + pos) ;         write_newline() ;         Solange raw_byteat(block, pos) != 0 ;             pos = pos + 1
```

### `umgebung_get(name)`
**功能**：------ Umgebungsvariable lesen ----------。
**示例**：
```starc
c = starc_to_cstr(name) ;     buf = alloc(4096) ;     n = GetEnvironmentVariableA(c, buf, 4096) ;     gebe_zurueck cstr_to_starc(buf)
```

### `umgebung_set(name, wert)`
**功能**：------ Umgebungsvariable setzen (Prozess) ----------。
**示例**：
```starc
c = starc_to_cstr(name) ;     w = starc_to_cstr(wert) ;     r = SetEnvironmentVariableA(c, w) ;     gebe_zurueck r
```

### `umgebung_setzen(args)`
**功能**：Umgebungsvariable setzen。
**示例**：
```starc
name = extract_arg(args, 0) ;     wert = argument_n(args, 0, 2) ;     Wenn str_len(name) == 0 dann ;         write_str("set <Name> <Wert>") ;         write_newline() ;         gebe_zurueck 1
```

### `umgebung_zeigen(args)`
**功能**：Umgebungsvariable ausgeben (Name=Wert)。
**示例**：
```starc
name = extract_arg(args, 0) ;     Wenn str_len(name) == 0 dann ;         umgebung_alle() ;         gebe_zurueck 1
```

### `unpack_h1(p)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck p / 4294967296
```

### `unpack_h2(p)`
**功能**：（功能描述未提取）。
**示例**：
```starc
gebe_zurueck p % 4294967296
```

### `uptime_sekunden()`
**功能**：------ Uptime in Sekunden ----------。
**示例**：
```starc
gebe_zurueck GetTickCount() / 1000
```

### `url_decode(str)`
**功能**：（功能描述未提取）。
**示例**：
```starc
len = str_len(str) ;     result = alloc(8 + len + 1) ;     pos = 0 ;     i = 0 ;     Solange i < len ;         ch = str_at(str, i) ;         Wenn ch == 37 und i + 2 < len dann ;             h1 = str_at(str, i + 1) ;             h2 = str_at(str, i + 2) ;             high = hex_digit_val(h1) ;             low = hex_digit_val(h2) ;             Wenn high != -1 und low != -1 dann
```

### `url_encode(str)`
**功能**：（功能描述未提取）。
**示例**：
```starc
len = str_len(str) ;     result = alloc(8 + len * 3 + 1) ;     pos = 0 ;     i = 0 ;     Solange i < len ;         ch = str_at(str, i) ;         Wenn ch == 32 dann ;             raw_setbyte(result, 8 + pos, 37) ;             pos = pos + 1 ;             raw_setbyte(result, 8 + pos, 50) ;             pos = pos + 1 ;             raw_setbyte(result, 8 + pos, 48)
```

### `uv_load(path, out)`
**功能**：.Uv 加载器: mmap 并解析 header, 填 out ----。
**示例**：
```starc
m = alloc(32) ;     r = file_mmap(path, m) ;     Wenn r != 1 dann ;         gebe_zurueck 0
```

### `uv_unload(out)`
**功能**：.Uv 卸载: 释放 uv_load 的 mmap ----。
**示例**：
```starc
mp = alloc(32) ;     raw_set64(mp, 0, raw_get64(out, 80)) ;     raw_set64(mp, 8, raw_get64(out, 88)) ;     raw_set64(mp, 16, raw_get64(out, 96)) ;     raw_set64(mp, 24, raw_get64(out, 104)) ;     file_munmap(mp) ;     gebe_zurueck 0
```

### `uv_write(path, points, faces, cell_idx, cell_cnt, nV, nF, V, occ, nPrim, flags)`
**功能**：.Uv 写入器: 把网云(点云+面云+体网络)写为 .Uv 文件 ----。
**示例**：
```starc
h = file_open_sh(path, 1073741824, 2)  // GENERIC_WRITE + CREATE_ALWAYS ;     Wenn h == -1 dann ;         gebe_zurueck 0
```

### `v4c70b_cfg(c)   // c: 参数表缓冲 (每项 8 字节)`
**功能**：（功能描述未提取）。
**示例**：
```starc
// 通用 ;     raw_set64(c, 0,   4096)      // hidden_size ;     raw_set64(c, 8,   28)        // num_hidden_layers (教师 43 -> 28) ;     raw_set64(c, 16,  64)        // num_attention_heads ;     raw_set64(c, 24,  512)       // head_dim ;     raw_set64(c, 32,  1)         // num_key_value_heads (MLA) ;     raw_set64(c, 40,  1024)      // q_lora_rank ;     raw_set64(c, 48,  1024)      // o_lora_rank ;     raw_set64(c, 56,  64)        // qk_rope_head_dim ;     // MoE (关键: moe_inter 与教师一致 2048) ;     raw_set64(c, 64,  2048)      // moe_intermediate_size (教师 2048, 直接复制) ;     raw_set64(c, 72,  92)        // n_routed_experts (教师 256 -> 选 92)
```

### `vae_acc(vox, prob, V)`
**功能**：prob>0.5 与 x 一致的体素比例 (0~1) ----。
**示例**：
```starc
n = V * V * V ;     ok = 0 ;     i = 0 ;     Solange i < n ;         p = t_get(prob, i) ;         y = t_get(vox, i) ;         pred = 0 ;         Wenn p > 1056964608 dann  // p > 0.5 ;             pred = 1065353216
```

### `vae_bce(vox, prob, V)`
**功能**：仅 BCE (平均每体素), 诊断用 ----。
**示例**：
```starc
n = V * V * V ;     bce = 0 ;     i = 0 ;     Solange i < n ;         p = t_get(prob, i) ;         y = t_get(vox, i) ;         pmin = 9900000 ;         pmax = 1059900000 ;         p2 = p ;         Wenn p2 < pmin dann ;             p2 = pmin
```

### `vae_decode(z, w, b, prob, z_dim, V, tmp1, tmp2, tmp3, tmp4)`
**功能**：z (z_dim) -> 重建体素概率 (V^3) ----。
**示例**：
```starc
hdim = 32 * (V / 4) * (V / 4) * (V / 4) ;     // fc: flat = fc_dw @ z + fc_db ;     j = 0 ;     Solange j < hdim ;         acc = 0 ;         k = 0 ;         Solange k < z_dim ;             acc = f32_add(acc, f32_mul(t_get(z, k), t_get(w, j * z_dim + k))) ;             k = k + 1
```

### `vae_encode(vox, w, b, mu, logvar, V, z_dim, tmp1, tmp2, tmp3)`
**功能**：输入体素 (V^3) -> 均值 mu (z_dim), 对数方差 logvar (z_dim) ----。
**示例**：
```starc
// V^3 输入, enc1 -> 8ch V^3 ;     conv3d_pad1_relu(vox, w, b, tmp1, 1, V, V, V, 8) ;     // enc2 stride2 -> 16ch (V/2)^3 ;     conv3d_s2_relu(tmp1, w + 216 * 4, b + 8 * 4, tmp2, 8, V, V, V, 16) ;     // enc3 stride2 -> 32ch (V/4)^3 ;     conv3d_s2_relu(tmp2, w + 3672 * 4, b + 24 * 4, tmp3, 16, V / 2, V / 2, V / 2, 32) ;     // flatten -> fc ;     hdim = 32 * (V / 4) * (V / 4) * (V / 4) ;     // fc: mu/logvar = fc_w @ flat + fc_b   (fc_w 在编码器权重偏移 17496 元素) ;     k = 0 ;     Solange k < z_dim ;         acc = 0
```

### `vae_kl(mu, logvar, z_dim)`
**功能**：仅 KL ----。
**示例**：
```starc
kl = 0 ;     i = 0 ;     Solange i < z_dim ;         m = t_get(mu, i) ;         lv = t_get(logvar, i) ;         elv = f32_exp(lv) ;         term = f32_sub(f32_add(f32_mul(m, m), elv), f32_add(lv, 1065353216)) ;         kl = f32_add(kl, term) ;         i = i + 1
```

### `vae_loss(vox, prob, mu, logvar, V, z_dim, beta)`
**功能**：重建损失 (BCE) + KL: loss = BCE + beta*KL ----。
**示例**：
```starc
n = V * V * V ;     bce = 0 ;     i = 0 ;     Solange i < n ;         p = t_get(prob, i) ;         y = t_get(vox, i) ;         // BCE = -(y*log(p) + (1-y)*log(1-p)), 裁剪防 log(0) ;         // 裁剪 p 到 [1e-7, 1-1e-7] ;         pmin = 9900000  // ~1e-7 fp32 近似 ;         pmax = 1059900000 ;         p2 = p ;         Wenn p2 < pmin dann
```

### `vae_relu(x)`
**功能**：ReLU ----。
**示例**：
```starc
Wenn (x / 2147483648) == 1 dann ;         gebe_zurueck 0
```

### `vae_reparam(zbuf, mu, logvar, z_dim, seed)`
**功能**：z = mu + sigma * eps (eps ~ N(0,1) 用简化伪随机) ----。
**示例**：
```starc
// sigma = exp(0.5*logvar) ;     i = 0 ;     Solange i < z_dim ;         lv = t_get(logvar, i) ;         sigma = f32_exp(f32_mul(lv, 1056964608))  // exp(0.5*logvar) ;         // 伪随机 eps in [-1,1] (确定性, 基于 seed+i) ;         r = (seed * 2654435761 + i * 40503) % 65536 ;         eps = f32_sub(f32_mul(s32_to_f32(r % 32768), f32_div(s32_to_f32(2), s32_to_f32(32767))), 1065353216) ;         zv = f32_add(t_get(mu, i), f32_mul(sigma, eps)) ;         t_set(zbuf, i, zv) ;         i = i + 1
```

### `vae_sigmoid(x)`
**功能**：sigmoid: 1/(1+exp(-x)) ----。
**示例**：
```starc
// exp(-x) ;     nx = fs_neg(x) ;     e = f32_exp(nx) ;     o = f32_add(e, 1065353216) ;     gebe_zurueck f32_div(1065353216, o)
```

### `vb_sig_deriv(y)`
**功能**：sigmoid 导数: dy/dlogit = y*(1-y) ----。
**示例**：
```starc
onem = f32_sub(1065353216, y) ;     gebe_zurueck f32_mul(y, onem)
```

### `vga_clear()`
**功能**：清屏。
**示例**：
```starc
gebe_zurueck 0
```

### `vga_putc(ch, row, col)`
**功能**：===== VGA 文本模式能力 (80x25, 0xB8000) =====。
**示例**：
```starc
gebe_zurueck 0
```

### `vga_str(s)`
**功能**：写字符串到当前行 (codegen 编译为循环, 能力在 .staro)。
**示例**：
```starc
gebe_zurueck 0
```

### `voxel_fill_flood(vox, V)`
**功能**：从网格边界 BFS 标记"外部"空体素, 未被标记的空体素=内部 -> 填充 ----。
**示例**：
```starc
n = V * V * V ;     visited = alloc(n) ;     queue = alloc(n * 4) ;     i = 0 ;     Solange i < n ;         Wenn t_get(vox, i) != 0 dann ;             raw_setbyte(visited, i, 2) ;         Sonst ;             raw_setbyte(visited, i, 0)
```

### `voxel_fill_flood_ws(vox, V, visited, queue)`
**功能**：泛洪填充内部 (预分配工作区版): visited/queue 由调用方提供 ----。
**示例**：
```starc
n = V * V * V ;     i = 0 ;     Solange i < n ;         Wenn t_get(vox, i) != 0 dann ;             raw_setbyte(visited, i, 2) ;         Sonst ;             raw_setbyte(visited, i, 0)
```

### `voxel_fill_interior(vox, V)`
**功能**：每 (y,z) 行沿 x 扫描, 在成对表面之间填充 ----。
**示例**：
```starc
y = 0 ;     Solange y < V ;         z = 0 ;         Solange z < V ;             // 沿 x 扫描, 找到第一个和最后一个占据, 中间填满 ;             first = -1 ;             last = -1 ;             x = 0 ;             Solange x < V ;                 idx = (z * V + y) * V + x ;                 Wenn t_get(vox, idx) != 0 dann ;                     Wenn first == -1 dann
```

### `voxelize_face(verts, v0, v1, v2, vox, V, samples)`
**功能**：面片采样体素化 (三角形) ----。
**示例**：
```starc
ax = t_get(verts, v0 * 3 + 0) ;     ay = t_get(verts, v0 * 3 + 1) ;     az = t_get(verts, v0 * 3 + 2) ;     bx = t_get(verts, v1 * 3 + 0) ;     by = t_get(verts, v1 * 3 + 1) ;     bz = t_get(verts, v1 * 3 + 2) ;     cx = t_get(verts, v2 * 3 + 0) ;     cy = t_get(verts, v2 * 3 + 1) ;     cz = t_get(verts, v2 * 3 + 2) ;     si = 0 ;     Solange si <= samples ;         sj = 0
```

### `voxelize_face_adaptive(verts, v0, v1, v2, vox, V)`
**功能**：按三角形体素边长选 samples (间距 <= 0.5 体素) ----。
**示例**：
```starc
ax = t_get(verts, v0 * 3 + 0) ;     ay = t_get(verts, v0 * 3 + 1) ;     az = t_get(verts, v0 * 3 + 2) ;     bx = t_get(verts, v1 * 3 + 0) ;     by = t_get(verts, v1 * 3 + 1) ;     bz = t_get(verts, v1 * 3 + 2) ;     cx = t_get(verts, v2 * 3 + 0) ;     cy = t_get(verts, v2 * 3 + 1) ;     cz = t_get(verts, v2 * 3 + 2) ;     // 用平方边长取最大边 (避免 3 次 sqrt), 只 sqrt 一次; 结果与 edge_len 一致 ;     d1x = f32_sub(ax, bx) ;     d1y = f32_sub(ay, by)
```

### `w32(p, off, v)`
**功能**：写 4 字节小端到 p+off。
**示例**：
```starc
raw_setbyte(p, off + 0, v - (v / 256) * 256) ;     raw_setbyte(p, off + 1, (v / 256) - (v / 65536) * 256) ;     raw_setbyte(p, off + 2, (v / 65536) - (v / 16777216) * 256) ;     raw_setbyte(p, off + 3, v / 16777216) ;     gebe_zurueck 0
```

### `warte_ms(ms)`
**功能**：------ Warten in Millisekunden ----------。
**示例**：
```starc
Sleep(ms) ;     gebe_zurueck 0
```

### `wf_set32(ptr, off, val)`
**功能**：小端 32 位写（written 指针用）。
**示例**：
```starc
raw_setbyte(ptr, off, val % 256) ;     raw_setbyte(ptr, off + 1, (val / 256) % 256) ;     raw_setbyte(ptr, off + 2, (val / 65536) % 256) ;     raw_setbyte(ptr, off + 3, (val / 16777216) % 256) ;     gebe_zurueck 0
```

### `wi_find_tensor(head_buf, n, want_buf, want_s, want_l, out)`
**功能**：按名字查找 tensor ----。
**示例**：
```starc
pos = 0 ;     o2 = alloc(88) ;     pos = st_next_tensor(head_buf, n, pos, o2) ;     Solange pos != 0 ;         name_s = raw_get64(o2, 8) ;         name_l = raw_get64(o2, 16) ;         Wenn name_l == want_l dann ;             match = 1 ;             k = 0 ;             Solange k < want_l ;                 Wenn raw_byteat(head_buf, name_s + k) != raw_byteat(want_buf, want_s + k) dann ;                     match = 0
```

### `worker0()`
**功能**：（功能描述未提取）。
**示例**：
```starc
worker_impl(0) ;     gebe_zurueck 0
```

### `worker1()`
**功能**：（功能描述未提取）。
**示例**：
```starc
worker_impl(1) ;     gebe_zurueck 0
```

### `worker2()`
**功能**：（功能描述未提取）。
**示例**：
```starc
worker_impl(2) ;     gebe_zurueck 0
```

### `worker3()`
**功能**：（功能描述未提取）。
**示例**：
```starc
worker_impl(3) ;     gebe_zurueck 0
```

### `worker_impl(t)`
**功能**：（功能描述未提取）。
**示例**：
```starc
h = shm_create() ;     base = shm_map(h) ;     n = read_u64(base, 0) ;     write_u64(base, 16 + t * 8, 0)          // 结果槽清零 ;     count = 0 ;     v = t ;     Solange v < n ;         prim = is_prime(v) ;         Wenn prim == 1 dann ;             count = count + 1
```

### `write_bytes(handle, data, size)`
**功能**：write_bytes(handle, data, size) - 写入字节到文件。
**示例**：
```starc
// 分配缓冲区接收写入字节数 ;     rn = alloc(4) ;     raw_setbyte(rn, 0, 0) ;     raw_setbyte(rn, 1, 0) ;     raw_setbyte(rn, 2, 0) ;     raw_setbyte(rn, 3, 0) ;  ;     ok = WriteFile(handle, data, size, rn, 0) ;  ;     Wenn ok == 0 dann ;         gebe_zurueck 0
```

### `write_cstr(c)`
**功能**：（功能描述未提取）。
**示例**：
```starc
n = 0 ;     Solange raw_byteat(c, n) != 0 ;         n = n + 1
```

### `write_file(path, content)`
**功能**：写入字符串到文件。
**示例**：
```starc
path_cstr = to_cstr(path) ;     content_cstr = to_cstr(content) ;  ;     handle = CreateFileA(path_cstr, 1073741824, 0, 0, 2, 0, 0) ;  ;     Wenn handle == -1 dann ;         gebe_zurueck 0
```

### `write_hash(p)`
**功能**：（功能描述未提取）。
**示例**：
```starc
write_str("check = ") ;     write_int(unpack_h1(p)) ;     write_str(":") ;     write_int(unpack_h2(p)) ;     write_newline() ;     gebe_zurueck 0
```

### `write_i64(buf, off, v)`
**功能**：有符号 int64 小端写入（二进制补码）。write_u64 已符号安全（迭代借位法），直接委托。
**示例**：
```starc
gebe_zurueck write_u64(buf, off, v)
```

### `write_int(num)`
**功能**：（功能描述未提取）。
**示例**：
```starc
write_str(int_to_str(num)) ;     gebe_zurueck 0
```

### `write_newline()`
**功能**：（功能描述未提取）。
**示例**：
```starc
nl = alloc(1) ;     raw_setbyte(nl, 0, 10) ;     WriteFile(get_stdout(), nl, 1, 0, 0) ;     gebe_zurueck 0
```

### `write_s32(ptr, off, v)`
**功能**：写有符号 32 位 (补码) 到缓冲. raw_set32 对负数有 bug (C 除法/取模符号不一致),。
**示例**：
```starc
Wenn v >= 0 dann ;         raw_set32(ptr, off, v) ;     Sonst ;         raw_set32(ptr, off, 4294967296 + v)
```

### `write_str(s)`
**功能**：（功能描述未提取）。
**示例**：
```starc
WriteFile(get_stdout(), s + 8, str_len(s), 0, 0) ;     gebe_zurueck 0
```

### `write_u32(buf, offset, value)`
**功能**：write_u32(buf, offset, value) - 写入32位整数到缓冲区。
**示例**：
```starc
raw_setbyte(buf, offset, value % 256) ;     raw_setbyte(buf, offset + 1, (value / 256) % 256) ;     raw_setbyte(buf, offset + 2, (value / 65536) % 256) ;     raw_setbyte(buf, offset + 3, (value / 16777216) % 256) ;     gebe_zurueck 0
```

### `write_u32_le(buf, off, v)`
**功能**：（功能描述未提取）。
**示例**：
```starc
raw_setbyte(buf, off + 0, v % 256) ;     raw_setbyte(buf, off + 1, (v / 256) % 256) ;     raw_setbyte(buf, off + 2, (v / 65536) % 256) ;     raw_setbyte(buf, off + 3, (v / 16777216) % 256) ;     gebe_zurueck 0
```

### `write_u64(buf, off, v)`
**功能**：（功能描述未提取）。
**示例**：
```starc
// 符号安全：对负 v 用迭代借位法写出二进制补码（StarC % 带符号、/ 向零截断）。 ;     b = v % 256 ;     Wenn b < 0 dann ;         b = b + 256
```

### `ws_ntohs(v)`
**功能**：（功能描述未提取）。
**示例**：
```starc
r = ntohs(v) ;     gebe_zurueck r
```

### `ws_socket()`
**功能**：（功能描述未提取）。
**示例**：
```starc
s = socket(2, 1, 6) ;     gebe_zurueck s
```

### `x86_read_cr2()`
**功能**：读取 CR2 (页错误地址)。
**示例**：
```starc
gebe_zurueck 0
```

### `x_func_epilogue(out, off)`
**功能**：mov rsp,rbp(48 89 EC); pop rbp(5D); ret(C3)。
**示例**：
```starc
off = b8(out, off, 72)         // 48 ;     off = b8(out, off, 137)        // 89 ;     off = b8(out, off, 236)        // EC ;     off = b8(out, off, 93)         // 5D pop rbp ;     off = b8(out, off, 195)        // C3 ret ;     gebe_zurueck off
```

### `x_func_prologue(out, off, framesize)`
**功能**：push rbp(55); mov rbp,rsp(48 89 E5); sub rsp,framesize(48 81 EC i32)。
**示例**：
```starc
off = b8(out, off, 85)         // push rbp ;     off = b8(out, off, 72)         // 48 ;     off = b8(out, off, 137)        // 89 ;     off = b8(out, off, 229)        // E5: mov rbp,rsp ;     off = b8(out, off, 72)         // 48 ;     off = b8(out, off, 129)        // 81 ;     off = b8(out, off, 236)        // EC: sub rsp,imm32 ;     off = b32(out, off, framesize) ;     gebe_zurueck off
```

### `x_jmp_back(out, off, target)`
**功能**：发射 jmp rel32 回已知目标 (loop 回跳)。
**示例**：
```starc
off = b8(out, off, 233) ;     rel = target - (off + 4) ;     off = b8(out, off, rel % 256) ;     off = b8(out, off, (rel / 256) % 256) ;     off = b8(out, off, (rel / 65536) % 256) ;     off = b8(out, off, (rel / 16777216) % 256) ;     gebe_zurueck off
```

### `x_jmp_fwd(out, off, fixbuf, cgst)`
**功能**：发射 jmp rel32 (E9) 到未知目标。
**示例**：
```starc
off = b8(out, off, 233) ;     raw_set32(fixbuf, raw_get32(cgst, 0) * 8, off) ;     raw_set32(cgst, 0, raw_get32(cgst, 0) + 1) ;     off = b8(out, off, 0) ;     off = b8(out, off, 0) ;     off = b8(out, off, 0) ;     off = b8(out, off, 0) ;     gebe_zurueck off
```

### `x_jz_fwd(out, off, fixbuf, cgst)`
**功能**：发射 jz rel32 (0F 84) 到未知目标; fixbuf[fx] = rel 字段位置(回填用); fx 由 cgst[0] 维护。
**示例**：
```starc
off = b8(out, off, 15) ;     off = b8(out, off, 132) ;     raw_set32(fixbuf, raw_get32(cgst, 0) * 8, off) ;     raw_set32(cgst, 0, raw_get32(cgst, 0) + 1) ;     off = b8(out, off, 0) ;     off = b8(out, off, 0) ;     off = b8(out, off, 0) ;     off = b8(out, off, 0) ;     gebe_zurueck off
```

### `x_patch_rel32(out, patch_off, target)`
**功能**：在 patch_off 处写 rel32 = target - (patch_off+4)。
**示例**：
```starc
rel = target - (patch_off + 4) ;     raw_setbyte(out, patch_off, rel % 256) ;     raw_setbyte(out, patch_off + 1, (rel / 256) % 256) ;     raw_setbyte(out, patch_off + 2, (rel / 65536) % 256) ;     raw_setbyte(out, patch_off + 3, (rel / 16777216) % 256) ;     gebe_zurueck 0
```

### `x_push_imm32(out, off, v)   // push imm32: 68 imm32 (符号扩展)`
**功能**：（功能描述未提取）。
**示例**：
```starc
off = b8(out, off, 104) ;     off = b32(out, off, v) ;     gebe_zurueck off
```

### `x_push_rbp_disp(out, off, disp8)`
**功能**：push [rbp - disp8]  (FF 75 disp8); disp8 为 0..127 内无符号, 表示负偏移。
**示例**：
```starc
off = b8(out, off, 255)   // FF ;     off = b8(out, off, 117)   // 75: /6 push r/m64, modrm=01 110 101 (rbp+disp8) ;     off = b8(out, off, disp8) ;     gebe_zurueck off
```

### `x_store_rbp_disp(out, off, slot)`
**功能**：写 int 结果到变量槽: mov [rbp - slot], rax   (48 89 45 disp8, disp8=(-slot)&0xFF 即 256-slot)。
**示例**：
```starc
off = b8(out, off, 72)        // 48 ;     off = b8(out, off, 137)       // 89 ;     off = b8(out, off, 69)        // 45 modrm: mod01 reg000 rbp ;     off = b8(out, off, 256 - slot) ;     gebe_zurueck off
```

### `x_test_rax(out, off)`
**功能**：发射 test rax,rax (48 85 C0)。
**示例**：
```starc
off = b8(out, off, 72) ;     off = b8(out, off, 133) ;     off = b8(out, off, 192) ;     gebe_zurueck off
```

### `zahl2(num)`
**功能**：------ Zwei Ziffern mit fuehrender Null ----------。
**示例**：
```starc
s = int_to_str(num) ;     Wenn str_len(s) < 2 dann ;         gebe_zurueck str_concat("0", s)
```

### `zeile_befehl(tab, pos, ende)`
**功能**：------ Befehlsname einer Zeile (bis '。
**示例**：
```starc
i = pos ;     Solange i < ende ;         Wenn raw_byteat(tab, 8 + i) == 124 dann ;             gebe_zurueck str_trim(str_slice(tab, pos, i - pos))
```

### `zeile_beschreibung(tab, pos, ende)`
**功能**：------ Beschreibung einer Zeile (nach '。
**示例**：
```starc
i = pos ;     Solange i < ende ;         Wenn raw_byteat(tab, 8 + i) == 124 dann ;             gebe_zurueck str_trim(str_slice(tab, i + 1, ende - i - 1))
```

### `zeilen_ende(tab, pos)`
**功能**：------ Zeilenende finden (Position von Zeilenumbruch oder Ende) ----------。
**示例**：
```starc
tl = str_len(tab) ;     i = pos ;     Solange i < tl ;         Wenn raw_byteat(tab, 8 + i) == 10 dann ;             gebe_zurueck i
```

### `zeit_stamp()`
**功能**：------ Zeitstempel JJJJ-MM-TT HH:MM:SS ----------。
**示例**：
```starc
st = alloc(16) ;     GetLocalTime(st) ;     jahr = raw_byteat(st, 0) + raw_byteat(st, 1) * 256 ;     monat = raw_byteat(st, 2) + raw_byteat(st, 3) * 256 ;     tag = raw_byteat(st, 6) + raw_byteat(st, 7) * 256 ;     stunde = raw_byteat(st, 8) + raw_byteat(st, 9) * 256 ;     minute = raw_byteat(st, 10) + raw_byteat(st, 11) * 256 ;     sekunde = raw_byteat(st, 12) + raw_byteat(st, 13) * 256 ;     s = int_to_str(jahr) ;     s = str_concat(s, "-") ;     s = str_concat(s, zahl2(monat)) ;     s = str_concat(s, "-")
```
