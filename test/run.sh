#!/bin/sh
# Tests for Archaic on the Mac (or any Unix), without a window (-t, and
# the headless display).
#   sh test/run.sh [archaic binary] [WRP address] [archaic-headless binary]
# Needs: python3 (a small web server), ImageMagick (magick) to check the
# GIF decoder. The WRP part runs only when WRP answers at the address.
bin=${1:-./archaic-asan}
wrp=${2:-127.0.0.1:8089}
hl=${3:-./archaic-headless}
port=8079
tmp=$(mktemp -d)
fail=0
here=$(cd "$(dirname "$0")" && pwd)

# The web server for the tests: python's, and GIFs cut into slices for
# ?y=Y&h=H, as WRP does for the tall picture (with ImageMagick). ?wait=S:
# the answer comes after S seconds, as when WRP makes a page.
# NAME.pic is NAME.abm for a request whose Accept has image/x-abm, as WRP
# does for a black and white picture, else NAME.gif; the log gets an
# "accept" line.
cat >"$tmp/srv.py" <<'EOF'
import functools, http.server, subprocess, sys, time, urllib.parse
class H(http.server.SimpleHTTPRequestHandler):
    extensions_map = dict(http.server.SimpleHTTPRequestHandler.extensions_map,
        **{'.abm': 'image/x-abm'})
    def do_GET(self):
        u = urllib.parse.urlsplit(self.path)
        q = urllib.parse.parse_qs(u.query)
        if 'wait' in q:
            time.sleep(float(q['wait'][0]))
        if u.path.endswith('.pic'):
            acc = [t.strip() for t in self.headers.get('Accept', '').split(',')]
            ext = '.abm' if 'image/x-abm' in acc else '.gif'
            sys.stderr.write('accept %s %s: %s\n' % (u.path, ext, ' '.join(acc)))
            b = open(self.translate_path(u.path)[:-4] + ext, 'rb').read()
            self.send_response(200)
            self.send_header('Content-Type', self.extensions_map.get(ext, 'image/gif'))
            self.send_header('Content-Length', str(len(b)))
            self.end_headers()
            self.wfile.write(b)
            return
        if u.path == '/archaic-update':     # as WRP: upd.txt, the file upd.bin
            n = 'upd.bin' if 'f' in q else 'upd.txt'
            try:
                b = open(self.translate_path('/' + n), 'rb').read()
            except OSError:
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header('Content-Type', 'application/octet-stream' if 'f' in q else 'text/plain')
            self.send_header('Content-Length', str(len(b)))
            self.end_headers()
            self.wfile.write(b)
            return
        if u.path.endswith('.gif') and 'y' in q and 'h' in q:
            crop = 'x%d+0+%d' % (int(q['h'][0]), int(q['y'][0]))
            b = subprocess.run(['magick', self.translate_path(u.path), '-crop',
                crop, '+repage', 'gif:-'], capture_output=True).stdout
            self.send_response(200)
            self.send_header('Content-Type', 'image/gif')
            self.send_header('Content-Length', str(len(b)))
            self.end_headers()
            self.wfile.write(b)
            return
        super().do_GET()
http.server.ThreadingHTTPServer(('127.0.0.1', int(sys.argv[1])),
    functools.partial(H, directory=sys.argv[2])).serve_forever()
EOF
python3 -I "$tmp/srv.py" "$port" "$here/data" >"$tmp/http.log" 2>&1 &
srv=$!
trap 'kill $srv 2>/dev/null; wait $srv 2>/dev/null; rm -rf "$tmp"' EXIT
sleep 1

ok() { echo "ok    $*"; }
bad() { echo "FAIL  $*"; fail=1; }

# 1. Black and white GIFs: the decoder must give exactly the same pixels
#    as ImageMagick (gray below 50% = black).
for f in hn2 wiki2; do
	"$bin" -t -o "$tmp/$f.pbm" "http://127.0.0.1:$port/$f.gif" >"$tmp/$f.out" 2>&1
	magick "$here/data/$f.gif" -colorspace gray -threshold 50% "$tmp/$f-ref.pbm"
	if cmp -s "$tmp/$f.pbm" "$tmp/$f-ref.pbm"; then
		ok "$f.gif decodes exactly"
	else
		bad "$f.gif differs from ImageMagick"
	fi
done

# 1b. ABM, WRP's black and white picture for the 1-bit displays: exactly
#     the pixels of the GIF.
for f in hn2 wiki2; do
	"$bin" -t -o "$tmp/$f-abm.pbm" "http://127.0.0.1:$port/$f.abm" >"$tmp/$f-abm.out" 2>&1
	if cmp -s "$tmp/$f-abm.pbm" "$tmp/$f-ref.pbm" &&
		grep -q ' got [0-9]*x[0-9]* abm sum [0-9a-f]*$' "$tmp/$f-abm.out"; then
		ok "$f.abm decodes exactly as the GIF"
	else
		bad "$f.abm differs from the GIF:"
		tail -2 "$tmp/$f-abm.out"
	fi
done
# a broken ABM (bytes from before the picture's start) is refused
printf 'ABM1\000\020\000\020\001\000\377\377\377' >"$here/data/bad.abm"
"$bin" -t -v "http://127.0.0.1:$port/bad.abm" >"$tmp/bad.out" 2>&1
if grep -q "^status: Picture: can't show image/x-abm" "$tmp/bad.out"; then
	ok "a broken ABM is refused"
else
	bad "a broken ABM:"
	tail -3 "$tmp/bad.out"
fi
rm -f "$here/data/bad.abm"

# 1c. What Archaic asks for: ABM and GIF; with -f gif, only GIF. ABM is
#     1 bit a pixel also on a display with colors.
want() {	# $1: Archaic's options, $2: what must come, $3: the Accept
	"$bin" -t $1 "http://127.0.0.1:$port/hn2.pic" >"$tmp/want.out" 2>&1
	acc=$(grep '^accept /hn2.pic' "$tmp/http.log" | tail -1)
	grep -q " got 632x330 $2\$" "$tmp/want.out" && [ "${acc#*: }" = "$3" ]
}
if want "" "abm sum ae2b58f3" "text/html image/x-abm image/gif" &&
	want "-c 256" "abm sum ae2b58f3" "text/html image/x-abm image/gif" &&
	want "-f abm" "abm sum ae2b58f3" "text/html image/x-abm image/gif" &&
	want "-f gif" "gif sum ae2b58f3" "text/html image/gif" &&
	want "-f gif -c 256" "gif sum [0-9a-f]* colors" "text/html image/gif"; then
	ok "asks for ABM and GIF; -f gif: only for GIF; ABM is 1 bit also with colors"
else
	bad "what Archaic asks for: $acc"
	tail -2 "$tmp/want.out"
fi

# 2. Gray and color GIFs (dithered): the size must be right.
for f in hn216 ex16 full runs ilace; do
	"$bin" -t -o "$tmp/$f.pbm" "http://127.0.0.1:$port/$f.gif" >"$tmp/$f.out" 2>&1
	want=$(magick identify -format '%wx%h' "$here/data/$f.gif[0]")
	got=$(grep -o 'got [0-9]*x[0-9]*' "$tmp/$f.out" | head -1 | cut -c5-)
	if [ "$want" = "$got" ]; then
		ok "$f.gif decodes, $got"
	else
		bad "$f.gif: want $want, got '$got'"
	fi
done

