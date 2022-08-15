
set(RFC6234_SHA_SOURCES
  hkdf.c
  hmac.c
  sha1.c
  sha224-256.c
  sha384-512.c
  usha.c
)

add_library(cre_rfc6234-shas OBJECT ${RFC6234_SHA_SOURCES})
