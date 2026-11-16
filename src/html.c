/* html.c - just enough HTML for WRP's pages: forms (text fields,
 * buttons, hidden fields, menus), links, images, words and breaks.
 * Everything else is skipped. The page becomes a flat list of items.
 */
#include "archaic.h"

Item items[MAXITEM];
int nitems;
Opt opts[MAXOPT];
int nopts;
Form forms[MAXFORM];
int nforms;
char title[MAXTITLE];

#define MAXATTR 16

static char pool[POOLSZ];
static int npool;
static char *src;
static int slen, spos;
static int curform;		/* the <FORM> we are in, -1 if none */
static char *curhref;		/* the <A HREF> we are in, 0 if none */
static int cursel;		/* the <SELECT> we are in, -1 if none */
static int inopt;		/* in an <OPTION>: words go to otext */
static int intitle;
static char skipto[12];		/* skip all up to this end tag (script) */
static int space;		/* white space since the last item */
static int full;		/* out of items or pool */
static char tag[16];
static char word[200];
static int nword;
static char otext[100];
static char aname[MAXATTR][12];
static char *aval[MAXATTR];
static int nattr;
static char abuf[2 * MAXURL];
static int nabuf;
static char empty[] = "";

/* a copy of s in the pool */
static char *keep(s)
char *s;
{
	char *p;
	int n;

	for (n = 0; s[n]; ++n)
		;
	if (npool + n + 1 > POOLSZ) {
		full = 1;
		return empty;
	}
	p = pool + npool;
	scopy(p, s, n + 1);
	npool += n + 1;
	return p;
}

/* MAXVAL bytes for a text field, holding s */
static char *keepval(s)
char *s;
{
	char *p;

	if (npool + MAXVAL > POOLSZ) {
		full = 1;
		return 0;
	}
	p = pool + npool;
	scopy(p, s, MAXVAL);
	npool += MAXVAL;
	return p;
}

static Item *additem(type)
int type;
{
	Item *it;

	if (full || nitems >= MAXITEM) {
		full = 1;
		return 0;
	}
	it = &items[nitems++];
	it->type = type;
	it->x = it->y = it->w = it->h = 0;
	it->sp = space;
	space = 0;
	it->form = curform;
	it->href = curhref;
	it->name = 0;
	it->text = empty;
	it->val = 0;
	it->size = 20;
	it->pass = 0;
	it->opt = it->nopt = 0;
	it->sel = -1;
	it->img = -1;
	it->iw = it->ih = 0;
	it->ismap = 0;
	return it;
}

/* the value of attribute n of the current tag, 0 if it has none */
static char *attr(n)
char *n;
{
	int i;

	for (i = 0; i < nattr; ++i)
		if (strieq(aname[i], n))
			return aval[i];
	return 0;
}

static int num(s)
char *s;
{
	int n = 0;

	if (!s)
		return 0;
	while (*s == ' ')
		++s;
	while (*s >= '0' && *s <= '9')
		n = n * 10 + *s++ - '0';
	return n;
}

static int isws(c)
int c;
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}

static int alnum(c)
int c;
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9');
}

/* &amp; &lt; &#65; ... at src[spos] (after the &): the character */
static int entity()
{
	static char nm[10];
	int i, c, n, p = spos;

	if (p < slen && src[p] == '#') {
		n = 0;
		++p;
		if (p < slen && (src[p] == 'x' || src[p] == 'X')) {
			for (++p; p < slen; ++p) {
				c = lc(src[p]);
				if (c >= '0' && c <= '9')
					n = n * 16 + c - '0';
				else if (c >= 'a' && c <= 'f')
					n = n * 16 + c - 'a' + 10;
				else
					break;
				if (n > 0x10ffff)
					n = 0x10ffff;
			}
		} else
			for (; p < slen && src[p] >= '0' && src[p] <= '9'; ++p)
				if (n < 0x10ffff)
					n = n * 10 + src[p] - '0';
		if (p < slen && src[p] == ';')
			++p;
		spos = p;
		return n == 0 ? '?' : n;
	}
	for (i = 0; p < slen && i < 8 && alnum(src[p] & 0377); ++p)
		nm[i++] = src[p];
	nm[i] = 0;
	if (p < slen && src[p] == ';')
		++p;
	c = 0;
	if (strieq(nm, "amp"))
		c = '&';
	else if (strieq(nm, "lt"))
		c = '<';
	else if (strieq(nm, "gt"))
		c = '>';
	else if (strieq(nm, "quot"))
		c = '"';
	else if (strieq(nm, "apos"))
		c = '\'';
	else if (strieq(nm, "nbsp"))
		c = ' ';
	else if (strieq(nm, "copy"))
		c = 0251;
	if (!c)
		return '&';	/* not one we know: keep the & */
	spos = p;
	return c;
}