# 2b. With colors (-c 256: as on a display with colors), the pixels
#     keep their colors: the same as ImageMagick's, exactly. The 8-bit
#     way (lzw8): also full.gif (the code table full, and no clear) and
#     runs.gif (long runs: codes made from the last one); ilace.gif is
#     interlaced, so it goes the slow way.
for f in hn216 ex16 hn2 full runs ilace; do
	"$bin" -t -c 256 -o "$tmp/$f.ppm" "http://127.0.0.1:$port/$f.gif" >"$tmp/$f.cout" 2>&1
	magick "$here/data/$f.gif[0]" -depth 8 "$tmp/$f-ref.ppm"
	if grep -q ' got [0-9]*x[0-9]* gif sum [0-9a-f]* colors$' "$tmp/$f.cout" &&
		[ "$(head -c 2 "$tmp/$f.ppm")" = P6 ] &&
		magick compare -metric AE "$tmp/$f.ppm" "$tmp/$f-ref.ppm" null: 2>/dev/null; then
		ok "$f.gif decodes in its colors exactly"
	else
		bad "$f.gif: its colors differ from ImageMagick's"
	fi
done

# 3. HTML: forms, menus, links, entities, comments.
cat >"$tmp/t.html" <<'EOF'
<HTML><HEAD><TITLE>T&amp;T &#65;&#x42;</TITLE></HEAD><BODY>
<!-- <INPUT NAME="commented"> -->
<FORM ACTION="/go" METHOD=GET>
<INPUT TYPE=TEXT NAME=q VALUE="a &quot;b&quot;" SIZE=10>
<SELECT NAME=s><OPTION VALUE=1>One<OPTION SELECTED VALUE=2>Two</SELECT>
<INPUT TYPE=HIDDEN NAME=h VALUE=x>
<INPUT TYPE=SUBMIT NAME=b VALUE="Send it">
</FORM>
<P>Text <A HREF="/l">a link</A> &lt;end&gt;
</BODY></HTML>
EOF
cp "$tmp/t.html" "$here/data/t.html"
"$bin" -t "http://127.0.0.1:$port/t.html" >"$tmp/t.out" 2>&1
rm -f "$here/data/t.html"
for want in 'title "T&T AB"' 'field' ' q ="a "b""' 'select' 'sel 1: 1=One 2=Two' \
	'hidden' 'button' '"Send it"' 'href=/l' '"<end>"'; do
	if grep -qF -- "$want" "$tmp/t.out"; then
		ok "html: $want"
	else
		bad "html: no $want"
	fi
done
if grep -q commented "$tmp/t.out"; then
	bad "html: a commented-out INPUT was used"
else
	ok "html: comments skipped"
fi

# 4. Clicking: the submit button sends the form as a GET query.
cp "$tmp/t.html" "$here/data/t.html"
x=$(awk '$2 == "button" { print $3 + 2; exit }' "$tmp/t.out")
y=$(awk '$2 == "button" { print $4 + 2; exit }' "$tmp/t.out")
"$bin" -t -s "@$x,$y;" "http://127.0.0.1:$port/t.html" >"$tmp/s.out" 2>&1
rm -f "$here/data/t.html"
if grep -q 'GET /go?q=a+%22b%22&s=2&h=x&b=Send+it ' "$tmp/http.log"; then
	ok "submit: GET /go?q=a+%22b%22&s=2&h=x&b=Send+it"
else
	bad "submit: the query is wrong:"
	grep 'GET /go' "$tmp/http.log"
fi

# 5. WRP's form: Archaic hides W, H, M, T, C (and their labels) and sends
#    its own values: the window's size, ISMAP, GIF, 2 colors. With -a,
#    M, T and C are shown and sent as set. WRP's buttons get pictures.
#    With colors (-c), C is shown, set to them, and a new choice there
#    sends the form at once with St. (D, the dither menu: 5a.)
cat >"$tmp/wrp.html" <<'EOF'
<HTML><HEAD><TITLE>WRP</TITLE></HEAD><BODY>
<FORM ACTION="/wrp" METHOD="GET">
<INPUT TYPE="TEXT" NAME="url" VALUE="" SIZE="20">
<INPUT TYPE="SUBMIT" VALUE="Go">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="Bk">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="St">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="Re">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="Up">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="Dn">
W <INPUT TYPE="TEXT" NAME="w" VALUE="1152" SIZE="4">
H <INPUT TYPE="TEXT" NAME="h" VALUE="600" SIZE="4">
Z <SELECT NAME="z"><OPTION DISABLED>Zoom</OPTION><OPTION VALUE="1.0" SELECTED>1.0 x</OPTION></SELECT>
M <SELECT NAME="m"><OPTION VALUE="ismap" SELECTED>ISMAP</OPTION><OPTION VALUE="html">HTML</OPTION></SELECT>
T <SELECT NAME="t"><OPTION VALUE="gip" SELECTED>GIP</OPTION><OPTION VALUE="gif">GIF</OPTION></SELECT>
C <SELECT NAME="c">
<OPTION DISABLED>Ncol</OPTION>
<OPTION VALUE="256">256</OPTION>
<OPTION VALUE="216" SELECTED>216</OPTION>
<OPTION VALUE="128">128</OPTION>
<OPTION VALUE="64">64</OPTION>
<OPTION VALUE="16">16</OPTION>
<OPTION VALUE="2">2</OPTION>
</SELECT>
K <INPUT TYPE="TEXT" NAME="k" VALUE="" SIZE="4">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="Bs">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="Rt">
</FORM>
</BODY></HTML>
EOF
cp "$tmp/wrp.html" "$here/data/wrp.html"
"$bin" -t "http://127.0.0.1:$port/wrp.html" >"$tmp/f.out" 2>&1
shown=$(awk '$2 != "hidden" && $8 ~ /^"?[whmtcWHMTC]"?$/' "$tmp/f.out")
if [ -z "$shown" ] && grep -q 'select.* z sel' "$tmp/f.out" &&
	grep -q 'field.* k =' "$tmp/f.out"; then
	ok "wrp form: W, H, M, T, C hidden; Z and K shown"
else
	bad "wrp form: shows the wrong things:"
	echo "$shown"
fi
# WRP's 7 buttons (NAME=Fn) are pictures; Go and other forms' buttons are text.
if [ "$(grep -c '^ *[0-9]* button .* Fn "[A-Za-z]*" icon$' "$tmp/f.out")" = 7 ] &&
	! grep -q '"Go" icon' "$tmp/f.out" && ! grep -q 'icon' "$tmp/t.out"; then
	ok "wrp form: Bk St Re Up Dn Bs Rt are pictures, Go is text"
else
	bad "wrp form: the wrong buttons are pictures:"
	grep 'button' "$tmp/f.out"
