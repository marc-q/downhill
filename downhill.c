/* Copyright 2017 Marc Volker Dickmann */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "src/html.h"
#include "src/tag.h"
#include "downhill.h"

//
// Parser (struct)
//

static void
parser_init (struct _parser *self, const char *buf, const size_t buf_len)
{
	self->list_depth = 0;
	
	// Flags: Misc
	self->emphasis = false;
	self->code = false;
	self->table = false;
	self->thead = false;
	
	// Flags: Line
	self->line_last = false;
	self->line_skip = false;
	
	// Tags
	self->pop_count = 0;
	taglist_init (&self->taglist);
	
	// Buffer
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
parser_insert_actag (struct _parser *self, const enum _tags tag)
{
	tag_add (&self->taglist, tag);
	self->pop_count++;
}

static void
parser_insert_tag (struct _parser *self, const enum _tags tag, const bool close)
{
	tag_push (&self->taglist, tag, close);
	self->pop_count++;
}

//
// Tags
//

static void
tag_header_cb_newline (struct _parser *parser)
{
	if (*parser->cursor == '#')
	{
		parser_insert_actag (parser, strspn (parser->cursor, "#"));
		parser_set_cursor (parser, parser->buf_pos + parser->taglist.tag[0]);
	}
	else if (!parser->line_last)
	{
		// MD-Header: tier one
		if (parser->line_end[1] == '=')
		{
			parser_insert_actag (parser, TAG_H1);
			parser->line_skip = true;
		}
		// MD-Header: tier two
		else if (parser->line_end[1] == '-')
		{
			parser_insert_actag (parser, TAG_H2);
			parser->line_skip = true;
		}
	}
}

static void
tag_blockquote_cb_newline (struct _parser *parser)
{
	if (*parser->cursor == '>')
	{
		parser_insert_actag (parser, TAG_BLOCKQUOTE);
		parser_set_cursor (parser, parser->buf_pos + 1);
	}
}

static void
tag_linebreak_cb_newline (struct _parser *parser)
{
	if (parser->buf_pos > 3 &&
	    parser->buf[parser->buf_pos - 2] == ' ' &&
	    parser->buf[parser->buf_pos - 3] == ' ')
	{
		parser_insert_tag (parser, TAG_BR, false);
	}
} 


static void
tag_ul_cb_newline (struct _parser *parser)
{
	// UL
	if (parser->cursor[0] == '*' &&
	    parser->cursor[1] == ' ')
	{
		if (parser->list_depth == 0)
		{
			tag_push (&parser->taglist, TAG_UL, true);
		}
		
		parser_set_cursor (parser, parser->buf_pos + 2);
		parser_insert_actag (parser, TAG_LI);
		
		if (parser->list_depth == 0)
		{
			parser_insert_tag (parser, TAG_UL, false);
			parser->list_depth++;
		}
		else if (parser->list_depth == 2)
		{
			parser_insert_tag (parser, TAG_UL, true);
			parser->list_depth--;
		}
	}
	// Sub UL
	else if (parser->cursor[0] == '\t' &&
		 parser->cursor[1] == '\t' &&
		 parser->cursor[2] == '*' &&
		 parser->cursor[3] == ' ')
	{
		parser_set_cursor (parser, parser->buf_pos + 4);
		parser_insert_actag (parser, TAG_LI);
		
		if (parser->list_depth == 1)
		{
			parser_insert_tag (parser, TAG_UL, false);
			parser->list_depth++;
		}
	}
	else
	{
		if (parser->list_depth == 2)
		{
			parser_insert_tag (parser, TAG_UL, true);
		}
		parser->list_depth = 0;
	}
}

static void
tag_table_cb_newline (struct _parser *parser, FILE *f_out)
{
	// Table
	if (parser->cursor[0] == '|')
	{
		// Header
		if (!parser->thead &&
		    !parser->line_last &&
		    parser->line_end[1] == '|' &&
		    parser->line_end[2] == ' ' &&
		    parser->line_end[3] == '-')
		{
			tag_push (&parser->taglist, TAG_TABLE, true);
			tag_push (&parser->taglist, TAG_THEAD, true);
			tag_push (&parser->taglist, TAG_TR, true);
			
			parser_insert_actag (parser, TAG_TH);
			parser_insert_tag (parser, TAG_TR, false);
			parser_insert_tag (parser, TAG_THEAD, false);
			parser_insert_tag (parser, TAG_TABLE, false);
			
			parser->line_skip = true;
			parser->thead = true;
			parser->table = true;
		}
		else
		{
			tag_pop (&parser->taglist, f_out);
			if (parser->thead)
			{
				tag_push (&parser->taglist, TAG_TBODY, true);
			}
			
			tag_push (&parser->taglist, TAG_TR, true);
			parser_insert_actag (parser, TAG_TD);
			parser_insert_tag (parser, TAG_TR, false);
			
			if (parser->thead)
			{
				parser_insert_tag (parser, TAG_TBODY, false);
				parser->thead = false;
			}
		}
		parser_set_cursor (parser, parser->buf_pos + 2);
	}
	else if (parser->table)
	{
		// Pop Table tag
		tag_pop (&parser->taglist, f_out);
		parser->table = false;
	}
}

static void
tag_table_cb_char (struct _parser *parser)
{
	if (!parser->code && *parser->cursor == '|')
	{
		if (parser->thead)
		{
			parser_insert_tag (parser, TAG_TH, false);
			parser_insert_tag (parser, TAG_TH, true);
			parser_set_cursor (parser, parser->buf_pos + 2);
		}
		else
		{
			parser_insert_tag (parser, TAG_TD, false);
			parser_insert_tag (parser, TAG_TD, true);
			parser_set_cursor (parser, parser->buf_pos + 2);
		}
	}
}

static void
tag_emphasis_cb_char (struct _parser *parser, FILE *f_out)
{
	if (*parser->cursor == '`')
	{
		html_print_tag ("pre", parser->code, f_out);
		parser_set_cursor (parser, parser->buf_pos + 1);
		parser->code = !parser->code;
	}
	else if (!parser->code)
	{
		const size_t i = strspn (parser->cursor, "*_");
		switch (i)
		{
			case 1:
				html_print_tag ("i", parser->emphasis, f_out);
				break;
			case 2:
				html_print_tag ("b", parser->emphasis, f_out);
				break;
			case 3:
				fputs ((parser->emphasis ? "</i></b>" : "<b><i>"), f_out);
				break;
			default:
				return;
		}
		parser_set_cursor (parser, parser->buf_pos + i);
	}
	parser->emphasis = !parser->emphasis;
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
		parser.pop_count = 0;
		
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
			
			if ((size_t) (parser.line_end - parser.buf) >= parser.buf_len)
			{
				parser.line_last = true;
			}
			
			if (parser.line_skip)
			{
				parser.buf_pos += (size_t) (parser.line_end - &buf[parser.buf_pos]) - 1;
				parser.line_skip = false;
				continue;
			}
			
			// Newline callbacks
			tag_header_cb_newline (&parser);
			
			tag_blockquote_cb_newline (&parser);
			tag_linebreak_cb_newline (&parser);
			
			tag_ul_cb_newline (&parser);
			tag_table_cb_newline (&parser, f_doc);
		}
		
		// Char callbacks
		tag_table_cb_char (&parser);
		
		// Close the previously opened tag
		if (*parser.cursor == '\n')
		{
			tag_pop (&parser.taglist, f_doc);
		}
		else
		{
			for (;parser.pop_count > 0; parser.pop_count--)
			{
				tag_pop (&parser.taglist, f_doc);
			}
		}
		tag_emphasis_cb_char (&parser, f_doc);
		// Format
		fputc (*parser.cursor, f_doc);
	}
	tag_flush (&parser.taglist, f_doc);
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
