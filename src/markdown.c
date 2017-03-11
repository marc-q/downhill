/* Copyright 2017 Marc Volker Dickmann */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "markdown.h"

//
// UTILS
//

/**
 * strscmp()
 * @a - String a.
 * @b - Substring to compare.
 *
 * Check if @a starts with @b.
 *
 * Return: True if @a starts with @b, otherwise false.
 */
static bool
strscmp (const char *a, const char *b)
{
	return (strlen (a) >= strlen (b) &&
		strncmp (a, b, strlen (b)) == 0);
}

//
// MD_PARSER
//

/**
 * md_parser_new()
 * @self - MD_Parser to initilize.
 * @buf - Buffer to parse.
 * @buf_len - Length of @buf
 *
 * Initilize @self with the default values and
 * set its buffer by initilizing the base parser.
 *
 * Return: none
 */
void
md_parser_new (struct _md_parser *self, const char *buf, const size_t buf_len)
{
	self->header = 0;
	self->blockquote = false;
	self->list_ul = 0;
	
	self->table = false;
	self->thead = false;
	self->tbody = false;
	
	self->code = false;
	self->emphasis = false;
	
	// Init the base parser
	parser_new (&self->p, buf, buf_len);
}

//
// BLOCK - Header
//

static void
md_close_header (struct _md_parser *self, FILE *f_out)
{
	if (self->header > 0)
	{
		html_print_header (self->header, true, f_out);
		self->header = 0;
	}
}

static void
md_process_header (struct _md_parser *self, FILE *f_out)
{
	md_close_header (self, f_out);
	
	if (*self->p.cursor == '#')
	{
		self->header = strspn (self->p.cursor, "#");
		parser_cursor_seek (&self->p, self->header);
	}
	else if (!self->p.line_last)
	{
		// MD-Header: tier one
		if (self->p.line_end[1] == '=')
		{
			self->header = 1;
			self->p.line_skip = true;
		}
		// MD-Header: tier two
		else if (self->p.line_end[1] == '-')
		{
			self->header = 2;
			self->p.line_skip = true;
		}
	}
	
	if (self->header > 0)
	{
		html_print_header (self->header, false, f_out);
	}
}

//
// BLOCK - Linebreak
//

static void
md_process_linebreak (struct _md_parser *self, FILE *f_out)
{
	if (self->p.buf_pos > 3 &&
	    self->p.buf[self->p.buf_pos - 2] == ' ' &&
	    self->p.buf[self->p.buf_pos - 3] == ' ')
	{
		fputs ("<br />", f_out);
	}
}

//
// BLOCK - Blockquote
//

static void
md_close_blockquote (struct _md_parser *self, FILE *f_out)
{
	if (self->blockquote)
	{
		html_print_tag ("blockquote", true, f_out);
		self->blockquote = false;
	}
}

static void
md_process_blockquote (struct _md_parser *self, FILE *f_out)
{
	if (*self->p.cursor == '>')
	{
		// Only open the html tag once for multiline blockquotes!
		if (!self->blockquote)
		{
			html_print_tag ("blockquote", false, f_out);
			self->blockquote = true;
		}
		
		parser_cursor_seek (&self->p, 1);
	}
	else
	{
		md_close_blockquote (self, f_out);
	}
}

//
// BLOCK - Image
//

static void
md_process_img (struct _md_parser *self, FILE *f_out)
{
	if (strscmp (self->p.cursor, "!["))
	{
		const char *alt_end = strstr (self->p.cursor, "](");
		const char *url_end = strchr (alt_end, ')');
		
		if (alt_end == NULL || url_end == NULL)
		{
			return;
		}
		
		parser_cursor_seek (&self->p, 2);
		fputs ("<img alt=\"", f_out);
		while (*self->p.cursor && self->p.cursor != alt_end)
		{
			fputc (*self->p.cursor, f_out);
			parser_cursor_seek (&self->p, 1);
		}
		
		parser_cursor_seek (&self->p, 2);
		fputs ("\" src=\"", f_out);
		while (*self->p.cursor && self->p.cursor != url_end)
		{
			fputc (*self->p.cursor, f_out);
			parser_cursor_seek (&self->p, 1);
		}
		fputs ("\">", f_out);
		parser_cursor_seek (&self->p, 1);
	}
}

//
// BLOCK - Link
//

static void
md_process_link (struct _md_parser *self, FILE *f_out)
{
	if (*self->p.cursor == '[')
	{
		const char *txt_end = strstr (self->p.cursor, "](");
		const char *url_end = strchr (txt_end, ')');
		
		if (txt_end == NULL || url_end == NULL)
		{
			return;
		}
		
		const char *url_start =  txt_end + 2;
		fputs ("<a href=\"", f_out);
		while (*url_start && url_start != url_end)
		{
			fputc (*url_start, f_out);
			url_start++;
		}
		
		parser_cursor_seek (&self->p, 1);
		fputs ("\">", f_out);
		while (*self->p.cursor && self->p.cursor != txt_end)
		{
			fputc (*self->p.cursor, f_out);
			parser_cursor_seek (&self->p, 1);
		}
		html_print_tag ("a", true, f_out);
		parser_cursor_seek (&self->p, (size_t) (url_end - txt_end) + 1);
	}
}

//
// BLOCK - Unordered list
//

static void
md_close_li (struct _md_parser *self, FILE *f_out)
{
	if (self->list_ul > 0)
	{
		html_print_tag ("li", true, f_out);
	}
}

