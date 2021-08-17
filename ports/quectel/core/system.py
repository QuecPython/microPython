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

'''
@Author: Pawn
@Date: 2020-12-26
@LastEditTime: 2020-12-26 14:46:08
@FilePath: systeam.py
'''


def replSetEnable(flag=0):
    import uos
    import misc
    import ujson
    
    if "system_config.json" in uos.listdir("usr/"):
        datacall_flag = -1
        try:
            with open("/usr/system_config.json", "r", encoding='utf-8') as fd:
                json_data = ujson.load(fd)
                datacall_flag = json_data.get("datacallFlag",-1)
                
            with open("/usr/system_config.json", "w+", encoding='utf-8') as fd:
                #ujson.load()
                if datacall_flag != -1:
                    json_data = ujson.dumps({"replFlag": flag, "datacallFlag": datacall_flag})
                    fd.write(json_data)
                else:
                    repl_data = ujson.dumps({"replFlag": flag})
                    fd.write(repl_data)
        except:
            raise OSError("The system_config.JSON file is abnormal, please check!")
    misc.replEnable(flag)
    return 0