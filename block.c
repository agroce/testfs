#include "testfs.h"
#include "block.h"
#include "ops.h"

static char zero[BLOCK_SIZE] = {0};

void
write_blocks(struct super_block *sb, char *blocks, int start, int nr)
{
        long pos;



        if ((pos = FOPS.seek(sb->dev_fd, 0, SEEK_CUR)) < 0) {
                EXIT("ftell");
        }
        if (FOPS.seek(sb->dev_fd, start * BLOCK_SIZE, SEEK_SET) < 0) {
                EXIT("fseek");
        }

        if (FOPS.write(sb->dev_fd, blocks, BLOCK_SIZE * nr) != (BLOCK_SIZE * nr)) {
                EXIT("fwrite");
        }
        if (FOPS.seek(sb->dev_fd, pos, SEEK_SET) < 0) {
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
        pos = FOPS.seek(sb->dev_fd, 0, SEEK_CUR);
        if (pos < 0) {
                EXIT("ftell");
        }
        if (FOPS.seek(sb->dev_fd, start * BLOCK_SIZE, SEEK_SET) < 0) {
                EXIT("fseek");
        }
        if (FOPS.read(sb->dev_fd, blocks, BLOCK_SIZE * nr) != (nr * BLOCK_SIZE)) {
                EXIT("fread");
        }
        if (FOPS.seek(sb->dev_fd, pos, SEEK_SET) < 0) {
                EXIT("fseek");
        }
}

