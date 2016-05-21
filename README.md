## pytoxcore

[![Build Status](https://secure.travis-ci.org/abbat/pytoxcore.png?branch=master)](http://travis-ci.org/abbat/pytoxcore) [![Coverity status](https://scan.coverity.com/projects/6250/badge.svg)](https://scan.coverity.com/projects/abbat-pytoxcore)

Python binding for [ToxCore](https://github.com/irungentoo/toxcore).

### Download / Install

* [Debian, Ubuntu](http://software.opensuse.org/download.html?project=home:antonbatenev:tox&package=python-toxcore)
* [Fedora, openSUSE, CentOS](http://software.opensuse.org/download.html?project=home:antonbatenev:tox&package=python-toxcore)
* [Arch](http://software.opensuse.org/download.html?project=home:antonbatenev:tox&package=python-toxcore), [Arch AUR](https://aur.archlinux.org/packages/python-toxcore) (see also [AUR Helpers](https://wiki.archlinux.org/index.php/AUR_Helpers))

### Usage

See [Echo Bot Example](https://github.com/abbat/pytoxcore/tree/master/examples).

### Documentation

Most methods of ToxCore, ToxAV and ToxDNS classes well documented in original [tox.h](https://github.com/irungentoo/toxcore/blob/master/toxcore/tox.h), [toxav.h](https://github.com/irungentoo/toxcore/blob/master/toxav/toxav.h) and [toxdns.h](https://github.com/irungentoo/toxcore/blob/master/toxdns/toxdns.h).

Also you can get help from extension itself:

```
$ python
>>> from pytoxcore import ToxCore, ToxAV, ToxDNS
>>> help(ToxCore)
```

Additional non libtoxcore api methods and callbacks described below.

#### ToxCore

##### tox_keypair_new

Return new `(public_key, secret_key)` tuple. Used to create new tox account.

```
tox_keypair_new()
```

##### tox_public_key_restore

Return public key restored from secret key.

```
tox_public_key_restore(secret_key)
```

##### tox_nospam_new

Return new random nospam value as hex-string.

```
tox_nospam_new()
```

##### tox_address_new

Return ToxID from public key and nospam value.

```
tox_address_new(public_key, nospam)
```

##### tox_address_check

Check given ToxID and throws exception if address is invalid.

```
tox_address_check(address)
```

##### tox_sendfile

Send file identified by `path` to a friend like system `sendfile`. Return `file_number` on success like original `tox_file_send`. Call `tox_sendfile_cb` callback (see below).

```
tox_sendfile(friend_number, kind, path, filename, timeout)
```

##### tox_sendfile_cb

This event is triggered when `tox_sendfile` call finished.

```
tox_sendfile_cb(friend_number, file_number, status)
```

`status` may be one of:

* `TOX_SENDFILE_COMPLETED` - call finished successfully;
* `TOX_SENDFILE_TIMEOUT` - send timeout occured;
* `TOX_SENDFILE_ERROR` - filesystem, toxcore or other error.

##### tox_recvfile

Receive file from a friend and store it to `path`. Call `tox_recvfile_cb` callback (see below).

```
tox_recvfile(friend_number, file_number, file_size, path, timeout)
```

##### tox_recvfile_cb

This event is triggered when `tox_recvfile` call finished.

```
tox_recvfile_cb(friend_number, file_number, status)
```

`status` may be one of:

* `TOX_RECVFILE_COMPLETED` - call finished successfully;
* `TOX_RECVFILE_TIMEOUT` - receive timeout occured;
* `TOX_RECVFILE_ERROR` - filesystem, toxcore or other error.

#### ToxAV

##### toxav_video_frame_format_set

Set video frame format for `toxav_video_receive_frame_cb` callback (see below).

```
toxav_video_frame_format_set(format)
```

Format may be one of:

* `TOXAV_VIDEO_FRAME_FORMAT_BGR` - BGR frame format;
* `TOXAV_VIDEO_FRAME_FORMAT_RGB` - RGB frame format;
* `TOXAV_VIDEO_FRAME_FORMAT_YUV420` - (default) YUV420 format.

##### toxav_video_send_frame

Original `toxav_video_send_frame` method splitted into three to send frame in BGR, RGB and YUV420 (default) formats.

```
toxav_video_send_bgr_frame(friend_number, width, height, bgr)
toxav_video_send_rgb_frame(friend_number, width, height, rgb)
toxav_video_send_yuv420_frame(friend_number, width, height, y, u, v)
```

##### toxav_video_receive_frame_cb

This event is triggered when a video frame received. First for RGB/BGR video frame format, second for YUV420 (default).

```
toxav_video_receive_frame_cb(friend_number, width, height, rgb)
toxav_video_receive_frame_cb(friend_number, width, height, y, u, v, ystride, ustride, vstride)
```
