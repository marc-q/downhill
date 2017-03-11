#ifndef __DOWNHILL_MARKDOWN_H__
#define __DOWNHILL_MARKDOWN_H__

#include "html.h"
#include "parser.h"

struct _md_parser
{
	// Will be set to the header tier to opend/close
	// header tags
	size_t header;
	
	// Will be toggled to opend/close the blockquote tag
	bool blockquote;
	
	size_t list_ul;
	
	bool table;
	bool thead;
	bool tbody;
	
	// Will be toggled to opend/close close tags
	bool code;
	
	// Will be toggled to opend/close emphasis tags
	bool emphasis;
	
	// Inherit from the base parser
	struct _parser p;
};

void md_parser_new (struct _md_parser*, const char*, const size_t);

void md_parser_render (struct _md_parser*, FILE*);

#endif /* __DOWNHILL_MARKDOWN_H__ */
