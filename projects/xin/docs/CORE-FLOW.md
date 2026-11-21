# Xin / 辛 — Core Flow, line by line

This document walks the entire pipeline from "a text file of domain names" to
"ads fail to load on the machine". Every stage cites the real source that
implements it. Nothing here is aspirational: if a step is not in the code, it is
marked **NOT IMPLEMENTED**.

```
data/src_*.txt (optional)
data/rules_raw.txt            hand-maintained domains
data/cosmetic_raw.txt         hand-maintained CSS selectors
        │
        ▼  (1) normalize + sort          build.sh
data/rules_sorted.txt                 110,302 lines
        │
        ▼  (2) pack                     xin_pack.starc  + u_cos.staro
data/rules.qrb                        5,295,423 bytes, "XINR" v2
        │
        ├──► (3) load + match            xin_core.starc  + u_xin.staro
        │        verdicts + benchmark
        │
        ├──► (4) local HTTP API          xin_serve.starc + u_http.staro
        │        BLOCK / ALLOW on 127.0.0.1:8353
        │
        └──► (5) hosts fragment          xin_hosts.starc + u_hosts.staro
                 data/hosts_block.txt    (2.97 MB)
                        │
                        ▼  (6) apply      scripts/apply_hosts.ps1  (admin)
                 C:\Windows\System32\drivers\etc\hosts
                        │
                        ▼  (7) page layer ext/content.js  (Chrome/Edge MV3)
                 containers + "广告" labels removed
```

---

## 1. Normalization — `build.sh`, stage 1

```bash
{
  cat "$ROOT"/data/src_*.txt 2>/dev/null || true
  cat "$ROOT/data/rules_raw.txt"
} \
  | grep -v '^#' \
  | grep -v '^[[:space:]]*$' \
  | tr 'A-Z' 'a-z' \
  | grep -E '^[a-z0-9]([a-z0-9._-]*[a-z0-9])?$' \
  | awk 'length($0) <= 47' \
  | sort -u > "$ROOT/data/rules_sorted.txt"
```

Filter by filter, and *why* each one exists:

| Step | Purpose | Why it must be here, not in StarC |
|---|---|---|
| `cat src_*.txt + rules_raw.txt` | merge upstream seed with own rules | upstream format is arbitrary text; StarC should never see it |
| `grep -v '^#'` | drop comments | avoids writing a comment parser in stage 2 |
| `grep -v '^[[:space:]]*$'` | drop blank lines | the packer counts records by newline |
| `tr 'A-Z' 'a-z'` | force lowercase | DNS is case-insensitive; container must be too |
| `grep -E '^[a-z0-9]([a-z0-9._-]*[a-z0-9])?$'` | keep only plausible hostnames | kills `||ads.com^`, `127.0.0.1 x`, wildcards, junk |
| `awk 'length($0) <= 47'` | **hard cap at 47 bytes** | a record holds `1 length byte + 47 payload bytes`; anything longer cannot be stored |
| `sort -u` | dedupe + **make binary search possible** | the matcher assumes lexicographic order; unsorted input silently breaks lookups |

> **The 47-byte cap is not cosmetic.** If a domain exceeds it, the packer clamps
> (`if ln > 47 then ln = 47`) and the stored string becomes a *different* domain —
> a truncated rule that can never match. The shell filter exists to guarantee
> that clamp never fires.

---

## 2. Container layout — `.qrb` v2

Fixed header, fixed-size records, then a variable-length cosmetic tail.

| Offset | Size | Field | Value / meaning |
|---|---|---|---|
| `0x00` | 4 | magic | `58 49 4E 52` = `XINR` |
| `0x04` | 4 | version | `2` (u32 LE) |
| `0x08` | 4 | `domain_count` | e.g. `110302` |
| `0x0C` | 4 | `recsize` | `48`, invariant |
| `0x10` | `N × 48` | domain records | sorted ascending, byte-wise |
| `base` | 4 | `cosmetic_count` | e.g. `53` |
| `base+4` | var | cosmetic entries | `[len:u16 LE][bytes:len]` … |

One domain record (48 bytes):

```
+0       1 byte    length  (1..47)
+1..+47  47 bytes  payload, zero-padded, ASCII, lowercase
```

Consequences of this layout, stated plainly:

* **Lookup needs no parsing.** Record *k* starts at `0x10 + k*48`; its length is
  one byte. There is no delimiters, no offsets table, no allocation.
* **The file is memory-mappable in principle** (current code reads it whole
  instead — see §4).
* **Backward compatibility**: a v1-era reader that only knows the domain table
  reads header + records and ignores the tail. It cannot crash on a v2 file, it
  just cannot see the selectors.
* **Wasted space**: 48 bytes per domain regardless of length. A 10-character
  domain still costs 38 zero bytes. With 110k rules that is ~4.2 MB of padding
  out of 5.3 MB. Cheap, and it buys O(1) random access.

---

## 3. Packing — `src/compiler/xin_pack.starc`

`pack(inpath, cospath, outpath)`, step by step:

1. **Open + size + read the whole list.**
   ```starc
   h = x_open_r(inpath);  sz = x_size(h);  x_seek(h, 0)
   buf = alloc(sz + 8)
   n = x_read_buf(h, buf, sz)
   ```
   `alloc(n)` in StarC returns a pointer whose payload starts at **offset 8**, so
   `buf+0..7` is header space and byte *i* of the payload is `raw_byteat(buf, 8+i)`.
   Every buffer in this project follows that convention.

2. **Count records by counting `\n`**, plus one if the file does not end with a
   newline:
   ```starc
   Solange i < n
       Wenn raw_byteat(buf, i) == 10 dann lines = lines + 1 Ende
   ```

3. **Allocate the output** with `16 + lines*48 + 16384`. The 16 KB tail is
   reserved for the cosmetic section so it does not need a second pass.

4. **Write the header**:
   ```starc
   raw_setbyte(out, 0, 88) ... raw_setbyte(out, 3, 82)   # "XINR"
   u32_put(out, 4, 2)                                    # version
   u32_put(out, 8, 0)                                    # count patched later
   u32_put(out, 12, rs)                                  # 48
   ```

5. **Split on newlines, trim `\r`, clamp to 47, write record by record.**
   ```starc
   Wenn raw_byteat(buf, e - 1) == 13 dann e = e - 1 Ende   # CRLF tolerance
   ln = e - s
   Wenn ln > 47 dann ln = 47 Ende
   raw_setbyte(out, roff, ln)                             # length byte
   # then ln payload bytes
   ```
   Empty lines produce `ln == 0` and are skipped (`Wenn ln > 0`), so a stray
   blank line in the middle of the file cannot create a zero-length record.

6. **Patch the real count** once the loop finishes: `u32_put(out, 8, rec)`.

7. **Append the cosmetic section** starting at `base = 16 + rec*48`.
   `pack_cos()` reads `cosmetic_raw.txt`, writes `[len:u16][bytes]` per line,
   skips lines longer than 200 bytes, and finally writes the count **backwards**
   at `base` (`u32_put(out, base, cnt)` — it is patched after the loop because
   the count is only known then). It returns the end offset, which becomes the
   file length.

8. **Write once.** `x_open_w` (CreateFileA with `CREATE_ALWAYS`), one
   `WriteFile`, one `CloseHandle`. No temp file, no rename dance — a crash
   mid-write leaves a truncated `rules.qrb`, which the loader rejects via the
   magic check.

Measured: 110,302 records + 53 selectors in **132 ms**, output 5,295,423 bytes.

---

## 4. Loading — `u_xin.staro`, `qrb_load`

```starc
h = x_open_r(path);  hs = x_size(h);  x_seek(h, 0)
buf = alloc(hs)
n = x_read_buf(h, buf, hs)
x_close(h)
```

**The entire container is read into one allocation.** For 5.1 MB that is fine
and it makes every subsequent lookup pure memory access. It also means:

* loading cost is one `ReadFile` + one `alloc` (≈ a few ms),
* a 50 MB container would need 50 MB of contiguous heap — acceptable today,
  worth revisiting if the rule set grows 10×,
* there is **no magic validation at this point**. `qrb_count()` blindly reads the
  u32 at offset 8. A corrupt file yields a garbage count, not an error message.
  This is a known weakness (§12).

`qrb_count(buf)` = `u32_get(buf, 8)`. That is the whole "index".

---

## 5. Matching — `qrb_find` + `qrb_check`

### 5.1 Exact lookup: binary search over fixed records