/* a character above 127 for the screen: Latin-1, or a lookalike */
static int latin(u)
int u;
{
	if (u == 0xa0)
		return ' ';
	if (u < 256)
		return u;
	if (u == 0x2018 || u == 0x2019 || u == 0x201a || u == 0x2032)
		return '\'';
	if (u == 0x201c || u == 0x201d || u == 0x201e || u == 0x2033)
		return '"';
	if (u == 0x2010 || u == 0x2011 || u == 0x2012 || u == 0x2013 ||
		u == 0x2014 || u == 0x2212)
		return '-';
	if (u == 0x2022 || u == 0x00b7)
		return '*';
	if (u == 0x2026)
		return '.';
	return '?';
}

/* a UTF-8 sequence that starts with byte c (src[spos] is next) */
static int utf8(c)
int c;
{
	int n, u;

	if (c >= 0xf0) {
		n = 3;
		u = c & 7;
	} else if (c >= 0xe0) {
		n = 2;
		u = c & 15;
	} else if (c >= 0xc0) {
		n = 1;
		u = c & 31;
	} else
		return c;	/* not UTF-8: take it as Latin-1 */
	while (n > 0 && spos < slen && (src[spos] & 0300) == 0200) {
		u = (u << 6) | (src[spos++] & 077);
		--n;
	}
	return latin(u);
}

/* the word in word[] is complete */
static void endword()
{
	Item *it;
	int i, n;

	if (nword == 0)
		return;
	word[nword] = 0;
	if (skipto[0] || (cursel >= 0 && !inopt))
		;
	else if (intitle) {
		n = 0;
		while (title[n])
			++n;
		if (n > 0 && n < MAXTITLE - 1)
			title[n++] = ' ';
		for (i = 0; word[i] && n < MAXTITLE - 1; ++i)
			title[n++] = word[i];
		title[n] = 0;
	} else if (inopt) {
		n = 0;
		while (otext[n])
			++n;
		if (n > 0 && n < (int)sizeof otext - 1)
			otext[n++] = ' ';
		for (i = 0; word[i] && n < (int)sizeof otext - 1; ++i)
			otext[n++] = word[i];
		otext[n] = 0;
	} else if ((it = additem(I_TEXT)) != 0)
		it->text = keep(word);
	nword = 0;
}

/* text up to the next tag */
static void text()
{
	int c;

	while (spos < slen) {
		c = src[spos] & 0377;
		if (c == '<' && spos + 1 < slen) {
			c = src[spos + 1] & 0377;
			if (alnum(c) || c == '/' || c == '!' || c == '?')
				break;
			c = '<';
		}
		++spos;
		if (c == '&')
			c = entity();
		else if (c >= 0200)
			c = utf8(c);
		if (isws(c)) {
			endword();
			space = 1;
		} else if (nword < (int)sizeof word - 1)
			word[nword++] = c;
	}
	endword();
}

/* the end of the <OPTION> we are in */
static void endopt()
{
	Opt *o;

	if (!inopt)
		return;
	endword();
	inopt = 0;
	o = &opts[nopts - 1];
	o->text = keep(otext);
	if (!o->val)
		o->val = o->text;
}

