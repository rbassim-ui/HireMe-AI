$ErrorActionPreference = 'Stop'

[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)
chcp 65001 | Out-Null

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$backend = Join-Path $root 'backend-c'
$exe = Join-Path $backend 'output\hireme.exe'
$frontend = Join-Path $root 'frontend\index.html'

if (-not (Test-Path (Join-Path $backend 'output'))) {
    New-Item -ItemType Directory -Path (Join-Path $backend 'output') | Out-Null
}

if (-not (Test-Path $exe)) {
    $compiler = Get-Command gcc -ErrorAction SilentlyContinue
    if (-not $compiler) {
        throw 'GCC is not available. Install MinGW-w64 first.'
    }

    Push-Location $backend
    try {
        & gcc -o output\hireme.exe main.c db.c api.c menu.c interview.c json.c report.c utils.c sqlite3.c -lws2_32 -D_WIN32_WINNT=0x0600 -std=c11 -Wall -Wextra -Wpedantic -Wno-unused-parameter
        if ($LASTEXITCODE -ne 0) {
            throw 'Build failed.'
        }
    }
    finally {
        Pop-Location
    }
}

Start-Process -FilePath $exe -WorkingDirectory $backend -WindowStyle Normal | Out-Null

for ($i = 0; $i -lt 20; $i++) {
    Start-Sleep -Milliseconds 500
    if (netstat -ano | Select-String ':3000' -Quiet) {
        break
    }
}

Start-Process -FilePath $frontend | Out-Null