fi
x=$(awk '$2 == "field" && $8 == "url" { print $3 + 5; exit }' "$tmp/f.out")
y=$(awk '$2 == "field" && $8 == "url" { print $4 + 5; exit }' "$tmp/f.out")
"$bin" -t -s "@$x,$y;x~" "http://127.0.0.1:$port/wrp.html" >/dev/null 2>&1
"$bin" -t -a -s "@$x,$y;y~" "http://127.0.0.1:$port/wrp.html" >"$tmp/fa.out" 2>&1
"$bin" -t -c 256 "http://127.0.0.1:$port/wrp.html" >"$tmp/fc.out" 2>&1
"$bin" -t -c 100 "http://127.0.0.1:$port/wrp.html" >"$tmp/fc100.out" 2>&1
cx=$(awk '$2 == "select" && $8 == "c" { print $3 + 5; exit }' "$tmp/fc.out")
cy=$(awk '$2 == "select" && $8 == "c" { print $4 + 5; exit }' "$tmp/fc.out")
oy=$(awk '$2 == "select" && $8 == "c" { print $4 + $6 + 1 + 5 * 15 + 7; exit }' "$tmp/fc.out")
"$bin" -t -c 256 -s "@$x,$y;z~" "http://127.0.0.1:$port/wrp.html" >/dev/null 2>&1
"$bin" -t -c 256 -s "@$cx,$cy;@$cx,$oy;" "http://127.0.0.1:$port/wrp.html" >/dev/null 2>&1
"$bin" -t -c 256 "http://127.0.0.1:$port/" >/dev/null 2>&1
# K: its text, then the key of Rt or Bs, as keys (k), and no Fn. The url
# field's clear button (at its right end) empties it.
kx=$(awk '$2 == "field" && $8 == "k" { print $3 + 5; exit }' "$tmp/f.out")
ky=$(awk '$2 == "field" && $8 == "k" { print $4 + 5; exit }' "$tmp/f.out")
rt=$(awk '$2 == "button" && $9 == "\"Rt\"" { print $3 + 5 "," $4 + 5; exit }' "$tmp/f.out")
bs=$(awk '$2 == "button" && $9 == "\"Bs\"" { print $3 + 5 "," $4 + 5; exit }' "$tmp/f.out")
xx=$(awk '$2 == "field" && $8 == "url" { print $3 + $5 - 8; exit }' "$tmp/f.out")
"$bin" -t -s "@$kx,$ky;ab@$rt;" "http://127.0.0.1:$port/wrp.html" >/dev/null 2>&1
"$bin" -t -s "@$kx,$ky;cd@$bs;" "http://127.0.0.1:$port/wrp.html" >/dev/null 2>&1
"$bin" -t -s "@$rt;" "http://127.0.0.1:$port/wrp.html" >/dev/null 2>&1
"$bin" -t -s "@$x,$y;abc@$xx,$y;qq~" "http://127.0.0.1:$port/wrp.html" >/dev/null 2>&1
rm -f "$here/data/wrp.html"
if grep -Eq 'GET /wrp\?url=x&z=1\.0&k=&w=632&h=[0-9]+&sl=1&m=ismap&t=gif&c=2 ' "$tmp/http.log"; then
	ok "wrp form: sends $(grep -Eo 'w=632&h=[0-9]+&sl=1&m=ismap&t=gif&c=2' "$tmp/http.log")"
else
	bad "wrp form: the query is wrong:"
	grep 'GET /wrp?url=x' "$tmp/http.log"
fi
if grep -Eq 'GET /wrp\?url=y&z=1\.0&m=ismap&t=gip&c=216&k=&w=632&h=[0-9]+&sl=1 ' "$tmp/http.log"; then
	ok "wrp form -a: shows M, T and C, sends them as set"
else
	bad "wrp form -a: the query is wrong:"
	grep 'GET /wrp?url=y' "$tmp/http.log"
fi
if grep -q 'select .* c sel 1: -Ncol=Ncol 256=256' "$tmp/fc.out" &&
	grep -q 'select .* c sel 4: ' "$tmp/fc100.out" &&
	! awk '$2 != "hidden" && $8 ~ /^"?[whmtWHMT]"?$/' "$tmp/fc.out" | grep -q .; then
	ok "wrp form, colors: C shown, set to 256 (or for 100 colors to 64)"
else
	bad "wrp form, colors: the C menu is wrong:"
	grep -E 'select|"C"' "$tmp/fc.out" "$tmp/fc100.out"
fi
# WRP's one-letter labels are pictures too: Z a magnifying glass, C a
# palette, K a keyboard (D: see 5a).
if grep -q 'text .* "Z" icon$' "$tmp/f.out" && grep -q 'text .* "K" icon$' "$tmp/f.out" &&
	grep -q 'text .* "C" icon$' "$tmp/fc.out"; then
	ok "wrp form: the labels Z, C and K are pictures"
else
	bad "wrp form: the labels Z, C and K are not pictures:"
	grep 'text' "$tmp/f.out" "$tmp/fc.out"
fi
if grep -Eq 'GET /wrp\?url=z&z=1\.0&c=256&k=&w=632&h=[0-9]+&sl=1&m=ismap&t=gif ' "$tmp/http.log" &&
	grep -Eq 'GET /wrp\?url=&Fn=St&z=1\.0&c=16&k=&w=632&h=[0-9]+&sl=1&m=ismap&t=gif ' "$tmp/http.log" &&
	grep -q 'GET /?m=ismap&t=gif&c=256&sl=1 ' "$tmp/http.log"; then
	ok "wrp form, colors: sends C's 256; 16 chosen sends St at once; WRP's start page asked with the C menu"
else
	bad "wrp form, colors: the queries are wrong:"
	grep -E 'GET /(wrp\?url=[z&]|\?)' "$tmp/http.log"
fi
if grep -Eq 'GET /wrp\?url=&z=1\.0&k=ab%0D&w=632&h=[0-9]+&sl=1&m=ismap&t=gif&c=2 ' "$tmp/http.log" &&
	grep -Eq 'GET /wrp\?url=&z=1\.0&k=cd%08&w=632&h=[0-9]+&sl=1&m=ismap&t=gif&c=2 ' "$tmp/http.log" &&
	grep -Eq 'GET /wrp\?url=&z=1\.0&k=&Fn=Rt&w=632&h=[0-9]+&sl=1&m=ismap&t=gif&c=2 ' "$tmp/http.log"; then
	ok "wrp form, keys: K's text then Return (Rt) or Backspace (Bs) as keys; Rt alone with K empty"
else
	bad "wrp form, keys: the queries are wrong:"
	grep -E 'GET /wrp\?url=&z=1\.0&k=' "$tmp/http.log"
fi
if grep -Eq 'GET /wrp\?url=qq&z=1\.0&k=&w=632&h=[0-9]+&sl=1&m=ismap&t=gif&c=2 ' "$tmp/http.log"; then
	ok "wrp form: the url field's clear button empties it"
else
	bad "wrp form: the clear button did not empty the url field:"
	grep -E 'GET /wrp\?url=[a-z]*qq' "$tmp/http.log"
fi

# 5a. WRP's D menu (what it dithers in a black and white picture): shown,
#     and a new choice sends the form at once with St. Not with -a.
cat >"$here/data/wrpd.html" <<'EOF'
<HTML><HEAD><TITLE>WRP</TITLE></HEAD><BODY>
<FORM ACTION="/wrpd" METHOD="GET">
<INPUT TYPE="TEXT" NAME="url" VALUE="" SIZE="20">
<INPUT TYPE="SUBMIT" VALUE="Go">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="St">
W <INPUT TYPE="TEXT" NAME="w" VALUE="1152" SIZE="4">
H <INPUT TYPE="TEXT" NAME="h" VALUE="600" SIZE="4">
D <SELECT NAME="d">
<OPTION DISABLED>Dith</OPTION>
<OPTION VALUE="all" SELECTED>All</OPTION>
<OPTION VALUE="pics">Pics</OPTION>
<OPTION VALUE="none">None</OPTION>
</SELECT>
</FORM>
</BODY></HTML>
EOF
"$bin" -t "http://127.0.0.1:$port/wrpd.html" >"$tmp/d.out" 2>&1
dx=$(awk '$2 == "select" && $8 == "d" { print $3 + 5; exit }' "$tmp/d.out")
dy=$(awk '$2 == "select" && $8 == "d" { print $4 + 5; exit }' "$tmp/d.out")
py=$(awk '$2 == "select" && $8 == "d" { print $4 + $6 + 1 + 2 * 15 + 7; exit }' "$tmp/d.out")
"$bin" -t -s "@$dx,$dy;@$dx,$py;" "http://127.0.0.1:$port/wrpd.html" >/dev/null 2>&1
"$bin" -t -a -s "@$dx,$dy;@$dx,$py;" "http://127.0.0.1:$port/wrpd.html" >/dev/null 2>&1
rm -f "$here/data/wrpd.html"
if grep -q 'select .* d sel 1: ' "$tmp/d.out" && grep -q 'text .* "D" icon$' "$tmp/d.out" &&
	[ "$(grep -c 'GET /wrpd?' "$tmp/http.log")" = 1 ] &&
	grep -Eq 'GET /wrpd\?url=&Fn=St&d=pics&w=632&h=[0-9]+&sl=1&m=ismap&t=gif&c=2 ' "$tmp/http.log"; then
	ok "wrp form, dither: D shown, its label a picture; a new choice sends St at once (not with -a)"
