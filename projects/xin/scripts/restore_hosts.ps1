$ErrorActionPreference = 'Stop'
$hostsPath = 'C:\Windows\System32\drivers\etc\hosts'

if (-not (Test-Path $hostsPath)) { Write-Output "FAIL: hosts not found"; exit 1 }
$orig = Get-Content $hostsPath -Raw

$before = $orig.Length
$clean = [regex]::Replace($orig, '(?s)# === XIN BLOCK START.*?# === XIN BLOCK END ===\r?\n?', '')
$clean = $clean.TrimEnd("`r", "`n") + "`r`n"

$enc = New-Object System.Text.ASCIIEncoding
[System.IO.File]::WriteAllText($hostsPath, $clean, $enc)

ipconfig /flushdns | Out-Null

$left = (Get-Content $hostsPath | Where-Object { $_ -match '^\s*0\.0\.0\.0\s+' }).Count
Write-Output "RESTORED: removed XIN block, remaining 0.0.0.0 lines = $left"
