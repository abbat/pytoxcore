#!/usr/bin/env python
# -*- coding: utf-8 -*-

__title__    = "toxdns"
__version__  = "0.0.17"
__author__   = "Anton Batenev"
__license__  = "BSD"

import dns.resolver

from pytoxcore import ToxDNS, ToxDNSException


def last_txt_record(host):
    response = dns.resolver.query(host, "TXT")
    if response:
        result = None
        for record in response:
           result = record
        return str(result).replace('"', "")
    return None


def tox_dns3_id(record):
    parts = record.split(";")
    for part in parts:
        kv = part.split("=")
        if len(kv) != 2:
            return None

        k = kv[0]
        v = kv[1]
        if k == "v" and v != "tox3":
            return None
        elif k == "id":
            return v

    return None


if __name__ == "__main__":
    request = "abbat@toxme.io"

    print("Request: {0}".format(request))

    parts = request.split("@")
    if len(parts) != 2:
        exit("User name must be user@domain")

    user   = parts[0]
    domain = parts[1]

    print("User: {0}".format(user))
    print("Domain: {0}".format(domain))

    host = "_tox.{0}".format(domain)
    key  = last_txt_record(host)
    if key == None:
        exit("No TXT record found for {0}".format(host))
    else:
        print("Public key: {0}".format(key))

    if len(key) != ToxDNS.TOX_CLIENT_ID_SIZE * 2:
        exit("Public key length = {0}, but {1} required".format(len(key), ToxDNS.TOX_CLIENT_ID_SIZE * 2))

    tox_dns = ToxDNS(key)

    id_record, request_id = tox_dns.tox_generate_dns3_string(user)

    print("Request record: {0}".format(id_record))
    print("Request Id: {0}".format(request_id))

    host   = "_{0}._tox.{1}".format(id_record, domain)
    record = last_txt_record(host)
    if record == None:
        exit("No TXT record found for {0}".format(host))
    else:
        print("Record: {0}".format(record))

    record = tox_dns3_id(record)
    if record == None:
        exit("Can not parse record")

    address = tox_dns.tox_decrypt_dns3_TXT(record, request_id)

    print("Address: {0}".format(address))
