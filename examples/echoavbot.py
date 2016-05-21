#!/usr/bin/env python
# -*- coding: utf-8 -*-

__title__    = "echoavbot"
__author__   = "Anton Batenev"
__license__  = "BSD"

import threading

from echobot import *
from pytoxcore import ToxAV, ToxAVException


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

        super(EchoAVBot, self).__init__(self.core)

        self.toxav_video_frame_format_set(self.TOXAV_VIDEO_FRAME_FORMAT_BGR)

        self.iterate_thread = threading.Thread(target = self.iterate_cb)
        self.iterate_thread.start()


    def stop(self):
        """
        Остановка всех потоков
        """
        self.core.verbose("stopping...")
        self.running = False
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
            self.toxav_answer(friend_number, 32, 5000)


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
        if self.running:
            try:
                self.toxav_audio_send_frame(friend_number, pcm, sample_count, channels, sampling_rate)
            except ToxAVException as e:
                self.core.verbose("ToxAVException: {0}".format(e))


    def toxav_video_receive_frame_cb(self, friend_number, width, height, bgr):
        """
        Событие получения видео-кадра
        (см. toxav_video_receive_frame_cb)

        Аргументы:
            friend_number (int) -- Номер друга
            width         (int) -- Ширина кадра
            height        (int) -- Высота кадра
            bgr           (str) -- Данные в формате BGR (см. toxav_video_frame_format_set)
        """
        if self.running:
            try:
                self.toxav_video_send_bgr_frame(friend_number, width, height, bgr)
            except ToxAVException as e:
                self.core.verbose("ToxAVException: {0}".format(e))


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
        pass

    botav.stop()
    bot.save_file()
