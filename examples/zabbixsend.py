#!/usr/bin/env python
# -*- coding: utf-8 -*-

__title__    = "zabbixsend"
__author__   = "Anton Batenev"
__license__  = "BSD"

import sys

# PEP-3108
try:
    from urllib.parse   import urlencode as zabbix_urlencode
    from urllib.request import urlopen   as zabbix_urlopen
except ImportError:
    from urllib         import urlencode as zabbix_urlencode
    from urllib2        import urlopen   as zabbix_urlopen

HOST = "127.0.0.1"
PORT = "32445"

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: {0} <address> <subject> <message>".format(sys.argv[0]))
        sys.exit(1)

    address = str(sys.argv[1])
    subject = str(sys.argv[2])
    message = str(sys.argv[3])

    args = {
        "address" : address,
        "message" : message
    }

    try:
        url = "http://{0}:{1}/send?{2}".format(HOST, PORT, zabbix_urlencode(args))
        result = zabbix_urlopen(url)
    except Exception as e:
        sys.exit(1)

    sys.exit(0)
