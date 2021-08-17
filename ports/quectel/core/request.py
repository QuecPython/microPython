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

import usocket
import ujson
import ussl


class Response:
    def __init__(self, f, decode=True, sizeof=4096):
        self.raw = f
        self.encoding = "utf-8"
        self.decode = decode
        self.sizeof = sizeof

    def close(self):
        if self.raw:
            if s_isopen:
                self.raw.close()
                self.raw = None
        if s_isopen:
            if self.raw:
                # jian.yao 2021-02-22 将套接字标记为关闭并释放所有资源
                s.close()
        if not len(uheaders) == 0:
            # jian.yao 2021-02-22 清空请求头的信息
            uheaders.clear()

    @property
    def content(self):
        global s_isopen
        # jian.yao 2021-01-04 新增参数decode,用户可选择开启关闭
        try:
            while True:
                # jian.yao 2020-12-28 分块yield输出
                block = self.raw.read(self.sizeof)
                # jian.yao 2021-01-11 增加read读取块的大小配置
                if block:
                    yield block.decode() if self.decode else block
                else:
                    s.close()  # jian.yao 2021-02-22 将套接字标记为关闭并释放所有资源
                    s_isopen = False
                    break
        except Exception as e:
            s.close() # 2021-05-27
            s_isopen = False
            pass
        return ""

    @property
    def text(self):
        # jian.yao 2021-01-04 text yield输出
        for i in self.content:
            yield str(i)
        return ""

    def json(self):
        # jian.yao 2021-01-04 TODO 大文件输出会出错
        try:
            json_str = ""
            for i in self.content:
                json_str += i
            # jian.yao 2021-02-07 如果调用此方法前使用了content/text则返回空
            if json_str:
                return ujson.loads(json_str)
            else:
                return None
        except Exception as e:
            raise ValueError(
                "The data for the response cannot be converted to JSON-type data,please try use response.content method")


def request(method, url, data=None, json=None, stream=None, decode=True, sizeof=255, timeout=None, headers={},
            ssl_params={}):
    global port
    global s_isopen
    s_isopen = True
    port_exist = False
    URL = url
    if not url.split(".")[0].isdigit():
        if not url.startswith("http"):
            url = "http://" + url
        try:
            proto, dummy, host, path = url.split("/", 3)
        except ValueError:
            proto, dummy, host = url.split("/", 2)
            path = ""
        # jian.yao 2020-12-08 新增对ip:port格式的判断
        if ":" in host:
            url_info = host.split(":")
            host = url_info[0]
            port = int(url_info[1])
            port_exist = True
        # jian.yao 2020-12-09
        if proto == "http:":
            if not port_exist:
                port = 80
        # jian.yao 2020-12-09
        elif proto == "https:":
            if not port_exist:
                port = 443
        else:
            raise ValueError("Unsupported protocol: " + proto)
        try:
            ai = usocket.getaddrinfo(host, port, 0, usocket.SOCK_STREAM)
            ai = ai[0]
        except IndexError:
            raise IndexError("Domain name resolution error, please check network connection")

    # jian.yao 2020-12-08 新增对错误ip的判断并提醒用户重新输入正确的ip:port
    elif url.split(".")[0].isdigit() and ":" not in url:
        raise ValueError(
            "MissingSchema: Invalid URL '{}': No schema supplied. Perhaps you meant http://{}? ".format(url, url))

    else:
        path = ""
        proto = ""
        if ":" not in url:
            raise ValueError("URL address error: !" + url)
        try:
            if "/" in url:
                ip_info = url.split('/', 2)
                path = ip_info[1]
                host, port = ip_info[0].split(":")
            else:
                host, port = url.split(":")
        except:
            raise ValueError("URL address error: " + url)
        try:
            ai = usocket.getaddrinfo(host, port, 0, usocket.SOCK_STREAM)
            ai = ai[0]
        except IndexError:
            raise IndexError("Domain name resolution error, please check network connection")
    global s
    s = usocket.socket(ai[0], ai[1], ai[2])
    # jian.yao 2021-01-11 增加socket超时时间
    s.settimeout(timeout)  # 设置socket阻塞模式
    try:
        # jian.yao 2020-12-09 check connect error
        try:
            s.connect(ai[-1])
        except Exception as e:
            raise RuntimeError(
                "HTTPConnectionPool(host='{}', port=8080): Max retries exceeded with url: / (Caused by NewConnectionError Failed to establish a new connection: [WinError 10060] 由于连接方在一段时间后没有正确答复或连接的主机没有反应，连接尝试失败。'))".format(
                    URL))

        if proto == "https:":
            try:
                if len(ssl_params):
                    s = ussl.wrap_socket(s, **ssl_params)
                else:
                    s = ussl.wrap_socket(s, server_hostname=host)
            except Exception as e:
                pass

        s.write(b"%s /%s HTTP/1.0\r\n" % (method, path))
        if not "Host" in headers:
            s.write(b"Host: %s\r\n" % host)
        for k in headers:
            s.write(k)
            s.write(b": ")
            s.write(headers[k])
            s.write(b"\r\n")
        if json is not None:
            assert data is None
            data = ujson.dumps(json)
            s.write(b"Content-Type: application/json\r\n")
        if data:
            s.write(b"Content-Length: %d\r\n" % len(data))
        s.write(b"\r\n")
        if data:
            s.write(data)
        l = s.readline()
        global uheaders
        uheaders = {}
        try:
            # jian.yao 2020-12-09 Abnormal response handle
            l = l.split(None, 2)
            status = int(l[1])
        except:
            raise ValueError("InvalidSchema: No connection adapters were found for '{}'".format(URL))
        reason = ""
        if len(l) > 2:
            reason = l[2].rstrip()
        while True:
            l = s.readline()
            j = l.decode().split(":")
            try:
                uheaders[j[0]] = j[1].replace("\n", "").replace("\r", "")
            except Exception as e:
                pass
            if not l or l == b"\r\n":
                break
            # 2021-01-23 jian.yao
            # if l.startswith(b"Transfer-Encoding:"):
            #     if b"chunked" in l:
            #         raise ValueError("Unsupported " + l.decode())
            if l.startswith(b"Location:") and not 200 <= status <= 299:
                raise NotImplementedError("Redirects not yet supported")
    except OSError:
        s.close()
        raise

    resp = Response(s, decode=decode, sizeof=sizeof)
    resp.status_code = status
    resp.reason = reason
    resp.headers = uheaders
    return resp


def head(url, **kw):
    return request("HEAD", url, **kw)


def get(url, **kw):
    return request("GET", url, **kw)


def post(url, **kw):
    return request("POST", url, **kw)


def put(url, **kw):
    return request("PUT", url, **kw)


def patch(url, **kw):
    return request("PATCH", url, **kw)


def delete(url, **kw):
    return request("DELETE", url, **kw)