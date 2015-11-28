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


class EchoAVBot(ToxAV):
    """
    AV Бот
    """
    def __init__(self, core):
        """
        Аргументы:
            core (ToxCore) -- экземпляр ToxCore
        """
        self.running = True
        self.options = core.options

        super(EchoAVBot, self).__init__(core)

        self.toxav_video_frame_format_set(self.TOXAV_VIDEO_FRAME_FORMAT_BGR)

        self.cap = cv2.VideoCapture()

        self.iterate_thread = threading.Thread(target = self.iterate)
        self.iterate_thread.start()


    def debug(self, message):
        """
        Вывод отладочной информации

        Аргументы:
            message (str) -- Сообщение для вывода
        """
        if self.options.debug:
            sys.stderr.write("[{0}] {1}\n".format(time.strftime("%Y-%m-%d %H:%M:%S"), message))


    def verbose(self, message):
        """
        Вывод расширенной информации

        Аргументы:
            message (str) -- Сообщение для вывода
        """
        if self.options.verbose:
            sys.stderr.write("[{0}] {1}\n".format(time.strftime("%Y-%m-%d %H:%M:%S"), message))


    def stop(self):
        """
        Остановка всех потоков
        """
        self.verbose("stopping...")
        self.running = False
        self.iterate_thread.join()
        self.verbose("stopped")


    def iterate(self):
        """
        Рабочий цикл, запускается в отдельном потоке относительно ToxCore
        """
        while self.running:
            self.toxav_iterate()
            interval = self.toxav_iteration_interval()
            time.sleep(interval / 1000.0)


    def toxav_call_cb(self, friend_number, audio_enabled, video_enabled):
        self.debug("toxav_call_cb: friend_number = {0}, audio_enabled = {1}, video_enabled = {2}".format(friend_number, audio_enabled, video_enabled))
        self.toxav_answer(friend_number, 16, 64)


    def toxav_call_state_cb(self, friend_number, state):
        self.debug("toxav_call_state_cb: friend_number = {0}, state = {1}".format(friend_number, state))

        if (state & self.TOXAV_FRIEND_CALL_STATE_ERROR or state & self.TOXAV_FRIEND_CALL_STATE_FINISHED):
            cv2.destroyWindow("frame-{0}".format(friend_number))


    def toxav_bit_rate_status_cb(self, friend_number, audio_bit_rate, video_bit_rate):
        self.debug("toxav_bit_rate_status_cb: friend_number = {0}, audio_bit_rate = {1}, video_bit_rate = {2}".format(friend_number, audio_bit_rate, video_bit_rate))


    def toxav_audio_receive_frame_cb(self, friend_number, pcm, sample_count, channels, sampling_rate):
        self.debug("toxav_audio_receive_frame_cb: friend_number = {0}, sample_count = {1}, channels = {2}, sampling_rate = {3}, size = {4}".format(friend_number, sample_count, channels, sampling_rate, len(pcm)))


    def toxav_video_receive_frame_cb(self, friend_number, width, height, rgb):
        """
        Получение видео-кадра
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
