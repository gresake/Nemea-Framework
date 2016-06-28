#include <Python.h>
#include <structmember.h>
#include <libtrap/trap.h>
#include <unirec/unirec.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "fields.h"

UR_FIELDS()

extern PyObject *TrapError;

/*********************/
/*    UnirecTime     */
/*********************/
static PyTypeObject pytrap_UnirecTime;

typedef struct {
    PyObject_HEAD
    ur_time_t timestamp;
} pytrap_unirectime;

static PyObject *
UnirecTime_compare(PyObject *a, PyObject *b, int op)
{
    PyObject *result;

    if (!PyObject_IsInstance(a, (PyObject *) &pytrap_UnirecTime) ||
             !PyObject_IsInstance(b, (PyObject *) &pytrap_UnirecTime)) {
        result = Py_NotImplemented;
        goto out;
    }

    pytrap_unirectime *ur_a = (pytrap_unirectime *) a;
    pytrap_unirectime *ur_b = (pytrap_unirectime *) b;

    switch (op) {
    case Py_EQ:
        result = (ur_a->timestamp == ur_b->timestamp ? Py_True : Py_False);
        break;
    case Py_NE:
        result = (ur_a->timestamp != ur_b->timestamp ? Py_True : Py_False);
        break;
    case Py_LE:
        result = (ur_a->timestamp <= ur_b->timestamp ? Py_True : Py_False);
        break;
    case Py_GE:
        result = (ur_a->timestamp >= ur_b->timestamp ? Py_True : Py_False);
        break;
    case Py_LT:
        result = (ur_a->timestamp < ur_b->timestamp ? Py_True : Py_False);
        break;
    case Py_GT:
        result = (ur_a->timestamp > ur_b->timestamp ? Py_True : Py_False);
        break;
    default:
        result = Py_NotImplemented;
    }

out:
    Py_INCREF(result);
    return result;
}

static PyObject *
UnirecTime_getSeconds(pytrap_unirectime *self)
{
    return PyLong_FromLong(ur_time_get_sec(self->timestamp));
}

static PyObject *
UnirecTime_getMiliSeconds(pytrap_unirectime *self)
{
    return PyLong_FromLong(ur_time_get_msec(self->timestamp));
}

static PyObject *
UnirecTime_getTimeAsFloat(pytrap_unirectime *self)
{
    double t = (double) ur_time_get_sec(self->timestamp);
    t += (double) ur_time_get_msec(self->timestamp) / 1000;
    return PyFloat_FromDouble(t);
}

static PyMethodDef pytrap_unirectime_methods[] = {
    {"getSeconds", (PyCFunction) UnirecTime_getSeconds, METH_NOARGS,
        "Get number of seconds of timestamp.\n\n"
        "Returns:\n"
        "    (long): Retrieved number of seconds.\n"
    },
    {"getMiliSeconds", (PyCFunction) UnirecTime_getMiliSeconds, METH_NOARGS,
        "Get number of seconds of timestamp.\n\n"
        "Returns:\n"
        "    (long): Retrieved number of seconds.\n"
    },
    {"getTimeAsFloat", (PyCFunction) UnirecTime_getTimeAsFloat, METH_NOARGS,
        "Get number of seconds of timestamp.\n\n"
        "Returns:\n"
        "    (double): Retrieved timestamp as floiting point number.\n"
    },

    {NULL, NULL, 0, NULL}
};

static PyObject *
UnirecTime_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    pytrap_unirectime *self;
    uint32_t secs, msecs;

    self = (pytrap_unirectime *) type->tp_alloc(type, 0);
    if (self != NULL) {
        if (!PyArg_ParseTuple(args, "II", &secs, &msecs)) {
            Py_DECREF(self);
            return NULL;
        }
        self->timestamp = ur_time_from_sec_msec(secs, msecs);
    }

    return (PyObject *) self;
}

static PyObject *
UnirecTime_str(pytrap_unirectime *self)
{
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromFormat("%u.%03u", ur_time_get_sec(self->timestamp),
        ur_time_get_msec(self->timestamp));
#else
    return PyString_FromFormat("%u.%03u", ur_time_get_sec(self->timestamp),
        ur_time_get_msec(self->timestamp));
#endif
}

