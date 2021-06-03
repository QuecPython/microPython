/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
