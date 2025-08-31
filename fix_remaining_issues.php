<?php
/**
 * Fix remaining PHP 8.3 compatibility issues
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
    
    // Fix 1: zend_std_get_properties calls in _gc functions
    $content = preg_replace(
        '/return zend_std_get_properties\(object TSRMLS_CC\);/',
        '#if PHP_VERSION_ID >= 80000' . "\n" . 
        '  return zend_std_get_properties(object);' . "\n" . 
        '#else' . "\n" . 
        '  return zend_std_get_properties(object TSRMLS_CC);' . "\n" . 
        '#endif',
        $content
    );
    
    // Fix 2: PHP_DRIVER_GET_* calls in properties functions for PHP 8.0+
    $content = preg_replace(
        '/(php_driver_[a-zA-Z_]+ \*self = PHP_DRIVER_GET_[A-Z_]+\(object\);\s*HashTable \*props = zend_std_get_properties\(object TSRMLS_CC\);)/',
        '#if PHP_VERSION_ID >= 80000' . "\n" . 
        '  zval obj_zval;' . "\n" . 
        '  ZVAL_OBJ(&obj_zval, object);' . "\n" . 
        '  $1' . "\n" . 
        '  HashTable *props = zend_std_get_properties(object);' . "\n" . 
        '#else' . "\n" . 
        '  $1' . "\n" . 
        '#endif',
        $content
    );
    
    // Fix 3: Clean up malformed compare_objects fixes
    $content = preg_replace(
        '/\s*#if PHP_VERSION_ID < 80000\s*\n\s*([a-zA-Z_]+_handlers\.std\.compare_objects = [a-zA-Z_]+_compare;)\s*\n\s*#endif/',
        "\n" . '#if PHP_VERSION_ID < 80000' . "\n" . '  $1' . "\n" . '#endif',
        $content
    );
    
    if ($content !== $originalContent) {
        file_put_contents($filePath, $content);
        echo "Fixed remaining issues in: " . basename($filePath) . "\n";
        $fixedFiles++;
    }
}

echo "Fixed remaining issues in $fixedFiles files\n";
?>