else
	bad "wrp form, dither: the D menu is wrong:"
	grep 'select' "$tmp/d.out"
	grep 'GET /wrpd' "$tmp/http.log"
fi

# 5c. WRP's one menu (WRP 4.12.0 or newer): C's lines are a number and a
#     letter, d dither, n no dither, p only photos (2 colors), and there
#     is no D menu. With colors: C is set to the first line of
#     the nearest number (256d; 216n; for 100 colors 64d). A new line sends
#     St at once, and the next page keeps it. Black and white: C shows only
#     the lines of 2, set to 2p, and its label is the dither pattern (9
#     pixels wide). A new line sends St at once.
cat >"$here/data/wrpc.html" <<'EOF'
<HTML><HEAD><TITLE>WRP</TITLE></HEAD><BODY>
<FORM ACTION="/wrpc.html" METHOD="GET">
<INPUT TYPE="TEXT" NAME="url" VALUE="" SIZE="20">
<INPUT TYPE="SUBMIT" VALUE="Go">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="St">
W <INPUT TYPE="TEXT" NAME="w" VALUE="1152" SIZE="4">
H <INPUT TYPE="TEXT" NAME="h" VALUE="600" SIZE="4">
C <SELECT NAME="c">
<OPTION DISABLED>Ncol</OPTION>
<OPTION VALUE="256d">256d</OPTION>
<OPTION VALUE="256n">256n</OPTION>
<OPTION VALUE="216n" SELECTED>216n</OPTION>
<OPTION VALUE="216d">216d</OPTION>
<OPTION VALUE="128d">128d</OPTION>
<OPTION VALUE="128n">128n</OPTION>
<OPTION VALUE="64d">64d</OPTION>
<OPTION VALUE="64n">64n</OPTION>
<OPTION VALUE="16d">16d</OPTION>
<OPTION VALUE="16n">16n</OPTION>
<OPTION VALUE="2p">2p</OPTION>
<OPTION VALUE="2d">2d</OPTION>
<OPTION VALUE="2n">2n</OPTION>
</SELECT>
</FORM>
</BODY></HTML>
EOF
"$bin" -t "http://127.0.0.1:$port/wrpc.html" >"$tmp/c2.out" 2>&1
"$bin" -t -c 256 "http://127.0.0.1:$port/wrpc.html" >"$tmp/c256.out" 2>&1
"$bin" -t -c 216 "http://127.0.0.1:$port/wrpc.html" >"$tmp/c216.out" 2>&1
"$bin" -t -c 100 "http://127.0.0.1:$port/wrpc.html" >"$tmp/c100.out" 2>&1
ux=$(awk '$2 == "field" && $8 == "url" { print $3 + 5; exit }' "$tmp/c256.out")
uy=$(awk '$2 == "field" && $8 == "url" { print $4 + 5; exit }' "$tmp/c256.out")
cx=$(awk '$2 == "select" && $8 == "c" { print $3 + 5; exit }' "$tmp/c256.out")
cy=$(awk '$2 == "select" && $8 == "c" { print $4 + 5; exit }' "$tmp/c256.out")
oy=$(awk '$2 == "select" && $8 == "c" { print $4 + $6 + 1 + 6 * 15 + 7; exit }' "$tmp/c256.out")
"$bin" -t -c 256 -s "@$cx,$cy;@$cx,$oy;@$ux,$uy;x~" "http://127.0.0.1:$port/wrpc.html" >/dev/null 2>&1
bx=$(awk '$2 == "select" && $8 == "c" { print $3 + 5; exit }' "$tmp/c2.out")
by=$(awk '$2 == "select" && $8 == "c" { print $4 + 5; exit }' "$tmp/c2.out")
py=$(awk '$2 == "select" && $8 == "c" { print $4 + $6 + 1 + 2 * 15 + 7; exit }' "$tmp/c2.out")
"$bin" -t -s "@$bx,$by;@$bx,$py;" "http://127.0.0.1:$port/wrpc.html" >/dev/null 2>&1
rm -f "$here/data/wrpc.html"
if grep -q 'select .* c sel 1: -Ncol=Ncol 256d=256d 256n=256n 216n=216n ' "$tmp/c256.out" &&
	grep -q 'select .* c sel 3: ' "$tmp/c216.out" && grep -q 'select .* c sel 7: ' "$tmp/c100.out" &&
	grep -Eq 'GET /wrpc\.html\?url=&Fn=St&c=128n&w=632&h=[0-9]+&sl=1&m=ismap&t=gif ' "$tmp/http.log" &&
	grep -Eq 'GET /wrpc\.html\?url=x&c=128n&w=632&h=[0-9]+&sl=1&m=ismap&t=gif ' "$tmp/http.log"; then
	ok "wrp one menu, colors: C set to 256d (216n; 64d for 100); 128n sends St at once, the next page keeps it"
else
	bad "wrp one menu, colors: the C menu is wrong:"
	grep 'select' "$tmp/c256.out" "$tmp/c216.out" "$tmp/c100.out"
	grep 'GET /wrpc' "$tmp/http.log"
