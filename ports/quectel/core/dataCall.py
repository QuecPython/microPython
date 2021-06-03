
"""
jayceon 2021/03/23
"""


import dial
import ujson

def start(profileidx, iptype, apn, username, password, authtype):
    return dial.start(profileidx, iptype, apn, username, password, authtype)


def getInfo(profileidx, iptype):
    return dial.getInfo(profileidx, iptype)


def setApn(profileidx, iptype, apn, username, password, authtype):
    with open("/usr/user_apn.json", "w+", encoding='utf-8') as fd:
        apn_dict = \
            {
                "pdp": str(profileidx),
                "iptype": str(iptype),
                "apn": apn,
                "user": username,
                "password": password,
                "authtype": str(authtype)
            }
        apn_data = ujson.dumps(apn_dict)
        fd.write(apn_data)
    return dial.start(profileidx, iptype, apn, username, password, authtype)


def recordApn(profileidx, iptype, apn, username, password, authtype, apntype):
    return dial.recordApn(profileidx, iptype, apn, username, password, authtype, apntype)


def setAsynMode(mode):
    return dial.setAsynMode(mode)


def setCallback(usrfun):
    return dial.setCallback(usrfun)


def setAutoConnect(profileidx, enable):
    return dial.setAutoConnect(profileidx, enable)