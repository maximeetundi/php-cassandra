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
#include "util/collections.h"
#include "util/hash.h"
#include "util/types.h"

#include "src/Type/Tuple.h"
#include "src/Tuple.h"

#include <zend_hash.h>

zend_class_entry *php_driver_tuple_ce = NULL;

void
php_driver_tuple_set(php_driver_tuple *tuple, zend_ulong index, zval *object)
{
  zend_hash_index_update(&tuple->values, index, object);
  Z_TRY_ADDREF_P(object);
  tuple->dirty = 1;
}

static void
php_driver_tuple_populate(php_driver_tuple *tuple, zval *array)
{
  zend_ulong index;
  php_driver_type *type;
  zval *current;
  zval null;

  ZVAL_NULL(&null);

  type = PHP_DRIVER_GET_TYPE(&tuple->type);

  ZEND_HASH_FOREACH_NUM_KEY_VAL(&type->data.tuple.types, index, current) {
    zval *value = NULL;
    if ((value = zend_hash_index_find(&tuple->values, index))) {
      if (add_next_index_zval(array, value) == SUCCESS)
        Z_TRY_ADDREF_P(value);
      else
        break;
    } else {
      if (add_next_index_zval(array, &null) == SUCCESS)
        Z_TRY_ADDREF_P(&null);
      else
        break;
    }
  } ZEND_HASH_FOREACH_END();
}

/* {{{ Tuple::__construct(types) */
PHP_METHOD(Tuple, __construct)
{
  php_driver_tuple *self;
  php_driver_type *type;
  HashTable *types;
  zval *current;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "h", &types) == FAILURE) {
    return;
  }

  self = PHP_DRIVER_GET_TUPLE(getThis());
  self->type = php_driver_type_tuple();
  type = PHP_DRIVER_GET_TYPE(&self->type);

  ZEND_HASH_FOREACH_VAL(types, current) {
    zval *sub_type = current;
    zval scalar_type;

    if (Z_TYPE_P(sub_type) == IS_STRING) {
      CassValueType value_type;
      if (!php_driver_value_type(Z_STRVAL_P(sub_type), &value_type)) {
        return;
      }
      scalar_type = php_driver_type_scalar(value_type);
      if (!php_driver_type_tuple_add(type,
                                        &scalar_type)) {
        return;
      }
    } else if (Z_TYPE_P(sub_type) == IS_OBJECT &&
               instanceof_function(Z_OBJCE_P(sub_type), php_driver_type_ce)) {
      if (!php_driver_type_validate(sub_type, "type")) {
        return;
      }
      if (php_driver_type_tuple_add(type,
                                        sub_type)) {
        Z_ADDREF_P(sub_type);
      } else {
        return;
      }
    } else {
      INVALID_ARGUMENT(sub_type, "a string or an instance of " PHP_DRIVER_NAMESPACE "\\Type");
    }

  } ZEND_HASH_FOREACH_END();
}
/* }}} */

/* {{{ Tuple::type() */
PHP_METHOD(Tuple, type)
{
  php_driver_tuple *self = PHP_DRIVER_GET_TUPLE(getThis());
  RETURN_ZVAL(&self->type, 1, 0);
}

/* {{{ Tuple::values() */
PHP_METHOD(Tuple, values)
{
  php_driver_tuple *self = NULL;
  array_init(return_value);
  self = PHP_DRIVER_GET_TUPLE(getThis());
  php_driver_tuple_populate(self, return_value);
}
/* }}} */

/* {{{ Tuple::set(int, mixed) */
PHP_METHOD(Tuple, set)
{
  php_driver_tuple *self = NULL;
  zend_long index;
  php_driver_type *type;
  zval *sub_type;
  zval *value;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz", &index, &value) == FAILURE)
    return;

  self = PHP_DRIVER_GET_TUPLE(getThis());
  type = PHP_DRIVER_GET_TYPE(&self->type);

  if (index < 0 || index >= zend_hash_num_elements(&type->data.tuple.types)) {
    zend_throw_exception_ex(php_driver_invalid_argument_exception_ce, 0,
                            "Index out of bounds");
    return;
  }

  if (!(sub_type = zend_hash_index_find(&type->data.tuple.types, index)) ||
      !php_driver_validate_object(value,
                                  sub_type)) {
    return;
  }

  php_driver_tuple_set(self, index, value);
}
/* }}} */

