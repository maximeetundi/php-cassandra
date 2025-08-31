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
#include "util/math.h"
#include "util/types.h"

#if !defined(HAVE_STDINT_H) && !defined(_MSC_STDINT_H_)
#  define INT8_MAX 127
#  define INT8_MIN (-INT8_MAX-1)
#endif

zend_class_entry *php_driver_tinyint_ce = NULL;

static int
to_double(zval *result, php_driver_numeric *tinyint)
{
  ZVAL_DOUBLE(result, (double) tinyint->data.tinyint.value);
  return SUCCESS;
}

static int
to_long(zval *result, php_driver_numeric *tinyint)
{
  ZVAL_LONG(result, (zend_long) tinyint->data.tinyint.value);
  return SUCCESS;
}

static int
to_string(zval *result, php_driver_numeric *tinyint)
{
  char *string;
  spprintf(&string, 0, "%d", tinyint->data.tinyint.value);
  ZVAL_STRING(result, string);
  efree(string);
  return SUCCESS;
}

void
php_driver_tinyint_init(INTERNAL_FUNCTION_PARAMETERS)
{
  php_driver_numeric *self;
  zval *value;
  cass_int32_t number;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &value) == FAILURE) {
    return;
  }

  if (getThis() && instanceof_function(Z_OBJCE_P(getThis()), php_driver_tinyint_ce)) {
    self = PHP_DRIVER_GET_NUMERIC(getThis());
  } else {
    object_init_ex(return_value, php_driver_tinyint_ce);
    self = PHP_DRIVER_GET_NUMERIC(return_value);
  }

  if (Z_TYPE_P(value) == IS_OBJECT &&
           instanceof_function(Z_OBJCE_P(value), php_driver_tinyint_ce)) {
    php_driver_numeric *other = PHP_DRIVER_GET_NUMERIC(value);
    self->data.tinyint.value = other->data.tinyint.value;
  } else {
    if (Z_TYPE_P(value) == IS_LONG) {
      number = (cass_int32_t) Z_LVAL_P(value);

      if (number < INT8_MIN || number > INT8_MAX) {
        zend_throw_exception_ex(php_driver_range_exception_ce, 0, 
          "value must be between -128 and 127, %ld given", Z_LVAL_P(value));
        return;
      }
    } else if (Z_TYPE_P(value) == IS_DOUBLE) {
      number = (cass_int32_t) Z_DVAL_P(value);

      if (number < INT8_MIN || number > INT8_MAX) {
        zend_throw_exception_ex(php_driver_range_exception_ce, 0, 
          "value must be between -128 and 127, %g given", Z_DVAL_P(value));
        return;
      }
    } else if (Z_TYPE_P(value) == IS_STRING) {
      if (!php_driver_parse_int(Z_STRVAL_P(value), Z_STRLEN_P(value),
                                        &number)) {
        if (errno == ERANGE) {
          zend_throw_exception_ex(php_driver_range_exception_ce, 0, 
            "value must be between -128 and 127, %s given", Z_STRVAL_P(value));
        }
        return;
      }

      if (number < INT8_MIN || number > INT8_MAX) {
        zend_throw_exception_ex(php_driver_range_exception_ce, 0, 
          "value must be between -128 and 127, %s given", Z_STRVAL_P(value));
        return;
      }
    } else {
      INVALID_ARGUMENT(value, "a long, a double, a numeric string or a " \
                              PHP_DRIVER_NAMESPACE "\Tinyint");
    }
    self->data.tinyint.value = (cass_int8_t) number;
  }
}

