#!/usr/bin/env python
# -*- coding: utf-8 -*-

__title__    = "avbot"
__author__   = "Anton Batenev"
__license__  = "BSD"

import cv2
import pyaudio
import threading

from echobot import *
from pytoxcore import ToxAV, ToxAVException


from pkg_resources import parse_version

if parse_version(cv2.__version__) >= parse_version("3"):
    CV_CAP_PROP_FPS          = cv2.CAP_PROP_FPS
    CV_CAP_PROP_FRAME_WIDTH  = cv2.CV_CAP_PROP_FRAME_WIDTH
    CV_CAP_PROP_FRAME_HEIGHT = cv2.CV_CAP_PROP_FRAME_HEIGHT
else:
    CV_CAP_PROP_FPS          = cv2.cv.CV_CAP_PROP_FPS
    CV_CAP_PROP_FRAME_WIDTH  = cv2.cv.CV_CAP_PROP_FRAME_WIDTH
    CV_CAP_PROP_FRAME_HEIGHT = cv2.cv.CV_CAP_PROP_FRAME_HEIGHT


class AVCall(object):
    """
    Описатель звонка
    """
    def __init__(self, friend_number, video_enabled, audio_enabled):
        """
        Аргументы:
            friend_number (int)  -- Номер друга
            audio_enabled (bool) -- Включена передача аудио
            video_enabled (bool) -- Включена передача видео
        """
        self.friend_number = friend_number
        self.video_enabled = video_enabled
        self.audio_enabled = audio_enabled


