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
#define CHECK_TOXAV(self)                                        \
    if ((self)->av == NULL) {                                    \
        PyErr_SetString(ToxAVException, "toxav object killed."); \
        return NULL;                                             \
    }
//----------------------------------------------------------------------------------------------
PyObject* ToxAVException;
//----------------------------------------------------------------------------------------------

inline static uint8_t rgb_to_y(int r, int g, int b)
{
    int y = ((9798 * r + 19235 * g + 3736 * b) >> 15);
    return y > 255 ? 255 : y < 0 ? 0 : y;
}
//----------------------------------------------------------------------------------------------

inline static uint8_t rgb_to_u(int r, int g, int b)
{
    int u = ((-5538 * r + -10846 * g + 16351 * b) >> 15) + 128;
    return u > 255 ? 255 : u < 0 ? 0 : u;
}
//----------------------------------------------------------------------------------------------

inline static uint8_t rgb_to_v(int r, int g, int b)
{
    int v = ((16351 * r + -13697 * g + -2664 * b) >> 15) + 128;
    return v > 255 ? 255 : v < 0 ? 0 : v;
}
//----------------------------------------------------------------------------------------------

static void bgr_to_yuv420(uint8_t* plane_y, uint8_t* plane_u, uint8_t* plane_v, uint8_t* bgr, uint16_t width, uint16_t height)
{
    uint16_t x;
    uint16_t y;
    uint8_t* p;
    uint8_t  b;
    uint8_t  g;
    uint8_t  r;

    for (y = 0; y != height; y += 2) {
        p = bgr;
        for (x = 0; x != width; x++) {
            b = *bgr++;
            g = *bgr++;
            r = *bgr++;

            *plane_y++ = rgb_to_y(r, g, b);
        }

        for (x = 0; x != width / 2; x++) {
            b = *bgr++;
            g = *bgr++;
            r = *bgr++;

            *plane_y++ = rgb_to_y(r, g, b);

            b = *bgr++;
            g = *bgr++;
            r = *bgr++;

            *plane_y++ = rgb_to_y(r, g, b);

            b = ((int)b + (int)*(bgr - 6) + (int)*p + (int)*(p + 3) + 2) / 4; p++;
            g = ((int)g + (int)*(bgr - 5) + (int)*p + (int)*(p + 3) + 2) / 4; p++;
            r = ((int)r + (int)*(bgr - 4) + (int)*p + (int)*(p + 3) + 2) / 4; p++;

            *plane_u++ = rgb_to_u(r, g, b);
            *plane_v++ = rgb_to_v(r, g, b);

            p += 3;
        }
    }
}
//----------------------------------------------------------------------------------------------

static void rgb_to_yuv420(uint8_t* plane_y, uint8_t* plane_u, uint8_t* plane_v, uint8_t* rgb, uint16_t width, uint16_t height)
{
    uint16_t x;
    uint16_t y;
    uint8_t* p;
    uint8_t  r;
    uint8_t  g;
    uint8_t  b;

    for (y = 0; y != height; y += 2) {
        p = rgb;
        for (x = 0; x != width; x++) {
            r = *rgb++;
            g = *rgb++;
            b = *rgb++;

            *plane_y++ = rgb_to_y(r, g, b);
        }

        for (x = 0; x != width / 2; x++) {
            r = *rgb++;
            g = *rgb++;
            b = *rgb++;

            *plane_y++ = rgb_to_y(r, g, b);

            r = *rgb++;
            g = *rgb++;
            b = *rgb++;

            *plane_y++ = rgb_to_y(r, g, b);

            r = ((int)b + (int)*(rgb - 6) + (int)*p + (int)*(p + 3) + 2) / 4; p++;
            g = ((int)g + (int)*(rgb - 5) + (int)*p + (int)*(p + 3) + 2) / 4; p++;
            b = ((int)r + (int)*(rgb - 4) + (int)*p + (int)*(p + 3) + 2) / 4; p++;

            *plane_u++ = rgb_to_u(r, g, b);
            *plane_v++ = rgb_to_v(r, g, b);

            p += 3;
        }
    }
}
//----------------------------------------------------------------------------------------------

static void yuv420_to_bgr(uint16_t width, uint16_t height, const uint8_t* y, const uint8_t* u, const uint8_t* v, unsigned int ystride, unsigned int ustride, unsigned int vstride, uint8_t* bgr)
{
    unsigned long int i;
    unsigned long int j;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            uint8_t* point = bgr + 3 * ((i * width) + j);

            int t_y = y[((i * ystride) + j)];
            int t_u = u[(((i / 2) * ustride) + (j / 2))];
            int t_v = v[(((i / 2) * vstride) + (j / 2))];

            t_y = t_y < 16 ? 16 : t_y;

            int b = (298 * (t_y - 16) + 516 * (t_u - 128) + 128) >> 8;
            int g = (298 * (t_y - 16) - 100 * (t_u - 128) - 208 * (t_v - 128) + 128) >> 8;
            int r = (298 * (t_y - 16) + 409 * (t_v - 128) + 128) >> 8;

            point[0] = b > 255 ? 255 : b < 0 ? 0 : b;
            point[1] = g > 255 ? 255 : g < 0 ? 0 : g;
            point[2] = r > 255 ? 255 : r < 0 ? 0 : r;
        }
    }
}
//----------------------------------------------------------------------------------------------

