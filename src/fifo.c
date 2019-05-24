#include "fifo.h"
#include <stdlib.h>
#include <string.h>

fifo_t* fifo_create(size_t element_size, size_t element_count)
{
  if (!element_count || !element_size)
    return NULL;

  fifo_t* fifo = malloc(sizeof(fifo_t));
  fifo->buf = malloc(element_size*element_count);
  fifo->read_pointer = fifo->buf;
  fifo->write_pointer = fifo->buf;
  fifo->buf_end = (uint8_t*)fifo->buf + element_size*element_count;
  fifo->element_size = element_size;
  fifo->element_count = element_count;
  fifo->stored_count = 0;

  InitializeConditionVariable(&fifo->buf_not_empty);
  InitializeConditionVariable(&fifo->buf_not_full);
  InitializeSRWLock(&fifo->lock);

  return fifo;
}

bool fifo_delete(fifo_t* fifo)
{
  if (!fifo)
    return false;

  WakeAllConditionVariable(&fifo->buf_not_empty);
  WakeAllConditionVariable(&fifo->buf_not_full);

  free(fifo->buf);
  free(fifo);

  return true;
}

void fifo_inc_write_pointer(fifo_t* fifo)
{
  fifo->write_pointer = (uint8_t*)fifo->write_pointer + fifo->element_size;
  if (fifo->write_pointer >= fifo->buf_end)
    fifo->write_pointer = fifo->buf;

  fifo->stored_count++;
}

void fifo_inc_read_pointer(fifo_t* fifo)
{
  fifo->read_pointer = (uint8_t*)fifo->read_pointer + fifo->element_size;
  if (fifo->read_pointer >= fifo->buf_end)
    fifo->read_pointer = fifo->buf;

  fifo->stored_count--;
}

bool fifo_is_full(fifo_t* fifo)
{
  return fifo->stored_count >= fifo->element_count;
}

bool fifo_add(fifo_t* fifo, const void* item, uint32_t timeout)
{
  AcquireSRWLockExclusive(&fifo->lock);

  if (fifo_is_full(fifo)) {
    // Wait for not full condition
    if (!SleepConditionVariableSRW(&fifo->buf_not_full, &fifo->lock, timeout, 0)) {
      // Return if timeouted
      ReleaseSRWLockExclusive(&fifo->lock);
      return false;
    }
  }

  memcpy(fifo->write_pointer, item, fifo->element_size);
  fifo_inc_write_pointer(fifo);

  ReleaseSRWLockExclusive(&fifo->lock);

  // Wake any waiting read
  WakeConditionVariable(&fifo->buf_not_empty);

  return true;
}

bool fifo_add_force(fifo_t* fifo, const void* item)
{
  AcquireSRWLockExclusive(&fifo->lock);

  if (fifo_is_full(fifo))
    fifo_inc_read_pointer(fifo);

  memcpy(fifo->write_pointer, item, fifo->element_size);
  fifo_inc_write_pointer(fifo);

  ReleaseSRWLockExclusive(&fifo->lock);

  // Wake any waiting read
  WakeConditionVariable(&fifo->buf_not_empty);

  return true;
}

bool fifo_get(fifo_t* fifo, void* item, uint32_t timeout)
{
  AcquireSRWLockExclusive(&fifo->lock);

  if (!fifo->stored_count) {
    // Wait for not empty condition
    if (!SleepConditionVariableSRW(&fifo->buf_not_empty, &fifo->lock, timeout, 0)) {
      // Return if timeouted
      ReleaseSRWLockExclusive(&fifo->lock);
      return false;
    }
  }

  memcpy(item, fifo->read_pointer, fifo->element_size);
  fifo_inc_read_pointer(fifo);

  ReleaseSRWLockExclusive(&fifo->lock);

  // Wake any waiting write
  WakeConditionVariable(&fifo->buf_not_full);

  return true;
}

// Optimised unsafe funtions
void* fifo_add_acquire(fifo_t* fifo)
{
  if (fifo_is_full(fifo))
    return NULL;
  else
    return fifo->write_pointer;
}

void* fifo_add_acquire_force(fifo_t* fifo)
{
  return fifo->write_pointer;
}

bool fifo_add_commit(fifo_t* fifo, void* item, uint32_t timeout)
{
  AcquireSRWLockExclusive(&fifo->lock);

  if (item != fifo->write_pointer) {
    // Write happened between acquire and commit
    ReleaseSRWLockExclusive(&fifo->lock);
    return false;
  }

  if (fifo_is_full(fifo)) {
    // Wait for not full condition
    if (!SleepConditionVariableSRW(&fifo->buf_not_full, &fifo->lock, timeout, 0)) {
      // Return if timeouted
      ReleaseSRWLockExclusive(&fifo->lock);
      return false;
    }
  }

  fifo_inc_write_pointer(fifo);

  ReleaseSRWLockExclusive(&fifo->lock);

  // Wake any waiting read
  WakeConditionVariable(&fifo->buf_not_empty);

  return true;
}

bool fifo_add_commit_force(fifo_t* fifo, void* item)
{
  AcquireSRWLockExclusive(&fifo->lock);

  if (item != fifo->write_pointer) {
    // Write happened between acquire and commit
    ReleaseSRWLockExclusive(&fifo->lock);
    return false;
  }

  if (fifo_is_full(fifo))
    fifo_inc_read_pointer(fifo);

  fifo_inc_write_pointer(fifo);

  ReleaseSRWLockExclusive(&fifo->lock);

  // Wake any waiting read
  WakeConditionVariable(&fifo->buf_not_empty);

  return true;
}