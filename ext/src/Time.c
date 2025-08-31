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
#include "util/math.h"
#include <time.h>
#include <ext/date/php_date.h>
#include <ext/date/lib/timelib.h>

zend_class_entry *php_driver_time_ce = NULL;

#if defined(_WIN32)
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <Windows.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <sys/time.h>
#else
#include <time.h>
#endif

#define NUM_NANOSECONDS_PER_DAY 86399999999999LL
#define NANOSECONDS_PER_SECOND 1000000000LL


#if defined(_WIN32)

cass_int64_t php_driver_time_now_ns() {
  FILETIME ft;
  cass_int64_t ns100;
  GetSystemTimeAsFileTime(&ft);
  ns100 = (((cass_int64_t) ft.dwHighDateTime) << 32 |
           (cass_int64_t) ft.dwLowDateTime) -
           116444736000000000LL; /* 100 nanosecond increments between */
  /* Jan. 1, 1601 - Jan. 1, 1970 */
  return (ns100 * 100) % NUM_NANOSECONDS_PER_DAY;
}

#elif defined(__APPLE__) && defined(__MACH__)

cass_int64_t php_driver_time_now_ns() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return cass_time_from_epoch((cass_int64_t) tv.tv_sec) +
      (cass_int64_t) tv.tv_usec * 1000;
}

#else

cass_int64_t php_driver_time_now_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return cass_time_from_epoch((cass_int64_t) ts.tv_sec) +
      (cass_int64_t) ts.tv_nsec;
}

#endif

static int
to_string(zval *result, php_driver_time *time) {
  char *string;
#ifdef WIN32
  spprintf(&string, 0, "%I64d", (long long int) time->time);
#else
  spprintf(&string, 0, "%lld", (long long int) time->time);
#endif
  ZVAL_STRING(result, string);
  efree(string);
  return SUCCESS;
}

void
php_driver_time_init(INTERNAL_FUNCTION_PARAMETERS)
{
  zval *nanoseconds = NULL;
  php_driver_time *self;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &nanoseconds) == FAILURE) {
    return;
  }

  if (getThis() && instanceof_function(Z_OBJCE_P(getThis()), php_driver_time_ce)) {
    self = PHP_DRIVER_GET_TIME(getThis());
  } else {
    object_init_ex(return_value, php_driver_time_ce);
    self = PHP_DRIVER_GET_TIME(return_value);
  }

  if (nanoseconds == NULL) {
    self->time = php_driver_time_now_ns();
  } else {
    if (Z_TYPE_P(nanoseconds) == IS_LONG) {
      self->time = Z_LVAL_P(nanoseconds);
    } else if (Z_TYPE_P(nanoseconds) == IS_STRING) {
      if (!php_driver_parse_bigint(Z_STRVAL_P(nanoseconds), Z_STRLEN_P(nanoseconds),
                                      &self->time)) {
        return;
      }
    } else {
      INVALID_ARGUMENT(nanoseconds, "a string or int representing a number of nanoseconds since midnight");
    }

    if (self->time < 0 || self->time > NUM_NANOSECONDS_PER_DAY) {
      INVALID_ARGUMENT(nanoseconds, "nanoseconds since midnight");
    }
  }
}

