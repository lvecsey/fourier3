
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int writefile(int fd, void *buf, size_t len) {

  size_t remaining;
  unsigned char *adv_p;
  ssize_t bytes_written;

  size_t chunk_sz = 4096;

  size_t amt;
  
  adv_p = buf;
  
  remaining = len;
  while (remaining > 0) {

    amt = chunk_sz;
    if (remaining < amt) {
      amt = remaining;
    }

    bytes_written = write(fd, adv_p + len - remaining, amt);
    if (!bytes_written) break;
    if (bytes_written < 0) {
      return -1;
    }
    remaining -= amt;

    
  }

  return len;
  
}
