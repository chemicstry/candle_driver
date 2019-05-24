#ifndef _FIFO_H_
#define _FIFO_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <windows.h>
#include <synchapi.h>

typedef struct fifo_t {
  size_t element_size;
  size_t element_count;
  size_t stored_count;

  void* buf;

  void* write_pointer;
  void* read_pointer;
  void* buf_end;

  // Thread synchronization
  CONDITION_VARIABLE buf_not_empty;
  CONDITION_VARIABLE buf_not_full;
  SRWLOCK lock;
} fifo_t;

fifo_t* fifo_create(size_t element_size, size_t element_count);
bool fifo_delete(fifo_t* fifo);

bool fifo_is_full(fifo_t* fifo);

// Returns false if fifo is full
bool fifo_add(fifo_t* fifo, const void* item, uint32_t timeout);
// Removes oldest item if fifo is full
bool fifo_add_force(fifo_t* fifo, const void* item);

bool fifo_get(fifo_t* fifo, void* item, uint32_t timeout);

// Optimised unsafe funtions
void* fifo_add_acquire(fifo_t* fifo);
void* fifo_add_acquire_force(fifo_t* fifo);
bool fifo_add_commit(fifo_t* fifo, void* item, uint32_t timeout);
bool fifo_add_commit_force(fifo_t* fifo, void* item);

#endif