/* {{{ Tinyint::__construct(string) */
PHP_METHOD(Tinyint, __construct)
{
  php_driver_tinyint_init(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tostring, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* {{{ Tinyint::__toString() */
PHP_METHOD(Tinyint, __toString)
{
  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(getThis());

  to_string(return_value, self);
}
/* }}} */

/* {{{ Tinyint::type() */
PHP_METHOD(Tinyint, type)
{
  zval type = php_driver_type_scalar(CASS_VALUE_TYPE_TINY_INT);
  RETURN_ZVAL(&type, 1, 1);
}
/* }}} */

/* {{{ Tinyint::value() */
PHP_METHOD(Tinyint, value)
{
  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(getThis());

  to_long(return_value, self);
}
/* }}} */

/* {{{ Tinyint::add() */
PHP_METHOD(Tinyint, add)
{
  zval *addend;
  php_driver_numeric *self;
  php_driver_numeric *tinyint;
  php_driver_numeric *result;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &addend) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(addend) == IS_OBJECT &&
      instanceof_function(Z_OBJCE_P(addend), php_driver_tinyint_ce)) {
    self = PHP_DRIVER_GET_NUMERIC(getThis());
    tinyint = PHP_DRIVER_GET_NUMERIC(addend);

    object_init_ex(return_value, php_driver_tinyint_ce);
    result = PHP_DRIVER_GET_NUMERIC(return_value);

    result->data.tinyint.value = self->data.tinyint.value + tinyint->data.tinyint.value;
    if (result->data.tinyint.value - tinyint->data.tinyint.value != self->data.tinyint.value) {
      zend_throw_exception_ex(php_driver_range_exception_ce, 0, "Sum is out of range");
      return;
    }
  } else {
    INVALID_ARGUMENT(addend, "a " PHP_DRIVER_NAMESPACE "\Tinyint");
  }
}
/* }}} */

/* {{{ Tinyint::sub() */
PHP_METHOD(Tinyint, sub)
{
  zval *difference;
  php_driver_numeric *result = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &difference) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(difference) == IS_OBJECT &&
      instanceof_function(Z_OBJCE_P(difference), php_driver_tinyint_ce)) {
    php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(getThis());
    php_driver_numeric *tinyint = PHP_DRIVER_GET_NUMERIC(difference);

    object_init_ex(return_value, php_driver_tinyint_ce);
    result = PHP_DRIVER_GET_NUMERIC(return_value);

    result->data.tinyint.value = self->data.tinyint.value - tinyint->data.tinyint.value;
    if (result->data.tinyint.value + tinyint->data.tinyint.value != self->data.tinyint.value) {
      zend_throw_exception_ex(php_driver_range_exception_ce, 0, "Difference is out of range");
      return;
    }
  } else {
    INVALID_ARGUMENT(difference, "a " PHP_DRIVER_NAMESPACE "\Tinyint");
  }
}
/* }}} */

/* {{{ Tinyint::mul() */
PHP_METHOD(Tinyint, mul)
{
  zval *multiplier;
  php_driver_numeric *result = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &multiplier) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(multiplier) == IS_OBJECT &&
      instanceof_function(Z_OBJCE_P(multiplier), php_driver_tinyint_ce)) {
    php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(getThis());
    php_driver_numeric *tinyint = PHP_DRIVER_GET_NUMERIC(multiplier);

    object_init_ex(return_value, php_driver_tinyint_ce);
    result = PHP_DRIVER_GET_NUMERIC(return_value);

    result->data.tinyint.value = self->data.tinyint.value * tinyint->data.tinyint.value;
    if (tinyint->data.tinyint.value != 0 &&
        result->data.tinyint.value / tinyint->data.tinyint.value != self->data.tinyint.value) {
      zend_throw_exception_ex(php_driver_range_exception_ce, 0, "Product is out of range");
      return;
    }
  } else {
    INVALID_ARGUMENT(multiplier, "a " PHP_DRIVER_NAMESPACE "\Tinyint");
  }
}
/* }}} */

/* {{{ Tinyint::div() */
PHP_METHOD(Tinyint, div)
{
  zval *divisor;
  php_driver_numeric *result = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &divisor) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(divisor) == IS_OBJECT &&
      instanceof_function(Z_OBJCE_P(divisor), php_driver_tinyint_ce)) {
    php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(getThis());
    php_driver_numeric *tinyint = PHP_DRIVER_GET_NUMERIC(divisor);

    object_init_ex(return_value, php_driver_tinyint_ce);
    result = PHP_DRIVER_GET_NUMERIC(return_value);

    if (tinyint->data.tinyint.value == 0) {
      zend_throw_exception_ex(php_driver_divide_by_zero_exception_ce, 0, "Cannot divide by zero");
      return;
    }

    result->data.tinyint.value = self->data.tinyint.value / tinyint->data.tinyint.value;
  } else {
    INVALID_ARGUMENT(divisor, "a " PHP_DRIVER_NAMESPACE "\Tinyint");
  }
}
/* }}} */