fi
if grep -q 'select .* c sel 1: -Ncol=Ncol 2p=2p 2d=2d 2n=2n$' "$tmp/c2.out" &&
	[ "$(awk '$2 == "text" && $8 == "\"C\"" && $9 == "icon" { print $5; exit }' "$tmp/c2.out")" = 9 ] &&
	grep -Eq 'GET /wrpc\.html\?url=&Fn=St&c=2d&w=632&h=[0-9]+&sl=1&m=ismap&t=gif ' "$tmp/http.log"; then
	ok "wrp one menu, black and white: C has only 2p 2d 2n, set to 2p, the dither picture; 2d sends St at once"
else
	bad "wrp one menu, black and white: the C menu is wrong:"
	grep -E 'select|"C"' "$tmp/c2.out"
	grep 'GET /wrpc' "$tmp/http.log"
fi

# 5b. WRP's errors: from WRP's page, an answer with no form (like WRP's
# "Unable to find map" after a click on an old picture) keeps WRP's page,
# puts the error on the status line, and gets WRP's new picture at once
# (St). With no page at all (no server), Return tries again, and the
# terminal says what is wrong.
cat >"$here/data/wrp2.html" <<'EOF'
<HTML><HEAD><TITLE>WRP</TITLE></HEAD><BODY>
<FORM ACTION="/wrp2.html" METHOD="GET">
<INPUT TYPE="TEXT" NAME="url" VALUE="http://x.org/" SIZE="20">
<INPUT TYPE="SUBMIT" VALUE="Go">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="Bk">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="St">
W <INPUT TYPE="TEXT" NAME="w" VALUE="1152" SIZE="4">
</FORM>
<A HREF="/err.txt"><IMG SRC="/hn2.gif" ISMAP></A>
</BODY></HTML>
EOF
printf 'Unable to find map /map/old.map\n' >"$here/data/err.txt"
"$bin" -t "http://127.0.0.1:$port/wrp2.html" >"$tmp/e0.out" 2>&1
ix=$(awk '$2 == "image" { print $3 + 5; exit }' "$tmp/e0.out")
iy=$(awk '$2 == "image" { print $4 + 7; exit }' "$tmp/e0.out")
"$bin" -t -v -s "@$ix,$iy;" "http://127.0.0.1:$port/wrp2.html" >"$tmp/e.out" 2>&1
"$bin" -t -v -s "~" "http://127.0.0.1:1/" >"$tmp/e1.out" 2>&1
rm -f "$here/data/wrp2.html" "$here/data/err.txt"
if grep -q '^status: WRP: Unable to find map /map/old.map$' "$tmp/e.out" &&
	[ "$(grep -c 'field .* url =' "$tmp/e.out")" = 3 ] &&
	grep -Eq 'GET /err\.txt\?5,7 ' "$tmp/http.log" &&
	grep -Eq 'GET /wrp2\.html\?url=http%3A%2F%2Fx\.org%2F&Fn=St&w=632&h=[0-9]+&sl=1&m=ismap&t=gif&c=2 ' "$tmp/http.log" &&
	grep -q "^status: The picture was old: here is WRP's new one. Click again.$" "$tmp/e.out"; then
	ok "wrp error: WRP's page stays, the error on the status line, then WRP's new picture (St)"
else
	bad "wrp error: wrong:"
	grep -E '^status|field .* url' "$tmp/e.out"
	grep -E 'GET /(err|wrp2)' "$tmp/http.log"
fi
if [ "$(grep -c '^status: Unable to connect to WRP at 127.0.0.1:1: connection refused. Return: try again$' "$tmp/e1.out")" = 2 ] &&
	[ "$(grep -c '^archaic: unable to connect to WRP at 127.0.0.1:1: connection refused$' "$tmp/e1.out")" = 2 ] &&
	grep -q '^archaic: ensure WRP is up and running. To use another address: archaic HOST:PORT$' "$tmp/e1.out"; then
	ok "no page: Return tries again; the terminal says WRP is not there"
else
	bad "no page: Return did not try again, or no message:"
	grep -e '^status' -e '^archaic' "$tmp/e1.out"
fi

# 6. WRP, when it runs: the start page, then a page with its picture,
#    as wide as the window (640 - 2 x 4).
if curl -s -m 3 -o /dev/null "http://$wrp/"; then
	"$bin" -t "$wrp" >"$tmp/w1.out" 2>&1
	if grep -q 'field.* url' "$tmp/w1.out" && grep -q 'select.* z sel' "$tmp/w1.out" &&
		! grep -q 'select.* t sel' "$tmp/w1.out"; then
		ok "wrp: start page has the url field and Z, but no T"
	else
		bad "wrp: start page"
	fi
	x=$(awk '$2 == "field" && $8 == "url" { print $3 + 5; exit }' "$tmp/w1.out")
	y=$(awk '$2 == "field" && $8 == "url" { print $4 + 5; exit }' "$tmp/w1.out")
	"$bin" -t -o "$tmp/w.pbm" -s "@$x,$y;http://example.com/~" "$wrp" >"$tmp/w2.out" 2>&1
	if grep -q 'image .* ismap got 632x[1-9]' "$tmp/w2.out"; then
		ok "wrp: example.com came as a picture ($(grep -o 'got [0-9x]*' "$tmp/w2.out" | tail -1))"
	else
		bad "wrp: no picture for example.com"
		tail -5 "$tmp/w2.out"
	fi
else
	echo "skip  wrp: nothing at $wrp"
fi

# 7. The display with no window (archaic-headless): commands on standard
#    input, PNG and PBM screenshots, typing into a field, a new size.
if [ -x "$hl" ]; then
	cp "$tmp/t.html" "$here/data/t.html"
	x=$(awk '$2 == "field" { print $3 + 2; exit }' "$tmp/t.out")
	y=$(awk '$2 == "field" { print $4 + 2; exit }' "$tmp/t.out")
	printf '%s\n' "shot $tmp/h1.png" "shot $tmp/h1.pbm" "click $x $y" \
		"key ctrl-u" "type hi" "key return" "size 500 300" \
		"shot $tmp/h2.png" quit |
		"$hl" -g 640x400 "http://127.0.0.1:$port/t.html" >"$tmp/h.out" 2>&1
	rm -f "$here/data/t.html"
	if grep -qx 'title T&T AB' "$tmp/h.out" &&
		grep -qx "shot $tmp/h2.png 500x300" "$tmp/h.out"; then
		ok "headless: title, shots, size 500 300"
	else
		bad "headless: wrong output:"
		cat "$tmp/h.out"
	fi
	if [ "$(magick identify -format '%wx%h' "$tmp/h1.png" 2>/dev/null)" = 640x400 ] &&
		magick compare -metric AE "$tmp/h1.png" "$tmp/h1.pbm" null: 2>/dev/null &&
		magick "$tmp/h1.pbm" -format '%[fx:mean < 0.999]' info: | grep -qx 1; then
		ok "headless: the PNG shot reads back, has the page, and equals the PBM shot"
	else
		bad "headless: the PNG shot is wrong or blank"
	fi
	if grep -q 'GET /go?q=hi&s=2&h=x' "$tmp/http.log"; then
		ok "headless: click, Ctrl-U, typing and Return send the form"
	else
		bad "headless: the typed form was not sent:"
		grep 'GET /go' "$tmp/http.log"
	fi
	# No WRP: the box in the middle of the empty window. Its title bar
	# is black; a window with no box is all white there.
	printf '%s\n' "shot $tmp/nb.png" quit |
		"$hl" -g 640x400 http://127.0.0.1:1/ >"$tmp/nb.out" 2>&1
	if magick "$tmp/nb.png" -crop 320x180+150+100 -format '%[fx:mean < 0.95]' info: 2>/dev/null | grep -qx 1 &&
		magick "$tmp/nb.png" -crop 600x90+0+0 -format '%[fx:mean]' info: 2>/dev/null | grep -qx 1 &&
		grep -q '^archaic: unable to connect to WRP at 127.0.0.1:1' "$tmp/nb.out"; then
		ok "headless: no WRP: a box in the middle says so"
	else
		bad "headless: no WRP: no box in the middle, or no message:"
		cat "$tmp/nb.out"
	fi
else
	echo "skip  headless: no $hl"
fi

# 7b. The headless display shows colors (like SDL and Cocoa): a color
#     GIF on the screen exactly, and the PNG shot in colors.
if [ -x "$hl" ]; then
	echo '<HTML><BODY><IMG SRC="/ex16.gif" WIDTH="632" HEIGHT="330"></BODY></HTML>' \
		>"$here/data/col.html"
	"$bin" -t "http://127.0.0.1:$port/col.html" >"$tmp/col.out" 2>&1
	ix=$(awk '$2 == "image" { print $3; exit }' "$tmp/col.out")
	iy=$(awk '$2 == "image" { print $4; exit }' "$tmp/col.out")
	printf '%s\n' "shot $tmp/c1.png" quit |
		"$hl" -g 640x400 "http://127.0.0.1:$port/col.html" >"$tmp/hc.out" 2>&1
	rm -f "$here/data/col.html"
	magick "$tmp/c1.png" -crop "600x330+$ix+$iy" +repage "$tmp/c1c.png"
	magick "$here/data/ex16.gif[0]" -crop 600x330+0+0 +repage "$tmp/c1r.png"
	if magick identify -format '%[channels]' "$tmp/c1.png" | grep -q '^srgb' &&
		magick compare -metric AE "$tmp/c1c.png" "$tmp/c1r.png" null: 2>/dev/null; then
		ok "headless: a color picture on the screen exactly; the shot in colors"
	else
		bad "headless: the color picture is wrong ($(magick identify -format '%[channels]' "$tmp/c1.png"))"
	fi
	# a display of 256 colors (an 8-bit colormap) asks WRP for 216
	echo quit | ARCHAIC_COLORS=256 "$hl" "http://127.0.0.1:$port/" >/dev/null 2>&1
	if grep -q 'GET /?m=ismap&t=gif&c=216&sl=1 ' "$tmp/http.log"; then
		ok "headless: a display of 256 colors asks WRP for 216"
	else
		bad "headless: a display of 256 colors did not ask for 216:"
		grep 'GET /?m=' "$tmp/http.log"
	fi
else
	echo "skip  headless colors: no $hl"
fi

# 7c. A new window width: once the size stays, WRP makes the picture
# again (St), as wide as the window. A new height alone sends nothing.
# Text typed in a field (K) stays.
cat >"$here/data/wrp3.html" <<'EOF'
<HTML><HEAD><TITLE>WRP http://x.org/</TITLE></HEAD><BODY>
<FORM ACTION="/wrp3.html" METHOD="GET">
<INPUT TYPE="TEXT" NAME="url" VALUE="http://x.org/" SIZE="20">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="St">
W <INPUT TYPE="TEXT" NAME="w" VALUE="1152" SIZE="4">
K <INPUT TYPE="TEXT" NAME="k" VALUE="" SIZE="4">
</FORM>
<A HREF="/x.map"><IMG SRC="/hn2.gif" WIDTH="615" HEIGHT="200" ISMAP></A>
</BODY></HTML>
EOF
"$bin" -t "http://127.0.0.1:$port/wrp3.html" >"$tmp/r0.out" 2>&1
kx=$(awk '$2 == "field" && $8 == "k" { print $3 + 5; exit }' "$tmp/r0.out")
ky=$(awk '$2 == "field" && $8 == "k" { print $4 + 5; exit }' "$tmp/r0.out")
printf '%s\n' "click $kx $ky" 'type ab' 'size 640 300' 'sleep 600' 'size 500 300' \
	'sleep 600' 'key return' 'sleep 300' 'quit' |
	"$hl" -g 640x400 "http://127.0.0.1:$port/wrp3.html" >"$tmp/r.out" 2>&1
rm -f "$here/data/wrp3.html"
q3='url=http%3A%2F%2Fx\.org%2F'
if [ "$(grep -c 'GET /wrp3\.html?.*Fn=St' "$tmp/http.log")" = 1 ] &&
	grep -Eq "GET /wrp3\.html\?$q3&Fn=St&k=ab&w=475&h=[0-9]+&sl=1&m=ismap&t=gif&c=256 " "$tmp/http.log" &&
	grep -Eq "GET /wrp3\.html\?$q3&k=ab&w=475&h=[0-9]+&sl=1&m=ismap&t=gif&c=256 " "$tmp/http.log"; then
	ok "new width: one St with w=475 (none for a new height), and K's text stays"
else
	bad "new width: wrong:"
	grep 'GET /wrp3' "$tmp/http.log"
fi
if grep -qx 'title Archaic - http://x.org/' "$tmp/r.out" && ! grep -q '^title WRP' "$tmp/r.out"; then
	ok "window title: Archaic in the place of WRP"
else
	bad "window title: wrong:"
	grep '^title' "$tmp/r.out"
fi

# 8. The tall picture: WRP's picture of a whole page (sl=1), in a frame
#    under the form, in slices. A random 600x1500 picture; each shot's
#    frame must be exactly the picture's rows at the scroll position.
if [ -x "$hl" ]; then
	magick -size 600x1500 xc: +noise Random -colorspace gray -threshold 50% \
		-type bilevel "$here/data/tall.gif"
	cat >"$here/data/tall.html" <<'EOF'
<HTML><HEAD><TITLE>Tall</TITLE></HEAD><BODY>
<FORM ACTION="/wrp" METHOD="GET">
<INPUT TYPE="TEXT" NAME="url" VALUE="tall" SIZE="20">
<INPUT TYPE="SUBMIT" VALUE="Go">
W <INPUT TYPE="TEXT" NAME="w" VALUE="632" SIZE="4">
</FORM>
<BR>
<A HREF="/map"><IMG SRC="/tall.gif" WIDTH="600" HEIGHT="1500" ISMAP></A>
</BODY></HTML>
EOF
	"$bin" -t "http://127.0.0.1:$port/tall.html" >"$tmp/tall.out" 2>&1
	fx=$(awk '$2 == "image" { print $3; exit }' "$tmp/tall.out")
	fy=$(awk '$2 == "image" { print $4; exit }' "$tmp/tall.out")
	fh=$(awk '$2 == "image" { print $6; exit }' "$tmp/tall.out")
	if grep -q "image .* ismap got 600x$fh gif sum [0-9a-f]* tall 600x1500 at 0" "$tmp/tall.out" &&
		grep -q "GET /tall.gif?y=0&h=$fh " "$tmp/http.log"; then
		ok "tall: a frame ${fh} high, its first slice"
	else
		bad "tall: no frame or no slice:"
		grep image "$tmp/tall.out"
	fi
	frame() {	# is shot $1's frame the picture's rows from $2?
		magick "$1" -crop "600x$fh+$fx+$fy" +repage "$tmp/f.png"
		magick "$here/data/tall.gif" -crop "600x$fh+0+$2" +repage "$tmp/r.png"
		magick compare -metric AE "$tmp/f.png" "$tmp/r.png" null: 2>/dev/null
	}
	q=$((fh / 4))
	p=$((fh - 26))
	last=$((1500 - fh))
	printf '%s\n' "shot $tmp/t0.png" 'wheel 1' "shot $tmp/t1.png" 'scroll down' \
		"shot $tmp/t2.png" 'pixels 5000' "shot $tmp/t3.png" \
		"click $((fx + 10)) $((fy + 20))" quit |
		"$hl" -g 640x400 "http://127.0.0.1:$port/tall.html" >"$tmp/th.out" 2>&1
	n=0
	for s in "t0 0" "t1 $q" "t2 $((q + p))" "t3 $last"; do
		set -- $s
		if frame "$tmp/$1.png" $2; then
			n=$((n + 1))
		else
			bad "tall: shot $1 is not rows $2 to $(($2 + fh - 1))"
		fi
	done
	[ $n = 4 ] && ok "tall: the frame shows the right rows at 0, a wheel step, a window, the end"
	if grep -q "GET /tall.gif?y=$fh&h=$fh " "$tmp/http.log" &&
		grep -q "GET /map?10,$((last + 20)) " "$tmp/http.log"; then
		ok "tall: the next slice comes before it is needed; a click sends page rows"
	else
		bad "tall: slices or the click are wrong:"
		grep -E 'GET /(tall|map)' "$tmp/http.log"
	fi

	# 9. Archaic's own scroll bar (SDL, X11, X10 have it): the arrows, a
	#    click in the shaft, the elevator dragged, an arrow held down.
	#    In each shot the frame must show the right rows, and column 638
	#    of the bar (no dots there) only the lines of the arrow boxes and
	#    of the elevator. The window is 640x400: the page part 384 high,
	#    the shaft rows 16 to 368, an arrow box 16 high.
	m=$last
	sh=352
	eh=$((sh * fh / 1500))
	room=$((sh - eh))
	sbrows() {	# the black rows of column 638 in shot $1
		magick "$1" -crop 1x400+638+0 +repage -depth 8 gray:- | od -An -tu1 -v |
			awk '{ for (i = 1; i <= NF; ++i) if ($i < 128)
				printf "%s%d", (n++ ? " " : ""), r + i - 1; r += NF }'
	}
	want() {	# what sbrows gives at place $1; $2: the down arrow held
		e=$((16 + room * $1 / m))
		w=16
		[ $e != 16 ] && w="$w $e"
		w="$w $((e + eh - 1))"
		[ $((e + eh - 1)) != 367 ] && w="$w 367"
		[ -n "$2" ] && w="$w$(awk 'BEGIN { for (i = 368; i < 384; ++i) printf " %d", i }')"
		echo "$w 384"
	}
	nmap=$(grep -c 'GET /map' "$tmp/http.log")
	g=$((100 - 16 - room * p / m))		# where the drag takes the elevator
	v4=$(((200 - g - 16) * m / room))
	bx=630
	printf '%s\n' "shot $tmp/s0.png" "click $bx 377" "shot $tmp/s1.png" \
		"click $bx 300" "shot $tmp/s2.png" "click $bx 5" "shot $tmp/s3.png" \
		"down $bx 100" "move $bx 200" "shot $tmp/s4.png" "move $bx 100" \
		"shot $tmp/s5.png" "move $bx 2000" "up $bx 2000" "shot $tmp/s6.png" \
		"click $bx 50" "shot $tmp/s7.png" 'pixels -5000' "down $bx 377" \
		"shot $tmp/s8.png" 'sleep 1000' "up $bx 377" "shot $tmp/s9.png" quit |
		"$hl" -g 640x400 "http://127.0.0.1:$port/tall.html" >"$tmp/sb.out" 2>&1
	n=0
	for s in "s0 0" "s1 $q" "s2 $((q + p))" "s3 $p" "s4 $v4" "s5 $p" "s6 $m" \
		"s7 $((m - p))" "s8 $q dn" "s9 $((8 * q))"; do
		set -- $s
		got=$(sbrows "$tmp/$1.png")
		if ! frame "$tmp/$1.png" $2; then
			bad "scroll bar: shot $1's frame is not rows $2 to $(($2 + fh - 1))"
		elif [ "$got" != "$(want $2 $3)" ]; then
			bad "scroll bar: shot $1 (place $2): bar rows $got, want $(want $2 $3)"
		else
			n=$((n + 1))
		fi
	done
	[ $n = 10 ] && ok "scroll bar: arrows, shaft, elevator dragged (and back, and past the end), an arrow held for 1 s"
	if [ "$(grep -c 'GET /map' "$tmp/http.log")" = "$nmap" ]; then
		ok "scroll bar: its clicks do not go to the page"
	else
		bad "scroll bar: a click on it went to the page"
	fi

	# 10. The status line says what is coming, as it comes: connecting,
	#     waiting for the answer (the seconds count), the page, each part
	#     of the tall picture in KB of its size, and at the end the time
	#     and the bytes. The server waits 2 s, as WRP while it makes a page.
	"$bin" -t -v "http://127.0.0.1:$port/tall.html?wait=2" >"$tmp/st.out" 2>&1
	sz=$(wc -c <"$here/data/tall.html" | tr -d ' ')
	parts=$(((1500 + fh - 1) / fh))
	n=0
	for w in "Page: connecting to 127.0.0.1 port $port" \
		"Page: waiting for 127.0.0.1" "Page: waiting for 127.0.0.1, 1 s" \
		"Page: $sz of $sz bytes" "Picture part 1 of $parts: 0 of [0-9]+ KB" \
		"[0-9]+ s, [0-9]+ KB  http://127.0.0.1:$port/tall.html[?]wait=2"; do
		if grep -q -E "^status: $w\$" "$tmp/st.out"; then
			n=$((n + 1))
		else
			bad "status line: no \"$w\""
		fi
	done
	# the last line of the part: all of it
	if grep "^status: Picture part 1 of $parts: " "$tmp/st.out" | tail -1 |
		awk '{ if ($7 == $9 && $7 > 0) ok = 1 } END { exit !ok }'; then
		n=$((n + 1))
	else
		bad "status line: the part does not end at its size:"
		grep "^status: Picture part" "$tmp/st.out" | tail -2
	fi
	[ $n = 7 ] && ok "status line: connecting, waiting 2 s (counted), the page in bytes, a part in KB to its end, time and size"
	rm -f "$here/data/tall.html" "$here/data/tall.gif"
