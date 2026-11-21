# StarC 语言项目简介

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Language: StarC](https://img.shields.io/badge/Language-StarC-7C3AED.svg)](https://github.com/8010456-wq/starc-inous)
[![Platform: x86-64](https://img.shields.io/badge/Platform-x86--64-1f6feb.svg)]()
[![Self-hosted Compiler](https://img.shields.io/badge/Compiler-Self--hosted-0a9d58.svg)]()

> **StarC** —— 一门德语式关键字、直出原生 PE32+、零外部工具链的自研编程语言。
> 本仓库（`inous`）是 StarC 的全部标准库（`.staro`）、函数清单、项目简介与编译器目录的统一整合仓库。
>
> 📄 **许可证**：本仓库以 **MIT License** 开源（见 [LICENSE](LICENSE)），可自由使用、修改、分发与商用。
>
> 🚀 **首个基于 StarC 的完整产品**：[**辛 / Xin**](projects/xin/README.md) —— 全系统去广告引擎（自研二进制规则容器 + 自研匹配器 + 本地 HTTP 判定接口），11 万条规则 656 ns 查询，纯 StarC 实现，零第三方依赖。

---

## 一、StarC 是什么

StarC 是一门**从零自研**的系统级编程语言，采用**德语式关键字**（`Funktion` / `Ende` / `Wenn` / `dann` / `Solange` / `gebe_zurueck`），语法接近自然语言，读起来像一份流程说明书。

它最大的特点是**不需要任何第三方工具链**：编译器用纯 C 编写，把 `.starc` 源码经「词法分析 → 语法分析 → 代码生成 → PE 封装」直接产出 **x86-64 机器码 PE32+ 可执行文件**，产物只依赖系统自带的 `kernel32.dll`。不调用 `gcc` / `as` / `ld` / `link.exe`，也不依赖 Python、Node 等任何运行时。

| 属性 | 说明 |
|---|---|
| 语言名 | StarC |
| 关键字风格 | 德语式（Funktion / Ende / Wenn / dann / Solange / gebe_zurueck） |
| 编译器 | `starcc`（自研，纯 C 实现，零外部工具链） |
| 编译产物 | 原生 x86-64 PE32+ 可执行文件 |
| 依赖 | 仅系统自带 `kernel32.dll` |
| 库文件 | `.staro`（标准库源码，可被 `linx-o` 导入） |
| 底层调用 | 通过 `linx-o` 直接调用 WinAPI / CUDA / PTX |
| 编译命令 | `starcc 源.starc [-o 输出.exe] [-S] [-v]` |

---

## 二、为什么发明 StarC

传统系统语言（C / C++ / Rust / Go）需要完整的编译器工具链、依赖管理、运行时和大量第三方库，开发与部署成本高。StarC 的定位是：

1. **极简** —— 一门语言只需要一个编译器二进制，语法精简，学习成本低。
2. **零依赖** —— 编译产物是裸 PE，运行时零第三方依赖，拷贝即用。
3. **直通底层** —— 通过 `linx-o` 能直接发射 WinAPI 调用、CUDA 内核、PTX 汇编，适合做系统工具、网络、图形、GPU 计算。
4. **可读性强** —— 德语式关键字让控制流接近自然语言，代码像文档。
5. **完全自主可控** —— 语言、编译器、标准库全部自研，不受任何第三方生态掣肘。

---

## 三、语法速览

### 函数定义
```starc
Funktion add(a, b)
    gebe_zurueck a + b
Ende
```

### 条件分支
```starc
Wenn x > 0 dann
    // 正数
sonst
    // 非正数
Ende
```

### 循环
```starc
i = 0
Solange i < 10
    i = i + 1
Ende
```

### 返回值
```starc
Funktion max(a, b)
    Wenn a > b dann
        gebe_zurueck a
    Ende
    gebe_zurueck b
Ende
```

### 导入库 / 底层调用
```starc
linx-o win32  // 引入 WinAPI 直接调用能力
r = CreateProcessA(0, cmd, 0, 0, 0, 0, 0, 0, si, pi)
```

---

## 四、标准库（`.staro`）

StarC 的标准库以 `.staro` 后缀的源码文件组织，涵盖字符串、内存、文件、网络、JSON、图形、GPU/PTX、时间等能力。本仓库整合了当前出现的**全部 187 个 `.staro` 文件**，并按原项目目录结构保持相对路径。

标准库能力概览（详见 `FUNKTIONEN.md` 完整函数清单，共 **813 个唯一函数**）：

| 领域 | 代表函数 |
|---|---|
| 内存/字节 | `alloc` / `mem_zero` / `raw_setbyte` / `raw_getbyte` |
| 字符串 | `str_len` / `str_cat` / `str_find` / `str_sub` |
| 文件 | `file_read` / `file_write` / `file_exists` |
| 网络 | `net_connect` / `net_dns_query` / `net_send_all` |
| JSON | `js_parse_int` / `js_parse_float` / `js_parse_str` |
| GPU/PTX | `kstr_emit` 系列（PTX/CUDA 内核发射） |
| 时间 | `zeit_stamp` / `zeit_now` |
| 网云/体数据 | `nc_sim_voxnet` / `nc_*`（网云检索、体网络相似度） |

> **函数清单**：`FUNKTIONEN.md` 完整列出全部 813 个唯一函数，每个含函数名（参数）、约 20 字功能说明、1–5 行调用示例。

---

## 五、仓库结构

```
inous/                       ← StarC 大型标准库仓库
├── README.md                ← 本文件（项目简介）
├── FUNKTIONEN.md            ← 全部 813 个函数清单（含功能+示例）
├── compiler/                ← 独立打包的编译器目录（starcc + 源码 + 构建脚本）
├── <项目>/                  ← 各项目标准库 .staro（保持相对路径）
│   └── *.staro
└── _tools/                  ← 函数提取/生成脚本（开发工具）
```

---

## 六、快速上手

### 1. 编译一个程序
```bash
starcc hello.starc -o hello.exe
```

### 2. 编译并查看统计
```bash
starcc hello.starc -o hello.exe -v
```

### 3. 合并多个源文件
```bash
starcc main.starc util.starc -o app.exe
```

### 4. 直接运行
```bash
./hello.exe
```

---

## 七、许可证与自主性

StarC 语言、`starcc` 编译器、`.staro` 标准库均为**完全自主开发**，无任何第三方依赖、无外部工具链、无运行时。语言设计、实现与生态皆由本仓库所有。

本仓库以 **MIT License** 开源（见 [LICENSE](LICENSE)）：允许自由使用、复制、修改、合并、发布、分发、再许可及销售本软件副本，仅需保留版权与许可声明。软件按"原样"提供，不附带任何明示或默示担保。

---

*本简介生成于 2026-11-06，数据来源：`D:\starc\inous` 仓库整合实录。*

---

## 八、产品：辛 / Xin（首个完整产品）

> 📖 完整文档：[`projects/xin/README.md`](projects/xin/README.md) ｜ 核心流程逐行剖析：[`projects/xin/docs/CORE-FLOW.md`](projects/xin/docs/CORE-FLOW.md)

**辛 / Xin** 是第一个完全跑在 StarC 上的完整产品：**全系统去广告引擎**。

它不写浏览器插件了事，而是把广告问题推回到 DNS 层——域名解析不出来，任何程序（浏览器、微信内嵌页、Steam、桌面软件）里的广告都加载不了：

| 层 | 实现 | 作用 |
|---|---|---|
| 网络层 | `hosts` 110,302 条 `0.0.0.0` | 广告域名解析失败，全系统生效 |
| 页面层 | Chrome/Edge MV3 扩展（53 条选择器） | 删掉残留广告容器与「广告」标识 |
| 判定层 | `127.0.0.1:8353/check?domain=` | 本地微秒级判定，供其他程序调用 |

**技术要点（全部自研，零第三方依赖）**

| 组件 | 说明 |
|---|---|
| `.qrb` 容器 | 自研二进制格式：`XINR` 魔数 + 定长 48 字节记录 + 选择器尾区，加载零解析 |
| `xin_pack` | 文本 → 二进制的规则编译器，11 万条 **132 ms** |
| 匹配器 | 定长表**二分查找** + 域名**后缀逐段下钻**，110,302 条规则 **656 ns/次** |
| 接口 | Winsock 手写 HTTP 服务（`WSAStartup`/`bind`/`listen`/`recv`），仅绑 `127.0.0.1` |
| 语言 | 全部 `.starc` / `.staro`，仅调用 `kernel32.dll` 与 `ws2_32.dll` |

**为什么它不会被浏览器政策杀死**：拦截发生在 DNS 层，扩展只做"删容器"的化妆品工作，因此**不依赖 `webRequest`，也不占用 `declarativeNetRequest` 规则配额**——Chrome 的 Manifest V3 收紧打不到这个架构上。

**诚实的边界**（详见 [README §5](projects/xin/README.md#5-honest-assessment)）：仅 Windows；`hosts` 方案会被 DoH/HTTPDNS 绕过；同域名自营广告拦不了；无白名单、无自动更新、无安装器、无测试套件。

```bash
cd projects/xin
STARC_CC=/path/to/starcc.exe bash build.sh     # 一键：归一化 → 编译 → 打包 → 自测
```
