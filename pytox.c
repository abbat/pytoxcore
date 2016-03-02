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
#include "pytoxav.h"
#include "pytoxdns.h"
//----------------------------------------------------------------------------------------------

void bytes_to_hex_string(const uint8_t* digest, int length, uint8_t* hex_digest)
{
    hex_digest[2 * length] = 0;

    int i;
    int j;
    for (i = j = 0; i < length; i++) {
        char c;
        c = (digest[i] >> 4) & 0xF;
        c = (c > 9) ? c + 'A'- 10 : c + '0';
        hex_digest[j++] = c;
        c = (digest[i] & 0xF);
        c = (c > 9) ? c + 'A' - 10 : c + '0';
        hex_digest[j++] = c;
    }
}
//----------------------------------------------------------------------------------------------

static int hex_char_to_int(char c)
{
    int val = 0;
    if (c >= '0' && c <= '9')
        val = c - '0';
    else if (c >= 'A' && c <= 'F')
        val = c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
        val = c - 'a' + 10;
    else
        val = 0;

    return val;
}
//----------------------------------------------------------------------------------------------

void hex_string_to_bytes(uint8_t* hexstr, int length, uint8_t* bytes)
{
    int i;
    for (i = 0; i < length; i++)
        bytes[i] = (hex_char_to_int(hexstr[2 * i]) << 4) | (hex_char_to_int(hexstr[2 * i + 1]));
}
//----------------------------------------------------------------------------------------------

PyObject* PyNone_New(void)
{
    Py_INCREF(Py_None);
    return Py_None;
}
//----------------------------------------------------------------------------------------------

#if PY_MAJOR_VERSION >= 3
struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "pytoxcore",
    "Python binding for ToxCore",
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif
//----------------------------------------------------------------------------------------------

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_pytoxcore(void)
{
    PyObject* module = PyModule_Create(&moduledef);
#else
PyMODINIT_FUNC initpytoxcore(void)
{
    PyObject* module = Py_InitModule("pytoxcore", NULL);
#endif

    if (module == NULL)
        goto error;

    if (sodium_init() == -1)
        goto error;

    //
    // initialize pytoxcore
    //

    ToxCore_install_dict();

    if (PyType_Ready(&ToxCoreType) < 0) {
        fprintf(stderr, "Invalid PyTypeObject 'ToxCoreType'\n");
        goto error;
    }

    Py_INCREF(&ToxCoreType);
    PyModule_AddObject(module, "ToxCore", (PyObject*)&ToxCoreType);

    ToxCoreException = PyErr_NewException("pytoxcore.ToxCoreException", NULL, NULL);
    PyModule_AddObject(module, "ToxCoreException", (PyObject*)ToxCoreException);

    //
    // initialize pytoxav
    //

    ToxAV_install_dict();

    if (PyType_Ready(&ToxAVType) < 0) {
        fprintf(stderr, "Invalid PyTypeObject 'ToxAVType'\n");
        goto error;
    }

    Py_INCREF(&ToxAVType);
    PyModule_AddObject(module, "ToxAV", (PyObject*)&ToxAVType);

    ToxAVException = PyErr_NewException("pytoxcore.ToxAVException", NULL, NULL);
    PyModule_AddObject(module, "ToxAVException", (PyObject*)ToxAVException);

    //
    // initialize pytoxdns
    //

    ToxDNS_install_dict();

    if (PyType_Ready(&ToxDNSType) < 0) {
        fprintf(stderr, "Invalid PyTypeObject 'ToxDNSType'\n");
        goto error;
    }

    Py_INCREF(&ToxDNSType);
    PyModule_AddObject(module, "ToxDNS", (PyObject*)&ToxDNSType);

    ToxDNSException = PyErr_NewException("pytoxcore.ToxDNSException", NULL, NULL);
    PyModule_AddObject(module, "ToxDNSException", (PyObject*)ToxDNSException);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif

error:
#if PY_MAJOR_VERSION >= 3
    return NULL;
#else
    return;
#endif
}
//----------------------------------------------------------------------------------------------
