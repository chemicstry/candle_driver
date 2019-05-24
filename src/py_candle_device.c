#include "py_candle_device.h"
#include "py_candle_channel.h"
#include "fifo.h"
#include <string.h>

// RX data processing thread is required for multiple reasons:
// a) winusb api returns frames out of order if multiple frames are
//    received between candle_frame_read calls. Some protocols require
//    frames to be in order (UAVCAN) so we need a dedicated thread that
//    reads USB as fast as possible.
// b) candle_frame_read has no channel parameter and returns frames for
//    all channels, so we have to do sorting here and push frames into
//    dedicated channel FIFOs
DWORD WINAPI py_candle_device_rx_thread(LPVOID lpParam) 
{
  py_candle_device* device = (py_candle_device*)lpParam;

  while (!device->_rx_thread_stop_req) {
    candle_frame_t frame;

    // Read with timeout, but keep it small to react to thread stop requests in time
    bool res = candle_frame_read(device->_handle, &frame, CANDLE_RX_THREAD_INTERVAL);

    // if res is false its probably timeout
    if (res)
    {
      uint8_t ch = frame.channel;

      // Sanity check and verify that channel is open
      if (ch < CANDLE_MAX_CHANNELS && device->_channels[ch])
      {
        fifo_t* fifo = device->_channels[ch]->_fifo;

        // If fifo is full, oldest frames will be pushed out
        fifo_add_force(fifo, &frame);
      }
    }
  }

  return 0;
}

void py_candle_device_start_rx_thread(py_candle_device* self)
{
  if (!self->_rx_thread) {
    self->_rx_thread_stop_req = false;
    DWORD id;
    self->_rx_thread = CreateThread(NULL, 0, py_candle_device_rx_thread, (PVOID)self, 0, &id);
  }
}

void py_candle_device_stop_rx_thread(py_candle_device* self)
{
  if (self->_rx_thread) {
    // Indicate stop request by variable and wait for the thread to terminate itself
    self->_rx_thread_stop_req = true;
    WaitForSingleObject(self->_rx_thread, INFINITE);
    self->_rx_thread = NULL;
  }
}

// called by the channel destructor
void py_candle_device_close_channel(py_candle_device* self, uint8_t ch)
{
  // This prevents RX thread from writing non existing FIFO
  self->_channels[ch] = NULL;
}

void py_candle_device_dealloc(py_candle_device* self)
{
  py_candle_device_stop_rx_thread(self);
  candle_dev_close(self->_handle);
  candle_dev_free(self->_handle);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject* py_candle_device_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  py_candle_device* self = (py_candle_device*)type->tp_alloc(type, 0);

  if (!self)
    return NULL;

  // device handle is passed as an argument
  if (!PyArg_ParseTuple(args, "K", &self->_handle))
  {
    type->tp_free((PyObject*)self);
    return NULL;
  }

  self->_rx_thread = NULL;
  self->_rx_thread_stop_req = false;
  memset(self->_channels, 0, sizeof(self->_channels));

  return (PyObject*)self;
}

int py_candle_device_init(py_candle_device *self, PyObject *args, PyObject *kwds)
{
  return 0;
}

PyMemberDef py_candle_device_members[] = {
  {NULL}  /* Sentinel */
};

PyObject* py_candle_device_state(py_candle_device* self, PyObject* Py_UNUSED(ignored))
{
  candle_devstate_t state;

  if (!candle_dev_get_state(self->_handle, &state))
    return PyErr_Format(PyExc_SystemError, "Unable to get device state");

  return Py_BuildValue("B", state);
}

PyObject* py_candle_device_open(py_candle_device* self, PyObject* Py_UNUSED(ignored))
{
  if (!candle_dev_open(self->_handle))
    return Py_BuildValue("O", Py_False);

  py_candle_device_start_rx_thread(self);

  return Py_BuildValue("O", Py_True);
}

PyObject* py_candle_device_close(py_candle_device* self, PyObject* Py_UNUSED(ignored))
{
  py_candle_device_stop_rx_thread(self);

  if (!candle_dev_close(self->_handle))
    return Py_BuildValue("O", Py_False);

  return Py_BuildValue("O", Py_True);
}

PyObject* py_candle_device_path(py_candle_device *self, PyObject *Py_UNUSED(ignored))
{
  wchar_t* path = candle_dev_get_path(self->_handle);
  return Py_BuildValue("U", path);
}

