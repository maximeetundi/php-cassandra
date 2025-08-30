#ifndef PHP_DRIVER_GLOBALS_H
#define PHP_DRIVER_GLOBALS_H

#include "php.h"
#include "standard/php_smart_string.h"

ZEND_BEGIN_MODULE_GLOBALS(php_driver)
  CassUuidGen  *uuid_gen;
  pid_t         uuid_gen_pid;
  unsigned int  persistent_clusters;
  unsigned int  persistent_sessions;
  
  /* Type definitions for different PHP versions */
#if PHP_VERSION_ID >= 80000
  /* PHP 8.0+ uses zval directly */
  zval          type_varchar;
  zval          type_text;
  zval          type_blob;
  zval          type_ascii;
  zval          type_bigint;
  zval          type_counter;
  zval          type_int;
  zval          type_varint;
  zval          type_boolean;
  zval          type_decimal;
  zval          type_double;
  zval          type_float;
  zval          type_inet;
  zval          type_timestamp;
  zval          type_date;
  zval          type_time;
  zval          type_uuid;
  zval          type_timeuuid;
  zval          type_smallint;
  zval          type_tinyint;
  zval          type_duration;
#else
  /* PHP 5.x and 7.x compatibility */
  php5to7_zval  type_varchar;
  php5to7_zval  type_text;
  php5to7_zval  type_blob;
  php5to7_zval  type_ascii;
  php5to7_zval  type_bigint;
  php5to7_zval  type_counter;
  php5to7_zval  type_int;
  php5to7_zval  type_varint;
  php5to7_zval  type_boolean;
  php5to7_zval  type_decimal;
  php5to7_zval  type_double;
  php5to7_zval  type_float;
  php5to7_zval  type_inet;
  php5to7_zval  type_timestamp;
  php5to7_zval  type_date;
  php5to7_zval  type_time;
  php5to7_zval  type_uuid;
  php5to7_zval  type_timeuuid;
  php5to7_zval  type_smallint;
  php5to7_zval  type_tinyint;
  php5to7_zval  type_duration;
#endif
  
  /* Additional globals for PHP 8.3+ */
#if PHP_VERSION_ID >= 80300
  zend_long     log_level;
  char         *log_location;
#endif
  
ZEND_END_MODULE_GLOBALS(php_driver)

ZEND_EXTERN_MODULE_GLOBALS(php_driver)

/* Helper macros for PHP 8.3+ compatibility */
#if PHP_VERSION_ID >= 80300
# define PHP_DRIVER_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(php_driver, v)
#else
# ifdef ZTS
#  define PHP_DRIVER_G(v) TSRMG(php_driver_globals_id, zend_php_driver_globals *, v)
# else
#  define PHP_DRIVER_G(v) (php_driver_globals.v)
# endif
#endif

#endif /* PHP_DRIVER_GLOBALS_H */
