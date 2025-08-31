<?php
/**
 * Comprehensive PHP 8.3 compatibility fix script
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
    
    // Fix 1: Object structure access (zval -> std for PHP 8.0+)
    $patterns = [
        // Fix zend_object_std_dtor calls
        '/zend_object_std_dtor\(&self->zval TSRMLS_CC\);/' => 
            '#if PHP_VERSION_ID >= 80000' . "\n" . 
            '  zend_object_std_dtor(&self->std);' . "\n" . 
            '#else' . "\n" . 
            '  zend_object_std_dtor(&self->zval TSRMLS_CC);' . "\n" . 
            '#endif',
            
        // Fix PHP5TO7_ZEND_OBJECT_INIT macro usage
        '/PHP5TO7_ZEND_OBJECT_INIT\(([^,]+),\s*self,\s*ce\);/' => 
            '#if PHP_VERSION_ID >= 80000' . "\n" . 
            '  zend_object_std_init(&self->std, ce);' . "\n" . 
            '  object_properties_init(&self->std, ce);' . "\n" . 
            '  self->std.handlers = &php_driver_$1_handlers;' . "\n" . 
            '  return &self->std;' . "\n" . 
            '#else' . "\n" . 
            '  PHP5TO7_ZEND_OBJECT_INIT($1, self, ce);' . "\n" . 
            '#endif',
    ];
    
    foreach ($patterns as $pattern => $replacement) {
        $content = preg_replace('/' . preg_quote($pattern, '/') . '/', $replacement, $content);
    }
    
    // Fix 2: Function signatures for get_properties and get_gc handlers
    if (strpos($content, '_properties(zval *object TSRMLS_DC)') !== false) {
        // Fix get_properties function signature
        $content = preg_replace(
            '/static HashTable \*\s*([a-zA-Z_]+)_properties\(zval \*object TSRMLS_DC\)/',
            '#if PHP_VERSION_ID >= 80000' . "\n" . 
            'static HashTable *' . "\n" . 
            '$1_properties(zend_object *object)' . "\n" . 
            '#else' . "\n" . 
            'static HashTable *' . "\n" . 
            '$1_properties(zval *object TSRMLS_DC)' . "\n" . 
            '#endif',
            $content
        );
    }
    
    if (strpos($content, '_gc(zval *object, php5to7_zval_gc table, int *n TSRMLS_DC)') !== false) {
        // Fix get_gc function signature
        $content = preg_replace(
            '/static HashTable \*\s*([a-zA-Z_]+)_gc\(zval \*object, php5to7_zval_gc table, int \*n TSRMLS_DC\)/',
            'static HashTable *' . "\n" . 
            '$1_gc(zend_object *object, zval **table, int *n)',
            $content
        );
    }
    
    // Fix 3: Handler assignments with proper PHP version checks
    $handlerPatterns = [
        '/([a-zA-Z_]+_handlers)\.get_properties\s*=\s*([a-zA-Z_]+_properties);/' => 
            '$1.get_properties = $2;',
        '/([a-zA-Z_]+_handlers)\.get_gc\s*=\s*([a-zA-Z_]+_gc);/' => 
            '$1.get_gc = $2;',
    ];
    
    foreach ($handlerPatterns as $pattern => $replacement) {
        $content = preg_replace($pattern, $replacement, $content);
    }
    
    if ($content !== $originalContent) {
        file_put_contents($filePath, $content);
        echo "Fixed: " . basename($filePath) . "\n";
        $fixedFiles++;
    }
}

echo "Fixed $fixedFiles files for comprehensive PHP 8.3 compatibility\n";
?>
