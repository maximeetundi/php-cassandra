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
#include "util/types.h"

zend_class_entry *php_driver_timestamp_gen_monotonic_ce = NULL;

static zend_function_entry php_driver_timestamp_gen_monotonic_methods[] = {
  PHP_FE_END
};

static zend_object_handlers php_driver_timestamp_gen_monotonic_handlers;

#if PHP_VERSION_ID >= 80000
static void
php_driver_timestamp_gen_monotonic_free(zend_object *object)
{
  php_driver_timestamp_gen *self = (php_driver_timestamp_gen *) ((char *) (object) - XtOffsetOf(php_driver_timestamp_gen, std));
  cass_timestamp_gen_free(self->gen);
  zend_object_std_dtor(&self->std);
}
#else
static void
php_driver_timestamp_gen_monotonic_free(void *object TSRMLS_DC)
{
  php_driver_timestamp_gen *self = (php_driver_timestamp_gen *) object;
  cass_timestamp_gen_free(self->gen);
  zend_object_std_dtor(&self->zval TSRMLS_CC);
  efree(self);
}
#endif

#if PHP_VERSION_ID >= 80000
static zend_object*
php_driver_timestamp_gen_monotonic_new(zend_class_entry *ce)
{
  php_driver_timestamp_gen *self = ecalloc(1, sizeof(php_driver_timestamp_gen) + zend_object_properties_size(ce));

  self->gen = cass_timestamp_gen_monotonic_new();
  zend_object_std_init(&self->std, ce);
  object_properties_init(&self->std, ce);
  self->std.handlers = &php_driver_timestamp_gen_monotonic_handlers;

  return &self->std;
}
#else
static zend_object_value
php_driver_timestamp_gen_monotonic_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_driver_timestamp_gen *self;

  self = (php_driver_timestamp_gen *) ecalloc(1, sizeof(php_driver_timestamp_gen));

  self->gen = cass_timestamp_gen_monotonic_new();

  zend_object_std_init(&self->zval, ce TSRMLS_CC);
  object_properties_init(&self->zval, ce TSRMLS_CC);

  retval.handle = zend_objects_store_put(self,
                                         (zend_objects_store_dtor_t) zend_objects_destroy_object,
                                         php_driver_timestamp_gen_monotonic_free, NULL TSRMLS_CC);
  retval.handlers = &php_driver_timestamp_gen_monotonic_handlers;

  return retval;
}
#endif

void php_driver_define_TimestampGeneratorMonotonic(TSRMLS_D)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, PHP_DRIVER_NAMESPACE "\\TimestampGenerator\\Monotonic", php_driver_timestamp_gen_monotonic_methods);
  php_driver_timestamp_gen_monotonic_ce = zend_register_internal_class(&ce TSRMLS_CC);
  zend_class_implements(php_driver_timestamp_gen_monotonic_ce, 1, php_driver_timestamp_gen_ce);
  php_driver_timestamp_gen_monotonic_ce->ce_flags     |= ZEND_ACC_FINAL;
  php_driver_timestamp_gen_monotonic_ce->create_object = php_driver_timestamp_gen_monotonic_new;

  memcpy(&php_driver_timestamp_gen_monotonic_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

#if PHP_VERSION_ID >= 80000
  php_driver_timestamp_gen_monotonic_handlers.offset = XtOffsetOf(php_driver_timestamp_gen, std);
  php_driver_timestamp_gen_monotonic_handlers.free_obj = php_driver_timestamp_gen_monotonic_free;
#endif
}