#include "testfs.h"
#include "block.h"

static char zero[BLOCK_SIZE] = {0};

void
write_blocks(struct super_block *sb, char *blocks, int start, int nr)
{
  printf("WRITING FROM %p TO %P: START + %d, %d blocks\n", blocks, sb->storage, start, nr)
  memcpy(sb->storage + (start * BLOCK_SIZE), blocks, nr * BLOCK_SIZE);
}

void
zero_blocks(struct super_block *sb, int start, int nr)
{
        int i;

        for (i = 0; i < nr; i++) {
                write_blocks(sb, zero, start + i, 1);
        }
}

void
read_blocks(struct super_block *sb, char *blocks, int start, int nr)
{
  printf("READING FROM %p TO %P: START + %d, %d blocks\n", sb->storage, blocks, start, nr)  
  memcpy(blocks, sb->storage + (start * BLOCK_SIZE), nr * BLOCK_SIZE);
}

