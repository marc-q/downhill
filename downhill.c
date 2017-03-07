/* Copyright 2017 Marc Volker Dickmann */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "src/html.h"
#include "downhill.h"

//
// Parser (struct)
//

static void
parser_init (struct _parser *self, const char *buf, const size_t buf_len)
{
	self->list = 0;
	self->emphasis = false;
	self->skip_line = false;
	
	for (size_t i = 0; i < TAG_MAX; i++)
	{
		self->tag[i] = TAG_NONE;
	}
	
	self->buf_pos = 0;
	self->buf_len = buf_len;
	self->buf = buf;
	
	self->cursor = NULL;
	
	self->line_end = NULL;
}

static void
parser_set_cursor (struct _parser *self, const size_t pos)
{
	self->buf_pos = pos;
	self->cursor = &self->buf[self->buf_pos];
}

static void
tag_push (struct _parser *self, const enum _tags tag, const bool close)
{
	for (size_t i = TAG_MAX - 1; i > 0; i--)
	{
		self->tag[i] = self->tag[i - 1];
	}
	self->tag[0] = (close << 31) | (tag & TAG_MASK_VALUE);
}

static void
tag_pop (struct _parser *self, FILE *f_out)
{
	const bool close = self->tag[0] & TAG_MASK_CLOSE;
	
	switch (self->tag[0] & TAG_MASK_VALUE)
	{
		case TAG_H1:
		case TAG_H2:
		case TAG_H3:
		case TAG_H4:
		case TAG_H5:
		case TAG_H6:
			html_print_header (self->tag[0] & TAG_MASK_VALUE, close, f_out);
			break;
		case TAG_BLOCKQUOTE:
			html_print_tag ("blockquote", close, f_out);
			break;
		case TAG_UL:
			self->list--;
			html_print_tag ("ul", close, f_out);
			break;
		case TAG_LI:
			html_print_tag ("li", close, f_out);
			break;
		default:
			return;
	}
	for (size_t i = 1; i < TAG_MAX; i++)
	{
		self->tag[i - 1] = self->tag[i];
	}
	self->tag[TAG_MAX - 1] = TAG_NONE;
}

static void
tag_add (struct _parser *self, const enum _tags tag)
{
	tag_push (self, tag, true);
	tag_push (self, tag, false);
}

//
// Tags
//

static bool
tag_header_cb_newline (struct _parser *parser)
{
	if (*parser->cursor == '#')
	{
		tag_add (parser, strspn (parser->cursor, "#"));
		parser_set_cursor (parser, parser->buf_pos + parser->tag[0]);
		return true;
	}
	else if ((size_t) (parser->line_end - parser->buf) < parser->buf_len)
	{
		// MD-Header: tier one
		if (parser->line_end[1] == '=')
		{
			tag_add (parser, TAG_H1);
			parser->skip_line = true;
			return true;
		}
		// MD-Header: tier two
		else if (parser->line_end[1] == '-')
		{
			tag_add (parser, TAG_H2);
			parser->skip_line = true;
			return true;
		}
	}
	return false;
}

static bool
tag_blockquote_cb_newline (struct _parser *parser)
{
	if (*parser->cursor == '>')
	{
		tag_add (parser, TAG_BLOCKQUOTE);
		parser_set_cursor (parser, parser->buf_pos + 1);
		return true;
	}
	return false;
}

static bool
tag_linebreak_cb_newline (struct _parser *parser, FILE *f_out)
{
	if (parser->buf_pos > 3 &&
	    parser->buf[parser->buf_pos - 2] == ' ' &&
	    parser->buf[parser->buf_pos - 3] == ' ')
	{
		fputs ("<br />", f_out);
	}
	return false;
} 

static bool
tag_ul_cb_newline (struct _parser *parser, FILE *f_out)
{
	// UL
	if (parser->cursor[0] == '*' &&
	    parser->cursor[1] == ' ')
	{
		if (parser->list == 0)
		{
			html_print_tag ("ul", false, f_out);
			tag_push (parser, TAG_UL, true);
			parser->list++;
		}
		else if (parser->list == 2)
		{
			tag_pop (parser, f_out);
		}
		
		parser_set_cursor (parser, parser->buf_pos + 2);
	}
	// Sub UL
	else if (parser->cursor[0] == '\t' &&
		 parser->cursor[1] == '\t' &&
		 parser->cursor[2] == '*' &&
		 parser->cursor[3] == ' ')
	{
		if (parser->list == 1)
		{
			html_print_tag ("ul", false, f_out);
			tag_push (parser, TAG_UL, true);
			parser->list++;
		}
		parser_set_cursor (parser, parser->buf_pos + 4);
	}
	else
	{
		return false;
	}
	// LI
	tag_add (parser, TAG_LI);
	return true;
}

