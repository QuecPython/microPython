import gc
import uos
import ujson
import dataCall
import backup_restore
import app_fota
import misc


'''
Jayceon.Fu - 2020/10/14
Perfect auto datacall function

Jayceon.Fu - 2020/10/24
Modify the restart data call, from synchronous datacall to asynchronous datacall
'''

def _call_by_default_apn():
    dataCall.setAutoConnect(1, 1)
    dataCall.recordApn(1, 0, '', '', '', 0, 0)
    dataCall.setAsynMode(1)
    ret = -1
    repeat_count = 2
    while (ret == -1) and (repeat_count >= 0):
        repeat_count -= 1
        ret = dataCall.start(1, 0, '', '', '', 0)
    dataCall.setAsynMode(0)


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
    dataCall.setAsynMode(1)
    ret = -1
    repeat_count = 2
    while (ret == -1) and (repeat_count >= 0):
        repeat_count -= 1
        ret = dataCall.start(int(pdp), int(ipv), apn, usr, pwd, int(ath))
    dataCall.setAsynMode(0)


def _auto_data_call():
    try:
        _call_by_user_apn()
    except OSError as e:
        if e.args[0] == 2:
            # print('######user_apn.json not found!')
            _call_by_default_apn()


def _check_data_call():
    for pdp in range(1, 8):
        nw_sta = dataCall.getInfo(pdp, 2)
        if (nw_sta != -1) and (nw_sta[2][0] == 0) and (nw_sta[3][0] == 0):
            continue
        elif (nw_sta != -1) and ((nw_sta[2][0] == 1) or (nw_sta[3][0] == 1)):
            return 1
    return 0

def _repl_enable():
    if "system_config.json" in uos.listdir("usr/"):
        with open("/usr/system_config.json", "r", encoding='utf-8') as fd:
            json_data = ujson.load(fd)
            repl_flag = json_data.get("replFlag", 0)
            misc.replEnable(repl_flag)
    else:
        with open("/usr/system_config.json", "w+", encoding='utf-8') as fd:
            repl_data = ujson.dumps({"replFlag": 0})
            fd.write(repl_data)

# bdev = uos.VfsLfs1(32, 32, 32, "customer_fs")
# udev = uos.VfsLfs1(32, 32, 32, "customer_backup_fs")

try:
    bdev = uos.VfsLfs1(32, 32, 32, "customer_fs")
    udev = uos.VfsLfs1(32, 32, 32, "customer_backup_fs")
    uos.mount(bdev, '/usr')
    uos.mount(udev, '/bak')
    print('mount.')
    backup_restore.main() # backup_restore main process
    
    fota = app_fota.new() # app fota update main process
    fota.update()
    
except Exception:
    print('error ocurs in boot step.')

finally:
    ret = _check_data_call()
    if ret == 0:
        _auto_data_call()
_repl_enable()

