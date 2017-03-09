/* Copyright 2017 Marc Volker Dickmann */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "html.h"
#include "tag.h"

void
taglist_init (struct _taglist *self)
{
	self->itemcount = 1;
	for (size_t i = 0; i < TAG_MAX; i++)
	{
		self->tag[i] = TAG_NONE;
	}
}

void
tag_push (struct _taglist *self, const enum _tags tag, const bool close)
{
	for (size_t i = TAG_MAX - 1; i > 0; i--)
	{
		self->tag[i] = self->tag[i - 1];
	}
	self->tag[0] = (close << 31) | (tag & TAG_MASK_VALUE);
	self->itemcount++;
}

void
tag_add (struct _taglist *self, const enum _tags tag)
{
	tag_push (self, tag, true);
	tag_push (self, tag, false);
}

void
tag_pop (struct _taglist *self, FILE *f_out)
{
	const bool close = self->tag[0] & TAG_MASK_CLOSE;
	bool newline = false;
	
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
		// Misc
		case TAG_BLOCKQUOTE:
			html_print_tag ("blockquote", close, f_out);
			break;
		case TAG_BR:
			html_print_tag ("br /", close, f_out);
			break;
		// List
		case TAG_UL:
			html_print_tag ("ul", close, f_out);
			break;
		case TAG_LI:
			html_print_tag ("li", close, f_out);
			break;
		// Table
		case TAG_TABLE:
			html_print_tag ("table", close, f_out);
			newline = true;
			break;
		case TAG_THEAD:
			html_print_tag ("thead", close, f_out);
			newline = true;
			break;
		case TAG_TH:
			html_print_tag ("th", close, f_out);
			break;
		case TAG_TBODY:
			html_print_tag ("tbody", close, f_out);
			newline = true;
			break;
		case TAG_TR:
			html_print_tag ("tr", close, f_out);
			newline = true;
			break;
		case TAG_TD:
			html_print_tag ("td", close, f_out);
			break;
		default:
			return;
	}
	
	if (newline)
	{
		fputc ('\n', f_out);
	}
	
	for (size_t i = 1; i < self->itemcount; i++)
	{
		self->tag[i - 1] = self->tag[i];
	}
	self->tag[TAG_MAX - 1] = TAG_NONE;
	self->itemcount--;
}

void
tag_flush (struct _taglist *self, FILE *f_out)
{
	while (self->itemcount > 1)
	{
		tag_pop (self, f_out);
	}
}
