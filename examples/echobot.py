#!/usr/bin/env python
# -*- coding: utf-8 -*-

__title__    = "echobot"
__version__  = "0.1"
__author__   = "Anton Batenev"
__license__  = "BSD"

import sys
import os
import time
import re

from pytoxcore import ToxCore


# PEP-8
try:
    import configparser as EchoBotConfigParser
except ImportError:
    import ConfigParser as EchoBotConfigParser


# PEP-469
try:
    dict.iteritems
except AttributeError:
    def itervalues(d):
        return iter(d.values())
    def iteritems(d):
        return iter(d.items())
    def listvalues(d):
        return list(d.values())
    def listitems(d):
        return list(d.items())
else:
    def itervalues(d):
        return d.itervalues()
    def iteritems(d):
        return d.iteritems()
    def listvalues(d):
        return d.values()
    def listitems(d):
        return d.items()


class EchoBotOptions(object):
    """
    Опции приложения
    """
    def __init__(self, options):
        """
        Аргументы:
            options (dict) -- опции приложения
        """
        self.debug          = self._bool(options["debug"])
        self.verbose        = self._bool(options["verbose"]) or self.debug
        self.name           = str(options["name"])
        self.status_message = str(options["status_message"])
        self.avatar         = str(options["avatar"])
        self.save_file      = str(options["save_file"])
        self.save_tmp_file  = str(options["save_tmp_file"])
        self.save_interval  = int(options["save_interval"])
        self.bootstrap_host = str(options["bootstrap_host"])
        self.bootstrap_port = int(options["bootstrap_port"])
        self.bootstrap_key  = str(options["bootstrap_key"])
        self.ipv6_enabled   = self._bool(options["ipv6_enabled"])
        self.udp_enabled    = self._bool(options["udp_enabled"])

        proxy_type = options["proxy_type"].lower()
        if len(proxy_type) == 0:
            self.proxy_type = ToxCore.TOX_PROXY_TYPE_NONE
        elif proxy_type == "http":
            self.proxy_type = ToxCore.TOX_PROXY_TYPE_HTTP
        elif proxy_type == "socks":
            self.proxy_type = ToxCore.TOX_PROXY_TYPE_SOCKS5
        else:
            raise ValueError("Unknown proxy type: {0}".format(options["proxy_type"]))

        self.proxy_host = str(options["proxy_host"])
        self.proxy_port = int(options["proxy_port"])
        self.start_port = int(options["start_port"])
        self.end_port   = int(options["end_port"])
        self.tcp_port   = int(options["tcp_port"])

        self.accept_avatars  = self._bool(options["accept_avatars"])
        self.max_avatar_size = int(options["max_avatar_size"])
        self.avatars_path    = str(options["avatars_path"])
        self.accept_files    = self._bool(options["accept_files"])
        self.max_file_size   = int(options["max_file_size"])
        self.files_path      = str(options["files_path"])


    def __repr__(self):
        return "{0!s}({1!r})".format(self.__class__, self.__dict__)


    @staticmethod
    def _bool(value):
        """
        Преобразование строкового значения к булевому

        Аргументы:
            value (str|bool) -- Строковое представление булева значения

        Результат (bool):
            Результат преобразования строкового значения к булеву - [true|yes|t|y|1] => True, иначе False
        """
        if type(value) is bool:
            return value

        value = value.lower().strip()

        if value == "true" or value == "yes" or value == "t" or value == "y" or value == "1":
            return True

        return False


    @staticmethod
    def defaultOptions():
        """
        Опции по умолчанию

        Результат (dict):
            Словарь опций по умолчанию
        """
        tox_opts = ToxCore.tox_options_default()

        options = {
            "debug"           : "yes",
            "verbose"         : "yes",
            "name"            : "EchoBot",
            "status_message"  : "Think Safety",
            "avatar"          : "echobot.png",
            "save_file"       : "echobot.data",
            "save_tmp_file"   : "echobot.data.tmp",
            "save_interval"   : "300",
            "bootstrap_host"  : "178.62.250.138",   # https://wiki.tox.chat/users/nodes
            "bootstrap_port"  : "33445",
            "bootstrap_key"   : "788236D34978D1D5BD822F0A5BEBD2C53C64CC31CD3149350EE27D4D9A2F9B6B",
            "ipv6_enabled"    : "yes" if tox_opts["ipv6_enabled"] else "no",
            "udp_enabled"     : "yes" if tox_opts["udp_enabled"]  else "no",
            "proxy_type"      : "",
            "proxy_host"      : "" if tox_opts["proxy_host"] is None else tox_opts["proxy_host"],
            "proxy_port"      : str(tox_opts["proxy_port"]),
            "start_port"      : str(tox_opts["start_port"]),
            "end_port"        : str(tox_opts["end_port"]),
            "tcp_port"        : str(tox_opts["tcp_port"]),
            "accept_avatars"  : "no",
            "max_avatar_size" : "0",
            "avatars_path"    : "",
            "accept_files"    : "no",
            "max_file_size"   : "0",
            "files_path"      : "",
        }

        if tox_opts["proxy_type"] == ToxCore.TOX_PROXY_TYPE_SOCKS5:
            options["proxy_type"] = "socks"
        elif tox_opts["proxy_type"] == ToxCore.TOX_PROXY_TYPE_HTTP:
            options["proxy_type"] = "http"
        elif tox_opts["proxy_type"] != ToxCore.TOX_PROXY_TYPE_NONE:
            raise NotImplementedError("Unknown proxy_type: {0}".format(tox_opts["proxy_type"]))

        return options


    @staticmethod
    def loadOptions(filename, options = None):
        """
        Чтение секции echobot INI файла echobot.cfg

        Аргументы:
            filename (str)  -- Имя INI файла
            options  (dict) -- Базовая конфигурация

        Результат (dict):
            Конфигурация приложения на основе файла конфигурации
        """
        if options is None:
            options = EchoBotOptions.defaultOptions()

        options = options.copy()

        parser = EchoBotConfigParser.ConfigParser()

        parser.read(filename)

        for section in parser.sections():
            name = section.lower()
            if name == "echobot":
                for option in parser.options(section):
                    options[option.lower()] = parser.get(section, option).strip()

        return options


