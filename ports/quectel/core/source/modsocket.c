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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "runtime0.h"
#include "nlr.h"
#include "objlist.h"
#include "objstr.h"
#include "runtime.h"
#include "mperrno.h"
#include "mphal.h"
#include "stream.h"
#include "netutils.h"
//#include "tcpip_adapter.h"
//#include "mdns.h"
//#include "modnetwork.h"

#if !defined(PLAT_Qualcomm)
#include "sockets.h"
#include "netdb.h"
#include "ip4.h"
#include "igmp.h"
#include "netif.h"
#endif
#include "helios_debug.h"
#if !defined(PLAT_RDA)
#include "helios_datacall.h"
#endif
//#if !MICROPY_ESP_IDF_4
//#define lwip_bind lwip_bind_r
//#define lwip_listen lwip_listen_r
//#define lwip_accept lwip_accept_r
//#define lwip_setsockopt lwip_setsockopt_r
//#define lwip_fnctl lwip_fnctl_r
//#define lwip_recvfrom lwip_recvfrom_r
//#define lwip_write lwip_write_r
//#define lwip_sendto lwip_sendto_r
//#define lwip_close lwip_close_r
//#endif

#if defined(PLAT_Qualcomm)
#define TCP_KEEPALIVE 0x02
#endif

#define QPY_SOCKET_LOG(msg, ...)      custom_log(modsocket, msg, ##__VA_ARGS__)

#define SOCKET_POLL_US (100000)
//#define MDNS_QUERY_TIMEOUT_MS (5000)
//#define MDNS_LOCAL_SUFFIX ".local"
#ifndef IP_ADD_MEMBERSHIP
#define IP_ADD_MEMBERSHIP 0x400
#endif
typedef struct _socket_obj_t {
    mp_obj_base_t base;
    int fd;
    uint8_t domain;
    uint8_t type;
    uint8_t proto;
    bool peer_closed;
    unsigned int retries;
    unsigned int timeout;
    #if MICROPY_PY_USOCKET_EVENTS
    mp_obj_t events_callback;
    struct _socket_obj_t *events_next;
    #endif
} socket_obj_t;

int socket_obj_t_get_fd(mp_obj_t sock)
{
	socket_obj_t *so = (socket_obj_t *)sock;
	return so->fd;
}


void _socket_settimeout(socket_obj_t *sock, uint64_t timeout_ms);

#if MICROPY_PY_USOCKET_EVENTS
// Support for callbacks on asynchronous socket events (when socket becomes readable)

// This divisor is used to reduce the load on the system, so it doesn't poll sockets too often
#define USOCKET_EVENTS_DIVISOR (8)

STATIC uint8_t usocket_events_divisor;
STATIC socket_obj_t *usocket_events_head;
//STATIC uint32_t usocket_timeout = 0;

void usocket_events_deinit(void) {
    usocket_events_head = NULL;
}

// Assumes the socket is not already in the linked list, and adds it
STATIC void usocket_events_add(socket_obj_t *sock) {
    sock->events_next = usocket_events_head;
    usocket_events_head = sock;
}

// Assumes the socket is already in the linked list, and removes it
STATIC void usocket_events_remove(socket_obj_t *sock) {
    for (socket_obj_t **s = &usocket_events_head;; s = &(*s)->events_next) {
        if (*s == sock) {
            *s = (*s)->events_next;
            return;
        }
    }
}

// Polls all registered sockets for readability and calls their callback if they are readable
void usocket_events_handler(void) {
    if (usocket_events_head == NULL) {
        return;
    }
    if (--usocket_events_divisor) {
        return;
    }
    usocket_events_divisor = USOCKET_EVENTS_DIVISOR;

    fd_set rfds;
    FD_ZERO(&rfds);
    int max_fd = 0;

    for (socket_obj_t *s = usocket_events_head; s != NULL; s = s->events_next) {
        FD_SET(s->fd, &rfds);
        max_fd = MAX(max_fd, s->fd);
    }

    // Poll the sockets
    struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };
    int r = lwip_select(max_fd + 1, &rfds, NULL, NULL, &timeout);
    if (r <= 0) {
        return;
    }

    // Call the callbacks
    for (socket_obj_t *s = usocket_events_head; s != NULL; s = s->events_next) {
        if (FD_ISSET(s->fd, &rfds)) {
            mp_call_function_1_protected(s->events_callback, s);
        }
    }
}

#endif // MICROPY_PY_USOCKET_EVENTS

NORETURN static void exception_from_errno(int _errno) {
    // Here we need to convert from lwip errno values to MicroPython's standard ones
    if (_errno == EADDRINUSE) {
        _errno = MP_EADDRINUSE;
    } else if (_errno == EINPROGRESS) {
        _errno = MP_EINPROGRESS;
    }
    mp_raise_OSError(_errno);
}

static inline void check_for_exceptions(void) {
    mp_handle_pending(true);
}

// This function mimics lwip_getaddrinfo, with added support for mDNS queries
static int _socket_getaddrinfo3(const char *nodename, const char *servname,
    const struct addrinfo *hints, struct addrinfo **res) {
#if defined(PLAT_ASR)
    if (netif_get_default_nw()) {
        return getaddrinfo(nodename, servname, hints, res);
    } else {
#endif
        // Normal query
        //uart_printf("getaddrinfo_with_pcid, nodename %s, servname %s\r\n", nodename, servname);
        /*
        ** Jayceon 2020/12/03 --part1 begin--
        ** Solve the problem of not being able to connect to the Internet after dialing 
        ** with a PDP other than the first PDP . 
        */
    #if defined(PLAT_RDA) || defined(PLAT_Qualcomm)
        int pdp = 1;
    #else
        int pdp = Helios_DataCall_GetCurrentPDP();
    #endif

        return getaddrinfo_with_pcid(nodename, servname, hints, res, pdp);
        /* Jayceon 2020/12/03 --part1 end-- */
#if defined(PLAT_ASR)
    }
#endif
}

