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

#include <ext/standard/php_filestat.h>

zend_class_entry *php_driver_ssl_builder_ce = NULL;

static int
file_get_contents(char *path, char **output, size_t *len)
{
  php_stream *stream = php_stream_open_wrapper(path, "rb",
                         USE_PATH|REPORT_ERRORS, NULL);

  if (!stream) {
    zend_throw_exception_ex(php_driver_runtime_exception_ce, 0,
      "The path '%s' doesn't exist or is not readable", path);
    return 0;
  }

  zend_string *str = php_stream_copy_to_mem(stream, PHP_STREAM_COPY_ALL, 0);
  if (str) {
    *output = estrndup(ZSTR_VAL(str), ZSTR_LEN(str));
    *len = ZSTR_LEN(str);
    zend_string_release(str);
  } else {
    php_stream_close(stream);
    return 0;
  }

  php_stream_close(stream);
  return 1;
}

PHP_METHOD(SSLOptionsBuilder, build)
{
  php_driver_ssl *ssl = NULL;
  size_t len;
  char *contents;
  CassError rc;

  php_driver_ssl_builder *builder = PHP_DRIVER_GET_SSL_BUILDER(getThis());

  object_init_ex(return_value, php_driver_ssl_ce);
  ssl = PHP_DRIVER_GET_SSL(return_value);

  cass_ssl_set_verify_flags(ssl->ssl, builder->flags);

  if (builder->trusted_certs) {
    int   i;
    char *path;

    for (i = 0; i < builder->trusted_certs_cnt; i++) {
      path = builder->trusted_certs[i];

      if (!file_get_contents(path, &contents, &len))
        return;

      rc = cass_ssl_add_trusted_cert_n(ssl->ssl, contents, len);
      efree(contents);
      ASSERT_SUCCESS(rc);
    }
  }

  if (builder->client_cert) {
    if (!file_get_contents(builder->client_cert, &contents, &len))
      return;

    rc = cass_ssl_set_cert_n(ssl->ssl, contents, len);
    efree(contents);
    ASSERT_SUCCESS(rc);
  }

  if (builder->private_key) {
    if (!file_get_contents(builder->private_key, &contents, &len))
      return;

    rc = cass_ssl_set_private_key(ssl->ssl, contents, builder->passphrase);
    efree(contents);
    ASSERT_SUCCESS(rc);
  }
}

PHP_METHOD(SSLOptionsBuilder, withTrustedCerts)
{
  zval *args = NULL;
  int argc = 0, i;
  php_driver_ssl_builder *builder = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "+ ", &args, &argc) == FAILURE) {
    return;
  }

  for (i = 0; i < argc; i++) {
    zval *path = &args[i];
    zval readable;

    if (Z_TYPE_P(path) != IS_STRING) {
      throw_invalid_argument(path, "path", "a path to a trusted cert file");
      return;
    }

    php_stat(Z_STR_P(path), FS_IS_R, &readable);

    if (Z_TYPE(readable) == IS_FALSE) {
      zend_throw_exception_ex(php_driver_invalid_argument_exception_ce, 0,
        "The path '%s' doesn't exist or is not readable", Z_STRVAL_P(path));
      return;
    }
  }

  builder = PHP_DRIVER_GET_SSL_BUILDER(getThis());

  if (builder->trusted_certs) {
    for (i = 0; i < builder->trusted_certs_cnt; i++) {
      efree(builder->trusted_certs[i]);
    }

    efree(builder->trusted_certs);
  }

  builder->trusted_certs_cnt = argc;
  builder->trusted_certs     = ecalloc(argc, sizeof(char *));

  for (i = 0; i < argc; i++) {
    zval *path = &args[i];

    builder->trusted_certs[i] = estrndup(Z_STRVAL_P(path), Z_STRLEN_P(path));
  }

  RETURN_ZVAL(getThis(), 1, 0);
}

PHP_METHOD(SSLOptionsBuilder, withVerifyFlags)
{
  zend_long flags;
  php_driver_ssl_builder *builder = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &flags) == FAILURE) {
    return;
  }

  builder = PHP_DRIVER_GET_SSL_BUILDER(getThis());

  builder->flags = (int) flags;

  RETURN_ZVAL(getThis(), 1, 0);
}