class AVBot(ToxAV):
    """
    AV Бот
    """
    def __init__(self, core):
        """
        Аргументы:
            core (ToxCore) -- Экземпляр ToxCore
        """
        self.core    = core
        self.running = True

        self.calls = {}

        self.audio         = None
        self.audio_stream  = None
        self.audio_thread  = None
        self.audio_running = False

        self.video         = None
        self.video_thread  = None
        self.video_running = False

        super(AVBot, self).__init__(self.core)

        self.toxav_video_frame_format_set(self.TOXAV_VIDEO_FRAME_FORMAT_BGR)

        self.iterate_thread = threading.Thread(target = self.iterate_cb)
        self.iterate_thread.start()


    def stop(self):
        """
        Остановка всех потоков
        """
        self.core.verbose("stopping...")

        self.running = False

        self.stop_video_thread()
        self.stop_audio_thread()

        self.iterate_thread.join()

        self.toxav_kill()

        self.core.verbose("stopped")


    def iterate_cb(self):
        """
        Рабочий цикл, запускается в отдельном потоке относительно ToxCore
        """
        while self.running:
            self.toxav_iterate()
            interval = self.toxav_iteration_interval()
            time.sleep(float(interval) / 1000.0)


    def start_video_thread(self):
        """
        Запуск потока кодирования видео
        """
        if self.video_thread != None:
            return

        self.video_running = True

        self.video = cv2.VideoCapture(0)
        self.video.set(CV_CAP_PROP_FPS, 25)
        self.video.set(CV_CAP_PROP_FRAME_WIDTH, 640)
        self.video.set(CV_CAP_PROP_FRAME_HEIGHT, 480)

        self.video_thread = threading.Thread(target = self.video_cb)
        self.video_thread.start()


    def stop_video_thread(self):
        """
        Остановка потока кодирования видео
        """
        if self.video_thread == None:
            return

        self.video_running = False

        self.video_thread.join()

        self.video_thread = None
        self.video        = None


    def video_cb(self):
        """
        Рабочий цикл отправки видео
        """
        while self.video_running:
            try:
                result, frame = self.video.read()
                if result:
                    height, width, channels = frame.shape
                    for call in itervalues(self.calls):
                        if call.video_enabled:
                            try:
                                self.toxav_video_send_bgr_frame(call.friend_number, width, height, frame.tostring())
                            except ToxAVException as e:
                                self.core.verbose("ToxAVException: {0}".format(e))
            except Exception as e:
                self.core.verbose("{0}".format(e))

            time.sleep(0.01)


    def start_audio_thread(self):
        """
        Запуск потока кодирования аудио
        """
        if self.audio_thread != None:
            return

        self.audio_running = True

        self.audio_device       = "USB 2.0 Camera: USB Audio (hw:1,0)"
        self.audio_device_index = None
        self.audio_rate         = 8000
        self.audio_channels     = 1
        self.audio_duration     = 60
        self.audio_sample_count = self.audio_rate * self.audio_channels * self.audio_duration / 1000

        self.audio = pyaudio.PyAudio()

        for i in range(0, self.audio.get_device_count() - 1):
            name = self.audio.get_device_info_by_index(i)["name"]
            self.core.debug("Found device {0}: {1}".format(i, name))
            if name == self.audio_device:
                self.audio_device_index = i

        if self.audio_device_index != None:
            device_info = self.audio.get_device_info_by_index(self.audio_device_index)
            self.core.debug("Use found audio device: {0}".format(device_info["name"]))
        else:
            device_info = self.audio.get_default_input_device_info()
            self.core.debug("Use default audio device: {0}".format(device_info["name"]))
            self.audio_device_index = device_info["index"]

        format_supported = self.audio.is_format_supported(self.audio_rate, self.audio_device_index, self.audio_channels, pyaudio.paInt16)
        self.core.debug("Input audio format supported: {0}".format("yes" if format_supported else "no"))

        if not format_supported:
            self.audio_running = False
            self.self.audio    = None
            return

        self.audio_stream = self.audio.open(format = pyaudio.paInt16,
                                            rate = self.audio_rate,
                                            channels = self.audio_channels,
                                            input = True,
                                            input_device_index = self.audio_device_index,
                                            frames_per_buffer = self.audio_sample_count * 10)

        self.audio_thread = threading.Thread(target = self.audio_cb)
        self.audio_thread.start()


    def stop_audio_thread(self):
        """
        Остановка потока кодирования аудио
        """
        if self.audio_thread == None:
            return

        self.audio_running = False

        self.audio_thread.join()

        self.audio_thread = None
        self.audio_stream = None
        self.audio        = None


    def audio_cb(self):
        """
        Рабочий цикл отправки видео
        """
        while self.audio_running:
            try:
                pcm = self.audio_stream.read(self.audio_sample_count)
                if pcm:
                    for call in itervalues(self.calls):
                        if call.audio_enabled:
                            try:
                                self.toxav_audio_send_frame(call.friend_number, pcm, self.audio_sample_count, self.audio_channels, self.audio_rate)
                            except ToxAVException as e:
                                self.core.verbose("ToxAVException: {0}".format(e))
            except Exception as e:
                self.core.verbose("{0}".format(e))

            time.sleep(0.01)


    def toxav_call_cb(self, friend_number, audio_enabled, video_enabled):
        """
        Событие входящего звонка
        (см. toxav_call_cb)

        Аргументы:
            friend_number (int)  -- Номер друга
            audio_enabled (bool) -- Включена передача аудио
            video_enabled (bool) -- Включена передача видео
        """
        if self.running:
            friend_name = self.core.tox_friend_get_name(friend_number)
            self.core.verbose("Friend {0}/{1} call with audio = {2} and video = {3}".format(friend_name, friend_number, audio_enabled, video_enabled))

            self.calls[friend_number] = AVCall(friend_number, audio_enabled, video_enabled)

            self.toxav_answer(friend_number, 32, 5000)

            self.start_audio_thread()
            self.start_video_thread()


    def toxav_call_state_cb(self, friend_number, state):
        """
        Событие смены состояния звонка
        (см. toxav_call_state_cb)

        Аргументы:
            friend_number (int) -- Номер друга
            state         (int) -- Битовая маска состояний
        """
        if self.running:
            friend_name = self.core.tox_friend_get_name(friend_number)
            self.core.verbose("Friend {0}/{1} change call state = {2}".format(friend_name, friend_number, state))

            if state == self.TOXAV_FRIEND_CALL_STATE_FINISHED or state == self.TOXAV_FRIEND_CALL_STATE_ERROR:
                del self.calls[friend_number]
                if len(self.calls) == 0:
                    self.stop_video_thread()
                    self.stop_audio_thread()

            if state & self.TOXAV_FRIEND_CALL_STATE_ACCEPTING_A:
                self.calls[friend_number].audio_enabled = True

            if state & self.TOXAV_FRIEND_CALL_STATE_ACCEPTING_V:
                self.calls[friend_number].video_enabled = True


    def toxav_bit_rate_status_cb(self, friend_number, audio_bit_rate, video_bit_rate):
        """
        Событие перегрузки сети
        (см. toxav_bit_rate_status_cb)

        Аргументы:
            friend_number  (int) -- Номер друга
            audio_bit_rate (int) -- Рекомендуемая скорость аудио
            video_bit_rate (int) -- Рекомендуемая скорость видео
        """
        if self.running:
            friend_name = self.core.tox_friend_get_name(friend_number)
            self.core.verbose("Friend {0}/{1} change audio bitrate = {2} and video bitrate = {3}".format(friend_name, friend_number, audio_bit_rate, video_bit_rate))


if __name__ == "__main__":
    regexp  = re.compile("--config=(.*)")
    cfgfile = [match.group(1) for arg in sys.argv for match in [regexp.search(arg)] if match]

    if len(cfgfile) == 0:
        cfgfile = "echobot.cfg"
    else:
        cfgfile = cfgfile[0]

    options = EchoBotOptions(EchoBotOptions.loadOptions(cfgfile))

    bot   = EchoBot(options)
    botav = AVBot(bot)

    try:
        bot.run()
    except KeyboardInterrupt:
        pass

    botav.stop()
    bot.save_file()
