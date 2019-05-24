#include <Python.h>
#include "py_candle_channel.h"
#include "py_candle_device.h"
#include "candle_api/candle.h"

static PyObject* py_candle_driver_list_devices(PyObject* self, PyObject* args)
{
  candle_list_handle clist;
  uint8_t num_devices;
  candle_handle dev;
  PyObject* python_list;

  if (candle_list_scan(&clist)) {
    if (candle_list_length(clist, &num_devices)) {
      python_list = PyList_New(num_devices);

      for (uint8_t i = 0; i < num_devices; ++i) {
        if (candle_dev_get(clist, i, &dev)) {
          py_candle_device* device = (py_candle_device*)PyObject_CallFunction((PyObject *)&py_candle_device_type, "(K)", dev);
          PyList_SetItem(python_list, i, (PyObject*)device);
        }
      }
    }
    candle_list_free(clist);
  }

  if (python_list)
    return python_list;
  else
    return Py_BuildValue("[]");
}

static PyMethodDef module_methods[] = {
  {"list_devices", py_candle_driver_list_devices, METH_VARARGS, "Lists all available candle devices"},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef py_candle_driver =
{
  PyModuleDef_HEAD_INIT,
  "candle_driver", /* name of module */
  "This module provides an interface for candle CAN driver windows API.",
  -1,   /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
  module_methods
};

PyMODINIT_FUNC PyInit_candle_driver(void)
{
  if (PyType_Ready(&py_candle_device_type) < 0)
    return NULL;
  
  if (PyType_Ready(&py_candle_channel_type) < 0)
    return NULL;

  PyObject* m = PyModule_Create(&py_candle_driver);
  if (m == NULL)
    return NULL;

  Py_INCREF(&py_candle_device_type);
  PyModule_AddObject(m, "device", (PyObject*)&py_candle_device_type);

  Py_INCREF(&py_candle_channel_type);
  PyModule_AddObject(m, "channel", (PyObject*)&py_candle_channel_type);

  PyModule_AddIntConstant(m, "CANDLE_MODE_NORMAL", CANDLE_MODE_NORMAL);
  PyModule_AddIntConstant(m, "CANDLE_MODE_LISTEN_ONLY", CANDLE_MODE_LISTEN_ONLY);
  PyModule_AddIntConstant(m, "CANDLE_MODE_LOOP_BACK", CANDLE_MODE_LOOP_BACK);
  PyModule_AddIntConstant(m, "CANDLE_MODE_TRIPLE_SAMPLE", CANDLE_MODE_TRIPLE_SAMPLE);
  PyModule_AddIntConstant(m, "CANDLE_MODE_ONE_SHOT", CANDLE_MODE_ONE_SHOT);
  PyModule_AddIntConstant(m, "CANDLE_MODE_HW_TIMESTAMP", CANDLE_MODE_HW_TIMESTAMP);

  PyModule_AddIntConstant(m, "CANDLE_DEVSTATE_AVAIL", CANDLE_DEVSTATE_AVAIL);
  PyModule_AddIntConstant(m, "CANDLE_DEVSTATE_INUSE", CANDLE_DEVSTATE_INUSE);

  PyModule_AddIntConstant(m, "CANDLE_ID_EXTENDED", CANDLE_ID_EXTENDED);
  PyModule_AddIntConstant(m, "CANDLE_ID_RTR", CANDLE_ID_RTR);
  PyModule_AddIntConstant(m, "CANDLE_ID_ERR", CANDLE_ID_ERR);

  PyModule_AddIntConstant(m, "CANDLE_FRAMETYPE_UNKNOWN", CANDLE_FRAMETYPE_UNKNOWN);
  PyModule_AddIntConstant(m, "CANDLE_FRAMETYPE_RECEIVE", CANDLE_FRAMETYPE_RECEIVE);
  PyModule_AddIntConstant(m, "CANDLE_FRAMETYPE_ECHO", CANDLE_FRAMETYPE_ECHO);
  PyModule_AddIntConstant(m, "CANDLE_FRAMETYPE_ERROR", CANDLE_FRAMETYPE_ERROR);
  PyModule_AddIntConstant(m, "CANDLE_FRAMETYPE_TIMESTAMP_OVFL", CANDLE_FRAMETYPE_TIMESTAMP_OVFL);

  PyModule_AddIntConstant(m, "CANDLE_ERR_OK", CANDLE_ERR_OK);
  PyModule_AddIntConstant(m, "CANDLE_ERR_CREATE_FILE", CANDLE_ERR_CREATE_FILE);
  PyModule_AddIntConstant(m, "CANDLE_ERR_WINUSB_INITIALIZE", CANDLE_ERR_WINUSB_INITIALIZE);
  PyModule_AddIntConstant(m, "CANDLE_ERR_QUERY_INTERFACE", CANDLE_ERR_QUERY_INTERFACE);
  PyModule_AddIntConstant(m, "CANDLE_ERR_QUERY_PIPE", CANDLE_ERR_QUERY_PIPE);
  PyModule_AddIntConstant(m, "CANDLE_ERR_PARSE_IF_DESCR", CANDLE_ERR_PARSE_IF_DESCR);
  PyModule_AddIntConstant(m, "CANDLE_ERR_SET_HOST_FORMAT", CANDLE_ERR_SET_HOST_FORMAT);
  PyModule_AddIntConstant(m, "CANDLE_ERR_GET_DEVICE_INFO", CANDLE_ERR_GET_DEVICE_INFO);
  PyModule_AddIntConstant(m, "CANDLE_ERR_GET_BITTIMING_CONST", CANDLE_ERR_GET_BITTIMING_CONST);
  PyModule_AddIntConstant(m, "CANDLE_ERR_PREPARE_READ", CANDLE_ERR_PREPARE_READ);
  PyModule_AddIntConstant(m, "CANDLE_ERR_SET_DEVICE_MODE", CANDLE_ERR_SET_DEVICE_MODE);
  PyModule_AddIntConstant(m, "CANDLE_ERR_SET_BITTIMING", CANDLE_ERR_SET_BITTIMING);
  PyModule_AddIntConstant(m, "CANDLE_ERR_BITRATE_FCLK", CANDLE_ERR_BITRATE_FCLK);
  PyModule_AddIntConstant(m, "CANDLE_ERR_BITRATE_UNSUPPORTED", CANDLE_ERR_BITRATE_UNSUPPORTED);
  PyModule_AddIntConstant(m, "CANDLE_ERR_SEND_FRAME", CANDLE_ERR_SEND_FRAME);
  PyModule_AddIntConstant(m, "CANDLE_ERR_READ_TIMEOUT", CANDLE_ERR_READ_TIMEOUT);
  PyModule_AddIntConstant(m, "CANDLE_ERR_READ_WAIT", CANDLE_ERR_READ_WAIT);
  PyModule_AddIntConstant(m, "CANDLE_ERR_READ_RESULT", CANDLE_ERR_READ_RESULT);
  PyModule_AddIntConstant(m, "CANDLE_ERR_READ_SIZE", CANDLE_ERR_READ_SIZE);
  PyModule_AddIntConstant(m, "CANDLE_ERR_SETUPDI_IF_DETAILS", CANDLE_ERR_SETUPDI_IF_DETAILS);
  PyModule_AddIntConstant(m, "CANDLE_ERR_SETUPDI_IF_DETAILS2", CANDLE_ERR_SETUPDI_IF_DETAILS2);
  PyModule_AddIntConstant(m, "CANDLE_ERR_MALLOC", CANDLE_ERR_MALLOC);
  PyModule_AddIntConstant(m, "CANDLE_ERR_PATH_LEN", CANDLE_ERR_PATH_LEN);
  PyModule_AddIntConstant(m, "CANDLE_ERR_CLSID", CANDLE_ERR_CLSID);
  PyModule_AddIntConstant(m, "CANDLE_ERR_GET_DEVICES", CANDLE_ERR_GET_DEVICES);
  PyModule_AddIntConstant(m, "CANDLE_ERR_SETUPDI_IF_ENUM", CANDLE_ERR_SETUPDI_IF_ENUM);
  PyModule_AddIntConstant(m, "CANDLE_ERR_SET_TIMESTAMP_MODE", CANDLE_ERR_SET_TIMESTAMP_MODE);
  PyModule_AddIntConstant(m, "CANDLE_ERR_DEV_OUT_OF_RANGE", CANDLE_ERR_DEV_OUT_OF_RANGE);
  PyModule_AddIntConstant(m, "CANDLE_ERR_GET_TIMESTAMP", CANDLE_ERR_GET_TIMESTAMP);
  PyModule_AddIntConstant(m, "CANDLE_ERR_SET_PIPE_RAW_IO", CANDLE_ERR_SET_PIPE_RAW_IO);

  return m;
}