/* {{{ Tinyint::mod() */
PHP_METHOD(Tinyint, mod)
{
  zval *divisor;
  php_driver_numeric *result = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &divisor) == FAILURE) {
    return;
  }

  if (Z_TYPE_P(divisor) == IS_OBJECT &&
      instanceof_function(Z_OBJCE_P(divisor), php_driver_tinyint_ce)) {
    php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(getThis());
    php_driver_numeric *tinyint = PHP_DRIVER_GET_NUMERIC(divisor);

    object_init_ex(return_value, php_driver_tinyint_ce);
    result = PHP_DRIVER_GET_NUMERIC(return_value);

    if (tinyint->data.tinyint.value == 0) {
      zend_throw_exception_ex(php_driver_divide_by_zero_exception_ce, 0, "Cannot modulo by zero");
      return;
    }

    result->data.tinyint.value = self->data.tinyint.value % tinyint->data.tinyint.value;
  } else {
    INVALID_ARGUMENT(divisor, "a " PHP_DRIVER_NAMESPACE "\Tinyint");
  }
}
/* }}} */

/* {{{ Tinyint::abs() */
PHP_METHOD(Tinyint, abs)
{
  php_driver_numeric *result = NULL;
  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(getThis());

  if (self->data.tinyint.value == INT8_MIN) {
    zend_throw_exception_ex(php_driver_range_exception_ce, 0, "Value doesn't exist");
    return;
  }

  object_init_ex(return_value, php_driver_tinyint_ce);
  result = PHP_DRIVER_GET_NUMERIC(return_value);
  result->data.tinyint.value = self->data.tinyint.value < 0 ? -self->data.tinyint.value : self->data.tinyint.value;
}
/* }}} */

/* {{{ Tinyint::neg() */
PHP_METHOD(Tinyint, neg)
{
  php_driver_numeric *result = NULL;
  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(getThis());

  if (self->data.tinyint.value == INT8_MIN) {
    zend_throw_exception_ex(php_driver_range_exception_ce, 0, "Value doesn't exist");
    return;
  }

  object_init_ex(return_value, php_driver_tinyint_ce);
  result = PHP_DRIVER_GET_NUMERIC(return_value);
  result->data.tinyint.value = -self->data.tinyint.value;
}
/* }}} */

/* {{{ Tinyint::sqrt() */
PHP_METHOD(Tinyint, sqrt)
{
  php_driver_numeric *result = NULL;
  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(getThis());

  if (self->data.tinyint.value < 0) {
    zend_throw_exception_ex(php_driver_range_exception_ce, 0, 
                            "Cannot take a square root of a negative number");
    return;
  }

  object_init_ex(return_value, php_driver_tinyint_ce);
  result = PHP_DRIVER_GET_NUMERIC(return_value);
  result->data.tinyint.value = (cass_int8_t) sqrt((long double) self->data.tinyint.value);
}
/* }}} */

/* {{{ Tinyint::toInt() */
PHP_METHOD(Tinyint, toInt)
{
  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(getThis());

  to_long(return_value, self);
}
/* }}} */

/* {{{ Tinyint::toDouble() */
PHP_METHOD(Tinyint, toDouble)
{
  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(getThis());

  to_double(return_value, self);
}
/* }}} */

/* {{{ Tinyint::min() */
PHP_METHOD(Tinyint, min)
{
  php_driver_numeric *tinyint = NULL;
  object_init_ex(return_value, php_driver_tinyint_ce);
  tinyint = PHP_DRIVER_GET_NUMERIC(return_value);
  tinyint->data.tinyint.value = INT8_MIN;
}
/* }}} */