static int _socket_getaddrinfo2(const mp_obj_t host, const mp_obj_t portx, struct addrinfo **resp) {
    const struct addrinfo hints = {
#if defined(PLAT_Qualcomm)
        .ai_flags = AI_CANONNAME,
#endif
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
    };

    mp_obj_t port = portx;
    if (mp_obj_is_small_int(port)) {
        // This is perverse, because lwip_getaddrinfo promptly converts it back to an int, but
        // that's the API we have to work with ...
        port = mp_obj_str_binary_op(MP_BINARY_OP_MODULO, mp_obj_new_str_via_qstr("%s", 2), port);
    }

    const char *host_str = mp_obj_str_get_str(host);
    const char *port_str = mp_obj_str_get_str(port);

    if (host_str[0] == '\0') {
        // a host of "" is equivalent to the default/all-local IP address
        host_str = "0.0.0.0";
    }

    MP_THREAD_GIL_EXIT();
    int res = _socket_getaddrinfo3(host_str, port_str, &hints, resp);
    MP_THREAD_GIL_ENTER();

    return res;
}

STATIC void _socket_getaddrinfo(const mp_obj_t addrtuple, struct addrinfo **resp) {
    mp_obj_t *elem;
    mp_obj_get_array_fixed_n(addrtuple, 2, &elem);
    int res = _socket_getaddrinfo2(elem[0], elem[1], resp);
    if (res != 0) {
        mp_raise_OSError(res);
    }
    if (*resp == NULL) {
        mp_raise_OSError(-2); // name or service not known
    }
}

STATIC mp_obj_t socket_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 3, false);
    socket_obj_t *sock = m_new_obj_with_finaliser(socket_obj_t);
    sock->base.type = type_in;
    sock->domain = AF_INET;
    sock->type = SOCK_STREAM;
    sock->proto = 0;
    sock->peer_closed = false;
    if (n_args > 0) {
        sock->domain = mp_obj_get_int(args[0]);
        if (n_args > 1) {
            sock->type = mp_obj_get_int(args[1]);
            if (n_args > 2) {
                sock->proto = mp_obj_get_int(args[2]);
            }
        }
    }
	QPY_SOCKET_LOG("lwip_socket: %d, %d, %d\r\n", sock->domain, sock->type, sock->proto);
    sock->fd = lwip_socket(sock->domain, sock->type, sock->proto);
    if (sock->fd < 0) {
		QPY_SOCKET_LOG("lwip_socket: create socket failed!\r\n");
#if defined(PLAT_Qualcomm)
        exception_from_errno(errno(sock->fd));
#else
        exception_from_errno(errno);
#endif
    }

#if defined(PLAT_Qualcomm)
    _socket_settimeout(sock, UINT64_MAX);
    return MP_OBJ_FROM_PTR(sock);
#else
#if defined(PLAT_ASR)
    if (!netif_get_default_nw()) {
#endif
    	/*
    	** Jayceon 2020/12/03 --part2 begin--
    	** Solve the problem of not being able to connect to the Internet after dialing 
    	** with a PDP other than the first PDP . 
    	*/
    #if defined(PLAT_RDA)
        int pdp = 1;
        struct netif* netif_cfg = netif_get_by_cid(pdp);
    #else
        int pdp = Helios_DataCall_GetCurrentPDP();
    #if defined (PLAT_Unisoc)
        struct netif* netif_cfg = netif_get_by_cid(pdp);
    #elif defined (PLAT_ASR)
        struct netif* netif_cfg = netif_get_by_cid(pdp - 1);
    #endif
    #endif

        /* Jayceon 2020/12/03 --part2 end-- */
        if(netif_cfg)
        {
            if (sock->domain == AF_INET)
            {
                struct sockaddr_in local_v4;
                local_v4.sin_family = AF_INET;
                local_v4.sin_port = 0;
    #if defined (PLAT_Unisoc) || defined(PLAT_RDA)
                local_v4.sin_addr.s_addr = netif_cfg->ip_addr.u_addr.ip4.addr;
    #elif defined (PLAT_ASR)
                local_v4.sin_addr.s_addr = netif_cfg->ip_addr.addr;
    #endif
                if(lwip_bind(sock->fd,(struct sockaddr *)&local_v4,sizeof(struct sockaddr)))
                {
                    //bind failed.
                    QPY_SOCKET_LOG("[socket]bind IPV4 failed.\r\n");
                    exception_from_errno(errno);
                }
                else
                {
                    QPY_SOCKET_LOG("[socket]bind IPV4 success.\r\n");
                }
            }
            else if (sock->domain == AF_INET6)
            {
                struct sockaddr_in6 local_v6;

    #if defined (PLAT_Unisoc)
                const ip6_addr_t * ip6_addr_ptr = NULL;
                ip6_addr_ptr = netif_ip6_addr(netif_cfg, 1);
                memcpy(&local_v6.sin6_addr, ip6_addr_ptr, sizeof(ip6_addr_t));
    #elif defined (PLAT_ASR)
                ip6_addr_t * ip6_addr_ptr = NULL;
                ip6_addr_ptr = netif_get_global_ip6addr(netif_cfg);
                memcpy(&local_v6.sin6_addr, ip6_addr_ptr, sizeof(ip6_addr_t));
    #endif
                local_v6.sin6_family = AF_INET6;
                local_v6.sin6_port = 0;
                if(lwip_bind(sock->fd,(struct sockaddr *)&local_v6,sizeof(struct sockaddr_in6)))
                {
                    //bind failed.
                    QPY_SOCKET_LOG("[socket]bind IPV6 failed.\r\n");
                    exception_from_errno(errno);
                }
                else
                {
                    QPY_SOCKET_LOG("[socket]bind IPV6 success.\r\n");
                }
            }
        }
        else
        {
            QPY_SOCKET_LOG("[socket]find netif cfg failed..\r\n");
        }

        //end
#if defined(PLAT_ASR)
    }
