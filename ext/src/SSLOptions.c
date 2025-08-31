/**
 * Copyright 2015-2017 DataStax, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "php_driver.h"
#include "php_driver_types.h"

zend_class_entry *php_driver_ssl_ce = NULL;

static zend_function_entry php_driver_ssl_methods[] = {
  PHP_FE_END
};

static zend_object_handlers php_driver_ssl_handlers;

#if PHP_VERSION_ID >= 80000
static HashTable *
php_driver_ssl_properties(zend_object *object)
{
  HashTable *props = zend_std_get_properties(object);
  return props;
}
#else
static HashTable *
php_driver_ssl_properties(zval *object TSRMLS_DC)
{
  HashTable *props = zend_std_get_properties(object TSRMLS_CC);
  return props;
}
#endif

#if PHP_VERSION_ID < 80000
static int
php_driver_ssl_compare(zval *obj1, zval *obj2 TSRMLS_DC)
#else
int
php_driver_ssl_compare(zval *obj1, zval *obj2)
#endif
{
  if (Z_OBJCE_P(obj1) != Z_OBJCE_P(obj2))
    return 1; /* different classes */

  return Z_OBJ_HANDLE_P(obj1) != Z_OBJ_HANDLE_P(obj2);
}

#if PHP_VERSION_ID >= 80000
static void
php_driver_ssl_free(zend_object *object)
{
  php_driver_ssl *self = (php_driver_ssl *) ((char *) (object) - XtOffsetOf(php_driver_ssl, std));
  cass_ssl_free(self->ssl);
  zend_object_std_dtor(&self->std);
}
#else
static void
php_driver_ssl_free(void *object TSRMLS_DC)
{
  php_driver_ssl *self = (php_driver_ssl *) object;
  cass_ssl_free(self->ssl);
  zend_object_std_dtor(&self->zval TSRMLS_CC);
  efree(self);
}
#endif

#if PHP_VERSION_ID >= 80000
static zend_object*
php_driver_ssl_new(zend_class_entry *ce)
{
  php_driver_ssl *self = ecalloc(1, sizeof(php_driver_ssl) + zend_object_properties_size(ce));

  self->ssl = cass_ssl_new();
  zend_object_std_init(&self->std, ce);
  object_properties_init(&self->std, ce);
  self->std.handlers = &php_driver_ssl_handlers;

  return &self->std;
}
#else
static zend_object_value
php_driver_ssl_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_driver_ssl *self;

  self = (php_driver_ssl *) ecalloc(1, sizeof(php_driver_ssl));

  self->ssl = cass_ssl_new();

  zend_object_std_init(&self->zval, ce TSRMLS_CC);
  object_properties_init(&self->zval, ce TSRMLS_CC);

  retval.handle = zend_objects_store_put(self,
                                         (zend_objects_store_dtor_t) zend_objects_destroy_object,
                                         php_driver_ssl_free,
                                         NULL TSRMLS_CC);
  retval.handlers = &php_driver_ssl_handlers;

  return retval;
}
#endif

void php_driver_define_SSLOptions(TSRMLS_D)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, PHP_DRIVER_NAMESPACE "\\SSLOptions", php_driver_ssl_methods);
  php_driver_ssl_ce = zend_register_internal_class(&ce TSRMLS_CC);
  php_driver_ssl_ce->ce_flags     |= ZEND_ACC_FINAL;
  php_driver_ssl_ce->create_object = php_driver_ssl_new;

  memcpy(&php_driver_ssl_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

#if PHP_VERSION_ID >= 80000
  php_driver_ssl_handlers.offset = XtOffsetOf(php_driver_ssl, std);
  php_driver_ssl_handlers.free_obj = php_driver_ssl_free;
  php_driver_ssl_handlers.get_properties = php_driver_ssl_properties;
  php_driver_ssl_handlers.compare = php_driver_ssl_compare;
  php_driver_ssl_handlers.clone_obj = NULL;
#else
  php_driver_ssl_handlers.get_properties = php_driver_ssl_properties;
  php_driver_ssl_handlers.compare_objects = php_driver_ssl_compare;
  php_driver_ssl_handlers.clone_obj = NULL;
#endif
}