/* the end of the <SELECT> we are in */
static void endsel()
{
	Item *it;
	int i;

	endopt();
	if (cursel < 0)
		return;
	it = &items[cursel];
	if (it->sel < 0)	/* nothing SELECTED: the first that can be */
		for (i = 0; i < it->nopt; ++i)
			if (!opts[it->opt + i].dis) {
				it->sel = i;
				break;
			}
	cursel = -1;
}

static void brk(type)
int type;
{
	endword();
	additem(type);
}

/* what to do with tag[] (end: </tag>) and its attributes */
static void dotag(end)
int end;
{
	Item *it;
	char *a, *t;

	if (skipto[0]) {
		if (end && strieq(tag, skipto))
			skipto[0] = 0;
		return;
	}
	t = tag;
	if (strieq(t, "title"))
		intitle = !end;
	else if (strieq(t, "script") || strieq(t, "style") ||
		strieq(t, "textarea")) {
		if (!end) {
			endword();
			scopy(skipto, t, sizeof skipto);
		}
	} else if (strieq(t, "a")) {
		endword();
		if (end)
			curhref = 0;
		else if ((a = attr("href")) != 0)
			curhref = keep(a);
	} else if (strieq(t, "form")) {
		endsel();
		if (end)
			curform = -1;
		else if (nforms < MAXFORM) {
			a = attr("action");
			forms[nforms].action = keep(a ? a : "");
			a = attr("method");
			forms[nforms].post = a && strieq(a, "post");
			curform = nforms++;
		}
		brk(I_BR);
	} else if (strieq(t, "input") && !end) {
		endword();
		a = attr("type");
		if (!a)
			a = "text";
		if (strieq(a, "submit") || strieq(a, "reset") ||
			strieq(a, "button")) {
			if ((it = additem(I_BUTTON)) == 0)
				return;
			if (!strieq(a, "submit"))
				it->form = -1;	/* does nothing here */
			if ((t = attr("name")) != 0)
				it->name = keep(t);
			t = attr("value");
			it->text = keep(t ? t : strieq(a, "submit") ? "Submit" : a);
		} else if (strieq(a, "hidden")) {
			if ((it = additem(I_HIDDEN)) == 0)
				return;
			if ((t = attr("name")) != 0)
				it->name = keep(t);
			t = attr("value");
			it->val = keep(t ? t : "");
		} else if (strieq(a, "checkbox") || strieq(a, "radio") ||
			strieq(a, "image") || strieq(a, "file"))
			;	/* not done */
		else {
			if ((it = additem(I_FIELD)) == 0)
				return;
			if ((t = attr("name")) != 0)
				it->name = keep(t);
			t = attr("value");
			it->val = keepval(t ? t : "");
			if (!it->val) {
				--nitems;
				return;
			}
			if (num(attr("size")) > 0)
				it->size = num(attr("size"));
			if (it->size > 60)
				it->size = 60;
			it->pass = strieq(a, "password");
		}
	} else if (strieq(t, "select")) {
		endsel();
		if (end)
			return;
		endword();
		if ((it = additem(I_SELECT)) == 0)
			return;
		if ((t = attr("name")) != 0)
			it->name = keep(t);
		it->opt = nopts;
		cursel = (int)(it - items);
	} else if (strieq(t, "option")) {
		endopt();
		if (end || cursel < 0 || nopts >= MAXOPT)
			return;
		it = &items[cursel];
		a = attr("value");
		opts[nopts].val = a ? keep(a) : 0;
		opts[nopts].text = empty;
		opts[nopts].dis = attr("disabled") != 0;
		if (attr("selected"))
			it->sel = it->nopt;
		++it->nopt;
		++nopts;
		inopt = 1;
		otext[0] = 0;
	} else if (strieq(t, "img") && !end) {
		endword();
		if ((it = additem(I_IMAGE)) == 0)
			return;
		a = attr("src");
		it->text = keep(a ? a : "");
		it->iw = num(attr("width"));
		it->ih = num(attr("height"));
		it->ismap = attr("ismap") != 0;
	} else if (strieq(t, "br") && !end)
		brk(I_BR);
	else if (strieq(t, "p") || strieq(t, "hr") || strieq(t, "pre") ||
		strieq(t, "blockquote") || (lc(t[0]) == 'h' && t[1] >= '1' &&
		t[1] <= '6' && !t[2]))
		brk(I_PARA);
	else if (strieq(t, "div") || strieq(t, "li") || strieq(t, "tr") ||
		strieq(t, "ul") || strieq(t, "ol") || strieq(t, "dl") ||
		strieq(t, "dt") || strieq(t, "dd") || strieq(t, "table") ||
		strieq(t, "center") || strieq(t, "address"))
		brk(I_BR);
}

