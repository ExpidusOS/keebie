#pragma once

#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

bool allocate_shm_file_pair(size_t size, int* rw_fd_ptr, int* ro_fd_ptr);
long get_time_ms();

#if defined(__cplusplus)
}
#endif