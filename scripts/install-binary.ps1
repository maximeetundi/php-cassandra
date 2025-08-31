# Requires: PowerShell 5+, PHP installed and on PATH
# Installer for prebuilt php-cassandra extension (Windows)
# Expects release asset naming scheme like:
#   php_cassandra-windows-<arch>-php<majmin>-<ts>.dll
# Examples:
#   php_cassandra-windows-x86_64-php83-nts.dll
#   php_cassandra-windows-x86_64-php83-ts.dll

param(
  [string]$Php = "php",
  [string]$Repo = "maximeetundi/php-cassandra-driver"
)

$ErrorActionPreference = 'Stop'

function Get-PHP-Info {
  param([string]$Php)
  $extensionDir = & $Php -r "echo ini_get('extension_dir');"
  $iniFile = & $Php -r "$f=php_ini_loaded_file(); echo $f?$f:'';"
  $majMin = & $Php -r "printf('%d%d', PHP_MAJOR_VERSION, PHP_MINOR_VERSION);"
  $zts = & $Php -r "echo ZEND_THREAD_SAFE?'ts':'nts';"
  $arch = (Get-CimInstance Win32_OperatingSystem).OSArchitecture
  if ($arch -like '*64*') { $arch = 'x86_64' } else { $arch = 'x86' }
  return [pscustomobject]@{
    ExtensionDir = $extensionDir
    IniFile      = $iniFile
    MajMin       = $majMin
    Zts          = $zts
    Arch         = $arch
  }
}

$info = Get-PHP-Info -Php $Php
if (-not $info.ExtensionDir) { throw "extension_dir introuvable (php.ini)." }

$asset = "php_cassandra-windows-$($info.Arch)-php$($info.MajMin)-$($info.Zts).dll"
$uri = "https://github.com/$Repo/releases/latest/download/$asset"

Write-Host "[INFO] Téléchargement: $uri"
$temp = New-TemporaryFile
try {
  Invoke-WebRequest -UseBasicParsing -Uri $uri -OutFile $temp
} catch {
  throw "Échec du téléchargement de $asset depuis $uri. Assurez-vous que le binaire existe dans les Releases."
}

$dest = Join-Path $info.ExtensionDir 'php_cassandra.dll'
New-Item -ItemType Directory -Force -Path $info.ExtensionDir | Out-Null
Copy-Item $temp $dest -Force
Remove-Item $temp -Force
Write-Host "[INFO] Copié dans: $dest"

if (-not $info.IniFile -or -not (Test-Path $info.IniFile)) {
  Write-Warning "php.ini introuvable. Ajoutez manuellement: extension=php_cassandra.dll"
  exit 0
}

$content = Get-Content -Raw $info.IniFile
if ($content -match '(?im)^\s*extension\s*=\s*php_cassandra\.dll\s*$') {
  Write-Host "[INFO] Entrée déjà présente dans $($info.IniFile)"
} else {
  Add-Content -Path $info.IniFile -Value "`nextension=php_cassandra.dll`n"
  Write-Host "[INFO] Ajouté extension=php_cassandra.dll dans $($info.IniFile)"
}

Write-Host "[DONE] Validation:"
& $Php -m | Select-String -Pattern '(?i)cassandra' | ForEach-Object { $_.Line } | Write-Host
