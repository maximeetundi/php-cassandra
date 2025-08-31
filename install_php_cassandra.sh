#!/bin/bash
set -e

# 🚀 Script d'installation de l'extension PHP Cassandra (build statique)
# Compatible Ubuntu + PHP 8.3 (amd64)

# 1. Vérification des dépendances système
echo "[1/5] Installation des dépendances système..."
sudo apt-get update
sudo apt-get install -y libuv1 libssl3 zlib1g libstdc++6 libgmp10

# 2. Détection du dossier des extensions PHP
echo "[2/5] Détection du dossier d'extensions PHP..."
EXT_DIR=$(php -i | grep ^extension_dir | awk '{print $3}')
echo "Extension_dir détecté : $EXT_DIR"

# 3. Copie du module cassandra.so
echo "[3/5] Copie de cassandra.so dans $EXT_DIR..."
sudo cp cassandra.so $EXT_DIR/

# 4. Création du fichier de configuration et activation
echo "[4/5] Activation de l'extension..."
echo "extension=cassandra.so" | sudo tee /etc/php/8.3/mods-available/cassandra.ini > /dev/null
sudo phpenmod cassandra

# 5. Redémarrage de PHP / Apache
echo "[5/5] Redémarrage des services PHP..."
if systemctl is-active --quiet apache2; then
    sudo systemctl restart apache2
fi

if systemctl is-active --quiet php8.3-fpm; then
    sudo systemctl restart php8.3-fpm
fi

echo "✅ Installation terminée ! Vérifiez avec : php -m | grep cassandra"