static void
md_close_ul (struct _md_parser *self, FILE *f_out)
{
	for (;self->list_ul > 0; self->list_ul--)
	{
		html_print_tag ("ul", true, f_out);
	}
}

static void
md_process_ul (struct _md_parser *self, FILE *f_out)
{
	md_close_li (self, f_out);
	
	// UL
	if (strscmp (self->p.cursor, "* "))
	{
		if (self->list_ul == 0)
		{
			html_print_tag ("ul", false, f_out );
			self->list_ul++;
		}
		else if (self->list_ul == 2)
		{
			html_print_tag ("ul", true, f_out);
			self->list_ul--;
		}
		
		parser_cursor_seek (&self->p, 2);
	}
	// Sub UL
	else if (strscmp (self->p.cursor, "\t\t* "))
	{
		parser_cursor_seek (&self->p, 4);
		
		if (self->list_ul == 1)
		{
			html_print_tag ("ul", false, f_out);
			self->list_ul++;
		}
	}
	else
	{
		md_close_ul (self, f_out);
		return;
	}
	
	if (self->list_ul > 0)
	{
		html_print_tag ("li", false, f_out);
	}
}

//
// BLOCK - Table
//

static void
md_close_table (struct _md_parser *self, FILE *f_out)
{
	if (self->table)
	{
		if (self->thead)
		{
			html_print_tag ("th", true, f_out);
			html_print_tag ("tr", true, f_out);
			html_print_tag ("thead", true, f_out);
			self->thead = false;
		}
		
		if (self->tbody)
		{
			html_print_tag ("td", true, f_out);
			html_print_tag ("tr", true, f_out);
			html_print_tag ("tbody", true, f_out);
			self->tbody = false;
		}
		html_print_tag ("table", true, f_out);
		self->table = false;
	}
}

static void
md_process_table_nl (struct _md_parser *self, FILE *f_out)
{
	// Table
	if (self->p.cursor[0] == '|')
	{
		// Header
		if (!self->thead &&
		    !self->p.line_last &&
		    self->p.line_end[1] == '|' &&
		    self->p.line_end[2] == ' ' &&
		    self->p.line_end[3] == '-')
		{
			html_print_tag ("table", false, f_out);
			html_print_tag ("thead", false, f_out);
			html_print_tag ("tr", false, f_out);
			html_print_tag ("th", false, f_out);
			
			self->p.line_skip = true;
			self->thead = true;
			self->table = true;
		}
		else
		{
			if (self->thead)
			{
				html_print_tag ("th", true, f_out);
				html_print_tag ("tr", true, f_out);
				html_print_tag ("thead", true, f_out);
				html_print_tag ("tbody", false, f_out);
				self->thead = false;
				self->tbody = true;
			}
			else if (self->tbody)
			{
				html_print_tag ("td", true, f_out);
				html_print_tag ("tr", true, f_out);
			}
			
			html_print_tag ("tr", false, f_out);
			html_print_tag ("td", false, f_out);
		}
		parser_cursor_seek (&self->p, 2);
	}
	else
	{
		md_close_table (self, f_out);
	}
}

static void
md_process_table_chr (struct _md_parser *self, FILE *f_out)
{
	if (!self->code && self->table && *self->p.cursor == '|')
	{
		html_print_tag ((self->thead ? "th" : "td"), true, f_out);
		html_print_tag ((self->thead ? "th" : "td"), false, f_out);
		parser_cursor_seek (&self->p, 2);
	}
}


//
// INLINE - EMPHASIS
//

static void
md_process_emphasis (struct _md_parser *self, FILE *f_out)
{
	if (*self->p.cursor == '`')
	{
		html_print_tag ("pre", self->code, f_out);
		self->code = !self->code;
		
		parser_cursor_seek (&self->p, 1);
	}
	else if (!self->code)
	{
		const size_t i = strspn (self->p.cursor, "*_");
		switch (i)
		{
			case 1:
				html_print_tag ("i", self->emphasis, f_out);
				break;
			case 2:
				html_print_tag ("b", self->emphasis, f_out);
				break;
			case 3:
				fputs ((self->emphasis ? "</i></b>" : "<b><i>"), f_out);
				break;
			default:
				return;
		}
		self->emphasis = !self->emphasis;
		parser_cursor_seek (&self->p, i);
	}
}

//
// Parsing
//

/**
 * md_parser_render()
 * @self - MD_Parser to render.
 * @f_out - File descriptor to output to.
 *
 * Renders the markdown buffer as HTML and
 * write the HTML code to @f_out.
 *
 * Return: none
 */
void
md_parser_render (struct _md_parser *self, FILE *f_out)
{
	while (self->p.buf_pos < self->p.buf_len)
	{
		if (self->p.prev == NULL || *self->p.prev == '\n')
		{
			if (parser_cb_newline (&self->p))
			{
				continue;
			}
			
			md_process_header (self, f_out);
			md_process_linebreak  (self, f_out);
			md_process_blockquote (self, f_out);
			md_process_img (self, f_out);
			md_process_link (self, f_out);
			md_process_ul (self, f_out);
			md_process_table_nl (self, f_out);
		}
		md_process_table_chr (self, f_out);
		md_process_emphasis (self, f_out);
		
		if (*self->p.cursor != '\0')
		{
			fputc (*self->p.cursor, f_out);
		}
		parser_cursor_seek (&self->p, 1);
	}
	
	md_close_header (self, f_out);
	md_close_blockquote (self, f_out);
	md_close_li (self, f_out);
	md_close_ul (self, f_out);
	md_close_table (self, f_out);
}
