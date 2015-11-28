#!/usr/bin/env python
# -*- coding: utf-8 -*-

__title__    = "avbot"
__version__  = "0.0.12"
__author__   = "Anton Batenev"
__license__  = "BSD"

import sys
import os
import time
import re
import cv2
import numpy
import pyaudio
import threading

from echobot import *
from pytoxcore import ToxAV


class AVCall(object):
    """
    Описатель звонка
    """
    def __init__(self, friend_number, audio_enabled, video_enabled):
        """
        Аргументы:
            friend_number (int)  -- Номер друга
            audio_enabled (bool) -- Включена отправка аудио
            video_enabled (bool) -- Включена отправка видео
        """
        self.friend_number = friend_number
        self.audio_enabled = audio_enabled
        self.video_enabled = video_enabled


class EchoAVBot(ToxAV):
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
        self.options = self.core.options
        self.calls   = {}

        super(EchoAVBot, self).__init__(self.core)

        self.toxav_video_frame_format_set(self.TOXAV_VIDEO_FRAME_FORMAT_BGR)

        self.channels      = 1                                # моно микрофон
        self.sampling_rate = 8000                             # стандартный телефон
        self.sample_count  = self.sampling_rate / 1000 * 10   # 10 ms буфер

        self.cv2   = cv2.VideoCapture(0)
        self.audio = pyaudio.PyAudio()

        self.iterate_thread = threading.Thread(target = self.iterate_cb)
        self.iterate_thread.start()

        self.input_audio = self.audio.open(channels = self.channels, rate = self.sampling_rate, format = pyaudio.paInt16, frames_per_buffer = self.sample_count, input = True)

        self.audio_thread = threading.Thread(target = self.audio_cb)
        self.audio_thread.start()

        self.video_thread = threading.Thread(target = self.video_cb)
        self.video_thread.start()


    def stop(self):
        """
        Остановка всех потоков
        """
        self.core.verbose("stopping...")

        self.running = False

        self.video_thread.join()
        self.audio_thread.join()
        self.iterate_thread.join()

        self.input_audio.stop_stream()
        self.input_audio.close()

        self.audio.terminate()

        self.core.verbose("stopped")


    def iterate_cb(self):
        """
        Рабочий цикл, запускается в отдельном потоке относительно ToxCore
        """
        while self.running:
            self.toxav_iterate()
            interval = self.toxav_iteration_interval()
            time.sleep(float(interval) / 1000.0)


    def audio_cb(self):
        """
        Поток получения аудио данных и отправки их респондентам
        """
        while self.running:
            available = self.input_audio.get_read_available()
            if available >= self.sample_count:
                pcm = self.input_audio.read(self.sample_count)
                if len(pcm) > 0:
                    for call in itervalues(self.calls):
                        if call.audio_enabled:
                            self.toxav_audio_send_frame(call.friend_number, pcm, self.sample_count, self.channels, self.sampling_rate)
            else:
                time.sleep(float(self.sample_count) / float(self.sampling_rate) / 2.0)


    def video_cb(self):
        """
        Поток получения видео данных и отправки их респондентам
        """
        while self.running:
            result, frame = self.cv2.read()
            if result:
                height, width, channels = frame.shape
                for call in itervalues(self.calls):
                    if call.video_enabled:
                        self.toxav_video_send_bgr_frame(call.friend_number, width, height, frame.tostring())
            time.sleep(1.0 / 30.0)


    def toxav_call_cb(self, friend_number, audio_enabled, video_enabled):
        """
        Событие входящего звонка
        (см. toxav_call_cb)

        Аргументы:
            friend_number (int)  -- Номер друга
            audio_enabled (bool) -- Включена передача аудио
            video_enabled (bool) -- Включена передача видео
        """
        friend_name = self.core.tox_friend_get_name(friend_number)

        self.core.verbose("Friend {0}/{1} call with audio = {2} and video = {3}".format(friend_name, friend_number, audio_enabled, video_enabled))

        self.toxav_answer(friend_number, self.sampling_rate / 1000, 64)

        self.calls[friend_number] = AVCall(friend_number, audio_enabled, video_enabled)


    def toxav_call_state_cb(self, friend_number, state):
        """
        Событие смены состояния звонка
        (см. toxav_call_state_cb)

        Аргументы:
            friend_number (int) -- Номер друга
            state         (int) -- Битовая маска состояний
        """
        friend_name = self.core.tox_friend_get_name(friend_number)

        self.core.verbose("Friend {0}/{1} change call state = {2}".format(friend_name, friend_number, state))

        if state & self.TOXAV_FRIEND_CALL_STATE_ACCEPTING_A:
            self.calls[friend_number].audio_enabled = True

        if state & self.TOXAV_FRIEND_CALL_STATE_ACCEPTING_V:
            self.calls[friend_number].video_enabled = True

        if (state & self.TOXAV_FRIEND_CALL_STATE_ERROR) or (state & self.TOXAV_FRIEND_CALL_STATE_FINISHED):
            del self.calls[friend_number]

            cv2.destroyWindow("frame-{0}".format(friend_number))
            cv2.waitKey(1)


    def toxav_bit_rate_status_cb(self, friend_number, audio_bit_rate, video_bit_rate):
        """
        Событие перегрузки сети
        (см. toxav_bit_rate_status_cb)

        Аргументы:
            friend_number  (int) -- Номер друга
            audio_bit_rate (int) -- Рекомендуемая скорость аудио
            video_bit_rate (int) -- Рекомендуемая скорость видео
        """
        self.core.debug("toxav_bit_rate_status_cb: friend_number = {0}, audio_bit_rate = {1}, video_bit_rate = {2}".format(friend_number, audio_bit_rate, video_bit_rate))


    def toxav_audio_receive_frame_cb(self, friend_number, pcm, sample_count, channels, sampling_rate):
        """
        Событие получения аудио-кадра
        (см. toxav_audio_receive_frame_cb)

        Аргументы:
            friend_number (int) -- Номер друга
            pcm           (str) -- Данные PCM
            sample_count  (int) -- Количество сэмплов
            channels      (int) -- Количество каналов
            sampling_rate (int) -- Частота дискретизации
        """
        #self.core.debug("toxav_audio_receive_frame_cb: friend_number = {0}, sample_count = {1}, channels = {2}, sampling_rate = {3}, size = {4}".format(friend_number, sample_count, channels, sampling_rate, len(pcm)))
        pass


    def toxav_video_receive_frame_cb(self, friend_number, width, height, rgb):
        """
        Событие получения видео-кадра
        (см. toxav_video_receive_frame_cb)

        Аргументы:
            friend_number (int) -- Номер друга
            width         (int) -- Ширина кадра
            height        (int) -- Высота кадра
            rgb           (str) -- Данные в формате RGB/BGR (см. toxav_video_frame_format_set)
        """
        frame = numpy.ndarray(shape = (height, width, 3), dtype = numpy.dtype(numpy.uint8), buffer = rgb)
        cv2.imshow("frame-{0}".format(friend_number), frame)
        cv2.waitKey(1)


if __name__ == "__main__":
    regexp  = re.compile("--config=(.*)")
    cfgfile = [match.group(1) for arg in sys.argv for match in [regexp.search(arg)] if match]

    if len(cfgfile) == 0:
        cfgfile = "echobot.cfg"
    else:
        cfgfile = cfgfile[0]

    options = EchoBotOptions(EchoBotOptions.loadOptions(cfgfile))

    bot   = EchoBot(options)
    botav = EchoAVBot(bot)

    try:
        bot.run()
    except KeyboardInterrupt:
        bot.save_file()

    botav.stop()
