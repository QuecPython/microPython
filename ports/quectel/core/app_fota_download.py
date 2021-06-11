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

#!python3
# -*- coding:utf-8 -*-

import uos
import request
import ujson
import ql_fs

updater_dir = '/usr/.updater'
download_stat_file = '/usr/.updater/download.stat'
download_stat_file_max_size = 16384
update_flag_file = '/usr/.updater/update.flag'


def _get_download_stat_by_file(file_name):
    try:
        if not ql_fs.path_exists(download_stat_file):
            return None

        fp = open(download_stat_file, 'rt')
        if not fp:
            return None

        fr = fp.read(download_stat_file_max_size)
        fp.close()
        if not fr:
            return None

        download_stat = ujson.loads(fr)
        for item in download_stat:
            if item['name'].lower() == file_name.lower():
                return item
        return None
    except Exception as e:
        return None


def _get_download_stat():
    try:
        if not ql_fs.path_exists(download_stat_file):
            return None

        fp = open(download_stat_file, 'rt')
        if not fp:
            return None

        fr = fp.read(download_stat_file_max_size)
        fp.close()
        if not fr:
            return None

        download_stat = ujson.loads(fr)
        return download_stat
    except Exception as e:
        return None


def get_download_stat():
    return _get_download_stat()


def _fetch(url, fetched_size):
    request_headers = {}
    request_headers['Range'] = 'bytes={}-'.format(fetched_size)
    return request.get(url, headers=request_headers)


def _update_download_stat(url, file_name, total_size):
    need_append_stat = 1
    download_stat = _get_download_stat()
    if download_stat:
        for item in download_stat:
            if item['name'].lower() == file_name.lower():
                item['url'] = url
                item['total_size'] = total_size
                need_append_stat = 0
                break
    else:
        download_stat = []

    if need_append_stat:
        single_download_stat = {}
        single_download_stat['url'] = url
        single_download_stat['name'] = file_name
        single_download_stat['total_size'] = total_size
        download_stat.append(single_download_stat)

    json_str = ujson.dumps(download_stat)
    fp = open(download_stat_file, 'wt')
    fp.write(json_str)
    fp.close()


def update_download_stat(url, file_name, total_size):
    _update_download_stat(url, file_name, total_size)


def delete_update_file(file_name):
    download_stat = _get_download_stat()
    if download_stat:
        for item in download_stat[:]:
            if item['name'].lower() == file_name.lower():
                download_stat.remove(item)
    json_str = ujson.dumps(download_stat)
    fp = open(download_stat_file, 'wt')
    fp.write(json_str)
    fp.close()


def download(url, file_name):
    download_file_name = updater_dir + '/' + file_name
    ql_fs.mkdirs(ql_fs.path_dirname(download_file_name))

    single_download_stat = _get_download_stat_by_file(file_name)
    if single_download_stat:
        if not ql_fs.path_exists(download_file_name):
            fetched_size = 0
        else:
            fetched_size = ql_fs.path_getsize(download_file_name)

        if fetched_size == single_download_stat['total_size']:
            return 0
    else:
        fetched_size = 0

    r = _fetch(url, fetched_size)
    if r.status_code == 200 or r.status_code == 206:
        total_size = -1
        need_refresh_stat_again = 0
        file_open_mode = ''
        if not single_download_stat:
            total_size = 'unknown'
            _update_download_stat(url, file_name, total_size)
            file_open_mode = 'wb+'
        else:
            file_open_mode = 'ab+'

        fp = open(download_file_name, file_open_mode)
        content = r.content

        try:
            while True:
                c = next(content)
                length = len(c)
                for i in range(0, length, 4096):
                    fp.write(c[i:i + 4096])
        except StopIteration:
            fp.close()

            r.close()
        else:
            fp.close()
            uos.remove(download_file_name)
            r.close()
            return -1

        total_size = ql_fs.path_getsize(download_file_name)
        _update_download_stat(url, file_name, total_size)

        return 0
    else:
        r.close()
        return -1


def bulk_download(info=None):
    if info is None:
        info = []
    fail_result = []
    for item in info:
        url = item['url']
        file_name = item['file_name']
        if download(url, file_name) == -1:
            fail_result.append(item)
    if len(fail_result):
        return fail_result
    return None


def set_update_flag():
    ql_fs.mkdirs(ql_fs.path_dirname(update_flag_file))
    fp = open(update_flag_file, 'wb')
    fp.close()