#endif
    _socket_settimeout(sock, UINT64_MAX);

    return MP_OBJ_FROM_PTR(sock);
#endif
}

STATIC mp_obj_t socket_bind(const mp_obj_t arg0, const mp_obj_t arg1) {
    socket_obj_t *self = MP_OBJ_TO_PTR(arg0);
    struct addrinfo *res;
    _socket_getaddrinfo(arg1, &res);
    int r = lwip_bind(self->fd, res->ai_addr, res->ai_addrlen);
    lwip_freeaddrinfo(res);
    if (r < 0) {
#if defined(PLAT_Qualcomm)
        exception_from_errno(errno(self->fd));
#else
        exception_from_errno(errno);
#endif
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_bind_obj, socket_bind);

STATIC mp_obj_t socket_listen(const mp_obj_t arg0, const mp_obj_t arg1) {
    socket_obj_t *self = MP_OBJ_TO_PTR(arg0);
    int backlog = mp_obj_get_int(arg1);
    int r = lwip_listen(self->fd, backlog);
    if (r < 0) {
#if defined(PLAT_Qualcomm)
        exception_from_errno(errno(self->fd));
#else
        exception_from_errno(errno);
#endif
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_listen_obj, socket_listen);

STATIC mp_obj_t socket_accept(const mp_obj_t arg0) {
    socket_obj_t *self = MP_OBJ_TO_PTR(arg0);

    struct sockaddr addr;
    socklen_t addr_len = sizeof(addr);

    int new_fd = -1;
    for (unsigned int i = 0; i <= self->retries; i++) {
        MP_THREAD_GIL_EXIT();
        new_fd = lwip_accept(self->fd, &addr, &addr_len);
        MP_THREAD_GIL_ENTER();
        if (new_fd >= 0) {
            break;
        }
#if defined(PLAT_Qualcomm)
        int socket_error = errno(self->fd);
        if (socket_error != 11) {
            exception_from_errno(socket_error);
        }
#else
        if (errno != EAGAIN) {
            exception_from_errno(errno);
        }
#endif
        check_for_exceptions();
    }
    if (new_fd < 0) {
        if (self->retries == 0) {
            mp_raise_OSError(MP_EAGAIN);
        } else {
            mp_raise_OSError(MP_ETIMEDOUT);
        }
    }

    // create new socket object
    socket_obj_t *sock = m_new_obj_with_finaliser(socket_obj_t);
    sock->base.type = self->base.type;
    sock->fd = new_fd;
    sock->domain = self->domain;
    sock->type = self->type;
    sock->proto = self->proto;
    sock->peer_closed = false;
    _socket_settimeout(sock, UINT64_MAX);

    // make the return value
    uint8_t *ip = (uint8_t *)&((struct sockaddr_in *)&addr)->sin_addr;
    mp_uint_t port = lwip_ntohs(((struct sockaddr_in *)&addr)->sin_port);
    mp_obj_tuple_t *client = mp_obj_new_tuple(2, NULL);
    client->items[0] = sock;
    client->items[1] = netutils_format_inet_addr(ip, port, NETUTILS_BIG);

    return client;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(socket_accept_obj, socket_accept);

#if defined(PLAT_Qualcomm)
#define _htons(x) \
        ((unsigned short)((((unsigned short)(x) & 0x00ff) << 8) | \
                  (((unsigned short)(x) & 0xff00) >> 8)))
#endif

STATIC mp_obj_t socket_connect(const mp_obj_t arg0, const mp_obj_t arg1) {
    socket_obj_t *self = MP_OBJ_TO_PTR(arg0);
    int r;
#if defined(PLAT_Qualcomm)
    MP_THREAD_GIL_EXIT();

    mp_obj_t *elem;
    mp_obj_get_array_fixed_n(arg1, 2, &elem);
    const char *host_str = mp_obj_str_get_str(elem[0]);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = _htons(mp_obj_get_int(elem[1]));
    server_addr.sin_addr.s_addr = inet_addr(host_str);

    r = lwip_connect(self->fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    MP_THREAD_GIL_ENTER();
    if (r != 0) 
    {
        exception_from_errno(errno(self->fd));
    }
#else
    struct addrinfo *res;
    _socket_getaddrinfo(arg1, &res);
    MP_THREAD_GIL_EXIT();
    if (self->timeout > 0 && self->timeout < 25)
    {
        QPY_SOCKET_LOG("socket_connect usocket timeout = %d \r\n", self->timeout);
        int sock_nbio = 1;
        struct timeval t;
        fd_set read_fds, write_fds;

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        FD_SET(self->fd, &read_fds);
        FD_SET(self->fd, &write_fds);
        t.tv_sec = self->timeout;
        t.tv_usec = 0;
        
        lwip_ioctl(self->fd, FIONBIO, &sock_nbio);
        r = lwip_connect(self->fd, res->ai_addr, res->ai_addrlen);
        if(r == -1 && errno != EINPROGRESS)
        {
            QPY_SOCKET_LOG("socket_connect r = %d  errno=  %d\r\n", r, errno);
            MP_THREAD_GIL_ENTER();
            lwip_freeaddrinfo(res);
            exception_from_errno(errno);
        }

        r = lwip_select(self->fd + 1, &read_fds, &write_fds, NULL, &t);
        if(r <= 0)
        {
            QPY_SOCKET_LOG("*** select timeout or error ***\r\n");
            MP_THREAD_GIL_ENTER();
            lwip_freeaddrinfo(res);
            mp_raise_OSError(MP_ETIMEDOUT);
        }
        sock_nbio = 0;
        lwip_ioctl(self->fd, FIONBIO, &sock_nbio);
    }
    else
    {
        r = lwip_connect(self->fd, res->ai_addr, res->ai_addrlen);
        if (r != 0) 
        {
            QPY_SOCKET_LOG("timeout or error = %d \n", errno);
            MP_THREAD_GIL_ENTER();
            lwip_freeaddrinfo(res);
            exception_from_errno(errno);
        }
    }

    MP_THREAD_GIL_ENTER();
    lwip_freeaddrinfo(res);
#endif

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_connect_obj, socket_connect);

STATIC mp_obj_t socket_setsockopt(size_t n_args, const mp_obj_t *args) {
    (void)n_args; // always 4
    socket_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int opt = mp_obj_get_int(args[2]);

    switch (opt) {
        // level: SOL_SOCKET
        case SO_REUSEADDR: {
            int val = mp_obj_get_int(args[3]);
            int ret = lwip_setsockopt(self->fd, SOL_SOCKET, opt, &val, sizeof(int));
            if (ret != 0) {
#if defined(PLAT_Qualcomm)
                exception_from_errno(errno(self->fd));
#else
                exception_from_errno(errno);
#endif
            }
            break;
        }

        case TCP_KEEPALIVE: 
        {
            if (self->type == SOCK_STREAM)
            {            
                int val = mp_obj_get_int(args[3]);
                int optval = 1;
                int ret = lwip_setsockopt(self->fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int));

                optval = val*60;
                ret = lwip_setsockopt(self->fd, IPPROTO_TCP, TCP_KEEPIDLE, &optval, sizeof(int));

                optval = 25;
                ret = lwip_setsockopt(self->fd, IPPROTO_TCP, TCP_KEEPINTVL, &optval, sizeof(int));//TCP_KEEPINTVL

                optval = 3;
                ret = lwip_setsockopt(self->fd, IPPROTO_TCP, TCP_KEEPCNT, &optval, sizeof(int));
                if (ret != 0) {
#if defined(PLAT_Qualcomm)
                    exception_from_errno(errno(self->fd));
#else
                    exception_from_errno(errno);
#endif
                }
            }
            break;
        }

        #if MICROPY_PY_USOCKET_EVENTS
        // level: SOL_SOCKET
        // special "register callback" option
        case 20: {
            if (args[3] == mp_const_none) {
                if (self->events_callback != MP_OBJ_NULL) {
                    usocket_events_remove(self);
                    self->events_callback = MP_OBJ_NULL;
                }
            } else {
                if (self->events_callback == MP_OBJ_NULL) {
                    usocket_events_add(self);
                }
                self->events_callback = args[3];
            }
            break;
        }
            #endif

        // level: IPPROTO_IP
//        case IP_ADD_MEMBERSHIP: { //IP_ADD_MEMBERSHIP
//            mp_buffer_info_t bufinfo;
//            mp_get_buffer_raise(args[3], &bufinfo, MP_BUFFER_READ);
//            if (bufinfo.len != sizeof(ip_addr_t) * 2) {
//                mp_raise_ValueError(NULL);
//            }
//
//            // POSIX setsockopt has order: group addr, if addr, lwIP has it vice-versa
//            err_t err = igmp_joingroup((const ip_addr_t *)bufinfo.buf + 1, bufinfo.buf);
//            if (err != ERR_OK) {
//                mp_raise_OSError(-err);
//            }
//            break;
//        }

        default:
            mp_printf(&mp_plat_print, "Warning: lwip.setsockopt() option not implemented\n");
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socket_setsockopt_obj, 4, 4, socket_setsockopt);

void _socket_settimeout(socket_obj_t *sock, uint64_t timeout_ms) {
    // Rather than waiting for the entire timeout specified, we wait sock->retries times
    // for SOCKET_POLL_US each, checking for a MicroPython interrupt between timeouts.
    // with SOCKET_POLL_MS == 100ms, sock->retries allows for timeouts up to 13 years.
    // if timeout_ms == UINT64_MAX, wait forever.
    sock->retries = (timeout_ms == UINT64_MAX) ? UINT_MAX : timeout_ms * 1000 / SOCKET_POLL_US;

    sock->timeout = timeout_ms/1000;

    struct timeval timeout = {
        .tv_sec = 0,
        .tv_usec = timeout_ms ? SOCKET_POLL_US : 0
    };
    lwip_setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, (const void *)&timeout, sizeof(timeout));
    lwip_setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&timeout, sizeof(timeout));
    lwip_fcntl(sock->fd, F_SETFL, timeout_ms ? 0 : O_NONBLOCK);
}

STATIC mp_obj_t socket_settimeout(const mp_obj_t arg0, const mp_obj_t arg1) {
    socket_obj_t *self = MP_OBJ_TO_PTR(arg0);
    if (arg1 == mp_const_none) {
        _socket_settimeout(self, UINT64_MAX);
    } else {
    	uint64_t tmp = 0;
        #if MICROPY_PY_BUILTINS_FLOAT
		//_socket_settimeout(self, mp_obj_get_float(arg1) * 1000L);
		tmp = (uint64_t)(mp_obj_get_float(arg1) * 1000.0);
		_socket_settimeout(self, tmp);
        #else
        //_socket_settimeout(self, mp_obj_get_int(arg1) * 1000);
		tmp = (uint64_t)(mp_obj_get_float(arg1) * 1000.0);
		_socket_settimeout(self, tmp);
        #endif
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_settimeout_obj, socket_settimeout);

STATIC mp_obj_t socket_setblocking(const mp_obj_t arg0, const mp_obj_t arg1) {
    socket_obj_t *self = MP_OBJ_TO_PTR(arg0);
    if (mp_obj_is_true(arg1)) {
        _socket_settimeout(self, UINT64_MAX);
    } else {
        _socket_settimeout(self, 0);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_setblocking_obj, socket_setblocking);

// XXX this can end up waiting a very long time if the content is dribbled in one character
// at a time, as the timeout resets each time a recvfrom succeeds ... this is probably not
// good behaviour.
STATIC mp_uint_t _socket_read_data(mp_obj_t self_in, void *buf, size_t size,
    struct sockaddr *from, socklen_t *from_len, int *errcode) {
    socket_obj_t *sock = MP_OBJ_TO_PTR(self_in);

    // If the peer closed the connection then the lwIP socket API will only return "0" once
    // from lwip_recvfrom and then block on subsequent calls.  To emulate POSIX behaviour,
    // which continues to return "0" for each call on a closed socket, we set a flag when
    // the peer closed the socket.
    if (sock->peer_closed) {
        return MP_STREAM_ERROR;
    }

    unsigned int retries = 0;
    if (sock->timeout > 5)
    {
        retries = ((sock->timeout/5) - 1);
    }
    else
    {
        retries = sock->retries;
    }
    // XXX Would be nicer to use RTC to handle timeouts
    for (unsigned int i = 0; i <= retries; ++i) {
        if (sock->timeout > 5)
        {
            unsigned int retries_i = 0;
            retries_i = sock->timeout/5;
            if (!((retries_i - 1) == retries))
            {
                retries = retries_i;
            }
        }
        else
        {
            retries = sock->retries;
        }
        // Poll the socket to see if it has waiting data and only release the GIL if it doesn't.
        // This ensures higher performance in the case of many small reads, eg for readline.
        bool release_gil;
        {
            fd_set rfds;
            int r = 0;
            FD_ZERO(&rfds);
            FD_SET(sock->fd, &rfds);
            MP_THREAD_GIL_EXIT();
            if (sock->timeout > 5)
            {
                struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
                r = lwip_select(sock->fd + 1, &rfds, NULL, NULL, &timeout);
            }
            else
            {
                struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };
                r = lwip_select(sock->fd + 1, &rfds, NULL, NULL, &timeout);
            }
            MP_THREAD_GIL_ENTER();
            release_gil = r != 1;
        }
        if (release_gil) {
            MP_THREAD_GIL_EXIT();
        }
        int r = lwip_recvfrom(sock->fd, buf, size, 0, from, from_len);
        if (release_gil) {
            MP_THREAD_GIL_ENTER();
        }
#if defined(PLAT_Qualcomm)
        int socket_error = errno(sock->fd);
#endif
        if (r == 0) {
            sock->peer_closed = true;
#if defined(PLAT_Qualcomm)
            if (socket_error == ENOTCONN)//recv FIN
#else
            if (errno == ENOTCONN)//recv FIN
#endif
            {
                *errcode = ENOTCONN;
                QPY_SOCKET_LOG("_socket_read_data recv FIN \n");
                return MP_STREAM_ERROR;
            }
        }
        if (r >= 0) {
            return r;
        }
#if defined(PLAT_Qualcomm)
        if (socket_error != 6) {
            *errcode = socket_error;
            return MP_STREAM_ERROR;
        }
#else
        if (errno != EWOULDBLOCK) {
            *errcode = errno;
            return MP_STREAM_ERROR;
        }
        check_for_exceptions();
#endif
    }

    *errcode = retries == 0 ? MP_EWOULDBLOCK : MP_ETIMEDOUT;
    return MP_STREAM_ERROR;
}

mp_obj_t _socket_recvfrom(mp_obj_t self_in, mp_obj_t len_in,
    struct sockaddr *from, socklen_t *from_len) {
    size_t len = mp_obj_get_int(len_in);
    vstr_t vstr;
    vstr_init_len(&vstr, len);

    int errcode;
    mp_uint_t ret = _socket_read_data(self_in, vstr.buf, len, from, from_len, &errcode);
    if (ret == MP_STREAM_ERROR) {
        exception_from_errno(errcode);
    }

    vstr.len = ret;
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}

STATIC mp_obj_t socket_recv(mp_obj_t self_in, mp_obj_t len_in) {
    return _socket_recvfrom(self_in, len_in, NULL, NULL);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_recv_obj, socket_recv);

STATIC mp_obj_t socket_recvfrom(mp_obj_t self_in, mp_obj_t len_in) {
    struct sockaddr from;
    socklen_t fromlen = sizeof(from);

    mp_obj_t tuple[2];
    tuple[0] = _socket_recvfrom(self_in, len_in, &from, &fromlen);

    uint8_t *ip = (uint8_t *)&((struct sockaddr_in *)&from)->sin_addr;
    mp_uint_t port = lwip_ntohs(((struct sockaddr_in *)&from)->sin_port);
    tuple[1] = netutils_format_inet_addr(ip, port, NETUTILS_BIG);

    return mp_obj_new_tuple(2, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_recvfrom_obj, socket_recvfrom);

int _socket_send(socket_obj_t *sock, const char *data, size_t datalen) {
    int sentlen = 0;
    for (unsigned int i = 0; i <= sock->retries && sentlen < (int)datalen; i++) {
        MP_THREAD_GIL_EXIT();
        int r = lwip_write(sock->fd, data + sentlen, datalen - sentlen);
        MP_THREAD_GIL_ENTER();
#if defined(PLAT_Qualcomm)
        int socket_error = errno(sock->fd);
        if (r < 0 && socket_error != 11) {
            exception_from_errno(socket_error);
        }
#else
        if (r < 0 && errno != EWOULDBLOCK) {
            exception_from_errno(errno);
        }
#endif

        if (r > 0) {
            sentlen += r;
        }
        check_for_exceptions();
    }
    if (sentlen == 0) {
        mp_raise_OSError(MP_ETIMEDOUT);
    }
    return sentlen;
}

STATIC mp_obj_t socket_send(const mp_obj_t arg0, const mp_obj_t arg1) {
    socket_obj_t *sock = MP_OBJ_TO_PTR(arg0);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(arg1, &bufinfo, MP_BUFFER_READ);
    int r = _socket_send(sock, bufinfo.buf, bufinfo.len);
    return mp_obj_new_int(r);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_send_obj, socket_send);

STATIC mp_obj_t socket_sendall(const mp_obj_t arg0, const mp_obj_t arg1) {
    // XXX behaviour when nonblocking (see extmod/modlwip.c)
    // XXX also timeout behaviour.
    socket_obj_t *sock = MP_OBJ_TO_PTR(arg0);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(arg1, &bufinfo, MP_BUFFER_READ);
    int r = _socket_send(sock, bufinfo.buf, bufinfo.len);
    if (r < (int)bufinfo.len) {
        mp_raise_OSError(MP_ETIMEDOUT);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_sendall_obj, socket_sendall);

STATIC mp_obj_t socket_sendto(mp_obj_t self_in, mp_obj_t data_in, mp_obj_t addr_in) {
    socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // get the buffer to send
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data_in, &bufinfo, MP_BUFFER_READ);

    // create the destination address
    struct sockaddr_in to;
#if !defined(PLAT_Qualcomm)
    to.sin_len = sizeof(to);
#endif
    to.sin_family = AF_INET;
    to.sin_port = lwip_htons(netutils_parse_inet_addr(addr_in, (uint8_t *)&to.sin_addr, NETUTILS_BIG));

    // send the data
    for (unsigned int i = 0; i <= self->retries; i++) {
        MP_THREAD_GIL_EXIT();
        int ret = lwip_sendto(self->fd, bufinfo.buf, bufinfo.len, 0, (struct sockaddr *)&to, sizeof(to));
        MP_THREAD_GIL_ENTER();
        if (ret > 0) {
            return mp_obj_new_int_from_uint(ret);
        }
#if defined(PLAT_Qualcomm)
        int socket_error = errno(self->fd);
        if (ret == -1 && socket_error != 11) {
            exception_from_errno(socket_error);
        }
#else
        if (ret == -1 && errno != EWOULDBLOCK) {
            exception_from_errno(errno);
        }
#endif
        check_for_exceptions();
    }
    mp_raise_OSError(MP_ETIMEDOUT);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(socket_sendto_obj, socket_sendto);

STATIC mp_obj_t socket_fileno(const mp_obj_t arg0) {
    socket_obj_t *self = MP_OBJ_TO_PTR(arg0);
    return mp_obj_new_int(self->fd);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(socket_fileno_obj, socket_fileno);

STATIC mp_obj_t socket_makefile(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socket_makefile_obj, 1, 3, socket_makefile);

/*[Jayceon][2021/03/18][start]add this api to get socket state*/
extern int lwip_getTcpState(int nSocket);
STATIC mp_obj_t socket_getstate(const mp_obj_t self_in) 
{
	socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int32_t tcpstate = (int32_t)lwip_getTcpState(self->fd);
	return mp_obj_new_int(tcpstate);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(socket_getstate_obj, socket_getstate);
/*[Jayceon][2021/03/18][end]add this api to get socket state*/


STATIC mp_uint_t socket_stream_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    return _socket_read_data(self_in, buf, size, NULL, NULL, errcode);
}

STATIC mp_uint_t socket_stream_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    socket_obj_t *sock = self_in;
    for (unsigned int i = 0; i <= sock->retries; i++) {
        MP_THREAD_GIL_EXIT();
        int r = lwip_write(sock->fd, buf, size);
        MP_THREAD_GIL_ENTER();
        if (r > 0) {
            return r;
        }
#if defined(PLAT_Qualcomm)
        int socket_error = errno(sock->fd);
        if (r < 0 && socket_error != 11) {
            *errcode = socket_error;
            return MP_STREAM_ERROR;
        }
#else
        if (r < 0 && errno != EWOULDBLOCK) {
            *errcode = errno;
            return MP_STREAM_ERROR;
        }
#endif
        check_for_exceptions();
    }
    *errcode = sock->retries == 0 ? MP_EWOULDBLOCK : MP_ETIMEDOUT;
    return MP_STREAM_ERROR;
}

STATIC mp_uint_t socket_stream_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    socket_obj_t *socket = self_in;
    if (request == MP_STREAM_POLL) {

        fd_set rfds;
        FD_ZERO(&rfds);
        fd_set wfds;
        FD_ZERO(&wfds);
        fd_set efds;
        FD_ZERO(&efds);
        struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };
        if (arg & MP_STREAM_POLL_RD) {
            FD_SET(socket->fd, &rfds);
        }
        if (arg & MP_STREAM_POLL_WR) {
            FD_SET(socket->fd, &wfds);
        }
        if (arg & MP_STREAM_POLL_HUP) {
            FD_SET(socket->fd, &efds);
        }

        int r = lwip_select((socket->fd) + 1, &rfds, &wfds, &efds, &timeout);
        if (r < 0) {
            *errcode = MP_EIO;
            return MP_STREAM_ERROR;
        }

        mp_uint_t ret = 0;
        if (FD_ISSET(socket->fd, &rfds)) {
            ret |= MP_STREAM_POLL_RD;
        }
        if (FD_ISSET(socket->fd, &wfds)) {
            ret |= MP_STREAM_POLL_WR;
        }
        if (FD_ISSET(socket->fd, &efds)) {
            ret |= MP_STREAM_POLL_HUP;
        }
        return ret;
    } else if (request == MP_STREAM_CLOSE) {
        if (socket->fd >= 0) {
            #if MICROPY_PY_USOCKET_EVENTS
            if (socket->events_callback != MP_OBJ_NULL) {
                usocket_events_remove(socket);
                socket->events_callback = MP_OBJ_NULL;
            }
            #endif
            int ret = lwip_close(socket->fd);
            if (ret != 0) {
#if defined(PLAT_Qualcomm)
                *errcode = errno(socket->fd);
#else
                *errcode = errno;
#endif
                return MP_STREAM_ERROR;
            }
            socket->fd = -1;
        }
        return 0;
    }

    *errcode = MP_EINVAL;
    return MP_STREAM_ERROR;
}

STATIC const mp_rom_map_elem_t socket_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_bind), MP_ROM_PTR(&socket_bind_obj) },
    { MP_ROM_QSTR(MP_QSTR_listen), MP_ROM_PTR(&socket_listen_obj) },
    { MP_ROM_QSTR(MP_QSTR_accept), MP_ROM_PTR(&socket_accept_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&socket_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&socket_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendall), MP_ROM_PTR(&socket_sendall_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendto), MP_ROM_PTR(&socket_sendto_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&socket_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_recvfrom), MP_ROM_PTR(&socket_recvfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_setsockopt), MP_ROM_PTR(&socket_setsockopt_obj) },
    { MP_ROM_QSTR(MP_QSTR_settimeout), MP_ROM_PTR(&socket_settimeout_obj) },
    { MP_ROM_QSTR(MP_QSTR_setblocking), MP_ROM_PTR(&socket_setblocking_obj) },
    { MP_ROM_QSTR(MP_QSTR_makefile), MP_ROM_PTR(&socket_makefile_obj) },
    { MP_ROM_QSTR(MP_QSTR_fileno), MP_ROM_PTR(&socket_fileno_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getsocketsta), MP_ROM_PTR(&socket_getstate_obj) },

    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
};
STATIC MP_DEFINE_CONST_DICT(socket_locals_dict, socket_locals_dict_table);

