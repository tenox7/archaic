#!/bin/sh
# Tests for Archaic on the Mac (or any Unix), without a window (-t).
#   sh test/run.sh [archaic binary] [WRP address]
# Needs: python3 (a small web server), ImageMagick (magick) to check the
# GIF decoder. The WRP part runs only when WRP answers at the address.
bin=${1:-./archaic-asan}
wrp=${2:-127.0.0.1:8089}
port=8079
tmp=$(mktemp -d)
fail=0
here=$(cd "$(dirname "$0")" && pwd)

python3 -I -m http.server "$port" --bind 127.0.0.1 --directory "$here/data" \
	>"$tmp/http.log" 2>&1 &
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

# 2. Gray and color GIFs (dithered): the size must be right.
for f in hn216 ex16; do
	"$bin" -t -o "$tmp/$f.pbm" "http://127.0.0.1:$port/$f.gif" >"$tmp/$f.out" 2>&1
	want=$(magick identify -format '%wx%h' "$here/data/$f.gif[0]")
	got=$(grep -o 'got [0-9]*x[0-9]*' "$tmp/$f.out" | head -1 | cut -c5-)
	if [ "$want" = "$got" ]; then
		ok "$f.gif decodes, $got"
	else
		bad "$f.gif: want $want, got '$got'"
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
#    M, T and C are shown and sent as set.
cat >"$tmp/wrp.html" <<'EOF'
<HTML><HEAD><TITLE>WRP</TITLE></HEAD><BODY>
<FORM ACTION="/wrp" METHOD="GET">
<INPUT TYPE="TEXT" NAME="url" VALUE="" SIZE="20">
<INPUT TYPE="SUBMIT" VALUE="Go">
<INPUT TYPE="SUBMIT" NAME="Fn" VALUE="Bk">
W <INPUT TYPE="TEXT" NAME="w" VALUE="1152" SIZE="4">
H <INPUT TYPE="TEXT" NAME="h" VALUE="600" SIZE="4">
Z <SELECT NAME="z"><OPTION DISABLED>Zoom</OPTION><OPTION VALUE="1.0" SELECTED>1.0 x</OPTION></SELECT>
M <SELECT NAME="m"><OPTION VALUE="ismap" SELECTED>ISMAP</OPTION><OPTION VALUE="html">HTML</OPTION></SELECT>
T <SELECT NAME="t"><OPTION VALUE="gip" SELECTED>GIP</OPTION><OPTION VALUE="gif">GIF</OPTION></SELECT>
K <INPUT TYPE="TEXT" NAME="k" VALUE="" SIZE="4">
</FORM>
</BODY></HTML>
EOF
cp "$tmp/wrp.html" "$here/data/wrp.html"
"$bin" -t "http://127.0.0.1:$port/wrp.html" >"$tmp/f.out" 2>&1
shown=$(awk '$2 != "hidden" && $8 ~ /^"?[whmtcWHMTC]"?$/' "$tmp/f.out")
if [ -z "$shown" ] && grep -q 'select.* z sel' "$tmp/f.out" &&
	grep -q 'field.* k =' "$tmp/f.out"; then
	ok "wrp form: W, H, M, T hidden; Z and K shown"
else
	bad "wrp form: shows the wrong things:"
	echo "$shown"
fi
x=$(awk '$2 == "field" && $8 == "url" { print $3 + 5; exit }' "$tmp/f.out")
y=$(awk '$2 == "field" && $8 == "url" { print $4 + 5; exit }' "$tmp/f.out")
"$bin" -t -s "@$x,$y;x~" "http://127.0.0.1:$port/wrp.html" >/dev/null 2>&1
"$bin" -t -a -s "@$x,$y;y~" "http://127.0.0.1:$port/wrp.html" >"$tmp/fa.out" 2>&1
rm -f "$here/data/wrp.html"
if grep -Eq 'GET /wrp\?url=x&z=1\.0&k=&w=632&h=[0-9]+&m=ismap&t=gif&c=2 ' "$tmp/http.log"; then
	ok "wrp form: sends $(grep -Eo 'w=632&h=[0-9]+&m=ismap&t=gif&c=2' "$tmp/http.log")"
else
	bad "wrp form: the query is wrong:"
	grep 'GET /wrp?url=x' "$tmp/http.log"
fi
if grep -Eq 'GET /wrp\?url=y&z=1\.0&m=ismap&t=gip&k=&w=632&h=[0-9]+ ' "$tmp/http.log"; then
	ok "wrp form -a: shows M and T, sends them as set"
else
	bad "wrp form -a: the query is wrong:"
	grep 'GET /wrp?url=y' "$tmp/http.log"
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

[ $fail = 0 ] && echo "all tests passed"
exit $fail
