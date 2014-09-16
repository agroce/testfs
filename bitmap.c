/*
 * Manager for arrays of bits.
 * See bitmap.h for more information.
 */

#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "bitmap.h"
#include "testfs.h"

/*
 * It would be a lot more efficient on most platforms to use u_int32_t
 * or unsigned long as the base type for holding bits. But we don't,
 * because if one uses any data type more than a single byte wide,
 * bitmap data saved on disk becomes endian-dependent, which is a
 * severe nuisance.
 */

struct bitmap {
	u_int32_t nbits;
	WORD_TYPE *v;
};

/* return negative value on error */
int
bitmap_create(u_int32_t nbits, struct bitmap **bp)
{
	struct bitmap *b; 
	u_int32_t words;

	words = DIVROUNDUP(nbits, BITS_PER_WORD);
	b = malloc(sizeof(struct bitmap));
	if (b == NULL) {
		return -ENOMEM;
	}
	b->v = malloc(words*sizeof(WORD_TYPE));
	if (b->v == NULL) {
		free(b);
		return -ENOMEM;
	}

	bzero(b->v, words*sizeof(WORD_TYPE));
	b->nbits = nbits;

	/* Mark any leftover bits at the end in use */
	if (nbits / BITS_PER_WORD < words) {
		u_int32_t j, ix = words-1;
		u_int32_t overbits = nbits - ix*BITS_PER_WORD;

		assert(nbits / BITS_PER_WORD == words-1);
		assert(overbits > 0 && overbits < BITS_PER_WORD);
		
		for (j=overbits; j<BITS_PER_WORD; j++) {
			b->v[ix] |= ((WORD_TYPE)1 << j);
		}
	}
        *bp = b;
	return 0;
}

void *
bitmap_getdata(struct bitmap *b)
{
	return b->v;
}

/* return negative value on error */
int
bitmap_alloc(struct bitmap *b, u_int32_t *index)
{
	u_int32_t ix;
	u_int32_t maxix = DIVROUNDUP(b->nbits, BITS_PER_WORD);
	u_int32_t offset;

	for (ix=0; ix<maxix; ix++) {
		if (b->v[ix]!=WORD_ALLBITS) {
			for (offset = 0; offset < BITS_PER_WORD; offset++) {
				WORD_TYPE mask = ((WORD_TYPE)1)<<offset;
				if ((b->v[ix] & mask)==0) {
					b->v[ix] |= mask;
					*index = (ix*BITS_PER_WORD)+offset;
					assert(*index < b->nbits);
					return 0;
				}
			}
			assert(0);
		}
	}
	return -ENOSPC;
}

static inline void
bitmap_translate(u_int32_t bitno, u_int32_t *ix, WORD_TYPE *mask)
{
	u_int32_t offset;
	*ix = bitno / BITS_PER_WORD;
	offset = bitno % BITS_PER_WORD;
	*mask = ((WORD_TYPE)1) << offset;
}

void
bitmap_mark(struct bitmap *b, u_int32_t index)
{
	u_int32_t ix;
	WORD_TYPE mask;
	assert(index < b->nbits);
	bitmap_translate(index, &ix, &mask);

	assert((b->v[ix] & mask)==0);

	b->v[ix] |= mask;
}

void
bitmap_unmark(struct bitmap *b, u_int32_t index)
{
	u_int32_t ix;
	WORD_TYPE mask;
	assert(index < b->nbits);
	bitmap_translate(index, &ix, &mask);

	assert((b->v[ix] & mask)!=0);

	b->v[ix] &= ~mask;
}


int
bitmap_isset(struct bitmap *b, u_int32_t index) 
{
        u_int32_t ix;
        WORD_TYPE mask;
        bitmap_translate(index, &ix, &mask);

        return (b->v[ix] & mask);
}

void
bitmap_destroy(struct bitmap *b)
{
	free(b->v);
	free(b);
}

/* return TRUE when equal, FALSE when not equal */
int
bitmap_equal(struct bitmap *a, struct bitmap *b)
{
	u_int32_t ix;
	u_int32_t maxix;

        if (a->nbits != b->nbits)
                return 0;
        maxix = DIVROUNDUP(b->nbits, BITS_PER_WORD);
	for (ix=0; ix<maxix; ix++) {
                if (a->v[ix] != b->v[ix])
                        return 0;
        }
        return 1;
}

int
bitmap_nr_allocated(struct bitmap *b)
{
        int i;
        int nr = 0;

        for (i = 0; i < b->nbits; i++) {
                if (bitmap_isset(b, i))
                        nr++;
        }
        return nr;
}

