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


'''
Forrest.Liu - 2021/06/26
Perfect auto mount fs

'''


try:
    bdev = uos.VfsLfs1(32, 32, 32, "customer_fs")
    udev = uos.VfsLfs1(32, 32, 32, "customer_backup_fs")
    uos.mount(bdev, '/usr')
    uos.mount(udev, '/bak')
    print('mount.')

except Exception:
    print('error ocurs in boot step.')

