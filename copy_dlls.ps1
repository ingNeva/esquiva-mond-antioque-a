param(
    [string]$ExePath,
    [string]$DestDir,
    [string]$LddPath = "C:/msys64/usr/bin/ldd.exe"
)

if (-not (Test-Path $LddPath)) {
    Write-Warning "ldd.exe no encontrado en: $LddPath"
    exit 0
}

Write-Host "Copiando DLLs para: $ExePath"
Write-Host "Destino: $DestDir"

# Patrones de DLLs del sistema Windows que NO hay que copiar
$windowsPatterns = @(
    'C:/Windows',
    '/c/Windows',
    'C:\Windows'
)

function Is-WindowsDll($path) {
    foreach ($pat in $windowsPatterns) {
        if ($path -like "$pat*") { return $true }
    }
    return $false
}

# Ejecutar ldd recursivamente sobre el exe y cada DLL encontrada
$queue   = [System.Collections.Queue]::new()
$queue.Enqueue($ExePath)
$visited = @{}
$toCopy  = @{}

while ($queue.Count -gt 0) {
    $current = $queue.Dequeue()

    if ($visited[$current]) { continue }
    $visited[$current] = $true

    $lddOut = & $LddPath $current 2>&1
    foreach ($line in $lddOut) {
        # Formato:  nombre.dll => /ruta/real (0xaddr)
        if ($line -match '=>\s+(\S+)\s+\(') {
            $dllPath = $matches[1]

            # Convertir rutas tipo /c/... a C:/...
            if ($dllPath -match '^/([a-zA-Z])/(.+)') {
                $dllPath = "$($matches[1]):/$($matches[2])"
            }
            # Convertir rutas tipo /ucrt64/bin/... o /mingw64/bin/...
            elseif ($dllPath -match '^/(ucrt64|mingw64|clang64)/(.+)') {
                $dllPath = "C:/msys64/$($matches[1])/$($matches[2])"
            }

            if (-not (Test-Path $dllPath))  { continue }
            if (Is-WindowsDll $dllPath)     { continue }
            if ($toCopy[$dllPath])          { continue }

            $toCopy[$dllPath] = $true
            $queue.Enqueue($dllPath)   # inspeccionar sus dependencias también
        }
    }
}

if ($toCopy.Count -eq 0) {
    Write-Host "No se encontraron DLLs para copiar."
    exit 0
}

foreach ($dll in $toCopy.Keys) {
    $name = Split-Path $dll -Leaf
    Write-Host "  Copiando: $name"
    Copy-Item $dll -Destination $DestDir -Force
}

Write-Host ""
Write-Host "OK - $($toCopy.Count) DLL(s) copiadas a $DestDir"