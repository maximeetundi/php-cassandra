<?php
// Vérifier si l'extension Cassandra est chargée
if (!extension_loaded('cassandra')) {
    die("L'extension PHP Cassandra n'est pas chargée. Veuillez vérifier votre installation.\n");
}

echo "Extension Cassandra chargée avec succès. Version: " . phpversion('cassandra') . "\n\n";

// Informations de connexion depuis votre fichier de configuration
$host = 'cassandra-201683-0.cloudclusters.net';
$port = 10014;

// Chemins des certificats (ajustés pour votre répertoire actuel)
$certPath = __DIR__ . '/cloudclustersio/cassandra/user.cer.pem';
$keyPath = __DIR__ . '/cloudclustersio/cassandra/user.key.pem';

echo "Tentative de connexion à Cassandra sur $host:$port\n";
echo "Utilisation du certificat: $certPath\n";
echo "Utilisation de la clé: $keyPath\n\n";

try {
    // Créer un objet cluster
    $cluster = Cassandra::cluster()
                 ->withContactPoints($host)
                 ->withPort($port)
                 ->withSSL(new Cassandra\SSLOptions\Builder()
                     ->withClientCert($certPath)
                     ->withPrivateKey($keyPath, '')  // Pas de mot de passe pour la clé privée
                     ->withVerifyFlags(Cassandra::VERIFY_NONE)  // Pour le test, désactiver la vérification
                     ->build())
                 ->build();
    
    echo "Objet cluster créé avec succès.\n";
    
    // Tenter d'établir une session
    $session = $cluster->connect();
    echo "Connexion établie avec succès!\n";
    
    // Exécuter une requête simple pour tester
    $statement = new Cassandra\SimpleStatement("SELECT release_version FROM system.local");
    $result = $session->execute($statement);
    
    foreach ($result as $row) {
        echo "Version de Cassandra: " . $row['release_version'] . "\n";
    }
    
} catch (Exception $e) {
    echo "Erreur lors de la connexion: " . $e->getMessage() . "\n";
    echo "Trace: " . $e->getTraceAsString() . "\n";
}
?>