#!/usr/bin/env python
# -*- coding: utf-8 -*-

__title__    = "toxdns"
__version__  = "0.0.17"
__author__   = "Anton Batenev"
__license__  = "BSD"

import dns.resolver

from pytoxcore import ToxDNS, ToxDNSException


class ToxDNSResolver(object):
    """
    Ресолвер адресов Tox
    """
    def __init__(self, v1_fallback = True, cache_keys = True):
        """
        Аргументы:
            v1_fallback (bool) -- Разрешить fallback на tox1
            cache_keys  (bool) -- Кэшировать публичные ключи
        """
        self.v1_fallback = v1_fallback
        self.cache_keys  = cache_keys

        self.cached_keys = {}


    def resolve_txt_record(self, query):
        """
        Получение TXT записи из DNS

        Аргументы:
            query (str) -- Запрос (имя домена)

        Результат (str)
            Значение TXT записи
        """
        response = dns.resolver.query(query, "TXT")
        if response:
            for record in response:
                return str(record).replace('"', "")

        raise ValueError("TXT record not found for {0}".format(query))


    def parse_txt_record(self, record):
        """
        Разбор TXT записи

        Аргументы:
            record (str) -- Значение TXT записи

        Результат (dict)
            Словарь ключей и значений в записи
        """
        result = {}

        comma_parts = record.split(";")
        for comma_part in comma_parts:
            comma_part = comma_part.strip()
            if len(comma_part) == 0:
                continue

            keyval = comma_part.split("=", 1)
            if len(keyval) != 2:
                continue

            key   = keyval[0].strip()
            value = keyval[1].strip()

            result[key] = value

        return result


    def is_v_record(self, record, version):
        """
        Проверка записи на соответствие версии tox

        Аргументы:
            record  (dict) -- Значение записи
            version (int)  -- Требуемая версия

        Результат (bool)
            True, если запись требуемой версии
        """
        if not ("id" in record and "v" in record):
            return False
        if record["v"].lower() != "tox{0}".format(version):
            return False
        return True


    def is_v3_record(self, record):
        """
        Проверка записи на соответствие версии tox3

        Аргументы:
            record (dict) -- Значение записи

        Результат (bool)
            True, если запись версии tox3
        """
        return self.is_v_record(record, 3)


    def is_v1_record(self, record):
        """
        Проверка записи на соответствие версии tox1

        Аргументы:
            record (dict) -- Значение записи

        Результат (bool)
            True, если запись версии tox1
        """
        return self.is_v_record(record, 1)


    def user_domain(self, request):
        """
        Получение части имени пользователя и домена из запроса

        Аргументы:
            request (str) -- Запрос

        Результат (tuple): user (str), domain (str)
            Имя пользователя и домен
        """
        request_parts = request.split("@", 1)
        if len(request_parts) != 2:
            raise ValueError("Request must be user@domain form.")

        return (request_parts[0].lower(), request_parts[1].lower())


    def public_key(self, domain):
        """
        Получение публичного ключа для домена

        Аргументы:
            domain (str) -- Домен

        Результат (str):
            Публичный ключ
        """
        if self.cache_keys and domain in self.cached_keys:
            return self.cached_keys[domain]["key"]

        request = "_tox.{0}".format(domain)
        record  = self.resolve_txt_record(request)

        if len(record) != ToxDNS.TOX_CLIENT_ID_SIZE * 2:
            raise ValueError("TXT record length for {0} is {1}, but {2} required".format(request, len(record), ToxDNS.TOX_CLIENT_ID_SIZE * 2))

        if self.cache_keys:
            self.cached_keys[domain] = { "key": record, "dns": ToxDNS(record) }

        return record


    def tox_dns(self, domain, public_key):
        """
        Получение экземпляра ToxDNS (из кэша или создание нового)

        Аргументы:
            domain     (str) -- Домен
            public_key (str) -- Публичный ключ

        Результат (ToxDNS):
            Экземпляр ToxDNS
        """
        if self.cache_keys and domain in self.cached_keys:
            return self.cached_keys[domain]["dns"]
        return ToxDNS(public_key)


    def tox3_request(self, request):
        """
        Выполнение tox3 запроса

        Аргументы:
            request (str) -- Запрос (user@domain)

        Результат (str):
            ToxID пользователя
        """
        user, domain = self.user_domain(request)

        public_key = self.public_key(domain)
        tox_dns    = self.tox_dns(domain, public_key)

        record_id, request_id = tox_dns.tox_generate_dns3_string(user)

        request = "_{0}._tox.{1}".format(record_id, domain)
        record  = self.resolve_txt_record(request)
        record  = self.parse_txt_record(record)

        if self.is_v3_record(record) == False:
            raise ValueError("TXT record for {0} is not {1} record".format(request, "tox3"))

        return tox_dns.tox_decrypt_dns3_TXT(record["id"], request_id)


    def tox1_request(self, request):
        """
        Выполнение tox1 запроса

        Аргументы:
            request (str) -- Запрос (user@domain)

        Результат (str):
            ToxID пользователя
        """
        user, domain = self.user_domain(request)

        request = "{0}._tox.{1}".format(user, domain)
        record  = self.resolve_txt_record(request)
        record  = self.parse_txt_record(record)

        if self.is_v1_record(record) == False:
            raise ValueError("TXT record for {0} is not {1} record".format(request, "tox1"))

        if "sign" in record:
            # TODO: sign == crypto_sign_ed25519(user + tox_id)
            pass

        return record["id"]


    def tox_request(self, request):
        """
        Выполнение tox-dns запроса

        Аргументы:
            request (str) -- Запрос (user@domain)

        Результат (str):
            ToxID пользователя
        """
        try:
            return self.tox3_request(request)
        except ValueError as e:
            if not self.v1_fallback:
                raise e
            return self.tox1_request(request)


if __name__ == "__main__":
    resolver = ToxDNSResolver()

    print(resolver.tox1_request("tox@toxme.io"))
    print(resolver.tox3_request("tox@toxme.io"))
    print(resolver.tox_request("tox@toxme.io"))
