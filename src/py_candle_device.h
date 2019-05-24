#ifndef _py_candle_device_H_
#define _py_candle_device_H_

#include <Python.h>
#include <structmember.h>
#include "candle_api/candle.h"
#include "py_candle_channel.h"

#define CANDLE_MAX_CHANNELS 4
#define CANDLE_RX_THREAD_INTERVAL 10 // in ms

typedef void* HANDLE;

typedef struct py_candle_device {
  PyObject_HEAD

  // Candle device handle
  candle_handle _handle;

  // Open channels
  py_candle_channel* _channels[CANDLE_MAX_CHANNELS];

  // RX thread
  HANDLE _rx_thread;
  bool _rx_thread_stop_req;
} py_candle_device;

extern PyTypeObject py_candle_device_type;

// Called by the channel destructor
void py_candle_device_close_channel(py_candle_device* self, uint8_t ch);

#endif