long
UnirecTime_hash(pytrap_unirectime *o)
{
    return (long) o->timestamp;
}

static PyObject *
UnirecTime_nb_add(PyObject *a, PyObject *b)
{
    uint64_t sec, msec;
    if (!PyObject_IsInstance(a, (PyObject *) &pytrap_UnirecTime)) {
        PyErr_SetString(PyExc_TypeError, "First argument must be of UnirecTime type.");
        return NULL;
    }

    pytrap_unirectime *ur_a = (pytrap_unirectime *) a;
    msec = ur_time_get_msec(ur_a->timestamp);
    sec = ur_time_get_sec(ur_a->timestamp);
    if (PyObject_IsInstance(b, (PyObject *) &pytrap_UnirecTime)) {
        pytrap_unirectime *ur_b = (pytrap_unirectime *) b;
        msec += ur_time_get_msec(ur_b->timestamp);
        sec += ur_time_get_sec(ur_b->timestamp) + (msec / 1000);
        msec %= 1000;
    } else if (PyLong_Check(b)) {
        int64_t tmp_b = PyLong_AsLong(b);
        sec += tmp_b;
#if PY_MAJOR_VERSION < 3
    } else if (PyInt_Check(b)) {
        int64_t tmp_b = PyInt_AsLong(b);
        sec += tmp_b;
#endif
    } else {
        PyErr_SetString(PyExc_TypeError, "Unsupported type for this operation.");
        return NULL;
    }

    ur_a = (pytrap_unirectime *) pytrap_UnirecTime.tp_alloc(&pytrap_UnirecTime, 0);
    ur_a->timestamp = ur_time_from_sec_msec((uint32_t) sec, (uint32_t) msec);
    return (PyObject *) ur_a;
}

static PyObject *
UnirecTime_nb_float(pytrap_unirectime *self)
{
    double res = (double) ur_time_get_sec(self->timestamp);
    res += (double) ur_time_get_msec(self->timestamp) / 1000;
    return PyFloat_FromDouble(res);
}

#if PY_MAJOR_VERSION >= 3
static PyNumberMethods UnirecTime_numbermethods = {
     (binaryfunc) UnirecTime_nb_add, /* binaryfunc nb_add; */
     0, /* binaryfunc nb_subtract; */
     0, /* binaryfunc nb_multiply; */
     0, /* binaryfunc nb_remainder; */
     0, /* binaryfunc nb_divmod; */
     0, /* ternaryfunc nb_power; */
     0, /* unaryfunc nb_negative; */
     0, /* unaryfunc nb_positive; */
     0, /* unaryfunc nb_absolute; */
     0, /* inquiry nb_bool; */
     0, /* unaryfunc nb_invert; */
     0, /* binaryfunc nb_lshift; */
     0, /* binaryfunc nb_rshift; */
     0, /* binaryfunc nb_and; */
     0, /* binaryfunc nb_xor; */
     0, /* binaryfunc nb_or; */
     0, /* unaryfunc nb_int; */
     0, /* void *nb_reserved; */
     (unaryfunc) UnirecTime_nb_float, /* unaryfunc nb_float; */
     (binaryfunc) UnirecTime_nb_add, /* binaryfunc nb_inplace_add; */
     0, /* binaryfunc nb_inplace_subtract; */
     0, /* binaryfunc nb_inplace_multiply; */
     0, /* binaryfunc nb_inplace_remainder; */
     0, /* ternaryfunc nb_inplace_power; */
     0, /* binaryfunc nb_inplace_lshift; */
     0, /* binaryfunc nb_inplace_rshift; */
     0, /* binaryfunc nb_inplace_and; */
     0, /* binaryfunc nb_inplace_xor; */
     0, /* binaryfunc nb_inplace_or; */
     0, /* binaryfunc nb_floor_divide; */
     0, /* binaryfunc nb_true_divide; */
     0, /* binaryfunc nb_inplace_floor_divide; */
     0, /* binaryfunc nb_inplace_true_divide; */
     0  /* unaryfunc nb_index; */
};
#else
static PyNumberMethods UnirecTime_numbermethods = {
     (binaryfunc) UnirecTime_nb_add, /* binaryfunc nb_add; */
     0, /* binaryfunc nb_subtract; */
     0, /* binaryfunc nb_multiply; */
     0, /* binaryfunc nb_divide; */
     0, /* binaryfunc nb_remainder; */
     0, /* binaryfunc nb_divmod; */
     0, /* ternaryfunc nb_power; */
     0, /* unaryfunc nb_negative; */
     0, /* unaryfunc nb_positive; */
     0, /* unaryfunc nb_absolute; */
     0, /* inquiry nb_nonzero; */
     0, /* unaryfunc nb_invert; */
     0, /* binaryfunc nb_lshift; */
     0, /* binaryfunc nb_rshift; */
     0, /* binaryfunc nb_and; */
     0, /* binaryfunc nb_xor; */
     0, /* binaryfunc nb_or; */
     0, /* coercion nb_coerce; */
     0, /* unaryfunc nb_int; */
     0, /* unaryfunc nb_long; */
     (unaryfunc) UnirecTime_nb_float, /* unaryfunc nb_float; */
     0, /* unaryfunc nb_oct; */
     0, /* unaryfunc nb_hex; */
     (binaryfunc) UnirecTime_nb_add, /* binaryfunc nb_inplace_add; */
     0, /* binaryfunc nb_inplace_subtract; */
     0, /* binaryfunc nb_inplace_multiply; */
     0, /* binaryfunc nb_inplace_divide; */
     0, /* binaryfunc nb_inplace_remainder; */
     0, /* ternaryfunc nb_inplace_power; */
     0, /* binaryfunc nb_inplace_lshift; */
     0, /* binaryfunc nb_inplace_rshift; */
     0, /* binaryfunc nb_inplace_and; */
     0, /* binaryfunc nb_inplace_xor; */
     0, /* binaryfunc nb_inplace_or; */
     0, /* binaryfunc nb_floor_divide; */
     0, /* binaryfunc nb_true_divide; */
     0, /* binaryfunc nb_inplace_floor_divide; */
     0, /* binaryfunc nb_inplace_true_divide; */
     0, /* unaryfunc nb_index; */
};
#endif

