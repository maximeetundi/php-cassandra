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
#include "util/hash.h"
#include "util/types.h"
#include "util/uuid_gen.h"

#include <ext/date/php_date.h>

zend_class_entry *php_driver_timeuuid_ce = NULL;

void
php_driver_timeuuid_init(INTERNAL_FUNCTION_PARAMETERS)
{
  php_driver_uuid *self;
  zval *param;
  int version;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &param) == FAILURE) {
    return;
  }

  if (getThis() && instanceof_function(Z_OBJCE_P(getThis()), php_driver_timeuuid_ce)) {
    self = PHP_DRIVER_GET_UUID(getThis());
  } else {
    object_init_ex(return_value, php_driver_timeuuid_ce);
    self = PHP_DRIVER_GET_UUID(return_value);
  }


  if (ZEND_NUM_ARGS() == 0) {
    php_driver_uuid_generate_time(&self->uuid);
  } else {

    switch (Z_TYPE_P(param)) {
      case IS_LONG:
        if (Z_LVAL_P(param) < 0) {
          zend_throw_exception_ex(php_driver_invalid_argument_exception_ce, 0, "Timestamp must be a positive integer, %ld given", Z_LVAL_P(param));
          return;
        }
        php_driver_uuid_generate_from_time(Z_LVAL_P(param), &self->uuid);
        break;
      case IS_STRING:
        if (cass_uuid_from_string(Z_STRVAL_P(param), &self->uuid) != CASS_OK) {
          zend_throw_exception_ex(php_driver_invalid_argument_exception_ce, 0, "Invalid UUID: '%.*s'", (int) Z_STRLEN_P(param), Z_STRVAL_P(param));
          return;
        }

        version = cass_uuid_version(self->uuid);
        if (version != 1) {
          zend_throw_exception_ex(php_driver_invalid_argument_exception_ce, 0, "UUID must be of type 1, type %d given", version);
        }
        break;
      default:
          zend_throw_exception_ex(php_driver_invalid_argument_exception_ce, 0, "Invalid argument - integer or string expected");
     }

  }
}

