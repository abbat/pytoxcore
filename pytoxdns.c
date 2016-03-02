/**
 * pytoxcore
 *
 * Copyright (C) 2015 Anton Batenev <antonbatenev@yandex.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
//----------------------------------------------------------------------------------------------
#include "pytoxdns.h"
//----------------------------------------------------------------------------------------------
#ifndef TOX_CLIENT_ID_SIZE
    #include <sodium/crypto_box.h>
    #define TOX_CLIENT_ID_SIZE crypto_box_PUBLICKEYBYTES
#endif
//----------------------------------------------------------------------------------------------
#ifndef TOX_FRIEND_ADDRESS_SIZE
    #include <sodium/crypto_box.h>
    #define TOX_FRIEND_ADDRESS_SIZE (crypto_box_PUBLICKEYBYTES + sizeof(uint32_t) + sizeof(uint16_t))
#endif
//----------------------------------------------------------------------------------------------
#define CHECK_TOXDNS(self)                                         \
    if ((self)->dns == NULL) {                                     \
        PyErr_SetString(ToxDNSException, "toxdns object killed."); \
        return NULL;                                               \
    }
//----------------------------------------------------------------------------------------------
PyObject* ToxDNSException;
//----------------------------------------------------------------------------------------------

static PyObject* ToxDNS_tox_dns3_kill(ToxDNS* self, PyObject* args)
{
    if (self->dns != NULL) {
        tox_dns3_kill(self->dns);
        self->dns = NULL;
    }

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxDNS_tox_decrypt_dns3_TXT(ToxDNS* self, PyObject* args)
{
    CHECK_TOXDNS(self);

    uint8_t*   id_record;
    Py_ssize_t id_record_len;
    uint32_t   request_id;

    if (PyArg_ParseTuple(args, "s#I", &id_record, &id_record_len, &request_id) == false)
        return NULL;

    uint8_t tox_id[TOX_FRIEND_ADDRESS_SIZE];
    int result = tox_decrypt_dns3_TXT(self->dns, tox_id, id_record, id_record_len, request_id);

    if (result == -1) {
        PyErr_SetString(ToxDNSException, "tox_decrypt_dns3_TXT failed.");
        return NULL;
    }

    uint8_t tox_id_hex[TOX_FRIEND_ADDRESS_SIZE * 2 + 1];
    memset(tox_id_hex, 0, sizeof(uint8_t) * (TOX_FRIEND_ADDRESS_SIZE * 2 + 1));

    bytes_to_hex_string(tox_id, TOX_FRIEND_ADDRESS_SIZE, tox_id_hex);

    return PYSTRING_FromString((const char*)tox_id_hex);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxDNS_tox_generate_dns3_string(ToxDNS* self, PyObject* args)
{
    CHECK_TOXDNS(self);

    uint8_t*   name;
    Py_ssize_t name_len;

    if (PyArg_ParseTuple(args, "s#", &name, &name_len) == false)
        return NULL;

    const size_t string_max_len = 256;
    uint8_t string[string_max_len + 1];
    memset(string, 0, string_max_len + 1);

    uint32_t request_id;
    int result = tox_generate_dns3_string(self->dns, string, string_max_len, &request_id, name, name_len);

    if (result == -1) {
        PyErr_SetString(ToxDNSException, "tox_generate_dns3_string failed.");
        return NULL;
    }

    return Py_BuildValue("s#I", string, result, request_id);
}
//----------------------------------------------------------------------------------------------

PyMethodDef ToxDNS_methods[] = {
    {
        "tox_generate_dns3_string", (PyCFunction)ToxDNS_tox_generate_dns3_string, METH_VARARGS,
        "tox_generate_dns3_string(name)\n"
        "Generate a dns3 string used to query the dns server.\n"
        "Returns tuple of string, request_id which must be passed to tox_decrypt_dns3_TXT() "
        "to correctly decode the response"
    },
    {
        "tox_decrypt_dns3_TXT", (PyCFunction)ToxDNS_tox_decrypt_dns3_TXT, METH_VARARGS,
        "tox_decrypt_dns3_TXT(id_record, request_id)\n"
        "Decode and decrypt the id_record.\n"
        "request_id is the request id given by tox_generate_dns3_string() when creating the request."
    },
    {
        NULL
    }
};
//----------------------------------------------------------------------------------------------

static int init_helper(ToxDNS* self, PyObject* args)
{
    ToxDNS_tox_dns3_kill(self, NULL);

    uint8_t*   key_hex;
    Py_ssize_t key_hex_len;
    if (args == NULL || PyArg_ParseTuple(args, "s#", &key_hex, &key_hex_len) == false) {
        PyErr_SetString(ToxDNSException, "You must supply a server public key as constructor argument.");
        return -1;
    }

    if (key_hex_len != TOX_CLIENT_ID_SIZE * 2) {
        PyErr_SetString(ToxDNSException, "Server public key must be hex string of TOX_CLIENT_ID_SIZE length.");
        return -1;
    }

    uint8_t key[TOX_CLIENT_ID_SIZE];
    if (hex_string_to_bytes(key_hex, TOX_CLIENT_ID_SIZE, key) == false) {
        PyErr_SetString(ToxDNSException, "Invalid key hex value.");
        return -1;
    }

    void* dns = tox_dns3_new(key);
    if (dns == NULL) {
        PyErr_SetString(ToxDNSException, "Failed to init ToxDNS.");
        return -1;
    }

    self->dns = dns;

    return 0;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxDNS_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    ToxDNS* self = (ToxDNS*)type->tp_alloc(type, 0);

    self->dns = NULL;

    if (init_helper(self, args) == -1)
        return NULL;

    return (PyObject*)self;
}
//----------------------------------------------------------------------------------------------

static int ToxDNS_init(ToxDNS* self, PyObject* args, PyObject* kwds)
{
    return init_helper(self, args);
}
//----------------------------------------------------------------------------------------------

static int ToxDNS_dealloc(ToxDNS* self)
{
    ToxDNS_tox_dns3_kill(self, NULL);

    return 0;
}
//----------------------------------------------------------------------------------------------

PyTypeObject ToxDNSType = {
#if PY_MAJOR_VERSION >= 3
    PyVarObject_HEAD_INIT(NULL, 0)
#else
    PyObject_HEAD_INIT(NULL)
    0,                                          /* ob_size           */
