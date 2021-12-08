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

import uos
import gc
import ujson
import dataCall
#import backup_restore
#import app_fota
import misc

'''
Forrest.Liu - 2021/09/11
Perfect auto mount fs

'''
global datacall_flag
datacall_flag = 1

def _call_by_default_apn():
    dataCall.setAutoConnect(1, 1)
    dataCall.recordApn(1, 0, '', '', '', 0, 0)
    #dataCall.setAsynMode(1)
    ret = -1
    repeat_count = 2
    while (ret == -1) and (repeat_count >= 0):
        repeat_count -= 1
        ret = dataCall.start(1, 0, '', '', '', 0)
    #dataCall.setAsynMode(0)


def _call_by_user_apn():
    with open("/usr/user_apn.json", "r", encoding='utf-8') as fd:
        json_data = ujson.load(fd)

    pdp = json_data.get('pdp')
    ipv = json_data.get('iptype')
    apn = json_data.get('apn')
    usr = json_data.get('user')
    pwd = json_data.get('password')
    ath = json_data.get('authtype')

    dataCall.setAutoConnect(int(pdp), 1)

    dataCall.recordApn(int(pdp), int(ipv), apn, usr, pwd, int(ath), 1)
    #dataCall.setAsynMode(1)
    ret = -1
    repeat_count = 2
    while (ret == -1) and (repeat_count >= 0):
        repeat_count -= 1
        ret = dataCall.start(int(pdp), int(ipv), apn, usr, pwd, int(ath))
    #dataCall.setAsynMode(0)


def _auto_data_call():
    try:
        _call_by_user_apn()
    except OSError as e:
        _call_by_default_apn()
        #if e.args[0] == 2:
            # print('######user_apn.json not found!')
            #_call_by_default_apn()


def _check_data_call():
    for pdp in range(1, 5):
        nw_sta = dataCall.getInfo(pdp, 2)
        if (nw_sta != -1) and (nw_sta[2][0] == 0) and (nw_sta[3][0] == 0):
            continue
        elif (nw_sta != -1) and ((nw_sta[2][0] == 1) or (nw_sta[3][0] == 1)):
            return 1
    return 0

def _repl_enable():
    global datacall_flag
    if "system_config.json" in uos.listdir("usr/"):
        with open("/usr/system_config.json", "r", encoding='utf-8') as fd:
            json_data = ujson.load(fd)
            repl_flag = json_data.get("replFlag", 0)
            datacall_flag = json_data.get("datacallFlag",1)
            #misc.replEnable(repl_flag)
    else:
        with open("/usr/system_config.json", "w+", encoding='utf-8') as fd:
            repl_data = ujson.dumps({"replFlag": 0})
            fd.write(repl_data)


try:
    bdev = uos.VfsEfs("customer_fs")
    udev = uos.VfsEfs("customer_backup_fs")
    uos.mount(bdev, '/usr')
    uos.mount(udev, '/bak')
    print('mount.')

except Exception:
    print('error ocurs in boot step.')

finally:
    _repl_enable()
    if datacall_flag == 1:
        ret = _check_data_call()
        if ret == 0:
            _auto_data_call()