PHP_METHOD(SSLOptionsBuilder, withClientCert)
{
  char *client_cert;
  size_t client_cert_len;
  zval readable;
  php_driver_ssl_builder *builder = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &client_cert, &client_cert_len) == FAILURE) {
    return;
  }

  php_stat(zend_string_init(client_cert, client_cert_len, 0), FS_IS_R, &readable);

  if (Z_TYPE(readable) == IS_FALSE) {
    zend_throw_exception_ex(php_driver_invalid_argument_exception_ce, 0,
      "The path '%s' doesn't exist or is not readable", client_cert);
    return;
  }

  builder = PHP_DRIVER_GET_SSL_BUILDER(getThis());

  if (builder->client_cert)
    efree(builder->client_cert);

  builder->client_cert = estrndup(client_cert, client_cert_len);

  RETURN_ZVAL(getThis(), 1, 0);
}

PHP_METHOD(SSLOptionsBuilder, withPrivateKey)
{
  char *private_key;
  char *passphrase = NULL;
  size_t private_key_len, passphrase_len;
  zval readable;
  php_driver_ssl_builder *builder = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|s", &private_key, &private_key_len, &passphrase, &passphrase_len) == FAILURE) {
    return;
  }

  php_stat(zend_string_init(private_key, private_key_len, 0), FS_IS_R, &readable);

  if (Z_TYPE(readable) == IS_FALSE) {
    zend_throw_exception_ex(php_driver_invalid_argument_exception_ce, 0,
      "The path '%s' doesn't exist or is not readable", private_key);
    return;
  }

  builder = PHP_DRIVER_GET_SSL_BUILDER(getThis());

  if (builder->private_key)
    efree(builder->private_key);

  builder->private_key = estrndup(private_key, private_key_len);

  if (builder->passphrase) {
    efree(builder->passphrase);
    builder->passphrase = NULL;
  }

  if (passphrase)
    builder->passphrase = estrndup(passphrase, passphrase_len);

  RETURN_ZVAL(getThis(), 1, 0);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_none, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_path, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_flags, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, path)
  ZEND_ARG_INFO(0, passphrase)
ZEND_END_ARG_INFO()

