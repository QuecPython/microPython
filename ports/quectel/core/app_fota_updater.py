#!python3
# -*- coding:utf-8 -*-

import uos
import ujson
import ql_fs
import checksum

updater_dir = '/usr/.updater'
download_stat_file = '/usr/.updater/download.stat'
download_stat_file_max_size = 16384
update_flag_file = '/usr/.updater/update.flag'

def _check_update_flag():
    try:
        if ql_fs.path_exists(update_flag_file):
            return 1
        else:
            return 0
    except Exception as e:
        return 0

def update():
    if _check_update_flag():
        try:
            if not ql_fs.path_exists(download_stat_file):
                return -1

            fp = open(download_stat_file, 'rt')
            if not fp:
                return -1

            fr = fp.read(download_stat_file_max_size)
            fp.close()
            if not fr:
                return -1
            download_stat = ujson.loads(fr)
            print(download_stat)
            download_stat_tmp = download_stat[:]
            print(download_stat_tmp)
            for item in download_stat:
                file_name = item['name']
                download_file_name = updater_dir + '/' + file_name
                if ql_fs.path_exists(download_file_name):
                    ql_fs.mkdirs(ql_fs.path_dirname(file_name))
                    ql_fs.file_copy(file_name, download_file_name)
                    checksum.update(file_name)
                download_stat_tmp.remove(item)
                json_str = ujson.dumps(download_stat_tmp)
                fp = open(download_stat_file, 'wt')
                if not fp:
                    return -1
                fp.write(json_str)
                fp.close()
                uos.remove(download_file_name)
            uos.remove(update_flag_file)
            uos.remove(download_stat_file)
            ql_fs.rmdirs(updater_dir)
            return 0
                
        except Exception as e:
            return -1
    return -1