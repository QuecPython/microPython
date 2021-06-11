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

import utime
import urandom
import hmac
import hashlib
import base64
import _thread
import hmacSha1
import ucryptolib
import request
import ujson
import uos
from machine import Timer
from umqtt import MQTTClient


class TXyun:

    def __init__(self, productID, devicename, devicePsk, ProductSecret=None):

        self.productID = productID
        self.devicename = devicename
        self.devicePsk = devicePsk
        self.ProductSecret = ProductSecret

        self.mqtt_server = "{}.iotcloud.tencentdevices.com".format(productID)
        self.username = None
        self.password = None
        # 鐢熸垚 MQTT 鐨� clientid 閮ㄥ垎, 鏍煎紡涓� ${productid}${devicename}
        self.clientid = "{}{}".format(productID, devicename)

        self.mqtt_client = None
        self.callback = self.setCallback
        self.recvCb = None
        self.url = "http://ap-guangzhou.gateway.tencentdevices.com/register/dev"
        self.flag = True
        self.mqttObj = True
        
    def setMqtt(self, clean_session=True, keepAlive=300, reconn=True):
        if (not self.mqttObj) and (self.mqtt_client != None):
            return self.mqtt_client
        if self.ProductSecret != None:
            self.DynamicConnectInfo()
            try:
                self.mqtt_client = self.connect(keepAlive, clean_session, reconn=reconn)
                return 0
            except:
                return -1
        else:
            try:
                self.mqtt_client = self.connect(keepAlive, clean_session,reconn=reconn)
                return 0
            except:
                return -1

    def connect(self, keepAlive, clean_session, reconn):
        self.formatConnectInfo()
        mqtt_client = MQTTClient(self.clientid, self.mqtt_server, 1883, self.username, self.password,keepAlive,reconn=reconn)
        mqtt_client.connect(clean_session=clean_session)
        self.mqttObj = False
        mqtt_client.set_callback(self.proc)
        return mqtt_client

    def proc(self, topic, msg):
        return self.recvCb(topic, msg)

    def formatConnectInfo(self):
        expiry = int(utime.mktime(utime.localtime())) + 60 * 60
        connid = self.rundom()

        self.username = "{};12010126;{};{}".format(self.clientid, connid, expiry)
        try:
            token = hmac.new(base64.b64decode(self.devicePsk), msg=bytes(self.username, "utf8"),
                         digestmod=hashlib.sha256).hexdigest()
        except:
            raise ("Key generation exception!Please check the device properties")
        self.password = "{};{}".format(token, "hmacsha256")


    def DynamicConnectInfo(self):
        if "tx_secret.json" in uos.listdir("usr/"):
            msg = check_secret(self.devicename)
            if msg:
                self.devicePsk = msg
                self.formatConnectInfo()
                return 0

        numTime = utime.mktime(utime.localtime())
        nonce = self.rundom()
        msg = "deviceName={}&nonce={}&productId={}&timestamp={}".format(self.devicename, nonce, self.productID ,numTime)
        hmacInfo = hmacSha1.hmac_sha1(self.ProductSecret, msg)
        base64_msg = base64.b64encode(bytes(hmacInfo, "utf8"))

        data = {
            "deviceName": self.devicename,
            "nonce": nonce,
            "productId": self.productID,
            "timestamp": numTime,
            "signature": base64_msg.decode()
        }

        data_json = ujson.dumps(data)

        response = request.post(self.url, data=data_json)
        res_raw = response.json()

        code = res_raw.get("code")
        if code == 0:
            payload = res_raw.get("payload")
            mode = ucryptolib.MODE_CBC
            raw = base64.b64decode(payload)
            key = self.ProductSecret[0:16].encode()
            iv = b"0000000000000000"
            cryptos = ucryptolib.aes(key, mode, iv)
            plain_text = cryptos.decrypt(raw)
            data = plain_text.decode().split("\x00")[0]
            self.devicePsk = ujson.loads(data).get("psk")
            data = {self.devicename: self.devicePsk}
            save_Secret(data)
            self.formatConnectInfo()
        else:
            print("[WARNING] Registration failed. Please check if the device is activated")
            return -1


    def setCallback(self, callback):
        self.recvCb = callback

    def subscribe(self, topic, qos=0):
        try:
            self.mqtt_client.subscribe(topic, qos)
            return 0
        except OSError as e:
            print("[WARNING] subscribe failed. Try to reconnect : %s" % str(e))
            return -1

    def publish(self, topic, msg, retain=False, qos=0):
        try:
            self.mqtt_client.publish(topic, msg, retain, qos)
            return 0
        except OSError as e:
            print("[WARNING] Publish failed. Try to reconnect : %s" % str(e))
            return -1

    def disconnect(self):
        self.flag = False
        self.mqttObj = True
        self.mqtt_client.disconnect()

    def close(self):
        self.mqtt_client.close()

    def ping(self):
        self.mqtt_client.ping()

    def getTXyunsta(self):
        txyunsta = self.mqtt_client.get_mqttsta()
        return txyunsta
        
    '''
    def __loop_forever(self, t):
        # print("loop_forever")
        try:
            self.mqtt_client.ping()
        except:
            return -1
    '''

    def __listen(self):
        while True:
            try:
                if not self.flag:
                    break
                else:
                    self.mqtt_client.wait_msg()
            except OSError:
                return -1

    def start(self):
        _thread.start_new_thread(self.__listen, ())
        # t = Timer(1)
        # t.start(period=20000, mode=t.PERIODIC, callback=self.__loop_forever)

    def rundom(self):
        msg = ""
        for i in range(0, 5):
            num = urandom.randint(1, 10)
            msg += str(num)
        return int(msg)


def save_Secret(data):
    secret_data = {}
    if "tx_secret.json" in uos.listdir("usr/"):
        with open("/usr/tx_secret.json", "r", encoding="utf-8") as f:
            secret_data = ujson.load(f)
            print(secret_data)
    try:
        with open("/usr/tx_secret.json", "w+", encoding="utf-8") as w:
            secret_data.update(data)
            ujson.dump(secret_data, w)
    except Exception as e:
        print("[ERROR] File write failed : %s" % str(e))


def check_secret(deviceName):
    try:
        with open("/usr/tx_secret.json", "r", encoding="utf-8") as f:
            secret_data = ujson.load(f)
    except Exception as e:
        print("[ERROR] File Open failed : %s" % str(e))
    device_secret = secret_data.get(deviceName, None)
    if device_secret != None:
        return device_secret
    return False


