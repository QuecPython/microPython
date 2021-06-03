# Test basic, stand-alone TCP socket functionality

try:
    import usocket as socket, uerrno as errno
except ImportError:
    try:
        import socket, errno
    except ImportError:
        print("SKIP")
        raise SystemExit

# recv() on a fresh socket should raise ENOTCONN
s = socket.socket()
try:
    s.recv(1)
except OSError as er:
    print("ENOTCONN:", er.args[0] == errno.ENOTCONN)
