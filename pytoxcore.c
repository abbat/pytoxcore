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
#include "pytoxcore.h"
//----------------------------------------------------------------------------------------------
#define CHECK_TOX(self)                                              \
    if ((self)->tox == NULL) {                                       \
        PyErr_SetString(ToxCoreException, "toxcore object killed."); \
        return NULL;                                                 \
    }
//----------------------------------------------------------------------------------------------
PyObject* ToxCoreException;
//----------------------------------------------------------------------------------------------

static void callback_self_connection_status(Tox* tox, TOX_CONNECTION connection_status, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_self_connection_status_cb", "I", connection_status);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_friend_request(Tox* tox, const uint8_t* public_key, const uint8_t* message, size_t length, void* self)
{
    uint8_t buf[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    memset(buf, 0, sizeof(uint8_t) * (TOX_PUBLIC_KEY_SIZE * 2 + 1));

    bytes_to_hex_string(public_key, TOX_PUBLIC_KEY_SIZE, buf);

    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_friend_request_cb", "ss#", buf, message, length);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_friend_message(Tox* tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_friend_message_cb", "Is#", friend_number, message, length);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_friend_name(Tox* tox, uint32_t friend_number, const uint8_t* name, size_t length, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_friend_name_cb", "Is#", friend_number, name, length);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_friend_status_message(Tox* tox, uint32_t friend_number, const uint8_t* message, size_t length, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_friend_status_message_cb", "Is#", friend_number, message, length);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_friend_status(Tox* tox, uint32_t friend_number, TOX_USER_STATUS status, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_friend_status_cb", "II", friend_number, status);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_friend_read_receipt(Tox* tox, uint32_t friend_number, uint32_t message_id, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_friend_read_receipt_cb", "II", friend_number, message_id);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_friend_connection_status(Tox* tox, uint32_t friend_number, TOX_CONNECTION connection_status, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_friend_connection_status_cb", "II", friend_number, connection_status);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_friend_typing(Tox* tox, uint32_t friend_number, bool is_typing, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_friend_typing_cb", "II", friend_number, is_typing);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_file_chunk_request(Tox* tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_file_chunk_request_cb", "IIKK", friend_number, file_number, position, length);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_file_recv(Tox* tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t* filename, size_t filename_length, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();

    if (kind == TOX_FILE_KIND_DATA)
        PyObject_CallMethod((PyObject*)self, "tox_file_recv_cb", "IIIKs#", friend_number, file_number, kind, file_size, filename, filename_length);
    else if (kind == TOX_FILE_KIND_AVATAR)
        PyObject_CallMethod((PyObject*)self, "tox_file_recv_cb", "IIIKs#", friend_number, file_number, kind, file_size, NULL, 0);

    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_file_recv_control(Tox* tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_file_recv_control_cb", "III", friend_number, file_number, control);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_file_recv_chunk(Tox* tox, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t* data, size_t length, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_file_recv_chunk_cb", "IIK" BUF_TCS, friend_number, file_number, position, data, length);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_friend_lossy_packet(Tox* tox, uint32_t friend_number, const uint8_t* data, size_t length, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_friend_lossy_packet_cb", "I" BUF_TCS, friend_number, data, length);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static void callback_friend_lossless_packet(Tox* tox, uint32_t friend_number, const uint8_t* data, size_t length, void* self)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject_CallMethod((PyObject*)self, "tox_friend_lossless_packet_cb", "I" BUF_TCS, friend_number, data, length);
    PyGILState_Release(gil);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_callback_stub(ToxCore* self, PyObject* args)
{
    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_version_major(ToxCore* self, PyObject* args)
{
    uint32_t result = tox_version_major();

    return PyLong_FromUnsignedLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_version_minor(ToxCore* self, PyObject* args)
{
    uint32_t result = tox_version_minor();

    return PyLong_FromUnsignedLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_version_patch(ToxCore* self, PyObject* args)
{
    uint32_t result = tox_version_patch();

    return PyLong_FromUnsignedLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_version_is_compatible(ToxCore* self, PyObject* args)
{
    uint32_t major;
    uint32_t minor;
    uint32_t patch;

    if (PyArg_ParseTuple(args, "III", &major, &minor, &patch) == false)
        return NULL;

    bool result = tox_version_is_compatible(major, minor, patch);

    return PyBool_FromLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_options_default(ToxCore* self, PyObject* args)
{
    struct Tox_Options options;
    tox_options_default(&options);

    PyObject* dict = PyDict_New();
    if (dict == NULL)
        return NULL;

    PyObject* obj_ipv6_enabled = PyBool_FromLong(options.ipv6_enabled);
    PyDict_SetItemString(dict, "ipv6_enabled", obj_ipv6_enabled);
    Py_DECREF(obj_ipv6_enabled);

    PyObject* obj_udp_enabled = PyBool_FromLong(options.udp_enabled);
    PyDict_SetItemString(dict, "udp_enabled", obj_udp_enabled);
    Py_DECREF(obj_udp_enabled);

    PyObject* obj_proxy_type = PyLong_FromUnsignedLong(options.proxy_type);
    PyDict_SetItemString(dict, "proxy_type", obj_proxy_type);
    Py_DECREF(obj_proxy_type);

    PyObject* obj_proxy_host = (options.proxy_host == NULL ? Py_None : PYSTRING_FromString(options.proxy_host));
    PyDict_SetItemString(dict, "proxy_host", obj_proxy_host);
    Py_DECREF(obj_proxy_host);

    PyObject* obj_proxy_port = PyLong_FromUnsignedLong(options.proxy_port);
    PyDict_SetItemString(dict, "proxy_port", obj_proxy_port);
    Py_DECREF(obj_proxy_port);

    PyObject* obj_start_port = PyLong_FromUnsignedLong(options.start_port);
    PyDict_SetItemString(dict, "start_port", obj_start_port);
    Py_DECREF(obj_start_port);

    PyObject* obj_end_port = PyLong_FromUnsignedLong(options.end_port);
    PyDict_SetItemString(dict, "end_port", obj_end_port);
    Py_DECREF(obj_end_port);

    PyObject* obj_tcp_port = PyLong_FromUnsignedLong(options.tcp_port);
    PyDict_SetItemString(dict, "tcp_port", obj_tcp_port);
    Py_DECREF(obj_tcp_port);

    PyObject* obj_savedata_data = (options.savedata_data == NULL ? Py_None : PYBYTES_FromStringAndSize((const char*)options.savedata_data, options.savedata_length));
    PyDict_SetItemString(dict, "savedata_data", obj_savedata_data);
    Py_DECREF(obj_savedata_data);

    return dict;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_address(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t address[TOX_ADDRESS_SIZE];
    uint8_t address_hex[TOX_ADDRESS_SIZE * 2 + 1];

    memset(address_hex, 0, sizeof(uint8_t) * (TOX_ADDRESS_SIZE * 2 + 1));

    tox_self_get_address(self->tox, address);

    bytes_to_hex_string(address, TOX_ADDRESS_SIZE, address_hex);

    return PYSTRING_FromString((const char*)address_hex);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_set_nospam(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t*   nospam_hex;
    Py_ssize_t nospam_hex_len;

    if (PyArg_ParseTuple(args, "s#", &nospam_hex, &nospam_hex_len) == false)
        return NULL;

    if (nospam_hex_len != sizeof(uint32_t) * 2) {
        PyErr_SetString(ToxCoreException, "nospam must be hex string of 4 bytes length.");
        return NULL;
    }

    uint32_t nospam;
    hex_string_to_bytes(nospam_hex, sizeof(uint32_t), (uint8_t*)(&nospam));

    tox_self_set_nospam(self->tox, nospam);

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_nospam(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t result = tox_self_get_nospam(self->tox);

    uint8_t result_hex[sizeof(uint32_t) * 2 + 1];
    memset(result_hex, 0, sizeof(uint8_t) * (sizeof(uint32_t) * 2 + 1));

    bytes_to_hex_string((const uint8_t*)(&result), sizeof(uint32_t), result_hex);

    return PYSTRING_FromString((const char*)result_hex);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_public_key(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    tox_self_get_public_key(self->tox, public_key);

    uint8_t public_key_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    memset(public_key_hex, 0, sizeof(uint8_t) * (TOX_PUBLIC_KEY_SIZE * 2 + 1));

    bytes_to_hex_string(public_key, TOX_PUBLIC_KEY_SIZE, public_key_hex);

    return PYSTRING_FromString((const char*)public_key_hex);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_secret_key(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t secret_key[TOX_SECRET_KEY_SIZE];
    tox_self_get_secret_key(self->tox, secret_key);

    uint8_t secret_key_hex[TOX_SECRET_KEY_SIZE * 2 + 1];
    memset(secret_key_hex, 0, sizeof(uint8_t) * (TOX_SECRET_KEY_SIZE * 2 + 1));

    bytes_to_hex_string(secret_key, TOX_SECRET_KEY_SIZE, secret_key_hex);

    return PYSTRING_FromString((const char*)secret_key_hex);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_kill(ToxCore* self, PyObject* args)
{
    if (self->tox != NULL) {
        tox_kill(self->tox);
        self->tox = NULL;
    }

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_get_savedata_size(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    size_t size = tox_get_savedata_size(self->tox);

    return PyLong_FromSize_t(size);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_get_savedata(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    size_t size = tox_get_savedata_size(self->tox);

    uint8_t* savedata = (uint8_t*)malloc(size);
    if (savedata == NULL) {
        PyErr_SetString(ToxCoreException, "Can not allocate memory.");
        return NULL;
    }

    tox_get_savedata(self->tox, savedata);

    PyObject* result = PYBYTES_FromStringAndSize((const char*)savedata, size);

    free(savedata);

    return result;
}
//----------------------------------------------------------------------------------------------

static PyObject* parse_TOX_ERR_BOOTSTRAP(bool result, TOX_ERR_BOOTSTRAP error)
{
    bool success = false;
    switch (error) {
        case TOX_ERR_BOOTSTRAP_OK:
            success = true;
            break;
        case TOX_ERR_BOOTSTRAP_NULL:
            PyErr_SetString(ToxCoreException, "One of the arguments to the function was NULL when it was not expected.");
            break;
        case TOX_ERR_BOOTSTRAP_BAD_HOST:
            PyErr_SetString(ToxCoreException, "The address could not be resolved to an IP address, or the IP address passed was invalid.");
            break;
        case TOX_ERR_BOOTSTRAP_BAD_PORT:
            PyErr_SetString(ToxCoreException, "The port passed was invalid. The valid port range is (1, 65535).");
            break;
    }

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_bootstrap(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    char*      address;
    Py_ssize_t address_len;
    uint16_t   port;
    uint8_t*   public_key_hex;
    Py_ssize_t public_key_hex_len;

    if (PyArg_ParseTuple(args, "s#Hs#", &address, &address_len, &port, &public_key_hex, &public_key_hex_len) == false)
        return NULL;

    if (public_key_hex_len != TOX_PUBLIC_KEY_SIZE * 2) {
        PyErr_SetString(ToxCoreException, "public_key must be hex string of TOX_PUBLIC_KEY_SIZE length.");
        return NULL;
    }

    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];

    hex_string_to_bytes(public_key_hex, TOX_PUBLIC_KEY_SIZE, public_key);

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_BOOTSTRAP error;
    bool result = tox_bootstrap(self->tox, address, port, public_key, &error);

    PyEval_RestoreThread(gil);

    return parse_TOX_ERR_BOOTSTRAP(result, error);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_add_tcp_relay(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    char*      address;
    Py_ssize_t address_len;
    uint16_t   port;
    uint8_t*   public_key_hex;
    Py_ssize_t public_key_hex_len;

    if (PyArg_ParseTuple(args, "s#Hs#", &address, &address_len, &port, &public_key_hex, &public_key_hex_len) == false)
        return NULL;

    if (public_key_hex_len != TOX_PUBLIC_KEY_SIZE * 2) {
        PyErr_SetString(ToxCoreException, "public_key must be hex string of TOX_PUBLIC_KEY_SIZE length.");
        return NULL;
    }

    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];

    hex_string_to_bytes(public_key_hex, TOX_PUBLIC_KEY_SIZE, public_key);

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_BOOTSTRAP error;
    bool result = tox_add_tcp_relay(self->tox, address, port, public_key, &error);

    PyEval_RestoreThread(gil);

    return parse_TOX_ERR_BOOTSTRAP(result, error);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_connection_status(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    TOX_CONNECTION connection_status = tox_self_get_connection_status(self->tox);

    return PyLong_FromUnsignedLong(connection_status);
}
//----------------------------------------------------------------------------------------------

static PyObject* parse_TOX_ERR_FRIEND_ADD(uint32_t friend_number, TOX_ERR_FRIEND_ADD error)
{
    bool success = false;
    switch (error) {
        case TOX_ERR_FRIEND_ADD_OK:
            success = true;
            break;
        case TOX_ERR_FRIEND_ADD_NULL:
            PyErr_SetString(ToxCoreException, "One of the arguments to the function was NULL when it was not expected.");
            break;
        case TOX_ERR_FRIEND_ADD_TOO_LONG:
            PyErr_SetString(ToxCoreException, "The length of the friend request message exceeded TOX_MAX_FRIEND_REQUEST_LENGTH.");
            break;
        case TOX_ERR_FRIEND_ADD_NO_MESSAGE:
            PyErr_SetString(ToxCoreException, "The friend request message was empty. This, and the TOO_LONG code will never be returned from tox_friend_add_norequest.");
            break;
        case TOX_ERR_FRIEND_ADD_OWN_KEY:
            PyErr_SetString(ToxCoreException, "The friend address belongs to the sending client.");
            break;
        case TOX_ERR_FRIEND_ADD_ALREADY_SENT:
            PyErr_SetString(ToxCoreException, "A friend request has already been sent, or the address belongs to a friend that is already on the friend list.");
            break;
        case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM:
            PyErr_SetString(ToxCoreException, "The friend address checksum failed.");
            break;
        case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM:
            PyErr_SetString(ToxCoreException, "The friend was already there, but the nospam value was different.");
            break;
        case TOX_ERR_FRIEND_ADD_MALLOC:
            PyErr_SetString(ToxCoreException, "A memory allocation failed when trying to increase the friend list size.");
            break;
    }

    if (success == false || friend_number == UINT32_MAX)
        return NULL;

    return PyLong_FromLong(friend_number);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_add(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t*   address_hex;
    Py_ssize_t address_hex_len;
    uint8_t*   message;
    Py_ssize_t message_len;

    if (PyArg_ParseTuple(args, "s#s#", &address_hex, &address_hex_len, &message, &message_len) == false)
        return NULL;

    if (address_hex_len != TOX_ADDRESS_SIZE * 2) {
        PyErr_SetString(ToxCoreException, "address must be hex string of TOX_ADDRESS_SIZE length.");
        return NULL;
    }

    uint8_t address[TOX_ADDRESS_SIZE];

    hex_string_to_bytes(address_hex, TOX_ADDRESS_SIZE, address);

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_FRIEND_ADD error;
    uint32_t friend_number = tox_friend_add(self->tox, address, message, message_len, &error);

    PyEval_RestoreThread(gil);

    return parse_TOX_ERR_FRIEND_ADD(friend_number, error);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_add_norequest(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t*   public_key_hex;
    Py_ssize_t public_key_hex_len;

    if (PyArg_ParseTuple(args, "s#", &public_key_hex, &public_key_hex_len) == false)
        return NULL;

    if (public_key_hex_len != TOX_PUBLIC_KEY_SIZE * 2) {
        PyErr_SetString(ToxCoreException, "public_key must be hex string of TOX_PUBLIC_KEY_SIZE length.");
        return NULL;
    }

    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];

    hex_string_to_bytes(public_key_hex, TOX_PUBLIC_KEY_SIZE, public_key);

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_FRIEND_ADD error;
    uint32_t friend_number = tox_friend_add_norequest(self->tox, public_key, &error);

    PyEval_RestoreThread(gil);

    return parse_TOX_ERR_FRIEND_ADD(friend_number, error);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_delete(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;

    if (PyArg_ParseTuple(args, "I", &friend_number) == false)
        return NULL;

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_FRIEND_DELETE error;
    bool result = tox_friend_delete(self->tox, friend_number, &error);

    PyEval_RestoreThread(gil);

    bool success = false;
    switch (error) {
        case TOX_ERR_FRIEND_DELETE_OK:
            success = true;
            break;
        case TOX_ERR_FRIEND_DELETE_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "There was no friend with the given friend number. No friends were deleted.");
            break;
    }

    if (success == false || result != true)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_by_public_key(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t*   public_key_hex;
    Py_ssize_t public_key_hex_len;

    if (PyArg_ParseTuple(args, "s#", &public_key_hex, &public_key_hex_len) == false)
        return NULL;

    if (public_key_hex_len != TOX_PUBLIC_KEY_SIZE * 2) {
        PyErr_SetString(ToxCoreException, "public_key must be hex string of TOX_PUBLIC_KEY_SIZE length.");
        return NULL;
    }

    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    hex_string_to_bytes(public_key_hex, TOX_PUBLIC_KEY_SIZE, public_key);

    TOX_ERR_FRIEND_BY_PUBLIC_KEY error;
    uint32_t result = tox_friend_by_public_key(self->tox, public_key, &error);

    bool success = false;
    switch (error) {
        case TOX_ERR_FRIEND_BY_PUBLIC_KEY_OK:
            success = true;
            break;
        case TOX_ERR_FRIEND_BY_PUBLIC_KEY_NULL:
            PyErr_SetString(ToxCoreException, "One of the arguments to the function was NULL when it was not expected.");
            break;
        case TOX_ERR_FRIEND_BY_PUBLIC_KEY_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "No friend with the given Public Key exists on the friend list.");
            break;
    }

    if (result == UINT32_MAX || success == false)
        return NULL;

    return PyLong_FromUnsignedLong(result);
}
//----------------------------------------------------------------------------------------------

static bool parse_TOX_ERR_FRIEND_QUERY(TOX_ERR_FRIEND_QUERY error)
{
    bool success = false;
    switch (error) {
        case TOX_ERR_FRIEND_QUERY_OK:
            success = true;
            break;
        case TOX_ERR_FRIEND_QUERY_NULL:
            PyErr_SetString(ToxCoreException, "The pointer parameter for storing the query result (name, message) was NULL. Unlike the _self_ variants of these functions, which have no effect when a parameter is NULL, these functions return an error in that case.");
            break;
        case TOX_ERR_FRIEND_QUERY_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "The friend_number did not designate a valid friend.");
            break;
    }

    return success;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_get_connection_status(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;

    if (PyArg_ParseTuple(args, "I", &friend_number) == false)
        return NULL;

    TOX_ERR_FRIEND_QUERY error;
    TOX_CONNECTION connection_status = tox_friend_get_connection_status(self->tox, friend_number, &error);

    bool success = parse_TOX_ERR_FRIEND_QUERY(error);
    if (success == false)
        return NULL;

    return PyLong_FromUnsignedLong(connection_status);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_exists(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;

    if (PyArg_ParseTuple(args, "I", &friend_number) == false)
        return NULL;

    bool result = tox_friend_exists(self->tox, friend_number);

    return PyBool_FromLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_send_message(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t   friend_number;
    int        type;
    uint8_t*   message;
    Py_ssize_t message_len;

    if (PyArg_ParseTuple(args, "Iis#", &friend_number, &type, &message, &message_len) == false)
        return NULL;

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_FRIEND_SEND_MESSAGE error = 0;
    uint32_t message_id = tox_friend_send_message(self->tox, friend_number, type, message, message_len, &error);

    PyEval_RestoreThread(gil);

    bool success = false;
    switch (error) {
        case TOX_ERR_FRIEND_SEND_MESSAGE_OK:
            success = true;
            break;
        case TOX_ERR_FRIEND_SEND_MESSAGE_NULL:
            PyErr_SetString(ToxCoreException, "One of the arguments to the function was NULL when it was not expected.");
            break;
        case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "The friend number did not designate a valid friend.");
            break;
        case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_CONNECTED:
            PyErr_SetString(ToxCoreException, "This client is currently not connected to the friend.");
            break;
        case TOX_ERR_FRIEND_SEND_MESSAGE_SENDQ:
            PyErr_SetString(ToxCoreException, "An allocation error occurred while increasing the send queue size.");
            break;
        case TOX_ERR_FRIEND_SEND_MESSAGE_TOO_LONG:
            PyErr_SetString(ToxCoreException, "Message length exceeded TOX_MAX_MESSAGE_LENGTH.");
            break;
        case TOX_ERR_FRIEND_SEND_MESSAGE_EMPTY:
            PyErr_SetString(ToxCoreException, "Attempted to send a zero-length message.");
            break;
    }

    if (success == false)
        return NULL;

    return PyLong_FromUnsignedLong(message_id);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_set_name(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t*   name;
    Py_ssize_t name_len;

    if (PyArg_ParseTuple(args, "s#", &name, &name_len) == false)
        return NULL;

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_SET_INFO error;
    bool result = tox_self_set_name(self->tox, name, name_len, &error);

    PyEval_RestoreThread(gil);

    bool success = false;
    switch (error) {
        case TOX_ERR_SET_INFO_OK:
            success = true;
            break;
        case TOX_ERR_SET_INFO_NULL:
            PyErr_SetString(ToxCoreException, "One of the arguments to the function was NULL when it was not expected.");
            break;
        case TOX_ERR_SET_INFO_TOO_LONG:
            PyErr_SetString(ToxCoreException, "Information length exceeded maximum permissible size.");
            break;
    }

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_name(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t name[TOX_MAX_NAME_LENGTH];
    memset(name, 0, sizeof(uint8_t) * TOX_MAX_NAME_LENGTH);

    tox_self_get_name(self->tox, name);

    return PYSTRING_FromString((const char*)name);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_get_name(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;

    if (PyArg_ParseTuple(args, "I", &friend_number) == false)
        return NULL;

    uint8_t name[TOX_MAX_NAME_LENGTH];
    memset(name, 0, sizeof(uint8_t) * TOX_MAX_NAME_LENGTH);

    TOX_ERR_FRIEND_QUERY error;
    bool result = tox_friend_get_name(self->tox, friend_number, name, &error);

    bool success = parse_TOX_ERR_FRIEND_QUERY(error);

    if (result == false || success == false)
        return NULL;

    return PYSTRING_FromString((const char*)name);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_set_status_message(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t*   status_message;
    Py_ssize_t status_message_len;

    if (PyArg_ParseTuple(args, "s#", &status_message, &status_message_len) == false)
        return NULL;

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_SET_INFO error;
    bool result = tox_self_set_status_message(self->tox, status_message, status_message_len, &error);

    PyEval_RestoreThread(gil);

    bool success = false;
    switch (error) {
        case TOX_ERR_SET_INFO_OK:
            success = true;
            break;
        case TOX_ERR_SET_INFO_NULL:
            PyErr_SetString(ToxCoreException, "One of the arguments to the function was NULL when it was not expected.");
            break;
        case TOX_ERR_SET_INFO_TOO_LONG:
            PyErr_SetString(ToxCoreException, "Information length exceeded maximum permissible size.");
            break;
    }

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_set_status(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    TOX_USER_STATUS status;

    if (PyArg_ParseTuple(args, "I", &status) == false)
        return NULL;

    PyThreadState* gil = PyEval_SaveThread();

    tox_self_set_status(self->tox, status);

    PyEval_RestoreThread(gil);

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_get_status_message(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;

    if (PyArg_ParseTuple(args, "I", &friend_number) == false)
        return NULL;

    uint8_t status_message[TOX_MAX_STATUS_MESSAGE_LENGTH];
    memset(status_message, 0, sizeof(uint8_t) * TOX_MAX_STATUS_MESSAGE_LENGTH);

    TOX_ERR_FRIEND_QUERY error;
    bool result = tox_friend_get_status_message(self->tox, friend_number, status_message, &error);

    bool success = parse_TOX_ERR_FRIEND_QUERY(error);

    if (result == false || success == false)
        return NULL;

    return PYSTRING_FromString((const char*)status_message);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_status_message(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t status_message[TOX_MAX_STATUS_MESSAGE_LENGTH];
    memset(status_message, 0, sizeof(uint8_t) * TOX_MAX_STATUS_MESSAGE_LENGTH);

    tox_self_get_status_message(self->tox, status_message);

    return PYSTRING_FromString((const char*)status_message);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_get_status(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;

    if (PyArg_ParseTuple(args, "I", &friend_number) == false)
        return NULL;

    TOX_ERR_FRIEND_QUERY error;
    TOX_USER_STATUS status = tox_friend_get_status(self->tox, friend_number, &error);

    bool success = parse_TOX_ERR_FRIEND_QUERY(error);
    if (success == false)
        return NULL;

    return PyLong_FromUnsignedLong(status);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_get_typing(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;

    if (PyArg_ParseTuple(args, "I", &friend_number) == false)
        return NULL;

    TOX_ERR_FRIEND_QUERY error;
    bool result = tox_friend_get_typing(self->tox, friend_number, &error);

    bool success = parse_TOX_ERR_FRIEND_QUERY(error);
    if (success == false)
        return NULL;

    return PyBool_FromLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_set_typing(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;
    bool     typing;

    if (PyArg_ParseTuple(args, "II", &friend_number, &typing) == false)
        return NULL;

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_SET_TYPING error;
    bool result = tox_self_set_typing(self->tox, friend_number, typing, &error);

    PyEval_RestoreThread(gil);

    bool success = false;

    switch (error) {
        case TOX_ERR_SET_TYPING_OK:
            success = true;
            break;
        case TOX_ERR_SET_TYPING_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "The friend number did not designate a valid friend.");
            break;
    }

    if (success == false || result == false)
        return NULL;

    return PyBool_FromLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_status(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    TOX_USER_STATUS status = tox_self_get_status(self->tox);

    return PyLong_FromUnsignedLong(status);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_get_last_online(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;

    if (PyArg_ParseTuple(args, "I", &friend_number) == false)
        return NULL;

    TOX_ERR_FRIEND_GET_LAST_ONLINE error;
    uint64_t status = tox_friend_get_last_online(self->tox, friend_number, &error);

    bool success = false;
    switch (error) {
        case TOX_ERR_FRIEND_GET_LAST_ONLINE_OK:
            success = true;
            break;
        case TOX_ERR_FRIEND_GET_LAST_ONLINE_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "No friend with the given number exists on the friend list.");
            break;
    }

    if (status == UINT64_MAX || success == false)
        return NULL;

    return PyLong_FromUnsignedLongLong(status);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_friend_list_size(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    size_t result = tox_self_get_friend_list_size(self->tox);

    return PyLong_FromSize_t(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_friend_list(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    size_t    count = tox_self_get_friend_list_size(self->tox);
    uint32_t* list  = (uint32_t*)malloc(count * sizeof(uint32_t));

    if (list == NULL) {
        PyErr_SetString(ToxCoreException, "Can not allocate memory.");
        return NULL;
    }

    tox_self_get_friend_list(self->tox, list);

    PyObject* plist = PyList_New(count);
    if (plist == NULL) {
        free(list);
        PyErr_SetString(ToxCoreException, "Can not allocate memory.");
        return NULL;
    }

    size_t i = 0;
    for (i = 0; i < count; i++)
        if (PyList_Append(plist, PyLong_FromUnsignedLong(list[i])) != 0) {
            free(list);
            Py_DECREF(plist);
            return NULL;
        }

    free(list);

    return plist;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_get_public_key(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;

    if (PyArg_ParseTuple(args, "I", &friend_number) == false)
        return NULL;

    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    TOX_ERR_FRIEND_GET_PUBLIC_KEY error;
    bool result = tox_friend_get_public_key(self->tox, friend_number, public_key, &error);

    bool success = false;
    switch (error) {
        case TOX_ERR_FRIEND_GET_PUBLIC_KEY_OK:
            success = true;
            break;
        case TOX_ERR_FRIEND_GET_PUBLIC_KEY_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "No friend with the given number exists on the friend list.");
            break;
    }

    if (result == false || success == false)
        return NULL;

    uint8_t public_key_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    memset(public_key_hex, 0, sizeof(uint8_t) * (TOX_PUBLIC_KEY_SIZE * 2 + 1));

    bytes_to_hex_string(public_key, TOX_PUBLIC_KEY_SIZE, public_key_hex);

    return PYSTRING_FromString((const char*)public_key_hex);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_file_send(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t   friend_number;
    uint32_t   kind;
    uint64_t   file_size;
    uint8_t*   file_id_hex;
    Py_ssize_t file_id_hex_len;
    uint8_t*   filename;
    Py_ssize_t filename_len;

    if (PyArg_ParseTuple(args, "IIKz#s#", &friend_number, &kind, &file_size, &file_id_hex, &file_id_hex_len, &filename, &filename_len) == false)
        return NULL;

    uint8_t  file_id_buf[TOX_FILE_ID_LENGTH];
    uint8_t* file_id = file_id_buf;

    if (file_id_hex == NULL || file_id_hex_len == 0)
        file_id = NULL;
    else if (file_id_hex_len != TOX_FILE_ID_LENGTH * 2) {
        PyErr_SetString(ToxCoreException, "file_id must be hex string of TOX_FILE_ID_LENGTH length.");
        return NULL;
    } else
        hex_string_to_bytes(file_id_hex, TOX_FILE_ID_LENGTH, file_id_buf);

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_FILE_SEND error;
    uint32_t file_number = tox_file_send(self->tox, friend_number, kind, file_size, file_id, filename, filename_len, &error);

    PyEval_RestoreThread(gil);

    bool success = false;
    switch (error) {
        case TOX_ERR_FILE_SEND_OK:
            success = true;
            break;
        case TOX_ERR_FILE_SEND_NULL:
            PyErr_SetString(ToxCoreException, "One of the arguments to the function was NULL when it was not expected.");
            break;
        case TOX_ERR_FILE_SEND_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "The friend_number passed did not designate a valid friend.");
            break;
        case TOX_ERR_FILE_SEND_FRIEND_NOT_CONNECTED:
            PyErr_SetString(ToxCoreException, "This client is currently not connected to the friend.");
            break;
        case TOX_ERR_FILE_SEND_NAME_TOO_LONG:
            PyErr_SetString(ToxCoreException, "Filename length exceeded TOX_MAX_FILENAME_LENGTH bytes.");
            break;
        case TOX_ERR_FILE_SEND_TOO_MANY:
            PyErr_SetString(ToxCoreException, "Too many ongoing transfers. The maximum number of concurrent file transfers is 256 per friend per direction (sending and receiving).");
            break;
    }

    if (file_number == UINT32_MAX || success == false)
        return NULL;

    return PyLong_FromUnsignedLong(file_number);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_file_control(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t         friend_number;
    uint32_t         file_number;
    TOX_FILE_CONTROL control;

    if (PyArg_ParseTuple(args, "III", &friend_number, &file_number, &control) == false)
        return NULL;

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_FILE_CONTROL error;
    bool result = tox_file_control(self->tox, friend_number, file_number, control, &error);

    PyEval_RestoreThread(gil);

    bool success = false;
    switch (error) {
        case TOX_ERR_FILE_CONTROL_OK:
            success = true;
            break;
        case TOX_ERR_FILE_CONTROL_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "The friend_number passed did not designate a valid friend.");
            break;
        case TOX_ERR_FILE_CONTROL_FRIEND_NOT_CONNECTED:
            PyErr_SetString(ToxCoreException, "This client is currently not connected to the friend.");
            break;
        case TOX_ERR_FILE_CONTROL_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "No file transfer with the given file number was found for the given friend.");
            break;
        case TOX_ERR_FILE_CONTROL_NOT_PAUSED:
            PyErr_SetString(ToxCoreException, "A RESUME control was sent, but the file transfer is running normally.");
            break;
        case TOX_ERR_FILE_CONTROL_DENIED:
            PyErr_SetString(ToxCoreException, "A RESUME control was sent, but the file transfer was paused by the other party. Only the party that paused the transfer can resume it.");
            break;
        case TOX_ERR_FILE_CONTROL_ALREADY_PAUSED:
            PyErr_SetString(ToxCoreException, "A PAUSE control was sent, but the file transfer was already paused.");
            break;
        case TOX_ERR_FILE_CONTROL_SENDQ:
            PyErr_SetString(ToxCoreException, "Packet queue is full.");
            break;
    }

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_file_send_chunk(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t   friend_number;
    uint32_t   file_number;
    uint64_t   position;
    uint8_t*   data;
    Py_ssize_t length;

    if (PyArg_ParseTuple(args, "IIKs#", &friend_number, &file_number, &position, &data, &length) == false)
        return NULL;

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_FILE_SEND_CHUNK error;
    bool result = tox_file_send_chunk(self->tox, friend_number, file_number, position, data, length, &error);

    PyEval_RestoreThread(gil);

    bool success = false;
    switch (error) {
        case TOX_ERR_FILE_SEND_CHUNK_OK:
            success = true;
            break;
        case TOX_ERR_FILE_SEND_CHUNK_NULL:
            PyErr_SetString(ToxCoreException, "The length parameter was non-zero, but data was NULL.");
            break;
        case TOX_ERR_FILE_SEND_CHUNK_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "The friend_number passed did not designate a valid friend.");
            break;
        case TOX_ERR_FILE_SEND_CHUNK_FRIEND_NOT_CONNECTED:
            PyErr_SetString(ToxCoreException, "This client is currently not connected to the friend.");
            break;
        case TOX_ERR_FILE_SEND_CHUNK_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "No file transfer with the given file number was found for the given friend.");
            break;
        case TOX_ERR_FILE_SEND_CHUNK_NOT_TRANSFERRING:
            PyErr_SetString(ToxCoreException, "File transfer was found but isn't in a transferring state: (paused, done, broken, etc...) (happens only when not called from the request chunk callback).");
            break;
        case TOX_ERR_FILE_SEND_CHUNK_INVALID_LENGTH:
            PyErr_SetString(ToxCoreException, "Attempted to send more or less data than requested. The requested data size is adjusted according to maximum transmission unit and the expected end of the file. Trying to send less or more than requested will return this error.");
            break;
        case TOX_ERR_FILE_SEND_CHUNK_SENDQ:
            PyErr_SetString(ToxCoreException, "Packet queue is full.");
            break;
        case TOX_ERR_FILE_SEND_CHUNK_WRONG_POSITION:
            PyErr_SetString(ToxCoreException, "Position parameter was wrong.");
            break;
    }

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_dht_id(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint8_t dht_id[TOX_PUBLIC_KEY_SIZE];
    tox_self_get_dht_id(self->tox, dht_id);

    uint8_t dht_id_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    memset(dht_id_hex, 0, sizeof(uint8_t) * (TOX_PUBLIC_KEY_SIZE * 2 + 1));

    bytes_to_hex_string(dht_id, TOX_PUBLIC_KEY_SIZE, dht_id_hex);

    return PYSTRING_FromString((const char*)dht_id_hex);
}
//----------------------------------------------------------------------------------------------

static PyObject* parse_TOX_ERR_GET_PORT(uint16_t result, TOX_ERR_GET_PORT error)
{
    bool success = false;
    switch (error) {
        case TOX_ERR_GET_PORT_OK:
            success = true;
            break;
        case TOX_ERR_GET_PORT_NOT_BOUND:
            PyErr_SetString(ToxCoreException, "The instance was not bound to any port.");
            break;
    }

    if (success == false)
        return NULL;

    return PyLong_FromUnsignedLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_udp_port(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    TOX_ERR_GET_PORT error;
    uint16_t result = tox_self_get_udp_port(self->tox, &error);

    return parse_TOX_ERR_GET_PORT(result, error);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_self_get_tcp_port(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    TOX_ERR_GET_PORT error;
    uint16_t result = tox_self_get_tcp_port(self->tox, &error);

    return parse_TOX_ERR_GET_PORT(result, error);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_file_seek(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;
    uint32_t file_number;
    uint64_t position;

    if (PyArg_ParseTuple(args, "IIK", &friend_number, &file_number, &position) == false)
        return NULL;

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_FILE_SEEK error;
    bool result = tox_file_seek(self->tox, friend_number, file_number, position, &error);

    PyEval_RestoreThread(gil);

    bool success = false;
    switch (error) {
        case TOX_ERR_FILE_SEEK_OK:
            success = true;
            break;
        case TOX_ERR_FILE_SEEK_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "The friend_number passed did not designate a valid friend.");
            break;
        case TOX_ERR_FILE_SEEK_FRIEND_NOT_CONNECTED:
            PyErr_SetString(ToxCoreException, "This client is currently not connected to the friend.");
            break;
        case TOX_ERR_FILE_SEEK_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "No file transfer with the given file number was found for the given friend.");
            break;
        case TOX_ERR_FILE_SEEK_DENIED:
            PyErr_SetString(ToxCoreException, "File was not in a state where it could be seeked.");
            break;
        case TOX_ERR_FILE_SEEK_INVALID_POSITION:
            PyErr_SetString(ToxCoreException, "Seek position was invalid.");
            break;
        case TOX_ERR_FILE_SEEK_SENDQ:
            PyErr_SetString(ToxCoreException, "Packet queue is full.");
            break;
    }

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_file_get_file_id(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t friend_number;
    uint32_t file_number;

    if (PyArg_ParseTuple(args, "II", &friend_number, &file_number) == false)
        return NULL;

    uint8_t file_id[TOX_FILE_ID_LENGTH];

    TOX_ERR_FILE_GET error;
    bool result = tox_file_get_file_id(self->tox, friend_number, file_number, file_id, &error);

    bool success = false;
    switch (error) {
        case TOX_ERR_FILE_GET_OK:
            success = true;
            break;
        case TOX_ERR_FILE_GET_NULL:
            PyErr_SetString(ToxCoreException, "One of the arguments to the function was NULL when it was not expected.");
            break;
        case TOX_ERR_FILE_GET_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "The friend_number passed did not designate a valid friend.");
            break;
        case TOX_ERR_FILE_GET_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "No file transfer with the given file number was found for the given friend.");
            break;
    }

    if (result == false || success == false)
        return NULL;

    uint8_t file_id_hex[TOX_FILE_ID_LENGTH * 2 + 1];

    memset(file_id_hex, 0, sizeof(uint8_t) * (TOX_FILE_ID_LENGTH * 2 + 1));

    bytes_to_hex_string(file_id, TOX_FILE_ID_LENGTH, file_id_hex);

    return PYSTRING_FromString((const char*)file_id_hex);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_hash(ToxCore* self, PyObject* args)
{
    uint8_t*   data;
    Py_ssize_t data_len;

    if (PyArg_ParseTuple(args, "s#", &data, &data_len) == false)
        return NULL;

    uint8_t hash[TOX_HASH_LENGTH];

    bool result = tox_hash(hash, data, data_len);

    if (result == false)
        return NULL;

    uint8_t hash_hex[TOX_HASH_LENGTH * 2 + 1];

    memset(hash_hex, 0, sizeof(uint8_t) * (TOX_HASH_LENGTH * 2 + 1));

    bytes_to_hex_string(hash, TOX_FILE_ID_LENGTH, hash_hex);

    return PYSTRING_FromString((const char*)hash_hex);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_iteration_interval(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t interval = tox_iteration_interval(self->tox);

    return PyLong_FromUnsignedLong(interval);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_iterate(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    PyThreadState* gil = PyEval_SaveThread();
    tox_iterate(self->tox);
    PyEval_RestoreThread(gil);

    if (PyErr_Occurred() != NULL)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* parse_TOX_ERR_FRIEND_CUSTOM_PACKET(bool result, TOX_ERR_FRIEND_CUSTOM_PACKET error)
{
    bool success = false;
    switch (error) {
        case TOX_ERR_FRIEND_CUSTOM_PACKET_OK:
            success = true;
            break;
        case TOX_ERR_FRIEND_CUSTOM_PACKET_NULL:
            PyErr_SetString(ToxCoreException, "One of the arguments to the function was NULL when it was not expected.");
            break;
        case TOX_ERR_FRIEND_CUSTOM_PACKET_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "The friend number did not designate a valid friend.");
            break;
        case TOX_ERR_FRIEND_CUSTOM_PACKET_FRIEND_NOT_CONNECTED:
            PyErr_SetString(ToxCoreException, "This client is currently not connected to the friend.");
            break;
        case TOX_ERR_FRIEND_CUSTOM_PACKET_INVALID:
            PyErr_SetString(ToxCoreException, "The first byte of data was not in the specified range for the packet type. This range is 200-254 for lossy, and 160-191 for lossless packets.");
            break;
        case TOX_ERR_FRIEND_CUSTOM_PACKET_EMPTY:
            PyErr_SetString(ToxCoreException, "Attempted to send an empty packet.");
            break;
        case TOX_ERR_FRIEND_CUSTOM_PACKET_TOO_LONG:
            PyErr_SetString(ToxCoreException, "Packet data length exceeded TOX_MAX_CUSTOM_PACKET_SIZE.");
            break;
        case TOX_ERR_FRIEND_CUSTOM_PACKET_SENDQ:
            PyErr_SetString(ToxCoreException, "Packet queue is full.");
            break;
    }

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_send_lossy_packet(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t   friend_number;
    uint8_t*   data;
    Py_ssize_t data_len;

    if (PyArg_ParseTuple(args, "Is#", &friend_number, &data, &data_len) == false)
        return NULL;

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_FRIEND_CUSTOM_PACKET error;
    bool result = tox_friend_send_lossy_packet(self->tox, friend_number, data, data_len, &error);

    PyEval_RestoreThread(gil);

    return parse_TOX_ERR_FRIEND_CUSTOM_PACKET(result, error);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_tox_friend_send_lossless_packet(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);

    uint32_t   friend_number;
    uint8_t*   data;
    Py_ssize_t data_len;

    if (PyArg_ParseTuple(args, "Is#", &friend_number, &data, &data_len) == false)
        return NULL;

    PyThreadState* gil = PyEval_SaveThread();

    TOX_ERR_FRIEND_CUSTOM_PACKET error;
    bool result = tox_friend_send_lossless_packet(self->tox, friend_number, data, data_len, &error);

    PyEval_RestoreThread(gil);

    return parse_TOX_ERR_FRIEND_CUSTOM_PACKET(result, error);
}
//----------------------------------------------------------------------------------------------

PyMethodDef ToxCore_methods[] = {
    //
    // callbacks
    //

    {
        "tox_self_connection_status_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_self_connection_status_cb(connection_status)\n"
        "This event is triggered whenever there is a change in the DHT connection "
        "state. When disconnected, a client may choose to call tox_bootstrap again, to "
        "reconnect to the DHT. Note that this state may frequently change for short "
        "amounts of time. Clients should therefore not immediately bootstrap on "
        "receiving a disconnect."
    },
    {
        "tox_friend_request_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_friend_request_cb(public_key, message)\n"
        "This event is triggered when a friend request is received."
    },
    {
        "tox_friend_message_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_friend_message_cb(friend_number, type, message)\n"
        "This event is triggered when a message from a friend is received."
    },
    {
        "tox_friend_name_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_friend_name_cb(friend_number, name)\n"
        "This event is triggered when a friend changes their name."
    },
    {
        "tox_friend_status_message_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_friend_status_message_cb(friend_number, message)\n"
        "This event is triggered when a friend changes their status message."
    },
    {
        "tox_friend_status_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_friend_status_cb(friend_number, status)\n"
        "This event is triggered when a friend changes their user status."
    },
    {
        "tox_friend_read_receipt_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_friend_read_receipt_cb(friend_number, message_id)\n"
        "This event is triggered when the friend receives the message sent with "
        "tox_friend_send_message with the corresponding message ID."
    },
    {
        "tox_friend_connection_status_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_friend_connection_status_cb(friend_number, connection_status)\n"
        "This event is triggered when a friend goes offline after having been online, "
        "or when a friend goes online.\n"
        "This callback is not called when adding friends. It is assumed that when "
        "adding friends, their connection status is initially offline."
    },
    {
        "tox_friend_typing_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_friend_typing_cb(friend_number, is_typing)\n"
        "This event is triggered when a friend starts or stops typing."
    },
    {
        "tox_file_chunk_request_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_file_chunk_request_cb(friend_number, file_number, position, length)\n"
        "This event is triggered when Core is ready to send more file data."
    },
    {
        "tox_file_recv_control_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_file_recv_control_cb(friend_number, file_number, control)\n"
        "This event is triggered when a file control command is received from a friend."
    },
    {
        "tox_file_recv_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_file_recv_cb(friend_number, file_number, kind, file_size, filename)\n"
        "This event is triggered when a file transfer request is received."
    },
    {
        "tox_file_recv_chunk_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_file_recv_chunk_cb(friend_number, file_number, position, data, length)\n"
        "This event is first triggered when a file transfer request is received, and "
        "subsequently when a chunk of file data for an accepted request was received."
    },
    {
        "tox_friend_lossy_packet_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_friend_lossy_packet_cb(friend_number, data)\n"
        "This event is triggered when a friend sends lossy packet."
    },
    {
        "tox_friend_lossless_packet_cb", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
        "tox_friend_lossless_packet_cb(friend_number, data)\n"
        "This event is triggered when a friend sends lossless packet."
    },

    //
    // methods
    //

    {
        "tox_version_major", (PyCFunction)ToxCore_tox_version_major, METH_NOARGS | METH_STATIC,
        "tox_version_major()\n"
        "Return the major version number of the library. Can be used to display the "
        "Tox library version or to check whether the client is compatible with the "
        "dynamically linked version of Tox."
    },
    {
        "tox_version_minor", (PyCFunction)ToxCore_tox_version_minor, METH_NOARGS | METH_STATIC,
        "tox_version_minor()\n"
        "Return the minor version number of the library."
    },
    {
        "tox_version_patch", (PyCFunction)ToxCore_tox_version_patch, METH_NOARGS | METH_STATIC,
        "tox_version_patch()\n"
        "Return the patch number of the library."
    },
    {
        "tox_version_is_compatible", (PyCFunction)ToxCore_tox_version_is_compatible, METH_VARARGS | METH_STATIC,
        "tox_version_is_compatible(major, minor, patch)\n"
        "Return whether the compiled library version is compatible with the passed "
        "version numbers."
    },
    {
        "tox_options_default", (PyCFunction)ToxCore_tox_options_default, METH_VARARGS | METH_STATIC,
        "tox_options_default()\n"
        "Return Tox_Options object with the default options."
    },
    {
        "tox_kill", (PyCFunction)ToxCore_tox_kill, METH_NOARGS,
        "tox_kill()\n"
        "Releases all resources associated with the Tox instance and disconnects from "
        "the network.\n"
        "After calling this function, the Tox pointer becomes invalid. No other "
        "functions can be called, and the pointer value can no longer be read."

    },
    {
        "tox_get_savedata_size", (PyCFunction)ToxCore_tox_get_savedata_size, METH_NOARGS,
        "tox_get_savedata_size()\n"
        "Calculates the number of bytes required to store the tox instance with "
        "tox_get_savedata. This function cannot fail. The result is always greater than 0."

    },
    {
        "tox_get_savedata", (PyCFunction)ToxCore_tox_get_savedata, METH_NOARGS,
        "tox_get_savedata()\n"
        "Return all information associated with the tox instance."
    },
    {
        "tox_bootstrap", (PyCFunction)ToxCore_tox_bootstrap, METH_VARARGS,
        "tox_bootstrap(address, port, public_key)\n"
        "Sends a \"get nodes\" request to the given bootstrap node with IP, port, and "
        "public key to setup connections.\n"
        "This function will attempt to connect to the node using UDP. You must use "
        "this function even if Tox_Options.udp_enabled was set to false."
    },
    {
        "tox_add_tcp_relay", (PyCFunction)ToxCore_tox_add_tcp_relay, METH_VARARGS,
        "tox_add_tcp_relay(address, port, public_key)\n"
        "Adds additional host:port pair as TCP relay.\n"
        "This function can be used to initiate TCP connections to different ports on "
        "the same bootstrap node, or to add TCP relays without using them as "
        "bootstrap nodes."
    },
    {
        "tox_self_get_connection_status", (PyCFunction)ToxCore_tox_self_get_connection_status, METH_NOARGS,
        "tox_self_get_connection_status()\n"
        "Return whether we are connected to the DHT. The return value is equal to the "
        "last value received through the `self_connection_status` callback."
    },
    {
        "tox_self_get_address", (PyCFunction)ToxCore_tox_self_get_address, METH_NOARGS,
        "tox_self_get_address()\n"
        "Return address to give to others."
    },
    {   "tox_self_set_nospam", (PyCFunction)ToxCore_tox_self_set_nospam, METH_VARARGS,
        "tox_self_set_nospam(nospam)\n"
        "Set the 4-byte nospam part of the address."
    },
    {   "tox_self_get_nospam", (PyCFunction)ToxCore_tox_self_get_nospam, METH_NOARGS,
        "tox_self_get_nospam()\n"
        "Get the 4-byte nospam part of the address."
    },
    {   "tox_self_get_public_key", (PyCFunction)ToxCore_tox_self_get_public_key, METH_NOARGS,
        "tox_self_get_public_key()\n"
        "Return Tox Public Key (long term) from the Tox object."
    },
    {   "tox_self_get_secret_key", (PyCFunction)ToxCore_tox_self_get_secret_key, METH_NOARGS,
        "tox_self_get_secret_key()\n"
        "Return the Tox Secret Key from the Tox object."
    },
    {
        "tox_friend_add", (PyCFunction)ToxCore_tox_friend_add, METH_VARARGS,
        "tox_friend_add(address, message)\n"
        "Add a friend to the friend list and send a friend request.\n"
        "A friend request message must be at least 1 byte long and at most "
        "TOX_MAX_FRIEND_REQUEST_LENGTH.\n"
        "Friend numbers are unique identifiers used in all functions that operate on "
        "friends. Once added, a friend number is stable for the lifetime of the Tox "
        "object. After saving the state and reloading it, the friend numbers may not "
        "be the same as before. Deleting a friend creates a gap in the friend number "
        "set, which is filled by the next adding of a friend. Any pattern in friend "
        "numbers should not be relied on.\n"
        "If more than INT32_MAX friends are added, this function causes undefined "
        "behaviour."
    },
    {
        "tox_friend_add_norequest", (PyCFunction)ToxCore_tox_friend_add_norequest, METH_VARARGS,
        "friend_add_norequest(public_key)\n"
        "Add a friend without sending a friend request.\n"
        "This function is used to add a friend in response to a friend request. If the "
        "client receives a friend request, it can be reasonably sure that the other "
        "client added this client as a friend, eliminating the need for a friend "
        "request.\n"
        "This function is also useful in a situation where both instances are "
        "controlled by the same entity, so that this entity can perform the mutual "
        "friend adding. In this case, there is no need for a friend request, either."
    },
    {
        "tox_friend_delete", (PyCFunction)ToxCore_tox_friend_delete, METH_VARARGS,
        "tox_friend_delete(friend_number)\n"
        "Remove a friend from the friend list.\n"
        "This does not notify the friend of their deletion. After calling this "
        "function, this client will appear offline to the friend and no communication "
        "can occur between the two."
    },
    {
        "tox_friend_by_public_key", (PyCFunction)ToxCore_tox_friend_by_public_key, METH_VARARGS,
        "tox_friend_by_public_key(public_key)\n"
        "Return the friend number associated with that Public Key."
    },
    {
        "tox_friend_get_connection_status", (PyCFunction)ToxCore_tox_friend_get_connection_status, METH_VARARGS,
        "tox_friend_get_connection_status(friend_number)\n"
        "Check whether a friend is currently connected to this client.\n"
        "The result of this function is equal to the last value received by the "
        "friend_connection_status callback."
    },
    {
        "tox_friend_exists", (PyCFunction)ToxCore_tox_friend_exists, METH_VARARGS,
        "tox_friend_exists(friend_number)\n"
        "Checks if a friend with the given friend number exists and returns true if "
        "it does."
    },
    {
        "tox_friend_send_message", (PyCFunction)ToxCore_tox_friend_send_message, METH_VARARGS,
        "tox_friend_send_message(friend_number, type, message)\n"
        "Send a text chat message to an online friend.\n"
        "This function creates a chat message packet and pushes it into the send "
        "queue.\n"
        "The message length may not exceed TOX_MAX_MESSAGE_LENGTH. Larger messages "
        "must be split by the client and sent as separate messages. Other clients can "
        "then reassemble the fragments. Messages may not be empty.\n"
        "The return value of this function is the message ID. If a read receipt is "
        "received, the triggered `friend_read_receipt` event will be passed this message ID.\n"
        "Message IDs are unique per friend. The first message ID is 0. Message IDs are "
        "incremented by 1 each time a message is sent. If UINT32_MAX messages were "
        "sent, the next message ID is 0."
    },
    {
        "tox_self_set_name", (PyCFunction)ToxCore_tox_self_set_name, METH_VARARGS,
        "tox_self_set_name(name)\n"
        "Set the nickname for the Tox client.\n"
        "Nickname length cannot exceed TOX_MAX_NAME_LENGTH. If length is 0, the name "
        "parameter is ignored (it can be NULL), and the nickname is set back to empty."
    },
    {
        "tox_self_get_name", (PyCFunction)ToxCore_tox_self_get_name, METH_NOARGS,
        "tox_self_get_name()\n"
        "Return the nickname set by tox_self_set_name."
    },
    {
        "tox_friend_get_name", (PyCFunction)ToxCore_tox_friend_get_name, METH_VARARGS,
        "tox_friend_get_name(friend_number)\n"
        "Return the nickname of friend.\n"
        "The data written to name is equal to the data received by the last friend_name callback."
    },
    {
        "tox_self_set_status_message", (PyCFunction)ToxCore_tox_self_set_status_message, METH_VARARGS,
        "tox_self_set_status_message(message)\n"
        "Set the client's status message.\n"
        "Status message length cannot exceed TOX_MAX_STATUS_MESSAGE_LENGTH. If "
        "length is 0, the status parameter is ignored (it can be NULL), and the "
        "user status is set back to empty."
    },
    {
        "tox_self_set_status", (PyCFunction)ToxCore_tox_self_set_status, METH_VARARGS,
        "tox_self_set_status(status)."
    },
    {
        "tox_friend_get_status_message", (PyCFunction)ToxCore_tox_friend_get_status_message, METH_VARARGS,
        "tox_friend_get_status_message(friend_number)\n"
        "Get status message of a friend.\n"
        "The data written to status_message is equal to the data received by the last "
        "friend_status_message callback."
    },
    {
        "tox_self_get_status_message", (PyCFunction)ToxCore_tox_self_get_status_message, METH_NOARGS,
        "tox_self_get_status_message()\n"
        "Get status message of yourself."
    },
    {
        "tox_friend_get_status", (PyCFunction)ToxCore_tox_friend_get_status, METH_VARARGS,
        "tox_friend_get_status(friend_number)\n"
        "Return the friend's user status (away/busy/...).\n"
        "The status returned is equal to the last status received through the friend_status callback."
    },
    {
        "tox_friend_get_typing", (PyCFunction)ToxCore_tox_friend_get_typing, METH_VARARGS,
        "tox_friend_get_typing(friend_number)\n"
        "Check whether a friend is currently typing a message."
    },
    {
        "tox_self_set_typing", (PyCFunction)ToxCore_tox_self_set_typing, METH_VARARGS,
        "tox_self_set_typing(friend_number, typing)\n"
        "Set the client's typing status for a friend.\n"
        "The client is responsible for turning it on or off."
    },
    {
        "tox_self_get_status", (PyCFunction)ToxCore_tox_self_get_status, METH_NOARGS,
        "tox_self_get_status()\n"
        "Returns the client's user status."
    },
    {
        "tox_friend_get_last_online", (PyCFunction)ToxCore_tox_friend_get_last_online, METH_VARARGS,
        "tox_friend_get_last_online(friend_number)\n"
        "Return a unix-time timestamp of the last time the friend associated with a given "
        "friend number was seen online. This function will return UINT64_MAX on error."
    },
    {
        "tox_self_get_friend_list_size", (PyCFunction)ToxCore_tox_self_get_friend_list_size, METH_NOARGS,
        "tox_self_get_friend_list_size()\n"
        "Return the number of friends."
    },
    {
        "tox_self_get_friend_list", (PyCFunction)ToxCore_tox_self_get_friend_list, METH_NOARGS,
        "tox_self_get_friend_list()\n"
        "Get a list of valid friend numbers."
    },
    {
        "tox_friend_get_public_key", (PyCFunction)ToxCore_tox_friend_get_public_key, METH_VARARGS,
        "tox_friend_get_public_key(friend_number)\n"
        "Return the Public Key associated with a given friend number."
    },
    {
        "tox_file_send", (PyCFunction)ToxCore_tox_file_send, METH_VARARGS,
        "tox_file_send(friend_number, kind, file_size, file_id, filename)\n"
        "Send a file transmission request.\n"
        "Maximum filename length is TOX_MAX_FILENAME_LENGTH bytes. The filename "
        "should generally just be a file name, not a path with directory names.\n"
        "If a non-UINT64_MAX file size is provided, it can be used by both sides to "
        "determine the sending progress. File size can be set to UINT64_MAX for streaming "
        "data of unknown size.\n"
        "File transmission occurs in chunks, which are requested through the "
        "`file_chunk_request` event.\n"
        "When a friend goes offline, all file transfers associated with the friend are "
        "purged from core.\n"
        "If the file contents change during a transfer, the behaviour is unspecified "
        "in general."
    },
    {
        "tox_file_control", (PyCFunction)ToxCore_tox_file_control, METH_VARARGS,
        "tox_file_control(friend_number, file_number, control)\n"
        "Sends a file control command to a friend for a given file transfer."
    },
    {
        "tox_file_send_chunk", (PyCFunction)ToxCore_tox_file_send_chunk, METH_VARARGS,
        "tox_file_send_chunk(friend_number, file_number, position, data)\n"
        "Send a chunk of file data to a friend.\n"
        "This function is called in response to the `file_chunk_request` callback. The "
        "length parameter should be equal to the one received though the callback. "
        "If it is zero, the transfer is assumed complete. For files with known size, "
        "Core will know that the transfer is complete after the last byte has been "
        "received, so it is not necessary (though not harmful) to send a zero-length "
        "chunk to terminate. For streams, core will know that the transfer is finished "
        "if a chunk with length less than the length requested in the callback is sent."
    },
    {
        "tox_self_get_dht_id", (PyCFunction)ToxCore_tox_self_get_dht_id, METH_NOARGS,
        "tox_self_get_dht_id()\n"
        "Return the temporary DHT public key of this instance\n"
        "This can be used in combination with an externally accessible IP address and "
        "the bound port (from tox_self_get_udp_port) to run a temporary bootstrap node.\n"
        "Be aware that every time a new instance is created, the DHT public key "
        "changes, meaning this cannot be used to run a permanent bootstrap node."
    },
    {
        "tox_self_get_udp_port", (PyCFunction)ToxCore_tox_self_get_udp_port, METH_NOARGS,
        "tox_self_get_udp_port()\n"
        "Return the UDP port this Tox instance is bound to."
    },
    {
        "tox_self_get_tcp_port", (PyCFunction)ToxCore_tox_self_get_tcp_port, METH_NOARGS,
        "tox_self_get_tcp_port()\n"
        "Return the TCP port this Tox instance is bound to. This is only relevant if "
        "the instance is acting as a TCP relay."
    },
    {
        "tox_file_seek", (PyCFunction)ToxCore_tox_file_seek, METH_VARARGS,
        "tox_file_seek(friend_number, file_number, position)\n"
        "Sends a file seek control command to a friend for a given file transfer.\n"
        "This function can only be called to resume a file transfer right before "
        "TOX_FILE_CONTROL_RESUME is sent."
    },
    {
        "tox_file_get_file_id", (PyCFunction)ToxCore_tox_file_get_file_id, METH_VARARGS,
        "tox_file_get_file_id(friend_number, file_number)\n"
        "Return the file id associated to the file transfer."
    },
    {
        "tox_hash", (PyCFunction)ToxCore_tox_hash, METH_VARARGS | METH_STATIC,
        "tox_hash(data)\n"
        "Generates a cryptographic hash of the given data.\n"
        "This function may be used by clients for any purpose, but is provided "
        "primarily for validating cached avatars. This use is highly recommended to "
        "avoid unnecessary avatar updates."
    },
    {
        "tox_iteration_interval", (PyCFunction)ToxCore_tox_iteration_interval, METH_NOARGS,
        "tox_iteration_interval()\n"
        "Return the time in milliseconds before tox_iterate() should be called again "
        "for optimal performance."
    },
    {
        "tox_iterate", (PyCFunction)ToxCore_tox_iterate, METH_NOARGS,
        "tox_iterate()\n"
        "The main loop that needs to be run in intervals of tox_iteration_interval() "
        "milliseconds."
    },
    {
        "tox_friend_send_lossy_packet", (PyCFunction)ToxCore_tox_friend_send_lossy_packet, METH_VARARGS,
        "tox_friend_send_lossy_packet(friend_number, data)\n"
        "Send a custom lossy packet to a friend.\n"
        "The first byte of data must be in the range 200-254. Maximum length of a "
        "custom packet is TOX_MAX_CUSTOM_PACKET_SIZE.\n"
        "Lossy packets behave like UDP packets, meaning they might never reach the "
        "other side or might arrive more than once (if someone is messing with the "
        "connection) or might arrive in the wrong order.\n"
        "Unless latency is an issue, it is recommended that you use lossless custom "
        "packets instead."
    },
    {
        "tox_friend_send_lossless_packet", (PyCFunction)ToxCore_tox_friend_send_lossless_packet, METH_VARARGS,
        "tox_friend_send_lossless_packet(friend_number, data)\n"
        "Send a custom lossless packet to a friend.\n"
        "The first byte of data must be in the range 160-191. Maximum length of a "
        "custom packet is TOX_MAX_CUSTOM_PACKET_SIZE.\n"
        "Lossless packet behaviour is comparable to TCP (reliability, arrive in order) "
        "but with packets instead of a stream."
    },
    {
        NULL
    }
};
//----------------------------------------------------------------------------------------------

static void init_options(PyObject* pyopts, struct Tox_Options* tox_opts)
{
    char*      buf = NULL;
    Py_ssize_t sz  = 0;
    PyObject*  p   = NULL;
    PyObject*  key = NULL;

    key = PYSTRING_FromString("savedata_data");
    if (PyDict_Contains(pyopts, key) == true)
        p = PyObject_GetItem(pyopts, key);
    else
        p = NULL;
    Py_DECREF(key);
    if (p != NULL && p != Py_None) {
        PyBytes_AsStringAndSize(p, &buf, &sz);
        if (sz > 0) {
            tox_opts->savedata_data = calloc(1, sz);
            memcpy((void*)tox_opts->savedata_data, buf, sz);
            tox_opts->savedata_length = sz;
            tox_opts->savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
        }
    }
    Py_XDECREF(p);

    key = PYSTRING_FromString("proxy_host");
    if (PyDict_Contains(pyopts, key) == true)
        p = PyObject_GetItem(pyopts, key);
    else
        p = NULL;
    Py_DECREF(key);
    if (p != NULL && p != Py_None) {
        PyStringUnicode_AsStringAndSize(p, &buf, &sz);
        if (sz > 0) {
            tox_opts->proxy_host = calloc(1, sz);
            memcpy((void*)tox_opts->proxy_host, buf, sz);
        }
    }
    Py_XDECREF(p);

    key = PYSTRING_FromString("proxy_port");
    if (PyDict_Contains(pyopts, key) == true)
        p = PyObject_GetItem(pyopts, key);
    else
        p = NULL;
    Py_DECREF(key);
    if (p != NULL && p != Py_None)
        tox_opts->proxy_port = PyLong_AsLong(p);
    Py_XDECREF(p);

    key = PYSTRING_FromString("proxy_type");
    if (PyDict_Contains(pyopts, key) == true)
        p = PyObject_GetItem(pyopts, key);
    else
        p = NULL;
    Py_DECREF(key);
    if (p != NULL && p != Py_None)
        tox_opts->proxy_type = PyLong_AsLong(p);
    Py_XDECREF(p);

    key = PYSTRING_FromString("ipv6_enabled");
    if (PyDict_Contains(pyopts, key) == true)
        p = PyObject_GetItem(pyopts, key);
    else
        p = NULL;
    Py_DECREF(key);
    if (p != NULL && p != Py_None)
        tox_opts->ipv6_enabled = (p == Py_True);
    Py_XDECREF(p);

    key = PYSTRING_FromString("udp_enabled");
    if (PyDict_Contains(pyopts, key) == true)
        p = PyObject_GetItem(pyopts, key);
    else
        p = NULL;
    Py_DECREF(key);
    if (p != NULL && p != Py_None)
        tox_opts->udp_enabled = p == Py_True;
    Py_XDECREF(p);

    key = PYSTRING_FromString("start_port");
    if (PyDict_Contains(pyopts, key) == true)
        p = PyObject_GetItem(pyopts, key);
    else
        p = NULL;
    Py_DECREF(key);
    if (p != NULL && p != Py_None)
        tox_opts->start_port = PyLong_AsLong(p);
    Py_XDECREF(p);

    key = PYSTRING_FromString("end_port");
    if (PyDict_Contains(pyopts, key) == true)
        p = PyObject_GetItem(pyopts, key);
    else
        p = NULL;
    Py_DECREF(key);
    if (p != NULL && p != Py_None)
        tox_opts->end_port = PyLong_AsLong(p);
    Py_XDECREF(p);

    key = PYSTRING_FromString("tcp_port");
    if (PyDict_Contains(pyopts, key) == true)
        p = PyObject_GetItem(pyopts, key);
    else
        p = NULL;
    Py_DECREF(key);
    if (p != NULL && p != Py_None)
        tox_opts->tcp_port = PyLong_AsLong(p);
    Py_XDECREF(p);
}
//----------------------------------------------------------------------------------------------

static int init_helper(ToxCore* self, PyObject* args)
{
    ToxCore_tox_kill(self, NULL);

    PyObject* pyopts = NULL;

    if (args != NULL && PyTuple_Size(args) == 1 && PyArg_ParseTuple(args, "O", &pyopts) == false)
        return -1;

    struct Tox_Options options = { 0 };
    tox_options_default(&options);

    if (pyopts != NULL) {
        if (PyDict_Check(pyopts) == true)
            init_options(pyopts, &options);
        else if (pyopts != Py_None) {
            PyErr_SetString(ToxCoreException, "You must supply a Tox_Options param as a dict.");
            return -1;
        }
    }

    TOX_ERR_NEW error = 0;
    Tox* tox = tox_new(&options, &error);

    bool success = false;
    switch (error) {
        case TOX_ERR_NEW_OK:
            success = true;
            break;
        case TOX_ERR_NEW_NULL:
            PyErr_SetString(ToxCoreException, "One of the arguments to the function was NULL when it was not expected.");
            break;
        case TOX_ERR_NEW_MALLOC:
            PyErr_SetString(ToxCoreException, "The function was unable to allocate enough memory to store the internal structures for the Tox object.");
            break;
        case TOX_ERR_NEW_PORT_ALLOC:
            PyErr_SetString(ToxCoreException, "The function was unable to bind to a port. This may mean that all ports have already been bound, e.g. by other Tox instances, or it may mean a permission error. You may be able to gather more information from errno.");
            break;
        case TOX_ERR_NEW_PROXY_BAD_TYPE:
            PyErr_SetString(ToxCoreException, "proxy_type was invalid.");
            break;
        case TOX_ERR_NEW_PROXY_BAD_HOST:
            PyErr_SetString(ToxCoreException, "proxy_type was valid but the proxy_host passed had an invalid format or was NULL.");
            break;
        case TOX_ERR_NEW_PROXY_BAD_PORT:
            PyErr_SetString(ToxCoreException, "proxy_type was valid, but the proxy_port was invalid.");
            break;
        case TOX_ERR_NEW_PROXY_NOT_FOUND:
            PyErr_SetString(ToxCoreException, "The proxy address passed could not be resolved.");
            break;
        case TOX_ERR_NEW_LOAD_ENCRYPTED:
            PyErr_SetString(ToxCoreException, "The byte array to be loaded contained an encrypted save.");
            break;
        case TOX_ERR_NEW_LOAD_BAD_FORMAT:
            PyErr_SetString(ToxCoreException, "The data format was invalid. This can happen when loading data that was saved by an older version of Tox, or when the data has been corrupted. When loading from badly formatted data, some data may have been loaded, and the rest is discarded. Passing an invalid length parameter also causes this error.");
            break;
    }

    if (tox == NULL || success == false)
        return -1;

    tox_callback_self_connection_status(tox, callback_self_connection_status, self);
    tox_callback_friend_request(tox, callback_friend_request, self);
    tox_callback_friend_message(tox, callback_friend_message, self);
    tox_callback_friend_name(tox, callback_friend_name, self);
    tox_callback_friend_status_message(tox, callback_friend_status_message, self);
    tox_callback_friend_status(tox, callback_friend_status, self);
    tox_callback_friend_read_receipt(tox, callback_friend_read_receipt, self);
    tox_callback_friend_connection_status(tox, callback_friend_connection_status, self);
    tox_callback_friend_typing(tox, callback_friend_typing, self);
    tox_callback_file_chunk_request(tox, callback_file_chunk_request, self);
    tox_callback_file_recv_control(tox, callback_file_recv_control, self);
    tox_callback_file_recv(tox, callback_file_recv, self);
    tox_callback_file_recv_chunk(tox, callback_file_recv_chunk, self);
    tox_callback_friend_lossy_packet(tox, callback_friend_lossy_packet, self);
    tox_callback_friend_lossless_packet(tox, callback_friend_lossless_packet, self);

    self->tox = tox;

    return 0;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxCore_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    ToxCore* self = (ToxCore*)type->tp_alloc(type, 0);

    self->tox = NULL;

    if (init_helper(self, NULL) == -1)
        return NULL;

    return (PyObject*)self;
}
//----------------------------------------------------------------------------------------------

static int ToxCore_init(ToxCore* self, PyObject* args, PyObject* kwds)
{
    return init_helper(self, args);
}
//----------------------------------------------------------------------------------------------

static int ToxCore_dealloc(ToxCore* self)
{
    ToxCore_tox_kill(self, NULL);

    return 0;
}
//----------------------------------------------------------------------------------------------

PyTypeObject ToxCoreType = {
#if PY_MAJOR_VERSION >= 3
    PyVarObject_HEAD_INIT(NULL, 0)
#else
    PyObject_HEAD_INIT(NULL)
    0,                                          /* ob_size           */
#endif
    "ToxCore",                                  /* tp_name           */
    sizeof(ToxCore),                            /* tp_basicsize      */
    0,                                          /* tp_itemsize       */
    (destructor)ToxCore_dealloc,                /* tp_dealloc        */
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
    "ToxCore object",                           /* tp_doc            */
    0,                                          /* tp_traverse       */
    0,                                          /* tp_clear          */
    0,                                          /* tp_richcompare    */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter           */
    0,                                          /* tp_iternext       */
    ToxCore_methods,                            /* tp_methods        */
    0,                                          /* tp_members        */
    0,                                          /* tp_getset         */
    0,                                          /* tp_base           */
    0,                                          /* tp_dict           */
    0,                                          /* tp_descr_get      */
    0,                                          /* tp_descr_set      */
    0,                                          /* tp_dictoffset     */
    (initproc)ToxCore_init,                     /* tp_init           */
    0,                                          /* tp_alloc          */
    ToxCore_new,                                /* tp_new            */
};
//----------------------------------------------------------------------------------------------

void ToxCore_install_dict(void)
{
#define SET(name)                                  \
    PyObject* obj_##name = PyLong_FromLong(name);  \
    PyDict_SetItemString(dict, #name, obj_##name); \
    Py_DECREF(obj_##name)

    PyObject* dict = PyDict_New();
    if (dict == NULL)
        return;

    // #define TOX_VERSION_MAJOR
    SET(TOX_VERSION_MAJOR);
    // #define TOX_VERSION_MINOR
    SET(TOX_VERSION_MINOR);
    // #define TOX_VERSION_PATCH
    SET(TOX_VERSION_PATCH);

    // #define TOX_PUBLIC_KEY_SIZE
    SET(TOX_PUBLIC_KEY_SIZE);
    // #define TOX_SECRET_KEY_SIZE
    SET(TOX_SECRET_KEY_SIZE);
    // #define TOX_ADDRESS_SIZE
    SET(TOX_ADDRESS_SIZE);

    // #define TOX_MAX_NAME_LENGTH
    SET(TOX_MAX_NAME_LENGTH);
    // #define TOX_MAX_STATUS_MESSAGE_LENGTH
    SET(TOX_MAX_STATUS_MESSAGE_LENGTH);
    // #define TOX_MAX_FRIEND_REQUEST_LENGTH
    SET(TOX_MAX_FRIEND_REQUEST_LENGTH);
    // #define TOX_MAX_MESSAGE_LENGTH
    SET(TOX_MAX_MESSAGE_LENGTH);
    // #define TOX_MAX_CUSTOM_PACKET_SIZE
    SET(TOX_MAX_CUSTOM_PACKET_SIZE);
    // #define TOX_HASH_LENGTH
    SET(TOX_HASH_LENGTH);
    // #define TOX_FILE_ID_LENGTH
    SET(TOX_FILE_ID_LENGTH);
    // #define TOX_MAX_FILENAME_LENGTH
    SET(TOX_MAX_FILENAME_LENGTH);

    // enum TOX_USER_STATUS
    SET(TOX_USER_STATUS_NONE);
    SET(TOX_USER_STATUS_AWAY);
    SET(TOX_USER_STATUS_BUSY);

    // enum TOX_MESSAGE_TYPE
    SET(TOX_MESSAGE_TYPE_NORMAL);
    SET(TOX_MESSAGE_TYPE_ACTION);

    // enum TOX_PROXY_TYPE
    SET(TOX_PROXY_TYPE_NONE);
    SET(TOX_PROXY_TYPE_HTTP);
    SET(TOX_PROXY_TYPE_SOCKS5);

    // enum TOX_CONNECTION
    SET(TOX_CONNECTION_NONE);
    SET(TOX_CONNECTION_TCP);
    SET(TOX_CONNECTION_UDP);

    // enum TOX_FILE_KIND
    SET(TOX_FILE_KIND_DATA);
    SET(TOX_FILE_KIND_AVATAR);

    // enum TOX_FILE_CONTROL
    SET(TOX_FILE_CONTROL_RESUME);
    SET(TOX_FILE_CONTROL_PAUSE);
    SET(TOX_FILE_CONTROL_CANCEL);

#undef SET

    ToxCoreType.tp_dict = dict;
}
//----------------------------------------------------------------------------------------------