static PyTypeObject pytrap_UnirecTime = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.UnirecTime", /* tp_name */
    sizeof(pytrap_unirectime), /* tp_basicsize */
    0, /* tp_itemsize */
    0, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_reserved */
    0, /* tp_repr */
    &UnirecTime_numbermethods, /* tp_as_number */
    0, /* tp_as_sequence */
    0, /* tp_as_mapping */
    (hashfunc) UnirecTime_hash, /* tp_hash  */
    0, /* tp_call */
    (reprfunc) UnirecTime_str, /* tp_str */
    0, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
#if PY_MAJOR_VERSION < 3
        Py_TPFLAGS_CHECKTYPES |
#endif
        Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Class for UniRec timestamp storage and base data access.", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    (richcmpfunc) UnirecTime_compare, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    pytrap_unirectime_methods, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
    0, /* tp_base */
    0, /* tp_dict */
    0, /* tp_descr_get */
    0, /* tp_descr_set */
    0, /* tp_dictoffset */
    0, /* tp_init */
    0, /* tp_alloc */
    UnirecTime_new, /* tp_new */
};


/*********************/
/*    UnirecIPAddr   */
/*********************/
static PyTypeObject pytrap_UnirecIPAddr;

typedef struct {
    PyObject_HEAD
    ip_addr_t ip;
} pytrap_unirecipaddr;

static PyObject *
UnirecIPAddr_compare(PyObject *a, PyObject *b, int op)
{
    PyObject *result;

    if (!PyObject_IsInstance(a, (PyObject *) &pytrap_UnirecIPAddr) ||
             !PyObject_IsInstance(b, (PyObject *) &pytrap_UnirecIPAddr)) {
        result = Py_NotImplemented;
        goto out;
    }

    pytrap_unirecipaddr *ur_a = (pytrap_unirecipaddr *) a;
    pytrap_unirecipaddr *ur_b = (pytrap_unirecipaddr *) b;

    int res = ip_cmp(&ur_a->ip, &ur_b->ip);

    switch (op) {
    case Py_EQ:
        result = (res == 0 ? Py_True : Py_False);
        break;
    case Py_NE:
        result = (res != 0 ? Py_True : Py_False);
        break;
    case Py_LE:
        result = (res <= 0 ? Py_True : Py_False);
        break;
    case Py_GE:
        result = (res >= 0 ? Py_True : Py_False);
        break;
    case Py_LT:
        result = (res < 0 ? Py_True : Py_False);
        break;
    case Py_GT:
        result = (res > 0 ? Py_True : Py_False);
        break;
    default:
        result = Py_NotImplemented;
    }

out:
    Py_INCREF(result);
    return result;
}

