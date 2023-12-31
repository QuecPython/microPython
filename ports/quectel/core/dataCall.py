# Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
#  
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#  
#     http://www.apache.org/licenses/LICENSE-2.0
#  
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


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
    retval = dial.getPdpRange()
    min = retval[0]
    max = retval[1]
    if profileidx < min or profileidx > max:
        raise ValueError("invalid value, profileIdx should be in [{},{}].".format(min, max))
    if iptype < 0 or iptype > 2:
        raise ValueError("invalid value, iptype should be in [0,2].")
    if len(apn) > 63:
        raise ValueError("invalid value, the length of apn should be no more than 63 bytes.")
    if len(username) > 15:
        raise ValueError("invalid value, the length of username should be no more than 15 bytes.")
    if len(password) > 15:
        raise ValueError("invalid value, the length of password should be no more than 15 bytes.")
    if authtype < 0 or authtype > 2:
        raise ValueError("invalid value, authtype should be in [0,2].")
        
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

def getApn(*args):
    return dial.getApn(*args)

def setDnsserver(profileidx, simid, new_pri, new_sec):
    return dial.setDnsserver(profileidx, simid, new_pri, new_sec)