static void yuv420_to_rgb(uint16_t width, uint16_t height, const uint8_t* y, const uint8_t* u, const uint8_t* v, unsigned int ystride, unsigned int ustride, unsigned int vstride, uint8_t* rgb)
{
    unsigned long int i;
    unsigned long int j;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            uint8_t* point = rgb + 3 * ((i * width) + j);

            int t_y = y[((i * ystride) + j)];
            int t_u = u[(((i / 2) * ustride) + (j / 2))];
            int t_v = v[(((i / 2) * vstride) + (j / 2))];

            t_y = t_y < 16 ? 16 : t_y;

            int r = (298 * (t_y - 16) + 409 * (t_v - 128) + 128) >> 8;
            int g = (298 * (t_y - 16) - 100 * (t_u - 128) - 208 * (t_v - 128) + 128) >> 8;
            int b = (298 * (t_y - 16) + 516 * (t_u - 128) + 128) >> 8;

            point[0] = r > 255 ? 255 : r < 0 ? 0 : r;
            point[1] = g > 255 ? 255 : g < 0 ? 0 : g;
            point[2] = b > 255 ? 255 : b < 0 ? 0 : b;
        }
    }
}
//----------------------------------------------------------------------------------------------

inline static vpx_image_t* vpx_image_realloc(vpx_image_t* image, uint32_t width, uint32_t height)
{
    if (image == NULL || image->d_w != width || image->d_h != height) {
        vpx_img_free(image);
        return vpx_img_alloc(NULL, VPX_IMG_FMT_I420, width, height, 1);
    }

    return image;
}
//----------------------------------------------------------------------------------------------

static void callback_call(ToxAV* av, uint32_t friend_number, bool audio_enabled, bool video_enabled, void* self)
{
    PyObject_CallMethod((PyObject*)self, "toxav_call_cb", "III", friend_number, audio_enabled, video_enabled);
}
//----------------------------------------------------------------------------------------------

static void callback_call_state(ToxAV* av, uint32_t friend_number, uint32_t state, void* self)
{
    PyObject_CallMethod((PyObject*)self, "toxav_call_state_cb", "II", friend_number, state);
}
//----------------------------------------------------------------------------------------------

static void callback_bit_rate_status(ToxAV* av, uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate, void* self)
{
    PyObject_CallMethod((PyObject*)self, "toxav_bit_rate_status_cb", "III", friend_number, audio_bit_rate, video_bit_rate);
}
//----------------------------------------------------------------------------------------------

static void callback_audio_receive_frame(ToxAV* av, uint32_t friend_number, const int16_t* pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate, void* self)
{
    size_t pcm_len = sample_count * channels * 2;

    PyObject_CallMethod((PyObject*)self, "toxav_audio_receive_frame_cb", "I" BUF_TCS "KII", friend_number, pcm, pcm_len, sample_count, channels, sampling_rate);
}
//----------------------------------------------------------------------------------------------

