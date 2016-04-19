#!/usr/bin/env python
# -*- coding: utf-8 -*-

__title__    = "zabbixbot"
__author__   = "Anton Batenev"
__license__  = "BSD"

import sys
import os
import time
import re
import flask
import threading

from pytoxcore import ToxCore, ToxCoreException


# PEP-8
try:
    import configparser as ZabbixBotConfigParser
except ImportError:
    import ConfigParser as ZabbixBotConfigParser


app = flask.Flask(__name__)


class ZabbixBotOptions(object):
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
        self.bind           = str(options["bind"])
        self.port           = int(options["port"])
        self.name           = str(options["name"])
        self.status_message = str(options["status_message"])
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
            "debug"           : "no",
            "verbose"         : "yes",
            "bind"            : "127.0.0.1",
            "port"            : "32445",
            "name"            : "ZabbixBot",
            "status_message"  : "Think Stability",
            "save_file"       : "zabbixbot.data",
            "save_tmp_file"   : "zabbixbot.data.tmp",
            "save_interval"   : "300",
            "bootstrap_host"  : "178.62.250.138",   # https://wiki.tox.chat/users/nodes
            "bootstrap_port"  : "32445",
            "bootstrap_key"   : "788236D34978D1D5BD822F0A5BEBD2C53C64CC31CD3149350EE27D4D9A2F9B6B",
            "ipv6_enabled"    : "yes" if tox_opts["ipv6_enabled"] else "no",
            "udp_enabled"     : "yes" if tox_opts["udp_enabled"]  else "no",
            "proxy_type"      : "",
            "proxy_host"      : "" if tox_opts["proxy_host"] is None else tox_opts["proxy_host"],
            "proxy_port"      : str(tox_opts["proxy_port"]),
            "start_port"      : str(tox_opts["start_port"]),
            "end_port"        : str(tox_opts["end_port"]),
            "tcp_port"        : str(tox_opts["tcp_port"])
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
        Чтение секции echobot INI файла zabbixbot.cfg

        Аргументы:
            filename (str)  -- Имя INI файла
            options  (dict) -- Базовая конфигурация

        Результат (dict):
            Конфигурация приложения на основе файла конфигурации
        """
        if options is None:
            options = ZabbixBotOptions.defaultOptions()

        options = options.copy()

        parser = ZabbixBotConfigParser.ConfigParser()

        parser.read(filename)

        for section in parser.sections():
            name = section.lower()
            if name == "zabbixbot":
                for option in parser.options(section):
                    options[option.lower()] = parser.get(section, option).strip()

        return options


class ZabbixBot(ToxCore):
    """
    Бот
    """
    def __init__(self, options):
        """
        Аргументы:
            options (ZabbixBotOptions) -- Опции приложения
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
                tox_opts["savedata_type"] = ToxCore.TOX_SAVEDATA_TYPE_TOX_SAVE
                tox_opts["savedata_data"] = f.read()

        super(ZabbixBot, self).__init__()

        self.debug("Set self name: {0}".format(self.options.name))
        self.tox_self_set_name(self.options.name)

        self.debug("Set self status: {0}".format(self.options.status_message))
        self.tox_self_set_status_message(self.options.status_message)

        self.debug("Get self ToxID: {0}".format(self.tox_self_get_address()))

        self.running = True

        self.thread = threading.Thread(target = self.run)
        self.thread.start()


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
        self.running = False
        self.thread.join()


    def run(self):
        self.debug("Connecting to: {0} {1} {2}".format(self.options.bootstrap_host, self.options.bootstrap_port, self.options.bootstrap_key))
        self.tox_bootstrap(self.options.bootstrap_host, self.options.bootstrap_port, self.options.bootstrap_key)
        self.debug("Connected to: {0} {1} {2}".format(self.options.bootstrap_host, self.options.bootstrap_port, self.options.bootstrap_key))

        checked       = False
        savetime      = 0
        save_interval = self.options.save_interval * 1000

        while self.running:
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

            time.sleep(float(interval) / 1000.0)

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


def rest_success(result):
    return "{0}\n".format(result)


def rest_error(result, code = 500):
    return ("{0}\n".format(result), code)


@app.route("/status", methods=["GET"])
def rest_status():
    try:
        status = bot.tox_self_get_connection_status()
        if status == ToxCore.TOX_CONNECTION_NONE:
            result = "NONE"
        elif status == ToxCore.TOX_CONNECTION_TCP or status == ToxCore.TOX_CONNECTION_UDP:
            result = "ONLINE"
        else:
            result = "UNKNOWN"

        return rest_success(result)
    except ToxCoreException as e:
        return rest_error(e.message)


@app.route("/address", methods=["GET"])
def rest_address():
    try:
        result = bot.tox_self_get_address()
        return rest_success(result)
    except ToxCoreException as e:
        return rest_error(e.message)


@app.route("/send", methods=["GET"])
def rest_message():
    try:
        address = flask.request.args.get("address", "")
        message = flask.request.args.get("message", "")

        public_key = address[0:ToxCore.TOX_PUBLIC_KEY_SIZE * 2]

        try:
            friend_number = bot.tox_friend_by_public_key(public_key)
        except ToxCoreException as e:
            result = bot.tox_friend_add(address, message)
            return rest_success(result)

        result = bot.tox_friend_send_message(friend_number, ToxCore.TOX_MESSAGE_TYPE_NORMAL, message)
        return rest_success(result)
    except ToxCoreException as e:
        return rest_error(e.message)


if __name__ == "__main__":
    regexp  = re.compile("--config=(.*)")
    cfgfile = [match.group(1) for arg in sys.argv for match in [regexp.search(arg)] if match]

    if len(cfgfile) == 0:
        cfgfile = "zabbixbot.cfg"
    else:
        cfgfile = cfgfile[0]

    options = ZabbixBotOptions(ZabbixBotOptions.loadOptions(cfgfile))

    bot = ZabbixBot(options)

    app.run(host = options.bind, port = options.port, debug = options.debug)

    bot.stop()