static PyMethodDef pytrap_unirecipaddr_methods[] = {
    {NULL, NULL, 0, NULL}
};

int
UnirecIPAddr_init(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
    pytrap_unirecipaddr *s = (pytrap_unirecipaddr *) self;
    char *ip_str;

    if (s != NULL) {
        if (!PyArg_ParseTuple(args, "s", &ip_str)) {
            return EXIT_FAILURE;
        }
        if (ip_from_str(ip_str, &s->ip) != 1) {
            PyErr_SetString(TrapError, "Could not parse given IP address.");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;

}

static PyObject *
UnirecIPAddr_str(pytrap_unirecipaddr *self)
{
    char str[INET6_ADDRSTRLEN];
    ip_to_str(&self->ip, str);
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromString(str);
#else
    return PyString_FromString(str);
#endif
}

long
UnirecIPAddr_hash(pytrap_unirecipaddr *o)
{
    if (ip_is4(&o->ip)) {
        return (long) o->ip.ui32[2];
    } else {
        return (long) (o->ip.ui64[0] ^ o->ip.ui64[1]);
    }
}

static PyTypeObject pytrap_UnirecIPAddr = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.UnirecIPAddr",          /* tp_name */
    sizeof(pytrap_unirecipaddr),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    (hashfunc) UnirecIPAddr_hash,                         /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc) UnirecIPAddr_str,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "Class for UniRec IP Address storage and base data access.",         /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    (richcmpfunc) UnirecIPAddr_compare,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pytrap_unirecipaddr_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc) UnirecIPAddr_init,                         /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew,         /* tp_new */
};


/*********************/
/* UnirecTemplate    */
/*********************/


typedef struct {
    PyObject_HEAD
    ur_template_t *urtmplt;
    char *data;
    Py_ssize_t data_size;
    PyDictObject *urdict;
} pytrap_unirectemplate;

static inline PyObject *
UnirecTemplate_get_local(pytrap_unirectemplate *self, char *data, int32_t field_id)
{
    void *value = ur_get_ptr_by_id(self->urtmplt, data, field_id);

    switch (ur_get_type(field_id)) {
    case UR_TYPE_UINT8:
        return Py_BuildValue("B", *(uint8_t *) value);
        break;
    case UR_TYPE_UINT16:
        return Py_BuildValue("H", *(uint16_t *) value);
        break;
    case UR_TYPE_UINT32:
        return Py_BuildValue("I", *(uint32_t *) value);
        break;
    case UR_TYPE_UINT64:
        return Py_BuildValue("K", *(uint64_t *) value);
        break;
    case UR_TYPE_INT8:
        return Py_BuildValue("c", *(int8_t *) value);
        break;
    case UR_TYPE_INT16:
        return Py_BuildValue("h", *(int16_t *) value);
        break;
    case UR_TYPE_INT32:
        return Py_BuildValue("i", *(int32_t *) value);
        break;
    case UR_TYPE_INT64:
        return Py_BuildValue("L", *(int64_t *) value);
        break;
    case UR_TYPE_CHAR:
        return Py_BuildValue("b", *(char *) value);
        break;
    case UR_TYPE_FLOAT:
        return Py_BuildValue("f", *(float *) value);
        break;
    case UR_TYPE_DOUBLE:
        return Py_BuildValue("d", *(double *) value);
        break;
    case UR_TYPE_IP:
        {
            pytrap_unirecipaddr *new_ip = (pytrap_unirecipaddr *) pytrap_UnirecIPAddr.tp_alloc(&pytrap_UnirecIPAddr, 0);
            memcpy(&new_ip->ip, value, sizeof(ip_addr_t));
            return (PyObject *) new_ip;
        }
    case UR_TYPE_TIME:
        {
            pytrap_unirectime *new_time = (pytrap_unirectime *) pytrap_UnirecTime.tp_alloc(&pytrap_UnirecTime, 0);
            new_time->timestamp = *((ur_time_t *) value);
            return (PyObject *) new_time;
        }
    case UR_TYPE_STRING:
        {
            Py_ssize_t value_size = ur_get_var_len(self->urtmplt, data, field_id);
#if PY_MAJOR_VERSION >= 3
            return PyUnicode_FromStringAndSize(value, value_size);
#else
            return PyString_FromStringAndSize(value, value_size);
#endif
        }
        break;
    case UR_TYPE_BYTES:
        {
            Py_ssize_t value_size = ur_get_var_len(self->urtmplt, data, field_id);
            return PyByteArray_FromStringAndSize(value, value_size);
        }
        break;
    default:
        PyErr_SetString(PyExc_NotImplementedError, "Unknown UniRec field type.");
        return NULL;
    } // case (field type)
    Py_RETURN_NONE;
}

