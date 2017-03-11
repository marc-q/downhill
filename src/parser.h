#ifndef __DOWNHILL_PARSER_H__
#define __DOWNHILL_PARSER_H__

struct _parser
{
	bool line_skip;
	bool line_last;
	
	// Buffer
	size_t buf_pos;
	size_t buf_len;
	const char *buf;
	
	// Points to the current char
	const char *cursor;
	
	// Points to the previous location
	const char *prev;
	
	// Points to the end of the current line
	const char *line_end;
};

void parser_new (struct _parser*, const char*, const size_t);

void parser_cursor_seek (struct _parser*, const long);

bool parser_cb_newline (struct _parser*);

#endif /* __DOWNHILL_PARSER_H__ */
