<?php

$files = [
    'ext/src/Decimal.c',
    'ext/src/Duration.c',
    'ext/src/Float.c',
    'ext/src/Inet.c',
    'ext/src/Map.c',
    'ext/src/Set.c',
    'ext/src/Time.c',
    'ext/src/Timestamp.c',
    'ext/src/Timeuuid.c',
    'ext/src/Tuple.c',
    'ext/src/UserTypeValue.c',
    'ext/src/Uuid.c',
    'ext/src/Varint.c'
];

foreach ($files as $file) {
    if (!file_exists($file)) continue;
    
    $content = file_get_contents($file);
    $original = $content;
    
    // Fix get_properties function signatures
    $content = preg_replace(
        '/static HashTable \*\s*(\w+)_properties\(zval \*object TSRMLS_DC\)/',
        "#if PHP_VERSION_ID >= 80000\nstatic HashTable *\n$1_properties(zend_object *object)\n{\n  zval obj_zval;\n  ZVAL_OBJ(&obj_zval, object);\n  // Function body will be updated below\n#else\nstatic HashTable *\n$1_properties(zval *object TSRMLS_DC)\n{\n#endif",
        $content
    );
    
    // Fix get_gc function signatures
    $content = preg_replace(
        '/static HashTable \*\s*(\w+)_gc\(zval \*object, zval \*\*table, int \*n TSRMLS_DC\)/',
        "#if PHP_VERSION_ID >= 80000\nstatic HashTable *\n$1_gc(zend_object *object, zval **table, int *n)\n{\n  zval obj_zval;\n  ZVAL_OBJ(&obj_zval, object);\n  // Function body will be updated below\n#else\nstatic HashTable *\n$1_gc(zval *object, zval **table, int *n TSRMLS_DC)\n{\n#endif",
        $content
    );
    
    // Fix zend_std_get_properties calls in get_gc functions
    $content = preg_replace(
        '/return zend_std_get_properties\(object\);/',
        "#if PHP_VERSION_ID >= 80000\n  return zend_std_get_properties(object);\n#else\n  return zend_std_get_properties(Z_OBJ_P(object) TSRMLS_CC);\n#endif",
        $content
    );
    
    // Fix zend_std_get_properties calls in properties functions  
    $content = preg_replace(
        '/HashTable\s+\*props = zend_std_get_properties\(object TSRMLS_CC\);/',
        "#if PHP_VERSION_ID >= 80000\n  HashTable *props = zend_std_get_properties(object);\n#else\n  HashTable *props = zend_std_get_properties(Z_OBJ_P(object) TSRMLS_CC);\n#endif",
        $content
    );
    
    // Wrap compare functions
    $content = preg_replace(
        '/static int\s*(\w+)_compare\(zval \*obj1, zval \*obj2 TSRMLS_DC\)/',
        "#if PHP_VERSION_ID < 80000\nstatic int\n$1_compare(zval *obj1, zval *obj2 TSRMLS_DC)",
        $content
    );
    
    // Wrap free functions for PHP < 8.0
    $content = preg_replace(
        '/static void\s*(\w+)_free\(php5to7_zend_object_free \*object TSRMLS_DC\)/',
        "#if PHP_VERSION_ID < 80000\nstatic void\n$1_free(php5to7_zend_object_free *object TSRMLS_DC)",
        $content
    );
    
    // Fix object structure access in free functions
    $content = preg_replace(
        '/zend_object_std_dtor\(&self->zval TSRMLS_CC\);/',
        "#if PHP_VERSION_ID >= 80000\n  zend_object_std_dtor(&self->std);\n#else\n  zend_object_std_dtor(&self->zval TSRMLS_CC);\n#endif",
        $content
    );
    
    // Replace PHP5TO7_ZEND_OBJECT_INIT macro calls with explicit PHP 8.0+ handling
    $content = preg_replace(
        '/PHP5TO7_ZEND_OBJECT_INIT\((\w+), self, ce\);/',
        "#if PHP_VERSION_ID >= 80000\n  zend_object_std_init(&self->std, ce);\n  object_properties_init(&self->std, ce);\n  self->std.handlers = &php_driver_$1_handlers.std;\n  return &self->std;\n#else\n  zend_object_std_init(&self->zval, ce);\n  object_properties_init(&self->zval, ce);\n  self->zval.handlers = &php_driver_$1_handlers;\n  return &self->zval;\n#endif",
        $content
    );
    
    // Replace PHP5TO7_ZEND_OBJECT_INIT_EX macro calls
    $content = preg_replace(
        '/PHP5TO7_ZEND_OBJECT_INIT_EX\((\w+), (\w+), self, ce\);/',
        "#if PHP_VERSION_ID >= 80000\n  zend_object_std_init(&self->std, ce);\n  object_properties_init(&self->std, ce);\n  self->std.handlers = &php_driver_$2_handlers.std;\n  return &self->std;\n#else\n  zend_object_std_init(&self->zval, ce);\n  object_properties_init(&self->zval, ce);\n  self->zval.handlers = &php_driver_$2_handlers;\n  return &self->zval;\n#endif",
        $content
    );
    
    // Close compare function conditionals
    $content = preg_replace(
        '/(\w+_compare\(zval \*obj1, zval \*obj2 TSRMLS_DC\)\s*\{[^}]+\})\s*(?=\s*static)/s',
        "$1\n#endif\n\n",
        $content
    );
    
    // Close free function conditionals  
    $content = preg_replace(
        '/(\w+_free\(php5to7_zend_object_free \*object TSRMLS_DC\)\s*\{[^}]+\})\s*(?=\s*static)/s',
        "$1\n#endif\n\n",
        $content
    );
    
    if ($content !== $original) {
        file_put_contents($file, $content);
        echo "Fixed: $file\n";
    }
}

echo "Done fixing remaining object structure issues.\n";
?>
