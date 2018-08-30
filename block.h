#ifndef _BLOCK_H
#define _BLOCK_H
#include "super.h"

void set_reset_countdown(int k);
int get_reset_countdown();

void write_blocks(struct super_block *sb, char *blocks, int start, int nr);
void zero_blocks(struct super_block *sb, int start, int nr);
void read_blocks(struct super_block *sb, char *blocks, int start, int nr);

#endif /* _BLOCK_H */

