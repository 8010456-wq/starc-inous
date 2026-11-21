$ErrorActionPreference = 'Stop'
$hostsPath = 'C:\Windows\System32\drivers\etc\hosts'
# 规则片段位于仓库内 projects/xin/data/hosts_block.txt，脚本在 projects/xin/scripts/
$blockPath = Join-Path (Split-Path $PSScriptRoot -Parent) 'data\hosts_block.txt'

if (-not (Test-Path $blockPath)) { Write-Output "FAIL: block file missing"; exit 1 }
$block = Get-Content $blockPath -Raw
$orig = Get-Content $hostsPath -Raw

# 先移除可能存在的旧 XIN 区块（可重复执行）
$orig = [regex]::Replace($orig, '(?s)# === XIN BLOCK START.*?# === XIN BLOCK END ===\r?\n?', '')

if (-not $orig.EndsWith("`r`n")) { $orig = $orig + "`r`n" }
$new = $orig + "`r`n" + $block

# hosts 必须是 ASCII/ANSI，不能带 BOM
$enc = New-Object System.Text.ASCIIEncoding
[System.IO.File]::WriteAllText($hostsPath, $new, $enc)

ipconfig /flushdns | Out-Null

$lines = (Get-Content $hostsPath | Where-Object { $_ -match '^\s*0\.0\.0\.0\s+' }).Count
Write-Output "APPLIED: block lines = $lines"