STATIC const mp_stream_p_t socket_stream_p = {
    .read = socket_stream_read,
    .write = socket_stream_write,
    .ioctl = socket_stream_ioctl
};

STATIC const mp_obj_type_t socket_type = {
    { &mp_type_type },
    .name = MP_QSTR_socket,
    .make_new = socket_make_new,
    .protocol = &socket_stream_p,
    .locals_dict = (mp_obj_t)&socket_locals_dict,
};

#if 0
STATIC mp_obj_t socket_getaddrinfo2(size_t n_args, const mp_obj_t *args) {
    // TODO support additional args beyond the first two
	mp_hal_stdout_tx_str("esp_socket_getaddrinfo entry");
    struct addrinfo *res = NULL;
    _socket_getaddrinfo2(args[0], args[1], &res);
    mp_obj_t ret_list = mp_obj_new_list(0, NULL);

    for (struct addrinfo *resi = res; resi; resi = resi->ai_next) {
        mp_obj_t addrinfo_objs[5] = {
            mp_obj_new_int(resi->ai_family),
            mp_obj_new_int(resi->ai_socktype),
            mp_obj_new_int(resi->ai_protocol),
            mp_obj_new_str(resi->ai_canonname, strlen(resi->ai_canonname)),
            mp_const_none
        };

        if (resi->ai_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)resi->ai_addr;
            // This looks odd, but it's really just a u32_t
            ip_addr_t ip4_addr = { .addr = addr->sin_addr.s_addr };
			//ip_addr_t ip4_addr = { .u_addr.ip4.addr = addr->sin_addr.s_addr };
            char buf[16];
            ipaddr_ntoa_r(&ip4_addr, buf, sizeof(buf));
            mp_obj_t inaddr_objs[2] = {
                mp_obj_new_str(buf, strlen(buf)),
                mp_obj_new_int(ntohs(addr->sin_port))
            };
            addrinfo_objs[4] = mp_obj_new_tuple(2, inaddr_objs);
        }
        mp_obj_list_append(ret_list, mp_obj_new_tuple(5, addrinfo_objs));
    }

    if (res) {
        lwip_freeaddrinfo(res);
    }
    return ret_list;
}
#endif

