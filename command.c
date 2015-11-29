/*
 * command.c            
 *
 * AttoEmacs, Hugh Barney, November 2015
 * Derived from: Anthony's Editor January 93, (Public Domain 1991, 1993 by Anthony Howe)
 *
 */

#include <ctype.h>
#include <string.h>
#include "header.h"

void top()
{
	curbp->b_point = 0;
}

void bottom()
{
	curbp->b_epage = curbp->b_point = pos(curbp->b_ebuf);
}

void quit_ask()
{
	if (modified_buffers() > 0) {
		mvaddstr(MSGLINE, 0, str_modified_buffers);
		clrtoeol();
		if (!yesno(FALSE))
			return;
	}
	quit();
}

/* flag = default answer, FALSE=n, TRUE=y */
int yesno(int flag)
{
	int ch;

	addstr(flag ? str_yes : str_no);
	refresh();
	ch = getch();
	if (ch == '\r' || ch == '\n')
		return (flag);
	return (ch == str_yes[1]);

}

void quit()
{
	done = 1;
}

void redraw()
{
	clear();
	display();
}

void left()
{
	if (0 < curbp->b_point)
		--curbp->b_point;
}

void right()
{
	if (curbp->b_point < pos(curbp->b_ebuf))
		++curbp->b_point;
}

void up()
{
	curbp->b_point = lncolumn(upup(curbp->b_point), col);
}

void down()
{
	curbp->b_point = lncolumn(dndn(curbp->b_point), col);
}

void lnbegin()
{
	curbp->b_point = segstart(lnstart(curbp->b_point), curbp->b_point);
}

void lnend()
{
	curbp->b_point = dndn(curbp->b_point);
	left();
}

void wleft()
{
	char_t *p;
	while (!isspace(*(p = ptr(curbp->b_point))) && curbp->b_buf < p)
		--curbp->b_point;
	while (isspace(*(p = ptr(curbp->b_point))) && curbp->b_buf < p)
		--curbp->b_point;
}

void pgdown()
{
	curbp->b_page = curbp->b_point = upup(curbp->b_epage);
	while (FIRST_LINE < row--)
		down();
	curbp->b_epage = pos(curbp->b_ebuf);
}

void pgup()
{
	int i = LINES;
	while (FIRST_LINE < --i) {
		curbp->b_page = upup(curbp->b_page);
		up();
	}
}

void wright()
{
	char_t *p;
	while (!isspace(*(p = ptr(curbp->b_point))) && p < curbp->b_ebuf)
		++curbp->b_point;
	while (isspace(*(p = ptr(curbp->b_point))) && p < curbp->b_ebuf)
		++curbp->b_point;
}

void insert()
{
	assert(curbp->b_gap <= curbp->b_egap);
	if (curbp->b_gap == curbp->b_egap && !growgap(CHUNK))
		return;
	curbp->b_point = movegap(curbp->b_point);
	*curbp->b_gap++ = input == '\r' ? '\n' : input;
	curbp->b_modified = TRUE;
	curbp->b_point = pos(curbp->b_egap);
}

void backsp()
{
	curbp->b_point = movegap(curbp->b_point);
	undoset();
	if (curbp->b_buf < curbp->b_gap) {
		--curbp->b_gap;
		curbp->b_modified = TRUE;
	}
	curbp->b_point = pos(curbp->b_egap);
}

void delete()
{
	curbp->b_point = movegap(curbp->b_point);
	undoset();
	if (curbp->b_egap < curbp->b_ebuf) {
		curbp->b_point = pos(++curbp->b_egap);
		curbp->b_modified = TRUE;
	}
}

void insertfile()
{
	temp[0] = '\0';
	result = getinput(str_insert_file, (char*) temp, BUFSIZ);
	if (temp[0] != '\0' && result)
		(void) insert_file(temp, TRUE);
}

void readfile()
{
	buffer_t *bp;
	
	temp[0] = '\0';
	result = getinput(str_read, (char*) temp, BUFSIZ);
	if (result) {
		bp = find_buffer(temp, TRUE);
		curbp = bp;

		/* load the file if not already loaded */
		if (bp != NULL && bp->b_fname[0] == '\0') {
			if (!load_file(temp)) {
				msg(m_newfile, temp);
			}
			strcpy(curbp->b_fname, temp);
		}
	}
}

