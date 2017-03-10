/* Copyright 2017 Marc Volker Dickmann */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "html.h"

//
// HTML Templates
//

void
html_print_head (const char *pagename, FILE *f)
{
	fputs ("<html>\n<head>\n\t\t<meta charset=\"UTF-8\">\n\t\t<title>", f);
	fputs (pagename, f);
	fputs ("</title>\n<link rel=\"stylesheet\" href=\"style.css\">\n</head>\n<body>\n", f);
}

void
html_print_footer (FILE *f)
{
	fputs ("\n</body>\n</html>", f);
}

void
html_print_tag (const char *name, const bool close, FILE *f)
{
	fputs ((close ? "</" : "<"), f);
	fputs (name, f);
	fputc ('>', f);
}

void
html_print_header (const size_t tier, const bool close, FILE *f)
{
	fputs ((close ? "</h" : "<h"), f);
	fputc (tier + '0', f);
	fputs ((close ? ">" : ">"), f);
}