static bool
tag_emphasis_cb_char (struct _parser *parser, FILE *f_out)
{
	if (*parser->cursor == '`')
	{
		html_print_tag ("pre", parser->emphasis, f_out);
	}
	else
	{
		switch (strspn (parser->cursor, "*_"))
		{
			case 1:
				html_print_tag ("i", parser->emphasis, f_out);
				break;
			case 2:
				html_print_tag ("b", parser->emphasis, f_out);
				parser->buf_pos++;
				break;
			case 3:
				fputs ((parser->emphasis ? "</i></b>" : "<b><i>"), f_out);
				parser->buf_pos += 2;
				break;
			default:
				return false;
		}
	}
	parser->emphasis = !parser->emphasis;
	return true;
}

//
// Parser
//

static void
parse_markdown (const char *buf, const size_t buf_len, FILE *f_doc)
{
	char prev = '\0';
	struct _parser parser;
	
	parser_init (&parser, buf, buf_len - 1);
	
	for (parser.buf_pos = 0; parser.buf_pos < parser.buf_len; parser.buf_pos++)
	{
		// Store previous char
		if (parser.cursor != NULL)
		{
			prev = *parser.cursor;
		}
		
		parser.cursor = &buf[parser.buf_pos];
		
		// Start of this line
		if (prev == '\n' || parser.buf_pos == 0)
		{
			parser.line_end = strchr (parser.cursor, '\n');
			
			if (parser.skip_line)
			{
				parser.buf_pos += (size_t) (parser.line_end - &buf[parser.buf_pos]);
				parser.skip_line = false;
				continue;
			}
			
			if (tag_header_cb_newline (&parser) ||
			    tag_blockquote_cb_newline (&parser) ||
			    tag_linebreak_cb_newline (&parser, f_doc) ||
			    tag_ul_cb_newline (&parser, f_doc))
			{
				tag_pop (&parser, f_doc);
			}
		}
		
		if (tag_emphasis_cb_char (&parser, f_doc))
		{
			continue;
		}
		
		// Close the previously opened tag
		if (*parser.cursor == '\n')
		{
			tag_pop (&parser, f_doc);
		}
		// Format
		fputc (*parser.cursor, f_doc);
	}
}

static void
parse_file (const char *filename_src, const char *filename_doc)
{
	FILE *f_src = fopen (filename_src, "r");
	if (f_src == NULL)
	{
		printf ("Error: Could'nt open the file: %s!\n", filename_src);
		return;
	}
	
	FILE *f_doc = fopen (filename_doc, "w");
	if (f_doc == NULL)
	{
		printf ("Error: Could'nt open the file: %s!\n", filename_doc);
		fclose (f_src);
		return;
	}
	
	// Read the Markdown file
	fseek (f_src, 0, SEEK_END);
	
	const size_t buf_len = ftell (f_src);
	char *buf = malloc (buf_len);
	
	fseek (f_src, 0, SEEK_SET);
	if (fread (buf, sizeof (char), buf_len, f_src) != buf_len)
	{
		printf ("Error: Could'nt read the file: %s!\n", filename_src);
		free (buf);
		fclose (f_src);
		fclose (f_doc);
		return;
	}
	fclose (f_src);
	
	// Generate the HTML file
	html_print_head (filename_doc, f_doc);
	
	parse_markdown (buf, buf_len, f_doc);
	
	html_print_footer (f_doc);
	
	free (buf);
	fclose (f_doc);
}

int
main (int argc, char *argv[])
{
	printf ("Downhill (C) 2017 Marc Volker Dickmann\n\n");
	
	if (argc == 3)
	{
		parse_file (argv[1], argv[2]);
	}
	else
	{
		printf ("Usage: %s <filename_in> <filename_out>!\n", argv[0]);
	}
	
	return 0;
}
