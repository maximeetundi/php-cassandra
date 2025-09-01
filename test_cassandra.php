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

// 3b) Identifiants (si requis) via variables d'environnement
$username = getenv('CASSANDRA_USERNAME') ?: 'admin';
$password = getenv('CASSANDRA_PASSWORD') ?: 'aa5564@#';

// Vérifications de base
foreach ([$certPath, $keyPath] as $p) {
    if (!is_file($p)) {
        fwrite(STDERR, "[ERREUR] Fichier introuvable: $p\n");
        exit(1);
    }
}

// Pré-déclarer la variable pour pouvoir la fermer dans finally
$session = null;

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
    if ($username !== '' && $password !== '') {
        $clusterBuilder->withCredentials($username, $password);
    }
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

    // Test minimal: création d'un keyspace/table via CQL brut (évite SimpleStatement)
    echo "\n[TEST DDL] Création d'un keyspace et d'une table de test...\n";
    try {
        $session->execute("CREATE KEYSPACE IF NOT EXISTS test_ks WITH replication = {'class': 'SimpleStrategy', 'replication_factor': '1'} AND durable_writes = true");
        $session->execute("CREATE TABLE IF NOT EXISTS test_ks.demo (id int PRIMARY KEY, v text)");
        echo "Keyspace et table créés (ou déjà existants).\n";

        // Insertion et lecture
        $session->execute("INSERT INTO test_ks.demo (id, v) VALUES (1, 'ok')");
        $rows = $session->execute("SELECT id, v FROM test_ks.demo WHERE id = 1");
        $r = $rows->first();
        if ($r) {
            printf("Lecture: id=%d, v=%s\n", (int)$r['id'], (string)$r['v']);
        } else {
            echo "Aucune ligne lue dans test_ks.demo.\n";
        }
    } catch (Throwable $e) {
        fwrite(STDERR, "[TEST DDL] Erreur: " . $e->getMessage() . "\n");
    }

} catch (Throwable $e) {
    fwrite(STDERR, "Erreur lors de la connexion: " . $e->getMessage() . "\n");
    if (method_exists($e, 'getTraceAsString')) {
        fwrite(STDERR, $e->getTraceAsString() . "\n");
    }
} finally {
    // Fermer proprement la session pour éviter un segfault à la fin du script
    if ($session !== null && method_exists($session, 'close')) {
        try { $session->close(); } catch (Throwable $t) { /* ignore */ }
    }
}
?>