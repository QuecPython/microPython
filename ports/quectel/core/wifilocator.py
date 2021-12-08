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

import usocket as socket
import log
import net
import dataCall
import __wifiLocator

class wifilocator:

    def __init__(self, token=None):
        self.wifitoken = token

    def getwifilocator(self):
        net_sta = net.getState()
        if net_sta != -1 and ((net_sta[1][0] == 1) or (net_sta[1][0] == 5)):
            call_state = dataCall.getInfo(1, 0)
            if (call_state != -1) and (call_state[2][0] == 1):
                if len(self.wifitoken) != 16:
                    return -2
                try:
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sockaddr = socket.getaddrinfo('www.queclocator.com', 80)[0][-1]
                    sock.connect(sockaddr)
                    cellinfo = __wifiLocator.getWifilocreq(self.wifitoken)
                    senddata = b"POST /location/QLOC HTTP/1.0\r\nHost: www.queclocator.com\r\nContent-Length: {}\r\nContent-Type: 05\r\nAccept-Charset: utf-8\r\n\r\n{}".format(cellinfo[0], cellinfo[1])
                    sock.write(senddata)
                    l = sock.readline()
                    try:
                        l = l.split(None, 2)
                        status = int(l[1])
                    except:
                        raise ValueError("Connect FAIL!")
                    reason = ""
                    if status == 200:
                        while True:
                            l = sock.readline()
                            j = l.decode().split(":")
                            if not l or l == b"\r\n":
                                break
                        data = sock.recv(1024)
                        return __wifiLocator.encodeWifilocreq(data)
                    else:
                        if len(l) > 2:
                            reason = l[2].rstrip()
                            raise ValueError("error info:===='{}'====".format(reason))
                except Exception as e:
                    print("Wifi locator Get the coordinate error:%s "%str(e))
                    return -3
            else:
                return -1
        else:
            return -1


