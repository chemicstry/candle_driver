#ifndef _PY_CANDLE_CHANNEL_H_
#define _PY_CANDLE_CHANNEL_H_

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include "candle_api/candle.h"
#include "fifo.h"

#define CANDLE_RX_FIFO_SIZE 20

struct py_candle_device;

typedef struct py_candle_channel {
  PyObject_HEAD

  // Candle device
  candle_handle _handle;
  uint8_t _ch;

  // Reverse reference (to unlink open channels)
  struct py_candle_device* _device;

  // RX FIFO
  struct fifo_t* _fifo;
} py_candle_channel;

extern PyTypeObject py_candle_channel_type;

#endif