STATIC mp_obj_t socket_getaddrinfo(size_t n_args, const mp_obj_t *args) {
    // TODO support additional args beyond the first two

    struct addrinfo *res = NULL;
    _socket_getaddrinfo2(args[0], args[1], &res);
    mp_obj_t ret_list = mp_obj_new_list(0, NULL);

    for (struct addrinfo *resi = res; resi; resi = resi->ai_next) {
        mp_obj_t addrinfo_objs[5] = {
            mp_obj_new_int(resi->ai_family),
            mp_obj_new_int(resi->ai_socktype),
            mp_obj_new_int(resi->ai_protocol),
            mp_obj_new_str(resi->ai_canonname, strlen(resi->ai_canonname)),
            mp_const_none
        };

        if (resi->ai_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)resi->ai_addr;
            // This looks odd, but it's really just a u32_t
        #if defined (PLAT_Unisoc) || defined (PLAT_RDA)
            ip_addr_t ip4_addr = { .u_addr.ip4.addr = addr->sin_addr.s_addr };
        #elif defined (PLAT_ASR)
            ip_addr_t ip4_addr = { .addr = addr->sin_addr.s_addr };
        #endif
            char buf[16];
#if defined (PLAT_Qualcomm)
            //struct ipv4_info ip4_addr = { .addr = addr->sin_addr.s_addr };
            inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf));
