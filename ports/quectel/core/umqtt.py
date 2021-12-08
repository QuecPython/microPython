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
import net
import dataCall
import log
import _thread
import usocket as socket
import ustruct as struct

log.basicConfig(level=log.INFO)
mqtt_log = log.getLogger("MQTT")


class MQTTException(Exception):
    pass

def pid_gen():
    pid = 0
    while True:
        pid = pid + 1 if pid < 65535 else 1
        yield pid

class BaseMqtt:
    CONNECTEXCE = -1
    CONNECTSUCCESS = 0
    ARECONNECT = 1
    SEVCLOSE = 2

    def __init__(self, client_id, server, port=0, user=None, password=None, keepalive=0,
                 ssl=False, ssl_params={}, reconn=True, version=4, pingmaxnum=0):
        if port == 0:
            port = 8883 if ssl else 1883
        self.client_id = client_id
        self.sock = None
        self.server = server
        self.port = port
        self.ssl = ssl
        self.ssl_params = ssl_params
        self.pid = 0
        self.__newpid = pid_gen()
        self.__rcv_pids = set()  # PUBACK and SUBACK pids awaiting ACK response
        self.cb = None
        self.user = user
        self.pswd = password
        self.keepalive = keepalive
        self.lw_topic = None
        self.lw_msg = None
        self.lw_qos = 0
        self.lw_retain = False
        self.last_time = None
        self.timerFlag = True
        self.pingFlag = False
        self.connSta = False
        self.reconn = reconn
        self.topic_list = []
        self.qos = 0
        self.mqttversion = version
        self.clean_session = True
        self.ping_outstanding = False
        self.pingmaxnum = pingmaxnum
        self.pingnum = 0
        self.mqttlock = None
        self.mqttsendlock = None
        self.threadId = None
        self.__response_time = 5000
        if keepalive != 0 and keepalive < 5:
            raise ValueError("inport parameter error : keepalive >= 5S")

    def _send_str(self, s):
        self.sock.write(struct.pack("!H", len(s)))
        self.sock.write(s)

    def _recv_len(self):
        n = 0
        sh = 0
        while 1:
            b = self.sock.read(1)[0]
            n |= (b & 0x7f) << sh
            if not b & 0x80:
                return n
            sh += 7

    def set_callback(self, f):
        self.cb = f

    def set_last_will(self, topic, msg, retain=False, qos=0):
        assert 0 <= qos <= 2
        assert topic
        self.lw_topic = topic
        self.lw_msg = msg
        self.lw_qos = qos
        self.lw_retain = retain

    def connect(self, clean_session=True):

        if self.mqttlock is None:
            self.mqttlock = _thread.allocate_lock()
        if self.mqttsendlock is None:
            self.mqttsendlock = _thread.allocate_lock()
        j = 1
        while True:
            if j > 5:
                raise ValueError("DNS Parsing NULL")
            try:
                addrall = socket.getaddrinfo(self.server, self.port)
            except Exception as e:
                raise ValueError("DNS Parsing '{}'".format(self.server))
            if not addrall:
                j += 1
                utime.sleep_ms(500)
                continue
            addr = addrall[0][-1]
            break

        self.sock = socket.socket()
        self.sock.settimeout(20)
        self.sock.connect(addr)
        if self.ssl:
            import ussl
            self.sock = ussl.wrap_socket(self.sock, **self.ssl_params)

        if self.mqttversion == 3:
            protocol = b"MQIsdp"
        else:
            protocol = b"MQTT"

        connect_flags = 0
        connect_flags = clean_session << 1
        remaining_length = 2 + len(protocol) + 1 + 1 + 2

        if self.client_id:
            remaining_length = remaining_length + 2 + len(self.client_id)
        if self.user:
            remaining_length = remaining_length + 2 + len(self.user)
            connect_flags = connect_flags | 0x80
        if self.pswd:
            connect_flags = connect_flags | 0x40
            remaining_length = remaining_length + 2 + len(self.pswd)
        if self.lw_topic:
            connect_flags = connect_flags | 0x04 | ((self.lw_qos & 0x03) << 3) | ((self.lw_retain & 0x01) << 5)
            remaining_length += 2 + len(self.lw_topic) + 2 + len(self.lw_msg)

        command = 0x10
        packet = bytearray()
        packet.extend(struct.pack("!B", command))
        remaining_bytes = []
        while True:
            byte = remaining_length % 128
            remaining_length = remaining_length // 128
            if remaining_length > 0:
                byte = byte | 0x80

            remaining_bytes.append(byte)
            packet.extend(struct.pack("!B", byte))
            if remaining_length == 0:
                break

        if self.mqttversion == 3:
            packet.extend(struct.pack("!H" + str(len(protocol)) + "sBBH", len(protocol), protocol, 3, connect_flags,
                                      self.keepalive))
        else:
            packet.extend(struct.pack("!H" + str(len(protocol)) + "sBBH", len(protocol), protocol, 4, connect_flags,
                                      self.keepalive))
        if self.client_id:
            packet.extend(struct.pack("!H" + str(len(self.client_id)) + "s", len(self.client_id), self.client_id))
        if self.lw_topic:
            packet.extend(struct.pack("!H" + str(len(self.lw_topic)) + "s", len(self.lw_topic), self.lw_topic))
            packet.extend(struct.pack("!H" + str(len(self.lw_msg)) + "s", len(self.lw_msg), self.lw_msg))
        if self.user:
            packet.extend(struct.pack("!H" + str(len(self.user)) + "s", len(self.user), self.user))
        if self.pswd:
            packet.extend(struct.pack("!H" + str(len(self.pswd)) + "s", len(self.pswd), self.pswd))

        self.sock.write(packet)

        resp = self.sock.read(4)
        self.sock.setblocking(True)
        assert resp[0] == 0x20 and resp[1] == 0x02
        if resp[3] != 0:
            raise MQTTException(resp[3])
        self.last_time = utime.mktime(utime.localtime())
        self.connSta = True
        self.clean_session = clean_session
        self.ping_outstanding = False
        self.pingnum = self.pingmaxnum
        return resp[2] & 1

    def disconnect(self):
        # Pawn.zhou 2021/1/12 for JIRA STASR3601-2523 begin
        try:
            self.timerFlag = False
            self.pingFlag = False
            self.connSta = False
            self.ping_outstanding = False
            self.pingnum = self.pingmaxnum
            self.topic = []
            self.sock.write(b"\xe0\0")
            if self.mqttsendlock.locked():
                self.mqttsendlock.release()
            if self.mqttlock.locked():
                self.mqttlock.release()
            if self.mqttlock is not None:
                _thread.delete_lock(self.mqttlock)
                self.mqttlock = None
            if self.mqttsendlock is not None:
                _thread.delete_lock(self.mqttsendlock)
                self.mqttsendlock = None
        # Pawn.zhou 2021/1/12 for JIRA STASR3601-2523 end
        except:
            mqtt_log.warning("Error requesting to close connection.")
        utime.sleep_ms(500)
        self.sock.close()

    def close(self):
        self.sock.close()

    def ping(self):
        self.last_time = utime.mktime(utime.localtime())
        self.sock.write(b"\xc0\0")
        self.ping_outstanding = True

    def publish(self, topic, msg, retain=False, qos=0):
        pkt = bytearray(b"\x30\0\0\0")
        pid = next(self.__newpid)
        pkt[0] |= qos << 1 | retain
        sz = 2 + len(topic) + len(msg)
        if qos > 0:
            sz += 2
        assert sz < 2097152
        i = 1
        while sz > 0x7f:
            pkt[i] = (sz & 0x7f) | 0x80
            sz >>= 7
            i += 1
        pkt[i] = sz
        self.sock.write(pkt, i + 1)
        self._send_str(topic)
        if qos > 0:
            self.__rcv_pids.add(pid)
            struct.pack_into("!H", pkt, 0, pid)
            self.sock.write(pkt, 2)
        self.sock.write(msg)
        # self.last_time = utime.mktime(utime.localtime())
        if qos == 0:
            return
        if not self._await_pid(pid):
            mqtt_log.warning("publish QOS1 message was not received correctly")
        elif qos == 2:
            assert 0

    def __subscribe(self, topic, qos):
        pkt = bytearray(b"\x82\0\0\0")
        self.pid += 1
        # self.__rcv_pids.add(pid)
        struct.pack_into("!BH", pkt, 1, 2 + 2 + len(topic) + 1, self.pid)
        self.sock.write(pkt)
        self._send_str(topic)
        self.sock.write(qos.to_bytes(1, "little"))
        if self.ssl == False:
            self.sock.settimeout(5)
        while 1:
            op = self.wait_msg()
            if op == 0x90:
                resp = self.sock.read(4)
                # print(resp)
                assert resp[1] == pkt[2] and resp[2] == pkt[3]
                if resp[3] == 0x80:
                    raise MQTTException(resp[3])
                if self.ssl == False:
                    self.sock.setblocking(True)
                return
            elif op is not None:
                if op & 0xf0 == 0x30 or op == b"\xd0":
                    continue
            else:
                break

    def subscribe(self, topic, qos=0):
        assert self.cb is not None, "Subscribe callback is not set"
        if topic not in self.topic_list:
            self.topic_list.append(topic)
        self.qos = qos
        self.__subscribe(topic, qos=qos)

    # Wait for a single incoming MQTT message and process it.
    # Subscribed messages are delivered to a callback previously
    # set by .set_callback() method. Other (internal) MQTT
    # messages processed internally.
    def wait_msg(self):
        if self.ssl:
            res = self.sock.read(1)
        else:
            res = self.sock.recv(1)
        self.sock.setblocking(True)
        if res is None:
            return None
        if res == b"":
            # raise OSError(-1)
            # Pawn 2020/11/14 - WFZT mqttBUg -1
            return None

        if res == b"\xd0":  # PINGRESP
            self.ping_outstanding = False
            sz = self.sock.read(1)[0]
            assert sz == 0
            return res

        if res == b'\x40':  # PUBACK: save pid
            sz = self.sock.read(1)
            if sz != b"\x02":
                mqtt_log.warning("Publish message does not return ACK correctly")
                return
            rcv_pid = self.sock.read(2)
            pid = rcv_pid[0] << 8 | rcv_pid[1]
            if pid in self.__rcv_pids:
                self.__rcv_pids.discard(pid)
                return
            else:
                mqtt_log.warning("Publish message does not return ACK correctly")
                return

        op = res[0]
        if op & 0xf0 != 0x30:
            return op
        sz = self._recv_len()
        topic_len = self.sock.read(2)
        topic_len = (topic_len[0] << 8) | topic_len[1]
        topic = self.sock.read(topic_len)
        sz -= topic_len + 2
        if op & 6:
            pid = self.sock.read(2)
            pid = pid[0] << 8 | pid[1]
            sz -= 2
        msg = self.sock.read(sz)
        self.cb(topic, msg)
        # self.last_time = utime.mktime(utime.localtime())
        if op & 6 == 2:
            pkt = bytearray(b"\x40\x02\0\0")
            struct.pack_into("!H", pkt, 2, pid)
            self.sock.write(pkt)
            return op
        if op & 6 == 4:
            assert 0

    # Checks whether a pending message from server is available.
    # If not, returns immediately with None. Otherwise, does
    # the same processing as wait_msg.
    def check_msg(self):
        self.sock.setblocking(False)
        return self.wait_msg()

    def _timeout(self, t):
        return utime.ticks_diff(utime.ticks_ms(), t) > self.__response_time

    def _await_pid(self, pid):
        t = utime.ticks_ms()
        while pid in self.__rcv_pids:  # local copy
            if self._timeout(t):
                break  # Must repub or bail out
            utime.sleep_ms(1000)
        else:
            return True  # PID received. All done.
        return False

    def get_mqttsta(self):
        '''
        Get the MQTT connection status
        CONNECTEXCE     -1:Connect the interrupt
        CONNECTSUCCESS  0:connection is successful
        ARECONNECT      1:In the connection
        SEVCLOSE        2:server closes the connection
        '''
        socket_sta = self.sock.getsocketsta()
        if socket_sta == 0:
            return self.ARECONNECT
        elif (socket_sta == 2) or (socket_sta == 3):
            return self.ARECONNECT
        elif (socket_sta == 4) and self.connSta:
            return self.CONNECTSUCCESS
        elif (socket_sta == 7) or (socket_sta == 8):
            return self.SEVCLOSE
        else:
            return self.CONNECTEXCE