else
	echo "skip  tall: no $hl"
fi

# 11. Updates (update.c), with the test server as WRP: a build with a
#     system name (SYSID "test", with the headless display) asks
#     archaic-update?p=test&v=VERSION. -u installs a newer version: the
#     file in the place of the program, the old one as archaic.old. A
#     broken or short file, the same version, WRP's note, an older WRP,
#     a folder it can't write: nothing changes, and no file is left. In
#     the window the update comes once WRP's page is here; not with -n
#     or -t.
src=$here/../src
ver=$(sed -n 's/^#define IDENT "@(#)Archaic \(.*\)"$/\1/p' "$src/archaic.h")
uw="-std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-non-prototype -Wno-strict-prototypes -Wno-deprecated-declarations -g -O1 -fsanitize=address,undefined"
usrc="$src/main.c $src/html.c $src/http.c $src/gif.c $src/pic.c $src/update.c $src/fb.c $src/headless.c $src/nsock.c"
mkdir "$tmp/u"
if ${CC:-cc} $uw -DSYSID='"@(#)Archaic for test"' -o "$tmp/u/old" $usrc >"$tmp/u/cc.out" 2>&1 &&
	${CC:-cc} $uw -DSYSID='"@(#)Archaic for test"' -DIDENT='"@(#)Archaic 99.0"' \
		-o "$tmp/u/new" $usrc >>"$tmp/u/cc.out" 2>&1 && [ ! -s "$tmp/u/cc.out" ]; then
	cp "$tmp/u/new" "$here/data/upd.bin"
	sz=$(wc -c <"$tmp/u/new" | tr -d ' ')
	crc=$(python3 -I -c 'import sys, zlib; print("%08x" % zlib.crc32(open(sys.argv[1], "rb").read()))' "$tmp/u/new")
	uans() {	# WRP's answer: $1 version, $2 size, $3 CRC
		printf 'archaic-update 1\nversion %s\nfile archaic-test\nsize %s\ncrc %s\nget archaic-update?p=test&f=%s\n' \
			"$1" "$2" "$3" "$1" >"$here/data/upd.txt"
	}
	urun() {	# archaic -u in a new folder, $tmp/u/bin; $1: what it must say
		rm -rf "$tmp/u/bin"
		mkdir "$tmp/u/bin"
		cp "$tmp/u/old" "$tmp/u/bin/archaic"
		"$tmp/u/bin/archaic" -u "127.0.0.1:$port" >"$tmp/u/out" 2>&1
		echo $? >"$tmp/u/rc"
		grep -q "^archaic: $1\$" "$tmp/u/out"
	}
	same() {	# exit status $1, and the program as it was, alone
		[ "$(cat "$tmp/u/rc")" = "$1" ] && cmp -s "$tmp/u/bin/archaic" "$tmp/u/old" &&
			[ "$(ls -A "$tmp/u/bin")" = archaic ]
	}
	uans 99.0 "$sz" "$crc"
	if urun "Archaic 99.0 is installed. Start archaic again to use it: $tmp/u/bin/archaic" &&
		[ "$(cat "$tmp/u/rc")" = 0 ] && cmp -s "$tmp/u/bin/archaic" "$tmp/u/new" &&
		cmp -s "$tmp/u/bin/archaic.old" "$tmp/u/old" && [ -x "$tmp/u/bin/archaic" ] &&
		[ "$(ls -A "$tmp/u/bin" | tr '\n' ' ')" = "archaic archaic.old " ] &&
		grep -q "GET /archaic-update?p=test&v=$ver HTTP" "$tmp/http.log" &&
		grep -q "GET /archaic-update?p=test&f=99.0 HTTP" "$tmp/http.log" &&
		"$tmp/u/bin/archaic" -u "127.0.0.1:$port" | grep -q '^archaic: Archaic 99.0 is the newest$'; then
		ok "update -u: asks p=test&v=$ver, installs 99.0, keeps archaic.old; the new one runs"
	else
		bad "update -u: not installed:"
		cat "$tmp/u/out"
		ls -l "$tmp/u/bin"
	fi
	# another Archaic has installed it: the old program, started as the
	# new file, gets nothing
	rm -rf "$tmp/u/bin"
	mkdir "$tmp/u/bin"
	cp "$tmp/u/new" "$tmp/u/bin/archaic"
	n0=$(grep -c 'GET /archaic-update?p=test&f=' "$tmp/http.log")
	python3 -I -c 'import os, sys; os.execv(sys.argv[1], sys.argv[2:])' \
		"$tmp/u/old" "$tmp/u/bin/archaic" -u "127.0.0.1:$port" >"$tmp/u/out" 2>&1
	rc=$?
	n1=$(grep -c 'GET /archaic-update?p=test&f=' "$tmp/http.log")
	if [ $rc = 0 ] && [ "$n0" = "$n1" ] &&
		grep -q "^archaic: Archaic 99.0 is installed. Start archaic again to use it: $tmp/u/bin/archaic\$" "$tmp/u/out" &&
		[ "$(ls -A "$tmp/u/bin")" = archaic ] && cmp -s "$tmp/u/bin/archaic" "$tmp/u/new"; then
		ok "update -u: installed already, by another Archaic: no download"
	else
		bad "update -u: installed already ($n0 $n1 downloads):"
		cat "$tmp/u/out"
	fi
	# started by its name alone: the search path finds the file, as on
	# Unix (dir:dir) and as on Idris (|dir/|dir/). An empty entry is the
	# current folder: here an empty one, not the folder of a build
	mkdir "$tmp/u/cwd"
	for pth in "/nonexistent:$tmp/u/bin" "|/nonexistent/|$tmp/u/bin/"; do
		rm -rf "$tmp/u/bin"
		mkdir "$tmp/u/bin"
		cp "$tmp/u/old" "$tmp/u/bin/archaic"
		(cd "$tmp/u/cwd" &&
			python3 -I -c 'import os, sys; os.execve(sys.argv[1], ["archaic", "-u", sys.argv[3]], {"PATH": sys.argv[2]})' \
			"$tmp/u/old" "$pth" "127.0.0.1:$port") >"$tmp/u/out" 2>&1
		if grep -q "^archaic: Archaic 99.0 is installed. Start archaic again to use it: $tmp/u/bin/archaic\$" "$tmp/u/out" &&
			cmp -s "$tmp/u/bin/archaic" "$tmp/u/new"; then
			ok "update -u: started by its name: PATH $pth finds it"
		else
			bad "update -u: started by its name, PATH $pth:"
			cat "$tmp/u/out"
		fi
	done
	uans "$ver" "$sz" "$crc"
	if urun "Archaic $ver is the newest" && same 0; then
		ok "update -u: the same version: nothing changes"
	else
		bad "update -u: the same version:"
		cat "$tmp/u/out"
	fi
	uans 99.0 "$sz" 0badc0de
	if urun "Archaic 99.0 came broken (its size or CRC): it is not installed" && same 1; then
		ok "update -u: a wrong CRC: not installed, no file left"
	else
		bad "update -u: a wrong CRC:"
		cat "$tmp/u/out"
		ls -l "$tmp/u/bin"
	fi
	uans 99.0 $((sz + 10)) "$crc"
	if urun "Archaic 99.0: WRP sends HTTP 200, $sz bytes" && same 1; then
		ok "update -u: a short file: not installed"
	else
		bad "update -u: a short file:"
		cat "$tmp/u/out"
	fi
	printf 'archaic-update 1\nnote no archaic-test in release v99.0\n' >"$here/data/upd.txt"
	if urun "WRP has no update: no archaic-test in release v99.0" && same 1; then
		ok "update -u: WRP's note"
	else
		bad "update -u: WRP's note:"
		cat "$tmp/u/out"
	fi
	rm -f "$here/data/upd.txt"
	if urun "This WRP has no updates for Archaic: it is an older WRP" && same 1; then
		ok "update -u: an older WRP"
	else
		bad "update -u: an older WRP:"
		cat "$tmp/u/out"
	fi
	uans 99.0 "$sz" "$crc"
	if [ "$(id -u)" != 0 ]; then
		rm -rf "$tmp/u/bin"
		mkdir "$tmp/u/bin"
		cp "$tmp/u/old" "$tmp/u/bin/archaic"
		chmod 555 "$tmp/u/bin"
		"$tmp/u/bin/archaic" -u "127.0.0.1:$port" >"$tmp/u/out" 2>&1
		echo $? >"$tmp/u/rc"
		chmod 755 "$tmp/u/bin"
		if grep -q '^archaic: Archaic 99.0 is out. To install it, start archaic as root$' "$tmp/u/out" &&
			same 1; then
			ok "update -u: a folder it can't write: says to start it as root"
		else
			bad "update -u: a folder it can't write:"
			cat "$tmp/u/out"
		fi
	fi
	# in the window: once WRP's page is here (a form with url and w)
	cat >"$here/data/updwrp.html" <<'EOF'
