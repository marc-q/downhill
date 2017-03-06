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
	self->close_tag[0] = CLOSE_TAG_NONE;
	self->close_tag[1] = CLOSE_TAG_NONE;
	self->close_tag[2] = CLOSE_TAG_NONE;
	
	self->buf_pos = 0;
	self->buf_len = buf_len;
	self->buf = buf;
	
	self->cursor = NULL;
	
	self->line_end = NULL;
}

static void
close_tag_push (struct _parser *self, const enum close_tags tag)
{
	self->close_tag[2] = self->close_tag[1];
	self->close_tag[1] = self->close_tag[0];
	self->close_tag[0] = tag;
}

static void
close_tag_pop (struct _parser *self, FILE *f_out)
{
	switch (self->close_tag[0])
	{
		case CLOSE_TAG_H1:
		case CLOSE_TAG_H2:
		case CLOSE_TAG_H3:
		case CLOSE_TAG_H4:
		case CLOSE_TAG_H5:
		case CLOSE_TAG_H6:
			html_print_header (self->close_tag[0], true, f_out);
			break;
		case CLOSE_TAG_BLOCKQUOTE:
			html_print_tag ("blockquote", true, f_out);
			break;
		case CLOSE_TAG_UL:
			self->list--;
			html_print_tag ("ul", true, f_out);
			break;
		case CLOSE_TAG_LI:
			html_print_tag ("li", true, f_out);
			fputc ('\n', f_out);
			break;
		default:
			fputc ('\n', f_out);
			return;
	}
	self->close_tag[0] = self->close_tag[1];
	self->close_tag[1] = self->close_tag[2];
	self->close_tag[2] = CLOSE_TAG_NONE;
}

//
// Tags
//

static bool
tag_header_cb_newline (struct _parser *parser, FILE *f_out)
{
	if (*parser->cursor == '#')
	{
		close_tag_push (parser, strspn (parser->cursor, "#"));
		html_print_header (parser->close_tag[0], false, f_out);
		parser->buf_pos += parser->close_tag[0];
		// End parsing
		return true;
	}
	else if ((size_t) (parser->line_end - parser->buf) < parser->buf_len)
	{
		// MD-Header: tier one
		if (parser->line_end[1] == '=')
		{
			close_tag_push (parser, CLOSE_TAG_H1);
			html_print_tag ("h1", false, f_out);
			parser->skip_line = true;
		}
		// MD-Header: tier two
		else if (parser->line_end[1] == '-')
		{
			close_tag_push (parser, CLOSE_TAG_H2);
			html_print_tag ("h2", false, f_out);
			parser->skip_line = true;
		}
	}
	// Do not end parsing
	return false;
}

static bool
tag_blockquote_cb_newline (struct _parser *parser, FILE *f_out)
{
	if (*parser->cursor == '>')
	{
		html_print_tag ("blockquote", false, f_out);
		close_tag_push (parser, CLOSE_TAG_BLOCKQUOTE);
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
		fputs ("<br />\n", f_out);
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
			close_tag_push (parser, CLOSE_TAG_UL);
			parser->list++;
		}
		else if (parser->list == 2)
		{
			close_tag_pop (parser, f_out);
		}
		
		parser->buf_pos++;
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
			close_tag_push (parser, CLOSE_TAG_UL);
			parser->list++;
		}
		parser->buf_pos += 3;
	}
	else
	{
		return false;
	}
	// LI
	html_print_tag ("li", false, f_out);
	close_tag_push (parser, CLOSE_TAG_LI);
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
			
			if (tag_header_cb_newline (&parser, f_doc) ||
			    tag_blockquote_cb_newline (&parser, f_doc) ||
			    tag_linebreak_cb_newline (&parser, f_doc) ||
			    tag_ul_cb_newline (&parser, f_doc))
			{
				continue;
			}
		}
		
		if (tag_emphasis_cb_char (&parser, f_doc))
		{
			continue;
		}
		
		// Close the previously opened tag
		if (*parser.cursor == '\n')
		{
			close_tag_pop (&parser, f_doc);
		}
		// Format
		else
		{
			fputc (*parser.cursor, f_doc);
		}
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