#else
            ipaddr_ntoa_r(&ip4_addr, buf, sizeof(buf));
#endif
            mp_obj_t inaddr_objs[2] = {
                mp_obj_new_str(buf, strlen(buf)),
                mp_obj_new_int(ntohs(addr->sin_port))
            };
            addrinfo_objs[4] = mp_obj_new_tuple(2, inaddr_objs);
        }
#if !defined (PLAT_Qualcomm)
        if (resi->ai_family == AF_INET6) {
            char ip6_addr_buf[128] = {0};
            struct sockaddr_in6 *addr = (struct sockaddr_in6 *)resi->ai_addr;
            inet_ntop(AF_INET6, &addr->sin6_addr, ip6_addr_buf, sizeof(ip6_addr_buf));
            mp_obj_t inaddr_objs[2] = {
                mp_obj_new_str(ip6_addr_buf, strlen(ip6_addr_buf)),
                mp_obj_new_int(ntohs(addr->sin6_port))
            };
            addrinfo_objs[4] = mp_obj_new_tuple(2, inaddr_objs);
        }
#endif
        mp_obj_list_append(ret_list, mp_obj_new_tuple(5, addrinfo_objs));
    }

    if (res) {
        lwip_freeaddrinfo(res);
    }
    return ret_list;

}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socket_getaddrinfo_obj, 2, 6, socket_getaddrinfo);

