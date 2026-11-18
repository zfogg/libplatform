#import <Foundation/Foundation.h>

#include <libplatform/compiler.h>
#include <libplatform/system.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

const char *platform_system_ca_certs_path(void) {
  static char path[PATH_MAX];
  NSURL *url = [[NSBundle mainBundle] URLForResource:@"ca-certificates" withExtension:@"pem"];
  if (!url || !url.path || url.path.length == 0 || url.path.length >= sizeof(path))
    return NULL;
  if (![url.path getFileSystemRepresentation:path maxLength:sizeof(path)])
    return NULL;
  return path;
}

int platform_load_system_ca_certs(char **pem_data_out, size_t *pem_size_out) {
  if (!pem_data_out || !pem_size_out)
    return -1;

  *pem_data_out = NULL;
  *pem_size_out = 0;
  NSURL *url = [[NSBundle mainBundle] URLForResource:@"ca-certificates" withExtension:@"pem"];
  NSData *encoded = url ? [NSData dataWithContentsOfURL:url] : nil;
  if (!encoded || encoded.length == 0)
    return -1;

  char *result = (char *)malloc(encoded.length + 1);
  if (!result)
    return -1;
  memcpy(result, encoded.bytes, encoded.length);
  result[encoded.length] = '\0';
  *pem_data_out = result;
  *pem_size_out = encoded.length;
  return 0;
}
