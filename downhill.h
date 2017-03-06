#ifndef __DOWNHILL_H__
#define __DOWNHILL_H__

enum close_tags
{
	CLOSE_TAG_NONE = 0,
	CLOSE_TAG_H1,
	CLOSE_TAG_H2,
	CLOSE_TAG_H3,
	CLOSE_TAG_H4,
	CLOSE_TAG_H5,
	CLOSE_TAG_H6,
	CLOSE_TAG_BLOCKQUOTE,
	CLOSE_TAG_UL,
	CLOSE_TAG_LI
};

struct _parser
{
	size_t list;
	bool emphasis;
	bool skip_line;
	enum close_tags close_tag[3];
	
	size_t buf_pos;
	size_t buf_len;
	const char *buf;
	
	const char *cursor;
	
	char *line_end;
};

#endif /* __DOWNHILL_H__ */