/* {{{ Tuple::get(int) */
PHP_METHOD(Tuple, get)
{
  php_driver_tuple *self = NULL;
  zend_long index;
  php_driver_type *type;
  zval *value;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &index) == FAILURE)
    return;

  self = PHP_DRIVER_GET_TUPLE(getThis());
  type = PHP_DRIVER_GET_TYPE(&self->type);

  if (index < 0 || index >= zend_hash_num_elements(&type->data.tuple.types)) {
    zend_throw_exception_ex(php_driver_invalid_argument_exception_ce, 0,
                            "Index out of bounds");
    return;
  }

  if ((value = zend_hash_index_find(&self->values, index))) {
    RETURN_ZVAL(value, 1, 0);
  }
}
/* }}} */

/* {{{ Tuple::count() */
PHP_METHOD(Tuple, count)
{
  php_driver_tuple *self = PHP_DRIVER_GET_TUPLE(getThis());
  php_driver_type *type = PHP_DRIVER_GET_TYPE(&self->type);
  RETURN_LONG(zend_hash_num_elements(&type->data.tuple.types));
}
/* }}} */

/* {{{ Tuple::current() */
PHP_METHOD(Tuple, current)
{
  zend_ulong index;
  php_driver_tuple *self = PHP_DRIVER_GET_TUPLE(getThis());
  php_driver_type *type = PHP_DRIVER_GET_TYPE(&self->type);

  if (zend_hash_get_current_key_ex(&type->data.tuple.types, NULL, &index, &self->pos) == HASH_KEY_IS_LONG) {
    zval *value;
    if ((value = zend_hash_index_find(&self->values, index))) {
      RETURN_ZVAL(value, 1, 0);
    }
  }
}
/* }}} */

/* {{{ Tuple::key() */
PHP_METHOD(Tuple, key)
{
  zend_ulong index;
  php_driver_tuple *self = PHP_DRIVER_GET_TUPLE(getThis());
  php_driver_type *type = PHP_DRIVER_GET_TYPE(&self->type);
  if (zend_hash_get_current_key_ex(&type->data.tuple.types, NULL, &index, &self->pos) == HASH_KEY_IS_LONG) {
    RETURN_LONG(index);
  }
}
/* }}} */

/* {{{ Tuple::next() */
PHP_METHOD(Tuple, next)
{
  php_driver_tuple *self = PHP_DRIVER_GET_TUPLE(getThis());
  php_driver_type *type = PHP_DRIVER_GET_TYPE(&self->type);
  zend_hash_move_forward_ex(&type->data.tuple.types, &self->pos);
}
/* }}} */

/* {{{ Tuple::valid() */
PHP_METHOD(Tuple, valid)
{
  php_driver_tuple *self = PHP_DRIVER_GET_TUPLE(getThis());
  php_driver_type *type = PHP_DRIVER_GET_TYPE(&self->type);
  RETURN_BOOL(zend_hash_has_more_elements_ex(&type->data.tuple.types, &self->pos) == SUCCESS);
}
/* }}} */

