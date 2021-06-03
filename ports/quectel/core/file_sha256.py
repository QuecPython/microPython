#!python3
# -*- coding:utf-8 -*-

import hashlib

READ_BLOCK_SIZE = 4096

def _check(fp):
	fp.seek(0)
	fr = fp.read(READ_BLOCK_SIZE)
	while fr:
		yield fr
		fr = fp.read(READ_BLOCK_SIZE)
	else:
		fp.seek(0)

def calc(file_name):
    sha256 = hashlib.sha256()
    flag = 0
    try:
        with open(file_name, 'rb') as fp:
            for fr in _check(fp):
                sha256.update(fr)
                flag = 1
            fp.close()
    except Exception as e:
        pass
    finally:
        if not flag:
            return None
        return sha256.hexdigest()