```starc
lo = 0;  hi = cnt - 1
Solange lo <= hi
    mid = (lo + hi) / 2
    roff = 16 + mid * rs
    rl = raw_byteat(buf, roff)
    c = str_cmp(buf, roff + 1, rl, src, s, l1)
    Wenn c == 0 dann gebe_zurueck 1 Ende
    Wenn c < 0 dann lo = mid + 1 Sonst hi = mid - 1 Ende
Ende
```

`str_cmp` is byte-wise lexicographic with the shorter string ordered first when
it is a prefix. That ordering **must** match the `sort -u` used at build time —
which it does, because both are plain ASCII byte comparisons under `LC_ALL=C`.
This is the single most fragile coupling in the project: change the collation in
`build.sh` and every lookup silently returns the wrong answer, with no error.

Cost: `log2(110302) ≈ 17` comparisons, each a few bytes. Measured 656 ns total
including the suffix walk and the line-scanning overhead of the benchmark.

### 5.2 Suffix semantics: walk the labels

```starc
Funktion qrb_check(buf, src, s, e)
    hit = qrb_find(buf, src, s, e)          # whole name
    Wenn hit == 1 dann gebe_zurueck 1 Ende
    p = s
    Solange p < e
        Wenn raw_byteat(src, p) == 46 dann   # '.'
            hit = qrb_find(buf, src, p + 1, e)
            Wenn hit == 1 dann gebe_zurueck 1 Ende
        Ende
        p = p + 1
    Ende
    gebe_zurueck 0
```

For `log.analytic.umeng.com` it tries, in order:

```
log.analytic.umeng.com      → miss
analytic.umeng.com          → HIT   (rule "umeng.com" style entries)
```

This is what makes the blocker usable: one rule covers every subdomain. It is
also **exactly why `notdoubleclick.net` is not blocked** — there is no substring
search anywhere, so a rule for `doubleclick.net` can never match a name that
merely *contains* those characters.

Complexity: `k + 1` binary searches where `k` = number of labels (typically 2–4)
→ ~34–68 comparisons worst case. Still sub-microsecond.

**Termination detail:** the loop condition is `p < e` and the split point is
`p + 1`, so the TLD itself (`com`) is attempted as a rule. A rule consisting of a
bare TLD would therefore block an entire namespace — legal, and dangerous.

---

## 6. Local verdict API — `core/u_http.staro` + `core/xin_serve.starc`

Startup:

```starc
WSAStartup(514, wsa)          # 0x0202, Winsock 2.2
s = socket(2, 1, 6)           # AF_INET, SOCK_STREAM, IPPROTO_TCP
bind(s, {AF_INET, port, 127.0.0.1}, 16)
listen(s, 32)
```

Address bytes are written by hand (`127,0,0,1`), so the listener is **loopback
only** by construction — there is no flag to misconfigure, and no code path that
binds `0.0.0.0`.

Request handling, per connection:

1. `accept()` (blocking, single-threaded).
2. Zero a 4 KB buffer, `recv(c, req, 4095, 0)` — **one** `recv`, no header
   reassembly. A request split across TCP segments is parsed as garbage.
3. Find the domain by scanning for `=` then ` ` or `\r`:
   ```starc
   Wenn ch == 61 dann Wenn dstart == 0 dann dstart = i + 1 Ende Ende
   Wenn ch == 32 oder ch == 13 dann ... dend = i
   ```
   This is a miniature parser for `GET /check?domain=X HTTP/1.1`. It does not
   tolerate `?foo=1&domain=X` (it would take the first `=`), does not URL-decode,
   and does not validate the method or path.
4. `qrb_check(...)` on the extracted span → `BLOCK` / `ALLOW`.
5. Write a fixed 200 response with `Access-Control-Allow-Origin: *`,
   `Content-Length: 6`, `Connection: close`.
6. `closesocket(c)`; loop.

Known limits, stated openly: single connection at a time, no keep-alive, no
timeouts (a client that connects and sends nothing blocks the loop forever), no
`listen` backlog growth, no logging beyond stdout.

---

## 7. hosts fragment — `core/u_hosts.staro`

```starc
p = h2_put(buf, p, "# === XIN BLOCK START (auto-generated by Xin, do not edit) ===")
Solange i < cnt
    roff = 16 + i * rs
    rl = raw_byteat(rules, roff)
    p = h2_put(buf, p, "0.0.0.0 ")
    # then rl payload bytes, then CRLF
```

