<?php
// Test de l'extension et de la connexion Cassandra (CloudClusters, SSL client cert)

// 1) Vérifier l'extension
if (!extension_loaded('cassandra')) {
    fwrite(STDERR, "[ERREUR] L'extension PHP cassandra n'est pas chargée.\n");
    exit(1);
}
printf("Extension cassandra chargée. Version: %s\n\n", phpversion('cassandra'));

// 2) Paramètres de connexion (depuis cloudclustersio/cassandra/cassandra.txt)
$host = 'cassandra-201683-0.cloudclusters.net';
$port = 10014;

// 3) Chemins des certificats/clé (PEM)
$certPath = __DIR__ . '/cloudclustersio/cassandra/user.cer.pem';
$keyPath  = __DIR__ . '/cloudclustersio/cassandra/user.key.pem';

// Vérifications de base
foreach ([$certPath, $keyPath] as $p) {
    if (!is_file($p)) {
        fwrite(STDERR, "[ERREUR] Fichier introuvable: $p\n");
        exit(1);
    }
}

try {
    // 4) Options SSL: cert client + clé privée
    $sslBuilder = new Cassandra\SSLOptions\Builder();
    $sslBuilder->withClientCert($certPath);
    $sslBuilder->withPrivateKey($keyPath);
    // Facultatif: si vous avez un certificat CA séparé, utilisez ->withTrustedCerts($caPath)
    $ssl = $sslBuilder->build();

    // 5) Construction du cluster
    $clusterBuilder = new Cassandra\Cluster\Builder();
    $clusterBuilder->withContactPoints($host);
    $clusterBuilder->withPort($port);
    $clusterBuilder->withSSL($ssl);
    $cluster = $clusterBuilder->build();

    echo "Connexion au cluster...\n";
    
    // Tenter d'établir une session
    $session = $cluster->connect();
    echo "Connexion établie avec succès!\n";
    
    // Exécuter une requête simple pour tester
    $statement = new Cassandra\SimpleStatement('SELECT release_version FROM system.local');
    $result = $session->execute($statement);
    $row = $result->first();
    if ($row && isset($row['release_version'])) {
        printf("Version de Cassandra: %s\n", (string) $row['release_version']);
    } else {
        echo "Aucune version retournée par system.local.\n";
    }

} catch (Throwable $e) {
    fwrite(STDERR, "Erreur lors de la connexion: " . $e->getMessage() . "\n");
    if (method_exists($e, 'getTraceAsString')) {
        fwrite(STDERR, $e->getTraceAsString() . "\n");
    }
}
?>