/* a tag at src[spos] ('<') */
static void dotags()
{
	int c, i, end, q;

	++spos;
	if (spos + 2 < slen && src[spos] == '!' && src[spos + 1] == '-' &&
		src[spos + 2] == '-') {
		for (spos += 3; spos + 2 < slen; ++spos)
			if (src[spos] == '-' && src[spos + 1] == '-' &&
				src[spos + 2] == '>')
				break;
		spos += 3;
		return;
	}
	if (src[spos] == '!' || src[spos] == '?') {
		while (spos < slen && src[spos] != '>')
			++spos;
		++spos;
		return;
	}
	end = 0;
	if (src[spos] == '/') {
		end = 1;
		++spos;
	}
	for (i = 0; spos < slen && alnum(src[spos] & 0377); ++spos)
		if (i < (int)sizeof tag - 1)
			tag[i++] = lc(src[spos]);
	tag[i] = 0;
	nattr = 0;
	nabuf = 0;
	for (;;) {
		while (spos < slen && (isws(src[spos]) || src[spos] == '/'))
			++spos;
		if (spos >= slen)
			break;
		if (src[spos] == '>') {
			++spos;
			break;
		}
		/* NAME[=VALUE] */
		for (i = 0; spos < slen && !isws(src[spos]) && src[spos] != '=' &&
			src[spos] != '>'; ++spos)
			if (i < (int)sizeof aname[0] - 1 && nattr < MAXATTR)
				aname[nattr][i++] = lc(src[spos]);
		if (nattr < MAXATTR) {
			aname[nattr][i] = 0;
			aval[nattr] = abuf + nabuf;
		}
		while (spos < slen && isws(src[spos]))
			++spos;
		if (spos < slen && src[spos] == '=') {
			++spos;
			while (spos < slen && isws(src[spos]))
				++spos;
			q = 0;
			if (spos < slen && (src[spos] == '"' || src[spos] == '\''))
				q = src[spos++];
			while (spos < slen) {
				c = src[spos];
				if (q ? c == q : (isws(c) || c == '>'))
					break;
				++spos;
				if (c == '&')
					c = entity();
				if (nattr < MAXATTR && nabuf < (int)sizeof abuf - 2)
					abuf[nabuf++] = c;
			}
			if (q && spos < slen)
				++spos;
		}
		if (nattr < MAXATTR) {	/* nabuf stays below sizeof abuf */
			abuf[nabuf] = 0;
			if (nabuf < (int)sizeof abuf - 1)
				++nabuf;
			++nattr;
		}
	}
	if (tag[0])
		dotag(end);
}

/* the page in buf (len bytes) to items[]: 0 ok, -1 if it did not fit */
int html_parse(buf, len)
char *buf;
int len;
{
	src = buf;
	slen = len;
	spos = 0;
	nitems = nopts = nforms = 0;
	npool = 0;
	curform = -1;
	curhref = 0;
	cursel = -1;
	inopt = intitle = 0;
	skipto[0] = 0;
	space = 0;
	full = 0;
	nword = 0;
	title[0] = 0;
	while (spos < slen && !full) {
		if (src[spos] == '<' && spos + 1 < slen &&
			(alnum(src[spos + 1] & 0377) || src[spos + 1] == '/' ||
			src[spos + 1] == '!' || src[spos + 1] == '?'))
			dotags();
		else
			text();
	}
	endsel();
	return full ? -1 : 0;
}
