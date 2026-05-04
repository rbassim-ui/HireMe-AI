$ErrorActionPreference = 'Stop'

[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)
chcp 65001 | Out-Null

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$backend = Join-Path $root 'backend-c'
$exe = Join-Path $backend 'output\hireme.exe'
$frontend = Join-Path $root 'frontend\index.html'

$envFile = Join-Path $root '.env'
function Get-KeyFromEnvFile {
    param([string]$Path)

    if (-not (Test-Path $Path)) { return $null }
    $line = Get-Content $Path | Where-Object { $_ -match '^\s*GROQ_API_KEY\s*=' } | Select-Object -First 1
    if (-not $line) { return $null }
    $value = ($line -replace '^\s*GROQ_API_KEY\s*=\s*', '').Trim().Trim('"').Trim("'")
    if ([string]::IsNullOrWhiteSpace($value)) { return $null }
    return $value
}

function Save-KeyToEnvFile {
    param(
        [string]$Path,
        [string]$Value
    )

    if (-not (Test-Path $Path)) {
        Set-Content -Path $Path -Value "GROQ_API_KEY=$Value" -Encoding UTF8
        return
    }

    $lines = Get-Content $Path
    $updated = $false
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '^\s*GROQ_API_KEY\s*=') {
            $lines[$i] = "GROQ_API_KEY=$Value"
            $updated = $true
            break
        }
    }

    if (-not $updated) {
        $lines += "GROQ_API_KEY=$Value"
    }

    Set-Content -Path $Path -Value $lines -Encoding UTF8
}

if (-not $env:GROQ_API_KEY) {
    $loaded = Get-KeyFromEnvFile -Path $envFile
    if ($loaded) {
        $env:GROQ_API_KEY = $loaded
    }
}

if (-not $env:GROQ_API_KEY) {
    Write-Host 'GROQ_API_KEY introuvable. Configuration initiale requise.' -ForegroundColor Yellow
    $entered = Read-Host 'Collez votre cle Groq (gsk_...)'
    if ([string]::IsNullOrWhiteSpace($entered)) {
        throw 'GROQ_API_KEY est requise pour generer/corriger via Groq.'
    }

    $entered = $entered.Trim()
    $env:GROQ_API_KEY = $entered
    Save-KeyToEnvFile -Path $envFile -Value $entered
    Write-Host 'Cle enregistree dans .env' -ForegroundColor Green
}

$curl = Get-Command curl -ErrorAction SilentlyContinue
if (-not $curl) {
    throw 'curl est requis pour appeler Groq. Installez curl puis relancez.'
}

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
        & gcc -o output\hireme.exe main.c db.c api.c gemini.c menu.c interview.c json.c report.c utils.c sqlite3.c -lws2_32 -D_WIN32_WINNT=0x0600 -std=c11 -Wall -Wextra -Wpedantic -Wno-unused-parameter
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