static PyObject *
UnirecTemplate_get(pytrap_unirectemplate *self, PyObject *args, PyObject *keywds)
{
    int32_t field_id;
    PyObject *dataObj;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"field_id", "data", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "IO", kwlist, &field_id, &dataObj)) {
        return NULL;
    }

    if (PyObject_IsInstance(dataObj, (PyObject *) &PyBytes_Type)) {
        PyBytes_AsStringAndSize(dataObj, &data, &data_size);
    } else if (PyByteArray_Check(dataObj)) {
        //data_size = PyByteArray_Size(dataObj);
        data = PyByteArray_AsString(dataObj);
    } else {
        PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
        return NULL;
    }

    return UnirecTemplate_get_local(self, data, field_id);

}

static inline PyObject *
UnirecTemplate_set_local(pytrap_unirectemplate *self, char *data, int32_t field_id, PyObject *valueObj)
{
    PY_LONG_LONG longval;
    double floatval;
    void *value = ur_get_ptr_by_id(self->urtmplt, data, field_id);

    switch (ur_get_type(field_id)) {
    case UR_TYPE_UINT8:
        longval = PyLong_AsLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((uint8_t *) value) = (uint8_t) longval;
        }
        break;
    case UR_TYPE_UINT16:
        longval = PyLong_AsLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((uint16_t *) value) = (uint16_t) longval;
        }
        break;
    case UR_TYPE_UINT32:
        longval = PyLong_AsLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((uint32_t *) value) = (uint32_t) longval;
        }
        break;
    case UR_TYPE_UINT64:
        longval = PyLong_AsLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((uint64_t *) value) = (uint64_t) longval;
        }
        break;
    case UR_TYPE_INT8:
        longval = PyLong_AsLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((int8_t *) value) = (int8_t) longval;
        }
        break;
    case UR_TYPE_INT16:
        longval = PyLong_AsLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((int16_t *) value) = (int16_t) longval;
        }
        break;
    case UR_TYPE_INT32:
        longval = PyLong_AsLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((int32_t *) value) = (int32_t) longval;
        }
        break;
    case UR_TYPE_INT64:
        longval = PyLong_AsLongLong(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((int64_t *) value) = (int64_t) longval;
        }
        break;
    case UR_TYPE_CHAR:
        PyErr_SetString(PyExc_NotImplementedError, "Unknown UniRec field type.");
        return NULL;
        break;
    case UR_TYPE_FLOAT:
        floatval = PyFloat_AsDouble(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((float *) value) = (float) floatval;
        }
        break;
    case UR_TYPE_DOUBLE:
        floatval = PyFloat_AsDouble(valueObj);
        if (PyErr_Occurred() == NULL) {
            *((double *) value) = floatval;
        }
        break;
    case UR_TYPE_IP:
        if (PyObject_IsInstance(valueObj, (PyObject *) &pytrap_UnirecIPAddr)) {
            pytrap_unirecipaddr *src = ((pytrap_unirecipaddr *) valueObj);
            ip_addr_t *dest = (ip_addr_t *) value;
            dest->ui64[0] = src->ip.ui64[0];
            dest->ui64[1] = src->ip.ui64[1];
        }
        break;
    case UR_TYPE_TIME:
        if (PyObject_IsInstance(valueObj, (PyObject *) &pytrap_UnirecTime)) {
            pytrap_unirectime *src = ((pytrap_unirectime *) valueObj);
            *((ur_time_t *) value) = src->timestamp;
        }
        break;
    case UR_TYPE_STRING:
        {
            Py_ssize_t size;
            char *str;
#if PY_MAJOR_VERSION >= 3
            if (!PyUnicode_Check(valueObj)) {
                PyErr_SetString(PyExc_TypeError, "String object expected.");
                return NULL;
            }
            str = PyUnicode_AsUTF8AndSize(valueObj, &size);
#else
            if (!PyString_Check(valueObj)) {
                PyErr_SetString(PyExc_TypeError, "String object expected.");
                return NULL;
            }
            if (PyString_AsStringAndSize(valueObj, &str, &size) == -1) {
                return NULL;
            }
#endif
            if (str != NULL) {
                /* TODO check return value */
                ur_set_var(self->urtmplt, data, field_id, str, size);
            }
        }
        break;
    case UR_TYPE_BYTES:
        {
            Py_ssize_t size;
            char *str;

            if (PyObject_IsInstance(valueObj, (PyObject *) &PyBytes_Type)) {
                PyBytes_AsStringAndSize(valueObj, &str, &size);
            } else if (PyByteArray_Check(valueObj)) {
                size = PyByteArray_Size(valueObj);
                str = PyByteArray_AsString(valueObj);
            } else {
                PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
                return NULL;
            }

            if (str != NULL) {
                /* TODO check return value */
                ur_set_var(self->urtmplt, data, field_id, str, size);
            }
        }
        break;
    default:
        {
            PyErr_SetString(PyExc_TypeError, "Unknown UniRec field type.");
            return NULL;
        }
        break;
    } // case (field type)
    Py_RETURN_NONE;
}