/* {{{ Time::__construct(string) */
PHP_METHOD(Time, __construct)
{
  php_driver_time_init(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/* {{{ Time::type() */
PHP_METHOD(Time, type)
{
  zval type = php_driver_type_scalar(CASS_VALUE_TYPE_TIME);
  RETURN_ZVAL(&type, 1, 1);
}
/* }}} */

/* {{{ Time::seconds() */
PHP_METHOD(Time, seconds)
{
  php_driver_time *self = PHP_DRIVER_GET_TIME(getThis());
  RETURN_LONG(self->time / NANOSECONDS_PER_SECOND);
}
/* }}} */

/* {{{ Time::fromDateTime() */
PHP_METHOD(Time, fromDateTime)
{
  php_driver_time *self;
  zval *zdatetime;
  zval retval;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &zdatetime, php_date_get_date_ce()) == FAILURE) {
    return;
  }

  zend_call_method_with_0_params(Z_OBJ_P(zdatetime),
                                 php_date_get_date_ce(),
                                 NULL,
                                 "gettimestamp",
                                 &retval);

  if (Z_TYPE(retval) == IS_LONG) {
    object_init_ex(return_value, php_driver_time_ce);
    self = PHP_DRIVER_GET_TIME(return_value);
    self->time = cass_time_from_epoch(Z_LVAL(retval));
    zval_ptr_dtor(&retval);
    return;
  }
}
/* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tostring, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* {{{ Time::__toString() */
PHP_METHOD(Time, __toString)
{
  php_driver_time *self;

  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  self = PHP_DRIVER_GET_TIME(getThis());
  to_string(return_value, self);
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo__construct, 0, ZEND_RETURN_VALUE, 0)
  ZEND_ARG_INFO(0, nanoseconds)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_datetime, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_OBJ_INFO(0, datetime, DateTime, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_none, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_driver_time_methods[] = {
  PHP_ME(Time, __construct, arginfo__construct, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
  PHP_ME(Time, type, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Time, seconds, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Time, fromDateTime, arginfo_datetime, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Time, __toString, arginfo_tostring, ZEND_ACC_PUBLIC)
  PHP_FE_END
};

static zend_object_handlers php_driver_time_handlers;

#if PHP_VERSION_ID >= 80000
static HashTable *
php_driver_time_gc(zend_object *object, zval **table, int *n)
{
  *table = NULL;
  *n = 0;
  return zend_std_get_properties(object);
}
#else
static HashTable *
php_driver_time_gc(zval *object, zval **table, int *n)
{
  *table = NULL;
  *n = 0;
  return zend_std_get_properties(object TSRMLS_CC);
}
#endif

#if PHP_VERSION_ID >= 80000
static HashTable *
php_driver_time_properties(zend_object *object)
{
  zval type;
  zval nanoseconds;
  zval obj_zval;
  ZVAL_OBJ(&obj_zval, object);

  php_driver_time *self = PHP_DRIVER_GET_TIME(&obj_zval);
  HashTable      *props = zend_std_get_properties(object);

  type = php_driver_type_scalar(CASS_VALUE_TYPE_TIME);
  zend_hash_update(props, zend_string_init("type", 4, 0), &type);

  to_string(&nanoseconds, self);
  zend_hash_update(props, zend_string_init("nanoseconds", 11, 0), &nanoseconds);

  return props;
}
#else
static HashTable *
php_driver_time_properties(zval *object TSRMLS_DC)
{
  zval* type;
  zval* nanoseconds;

  php_driver_time *self = PHP_DRIVER_GET_TIME(object);
  HashTable      *props = zend_std_get_properties(object TSRMLS_CC);

  MAKE_STD_ZVAL(type);
  *type = php_driver_type_scalar(CASS_VALUE_TYPE_TIME TSRMLS_CC);
  zend_hash_update(props, "type", sizeof("type"), &type, sizeof(zval), NULL);

  MAKE_STD_ZVAL(nanoseconds);
  to_string(nanoseconds, self TSRMLS_CC);
  zend_hash_update(props, "nanoseconds", sizeof("nanoseconds"), &nanoseconds, sizeof(zval), NULL);

  return props;
}
#endif

#if PHP_VERSION_ID < 80000
static int
php_driver_time_compare(zval *obj1, zval *obj2 TSRMLS_DC)
#else
int
php_driver_time_compare(zval *obj1, zval *obj2)
#endif
{
  php_driver_time *time1 = NULL;
  php_driver_time *time2 = NULL;
  if (Z_OBJCE_P(obj1) != Z_OBJCE_P(obj2))
    return 1; /* different classes */

  time1 = PHP_DRIVER_GET_TIME(obj1);
  time2 = PHP_DRIVER_GET_TIME(obj2);

  return PHP_DRIVER_COMPARE(time1->time, time2->time);
}

#if PHP_VERSION_ID < 80000
static unsigned
php_driver_time_hash_value(zval *obj TSRMLS_DC)
{
  php_driver_time *self = PHP_DRIVER_GET_TIME(obj);
  return php_driver_bigint_hash(self->time);
}
#endif

#if PHP_VERSION_ID >= 80000
static void
php_driver_time_free(zend_object *object)
{
  php_driver_time *self = (php_driver_time *) ((char *) (object) - XtOffsetOf(php_driver_time, std));
  zend_object_std_dtor(&self->std);
}
#else
static void
php_driver_time_free(void *object TSRMLS_DC)
{
  php_driver_time *self = (php_driver_time *) object;
  zend_object_std_dtor(&self->zval TSRMLS_CC);
  efree(self);
}
#endif

#if PHP_VERSION_ID >= 80000
static zend_object*
php_driver_time_new(zend_class_entry *ce)
{
  php_driver_time *self = ecalloc(1, sizeof(php_driver_time) + zend_object_properties_size(ce));

  self->time = 0;
  zend_object_std_init(&self->std, ce);
  object_properties_init(&self->std, ce);
  self->std.handlers = &php_driver_time_handlers;

  return &self->std;
}
#else
static zend_object_value
php_driver_time_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_driver_time *self;

  self = (php_driver_time *) ecalloc(1, sizeof(php_driver_time));

  self->time = 0;

  zend_object_std_init(&self->zval, ce TSRMLS_CC);
  object_properties_init(&self->zval, ce TSRMLS_CC);

  retval.handle = zend_objects_store_put(self,
                                         (zend_objects_store_dtor_t) zend_objects_destroy_object,
                                         php_driver_time_free, NULL TSRMLS_CC);
  retval.handlers = &php_driver_time_handlers;

  return retval;
}
#endif

void php_driver_define_Time(TSRMLS_D)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, PHP_DRIVER_NAMESPACE "\\Time", php_driver_time_methods);
  php_driver_time_ce = zend_register_internal_class(&ce TSRMLS_CC);
  zend_class_implements(php_driver_time_ce, 1, php_driver_value_ce);
  memcpy(&php_driver_time_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

#if PHP_VERSION_ID >= 80000
  php_driver_time_handlers.offset = XtOffsetOf(php_driver_time, std);
  php_driver_time_handlers.free_obj = php_driver_time_free;
  php_driver_time_handlers.get_properties = php_driver_time_properties;
  php_driver_time_handlers.get_gc = php_driver_time_gc;
  php_driver_time_handlers.compare = php_driver_time_compare;
  php_driver_time_handlers.clone_obj = NULL;
#else
  php_driver_time_handlers.get_properties = php_driver_time_properties;
  php_driver_time_handlers.compare_objects = php_driver_time_compare;
  php_driver_time_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 50400
  php_driver_time_handlers.get_gc = php_driver_time_gc;
#endif
  php_driver_time_handlers.hash_value = php_driver_time_hash_value;
#endif

  php_driver_time_ce->ce_flags |= ZEND_ACC_FINAL;
  php_driver_time_ce->create_object = php_driver_time_new;
}
