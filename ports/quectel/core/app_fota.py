#!python3
# -*- coding:utf-8 -*-

import app_fota_download
import app_fota_updater

class new(object):
    def download(self, url, file_name):
        app_fota_download.download(url, file_name)
    def bulk_download(self, info=[]):
        app_fota_download.bulk_download(info)
    def set_update_flag(self):
        app_fota_download.set_update_flag()
    def update(self):
        app_fota_updater.update()