<HTML><HEAD><TITLE>WRP</TITLE></HEAD><BODY>
<FORM ACTION="/updwrp.html" METHOD="GET">
<INPUT TYPE="TEXT" NAME="url" VALUE="" SIZE="20">
<INPUT TYPE="SUBMIT" VALUE="Go">
W <INPUT TYPE="TEXT" NAME="w" VALUE="1152" SIZE="4">
</FORM></BODY></HTML>
EOF
	n0=$(grep -c 'GET /archaic-update' "$tmp/http.log")
	cp "$tmp/u/old" "$tmp/u/bin/archaic"
	printf 'sleep 100\n' | "$tmp/u/bin/archaic" -n -g 640x400 "http://127.0.0.1:$port/updwrp.html" >/dev/null 2>&1
	"$tmp/u/bin/archaic" -t "http://127.0.0.1:$port/updwrp.html" >/dev/null 2>&1
	n1=$(grep -c 'GET /archaic-update' "$tmp/http.log")
	printf 'sleep 100\nshot %s\n' "$tmp/u/win.png" |
		"$tmp/u/bin/archaic" -g 640x400 "http://127.0.0.1:$port/updwrp.html" >/dev/null 2>"$tmp/u/err"
	if [ "$n0" = "$n1" ] &&
		grep -q "^archaic: Archaic 99.0 is installed. Start archaic again to use it: $tmp/u/bin/archaic\$" "$tmp/u/err" &&
		cmp -s "$tmp/u/bin/archaic" "$tmp/u/new" && cmp -s "$tmp/u/bin/archaic.old" "$tmp/u/old"; then
		ok "update in the window: installed once WRP's page is here; none with -n or -t"
	else
		bad "update in the window: $n0 $n1 questions before:"
		cat "$tmp/u/err"
	fi
	rm -f "$here/data/upd.txt" "$here/data/upd.bin" "$here/data/updwrp.html"
else
	bad "update: the test builds failed:"
	cat "$tmp/u/cc.out"
fi

[ $fail = 0 ] && echo "all tests passed"
exit $fail
