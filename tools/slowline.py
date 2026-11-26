#!/usr/bin/env python3
"""A TCP proxy that is a slow line, to try Archaic on a slow line.

  slowline.py PORT TARGET BPS [UP [ADDR]]

It listens on 127.0.0.1:PORT (or on ADDR) and connects each client to
127.0.0.1:TARGET, where WRP is. Bytes to the client go at BPS bytes a
second, bytes from the client at UP (default: BPS). 0: as fast as they
come. A line of 19200 baud carries 1920 bytes a second:

  python3 tools/slowline.py 8098 8080 1920
  archaic 127.0.0.1:8098        (in an emulator kit: 10.0.2.2:8098)

For each connection it prints the seconds it was open, and the bytes
that went to the client.
"""
import socket
import sys
import threading
import time


def pump(src, dst, bps, count):
    t = time.time()
    try:
        while True:
            b = src.recv(max(1, int(bps / 20)) if bps else 4096)
            if not b:
                break
            if bps:
                t = max(t, time.time() - 0.2) + len(b) / bps
                d = t - time.time()
                if d > 0:
                    time.sleep(d)
            dst.sendall(b)
            count[0] += len(b)
    except OSError:
        pass
    try:
        dst.shutdown(socket.SHUT_WR)
    except OSError:
        pass


def serve(c, target, down, up):
    try:
        s = socket.create_connection(("127.0.0.1", target))
    except OSError as e:
        print("port %d: %s" % (target, e), flush=True)
        c.close()
        return
    for x in (c, s):
        x.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    n, m, t0 = [0], [0], time.time()
    a = threading.Thread(target=pump, args=(c, s, up, m), daemon=True)
    a.start()
    pump(s, c, down, n)
    a.join(5)
    print("%.2f s, %d bytes" % (time.time() - t0, n[0]), flush=True)
    c.close()
    s.close()


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    port, target, down = int(sys.argv[1]), int(sys.argv[2]), float(sys.argv[3])
    up = float(sys.argv[4]) if len(sys.argv) > 4 else down
    addr = sys.argv[5] if len(sys.argv) > 5 else "127.0.0.1"
    l = socket.socket()
    l.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    l.bind((addr, port))
    l.listen(16)
    print("a line of %g bytes a second on %s:%d, to port %d" % (down, addr, port, target), flush=True)
    while True:
        c, _ = l.accept()
        threading.Thread(target=serve, args=(c, target, down, up), daemon=True).start()


if __name__ == "__main__":
    main()