void savebuffer()
{
	if (curbp->b_fname[0] != '\0') {
		save(curbp->b_fname);
		return;
	} else {
		writefile();
	}
	refresh();
}

void writefile()
{
	strcpy(temp, curbp->b_fname);
	result = getinput(str_write, (char*)temp, BUFSIZ);
	if (temp[0] != '\0' && result)
		if (save(temp) == TRUE)
			strcpy(curbp->b_fname, temp);
}

void killbuffer()
{
	buffer_t *kill_bp = curbp;
	buffer_t *bp;
	int bcount = count_buffers();

	/* do nothing if only buffer left is the scratch buffer */
	if (bcount == 1 && 0 == strcmp(get_buffer_name(curbp), str_scratch))
		return;
	
	if (curbp->b_modified) {
		mvaddstr(MSGLINE, 0, str_notsaved);
		clrtoeol();
		if (!yesno(FALSE))
			return;
	}

	if (bcount == 1) {
		/* create a scratch buffer */
		bp = find_buffer(str_scratch, TRUE);
		strcpy(bp->b_bname, str_scratch);
	}

	next_buffer();
	assert(kill_bp != curbp);
	delete_buffer(kill_bp);
}

void iblock()
{
	block();
	msg(str_mark);
}

void block()
{
	curbp->b_mark = curbp->b_mark == NOMARK ? curbp->b_point : NOMARK;
}

void killtoeol()
{
	/* point = start of empty line or last char in file */
	if (*(ptr(curbp->b_point)) == 0xa || (curbp->b_point + 1 == ((curbp->b_ebuf - curbp->b_buf) - (curbp->b_egap - curbp->b_gap))) ) {
		delete();
	} else {
		curbp->b_mark = curbp->b_point;
		lnend();
		copy_cut(TRUE);
	}
}

void copy() {
	copy_cut(FALSE);
}

void cut() {
	copy_cut(TRUE);
}

void copy_cut(int cut)
{
	char_t *p;
	/* if no mark or point == marker, nothing doing */
	if (curbp->b_mark == NOMARK || curbp->b_point == curbp->b_mark)
		return;
	if (scrap != NULL) {
		free(scrap);
		scrap = NULL;
	}
	if (curbp->b_point < curbp->b_mark) {
		/* point above marker: move gap under point, region = marker - point */
		p = ptr(curbp->b_point);
		(void) movegap(curbp->b_point);
		nscrap = curbp->b_mark - curbp->b_point;
	} else {
		/* if point below marker: move gap under marker, region = point - marker */
		p = ptr(curbp->b_mark);
		(void) movegap(curbp->b_mark);
		nscrap = curbp->b_point - curbp->b_mark;
	}
	if ((scrap = (char_t*) malloc(nscrap)) == NULL) {
		msg(m_alloc);
	} else {
		undoset();
		(void) memcpy(scrap, p, nscrap * sizeof (char_t));
		if (cut) {
			curbp->b_egap += nscrap; /* if cut expand gap down */
			block();
			curbp->b_point = pos(curbp->b_egap); /* set point to after region */
			curbp->b_modified = TRUE;
		} else {
			block(); /* can maybe do without */
		}
	}
}

void paste()
{
	if (nscrap <= 0) {
		msg(m_scrap);
	} else if (nscrap < curbp->b_egap - curbp->b_gap || growgap(nscrap)) {
		curbp->b_point = movegap(curbp->b_point);
		undoset();
		memcpy(curbp->b_gap, scrap, nscrap * sizeof (char_t));
		curbp->b_gap += nscrap;
		curbp->b_point = pos(curbp->b_egap);
		curbp->b_modified = TRUE;
	}
}

void showpos()
{
	msg(str_pos, unctrl(*(ptr(curbp->b_point))), *(ptr(curbp->b_point)), curbp->b_point, ((curbp->b_ebuf - curbp->b_buf) - (curbp->b_egap - curbp->b_gap)) );
}

void version()
{
	msg(m_version);
}

void search()
{
	dosearch("Search: ", searchtext, BUFSIZ);
}
