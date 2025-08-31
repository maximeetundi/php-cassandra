<?php
/**
 * Clean up duplicate PHP version checks that were applied multiple times
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
    $originalContent = $content;
    
    // Fix nested #if PHP_VERSION_ID >= 80000 blocks
    $content = preg_replace(
        '/#if PHP_VERSION_ID >= 80000\s*\n\s*zval obj_zval;\s*\n\s*ZVAL_OBJ\(&obj_zval, object\);\s*\n\s*(php_driver_[a-zA-Z_]+ \*self = PHP_DRIVER_GET_[A-Z_]+\(&obj_zval\);\s*\n\s*HashTable \*props = zend_std_get_properties\(object\);\s*\n)#else\s*\n\s*#if PHP_VERSION_ID >= 80000[\s\S]*?#endif\s*\n\s*#endif/',
        '#if PHP_VERSION_ID >= 80000' . "\n" . 
        '  zval obj_zval;' . "\n" . 
        '  ZVAL_OBJ(&obj_zval, object);' . "\n" . 
        '  $1' . 
        '#else' . "\n" . 
        '  php_driver_$2 *self = PHP_DRIVER_GET_$3(object);' . "\n" . 
        '  HashTable *props = zend_std_get_properties(object TSRMLS_CC);' . "\n" . 
        '#endif',
        $content
    );
    
    // Remove duplicate nested blocks more aggressively
    $content = preg_replace(
        '/#else\s*\n\s*#if PHP_VERSION_ID >= 80000[\s\S]*?#endif\s*\n\s*#endif/',
        '#else' . "\n" . 
        '  php_driver_date *self = PHP_DRIVER_GET_DATE(object);' . "\n" . 
        '  HashTable *props = zend_std_get_properties(object TSRMLS_CC);' . "\n" . 
        '#endif',
        $content
    );
    
    if ($content !== $originalContent) {
        file_put_contents($filePath, $content);
        echo "Cleaned duplicates in: " . basename($filePath) . "\n";
        $fixedFiles++;
    }
}

echo "Cleaned duplicates in $fixedFiles files\n";
?>