/* {{{ Tuple::rewind() */
PHP_METHOD(Tuple, rewind)
{
  php_driver_tuple *self = PHP_DRIVER_GET_TUPLE(getThis());
  php_driver_type *type = PHP_DRIVER_GET_TYPE(&self->type);
  zend_hash_internal_pointer_reset_ex(&type->data.tuple.types, &self->pos);
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo__construct, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, types)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_value, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_index, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_none, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_driver_tuple_methods[] = {
  PHP_ME(Tuple, __construct, arginfo__construct, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
  PHP_ME(Tuple, type, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tuple, values, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tuple, set, arginfo_value, ZEND_ACC_PUBLIC)
  PHP_ME(Tuple, get, arginfo_index, ZEND_ACC_PUBLIC)
  /* Countable */
  PHP_ME(Tuple, count, arginfo_none, ZEND_ACC_PUBLIC)
  /* Iterator */
  PHP_ME(Tuple, current, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tuple, key, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tuple, next, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tuple, valid, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Tuple, rewind, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_FE_END
};

static zend_object_handlers php_driver_tuple_handlers;

#if PHP_VERSION_ID >= 80000
static HashTable *
php_driver_tuple_gc(zend_object *object, zval **table, int *n)
{
  *table = NULL;
  *n = 0;
  return zend_std_get_properties(object);
}
#else
static HashTable *
php_driver_tuple_gc(zval *object, zval **table, int *n)
{
  *table = NULL;
  *n = 0;
  return zend_std_get_properties(object TSRMLS_CC);
}
#endif

#if PHP_VERSION_ID >= 80000
static HashTable *
php_driver_tuple_properties(zend_object *object)
#else
static HashTable *
php_driver_tuple_properties(zval *object TSRMLS_DC)
#endif
{
  zval values;
  zval obj_zval;
  ZVAL_OBJ(&obj_zval, object);

  php_driver_tuple  *self = PHP_DRIVER_GET_TUPLE(&obj_zval);
#if PHP_VERSION_ID >= 80000
  HashTable *props = zend_std_get_properties(object);
#else
  HashTable *props = zend_std_get_properties(object TSRMLS_CC);
#endif

#if PHP_VERSION_ID >= 80000
  zend_hash_update(props, zend_string_init("type", 4, 0), &self->type);
  Z_TRY_ADDREF_P(&self->type);
#else
  zend_hash_update(props, "type", sizeof("type"), &self->type, sizeof(zval), NULL);
  Z_ADDREF_P(&self->type);
#endif

  array_init(&values);
  php_driver_tuple_populate(self, &values);
#if PHP_VERSION_ID >= 80000
  zend_hash_update(props, zend_string_init("values", 6, 0), &values);
#else
  zend_hash_update(props, "values", sizeof("values"), values, sizeof(zval), NULL);
#endif

  return props;
}

#if PHP_VERSION_ID < 80000
static int
php_driver_tuple_compare(zval *obj1, zval *obj2 TSRMLS_DC)
#else
int
php_driver_tuple_compare(zval *obj1, zval *obj2)
#endif
{
  HashPosition pos1;
  HashPosition pos2;
  zval *current1;
  zval *current2;
  php_driver_tuple *tuple1;
  php_driver_tuple *tuple2;
  php_driver_type *type1;
  php_driver_type *type2;
  int result;

  if (Z_OBJCE_P(obj1) != Z_OBJCE_P(obj2))
    return 1; /* different classes */

  tuple1 = PHP_DRIVER_GET_TUPLE(obj1);
  tuple2 = PHP_DRIVER_GET_TUPLE(obj2);

  type1 = PHP_DRIVER_GET_TYPE(&tuple1->type);
  type2 = PHP_DRIVER_GET_TYPE(&tuple2->type);

#if PHP_VERSION_ID < 80000
  result = php_driver_type_compare(type1, type2 TSRMLS_CC);
#else
  result = php_driver_type_compare(type1, type2);
#endif
  if (result != 0) return result;

  if (zend_hash_num_elements(&tuple1->values) != zend_hash_num_elements(&tuple2->values)) {
    return zend_hash_num_elements(&tuple1->values) < zend_hash_num_elements(&tuple2->values) ? -1 : 1;
  }

  zend_hash_internal_pointer_reset_ex(&tuple1->values, &pos1);
  zend_hash_internal_pointer_reset_ex(&tuple2->values, &pos2);

  while ((current1 = zend_hash_get_current_data_ex(&tuple1->values, &pos1)) &&
         (current2 = zend_hash_get_current_data_ex(&tuple2->values, &pos2))) {
#if PHP_VERSION_ID < 80000
    result = php_driver_value_compare(current1,
                                         current2 TSRMLS_CC);
#else
    result = php_driver_value_compare(current1,
                                         current2);
#endif
    if (result != 0) return result;
    zend_hash_move_forward_ex(&tuple1->values, &pos1);
    zend_hash_move_forward_ex(&tuple2->values, &pos2);
  }

  return 0;
}

#if PHP_VERSION_ID < 80000
static unsigned
php_driver_tuple_hash_value(zval *obj TSRMLS_DC)
#else
unsigned
php_driver_tuple_hash_value(zval *obj)
#endif
{
  zval *current;
  unsigned hashv = 0;
  php_driver_tuple *self = PHP_DRIVER_GET_TUPLE(obj);

  if (!self->dirty) return self->hashv;

  ZEND_HASH_FOREACH_VAL(&self->values, current) {
    hashv = php_driver_combine_hash(hashv,
                                       php_driver_value_hash(current));
  } ZEND_HASH_FOREACH_END();

  self->hashv = hashv;
  self->dirty = 0;

  return hashv;
}

#if PHP_VERSION_ID >= 80000
static void
php_driver_tuple_free(zend_object *object)
#else
static void
php_driver_tuple_free(void *object TSRMLS_DC)
#endif
{
#if PHP_VERSION_ID >= 80000
  php_driver_tuple *self = (php_driver_tuple *) ((char *) (object) - XtOffsetOf(php_driver_tuple, std));
#else
  php_driver_tuple *self = (php_driver_tuple *) object;
#endif
  zend_hash_destroy(&self->values);
  zval_ptr_dtor(&self->type);
#if PHP_VERSION_ID >= 80000
  zend_object_std_dtor(&self->std);
#else
  zend_object_std_dtor(&self->zval TSRMLS_CC);
  efree(self);
#endif
}

#if PHP_VERSION_ID >= 80000
static zend_object*
php_driver_tuple_new(zend_class_entry *ce)
#else
static zend_object_value
php_driver_tuple_new(zend_class_entry *ce TSRMLS_DC)
#endif
{
#if PHP_VERSION_ID >= 80000
  php_driver_tuple *self = ecalloc(1, sizeof(php_driver_tuple) + zend_object_properties_size(ce));

  zend_hash_init(&self->values, 0, NULL, ZVAL_PTR_DTOR, 0);
  self->pos = HT_INVALID_IDX;
  self->dirty = 1;
  ZVAL_UNDEF(&self->type);

  zend_object_std_init(&self->std, ce);
  object_properties_init(&self->std, ce);
  self->std.handlers = &php_driver_tuple_handlers;

  return &self->std;
#else
  zend_object_value retval;
  php_driver_tuple *self;

  self = (php_driver_tuple *) ecalloc(1, sizeof(php_driver_tuple));

  zend_hash_init(&self->values, 0, NULL, ZVAL_PTR_DTOR, 0);
  self->pos = NULL;
  self->dirty = 1;
  ZVAL_UNDEF(&self->type);

  zend_object_std_init(&self->zval, ce TSRMLS_CC);
  object_properties_init(&self->zval, ce TSRMLS_CC);

  retval.handle = zend_objects_store_put(self,
                                         (zend_objects_store_dtor_t) zend_objects_destroy_object,
                                         php_driver_tuple_free, NULL TSRMLS_CC);
  retval.handlers = &php_driver_tuple_handlers;

  return retval;
#endif
}

void php_driver_define_Tuple(TSRMLS_D)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, PHP_DRIVER_NAMESPACE "\\Tuple", php_driver_tuple_methods);
  php_driver_tuple_ce = zend_register_internal_class(&ce TSRMLS_CC);
  zend_class_implements(php_driver_tuple_ce, 1, php_driver_value_ce);
  memcpy(&php_driver_tuple_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

#if PHP_VERSION_ID >= 80000
  php_driver_tuple_handlers.offset = XtOffsetOf(php_driver_tuple, std);
  php_driver_tuple_handlers.free_obj = php_driver_tuple_free;
  php_driver_tuple_handlers.get_properties = php_driver_tuple_properties;
  php_driver_tuple_handlers.get_gc = php_driver_tuple_gc;
  php_driver_tuple_handlers.compare = php_driver_tuple_compare;
  php_driver_tuple_handlers.clone_obj = NULL;
  zend_class_implements(php_driver_tuple_ce, 2, zend_ce_countable, zend_ce_iterator);
#else
  php_driver_tuple_handlers.get_properties = php_driver_tuple_properties;
  php_driver_tuple_handlers.compare_objects = php_driver_tuple_compare;
  php_driver_tuple_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 50400
  php_driver_tuple_handlers.get_gc = php_driver_tuple_gc;
#endif
  php_driver_tuple_handlers.hash_value = php_driver_tuple_hash_value;
  zend_class_implements(php_driver_tuple_ce TSRMLS_CC, 2, spl_ce_Countable, zend_ce_iterator);
#endif

  php_driver_tuple_ce->ce_flags |= ZEND_ACC_FINAL;
  php_driver_tuple_ce->create_object = php_driver_tuple_new;
}