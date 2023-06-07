#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"

#define RANDNAME_PATTERN "/keebie-XXXXXX"

struct LocaleMap {
  const char* code;
  const char* name;
};

static const struct LocaleMap locale_map[] = {
  { "en-US", "English (US)" },
  { "ja-JP", "Japanese" }
};
static const size_t locale_map_size = sizeof (locale_map) / sizeof (locale_map[0]);

const char* get_locale_name(const char* code) {
  for (size_t i = 0; i < locale_map_size; i++) {
    const struct LocaleMap* entry = &locale_map[i];
    if (strcmp(entry->code, code) == 0) {
      return entry->name;
    }
  }
  return NULL;
}

static void randname(char* buf) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	long r = ts.tv_nsec;
	for (int i = 0; i < 6; ++i) {
		buf[i] = 'A'+(r&15)+(r&16)*2;
		r >>= 5;
	}
}

static int excl_shm_open(char* name) {
	int retries = 100;
	do {
		randname(name + strlen(RANDNAME_PATTERN) - 6);

		--retries;
		int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (fd >= 0) {
			return fd;
		}
	} while (retries > 0 && errno == EEXIST);
	return -1;
}

bool allocate_shm_file_pair(size_t size, int* rw_fd_ptr, int* ro_fd_ptr) {
  char name[] = RANDNAME_PATTERN;
  int rw_fd = excl_shm_open(name);
  if (rw_fd < 0) {
    return false;
  }

  int ro_fd = shm_open(name, O_RDONLY, 0);
  if (ro_fd < 0) {
     shm_unlink(name);
     close(rw_fd);
     return false;
  }

  shm_unlink(name);

  if (fchmod(rw_fd, 0) != 0) {
    close(rw_fd);
    close(ro_fd);
    return false;
  }

  int ret;
  do {
    ret = ftruncate(rw_fd, size);
  } while (ret < 0 && errno == EINTR);
  if (ret < 0) {
    close(rw_fd);
    close(ro_fd);
    return false;
  }

  *rw_fd_ptr = rw_fd;
  *ro_fd_ptr = ro_fd;
  return true;
}

long get_time_ms() {
  struct timespec curr;
  clock_gettime(CLOCK_REALTIME, &curr);
  return curr.tv_sec * 1000 + curr.tv_nsec / 1000000;
}