#!/usr/bin/env bash
set -euo pipefail

# Installer for prebuilt php-cassandra extension (Linux/macOS)
# Requires prebuilt binaries published on GitHub Releases with naming scheme:
#   cassandra-${OS}-${ARCH}-php${PHPVER}-${TS}.so
# Example: cassandra-linux-x86_64-php83-nts.so

REPO="maximeetundi/php-cassandra-driver"
API_BASE="https://github.com/${REPO}/releases/latest/download"

php_bin=${PHP_BIN:-php}
ext_dir=$($php_bin -r 'echo ini_get("extension_dir");')
php_ini=$($php_bin -r '$f=php_ini_loaded_file(); echo $f?$f:"";')
php_api=$($php_bin -r 'echo PHP_VERSION_ID;')
php_zts=$($php_bin -r 'echo ZEND_THREAD_SAFE?"ts":"nts";')
php_major_minor=$($php_bin -r 'printf("%d%d", PHP_MAJOR_VERSION, PHP_MINOR_VERSION);')

if [[ -z "$php_ini" ]]; then
  echo "[INFO] Aucun php.ini chargé. J'essaie d'utiliser php --ini pour le localiser..."
  php_ini=$($php_bin --ini | awk '/Loaded Configuration File/ {print $NF}') || true
fi

os=$(uname -s | tr '[:upper:]' '[:lower:]')
arch=$(uname -m)
case "$arch" in
  x86_64|amd64) arch=x86_64;;
  aarch64|arm64) arch=arm64;;
  *) echo "Architecture non supportée: $arch"; exit 1;;
 esac

case "$os" in
  linux)   ext_name="cassandra-linux-${arch}-php${php_major_minor}-${php_zts}.so";;
  darwin)  ext_name="cassandra-macos-${arch}-php${php_major_minor}-${php_zts}.so";;
  *) echo "OS non supporté: $os"; exit 1;;
 esac

url="${API_BASE}/${ext_name}"

echo "[INFO] Téléchargement: $url"
tmpfile=$(mktemp)
if command -v curl >/dev/null 2>&1; then
  curl -fsSL "$url" -o "$tmpfile"
elif command -v wget >/dev/null 2>&1; then
  wget -q "$url" -O "$tmpfile"
else
  echo "curl ou wget requis"; exit 1
fi

sudo mkdir -p "$ext_dir"
sudo install -m 0644 "$tmpfile" "$ext_dir/cassandra.so"
rm -f "$tmpfile"

echo "[INFO] Extension installée dans: $ext_dir/cassandra.so"

if [[ -z "$php_ini" || ! -f "$php_ini" ]]; then
  echo "[WARN] php.ini introuvable. Ajoutez manuellement: extension=cassandra.so"
  exit 0
fi

if grep -qi '^\s*extension\s*=\s*cassandra\.so' "$php_ini"; then
  echo "[INFO] Entrée déjà présente dans $php_ini"
else
  echo "[INFO] Ajout de extension=cassandra.so dans $php_ini"
  printf "\nextension=cassandra.so\n" | sudo tee -a "$php_ini" >/dev/null
fi

echo "[DONE] php -m | grep cassandra"
$php_bin -m | grep -i cassandra || true