/* {{{ Tinyint::max() */
PHP_METHOD(Tinyint, max)
{
  php_driver_numeric *tinyint = NULL;
  object_init_ex(return_value, php_driver_tinyint_ce);
  tinyint = PHP_DRIVER_GET_NUMERIC(return_value);
  tinyint->data.tinyint.value = INT8_MAX;
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo__construct, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_none, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_num, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, num)
ZEND_END_ARG_INFO()

static zend_function_entry php_driver_tinyint_methods[] = {
  PHP_ME(Tinyint, __construct, arginfo__construct, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, __toString, arginfo_tostring, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, type, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, value, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, add, arginfo_num, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, sub, arginfo_num, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, mul, arginfo_num, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, div, arginfo_num, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, mod, arginfo_num, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, abs, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, neg, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, sqrt, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, toInt, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, toDouble, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tinyint, min, arginfo_none, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_ME(Tinyint, max, arginfo_none, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
  PHP_FE_END
};

static zend_object_handlers php_driver_tinyint_handlers;

#if PHP_VERSION_ID >= 80000
static HashTable *
php_driver_tinyint_gc(zend_object *object, zval **table, int *n)
{
  *table = NULL;
  *n = 0;
  return zend_std_get_properties(object);
}
#else
static HashTable *
php_driver_tinyint_gc(zval *object, zval **table, int *n)
{
  *table = NULL;
  *n = 0;
  return zend_std_get_properties(object TSRMLS_CC);
}
#endif

#if PHP_VERSION_ID >= 80000
static HashTable *
php_driver_tinyint_properties(zend_object *object)
{
  zval type;
  zval value;
  zval obj_zval;
  ZVAL_OBJ(&obj_zval, object);

  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(&obj_zval);
  HashTable         *props = zend_std_get_properties(object);

  type = php_driver_type_scalar(CASS_VALUE_TYPE_TINY_INT);
  zend_hash_update(props, zend_string_init("type", 4, 0), &type);

  to_string(&value, self);
  zend_hash_update(props, zend_string_init("value", 5, 0), &value);

  return props;
}
#else
static HashTable *
php_driver_tinyint_properties(zval *object TSRMLS_DC)
{
  zval* type;
  zval* value;

  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(object);
  HashTable         *props = zend_std_get_properties(object TSRMLS_CC);

  MAKE_STD_ZVAL(type);
  *type = php_driver_type_scalar(CASS_VALUE_TYPE_TINY_INT TSRMLS_CC);
  zend_hash_update(props, "type", sizeof("type"), &type, sizeof(zval), NULL);

  MAKE_STD_ZVAL(value);
  to_string(value, self TSRMLS_CC);
  zend_hash_update(props, "value", sizeof("value"), &value, sizeof(zval), NULL);

  return props;
}
#endif

#if PHP_VERSION_ID < 80000
static int
php_driver_tinyint_compare(zval *obj1, zval *obj2 TSRMLS_DC)
#else
int
php_driver_tinyint_compare(zval *obj1, zval *obj2)
#endif
{
  php_driver_numeric *tinyint1 = NULL;
  php_driver_numeric *tinyint2 = NULL;

  if (Z_OBJCE_P(obj1) != Z_OBJCE_P(obj2))
    return 1; /* different classes */

  tinyint1 = PHP_DRIVER_GET_NUMERIC(obj1);
  tinyint2 = PHP_DRIVER_GET_NUMERIC(obj2);

  if (tinyint1->data.tinyint.value == tinyint2->data.tinyint.value)
    return 0;
  else if (tinyint1->data.tinyint.value < tinyint2->data.tinyint.value)
    return -1;
  else
    return 1;
}

#if PHP_VERSION_ID < 80000
static unsigned
php_driver_tinyint_hash_value(zval *obj TSRMLS_DC)
{
  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(obj);
  return 31 * 17 + self->data.tinyint.value;
}
#endif

#if PHP_VERSION_ID >= 80000
static int
php_driver_tinyint_cast(zend_object *object, zval *retval, int type)
{
  zval obj_zval;
  ZVAL_OBJ(&obj_zval, object);
  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(&obj_zval);

  switch (type) {
  case IS_LONG:
      return to_long(retval, self);
  case IS_DOUBLE:
      return to_double(retval, self);
  case IS_STRING:
      return to_string(retval, self);
  default:
     return FAILURE;
  }

  return SUCCESS;
}
#else
static int
php_driver_tinyint_cast(zval *object, zval *retval, int type TSRMLS_DC)
{
  php_driver_numeric *self = PHP_DRIVER_GET_NUMERIC(object);

  switch (type) {
  case IS_LONG:
      return to_long(retval, self TSRMLS_CC);
  case IS_DOUBLE:
      return to_double(retval, self TSRMLS_CC);
  case IS_STRING:
      return to_string(retval, self TSRMLS_CC);
  default:
     return FAILURE;
  }

  return SUCCESS;
}
#endif

#if PHP_VERSION_ID >= 80000
static void
php_driver_tinyint_free(zend_object *object)
{
  php_driver_numeric *self = (php_driver_numeric *) ((char *) (object) - XtOffsetOf(php_driver_numeric, std));
  zend_object_std_dtor(&self->std);
}
#else
static void
php_driver_tinyint_free(void *object TSRMLS_DC)
{
  php_driver_numeric *self = (php_driver_numeric *) object;
  zend_object_std_dtor(&self->zval TSRMLS_CC);
  efree(self);
}
#endif

#if PHP_VERSION_ID >= 80000
static zend_object*
php_driver_tinyint_new(zend_class_entry *ce)
{
  php_driver_numeric *self = ecalloc(1, sizeof(php_driver_numeric) + zend_object_properties_size(ce));

  self->type = PHP_DRIVER_TINYINT;
  zend_object_std_init(&self->std, ce);
  object_properties_init(&self->std, ce);
  self->std.handlers = &php_driver_tinyint_handlers;

  return &self->std;
}
#else
static zend_object_value
php_driver_tinyint_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_driver_numeric *self;

  self = (php_driver_numeric *) ecalloc(1, sizeof(php_driver_numeric));

  self->type = PHP_DRIVER_TINYINT;

  zend_object_std_init(&self->zval, ce TSRMLS_CC);
  object_properties_init(&self->zval, ce TSRMLS_CC);

  retval.handle = zend_objects_store_put(self,
                                         (zend_objects_store_dtor_t) zend_objects_destroy_object,
                                         php_driver_tinyint_free, NULL TSRMLS_CC);
  retval.handlers = &php_driver_tinyint_handlers;

  return retval;
}
#endif

void php_driver_define_Tinyint(TSRMLS_D)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, PHP_DRIVER_NAMESPACE "\\Tinyint", php_driver_tinyint_methods);
  php_driver_tinyint_ce = zend_register_internal_class(&ce TSRMLS_CC);
  zend_class_implements(php_driver_tinyint_ce, 2, php_driver_value_ce, php_driver_numeric_ce);
  php_driver_tinyint_ce->ce_flags     |= ZEND_ACC_FINAL;
  php_driver_tinyint_ce->create_object = php_driver_tinyint_new;

  memcpy(&php_driver_tinyint_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

#if PHP_VERSION_ID >= 80000
  php_driver_tinyint_handlers.offset = XtOffsetOf(php_driver_numeric, std);
  php_driver_tinyint_handlers.free_obj = php_driver_tinyint_free;
  php_driver_tinyint_handlers.get_properties = php_driver_tinyint_properties;
  php_driver_tinyint_handlers.get_gc = php_driver_tinyint_gc;
  php_driver_tinyint_handlers.compare = php_driver_tinyint_compare;
  php_driver_tinyint_handlers.cast_object = php_driver_tinyint_cast;
  php_driver_tinyint_handlers.clone_obj = NULL;
#else
  php_driver_tinyint_handlers.get_properties = php_driver_tinyint_properties;
  php_driver_tinyint_handlers.compare_objects = php_driver_tinyint_compare;
  php_driver_tinyint_handlers.cast_object = php_driver_tinyint_cast;
  php_driver_tinyint_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 50400
  php_driver_tinyint_handlers.get_gc = php_driver_tinyint_gc;
#endif
  php_driver_tinyint_handlers.hash_value = php_driver_tinyint_hash_value;
#endif
}