* Output is CRLF-terminated ASCII, capacity `cnt*64 + 512`.
* Both **markers** are mandatory: the apply/restore scripts locate the block by
  them. Dropping a marker means restore can no longer find its own output.
* 110,302 records → 2.97 MB, 110,304 lines.
* There is **no whitelist subtraction step**. If a domain must be excluded, it has
  to be removed from the source list before packing. `allow.qrb` is reserved in
  the format discussion but **NOT IMPLEMENTED**.

---

## 8. Applying to the system — `scripts/apply_hosts.ps1`

1. Read `data/hosts_block.txt`; abort with `FAIL:` if missing.
2. Read the live hosts file as raw text.
3. **Remove any previous XIN block first** — this is what makes the script
   idempotent:
   ```powershell
   $orig = [regex]::Replace($orig, '(?s)# === XIN BLOCK START.*?# === XIN BLOCK END ===\r?\n?', '')
   ```
4. Ensure a trailing CRLF, append the new block.
5. Write back as **ASCII without BOM** — a BOM at the top of `hosts` breaks
   resolution on some Windows configurations.
6. `ipconfig /flushdns`.
7. Report `APPLIED: block lines = N` by counting `0.0.0.0` lines.

`restore_hosts.ps1` performs only steps 2–3 and 5–6: it strips the marker block
and leaves every other line untouched. That property matters — a user with
existing entries must not lose them.

**Why this step is manual:** writing under `System32` requires elevation. The
tooling refuses to auto-elevate (`Start-Process -Verb RunAs` is blocked as a
LOLBin pattern). This is a deliberate boundary, not an oversight. Result: Xin
cannot be a one-click install until it ships a signed installer or a service.

---

## 9. Page layer — `ext/content.js` (MV3)

Two independent jobs.

### 9.1 Delete containers (declarative, cheap)

53 selectors are joined into one list and injected **twice**:

1. as a `<style>` rule — `display:none!important` etc. — which catches elements
   that exist at parse time, and
2. as a DOM sweep — `querySelectorAll(JOINED)` — which tags survivors with
   `data-xin-hidden="1"`.

Both are needed: the stylesheet wins the cascade for static markup, the sweep
covers nodes created after injection.

### 9.2 Delete the "广告" labels (heuristic, expensive)

The label is often **not a text node** — it is `::before { content: "广告" }` on
an overlay anchor:

```html
<a href="javascript:;" target="_self" class="ad_hover_href"></a>
```

`textContent` cannot see pseudo-element content; `innerText` can, but only after
layout. Hence three passes:

| Pass | Mechanism | Catches |
|---|---|---|
| A | `TreeWalker` over text nodes | plain `<span>广告</span>` |
| B | `[class*="ad"], [class*="gg"], [class*="tag"]…` then `innerText` | pseudo-elements, split text |
| C | every leaf element's `innerText` | whatever B missed |

Guard rails, because a wrong hide is worse than a missed ad:

* label must equal one of `广告 / 廣告 / 广告位 / AD / Advertisement / 赞助`
  after whitespace collapse, length ≤ 10;
* element must be small — 520×260 normally, relaxed to 1600×1200 only when its
  `className` matches `/(^|[\s_-])(ad|ads|gg|guanggao)([\s_-]|$)/i`
  (this relaxation is exactly what fixed the `ad_hover_href` overlay, which is
  as large as the ad slot it covers);
* sweep runs at 600 / 1500 / 3000 / 6000 / 10000 ms after load, then on
  `MutationObserver` with a 150 ms debounce.

`?xin-debug=1` turns on `console.log('[xin] hide …')` for per-element auditing.

### 9.3 Soft mode

`mode === 'soft'` removes the injected stylesheet and clears every
`data-xin-hidden` marker and inline `display` it set. Nothing else in the browser
is touched, so switching modes is reversible without a reload of the rule pack.

---

## 10. Verification — what "it works" means here

