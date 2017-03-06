#ifndef __DOWNHILL_HTML_H__
#define __DOWNHILL_HTML_H__

void html_print_head (const char*, FILE*);
void html_print_footer (FILE*);

void html_print_tag (const char*, const bool, FILE*);

void html_print_header (const size_t, const bool, FILE*);

#endif /* __DOWNHILL_HTML_H__ */