static PyObject *
UnirecTemplate_set(pytrap_unirectemplate *self, PyObject *args, PyObject *keywds)
{
    uint32_t field_id;
    PyObject *dataObj, *valueObj;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"field_id", "data", "value", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "IOO", kwlist, &field_id, &dataObj, &valueObj)) {
        return NULL;
    }

    if (PyObject_IsInstance(dataObj, (PyObject *) &PyBytes_Type)) {
        PyBytes_AsStringAndSize(dataObj, &data, &data_size);
    } else if (PyByteArray_Check(dataObj)) {
        data = PyByteArray_AsString(dataObj);
    } else {
        PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
        return NULL;
    }

    return UnirecTemplate_set_local(self, data, field_id, valueObj);
}

PyObject *
UnirecTemplate_getFieldsDict(pytrap_unirectemplate *self)
{
    PyObject *d = PyDict_New();
    PyObject *key;
    int i;

    if (d != NULL) {
        for (i = 0; i < self->urtmplt->count; i++) {
#if PY_MAJOR_VERSION >= 3
            key = PyUnicode_FromString(ur_get_name(self->urtmplt->ids[i]));
#else
            key = PyString_FromString(ur_get_name(self->urtmplt->ids[i]));
#endif
            PyDict_SetItem(d, key, PyLong_FromLong(self->urtmplt->ids[i]));
        }
        return d;
    }
    Py_RETURN_NONE;
}

static PyObject *
UnirecTemplate_setData(pytrap_unirectemplate *self, PyObject *args, PyObject *kwds)
{
    PyObject *dataObj;
    char *data;
    Py_ssize_t data_size;

    static char *kwlist[] = {"data", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &dataObj)) {
        return NULL;
    }

    if (PyObject_IsInstance(dataObj, (PyObject *) &PyBytes_Type)) {
        PyBytes_AsStringAndSize(dataObj, &data, &data_size);
    } else if (PyByteArray_Check(dataObj)) {
        data_size = PyByteArray_Size(dataObj);
        data = PyByteArray_AsString(dataObj);
    } else {
        PyErr_SetString(PyExc_TypeError, "Argument data must be of bytes or bytearray type.");
        return NULL;
    }

    self->data = data;
    self->data_size = data_size;

    Py_RETURN_NONE;
}