STATIC mp_obj_t socket_initialize() {
    static int initialized = 0;
    if (!initialized) {
        mp_hal_stdout_tx_str("modsocket:Initializing");
       // tcpip_adapter_init();
        initialized = 1;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(socket_initialize_obj, socket_initialize);

STATIC const mp_rom_map_elem_t mp_module_socket_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_usocket) },
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&socket_initialize_obj) },
    { MP_ROM_QSTR(MP_QSTR_socket), MP_ROM_PTR(&socket_type) },
    { MP_ROM_QSTR(MP_QSTR_getaddrinfo), MP_ROM_PTR(&socket_getaddrinfo_obj) },

    { MP_ROM_QSTR(MP_QSTR_AF_INET), MP_ROM_INT(AF_INET) },
    { MP_ROM_QSTR(MP_QSTR_AF_INET6), MP_ROM_INT(AF_INET6) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_STREAM), MP_ROM_INT(SOCK_STREAM) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_DGRAM), MP_ROM_INT(SOCK_DGRAM) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_RAW), MP_ROM_INT(SOCK_RAW) },
    { MP_ROM_QSTR(MP_QSTR_IPPROTO_TCP), MP_ROM_INT(IPPROTO_TCP) },
    { MP_ROM_QSTR(MP_QSTR_IPPROTO_UDP), MP_ROM_INT(IPPROTO_UDP) },
    { MP_ROM_QSTR(MP_QSTR_IPPROTO_IP), MP_ROM_INT(IPPROTO_IP) },
    { MP_ROM_QSTR(MP_QSTR_SOL_SOCKET), MP_ROM_INT(SOL_SOCKET) },
    { MP_ROM_QSTR(MP_QSTR_SO_REUSEADDR), MP_ROM_INT(SO_REUSEADDR) },
    { MP_ROM_QSTR(MP_QSTR_TCP_KEEPALIVE), MP_ROM_INT(TCP_KEEPALIVE) },
    { MP_ROM_QSTR(MP_QSTR_IP_ADD_MEMBERSHIP), MP_ROM_INT(IP_ADD_MEMBERSHIP) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_socket_globals, mp_module_socket_globals_table);

const mp_obj_module_t mp_module_usocket = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_socket_globals,
};
