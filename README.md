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

```bash
# Installation des dépendances
# Sur Debian/Ubuntu :
sudo apt-get install php8.3-dev cmake g++ libssl-dev

# Sur CentOS/RHEL :
sudo yum install php83-php-devel cmake gcc-c++ openssl-devel

# Compilation
mkdir build
cd build
cmake ..
make

# Installation
sudo make install
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
$cluster   = Cassandra::cluster()
                 ->withContactPoints('127.0.0.1')
                 ->build();
$session   = $cluster->connect('system_schema');
$statement = new Cassandra\SimpleStatement("SELECT * FROM keyspaces");
$result    = $session->execute($statement);

foreach ($result as $row) {
    printf("%-30s %s\n", $row['keyspace_name'], $row['replication']);
}
?>
```

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
