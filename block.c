#include "testfs.h"
#include "block.h"
#include "fslice.h"

static char zero[BLOCK_SIZE] = {0};

void
write_blocks(struct super_block *sb, char *blocks, int start, int nr)
{
        long pos;

        if ((pos = ftell(sb->dev)) < 0) {
                EXIT("ftell");
        }
        if (fseek(sb->dev, start * BLOCK_SIZE, SEEK_SET) < 0) {
                EXIT("fseek");
        }
        for (int i = 0; i < nr; i++) {
            fslice_write_block(blocks + i * BLOCK_SIZE, BLOCK_SIZE, start+i);
        }
        if (fwrite(blocks, BLOCK_SIZE, nr, sb->dev) != nr) {
                EXIT("fwrite");
        }
        if (fseek(sb->dev, pos, SEEK_SET) < 0) {
                EXIT("fseek");
        }
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
        long pos;

        if ((pos = ftell(sb->dev)) < 0) {
                EXIT("ftell");
        }
        if (fseek(sb->dev, start * BLOCK_SIZE, SEEK_SET) < 0) {
                EXIT("fseek");
        }
        if (fread(blocks, BLOCK_SIZE, nr, sb->dev) != nr) {
                EXIT("freed");
        }
        if (fseek(sb->dev, pos, SEEK_SET) < 0) {
                EXIT("fseek");
        }
        for (int i = 0; i < nr; i++) {
            fslice_read_block(blocks + i * BLOCK_SIZE, BLOCK_SIZE, start+i);
        }
}