| Claim | How it was checked |
|---|---|
| Container is well-formed | Python re-parser: walk header → records → cosmetic tail; assert final offset == file length (5,295,423 == 5,295,423) |
| Matcher is exact-suffix, not substring | fixture contains `doubleclick.net` (block) and `notdoubleclick.net` (allow) |
| Subdomain inheritance | `log.analytic.umeng.com` blocked by parent rule |
| Real blocking | `curl` to `pagead2.googlesyndication.com`, `securepubads.g.doubleclick.net`, `www.googletagmanager.com` → connection fails; `www.baidu.com` → 200 |
| hosts actually live | `ping doubleclick.net` → "host could not be found"; 110,302 `0.0.0.0` lines present |
| Speed | `bench()`: 3000 reps × 32 domains = 96,000 queries, `GetTickCount` delta, printed as ns/query and qps |

What is **not** verified: no unit tests, no CI, no fuzzing of the packer, no test
that the extension's selectors still match today's NetEase/Baidu markup (it is
checked by eye, per site, when a user reports a leftover box).

---

## 11. Engineering notes — bugs that actually happened

These are kept because they are the expensive part of the project, and they are
all invisible from the outside.

1. **`alloc(n)` payload starts at offset 8.** Writing to `buf[0]` corrupts the
   allocator header. Every byte access in this codebase goes through
   `raw_byteat(buf, 8+i)`.
2. **`2^31` literal overflow.** `2147483648` as a *signed* immediate becomes
   negative. `CreateFileA` access masks had to be written as
   `1073741824 + 1073741824` rather than as one literal.
3. **`ReadFile` short-read drift.** On very large sequential reads the returned
   byte count can be smaller than requested. A loop that advances the file
   pointer by the *requested* size instead of the *returned* size silently
   desynchronises. In an earlier 12 GB weight-quantisation pipeline this
   corrupted **52%** of blocks while single-block `seek` reads stayed perfect.
   Fix: loop until `total >= n`, advancing by the returned count, and re-zero the
   `lpNumberOfBytesRead` out-parameter each iteration.
4. **`build.sh` clobbering the rule set.** The script originally read only
   `rules_raw.txt` (69 lines), so one build silently replaced a 110,302-entry
   container with a 69-entry one. Fix: `cat data/src_*.txt` + `rules_raw.txt`.
   Lesson: a build step that *shrinks* its input needs a guard, not trust.
5. **`bind()` returning -1 because of a stale listener.** The previous
   `xin_serve.sc` still held the port; the process name in Task Manager includes
   the `.sc` extension, so `Get-Process -Name xin_serve` matched nothing. Fix:
   `-like 'xin_serve*'`.
6. **Rebuild fails while the binary runs.** Windows locks the running `.sc`, so
   `starcc -o dist/xin_serve.sc` fails. `build.sh` now treats this as a skip with
   an explicit message instead of aborting the whole build.
7. **Size-based filtering missed the real target.** The label hunt's 520×260 cap
   rejected `a.ad_hover_href` because that overlay is as big as the ad slot.
   Lesson: "hide small things" is the wrong heuristic for overlays; match on
   semantics (class) first, geometry second.

---

## 12. Known failure modes

| Symptom | Cause | Status |
|---|---|---|
| Ads reappear in browser only | DoH/DoT enabled in the browser bypasses hosts | **not handled** |
| Ads reappear in one app but not others | that app ships its own HTTPDNS | **not handled** |
| Site partially broken, blank areas | a rule killed a CDN the site needs | **no whitelist** |
| `BLOCK` for everything / garbage counts | corrupted or truncated `rules.qrb` | **no magic check at load** |
| Service hangs forever | a client connected and sent nothing | **no recv timeout** |
| First lookup after boot is slow | Windows loads 2.97 MB of hosts | **accepted** |
| Rule missing after rebuild | `rules_sorted.txt` unsorted or over 47 bytes | guard: shell filters |
| Extension shows nothing | dev-mode extension not reloaded after edit | documented in README |

---

## 13. Where to extend

| Extension point | File | Cost |
|---|---|---|
| Add domain rules | `data/rules_raw.txt` | trivial, rebuild |
| Add page selectors | `data/cosmetic_raw.txt` + `ext/content.js` array (keep in sync) | trivial, rebuild + reload extension |
| Whitelist | `u_xin.staro` (`qrb_check`), new `.qrb` section | medium — needs format v3 |
| Local DNS server | new `u_dns.staro`, `xin_dns.starc` | large — replaces hosts entirely |
| Auto-update | new `xin_update.starc` + manifest fetch | medium |
| Linux/Android | blocked on StarC codegen targets | out of scope for now |

---

*Xin / 辛 — first product built on StarC. MIT licensed.*
