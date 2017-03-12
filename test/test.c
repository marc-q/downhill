/* Copyright 2017 Marc Volker Dickmann */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../src/markdown.h"

#define DOWNHILL_TEST_PASSED 1
#define DOWNHILL_TEST_FAILED 0

//
// UTILS
//

static bool
streql (const char *a, const char *b)
{
	return (strlen (a) == strlen (b) &&
		strcmp (a, b) == 0);
}

static void
test_print_summary (const int points, const int points_max)
{
	printf ("\n\nPoints\t%i/%i\n", points, points_max);
	printf ("Failed\t%i\n", points_max - points);
}

static void
test_print_passed (const char *name)
{
	printf ("%s\t\tPASSED!\n", name);
}

static void
test_print_failed (const char *name)
{
	printf ("%s\t\tFAILED!\n", name);
}


static char*
test_read_file (char *buf, size_t *buf_len, const char *filename)
{
	FILE *f = fopen (filename, "r");
	if (f == NULL)
	{
		printf ("Error: Could'nt open the file: %s!\n", filename);
		return NULL;
	}
	
	// Read the file
	fseek (f, 0, SEEK_END);
	
	*buf_len = ftell (f);
	buf = malloc (*buf_len + 1);
	
	fseek (f, 0, SEEK_SET);
	if (fread (buf, sizeof (char), *buf_len, f) != *buf_len)
	{
		printf ("Error: Could'nt read the file: %s!\n", filename);
		free (buf);
		buf = NULL;
	}
	else
	{
		buf[*buf_len - 1] = '\0';
	}
	fclose (f);
	return buf;
}

static int
test_file (const char *testcase, const char *filename_src, const char *filename_cmp)
{
	FILE *f_doc = fopen ("out.html", "w");
	if (f_doc == NULL)
	{
		printf ("Error: Could'nt open the file: out.html!\n");
		return DOWNHILL_TEST_FAILED;
	}
	
	// Read the Markdown file
	size_t buf_len = 0;
	char *buf = NULL;
	
	buf = test_read_file (buf, &buf_len, filename_src);
	if (buf == NULL)
	{
		fclose (f_doc);
		return DOWNHILL_TEST_FAILED;
	}
	
	// Generate the HTML file
	struct _md_parser parser;
	md_parser_new (&parser, buf, buf_len);
	md_parser_render (&parser, f_doc);
	
	free (buf);
	fclose (f_doc);
	
	// Read the output
	buf_len = 0;
	buf = test_read_file (buf, &buf_len, "out.html");
	if (buf == NULL)
	{
		return DOWNHILL_TEST_FAILED;
	}
	
	// Read the expected output
	size_t buf_cmp_len = 0;
	char *buf_cmp = NULL;
	
	buf_cmp = test_read_file (buf_cmp, &buf_cmp_len, filename_cmp);
	if (buf_cmp == NULL)
	{
		free (buf);
		return DOWNHILL_TEST_FAILED;
	}
	
	int r = DOWNHILL_TEST_FAILED;
	if (streql (buf, buf_cmp))
	{
		test_print_passed (testcase);
		r = DOWNHILL_TEST_PASSED;
	}
	else
	{
		test_print_failed (testcase);
	}
	
	free (buf);
	free (buf_cmp);
	return r;
}

int
main (int argc, char *argv[])
{
	int t = 0;
	
	t += test_file ("link_base", "testcases/link/base.md", "testcases/link/base.html");
	
	test_print_summary (t, 1);
	return 0;
}
