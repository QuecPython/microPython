/*
 * Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  
 *     http://www.apache.org/licenses/LICENSE-2.0
 *  
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "builtin.h"

const char quecpython_help_text[] =
    "Welcome to QuecPython! The Quecpython is a \n"
    "small microcontroller that runs Micro Python.\n"
    "\n"
    "For online help please visit http://micropython.org/help/.\n"
    "\n"
    "For access to the hardware use the 'machine' module:\n"
    "\n"
    "import machine\n"
    "m = machine.lcd()\n"
    "m.lcd_init()\n"
    "m.lcd_brightness(5)\n"
    "m.lcd_show()\n"
    "\n"
    "Basic socket operation:\n"
    "\n"
    "import usocket\n"
    "sock = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)\n"
    "sockaddr = usocket.getaddrinfo('www.tongxinmao.com',80)[0][-1]\n"
    "sock.connect(sockaddr)\n"
    "ret = sock.send('GET /News HTTP/1.1\\r\\nHost: www.tongxinmao.com\\r\\nAccept-Encoding: deflate\\r\\nConnection: keep-alive\\r\\n\\r\\n')\n"
	"print('send %d bytes' % ret)\n"
	"data = sock.recv(256)\n"
	"print('recv %s bytes:' % len(data))\n"
	"print(data.decode())\n"
	"sock.close()\n"
    "\n"
    "Control commands:\n"
    "  CTRL-A        -- on a blank line, enter raw REPL mode\n"
    "  CTRL-B        -- on a blank line, enter normal REPL mode\n"
    "  CTRL-C        -- interrupt a running program\n"
    "  CTRL-D        -- on a blank line, do a soft reset of the board\n"
    "  CTRL-E        -- on a blank line, enter paste mode\n"
    "\n"
    "For further help on a specific object, type help(obj)\n"
    "For a list of available modules, type help('modules')\n"
;
