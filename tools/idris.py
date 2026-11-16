#!/usr/bin/env python3
"""Drive Idris (Atari ST, Hatari) through netd's telnet port.

  idris.py PORT run CMD...          log in as root, run each command, print
                                    what it prints
  idris.py PORT put DIR FILE...     copy text files into DIR (cat > NAME),
                                    one line at a time (slow: use it only
                                    to bring in hget.c)
  idris.py PORT serve DIR [HPORT]   serve DIR over HTTP on 127.0.0.1:HPORT
                                    (default 8077) for hget in Idris, which
                                    reaches it as 10.0.2.2:HPORT

PORT is the Mac port that run.sh forwards to Idris port 23 (NETPORT).
"""
import functools
import http.server
import select
import socket
import sys
import time

IAC, DONT, DO, WONT, WILL = 255, 254, 253, 252, 251


class Tel:
    def __init__(self, port):
        self.s = socket.create_connection(("127.0.0.1", port), timeout=10)
        self.buf = b""

    def feed(self, data):
        out, i = bytearray(), 0
        while i < len(data):
            c = data[i]
            if c == IAC and i + 1 < len(data):
                cmd = data[i + 1]
                if cmd in (WILL, WONT, DO, DONT) and i + 2 < len(data):
                    opt = data[i + 2]
                    if cmd == WILL:
                        self.s.sendall(bytes([IAC, DO if opt in (1, 3) else DONT, opt]))
                    elif cmd == DO:
                        self.s.sendall(bytes([IAC, WONT, opt]))
                    i += 3
                    continue
                if cmd == IAC:
                    out.append(IAC)
                i += 2
                continue
            out.append(c)
            i += 1
        self.buf += bytes(out)

    def expect(self, text, secs=60):
        end = time.time() + secs
        while text.encode() not in self.buf:
            r, _, _ = select.select([self.s], [], [], 0.2)
            if r:
                d = self.s.recv(4096)
                if not d:
                    raise SystemExit("closed while waiting for %r; got %r" % (text, self.buf[-300:]))
                self.feed(d)
            if time.time() > end:
                raise SystemExit("timeout waiting for %r; got %r" % (text, self.buf[-300:]))
        i = self.buf.index(text.encode()) + len(text)
        seen, self.buf = self.buf[:i], self.buf[i:]
        return seen

    def send(self, text):
        self.s.sendall(text.encode("latin-1"))


def login(port):
    t = Tel(port)
    t.expect("login: ", 90)
    t.send("root\r\n")
    t.expect("# ", 60)
    return t


def clean(b):
    return b.replace(b"\0", b"").replace(b"\r", b"").decode("latin-1")


def run(port, cmds):
    t = login(port)
    for i, cmd in enumerate(cmds):
        mark = "XDONE%d" % i
        line = cmd + "; echo " + mark
        t.send(line + "\r")
        t.expect(line, 120)
        out = t.expect(mark + "\r\n", 3600)
        sys.stdout.write(clean(out[: -len(mark) - 2]).lstrip("\n"))
        sys.stdout.flush()
    t.s.close()


def put(port, d, files):
    t = login(port)
    for path in files:
        name = path.rsplit("/", 1)[-1]
        data = open(path, encoding="latin-1").read()
        t.send("cd %s; cat > %s\r\n" % (d, name))
        time.sleep(2)
        t0 = time.time()
        for line in data.splitlines():
            if "\t" in line or any(ord(c) < 32 for c in line):
                line = line.expandtabs(8)
            t.send(line + "\r\n")
            t.expect("\r\n", 60)
        # Idris takes Ctrl-D as end of file only while cat waits for input
        time.sleep(3)
        t.send("\x04")
        t.expect("# ", 30)
        t.send("wc %s\r\n" % name)
        seen = t.expect("# ", 30)
        print("%s: %d bytes sent in %.0f s; Idris wc: %s" % (
            name, len(data), time.time() - t0,
            clean(seen).strip().splitlines()[-2].strip()))
    t.s.close()


def serve(d, hport):
    h = functools.partial(http.server.SimpleHTTPRequestHandler, directory=d)
    http.server.ThreadingHTTPServer.allow_reuse_address = True
    srv = http.server.ThreadingHTTPServer(("127.0.0.1", hport), h)
    print("serving %s on 127.0.0.1:%d (Idris: 10.0.2.2:%d)" % (d, hport, hport), flush=True)
    srv.serve_forever()


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    port, what = int(sys.argv[1]), sys.argv[2]
    if what == "run":
        run(port, sys.argv[3:])
    elif what == "put":
        put(port, sys.argv[3], sys.argv[4:])
    elif what == "serve":
        serve(sys.argv[3], int(sys.argv[4]) if len(sys.argv) > 4 else 8077)
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