class MQTTClient(BaseMqtt):
    DELAY = 2
    DEBUG = False

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.error_callback = None

    def delay(self, i):
        utime.sleep(self.DELAY)

    def error_register_cb(self, func):
        self.error_callback = func

    def base_reconnect(self):
        i = 0
        if self.mqttlock.locked():
            return
        self.mqttlock.acquire()
        if self.threadId is not None and self.ping_outstanding != True:
            if self.mqttsendlock.locked():
                self.mqttsendlock.release()
            _thread.stop_thread(self.threadId)
            self.threadId = None
        if self.reconn:
            if self.error_callback is not None:
                e = "reconnect_start"
                self.error_callback(e)#start reconnect
        while 1:
            try:
                self.sock.close()
                net_sta = net.getState()
                if net_sta != -1 and ((net_sta[1][0] == 1) or (net_sta[1][0] == 5)):
                    call_state = dataCall.getInfo(1, 0)
                    if (call_state != -1) and (call_state[2][0] == 1):
                        time_info = self.logTime()
                        mqtt_log.info(
                            "[%s] The network condition has been restored and an attempt will be made to reconnect" % time_info)
                        self.pingFlag = False
                        self.connect(self.clean_session)
                        mqtt_log.info("[%s] Reconnection successful!" % time_info)
                        if self.topic_list:
                            for topic_re in self.topic_list:
                                super().subscribe(topic_re, self.qos)
                        time_info = self.logTime()
                        if self.mqttlock.locked():
                            self.mqttlock.release()
                        if self.reconn:
                            if self.error_callback is not None:
                                e = "reconnect_success"
                                self.error_callback(e)#reconnect success
                        return
                    else:
                        time_info = self.logTime()
                        mqtt_log.info("[%s] Wait for reconnection." % time_info)
                        utime.sleep(5)
                        continue
                else:
                    time_info = self.logTime()
                    mqtt_log.info("[%s] Wait for reconnection." % time_info)
                    utime.sleep(5)
                    continue
            except Exception as e:
                i += 1
                time_info = self.logTime()
                mqtt_log.warning(
                    "[%s] The connection attempt failed and will be tried again after %d seconds." % (time_info, 5 + i))
                utime.sleep(5 + i)

    def publish(self, topic, msg, retain=False, qos=0):
        while True:
            try:
                if self.mqttsendlock.locked():
                    utime.sleep_ms(10)
                    continue
                self.mqttsendlock.acquire()
                ret = super().publish(topic, msg, retain, qos)
                self.mqttsendlock.release()
                return ret
            except Exception as e:
                if self.mqttsendlock.locked():
                    self.mqttsendlock.release()
                break

    def wait_msg(self):
        while True:
            try:
                # The state changes when disconnect is called
                if not self.timerFlag:
                    break
                return super().wait_msg()
            except Exception as e:
                if not self.timerFlag:
                    break
                if not self.reconn:
                    raise e
                # Whether to use the built-in reconnect mechanism
                time_info = self.logTime()
                if self.ping_outstanding == True:
                    mqtt_log.warning("[%s] wait msg, send ping timeout. Trying to reconnect" % time_info)
                else:
                    mqtt_log.warning("[%s] wait msg, Network exception. Trying to reconnect" % time_info)
                utime.sleep(1)
                self.base_reconnect()

    def connect(self, clean_session=True):
        try:
            super().connect(clean_session)
        except Exception as e:
            if str(e) == "104":
                raise ValueError(
                    "MQTT Connect Error='{}' Server Actively RST Disconnected.Need Check addr&port".format(e))
            elif str(e) == "107" or str(e) == "4":
                raise ValueError(
                    "MQTT Connect Error='{}' Server Actively FIN Disconnected.Need Check Connection parameters".format(
                        e))
            elif str(e) == "110":
                raise ValueError("MQTT Connect Error='{}' Timeout.Need Check Network".format(e))
            raise ValueError("MQTT Connect error='{}' FAIL".format(e))
        if self.keepalive > 0 and not self.pingFlag:
            # Pawn.zhou 2021/1/12 for JIRA STASR3601-2523 begin
            task_stacksize = _thread.stack_size()
            _thread.stack_size(16 * 1024)
            self.threadId = _thread.start_new_thread(self.__loop_forever, ())
            _thread.stack_size(task_stacksize)
            self.pingFlag = True
            return 0
            # Pawn.zhou 2021/1/12 for JIRA STASR3601-2523 end
        else:
            return 0

    def __loop_forever(self):
        while True:
            if self.keepalive >= 5:
                keepalive = self.keepalive - 3
            try:
                if not self.timerFlag:
                    if self.mqttsendlock is not None and self.mqttsendlock.locked():
                        self.mqttsendlock.release()
                    self.threadId = None
                    break
                time = utime.mktime(utime.localtime())
                if time - self.last_time > keepalive:
                    if self.mqttlock.locked():
                        utime.sleep(5)
                        continue
                    if self.ping_outstanding == True:
                        mqtt_log.warning("[%s] Send ping timeout. Trying to reconnect" % time)
                        if not self.reconn:
                            if self.mqttsendlock is not None and self.mqttsendlock.locked():
                                self.mqttsendlock.release()
                            self.threadId = None
                            raise Exception("Send ping timeout.")
                        else:
                            if self.pingnum <= 0:
                                mqtt_log.warning("[%s] Send ping timeout. Trying to reconnect" % time)
                                self.sock.settimeout(3)
                                self.pingFlag = False
                                if self.mqttsendlock is not None and self.mqttsendlock.locked():
                                    self.mqttsendlock.release()
                                self.threadId = None
                                break
                            else:
                                mqtt_log.warning("[%s] Trying to resend ping(%d)" % (time, self.pingnum))
                                self.pingnum = self.pingnum - 1
                    while True:
                        if self.mqttsendlock is not None and self.mqttsendlock.locked():
                            utime.sleep_ms(10)
                            continue
                        self.mqttsendlock.acquire()
                        super().ping()
                        self.mqttsendlock.release()
                        break
                else:
                    utime.sleep(5)
                    continue
            except Exception as e:
                if self.mqttsendlock is not None and self.mqttsendlock.locked():
                    self.mqttsendlock.release()
                if not self.timerFlag:
                    self.threadId = None
                    break
                # Network normal, take the initiative to throw exception
                if not self.reconn:
                    if self.error_callback is not None:
                        self.error_callback(str(e))
                        self.pingFlag = False
                        self.threadId = None
                        break
                    else:
                        self.pingFlag = False
                        self.threadId = None
                        break
                # Whether to use the built-in reconnect mechanism
                time_info = self.logTime()
                if self.ping_outstanding == True:
                    mqtt_log.warning("[%s] Send ping timeout. Trying to reconnect" % time_info)
                else:
                    mqtt_log.warning("[%s] Send ping, Network exception. Trying to reconnect" % time_info)
                utime.sleep(2)
                self.base_reconnect()
                break

    def logTime(self):
        log_time = utime.localtime()
        time_info = "%d-%d-%d %d:%d:%d" % (
            log_time[0], log_time[1], log_time[2], log_time[3], log_time[4], log_time[5],)
        return time_info
