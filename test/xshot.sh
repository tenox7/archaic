#!/bin/sh
# Run Archaic on an X display with a script, take a screenshot when the
# page has loaded N times, then let it quit.
#   test/xshot.sh DISPLAY OUT.png LOADS 'script' [archaic args...]
# Archaic quits when the script ends; waits (%) at its end keep the window.
dpy=$1 out=$2 loads=$3 script=$4
shift 4
log=${out%.png}.log
DISPLAY=$dpy ./archaic -v -s "$script" "$@" 2>"$log" &
pid=$!
n=0
while [ "$n" -lt 120 ]; do
	sleep 0.5
	n=$((n + 1))
	[ "$(grep -c -E ' s, [0-9]+ (KB|bytes)  http' "$log")" -ge "$loads" ] && break
	kill -0 "$pid" 2>/dev/null || break
done
sleep 0.5
xwd -root -display "$dpy" -silent | xwdtopnm 2>/dev/null | pnmtopng > "$out"
wait "$pid"
tail -3 "$log"
