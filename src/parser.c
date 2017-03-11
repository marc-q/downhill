/* Copyright 2017 Marc Volker Dickmann */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "parser.h"

void
parser_new (struct _parser *self, const char *buf, const size_t buf_len)
{
	// Flags
	self->line_skip = false;
	self->line_last = false;
	
	// Buffer
	self->buf_pos = 0;
	self->buf_len = buf_len;
	self->buf = buf;
	
	self->cursor = buf;
	self->prev = NULL;
	self->line_end = NULL;
}

void
parser_cursor_seek (struct _parser *self, const long pos)
{
	self->buf_pos += pos;
	self->prev = self->cursor;
	self->cursor = &self->buf[self->buf_pos];
}

static void
parser_lineend_find (struct _parser *self)
{
	self->line_end = strchr (self->cursor, '\n');
	
	if ((size_t) (self->line_end - self->buf) >= self->buf_len)
	{
		self->line_last = true;
	}
}

//
// CALLBACKS
//

bool
parser_cb_newline (struct _parser *self)
{
	parser_lineend_find (self);
	
	if (self->line_skip && !self->line_last)
	{
		parser_cursor_seek (self, (size_t) (self->line_end - self->cursor));
		parser_lineend_find (self);
		self->line_skip = false;
		return true;
	}
	return false;
}
