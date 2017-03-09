#ifndef __DOWNHILL_TAG_H__
#define __DOWNHILL_TAG_H__

#define TAG_MAX 20
#define TAG_MASK_VALUE 0x7FFFFFFF
#define TAG_MASK_CLOSE 0x80000000

// The last bit is reserved for the open/close flag!
enum _tags
{
	TAG_NONE = 0,
	
	// Header
	TAG_H1,
	TAG_H2,
	TAG_H3,
	TAG_H4,
	TAG_H5,
	TAG_H6,
	
	// MISC
	TAG_BLOCKQUOTE,
	TAG_BR,
	
	// List
	TAG_UL,
	TAG_LI,
	
	// Table
	TAG_TABLE,
	TAG_THEAD,
	TAG_TH,
	TAG_TBODY,
	TAG_TR,
	TAG_TD
};

struct _taglist
{
	size_t itemcount;
	enum _tags tag[TAG_MAX];
};

void taglist_init (struct _taglist*);

void tag_push (struct _taglist*, const enum _tags, const bool);
void tag_add (struct _taglist*, const enum _tags);
void tag_pop (struct _taglist*, FILE*);

void tag_flush (struct _taglist*, FILE*);

#endif /* __DOWNHILL_TAG_H__ */
