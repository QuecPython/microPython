# coding: utf-8
import request
import audio
import utime
import _thread
import checkNet
from machine import UART

rtmp_support = 10

try:
    from librtmp import RTMP
    rtmp_support = 10
    print('RTMP Decoder import success.')
except:
    rtmp_support = 0
    print('HLS without RTMP Decoder.')

import net
import dataCall

PROJECT_NAME = "QuecPython_RTMP_example"
PROJECT_VERSION = "1.0.0"
checknet = checkNet.CheckNetwork(PROJECT_NAME, PROJECT_VERSION)

aud = audio.Audio(0)
aud.setVolume(2)
lock = _thread.allocate_lock()


def is_print(*args, **kwagrs):
    print(*args, **kwagrs)


class hls_audio():
    # 音频格式，简单的后缀判断
    def parse_format(self):
        af = {
            'pcm': {'sizeof': 10*1024, 'format': 1},   # 单通道，8000
            'wav': {'sizeof': 20*1024, 'format': 2}, 
            'mp3': {'sizeof': 15*1024, 'format': 3},
            'amr': {'sizeof': 5*960, 'format': 4},
            'aac': {'sizeof': 10*1024, 'format': 6},
            'm4a': {'sizeof': 10*1024, 'format': 7}, 
            'ts': {'sizeof': 188*1, 'format': 8}, 
            'flv': {'sizeof': 100*1024, 'format': 9},
            'rtmp': {'sizeof': 150*1024, 'format': rtmp_support},
            
            'm3u8': {'sizeof': 188*1, 'format': 20},
        }
        # sizeof: 单次切片的大小；format：固件包接口不同格式的定义
        f = self.url.split('?')[0].split('.')[-1]
        if f not in af.keys():
            f = self.url.split(':')[0]
            if f not in af.keys(): 
                raise Exception('unsupported audio format.{}'.format(f))
        return af[f]

    # 播放函数
    def play_run(self, timeout = 30):
        #num = 0;
        stagecode, subcode = checknet.wait_network_connected(30)
        print('stagecode = {}, subcode = {}'.format(stagecode, subcode))
        if stagecode != 3 and subcode != 1:
            print('Network is not ready please retry later')
        p = self.params
        # 请求获取资源

        if p['format'] != 10:
            try:
                lock.acquire()  # 获取锁
                r = request.get(self.url, headers=self.headers, timeout = timeout)
                lock.release()  # 释放锁
                is_print('request code ==', r.status_code)
            except:
                print('request data failed.')
                return
        elif p['format'] == 10:
            rtmp = RTMP(self.url)
            ret = rtmp.connect()
            if ret == 0:
                print('rtmp connect successful.')
            else:
                print('rtmp connect failed.')
                return -1
            ret = rtmp.connectStream()
            if ret == 0:
                print('rtmp connect stream successful.')
            else:
                print('rtmp connect stream failed.')
                return -1

        
        # 播放文件头
        if p['format'] == 4 and not self.headers: # amr
            head_content = r.raw.read(p['sizeof'])
            p['format'] = self.parse_amr_head(head_content, p['format'])
        # 循环播放文件内容
        
        #如果是m3a8则进行处理 整合成一个TS 再调用TS播放
        if p['format'] == 20: # m3a8
            https_state = 0; #0:http 1:https
            tsurl = ""
            last_sequence = 0
            sequence = 0
            distance = 0
            num_of_ts = 0
            skip_num = 0
            self.m3u8_url_list = []
            len_cauced = 0
            ts = bytearray()
            if self.url.find("https:") != -1:
                https_state = 1;
            else:
                https_state = 0;
            done = 0
            self.is_runing = True
            while True:
                if self.is_close:
                    is_print('is_close == ', self.is_close)
                    temp.close()
                    self.is_runing = False
                    r.close() #
                    self.close()
                    self.m3u8_url_list = []
                    sequence = 0
                    distance = 0
                    num_of_ts = 0
                    skip_num = 0
                    is_print('CLOSE Successful')
                    return
            
                try:
                    lock.acquire()  # 获取锁
                    rc = request.get(self.url, timeout = 200)
                    lock.release()  # 释放锁
                    is_print('get m3u8 file request code ==', rc.status_code)
                except:
                    rc.close()
                    is_print('get m3a8 file failed later retry')
                    continue
                while True:  # 获取m3u8文件
                    try:
                        ret = rc.raw.readline()
                        tsurl = ret.decode()
                        if tsurl.find("EXT-X-TARGETDURATION") != -1:
                            num_of_ts = tsurl.split(':')[-1]
                            num_of_ts = int(num_of_ts)
                        
                        if tsurl.find("EXT-X-MEDIA-SEQUENCE") != -1:
                            last_sequence = sequence
                            sequence = tsurl.split(':')[-1]
                            sequence = int(sequence)
                            last_sequence = int(last_sequence)
                        
                        if((sequence - last_sequence) <= num_of_ts) and len_cauced == 0 :
                            skip_num = num_of_ts - (sequence - last_sequence)
                            is_print("now:",sequence)
                            is_print("last_sequence:",last_sequence)
                            is_print("skip:",skip_num)
                            if len_cauced == 1 and num_of_ts == skip_num:
                                rc.close()
                                len_cauced = 0
                                is_print('finish loading m3u8 file')
                                break;
                            len_cauced = 1

                        if tsurl.find("//") != -1:
                            if tsurl.find("http") == -1:
                                if https_state == 0:
                                    tsurl = "http:" + tsurl
                                    #is_print("http url found")
                                elif https_state == 1:
                                    tsurl = "https:" + tsurl
                                    is_print("https url found")
                                else:
                                    is_print('URL unknow error')
                                    return
                            else:
                                tsurl = tsurl
                            tsurl = tsurl.replace("\r", "")
                            tsurl = tsurl.replace("\n", "")
                            tsurl = tsurl.replace(" ", "")
                            
                            #num_of_ts - (sequence - last_sequence)
                            if(skip_num != 0):
                                skip_num = skip_num - 1
                                #is_print("already have this url skip:",skip_num)
                                if (skip_num == 0):
                                    if(tsurl not in self.m3u8_url_list):
                                        #is_print(tsurl)
                                        self.m3u8_url_list.append(tsurl)
                                        is_print("add:",tsurl)
                            else:
                                if(tsurl not in self.m3u8_url_list):
                                    #is_print(tsurl)
                                    self.m3u8_url_list.append(tsurl)
                                    is_print("add:",tsurl)
                    except:
                        rc.close()
                        len_cauced = 0
                        is_print('finish loading m3u8 file')
                        break;
            
                try:
                    is_print('list remain:',len(self.m3u8_url_list))
                    if len(self.m3u8_url_list) == 0 :
                        
                        is_print('run out of list now get new list')
                        utime.sleep_ms(3000)
                        continue
                    is_print('begin to request TS file url:',self.m3u8_url_list[0])
                    try:
                        lock.acquire()  # 获取锁
                        temp = request.get(self.m3u8_url_list[0], timeout = 3000)
                        lock.release()  # 释放锁
                    except:
                        temp.close()
                        is_print('get file failed skip frame now')
                        #del(self.m3u8_url_list[0])
                        utime.sleep_ms(2000)
                        continue
                    is_print('request code ==', temp.status_code)
                    if temp.status_code == 200:
                        while True:
                            # if self.is_close:
                                # is_print('is_close == ', self.is_close)
                                # r.close()
                                # temp.close()
                                # self.is_runing = False
                                # return
                            # if self.is_close:
                                # is_print('is_close == ', self.is_close)
                                # r.close()
                                # self.is_runing = False
                                # return
                            try: # 读取可能会有报错，直接break
                                i = temp.raw.read(188)
                                if not i:
                                    break
                            except Exception as e:
                                is_print('read error:{}'.format(e))
                                break
                            ret = aud.playStream(8, i)
                            #temp.close()
                        del(self.m3u8_url_list[0])
                        temp.close()
                    else:
                        is_print('request error code ==',temp.status_code)
                        break
                            
                except :
                    is_print('is_close == ', self.is_close)
                    temp.close()
                    self.is_runing = False
                    r.close() #
                    utime.sleep_ms(1000)
                    self.close()
                    self.m3u8_url_list = []
                    sequence = 0
                    distance = 0
                    num_of_ts = 0
                    skip_num = 0
                    is_print('CLOSE SUCCESSFUL')
                    return
            
            is_print('play done----------')
            r.close()
            temp.close()
            return

        self.is_runing = True
        while True:
            if self.is_close:
                is_print('is_close == ', self.is_close)
                if p['format'] == 10:
                    rtmp.close()
                else:
                    r.close()
                self.is_runing = False
                return
            # if self.is_pause:
            #     utime.sleep_ms(50)
            #     continue
            try: # 读取可能会有报错，直接break
                if p['format'] == 10:
                    i = rtmp.read(p['sizeof'])
                else:
                    i = r.raw.read(p['sizeof'])
                if not i:
                    break
            except Exception as e:
                is_print('read error:{}'.format(e))
                break
            ret = aud.playStream(p['format'], i)
            #num = num +1
            #is_print('buff num:{}'.format(num))
            #is_print('play code == ', ret)
        self.is_runing = False
        if p['format'] == 10:
            rtmp.close()
        else:
            r.close()
        utime.sleep_ms(1000)
        self.close()
        is_print('CLOSE SUCCESSFUL')     
        is_print('play done----------')
        

    # amr
    def parse_amr_head(self, head_content, format):
        # if head_content[:5] != b'#!AMR':
        #     # amr
        #     pass
        # elif head_content[:4] != b'RIFF' and head_content[8:15] == b'WAVEfmt':
        #     # wav
        #     pass
        # elif head_content[:3] == b'ID3':
        #     # mp3 的文件格式可能不统一，不一定是在文件头进行标记；
        #     # ID3V2 在文件头以 ID3 开头；
        #     # ID3V1 在文件尾以 TAG 开头。
        #     pass

        single_amr_nb = '2321414d520a'
        single_amr_wb = '2321414d522d57420a'
        mult_amr_nb = '2321414d525f4d43312e300a'
        mult_amr_wb = '2321414d522d57425f4d43312e300a'

        head_hexs = []
        for i in head_content[:16]:
            char = hex(i).replace('0x', '')
            if len(char) <= 1:
                char = '0{}'.format(char)
            head_hexs.append(char.lower())
        # 单声道还是多声道
        if ''.join(head_hexs[:6]) == single_amr_nb:
            format = 4 # 单声道 AMR-NB
        elif ''.join(head_hexs[:9]) == single_amr_wb:
            format = 5 # 单声道 AMR-WB
        elif ''.join(head_hexs[:12]) == mult_amr_nb:
            format = 4 # 多声道 AMR-NB
        elif ''.join(head_hexs[:15]) == mult_amr_wb:
            format = 5 # 多声道 AMR-WB
        # 播放头
        aud.playStream(format, head_content)
        return format
    

    # 播放前将所有参数初始化
    def play_init(self, url, range=None):
        # self.is_pause = False 
        self.is_close = False
        self.url = url
        self.params = self.parse_format()
        self.headers = {}
        if range: # 断点续传
            self.headers['Range'] = 'bytes={}-'.format(range)
        is_print(self.params)
        is_print(self.headers)

    # play
    def play(self, url):
        while True:
            net_sta = net.getState()
            if net_sta != -1 and net_sta[1][0] == 1:
                is_print("Net State OK")
                break
            else:
                utime.sleep(3)
                is_print(".")
                continue
        while True:
            call_state = dataCall.getInfo(1,0)
            if call_state != -1 and call_state[2][0] == 1:
                is_print("Data State OK")
                break
            else:
                utime.sleep(3)
                is_print(".")
                continue
    
        is_print(url)
        self.play_init(url)
        _thread.start_new_thread(self.play_run, ())

    # pause
    def pause(self, f):
        if f == 1:
            aud.StreamPause()
            is_print('stream play pause')
        else:
            aud.StreamContinue()
            is_print('stream play continue')

    # close
    def close(self):
        self.url = None
        self.is_close = True
        while True:
            if not self.is_runing:
                break
        aud.stopPlayStream()
        is_print('stream play close')


    # 快进，快退
    def seek_secs(self, secs):
        is_print('stream play will seek to {}s'.format(secs))
        if not self.url:
            raise Exception("You can't fast forward or rewind without playing audio")
        self.is_close = True
        while True:
            if not self.is_runing:
                break
        print('seek start')
        seek = aud.StreamSeek(secs)
        is_print('stream seek byte is {}s'.format(seek))
        utime.sleep(1)
        url = self.url
        
        self.play_init(url, range=seek)
        _thread.start_new_thread(self.play_run, ())

    def set_volume(self,volume):
        aud.setVolume(volume)



if __name__ == '__main__':
    # from usr.hls_point import hls_audio 
    hls = hls_audio()
    hls.play('http://111.229.132.74/httptest/user/James/down.pcm')
 

