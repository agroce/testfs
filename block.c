#include "testfs.h"
#include "block.h"

static char zero[BLOCK_SIZE] = {0};

static int reset_countdown = -1;

static jmp_buf jmp_target;

int set_reset_countdown(int k) {
  reset_countdown = k;
  return setjmp(jmp_target);
}

int get_reset_countdown() {
  return reset_countdown;
}

void
write_blocks(struct super_block *sb, char *blocks, int start, int nr)
{
  if (reset_countdown > 0) {
    reset_countdown -= 1;
  } else if (reset_countdown == 0) {
    longjmp(jmp_target, 1);
  }
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
  memcpy(blocks, sb->storage + (start * BLOCK_SIZE), nr * BLOCK_SIZE);
}