#endif
    "ToxDNS",                                   /* tp_name           */
    sizeof(ToxDNS),                             /* tp_basicsize      */
    0,                                          /* tp_itemsize       */
    (destructor)ToxDNS_dealloc,                 /* tp_dealloc        */
    0,                                          /* tp_print          */
    0,                                          /* tp_getattr        */
    0,                                          /* tp_setattr        */
    0,                                          /* tp_compare        */
    0,                                          /* tp_repr           */
    0,                                          /* tp_as_number      */
    0,                                          /* tp_as_sequence    */
    0,                                          /* tp_as_mapping     */
    0,                                          /* tp_hash           */
    0,                                          /* tp_call           */
    0,                                          /* tp_str            */
    0,                                          /* tp_getattro       */
    0,                                          /* tp_setattro       */
    0,                                          /* tp_as_buffer      */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags          */
    "ToxDNS object",                            /* tp_doc            */
    0,                                          /* tp_traverse       */
    0,                                          /* tp_clear          */
    0,                                          /* tp_richcompare    */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter           */
    0,                                          /* tp_iternext       */
    ToxDNS_methods,                             /* tp_methods        */
    0,                                          /* tp_members        */
    0,                                          /* tp_getset         */
    0,                                          /* tp_base           */
    0,                                          /* tp_dict           */
    0,                                          /* tp_descr_get      */
    0,                                          /* tp_descr_set      */
    0,                                          /* tp_dictoffset     */
    (initproc)ToxDNS_init,                      /* tp_init           */
    0,                                          /* tp_alloc          */
    ToxDNS_new                                  /* tp_new            */
};
//----------------------------------------------------------------------------------------------

void ToxDNS_install_dict(void)
{
#define SET(name)                                  \
    PyObject* obj_##name = PyLong_FromLong(name);  \
    PyDict_SetItemString(dict, #name, obj_##name); \
    Py_DECREF(obj_##name)

    PyObject* dict = PyDict_New();
    if (dict == NULL)
        return;

    SET(TOX_CLIENT_ID_SIZE);

#undef SET

    ToxDNSType.tp_dict = dict;
}
//----------------------------------------------------------------------------------------------