static void callback_video_receive_frame(ToxAV* av, uint32_t friend_number, uint16_t width, uint16_t height, const uint8_t* y, const uint8_t* u, const uint8_t* v, int32_t ystride, int32_t ustride, int32_t vstride, void* self)
{
    uint8_t* rgb = malloc(width * height * 3);
    if (rgb == NULL)
        return;

    // TODO: Choose bgr / rbg
    ystride = abs(ystride);
    ustride = abs(ustride);
    vstride = abs(vstride);

    yuv420_to_bgr(width, height, y, u, v, ystride, ustride, vstride, rgb);

    PyObject_CallMethod((PyObject*)self, "toxav_video_receive_frame_cb", "III" BUF_TCS, friend_number, width, height, rgb);

    free(rgb);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_callback_stub(ToxCoreAV* self, PyObject* args)
{
    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_version_major(ToxCoreAV* self, PyObject* args)
{
    uint32_t result = toxav_version_major();

    return PyLong_FromUnsignedLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_version_minor(ToxCoreAV* self, PyObject* args)
{
    uint32_t result = toxav_version_minor();

    return PyLong_FromUnsignedLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_version_patch(ToxCoreAV* self, PyObject* args)
{
    uint32_t result = toxav_version_patch();

    return PyLong_FromUnsignedLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_version_is_compatible(ToxCoreAV* self, PyObject* args)
{
    uint32_t major;
    uint32_t minor;
    uint32_t patch;

    if (PyArg_ParseTuple(args, "III", &major, &minor, &patch) == false)
        return NULL;

    bool result = toxav_version_is_compatible(major, minor, patch);

    return PyBool_FromLong(result);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_kill(ToxCoreAV* self, PyObject* args)
{
    if (self->av != NULL) {
        toxav_kill(self->av);
        self->av = NULL;
    }

    if (self->core != NULL) {
        Py_DECREF(self->core);
        self->core = NULL;
    }

    if (self->frame != NULL) {
        vpx_img_free(self->frame);
        self->frame = NULL;
    }

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_iteration_interval(ToxCoreAV* self, PyObject* args)
{
    CHECK_TOXAV(self);

    uint32_t interval = toxav_iteration_interval(self->av);

    return PyLong_FromUnsignedLong(interval);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_iterate(ToxCoreAV* self, PyObject* args)
{
    CHECK_TOXAV(self);

    toxav_iterate(self->av);

    if (PyErr_Occurred() != NULL)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_call(ToxCoreAV* self, PyObject* args)
{
    CHECK_TOXAV(self);

    uint32_t friend_number;
    uint32_t audio_bit_rate;
    uint32_t video_bit_rate;

    if (PyArg_ParseTuple(args, "III", &friend_number, &audio_bit_rate, &video_bit_rate) == false)
        return NULL;

    TOXAV_ERR_CALL error;
    bool result = toxav_call(self->av, friend_number, audio_bit_rate, video_bit_rate, &error);

    bool success = false;
    switch (error) {
        case TOXAV_ERR_CALL_OK:
            success = true;
            break;
        case TOXAV_ERR_CALL_MALLOC:
            PyErr_SetString(ToxAVException, "A resource allocation error occurred while trying to create the structures required for the call.");
            break;
        case TOXAV_ERR_CALL_SYNC:
            PyErr_SetString(ToxAVException, "Synchronization error occurred.");
            break;
        case TOXAV_ERR_CALL_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxAVException, "The friend number did not designate a valid friend.");
            break;
        case TOXAV_ERR_CALL_FRIEND_NOT_CONNECTED:
            PyErr_SetString(ToxAVException, "The friend was valid, but not currently connected.");
            break;
        case TOXAV_ERR_CALL_FRIEND_ALREADY_IN_CALL:
            PyErr_SetString(ToxAVException, "Attempted to call a friend while already in an audio or video call with them.");
            break;
        case TOXAV_ERR_CALL_INVALID_BIT_RATE:
            PyErr_SetString(ToxAVException, "Audio or video bit rate is invalid.");
            break;
    };

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_answer(ToxCoreAV* self, PyObject* args)
{
    CHECK_TOXAV(self);

    uint32_t friend_number;
    uint32_t audio_bit_rate;
    uint32_t video_bit_rate;

    if (PyArg_ParseTuple(args, "III", &friend_number, &audio_bit_rate, &video_bit_rate) == false)
        return NULL;

    TOXAV_ERR_ANSWER error;
    bool result = toxav_answer(self->av, friend_number, audio_bit_rate, video_bit_rate, &error);

    bool success = false;
    switch (error) {
        case TOXAV_ERR_ANSWER_OK:
            success = true;
            break;
        case TOXAV_ERR_ANSWER_SYNC:
            PyErr_SetString(ToxAVException, "Synchronization error occurred.");
            break;
        case TOXAV_ERR_ANSWER_CODEC_INITIALIZATION:
            PyErr_SetString(ToxAVException, "Failed to initialize codecs for call session. Note that codec initiation will fail if there is no receive callback registered for either audio or video.");
            break;
        case TOXAV_ERR_ANSWER_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxAVException, "The friend number did not designate a valid friend.");
            break;
        case TOXAV_ERR_ANSWER_FRIEND_NOT_CALLING:
            PyErr_SetString(ToxAVException, "The friend was valid, but they are not currently trying to initiate a call. This is also returned if this client is already in a call with the friend.");
            break;
        case TOXAV_ERR_ANSWER_INVALID_BIT_RATE:
            PyErr_SetString(ToxAVException, "Audio or video bit rate is invalid.");
            break;
    };

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_call_control(ToxCoreAV* self, PyObject* args)
{
    CHECK_TOXAV(self);

    uint32_t           friend_number;
    TOXAV_CALL_CONTROL control;

    if (PyArg_ParseTuple(args, "II", &friend_number, &control) == false)
        return NULL;

    TOXAV_ERR_CALL_CONTROL error;
    bool result = toxav_call_control(self->av, friend_number, control, &error);

    bool success = false;
    switch (error) {
        case TOXAV_ERR_CALL_CONTROL_OK:
            success = true;
            break;
        case TOXAV_ERR_CALL_CONTROL_SYNC:
            PyErr_SetString(ToxAVException, "Synchronization error occurred.");
            break;
        case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxAVException, "The friend_number passed did not designate a valid friend.");
            break;
        case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_IN_CALL:
            PyErr_SetString(ToxAVException, "This client is currently not in a call with the friend. Before the call is answered, only CANCEL is a valid control.");
            break;
        case TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION:
            PyErr_SetString(ToxAVException, "Happens if user tried to pause an already paused call or if trying to resume a call that is not paused.");
            break;
    };

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_bit_rate_set(ToxCoreAV* self, PyObject* args)
{
    CHECK_TOXAV(self);

    uint32_t friend_number;
    uint32_t audio_bit_rate;
    uint32_t video_bit_rate;

    if (PyArg_ParseTuple(args, "III", &friend_number, &audio_bit_rate, &video_bit_rate) == false)
        return NULL;

    TOXAV_ERR_BIT_RATE_SET error;
    bool result = toxav_bit_rate_set(self->av, friend_number, audio_bit_rate, video_bit_rate, &error);

    bool success = false;
    switch (error) {
        case TOXAV_ERR_BIT_RATE_SET_OK:
            success = true;
            break;
        case TOXAV_ERR_BIT_RATE_SET_SYNC:
            PyErr_SetString(ToxAVException, "Synchronization error occurred.");
            break;
        case TOXAV_ERR_BIT_RATE_SET_INVALID_AUDIO_BIT_RATE:
            PyErr_SetString(ToxAVException, "The audio bit rate passed was not one of the supported values.");
            break;
        case TOXAV_ERR_BIT_RATE_SET_INVALID_VIDEO_BIT_RATE:
            PyErr_SetString(ToxAVException, "The video bit rate passed was not one of the supported values.");
            break;
        case TOXAV_ERR_BIT_RATE_SET_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxAVException, "The friend_number passed did not designate a valid friend.");
            break;
        case TOXAV_ERR_BIT_RATE_SET_FRIEND_NOT_IN_CALL:
            PyErr_SetString(ToxAVException, "This client is currently not in a call with the friend.");
            break;
    };

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* parse_TOXAV_ERR_SEND_FRAME(bool result, TOXAV_ERR_SEND_FRAME error)
{
    bool success = false;
    switch (error) {
        case TOXAV_ERR_SEND_FRAME_OK:
            success = true;
            break;
        case TOXAV_ERR_SEND_FRAME_NULL:
            PyErr_SetString(ToxAVException, "In case of video, one of Y, U, or V was NULL. In case of audio, the samples data pointer was NULL.");
            break;
        case TOXAV_ERR_SEND_FRAME_FRIEND_NOT_FOUND:
            PyErr_SetString(ToxAVException, "The friend_number passed did not designate a valid friend.");
            break;
        case TOXAV_ERR_SEND_FRAME_FRIEND_NOT_IN_CALL:
            PyErr_SetString(ToxAVException, "This client is currently not in a call with the friend.");
            break;
        case TOXAV_ERR_SEND_FRAME_SYNC:
            PyErr_SetString(ToxAVException, "Synchronization error occurred.");
            break;
        case TOXAV_ERR_SEND_FRAME_INVALID:
            PyErr_SetString(ToxAVException, "One of the frame parameters was invalid. E.g. the resolution may be too small or too large, or the audio sampling rate may be unsupported.");
            break;
        case TOXAV_ERR_SEND_FRAME_PAYLOAD_TYPE_DISABLED:
            PyErr_SetString(ToxAVException, "Either friend turned off audio or video receiving or we turned off sending for the said payload.");
            break;
        case TOXAV_ERR_SEND_FRAME_RTP_FAILED:
            PyErr_SetString(ToxAVException, "Failed to push frame through rtp interface.");
            break;
    };

    if (result == false || success == false)
        return NULL;

    Py_RETURN_NONE;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_audio_send_frame(ToxCoreAV* self, PyObject* args)
{
    CHECK_TOXAV(self);

    uint32_t   friend_number;
    uint8_t*   pcm;
    Py_ssize_t pcm_len;
    size_t     sample_count;
    uint8_t    channels;
    uint32_t   sampling_rate;

    if (PyArg_ParseTuple(args, "Is#KBI", &friend_number, &pcm, &pcm_len, &sample_count, &channels, &sampling_rate) == false)
        return NULL;

    if (!(channels == 1 || channels == 2)) {
        PyErr_SetString(ToxAVException, "Invalid channels count - supported values are 1 and 2.");
        return NULL;
    }

    if (!(sampling_rate == 8000 || sampling_rate == 12000 || sampling_rate == 16000 || sampling_rate == 24000 || sampling_rate == 48000)) {
        PyErr_SetString(ToxAVException, "Invalid sampling_rate - supported values are 8K, 12K, 16K, 24K and 48K.");
        return NULL;
    }

    uint32_t sampling_rate_ms = sampling_rate / 1000;
    if (!(sample_count == sampling_rate_ms * 2 + sampling_rate_ms / 2 ||
          sample_count == sampling_rate_ms * 5  ||
          sample_count == sampling_rate_ms * 10 ||
          sample_count == sampling_rate_ms * 20 ||
          sample_count == sampling_rate_ms * 40 ||
          sample_count == sampling_rate_ms * 60)) {
        PyErr_SetString(ToxAVException, "Invalid sample_count - supported values are 2.5, 5, 10, 20, 40 and 60 ms of data per frame.");
    }

    if (pcm_len != sample_count * channels * 2) {
        PyErr_SetString(ToxAVException, "Invalid pcm size - must be sample_count * channels * 2 bytes.");
        return NULL;
    }

    TOXAV_ERR_SEND_FRAME error;
    bool result = toxav_audio_send_frame(self->av, friend_number, (int16_t*)pcm, sample_count, channels, sampling_rate, &error);

    return parse_TOXAV_ERR_SEND_FRAME(result, error);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_video_send_yuv420_frame(ToxCoreAV* self, PyObject* args)
{
    CHECK_TOXAV(self);

    uint32_t   friend_number;
    uint32_t   width;
    uint32_t   height;
    uint8_t*   y;
    Py_ssize_t y_len;
    uint8_t*   u;
    Py_ssize_t u_len;
    uint8_t*   v;
    Py_ssize_t v_len;

    if (PyArg_ParseTuple(args, "IIIs#s#s#", &friend_number, &width, &height, &y, &y_len, &u, &u_len, &v, &v_len) == false)
        return NULL;

    if (width < 1 || width > 0xFFFF) {
        PyErr_SetString(ToxAVException, "Invalid width - must be 16 bit unsigned.");
        return NULL;
    }

    if (height < 1 || height > 0xFFFF) {
        PyErr_SetString(ToxAVException, "Invalid height - must be 16 bit unsigned.");
        return NULL;
    }

    if (y_len != height * width) {
        PyErr_SetString(ToxAVException, "Invalid Y-plane size - must be height * width.");
        return NULL;
    }

    if (u_len != (height / 2) * (width / 2)) {
        PyErr_SetString(ToxAVException, "Invalid U-plane size - must be (height / 2) * (width / 2).");
        return NULL;
    }

    if (v_len != (height / 2) * (width / 2)) {
        PyErr_SetString(ToxAVException, "Invalid V-plane size - must be (height / 2) * (width / 2).");
        return NULL;
    }

    TOXAV_ERR_SEND_FRAME error;
    bool result = toxav_video_send_frame(self->av, friend_number, width, height, y, u, v, &error);

    return parse_TOXAV_ERR_SEND_FRAME(result, error);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_video_send_bgr_frame(ToxCoreAV* self, PyObject* args)
{
    CHECK_TOXAV(self);

    uint32_t   friend_number;
    uint32_t   width;
    uint32_t   height;
    uint8_t*   bgr;
    Py_ssize_t bgr_len;

    if (PyArg_ParseTuple(args, "IIIs#", &friend_number, &width, &height, &bgr, &bgr_len) == false)
        return NULL;

    if (width < 1 || width > 0xFFFF) {
        PyErr_SetString(ToxAVException, "Invalid width - must be 16 bit unsigned.");
        return NULL;
    }

    if (height < 1 || height > 0xFFFF) {
        PyErr_SetString(ToxAVException, "Invalid height - must be 16 bit unsigned.");
        return NULL;
    }

    if (bgr_len != 3 * width * height) {
        PyErr_SetString(ToxAVException, "Invalid BGR size - must be height * width * 3.");
        return NULL;
    }

    self->frame = vpx_image_realloc(self->frame, width, height);

    if (self->frame == NULL) {
        PyErr_SetString(ToxAVException, "A resource allocation error occurred while trying to allocate image frame.");
        return NULL;
    }

    bgr_to_yuv420(self->frame->planes[0], self->frame->planes[1], self->frame->planes[2], bgr, self->frame->d_w, self->frame->d_h);

    TOXAV_ERR_SEND_FRAME error;
    bool result = toxav_video_send_frame(self->av, friend_number, self->frame->d_w, self->frame->d_h, self->frame->planes[0], self->frame->planes[1], self->frame->planes[2], &error);

    return parse_TOXAV_ERR_SEND_FRAME(result, error);
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_toxav_video_send_rgb_frame(ToxCoreAV* self, PyObject* args)
{
    CHECK_TOXAV(self);

    uint32_t   friend_number;
    uint32_t   width;
    uint32_t   height;
    uint8_t*   rgb;
    Py_ssize_t rgb_len;

    if (PyArg_ParseTuple(args, "IIIs#", &friend_number, &width, &height, &rgb, &rgb_len) == false)
        return NULL;

    if (width < 1 || width > 0xFFFF) {
        PyErr_SetString(ToxAVException, "Invalid width - must be 16 bit unsigned.");
        return NULL;
    }

    if (height < 1 || height > 0xFFFF) {
        PyErr_SetString(ToxAVException, "Invalid height - must be 16 bit unsigned.");
        return NULL;
    }

    if (rgb_len != 3 * width * height) {
        PyErr_SetString(ToxAVException, "Invalid RGB size - must be height * width * 3.");
        return NULL;
    }

    self->frame = vpx_image_realloc(self->frame, width, height);

    if (self->frame == NULL) {
        PyErr_SetString(ToxAVException, "A resource allocation error occurred while trying to allocate image frame.");
        return NULL;
    }

    rgb_to_yuv420(self->frame->planes[0], self->frame->planes[1], self->frame->planes[2], rgb, self->frame->d_w, self->frame->d_h);

    TOXAV_ERR_SEND_FRAME error;
    bool result = toxav_video_send_frame(self->av, friend_number, self->frame->d_w, self->frame->d_h, self->frame->planes[0], self->frame->planes[1], self->frame->planes[2], &error);

    return parse_TOXAV_ERR_SEND_FRAME(result, error);
}
//----------------------------------------------------------------------------------------------

#if PY_MAJOR_VERSION >= 3
struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "pytoxav",
    "Python binding for ToxAV",
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif
//----------------------------------------------------------------------------------------------

PyMethodDef ToxAV_methods[] = {
    //
    // callbacks
    //

    {
        "toxav_call_cb", (PyCFunction)ToxAV_callback_stub, METH_VARARGS,
        "toxav_call_cb(friend_number, audio_enabled, video_enabled)\n"
        "This event is triggered when a friend answer for call."
    },
    {
        "toxav_call_state_cb", (PyCFunction)ToxAV_callback_stub, METH_VARARGS,
        "toxav_call_state_cb(friend_number, state)\n"
        "This event is triggered when a call state changed."
    },
    {
        "toxav_bit_rate_status_cb", (PyCFunction)ToxAV_callback_stub, METH_VARARGS,
        "toxav_bit_rate_status_cb(friend_number, audio_bit_rate, video_bit_rate)\n"
        "This event is triggered when the network becomes too saturated for current bit rates at which "
        "point core suggests new bit rates."
    },
    {
        "toxav_audio_receive_frame_cb", (PyCFunction)ToxAV_callback_stub, METH_VARARGS,
        "toxav_audio_receive_frame_cb(friend_number, pcm, sample_count, channels, sampling_rate)\n"
        "This event is triggered when a audio data received."
    },
    {
        "toxav_video_receive_frame_cb", (PyCFunction)ToxAV_callback_stub, METH_VARARGS,
        "toxav_video_receive_frame_cb(friend_number, width, height, data)\n"
        "This event is triggered when a video data received."
    },

    //
    // methods
    //

    {
        "toxav_version_major", (PyCFunction)ToxAV_toxav_version_major, METH_NOARGS | METH_STATIC,
        "toxav_version_major()\n"
        "Return the major version number of the library. Can be used to display the "
        "ToxAV library version or to check whether the client is compatible with the "
        "dynamically linked version of ToxAV."
    },
    {
        "toxav_version_minor", (PyCFunction)ToxAV_toxav_version_minor, METH_NOARGS | METH_STATIC,
        "toxav_version_minor()\n"
        "Return the minor version number of the library."
    },
    {
        "toxav_version_patch", (PyCFunction)ToxAV_toxav_version_patch, METH_NOARGS | METH_STATIC,
        "toxav_version_patch()\n"
        "Return the patch number of the library."
    },
    {
        "toxav_version_is_compatible", (PyCFunction)ToxAV_toxav_version_is_compatible, METH_VARARGS | METH_STATIC,
        "toxav_version_is_compatible(major, minor, patch)\n"
        "Return whether the compiled library version is compatible with the passed "
        "version numbers."
    },
    {
        "toxav_kill", (PyCFunction)ToxAV_toxav_kill, METH_NOARGS,
        "toxav_kill()\n"
        "Releases all resources associated with the A/V session.\n"
        "If any calls were ongoing, these will be forcibly terminated without "
        "notifying peers. After calling this function, no other functions may be "
        "called and the av pointer becomes invalid."
    },
    // TODO: toxav_get_tox
    {
        "toxav_iteration_interval", (PyCFunction)ToxAV_toxav_iteration_interval, METH_NOARGS,
        "toxav_iteration_interval()\n"
        "Returns the interval in milliseconds when the next toxav_iterate call should "
        "be. If no call is active at the moment, this function returns 200."
    },
    {
        "toxav_iterate", (PyCFunction)ToxAV_toxav_iterate, METH_NOARGS,
        "toxav_iterate()\n"
        "Main loop for the session. This function needs to be called in intervals of "
        "toxav_iteration_interval() milliseconds. It is best called in the separate "
        "thread from tox_iterate."
    },
    {
        "toxav_call", (PyCFunction)ToxAV_toxav_call, METH_VARARGS,
        "toxav_call(friend_number, audio_bit_rate, video_bit_rate)\n"
        "Call a friend. This will start ringing the friend.\n"
        "It is the client's responsibility to stop ringing after a certain timeout, "
        "if such behaviour is desired. If the client does not stop ringing, the "
        "library will not stop until the friend is disconnected. Audio and video "
        "receiving are both enabled by default."
    },
    {
        "toxav_answer", (PyCFunction)ToxAV_toxav_answer, METH_VARARGS,
        "toxav_answer(friend_number, audio_bit_rate, video_bit_rate)\n"
        "Accept an incoming call.\n"
        "If answering fails for any reason, the call will still be pending and it is "
        "possible to try and answer it later. Audio and video receiving are both "
        "enabled by default."
    },
    {
        "toxav_call_control", (PyCFunction)ToxAV_toxav_call_control, METH_VARARGS,
        "toxav_call_control(friend_number, control)\n"
        "Sends a call control command to a friend."
    },
    {
        "toxav_bit_rate_set", (PyCFunction)ToxAV_toxav_bit_rate_set, METH_VARARGS,
        "toxav_bit_rate_set(friend_number, audio_bit_rate, video_bit_rate)\n"
        "Set the bit rate to be used in subsequent audio/video frames."
    },
    {
        "toxav_audio_send_frame", (PyCFunction)ToxAV_toxav_audio_send_frame, METH_VARARGS,
        "toxav_audio_send_frame(friend_number, pcm, sample_count, channels, sampling_rate)\n"
        "Send an audio frame to a friend.\n"
        "The expected format of the PCM data is: [s1c1][s1c2][...][s2c1][s2c2][...]...\n"
        "Meaning: sample 1 for channel 1, sample 1 for channel 2, ...\n"
        "For mono audio, this has no meaning, every sample is subsequent. For stereo, "
        "this means the expected format is LRLRLR... with samples for left and right "
        "alternating.\n"
        "pcm - an array of audio samples. The size of this array must be sample_count * channels.\n"
        "sample_count - number of samples in this frame. Valid numbers here are "
        "((sample rate) * (audio length) / 1000), where audio length can be "
        "2.5, 5, 10, 20, 40 or 60 millseconds.\n"
        "channels - number of audio channels. Supported values are 1 and 2.\n"
        "sampling_rate - audio sampling rate used in this frame. Valid sampling "
        "rates are 8000, 12000, 16000, 24000, or 48000."
    },
    {
        "toxav_video_send_yuv420_frame", (PyCFunction)ToxAV_toxav_video_send_yuv420_frame, METH_VARARGS,
        "toxav_video_send_yuv420_frame(friend_number, width, height, y, u, v)\n"
        "Send a I420 (IYUV) video frame to a friend."
    },
    {
        "toxav_video_send_bgr_frame", (PyCFunction)ToxAV_toxav_video_send_bgr_frame, METH_VARARGS,
        "toxav_video_send_bgr_frame(friend_number, width, height, bgr)\n"
        "Send a BGR video frame to a friend."
    },
    {
        "toxav_video_send_rgb_frame", (PyCFunction)ToxAV_toxav_video_send_rgb_frame, METH_VARARGS,
        "toxav_video_send_rgb_frame(friend_number, width, height, rgb)\n"
        "Send a RGB video frame to a friend."
    },
    {
        NULL
    }
};
//----------------------------------------------------------------------------------------------

static int init_helper(ToxCoreAV* self, PyObject* args)
{
    ToxAV_toxav_kill(self, NULL);

    PyObject* pycore = NULL;
    if (PyArg_ParseTuple(args, "O", &pycore) == false) {
        PyErr_SetString(ToxAVException, "You must supply a ToxCore param");
        return -1;
    }

    // TODO: Check arg class name - must be instance of ToxCore
    ToxCore* core = (ToxCore*)pycore;

    TOXAV_ERR_NEW error;
    ToxAV* av = toxav_new(core->tox, &error);

    bool success = false;
    switch (error) {
        case TOXAV_ERR_NEW_OK:
            success = true;
            break;
        case TOXAV_ERR_NEW_NULL:
            PyErr_SetString(ToxAVException, "One of the arguments to the function was NULL when it was not expected.");
            break;
        case TOXAV_ERR_NEW_MALLOC:
            PyErr_SetString(ToxAVException, "Memory allocation failure while trying to allocate structures required for the A/V session.");
            break;
        case TOXAV_ERR_NEW_MULTIPLE:
            PyErr_SetString(ToxAVException, "Attempted to create a second session for the same Tox instance.");
            break;
    }

    if (av == NULL || success == false)
        return -1;

    self->av   = av;
    self->core = core;

    Py_INCREF(self->core);

    toxav_callback_call(av, callback_call, self);
    toxav_callback_call_state(av, callback_call_state, self);
    toxav_callback_bit_rate_status(av, callback_bit_rate_status, self);
    toxav_callback_audio_receive_frame(av, callback_audio_receive_frame, self);
    toxav_callback_video_receive_frame(av, callback_video_receive_frame, self);

    return 0;
}
//----------------------------------------------------------------------------------------------

static PyObject* ToxAV_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    ToxCoreAV* self = (ToxCoreAV*)type->tp_alloc(type, 0);

    self->av    = NULL;
    self->core  = NULL;
    self->frame = NULL;

    // we don't care about subclass's arguments
    if (init_helper(self, NULL) == -1)
        return NULL;

    return (PyObject*)self;
}
//----------------------------------------------------------------------------------------------

static int ToxAV_init(ToxCoreAV* self, PyObject* args, PyObject* kwds)
{
    // since __init__ in Python is optional(superclass need to call it explicitly),
    // we need to initialize self->toxav in ToxAV_new instead of init.
    // If ToxAV_init is called, we re-initialize self->toxav and pass
    // the new ipv6enabled setting.
    return init_helper(self, args);
}
//----------------------------------------------------------------------------------------------

static int ToxAV_dealloc(ToxCoreAV* self)
{
    ToxAV_toxav_kill(self, NULL);

    return 0;
}
//----------------------------------------------------------------------------------------------

PyTypeObject ToxAVType = {
#if PY_MAJOR_VERSION >= 3
    PyVarObject_HEAD_INIT(NULL, 0)
#else
    PyObject_HEAD_INIT(NULL)
    0,                                          /* ob_size           */
#endif
    "ToxAV",                                    /* tp_name           */
    sizeof(ToxCoreAV),                          /* tp_basicsize      */
    0,                                          /* tp_itemsize       */
    (destructor)ToxAV_dealloc,                  /* tp_dealloc        */
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
    "ToxAV object",                             /* tp_doc            */
    0,                                          /* tp_traverse       */
    0,                                          /* tp_clear          */
    0,                                          /* tp_richcompare    */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter           */
    0,                                          /* tp_iternext       */
    ToxAV_methods,                              /* tp_methods        */
    0,                                          /* tp_members        */
    0,                                          /* tp_getset         */
    0,                                          /* tp_base           */
    0,                                          /* tp_dict           */
    0,                                          /* tp_descr_get      */
    0,                                          /* tp_descr_set      */
    0,                                          /* tp_dictoffset     */
    (initproc)ToxAV_init,                       /* tp_init           */
    0,                                          /* tp_alloc          */
    ToxAV_new,                                  /* tp_new            */
};
//----------------------------------------------------------------------------------------------

static void ToxAV_install_dict(void)
{
#define SET(name)                                  \
    PyObject* obj_##name = PyLong_FromLong(name);  \
    PyDict_SetItemString(dict, #name, obj_##name); \
    Py_DECREF(obj_##name)

    PyObject* dict = PyDict_New();
    if (dict == NULL)
        return;

    // #define TOXAV_VERSION_MAJOR
    SET(TOXAV_VERSION_MAJOR);
    // #define TOXAV_VERSION_MINOR
    SET(TOXAV_VERSION_MINOR);
    // #define TOXAV_VERSION_PATCH
    SET(TOXAV_VERSION_PATCH);

    // enum TOXAV_FRIEND_CALL_STATE
    SET(TOXAV_FRIEND_CALL_STATE_ERROR);
    SET(TOXAV_FRIEND_CALL_STATE_FINISHED);
    SET(TOXAV_FRIEND_CALL_STATE_SENDING_A);
    SET(TOXAV_FRIEND_CALL_STATE_SENDING_V);
    SET(TOXAV_FRIEND_CALL_STATE_ACCEPTING_A);
    SET(TOXAV_FRIEND_CALL_STATE_ACCEPTING_V);

    // enum TOXAV_CALL_CONTROL
    SET(TOXAV_CALL_CONTROL_RESUME);
    SET(TOXAV_CALL_CONTROL_PAUSE);
    SET(TOXAV_CALL_CONTROL_CANCEL);
    SET(TOXAV_CALL_CONTROL_MUTE_AUDIO);
    SET(TOXAV_CALL_CONTROL_UNMUTE_AUDIO);
    SET(TOXAV_CALL_CONTROL_HIDE_VIDEO);
    SET(TOXAV_CALL_CONTROL_SHOW_VIDEO);

#undef SET

    ToxAVType.tp_dict = dict;
}
//----------------------------------------------------------------------------------------------

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_pytoxav(void)
{
    PyObject* module = PyModule_Create(&moduledef);
#else
PyMODINIT_FUNC initpytoxav(void)
{
    PyObject* module = Py_InitModule("pytoxav", NULL);
#endif

    if (module == NULL)
        goto error;

    ToxAV_install_dict();

    // initialize toxav
    if (PyType_Ready(&ToxAVType) < 0) {
        fprintf(stderr, "Invalid PyTypeObject 'ToxAVType'\n");
        goto error;
    }

    Py_INCREF(&ToxAVType);
    PyModule_AddObject(module, "ToxAV", (PyObject*)&ToxAVType);

    ToxAVException = PyErr_NewException("pytoxav.ToxAVException", NULL, NULL);
    PyModule_AddObject(module, "ToxAVException", (PyObject*)ToxAVException);

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