static PyObject *
UnirecTemplate_createMessage(pytrap_unirectemplate *self, PyObject *args, PyObject *kwds)
{
    char *data;
    uint32_t data_size = 0;

    static char *kwlist[] = {"dyn_size", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|H", kwlist, &data_size)) {
        return NULL;
    }
    data_size += ur_rec_fixlen_size(self->urtmplt);
    if (data_size >= UR_MAX_SIZE) {
        PyErr_SetString(TrapError, "Max size of message is 65535 bytes.");
        return NULL;
    }
    data = ur_create_record(self->urtmplt, (uint16_t) data_size);
    PyObject *res = PyByteArray_FromStringAndSize(data, (uint16_t) data_size);
    free(data);
    return res;
}

static PyMethodDef pytrap_unirectemplate_methods[] = {
        {"get", (PyCFunction) UnirecTemplate_get, METH_VARARGS | METH_KEYWORDS,
            "Get value of the field from the UniRec message.\n\n"
            "Args:\n"
            "    field_id (int): Field ID.\n"
            "    data (bytes): Data - UniRec message.\n"
            "Returns:\n"
            "    (object): Retrieved value of the field (depends on UniRec template).\n"
        },

        {"set", (PyCFunction) UnirecTemplate_set, METH_VARARGS | METH_KEYWORDS,
            "Set value of the field in the UniRec message.\n\n"
            "Args:\n"
            "    field_id (int): Field ID.\n"
            "    data (bytes): Data - UniRec message.\n"
            "    value (object): New value of the field (depends on UniRec template).\n\n"
            "Raises:\n"
            "    TypeError: Bad object type of value was given.\n"
            "    ValueError: Bad value was given.\n"
        },

        {"setData", (PyCFunction) UnirecTemplate_setData, METH_VARARGS | METH_KEYWORDS,
            "Set data for attribute access.\n\n"
            "Args:\n"
            "    data (bytes): Data - UniRec message.\n"
        },

        {"getFieldsDict", (PyCFunction) UnirecTemplate_getFieldsDict, METH_NOARGS,
            "Get set of fields of the template.\n\n"
            "Returns:\n"
            "    Dict(str,int): Dictionary of field_id with field name as a key.\n"
        },

        {"createMessage", (PyCFunction) UnirecTemplate_createMessage, METH_VARARGS | METH_KEYWORDS,
            "Create a message that can be filled in with values according to the template.\n\n"
            "Args:\n"
            "    dyn_size (int): Maximal size of variable data (in total).\n\n"
            "Returns:\n"
            "    ByteArray: Allocated memory that can be filled in using get().\n"
        },

        {NULL, NULL, 0, NULL}
};

static PyObject *
UnirecTemplate_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    pytrap_unirectemplate *self;
    const char *spec;
    // TODO return error string of failure during unirec template init
    //char *errstring;

    self = (pytrap_unirectemplate *) type->tp_alloc(type, 0);
    if (self != NULL) {
        static char *kwlist[] = {"spec", NULL};
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &spec)) {
            Py_DECREF(self);
            return NULL;
        }
        self->urtmplt = NULL;
        int ret;
        if ((ret = ur_define_set_of_fields(spec)) != UR_OK) {
            PyErr_SetString(TrapError, "ur_define_set_of_fields() failed.");
            Py_DECREF(self);
            return NULL;
        }
        /* XXX errstring */
        //self->urtmplt = ur_create_template(spec, &errstring);
        self->urtmplt = ur_create_template_from_ifc_spec(spec);
        if (self->urtmplt == NULL) {
            //PyErr_SetString(TrapError, errstring);
            PyErr_SetString(TrapError, "Creation of UniRec template failed.");
            //free(errstring);
            Py_DECREF(self);
            return NULL;
        }
        self->data = NULL;
        self->data_size = 0;
        self->urdict = (PyDictObject *) UnirecTemplate_getFieldsDict(self);
    }

    return (PyObject *) self;
}