class EchoBotFile(object):
    """
    Описатель файла
    """
    def __init__(self):
        self.fd       = None
        self.write    = False
        self.read     = False
        self.size     = 0
        self.position = 0
        self.path     = ""
        self.name     = ""
        self.id       = ""
        self.kind     = ToxCore.TOX_FILE_KIND_DATA


class EchoBot(ToxCore):
    """
    Бот
    """
    def __init__(self, options):
        """
        Аргументы:
            options (EchoBotOptions) -- Опции приложения
        """
        self.options = options

        tox_opts = {
            "ipv6_enabled" : self.options.ipv6_enabled,
            "udp_enabled"  : self.options.udp_enabled,
            "proxy_type"   : self.options.proxy_type,
            "proxy_host"   : self.options.proxy_host,
            "proxy_port"   : self.options.proxy_port,
            "start_port"   : self.options.start_port,
            "end_port"     : self.options.end_port,
            "tcp_port"     : self.options.tcp_port
        }

        if os.path.isfile(self.options.save_file):
            self.debug("Load data from file: {0}".format(self.options.save_file))
            with open(self.options.save_file, "rb") as f:
                tox_opts["savedata_data"] = f.read()

        super(EchoBot, self).__init__(tox_opts)

        self.debug("Set self name: {0}".format(self.options.name))
        self.tox_self_set_name(self.options.name)

        self.debug("Set self status: {0}".format(self.options.status_message))
        self.tox_self_set_status_message(self.options.status_message)

        self.debug("Get self ToxID: {0}".format(self.tox_self_get_address()))

        # список активных файловых операций { friend_id: { file_id: EchoBotFile } }
        self.files = {}


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


    def run(self):
        """
        Рабочий цикл
        """
        self.debug("Connecting to: {0} {1} {2}".format(self.options.bootstrap_host, self.options.bootstrap_port, self.options.bootstrap_key))
        self.tox_bootstrap(self.options.bootstrap_host, self.options.bootstrap_port, self.options.bootstrap_key)
        self.debug("Connected to: {0} {1} {2}".format(self.options.bootstrap_host, self.options.bootstrap_port, self.options.bootstrap_key))

        checked       = False
        savetime      = 0
        save_interval = self.options.save_interval * 1000

        while True:
            status = self.tox_self_get_connection_status()

            if not checked and status != ToxCore.TOX_CONNECTION_NONE:
                checked = True

            if checked and status == ToxCore.TOX_CONNECTION_NONE:
                self.debug("Connecting to: {0} {1} {2}".format(self.options.bootstrap_host, self.options.bootstrap_port, self.options.bootstrap_key))
                self.tox_bootstrap(self.options.bootstrap_host, self.options.bootstrap_port, self.options.bootstrap_key)
                self.debug("Connected to: {0} {1} {2}".format(self.options.bootstrap_host, self.options.bootstrap_port, self.options.bootstrap_key))
                checked = False

            self.tox_iterate()

            interval = self.tox_iteration_interval()

            time.sleep(interval / 1000.0)

            savetime += interval
            if savetime > save_interval:
                self.save_file()
                savetime = 0


    def save_file(self):
        """
        Сохранение данных
        """
        self.debug("Save data to file: {0}".format(self.options.save_tmp_file))

        with open(self.options.save_tmp_file, "wb") as f:
            f.write(self.tox_get_savedata());

        self.debug("Move data to file: {0}".format(self.options.save_file))
        os.rename(self.options.save_tmp_file, self.options.save_file)


    def send_avatar(self, friend_number):
        """
        Отправка аватара

        Агрументы:
            friend_number (int) -- Номер друга
        """
        if len(self.options.avatar) == 0 or not os.path.isfile(self.options.avatar):
            return

        friend_name = self.tox_friend_get_name(friend_number)
        self.verbose("Send avatar to {0}/{1}".format(friend_name, friend_number))

        f = EchoBotFile()

        f.kind = ToxCore.TOX_FILE_KIND_AVATAR
        f.size = os.path.getsize(self.options.avatar)
        f.read = True
        f.path = self.options.avatar
        f.fd   = open(f.path, "rb")

        data = f.fd.read()
        f.fd.seek(0, 0)

        f.id   = ToxCore.tox_hash(data)
        f.name = f.id

        file_number = self.tox_file_send(friend_number, ToxCore.TOX_FILE_KIND_AVATAR, f.size, f.id, f.name)

        if friend_number not in self.files:
            self.files[friend_number] = {}

        self.files[friend_number][file_number] = f


    def send_file(self, friend_number, path, name = None):
        """
        Отправка файла

        Аргументы:
            friend_number (int) -- Номер друга
            path          (str) -- Путь к файлу
            name          (str) -- Имя файла (опционально, если не совпадает с именем из path)
        """
        if not os.path.isfile(path):
            return

        friend_name = self.tox_friend_get_name(friend_number)

        if name is not None:
            self.verbose("Send file {0} as {1} to {2}/{3}".format(path, name, friend_name, friend_number))
        else:
            self.verbose("Send file {0} to {1}/{2}".format(path, friend_name, friend_number))

        f = EchoBotFile()

        f.kind = ToxCore.TOX_FILE_KIND_DATA
        f.size = os.path.getsize(path)
        f.read = True
        f.path = path
        f.fd   = open(f.path, "rb")
        f.name = name

        if f.name is None:
            f.name = os.path.basename(f.path)

        file_number = self.tox_file_send(friend_number, ToxCore.TOX_FILE_KIND_DATA, f.size, None, f.name)

        f.id = self.tox_file_get_file_id(friend_number, file_number)

        if friend_number not in self.files:
            self.files[friend_number] = {}

        self.files[friend_number][file_number] = f


    def tox_self_connection_status_cb(self, connection_status):
        """
        Изменение состояния соединения

        Аргументы:
            connection_status (int) -- Статус
        """
        if connection_status == ToxCore.TOX_CONNECTION_NONE:
            self.debug("Disconnected from DHT")
        elif connection_status == ToxCore.TOX_CONNECTION_TCP:
            self.debug("Connected to DHT via TCP")
        elif connection_status == ToxCore.TOX_CONNECTION_UDP:
            self.debug("Connected to DHT via UDP")
        else:
            raise NotImplementedError("Unknown connection_status: {0}".format(connection_status))


    def tox_friend_request_cb(self, public_key, message):
        """
        Запрос на добавление в друзья

        Аргументы:
            public_key (str) -- Публичный ключ друга
            message    (str) -- Сообщение запроса для добавления в друзья
        """
        self.verbose("Friend request from {0}: {1}".format(public_key, message))
        self.tox_friend_add_norequest(public_key)
        self.verbose("Friend request from {0}: accepted".format(public_key))


    def tox_friend_connection_status_cb(self, friend_number, connection_status):
        """
        Изменение состояния соединения друга

        Агрументы:
            friend_number     (int) -- Номер друга
            connection_status (int) -- Статус соединения друга (см. enum TOX_CONNECTION)
        """
        friend_name = self.tox_friend_get_name(friend_number)

        if connection_status == ToxCore.TOX_CONNECTION_NONE:
            self.verbose("Friend {0}/{1} is offline".format(friend_name, friend_number))
            if friend_number in self.files:
                for f in itervalues(self.files[friend_number]):
                    f.fd.close()
                del self.files[friend_number]
        elif connection_status == ToxCore.TOX_CONNECTION_TCP:
            self.verbose("Friend {0}/{1} connected via TCP".format(friend_name, friend_number))
        elif connection_status == ToxCore.TOX_CONNECTION_UDP:
            self.verbose("Friend {0}/{1} connected via UDP".format(friend_name, friend_number))
        else:
            raise NotImplementedError("Unknown connection_status: {0}".format(connection_status))

        if connection_status == ToxCore.TOX_CONNECTION_TCP or connection_status == ToxCore.TOX_CONNECTION_UDP:
            self.send_avatar(friend_number)


    def tox_friend_name_cb(self, friend_number, name):
        """
        Смена имени друга

        Аргументы:
            friend_number (int) -- Номер друга
            name          (str) -- Новое имя
        """
        self.verbose("Friend name change {0}/{1}".format(name, friend_number))


    def tox_friend_status_message_cb(self, friend_number, message):
        """
        Смена сообщения статуса

        Аргументы:
            friend_number (int) -- Номер друга
            message       (str) -- Сообщение статуса
        """
        friend_name = self.tox_friend_get_name(friend_number)

        self.verbose("Friend status message change {0}/{1}: {2}".format(friend_name, friend_number, message))


    def tox_friend_status_cb(self, friend_number, status):
        """
        Смена статуса

        Аргументы:
            friend_number (int) -- Номер друга
            status        (int) -- Статус (см. TOX_USER_STATUS)
        """
        friend_name = self.tox_friend_get_name(friend_number)

        if status ==ToxCore.TOX_USER_STATUS_NONE:
            self.verbose("Friend {0}/{1} is online now".format(friend_name, friend_number))
        elif status == ToxCore.TOX_USER_STATUS_AWAY:
            self.verbose("Friend {0}/{1} is away now".format(friend_name, friend_number))
        elif status == ToxCore.TOX_USER_STATUS_BUSY:
            self.verbose("Friend {0}/{1} is busy now".format(friend_name, friend_number))
        else:
            raise NotImplementedError("Unknown status: {0}".format(status))


    def tox_friend_message_cb(self, friend_number, message):
        """
        Сообщение от друга

        Аргументы:
            friend_number (int) -- Номер друга
            message       (str) -- Сообщение
        """
        friend_name = self.tox_friend_get_name(friend_number)

        self.verbose("Message from {0}/{1}: {2}".format(friend_name, friend_number, message))
        message_id = self.tox_friend_send_message(friend_number, ToxCore.TOX_MESSAGE_TYPE_NORMAL, message)
        self.verbose("Message {0} to {1}/{2}: {3}".format(message_id, friend_name, friend_number, message))


    def tox_friend_read_receipt_cb(self, friend_number, message_id):
        """
        Квитанция о доставке сообщения

        Аргументы:
            friend_number (int) -- Номер друга
            message_id    (int) -- ID сообщения
        """
        friend_name = self.tox_friend_get_name(friend_number)

        self.verbose("Message receipt {0} from {1}/{2}".format(message_id, friend_name, friend_number))


    def can_accept_file(self, friend_number, file_number, kind, file_size, filename):
        """
        Проверка, что файл можно принимать

        Аргументы:
            friend_number (int) -- Номер друга
            file_number   (int) -- Номер файла (случайный номер в рамках передачи)
            kind          (int) -- Значение файла (см. TOX_FILE_KIND)
            file_size     (int) -- Размер файла
            filename      (str) -- Имя файла

        Результат (bool):
            Флаг разрешения на принятие файла
        """
        # поток?
        if file_size <= 0:
            return False

        # ограничение количества отдновременных файлов до 10 в обе стороны
        if friend_number in self.files and len(self.files[friend_number]) >= 20:
            return False

        if kind == ToxCore.TOX_FILE_KIND_DATA:
            return (
                self.options.accept_files and
                (self.options.max_file_size == 0 or file_size <= self.options.max_file_size) and
                (os.path.isdir(self.options.files_path)))

        elif kind == ToxCore.TOX_FILE_KIND_AVATAR:
            return (
                self.options.accept_avatars and
                (self.options.max_avatar_size == 0 or file_size <= self.options.max_avatar_size) and \
                (os.path.isdir(self.options.avatars_path)))

        raise NotImplementedError("Unknown kind: {0}".format(kind))


    def tox_file_recv_cb(self, friend_number, file_number, kind, file_size, filename):
        """
        Получение файла
        (см. tox_file_recv_cb)

        Аргументы:
            friend_number (int) -- Номер друга
            file_number   (int) -- Номер файла (случайный номер в рамках передачи)
            kind          (int) -- Значение файла (см. TOX_FILE_KIND)
            file_size     (int) -- Размер файла
            filename      (str) -- Имя файла
        """
        friend_name = self.tox_friend_get_name(friend_number)

        if kind == ToxCore.TOX_FILE_KIND_DATA:
            file_id = self.tox_file_get_file_id(friend_number, file_number)
            self.verbose("File from {0}/{1}: number = {2}, size = {3}, id = {4}, name = {5}".format(friend_name, friend_number, file_number, file_size, file_id, filename))
        elif kind == ToxCore.TOX_FILE_KIND_AVATAR:
            if file_size != 0:
                file_id = self.tox_file_get_file_id(friend_number, file_number)
                self.verbose("Avatar from {0}/{1}: number = {2}, size = {3}, id = {4}".format(friend_name, friend_number, file_number, file_size, file_id))
            else:
                self.verbose("No Avatar from {0}/{1}: number = {2}".format(friend_name, friend_number, file_number))
        else:
            raise NotImplementedError("Unknown kind: {0}".format(kind))

        if self.can_accept_file(friend_number, file_number, kind, file_size, filename):
            f = EchoBotFile()

            f.kind  = kind
            f.size  = file_size
            f.write = True
            f.name  = filename
            f.id    = file_id

            if f.kind == ToxCore.TOX_FILE_KIND_DATA:
                f.path = self.options.files_path + "/" + f.id
            elif f.kind == ToxCore.TOX_FILE_KIND_AVATAR:
                f.path = self.options.avatars_path + "/" + f.id

            f.fd = open(f.path, "wb")

            if friend_number not in self.files:
                self.files[friend_number] = {}

            self.files[friend_number][file_number] = f

            self.tox_file_control(friend_number, file_number, ToxCore.TOX_FILE_CONTROL_RESUME)
        else:
            self.tox_file_control(friend_number, file_number, ToxCore.TOX_FILE_CONTROL_CANCEL)


    def tox_file_recv_control_cb(self, friend_number, file_number, control):
        """
        Контроль получения файла
        (см. tox_file_recv_control_cb)

        Аргументы:
            friend_number (int) -- Номер друга
            file_number   (int) -- Номер файла (случайный номер в рамках передачи)
            control       (int) -- Полученная команда контроля (см. TOX_FILE_CONTROL)
        """
        friend_name = self.tox_friend_get_name(friend_number)

        if control == ToxCore.TOX_FILE_CONTROL_RESUME:
            self.verbose("File resumed from {0}/{1}: number = {2}".format(friend_name, friend_number, file_number))
        elif control == ToxCore.TOX_FILE_CONTROL_PAUSE:
            self.verbose("File paused from {0}/{1}: number = {2}".format(friend_name, friend_number, file_number))
        elif control == ToxCore.TOX_FILE_CONTROL_CANCEL:
            self.verbose("File canceled from {0}/{1}: number = {2}".format(friend_name, friend_number, file_number))
            if friend_number in self.files and file_number in self.files[friend_number]:
                self.files[friend_number][file_number].fd.close()
                del self.files[friend_number][file_number]
        else:
            raise NotImplementedError("Unknown control: {0}".format(control))


    def tox_file_recv_chunk_cb(self, friend_number, file_number, position, data):
        """
        Получение чанка данных при приеме
        (см. tox_file_recv_chunk_cb)

        Аргументы:
            friend_number (int) -- Номер друга
            file_number   (int) -- Номер файла (случайный номер в рамках передачи)
            position      (int) -- Номер позиции
            data          (str) -- Данные
        """
        if friend_number not in self.files:
            return
        if file_number not in self.files[friend_number]:
            return

        f = self.files[friend_number][file_number]

        if f.write == False:
            return

        if f.position != position:
            f.fd.seek(position, 0)
            f.position = position

        if data is not None:
            f.fd.write(data)

            length = len(data)
            f.position += length
        else:
            length = 0

        if length == 0 or f.position > f.size:
            f.fd.close()
            del self.files[friend_number][file_number]

            if f.kind == ToxCore.TOX_FILE_KIND_DATA:
                self.send_file(friend_number, f.path, f.name)
        else:
            self.files[friend_number][file_number] = f


    def tox_file_chunk_request_cb(self, friend_number, file_number, position, length):
        """
        Запрос чанка данных для передачи
        (см. tox_file_chunk_request_cb)

        Аргументы:
            friend_number (int) -- Номер друга
            file_number   (int) -- Номер файла (случайный номер в рамках передачи)
            position      (int) -- Номер позиции
            length        (str) -- Требуемая длина чанка
        """
        if friend_number not in self.files:
            return
        if file_number not in self.files[friend_number]:
            return

        f = self.files[friend_number][file_number]

        if f.read == False:
            return

        if length == 0:
            f.fd.close()
            del self.files[friend_number][file_number]
            return

        if f.position != position:
            f.fd.seek(position, 0)
            f.position = position

        data = f.fd.read(length)
        f.position += len(data)

        self.files[friend_number][file_number] = f

        self.tox_file_send_chunk(friend_number, file_number, position, data)


if __name__ == "__main__":
    regexp  = re.compile("--config=(.*)")
    cfgfile = [match.group(1) for arg in sys.argv for match in [regexp.search(arg)] if match]

    if len(cfgfile) == 0:
        cfgfile = "echobot.cfg"
    else:
        cfgfile = cfgfile[0]

    options = EchoBotOptions(EchoBotOptions.loadOptions(cfgfile))

    bot = EchoBot(options)

    try:
        bot.run()
    except KeyboardInterrupt:
        bot.save_file()
