<?php
/**
 * Script to fix PHP 8.3 compatibility issues in the Cassandra PHP driver
 */

$srcDir = __DIR__ . '/ext/src';
$files = new RecursiveIteratorIterator(
    new RecursiveDirectoryIterator($srcDir),
    RecursiveIteratorIterator::LEAVES_ONLY
);

$fixedFiles = 0;

foreach ($files as $file) {
    if ($file->getExtension() !== 'c') {
        continue;
    }
    
    $filePath = $file->getPathname();
    $content = file_get_contents($filePath);
    
    if (strpos($content, 'compare_objects') === false) {
        continue;
    }
    
    // Fix compare_objects handler assignment
    $pattern = '/(\s+)([a-zA-Z_]+_handlers\.std\.compare_objects\s*=\s*[a-zA-Z_]+;)/';
    $replacement = '$1#if PHP_VERSION_ID < 80000' . "\n" . '$1$2' . "\n" . '$1#endif';
    
    $newContent = preg_replace($pattern, $replacement, $content);
    
    if ($newContent !== $content) {
        file_put_contents($filePath, $newContent);
        echo "Fixed: " . basename($filePath) . "\n";
        $fixedFiles++;
    }
}

echo "Fixed $fixedFiles files for PHP 8.3 compatibility\n";
?>