static void UnirecTemplate_dealloc(pytrap_unirectemplate *self)
{
    Py_DECREF(self->urdict);
    ur_free_template(self->urtmplt);
    Py_DECREF(self);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyObject *
UnirecTemplate_str(pytrap_unirectemplate *self)
{
    char *s = ur_template_string_delimiter(self->urtmplt, ',');
    PyObject *result;
#if PY_MAJOR_VERSION >= 3
    result = PyUnicode_FromFormat("(%s)", s);
#else
    result = PyString_FromFormat("(%s)", s);
#endif
    free(s);
    return result;
}

/*
TODO
class URWrapper:
    def __iter__(self):
        for i in range(self._numfields):
            yield self._tmpl.get(i, self._data)

    def strRecord(self):
        return "\n".join(["{0} ({2})\t=\t{1}".format(i, self.__getattr__(i), self._urdict[i]) for i in self._urdict])
*/

static PyObject *
UnirecTemplate_getAttr(pytrap_unirectemplate *self, PyObject *attr)
{
    PyObject *v = PyDict_GetItem((PyObject *) self->urdict, attr);
    if (v == NULL) {
        return PyObject_GenericGetAttr((PyObject *) self, attr);
    }
    int32_t field_id;
    field_id = (int32_t) PyLong_AsLong(v);

    return UnirecTemplate_get_local(self, self->data, field_id);
}

static int
UnirecTemplate_setAttr(pytrap_unirectemplate *self, PyObject *attr, PyObject *value)
{
    PyObject *v = PyDict_GetItem((PyObject *) self->urdict, attr);
    if (v == NULL) {
        return PyObject_GenericSetAttr((PyObject *) self, attr, value);
    }
    int32_t field_id;
    field_id = (int32_t) PyLong_AsLong(v);

    if (UnirecTemplate_set_local(self, self->data, field_id, value) == NULL) {
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
}

Py_ssize_t UnirecTemplate_len(pytrap_unirectemplate *o)
{
   return PyDict_Size((PyObject *) o->urdict);
}

static PySequenceMethods UnirecTemplate_seqmethods = {
    (lenfunc) UnirecTemplate_len, /* lenfunc sq_length; */
    0, /* binaryfunc sq_concat; */
    0, /* ssizeargfunc sq_repeat; */
    0, /* ssizeargfunc sq_item; */
    0, /* void *was_sq_slice; */
    0, /* ssizeobjargproc sq_ass_item; */
    0, /* void *was_sq_ass_slice; */
    0, /* objobjproc sq_contains; */
    0, /* binaryfunc sq_inplace_concat; */
    0 /* ssizeargfunc sq_inplace_repeat; */
};

static PyTypeObject pytrap_UnirecTemplate = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pytrap.UnirecTemplate",          /* tp_name */
    sizeof(pytrap_unirectemplate),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) UnirecTemplate_dealloc,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    &UnirecTemplate_seqmethods,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc) UnirecTemplate_str,                         /* tp_str */
    (getattrofunc) UnirecTemplate_getAttr,                         /* tp_getattro */
    (setattrofunc) UnirecTemplate_setAttr,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "Class for UniRec template storage and base data access.",         /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pytrap_unirectemplate_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    UnirecTemplate_new,                         /* tp_new */
};

/**
 * \brief Initialize UniRec template class and add it to pytrap module.
 *
 * \param [in,out] m    pointer to the module Object
 * \return EXIT_SUCCESS or EXIT_FAILURE
 */
int init_unirectemplate(PyObject *m)
{
    /* Add Time */
    if (PyType_Ready(&pytrap_UnirecTime) < 0) {
        return EXIT_FAILURE;
    }
    Py_INCREF(&pytrap_UnirecTime);
    PyModule_AddObject(m, "UnirecTime", (PyObject *) &pytrap_UnirecTime);

    /* Add IPAddr */
    //pytrap_UnirecTemplate.tp_new = PyType_GenericNew;
    if (PyType_Ready(&pytrap_UnirecIPAddr) < 0) {
        return EXIT_FAILURE;
    }
    Py_INCREF(&pytrap_UnirecIPAddr);
    PyModule_AddObject(m, "UnirecIPAddr", (PyObject *) &pytrap_UnirecIPAddr);


    /* Add Template */
    if (PyType_Ready(&pytrap_UnirecTemplate) < 0) {
        return EXIT_FAILURE;
    }
    Py_INCREF(&pytrap_UnirecTemplate);
    PyModule_AddObject(m, "UnirecTemplate", (PyObject *) &pytrap_UnirecTemplate);


    return EXIT_SUCCESS;
}

