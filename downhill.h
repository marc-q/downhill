#ifndef __DOWNHILL_H__
#define __DOWNHILL_H__

#define TAG_MAX 20
#define TAG_MASK_VALUE 0x7FFFFFFF
#define TAG_MASK_CLOSE 0x80000000

// The last bit is reserved for the open/close flag!
enum _tags
{
	TAG_NONE = 0,
	
	TAG_H1,
	TAG_H2,
	TAG_H3,
	TAG_H4,
	TAG_H5,
	TAG_H6,
	
	TAG_BLOCKQUOTE,
	TAG_UL,
	TAG_LI
};

struct _parser
{
	size_t list;
	bool emphasis;
	bool skip_line;
	
	enum _tags tag[TAG_MAX];
	
	size_t buf_pos;
	size_t buf_len;
	const char *buf;
	
	const char *cursor;
	
	char *line_end;
};

#endif /* __DOWNHILL_H__ */
