#include "py_candle_channel.h"
#include "py_candle_device.h"
#include "fifo.h"

void py_candle_channel_dealloc(py_candle_channel* self)
{
  // Stop channel on the device
  candle_channel_stop(self->_handle, self->_ch);

  // Unlink channel from interface
  py_candle_device_close_channel(self->_device, self->_ch);

  // Release interface
  Py_DECREF(self->_device);

  // Remove fifo
  fifo_delete(self->_fifo);

  // Free self
  Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject* py_candle_channel_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  py_candle_channel* self = (py_candle_channel*)type->tp_alloc(type, 0);

  if (!self)
    return NULL;

  if (!PyArg_ParseTuple(args, "KBO", &self->_handle, &self->_ch, &self->_device))
  {
    type->tp_free((PyObject*)self);
    return NULL;
  }

  // Initialize RX FIFO
  self->_fifo = fifo_create(sizeof(candle_frame_t), CANDLE_RX_FIFO_SIZE);

  // Prevent device from deallocation
  Py_INCREF(self->_device);

  return (PyObject*)self;
}

int py_candle_channel_init(py_candle_channel *self, PyObject *args, PyObject *kwds)
{
  return 0;
}

PyMemberDef py_candle_channel_members[] = {
  {NULL}  /* Sentinel */
};

PyObject* py_candle_channel_start(py_candle_channel* self, PyObject* args)
{
  uint32_t flags = CANDLE_MODE_NORMAL;

  if (!PyArg_ParseTuple(args, "|k", &flags))
    return Py_BuildValue("O", Py_False);

  if (!candle_channel_start(self->_handle, self->_ch, flags))
    return Py_BuildValue("O", Py_False);

  return Py_BuildValue("O", Py_True);
}

PyObject* py_candle_channel_stop(py_candle_channel* self, PyObject* Py_UNUSED(ignored))
{
  if (!candle_channel_stop(self->_handle, self->_ch))
    return Py_BuildValue("O", Py_False);

  return Py_BuildValue("O", Py_True);
}

PyObject* py_candle_channel_set_bitrate(py_candle_channel* self, PyObject* args)
{
  uint32_t bitrate;

  if (!PyArg_ParseTuple(args, "k", &bitrate))
    return Py_BuildValue("O", Py_False);
  
  if (!candle_channel_set_bitrate(self->_handle, self->_ch, bitrate))
    return Py_BuildValue("O", Py_False);
  
  return Py_BuildValue("O", Py_True);
}

PyObject* py_candle_channel_set_timings(py_candle_channel* self, PyObject* args, PyObject* kwds)
{
  candle_bittiming_t timing;

  static char* kwlist[] = {"prop_seg", "phase_seg1", "phase_seg2", "sjw", "brp", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "kkkkk", kwlist,
    &timing.prop_seg, &timing.phase_seg1, &timing.phase_seg2, &timing.sjw, &timing.brp))
    return Py_BuildValue("O", Py_False);

  if (!candle_channel_set_timing(self->_handle, self->_ch, &timing))
    return Py_BuildValue("O", Py_False);

  return Py_BuildValue("O", Py_True);
}

PyObject* py_candle_channel_write(py_candle_channel* self, PyObject* args)
{
  candle_frame_t frame;
  const uint8_t* buf;
  size_t len;
  bool res;

  if (!PyArg_ParseTuple(args, "ky#", &frame.can_id, &buf, &len))
    return Py_BuildValue("O", Py_False);

  memcpy(frame.data, buf, len);
  frame.can_dlc = (uint8_t)len;

  Py_BEGIN_ALLOW_THREADS
  res = candle_frame_send(self->_handle, self->_ch, &frame);
  Py_END_ALLOW_THREADS

  if (!res)
    return Py_BuildValue("O", Py_False);

  return Py_BuildValue("O", Py_True);
}

PyObject* py_candle_channel_read(py_candle_channel* self, PyObject* args)
{
  uint32_t timeout_ms = 0;
  bool res;

  if (!PyArg_ParseTuple(args, "|k", &timeout_ms))
    return NULL;

  candle_frame_t frame;

  Py_BEGIN_ALLOW_THREADS
  // Read from FIFO which is filled by the RX thread in py_candle_device
  res = fifo_get(self->_fifo, &frame, timeout_ms);
  Py_END_ALLOW_THREADS

  if (!res)
    return PyErr_Format(PyExc_TimeoutError, "CAN read timeout.");

  return Py_BuildValue("kky#Ok",
    candle_frame_type(&frame),
    candle_frame_id(&frame),
    frame.data,
    frame.can_dlc,
    candle_frame_is_extended_id(&frame) ? Py_True : Py_False,
    candle_frame_timestamp_us(&frame)
  );
}


PyMethodDef py_candle_channel_methods[] = {
  {"start", (PyCFunction)py_candle_channel_start, METH_VARARGS, "Starts CAN channel"},
  {"stop", (PyCFunction)py_candle_channel_stop, METH_NOARGS, "Stops CAN channel"},
  {"set_bitrate", (PyCFunction)py_candle_channel_set_bitrate, METH_VARARGS, "Sets CAN bitrate"},
  {"set_timings", (PyCFunction)py_candle_channel_set_timings, METH_VARARGS | METH_KEYWORDS, "Sets CAN timings"},
  {"write", (PyCFunction)py_candle_channel_write, METH_VARARGS, "Send data to CAN"},
  {"read", (PyCFunction)py_candle_channel_read, METH_VARARGS, "Read data from CAN"},
  {NULL}  /* Sentinel */
};

PyTypeObject py_candle_channel_type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "candle_driver.channel",
  .tp_doc = "Candle CAN driver channel",
  .tp_basicsize = sizeof(py_candle_channel),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_new = py_candle_channel_new,
  .tp_init = (initproc)py_candle_channel_init,
  .tp_dealloc = (destructor)py_candle_channel_dealloc,
  .tp_members = py_candle_channel_members,
  .tp_methods = py_candle_channel_methods,
};
