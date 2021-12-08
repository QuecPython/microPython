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

from machine import UART
import utime
import ure

fixFlag=0 #是否定位成功标志
class GnssGetData:
    '''
    GPS数据采集及解析
    '''
    def __init__(self,uartn,baudrate,databits,parity,stopbits,flowctl):
        if uartn == 0:
            UARTN=UART.UART0
        elif uartn == 1:
            UARTN=UART.UART1
        elif uartn == 2:
            UARTN=UART.UART2
        elif uartn == 3:
            UARTN=UART.UART3
        else:
            UARTN=UART.UART1
        self.uart = UART(UARTN, baudrate, databits, parity, stopbits, flowctl)
        utime.sleep(3) # Delay to read data stability 

    #读取GNSS数据并加以解析
    def read_gnss_data(self):
        try:
            buf = self.uart.read(self.uart.any())
            gps_data = buf.decode().strip("b")
            self.r = ure.search("GNGGA(.+?)M", gps_data)
            self.r1 = ure.search("GNRMC(.+?)M", gps_data)
            self.r2 = ure.search("GPGSV(.+?)M", gps_data)
            self.r3 = ure.search("GNVTG(.+?)M", gps_data)
            global fixFlag
            if self.r1 is None:
                fixFlag = 0
            else:
                if self.r1.group(0).split(",")[2] == 'A': #有效定位
                    fixFlag=1
                else:
                    fixFlag=0
        except:
            print("Exception:read gnss data error!!!!!!!!")
            raise
    #获取GPS模块是否定位成功
    def isFix(self):
        global fixFlag
        return fixFlag
    #获取GPS模块定位的经纬度信息
    def getLocation(self):
        if self.isFix() is 1:
            if self.r is None:
                return -1
            lat=float(self.r.group(0).split(",")[2])//100 +float(float(float(self.r.group(0).split(",")[2])%100)/60) #
            lat_d=self.r.group(0).split(",")[3]  #
            log=float(self.r.group(0).split(",")[4])//100 +float(float(float(self.r.group(0).split(",")[4])%100)/60) #
            log_d=self.r.group(0).split(",")[5]  #
            return lat,lat_d,log,log_d
        else:
            return -1
    #获取GPS模块授时的UTC时间
    def getUtcTime(self):
        if self.r is None:
            return -1
        return self.r.group(0).split(",")[1]
    #获取GPS模块定位模式
    def getLocationMode(self):
        if self.r is None:
            return -1
        if self.r.group(0).split(",")[6] is '0':
            #print('定位不可用或者无效')
            return 0
        if self.r.group(0).split(",")[6] is '1':
            #print('定位有效,定位模式：GPS、SPS 模式')
            return 1
        if self.r.group(0).split(",")[6] is '2':
            #print('定位有效,定位模式： DGPS、DSPS 模式')
            return 2
 
    #获取GPS模块定位使用卫星数量
    def getUsedSateCnt(self):
        if self.r is None:
            return -1
        return self.r.group(0).split(",")[7]
    #获取GPS模块定位可见卫星数量
    def getViewedSateCnt(self):
        if self.r2 is None:
            return -1
        return self.r2.group(0).split(",")[3]
    #获取GPS模块定位方位角 范围：0~359。以真北为参考平面。
    def getCourse(self):
        if self.r2 is None:
            return -1
        return self.r2.group(0).split(",")[6]
    #获取GPS模块对地速度(单位:KM/h)
    def getSpeed(self):
        if self.r1 is None:
            return -1
        if self.r1.group(0).split(",")[7] == '':
            return None
        else:
            return float(self.r1.group(0).split(",")[7]) * 1.852
    #获取GPS模块定位大地高(单位:米)
    def getGeodeticHeight(self):
        if self.r is None:
            return -1
        return self.r.group(0).split(",")[9]

    
    