static zend_function_entry php_driver_ssl_builder_methods[] = {
  PHP_ME(SSLOptionsBuilder, build, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(SSLOptionsBuilder, withTrustedCerts, arginfo_path,
    ZEND_ACC_PUBLIC)
  PHP_ME(SSLOptionsBuilder, withVerifyFlags, arginfo_flags,
    ZEND_ACC_PUBLIC)
  PHP_ME(SSLOptionsBuilder, withClientCert, arginfo_path,
    ZEND_ACC_PUBLIC)
  PHP_ME(SSLOptionsBuilder, withPrivateKey, arginfo_key,
    ZEND_ACC_PUBLIC)
  PHP_FE_END
};

static zend_object_handlers php_driver_ssl_builder_handlers;

#if PHP_VERSION_ID >= 80000
static HashTable *
php_driver_ssl_builder_properties(zend_object *object)
{
  HashTable *props = zend_std_get_properties(object);
  return props;
}
#else
static HashTable *
php_driver_ssl_builder_properties(zval *object TSRMLS_CC)
{
  HashTable *props = zend_std_get_properties(object TSRMLS_CC);
  return props;
}
#endif

#if PHP_VERSION_ID < 80000
static int
php_driver_ssl_builder_compare(zval *obj1, zval *obj2 TSRMLS_DC)
#else
int
php_driver_ssl_builder_compare(zval *obj1, zval *obj2)
#endif
{
  if (Z_OBJCE_P(obj1) != Z_OBJCE_P(obj2))
    return 1; /* different classes */

  return Z_OBJ_HANDLE_P(obj1) != Z_OBJ_HANDLE_P(obj2);
}

#if PHP_VERSION_ID >= 80000
static void
php_driver_ssl_builder_free(zend_object *object)
{
  php_driver_ssl_builder *self = (php_driver_ssl_builder *) ((char *) (object) - XtOffsetOf(php_driver_ssl_builder, std));
  if (self->trusted_certs) {
    int i;

    for (i = 0; i < self->trusted_certs_cnt; i++)
      efree(self->trusted_certs[i]);

    efree(self->trusted_certs);
  }

  if (self->client_cert)
    efree(self->client_cert);

  if (self->private_key)
    efree(self->private_key);

  if (self->passphrase)
    efree(self->passphrase);

  zend_object_std_dtor(&self->std);
}
#else
static void
php_driver_ssl_builder_free(void *object TSRMLS_DC)
{
  php_driver_ssl_builder *self = (php_driver_ssl_builder *) object;
  if (self->trusted_certs) {
    int i;

    for (i = 0; i < self->trusted_certs_cnt; i++)
      efree(self->trusted_certs[i]);

    efree(self->trusted_certs);
  }

  if (self->client_cert)
    efree(self->client_cert);

  if (self->private_key)
    efree(self->private_key);

  if (self->passphrase)
    efree(self->passphrase);

  zend_object_std_dtor(&self->zval TSRMLS_CC);
  efree(self);
}
#endif

#if PHP_VERSION_ID >= 80000
static zend_object*
php_driver_ssl_builder_new(zend_class_entry *ce)
{
  php_driver_ssl_builder *self = ecalloc(1, sizeof(php_driver_ssl_builder) + zend_object_properties_size(ce));

  self->flags             = 0;
  self->trusted_certs     = NULL;
  self->trusted_certs_cnt = 0;
  self->client_cert       = NULL;
  self->private_key       = NULL;
  self->passphrase        = NULL;

  zend_object_std_init(&self->std, ce);
  object_properties_init(&self->std, ce);
  self->std.handlers = &php_driver_ssl_builder_handlers;

  return &self->std;
}
#else
static zend_object_value
php_driver_ssl_builder_new(zend_class_entry *ce TSRMLS_DC)
{
  zend_object_value retval;
  php_driver_ssl_builder *self;

  self = (php_driver_ssl_builder *) ecalloc(1, sizeof(php_driver_ssl_builder));

  self->flags             = 0;
  self->trusted_certs     = NULL;
  self->trusted_certs_cnt = 0;
  self->client_cert       = NULL;
  self->private_key       = NULL;
  self->passphrase        = NULL;

  zend_object_std_init(&self->zval, ce TSRMLS_CC);
  object_properties_init(&self->zval, ce TSRMLS_CC);

  retval.handle = zend_objects_store_put(self,
                                         (zend_objects_store_dtor_t) zend_objects_destroy_object,
                                         php_driver_ssl_builder_free, NULL TSRMLS_CC);
  retval.handlers = &php_driver_ssl_builder_handlers;

  return retval;
}
#endif

void php_driver_define_SSLOptionsBuilder(TSRMLS_D)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, PHP_DRIVER_NAMESPACE "\\SSLOptions\\Builder", php_driver_ssl_builder_methods);
  php_driver_ssl_builder_ce = zend_register_internal_class(&ce TSRMLS_CC);
  php_driver_ssl_builder_ce->ce_flags     |= ZEND_ACC_FINAL;
  php_driver_ssl_builder_ce->create_object = php_driver_ssl_builder_new;

  memcpy(&php_driver_ssl_builder_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

#if PHP_VERSION_ID >= 80000
  php_driver_ssl_builder_handlers.offset = XtOffsetOf(php_driver_ssl_builder, std);
  php_driver_ssl_builder_handlers.free_obj = php_driver_ssl_builder_free;
  php_driver_ssl_builder_handlers.get_properties = php_driver_ssl_builder_properties;
  php_driver_ssl_builder_handlers.compare = php_driver_ssl_builder_compare;
  php_driver_ssl_builder_handlers.clone_obj = NULL;
#else
  php_driver_ssl_builder_handlers.get_properties = php_driver_ssl_builder_properties;
  php_driver_ssl_builder_handlers.compare_objects = php_driver_ssl_builder_compare;
  php_driver_ssl_builder_handlers.clone_obj = NULL;
#endif
}