// Returns a friendly device name. I.e canable_35c414bb
PyObject* py_candle_device_name(py_candle_device *self, PyObject *Py_UNUSED(ignored))
{
  char path[256];
  char name[50];
  char* part;

  // This is actually char* but is returned as wchar_t ???
  char* path_o = (char*)candle_dev_get_path(self->_handle);

  // Copy string because strtok is destructive
  strcpy(path, path_o);

  // Path: \\\\?\\usb#vid_1d50&pid_606f&mi_00#6&35c414bb&0&0000#{c15b4308-04d3-11e6-b3ea-6057189e6443}
  // Extract 3rd part "6&35c414bb&0&0000" which is windows generated unique string
  part = strtok(path, "#"); // "\\\\?\\usb"
  part = strtok(NULL, "#"); // "vid_1d50&pid_606f&mi_00"
  part = strtok(NULL, "#"); // "6&35c414bb&0&0000"

  // Extract "35c414bb" which is the only non null part of the string
  part = strtok(part, "&");
  part = strtok(NULL, "&");

  // make name
  sprintf(name, "candle_%s", part);

  return Py_BuildValue("s", name);
}

PyObject* py_candle_device_error(py_candle_device *self, PyObject *Py_UNUSED(ignored))
{
  candle_err_t error = candle_dev_last_error(self->_handle);
  return Py_BuildValue("B", error);
}

PyObject* py_candle_device_channel_count(py_candle_device* self, PyObject* Py_UNUSED(ignored))
{
  uint8_t num_channels;

  if (!candle_channel_count(self->_handle, &num_channels))
    return PyErr_Format(PyExc_SystemError, "Unable to get device channel count");
  
  return Py_BuildValue("B", num_channels);
}

PyObject* py_candle_device_channel(py_candle_device* self, PyObject* args)
{
  uint8_t ch;

  if (!PyArg_ParseTuple(args, "B", &ch))
    return NULL;

  uint8_t num_channels;

  if (!candle_channel_count(self->_handle, &num_channels))
    return PyErr_Format(PyExc_SystemError, "Unable to get device channel count");

  if (ch >= num_channels)
    return PyErr_Format(PyExc_ValueError, "Channel number out of range");

  if (self->_channels[ch] == NULL) {
    self->_channels[ch] = (py_candle_channel*)PyObject_CallFunction((PyObject *)&py_candle_channel_type, "KBO", self->_handle, ch, self);
    if (self->_channels[ch] == NULL)
      return PyErr_NoMemory();
  }

  return (PyObject*)self->_channels[ch];
}

PyObject* py_candle_device_timestamp(py_candle_device* self, PyObject* Py_UNUSED(ignored))
{
  uint32_t timestamp;

  if (!candle_dev_get_timestamp_us(self->_handle, &timestamp))
    return PyErr_Format(PyExc_SystemError, "Unable to get device timestamp");
  
  return Py_BuildValue("k", timestamp);
}

PyMethodDef py_candle_device_methods[] = {
  {"state", (PyCFunction)py_candle_device_state, METH_NOARGS, "Returns candle device state"},
  {"open", (PyCFunction)py_candle_device_open, METH_NOARGS, "Opens device"},
  {"close", (PyCFunction)py_candle_device_close, METH_NOARGS, "Closes device"},
  {"path", (PyCFunction)py_candle_device_path, METH_NOARGS, "Returns path of the device"},
  {"name", (PyCFunction)py_candle_device_name, METH_NOARGS, "Returns friendly generated name"},
  {"error", (PyCFunction)py_candle_device_error, METH_NOARGS, "Returns last device error"},
  {"channel_count", (PyCFunction)py_candle_device_channel_count, METH_NOARGS, "Returns numbers of available channels"},
  {"channel", (PyCFunction)py_candle_device_channel, METH_VARARGS, "Returns specified device channel"},
  {"timestamp", (PyCFunction)py_candle_device_timestamp, METH_NOARGS, "Returns current device timestamp in us"},
  {NULL}  /* Sentinel */
};

PyTypeObject py_candle_device_type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "candle_driver.interface",
  .tp_doc = "Candle CAN driver interface",
  .tp_basicsize = sizeof(py_candle_device),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_new = py_candle_device_new,
  .tp_init = (initproc)py_candle_device_init,
  .tp_dealloc = (destructor)py_candle_device_dealloc,
  .tp_members = py_candle_device_members,
  .tp_methods = py_candle_device_methods,
};
