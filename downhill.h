#ifndef __DOWNHILL_H__
#define __DOWNHILL_H__

struct _parser
{
	// List
	bool list;
	size_t list_depth;
	
	// Flags: Misc
	bool emphasis;
	bool code;
	bool table;
	bool thead;
	
	// Flags: Line
	bool line_last;
	bool line_skip;
	
	// Tags
	size_t pop_count;
	struct _taglist taglist;
	
	// Buffer
	size_t buf_pos;
	size_t buf_len;
	const char *buf;
	
	const char *cursor;
	char *line_end;
};

#endif /* __DOWNHILL_H__ */