/* {{{ Timeuuid::__construct(string) */
PHP_METHOD(Timeuuid, __construct)
{
  php_driver_timeuuid_init(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tostring, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* {{{ Timeuuid::__toString() */
PHP_METHOD(Timeuuid, __toString)
{
  char string[CASS_UUID_STRING_LENGTH];
  php_driver_uuid *self = PHP_DRIVER_GET_UUID(getThis());

  cass_uuid_string(self->uuid, string);

  RETURN_STRING(string);
}
/* }}} */

/* {{{ Timeuuid::type() */
PHP_METHOD(Timeuuid, type)
{
  zval type = php_driver_type_scalar(CASS_VALUE_TYPE_TIMEUUID);
  RETURN_ZVAL(&type, 1, 1);
}
/* }}} */

/* {{{ Timeuuid::value() */
PHP_METHOD(Timeuuid, uuid)
{
  char string[CASS_UUID_STRING_LENGTH];
  php_driver_uuid *self = PHP_DRIVER_GET_UUID(getThis());

  cass_uuid_string(self->uuid, string);

  RETURN_STRING(string);
}
/* }}} */

/* {{{ Timeuuid::version() */
PHP_METHOD(Timeuuid, version)
{
  php_driver_uuid *self = PHP_DRIVER_GET_UUID(getThis());

  RETURN_LONG((long) cass_uuid_version(self->uuid));
}
/* }}} */

/* {{{ Timeuuid::time() */
PHP_METHOD(Timeuuid, time)
{
  php_driver_uuid *self = PHP_DRIVER_GET_UUID(getThis());
  RETURN_LONG((long) (cass_uuid_timestamp(self->uuid) / 1000));
}
/* }}} */

/* {{{ Timeuuid::toDateTime() */
PHP_METHOD(Timeuuid, toDateTime)
{
  php_driver_uuid *self;
  zval datetime_object;
  zval *datetime = &datetime_object;
  php_date_obj *datetime_obj = NULL;
  char *str;
  int str_len;

  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  self = PHP_DRIVER_GET_UUID(getThis());

  php_date_instantiate(php_date_get_date_ce(), datetime);

  datetime_obj = php_date_obj_from_obj(Z_OBJ_P(datetime));

  str_len      = spprintf(&str, 0, "@%ld", (long) (cass_uuid_timestamp(self->uuid) / 1000));
  php_date_initialize(datetime_obj, str, str_len, NULL, NULL, 0);
  efree(str);

  RETVAL_ZVAL(datetime, 0, 0);
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo__construct, 0, ZEND_RETURN_VALUE, 0)
  ZEND_ARG_INFO(0, timestamp)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_none, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_driver_timeuuid_methods[] = {
  PHP_ME(Timeuuid, __construct, arginfo__construct, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
  PHP_ME(Timeuuid, __toString, arginfo_tostring, ZEND_ACC_PUBLIC)
  PHP_ME(Timeuuid, type, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Timeuuid, uuid, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Timeuuid, version, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Timeuuid, time, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Timeuuid, toDateTime, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_FE_END
};

static zend_object_handlers php_driver_timeuuid_handlers;

#if PHP_VERSION_ID >= 80000
static HashTable *
php_driver_timeuuid_gc(zend_object *object, zval **table, int *n)
{
  *table = NULL;
  *n = 0;
  return zend_std_get_properties(object);
}
#else
static HashTable *
php_driver_timeuuid_gc(zval *object, zval **table, int *n)
{
  *table = NULL;
  *n = 0;
  return zend_std_get_properties(object TSRMLS_CC);
}
#endif

#if PHP_VERSION_ID >= 80000
static HashTable *
php_driver_timeuuid_properties(zend_object *object)
{
  char string[CASS_UUID_STRING_LENGTH];
  zval type;
  zval uuid;
  zval version;
  zval obj_zval;
  ZVAL_OBJ(&obj_zval, object);

  php_driver_uuid *self = PHP_DRIVER_GET_UUID(&obj_zval);
  HashTable      *props = zend_std_get_properties(object);

  type = php_driver_type_scalar(CASS_VALUE_TYPE_TIMEUUID);
  zend_hash_update(props, zend_string_init("type", 4, 0), &type);

  cass_uuid_string(self->uuid, string);

  ZVAL_STRING(&uuid, string);
  zend_hash_update(props, zend_string_init("uuid", 4, 0), &uuid);

  ZVAL_LONG(&version, (long) cass_uuid_version(self->uuid));
  zend_hash_update(props, zend_string_init("version", 7, 0), &version);

  return props;
}
#else
static HashTable *
php_driver_timeuuid_properties(zval *object TSRMLS_DC)
{
  char string[CASS_UUID_STRING_LENGTH];
  zval* type;
  zval* uuid;
  zval* version;

  php_driver_uuid *self = PHP_DRIVER_GET_UUID(object);
  HashTable      *props = zend_std_get_properties(object TSRMLS_CC);

  MAKE_STD_ZVAL(type);
  *type = php_driver_type_scalar(CASS_VALUE_TYPE_TIMEUUID TSRMLS_CC);
  zend_hash_update(props, "type", sizeof("type"), &type, sizeof(zval), NULL);

  cass_uuid_string(self->uuid, string);

  MAKE_STD_ZVAL(uuid);
  ZVAL_STRING(uuid, string, 1);
  zend_hash_update(props, "uuid", sizeof("uuid"), &uuid, sizeof(zval), NULL);

  MAKE_STD_ZVAL(version);
  ZVAL_LONG(version, (long) cass_uuid_version(self->uuid));
  zend_hash_update(props, "version", sizeof("version"), &version, sizeof(zval), NULL);

  return props;
}
#endif

#if PHP_VERSION_ID < 80000
static int
php_driver_timeuuid_compare(zval *obj1, zval *obj2 TSRMLS_DC)
#else
int
php_driver_timeuuid_compare(zval *obj1, zval *obj2)
#endif
{
  php_driver_uuid *uuid1 = NULL;
  php_driver_uuid *uuid2 = NULL;

  if (Z_OBJCE_P(obj1) != Z_OBJCE_P(obj2))
    return 1; /* different classes */

  uuid1 = PHP_DRIVER_GET_UUID(obj1);
  uuid2 = PHP_DRIVER_GET_UUID(obj2);

  if (uuid1->uuid.time_and_version != uuid2->uuid.time_and_version)
    return uuid1->uuid.time_and_version < uuid2->uuid.time_and_version ? -1 : 1;
  if (uuid1->uuid.clock_seq_and_node != uuid2->uuid.clock_seq_and_node)
    return uuid1->uuid.clock_seq_and_node < uuid2->uuid.clock_seq_and_node ? -1 : 1;

  return 0;
}

#if PHP_VERSION_ID < 80000
static unsigned
php_driver_timeuuid_hash_value(zval *obj TSRMLS_DC)
{
  php_driver_uuid *self = PHP_DRIVER_GET_UUID(obj);
  return php_driver_combine_hash(php_driver_bigint_hash(self->uuid.time_and_version),
                                    php_driver_bigint_hash(self->uuid.clock_seq_and_node));
}
#endif

#if PHP_VERSION_ID >= 80000
static void
php_driver_timeuuid_free(zend_object *object)
{
  php_driver_uuid *self = (php_driver_uuid *) ((char *) (object) - XtOffsetOf(php_driver_uuid, std));
  zend_object_std_dtor(&self->std);
}
#else
static void
php_driver_timeuuid_free(void *object TSRMLS_DC)
{
  php_driver_uuid *self = (php_driver_uuid *) object;
  zend_object_std_dtor(&self->zval TSRMLS_CC);
  efree(self);
}
#endif

#if PHP_VERSION_ID >= 80000
static zend_object*
php_driver_timeuuid_new(zend_class_entry *ce)
{
  php_driver_uuid *self = ecalloc(1, sizeof(php_driver_uuid) + zend_object_properties_size(ce));

  zend_object_std_init(&self->std, ce);
  object_properties_init(&self->std, ce);
  self->std.handlers = &php_driver_timeuuid_handlers;

  return &self->std;
}
#else
static zend_object_value
php_driver_timeuuid_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_driver_uuid *self;

  self = (php_driver_uuid *) ecalloc(1, sizeof(php_driver_uuid));

  zend_object_std_init(&self->zval, ce TSRMLS_CC);
  object_properties_init(&self->zval, ce TSRMLS_CC);

  retval.handle = zend_objects_store_put(self,
                                         (zend_objects_store_dtor_t) zend_objects_destroy_object,
                                         php_driver_timeuuid_free, NULL TSRMLS_CC);
  retval.handlers = &php_driver_timeuuid_handlers;

  return retval;
}
#endif

void
php_driver_define_Timeuuid(TSRMLS_D)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, PHP_DRIVER_NAMESPACE "\\Timeuuid", php_driver_timeuuid_methods);
  php_driver_timeuuid_ce = zend_register_internal_class(&ce TSRMLS_CC);
  zend_class_implements(php_driver_timeuuid_ce, 2, php_driver_value_ce, php_driver_uuid_interface_ce);
  memcpy(&php_driver_timeuuid_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

#if PHP_VERSION_ID >= 80000
  php_driver_timeuuid_handlers.offset = XtOffsetOf(php_driver_uuid, std);
  php_driver_timeuuid_handlers.free_obj = php_driver_timeuuid_free;
  php_driver_timeuuid_handlers.get_properties = php_driver_timeuuid_properties;
  php_driver_timeuuid_handlers.get_gc = php_driver_timeuuid_gc;
  php_driver_timeuuid_handlers.compare = php_driver_timeuuid_compare;
  php_driver_timeuuid_handlers.clone_obj = NULL;
#else
  php_driver_timeuuid_handlers.get_properties = php_driver_timeuuid_properties;
  php_driver_timeuuid_handlers.compare_objects = php_driver_timeuuid_compare;
  php_driver_timeuuid_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 50400
  php_driver_timeuuid_handlers.get_gc = php_driver_timeuuid_gc;
#endif
  php_driver_timeuuid_handlers.hash_value = php_driver_timeuuid_hash_value;
#endif

  php_driver_timeuuid_ce->ce_flags |= ZEND_ACC_FINAL;
  php_driver_timeuuid_ce->create_object = php_driver_timeuuid_new;
}