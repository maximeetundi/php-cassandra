# PHP Driver for Apache Cassandra (Fork)

[![GitHub license](https://img.shields.io/github/license/maximeetundi/php-cassandra-driver)](https://github.com/maximeetundi/php-cassandra-driver/blob/main/LICENSE)
[![PHP Version](https://img.shields.io/badge/php-8.3%2B-blue.svg)](https://www.php.net/)

Fork maintenu du driver PHP pour Apache Cassandra avec support de PHP 8.3+.

Ce fork est basé sur le driver officiel DataStax PHP Driver pour Apache Cassandra, avec des mises à jour de compatibilité pour PHP 8.3+.

## Fonctionnalités

- Support complet de PHP 8.3+
- Compatible avec Apache Cassandra 2.1+
- Utilisation du protocole binaire natif de Cassandra
- Support complet de CQL v3
- Haute performance et faible empreinte mémoire
- Support des types de données avancés de Cassandra
- Gestion des requêtes asynchrones
- Support de la pagination
- Gestion des transactions par lot (batch)
- Support SSL/TLS pour les connexions sécurisées

## Prérequis

- PHP 8.3+ (version Thread Safe recommandée)
- Extension PHP JSON
- Bibliothèque C/C++ DataStax pour Apache Cassandra (inclus dans le dépôt)
- Extensions PHP requises :
  - SPL
  - JSON
  - OpenSSL (pour le support SSL/TLS)

## Installation

### Depuis les sources

```bash
git clone https://github.com/maximeetundi/php-cassandra-driver.git
cd php-cassandra-driver
```

### Compilation

#### Sous Windows

1. Installez les dépendances :
   - Visual Studio 2019 ou supérieur avec les outils de développement C++
   - CMake 3.10 ou supérieur
   - PHP 8.3+ (version TS - Thread Safe)
   - Outils de développement PHP (SDK)

2. Ouvrez une invite de commande développeur Visual Studio (x64 Native Tools Command Prompt)

3. Compilez le projet :
   ```bash
   mkdir build
   cd build
   cmake -G "Visual Studio 17 2022" -A x64 ..
   cmake --build . --config Release
   ```

4. Installation :
   - Copiez le fichier `build/Release/php_cassandra.dll` dans votre répertoire d'extensions PHP
   - Ajoutez `extension=php_cassandra.dll` à votre fichier `php.ini`
   - Redémarrez votre serveur web si nécessaire

#### Sous Linux/macOS

1. Installation des dépendances requises :

```bash
# Sur Debian/Ubuntu :
sudo apt-get install php8.3-dev cmake g++ libssl-dev libuv1-dev zlib1g-dev

# Sur CentOS/RHEL :
sudo yum install php83-php-devel cmake gcc-c++ openssl-devel libuv-devel zlib-devel
```

2. Compilation et installation automatique avec le script :

```bash
cd ext
./install.sh
```

3. Vérifiez que l'extension est correctement installée :

```bash
php -m | grep cassandra
```

4. Si vous rencontrez des erreurs de compilation liées à PHP 8.3, vous devrez peut-être appliquer des correctifs de compatibilité :

```bash
# Correction des problèmes de compatibilité avec PHP 8.3
# Exemple pour le fichier Blob.c
sed -i 's/zend_std_get_properties(object TSRMLS_CC)/zend_std_get_properties(Z_OBJ_P(object) TSRMLS_CC)/g' ext/src/Blob.c
sed -i 's/zend_object_std_dtor(&self->zval TSRMLS_CC)/zend_object_std_dtor(object TSRMLS_CC)/g' ext/src/Blob.c

# Puis relancez la compilation
./install.sh
```

### Installation manuelle des dépendances

Si vous préférez compiler manuellement les dépendances :

```bash
# Clonez le dépôt
git clone https://github.com/maximeetundi/php-cassandra-driver.git
cd php-cassandra-driver

# Compilation de la bibliothèque C++ DataStax
mkdir -p lib/cpp-driver/build
cd lib/cpp-driver/build
cmake -DCMAKE_CXX_FLAGS="-fPIC" -DCASS_BUILD_STATIC=ON -DCASS_BUILD_SHARED=OFF -DCMAKE_BUILD_TYPE=RELEASE -DCASS_USE_ZLIB=ON ..
make
cd ../../../

# Compilation de l'extension PHP
cd ext
phpize
LIBS="-lssl -lcrypto -lz -luv -lm -lstdc++" ./configure --with-cassandra=../lib/cpp-driver/build
make
sudo make install
```

### Installation binaire (une commande, sans compilation)

Des binaires précompilés peuvent être fournis via les Releases GitHub. Utilisez les scripts suivants pour installer automatiquement le bon binaire selon votre OS, architecture et version de PHP.

- Linux/macOS:
  ```bash
  curl -fsSL https://raw.githubusercontent.com/maximeetundi/php-cassandra-driver/main/scripts/install-binary.sh | bash
  # ou
  wget -qO- https://raw.githubusercontent.com/maximeetundi/php-cassandra-driver/main/scripts/install-binary.sh | bash
  ```

- Windows (PowerShell):
  ```powershell
  irm https://raw.githubusercontent.com/maximeetundi/php-cassandra-driver/main/scripts/install-binary.ps1 | iex
  ```

Ces scripts:

- détectent `extension_dir` et `php.ini`
- téléchargent l’asset correspondant à votre plateforme depuis la dernière Release
- installent `cassandra.so` (Linux/macOS) ou `php_cassandra.dll` (Windows)
- ajoutent l’entrée `extension=` dans `php.ini` si nécessaire

Schéma de nommage attendu des assets Release:

- Linux: `cassandra-linux-<arch>-php<majmin>-<nts|ts>.so`
- macOS: `cassandra-macos-<arch>-php<majmin>-<nts|ts>.so`
- Windows: `php_cassandra-windows-<arch>-php<majmin>-<nts|ts>.dll`

Exemples:

- `cassandra-linux-x86_64-php83-nts.so`
- `cassandra-macos-arm64-php83-nts.so`
- `php_cassandra-windows-x86_64-php83-ts.dll`

Validation:

```bash
php -m | grep -i cassandra
```

## Configuration

Ajoutez la configuration suivante à votre `php.ini` :

```ini
[cassandra]
; Chemin vers le fichier de certificat CA (optionnel)
; cassandra.certfile=

; Chemin vers le fichier de clé privée (optionnel)
; cassandra.privatekeyfile=

; Chemin vers le fichier de certificat (optionnel)
; cassandra.certfile=

; Délai d'expiration par défaut en secondes (optionnel, défaut: 30)
; cassandra.default_timeout=30
```

## Utilisation de base

```php
<?php
// Construction via les classes Builder (API PHP 8.3+)
$builder = new Cassandra\Cluster\Builder();
$builder->withContactPoints('127.0.0.1');
$cluster = $builder->build();

$session   = $cluster->connect('system_schema');
$statement = new Cassandra\SimpleStatement('SELECT keyspace_name, replication FROM system_schema.keyspaces');
$result    = $session->execute($statement);

foreach ($result as $row) {
    printf("%-30s %s\n", $row['keyspace_name'], $row['replication']);
}

// Recommandé: fermer proprement la session en fin de script
if (method_exists($session, 'close')) {
    $session->close();
}
?>
```

## Connexion CloudClusters (SSL + Auth)

Exemple minimal avec certificats client et authentification utilisateur/mot de passe. Les chemins pointent vers les fichiers du dossier `cloudclustersio/cassandra/` du dépôt.

```php
$host = 'cassandra-XXXXXX-0.cloudclusters.net';
$port = 10014; // adapté à votre instance

$certPath = __DIR__ . '/cloudclustersio/cassandra/user.cer.pem';
$keyPath  = __DIR__ . '/cloudclustersio/cassandra/user.key.pem';

$username = getenv('CASSANDRA_USERNAME') ?: 'admin'; // ou définissez via votre CI/terminal
$password = getenv('CASSANDRA_PASSWORD') ?: '<votre_mot_de_passe>'; 

$sslBuilder = new Cassandra\SSLOptions\Builder();
$sslBuilder->withClientCert($certPath);
$sslBuilder->withPrivateKey($keyPath);
// Si un CA dédié est fourni par le provider:
// $sslBuilder->withTrustedCerts(__DIR__ . '/cloudclustersio/cassandra/ca.pem');
$ssl = $sslBuilder->build();

$builder = new Cassandra\Cluster\Builder();
$builder->withContactPoints($host);
$builder->withPort($port);
$builder->withSSL($ssl);
$builder->withCredentials($username, $password);
$cluster = $builder->build();

$session = $cluster->connect();
echo "OK connecté\n";

// ... vos requêtes ...

if (method_exists($session, 'close')) {
    $session->close();
}
```

## Dépannage

- __Authentification requise__: erreur « Authentication required but no auth provider set »
  - Appears si `withCredentials()` n'est pas configuré. Ajoutez `withCredentials($username, $password)`.
- __SSL/Certificats__:
  - Assurez-vous que `user.cer.pem` et `user.key.pem` existent et sont lisibles.
  - Si le provider fournit un CA, utilisez `withTrustedCerts($caPath)`.
  - Pour diagnostiquer uniquement (non production): `withVerifyFlags(Cassandra::VERIFY_NONE)`.
- __Segmentation fault à la fin du script__:
  - Fermez explicitement la session: `if (method_exists($session, 'close')) { $session->close(); }`.
  - Utilisez la dernière version de ce fork (compatibilité PHP 8.3).
  - Ouvrez une issue avec version PHP, version de l'extension et la sortie d'erreur si le problème persiste.

## Documentation complète

Consultez la [documentation officielle](https://docs.datastax.com/en/developer/php-driver/latest/) pour des exemples d'utilisation avancés et la référence complète de l'API.

## Contribution

Les contributions sont les bienvenues ! Voici comment contribuer :

1. Forkez le dépôt
2. Créez une branche pour votre fonctionnalité (`git checkout -b feature/ma-nouvelle-fonctionnalite`)
3. Committez vos changements (`git commit -am 'Ajout d\'une nouvelle fonctionnalité'`)
4. Poussez vers la branche (`git push origin feature/ma-nouvelle-fonctionnalite`)
5. Créez une Pull Request

## Licence

Ce projet est sous licence Apache 2.0. Voir le fichier [LICENSE](LICENSE) pour plus de détails.

## Remerciements

Ce fork est basé sur le travail de l'équipe DataStax et des contributeurs du projet original. Un grand merci à tous ceux qui ont contribué au développement et à l'amélioration de ce driver.

## Support

Pour les problèmes et les demandes de fonctionnalités, veuillez ouvrir un ticket sur [GitHub Issues](https://github.com/maximeetundi/php-cassandra-driver/issues).
