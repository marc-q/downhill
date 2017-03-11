/* Copyright 2017 Marc Volker Dickmann */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "src/markdown.h"

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
	char *buf = malloc (buf_len + 1);
	
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
	buf[buf_len - 1] = '\0';
	
	// Generate the HTML file
	html_print_head (filename_doc, f_doc);
	
	struct _md_parser parser;
	md_parser_new (&parser, buf, buf_len);
	md_parser_render (&parser, f_doc);
	
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
