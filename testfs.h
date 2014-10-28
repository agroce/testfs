#ifndef _TESTFS_H
#define _TESTFS_H

#include <unistd.h>
#include "common.h"

#define BLOCK_SIZE 64

#define SUPER_BLOCK_SIZE    1           /* start 0x0000 */
#define INODE_FREEMAP_SIZE  1           /* start 0x0040 */
#define BLOCK_FREEMAP_SIZE  2           /* start 0x0080 */
#define CSUM_TABLE_SIZE    60           /* start 0x0100 */
#define NR_INODE_BLOCKS   128           /* start 0x1000 */
#define NR_DATA_BLOCKS    512           /* start 0x3000 */

struct super_block;
struct inode;

#define MAX_ARGS 5

struct context {
        int nargs;
        char *cmd[MAX_ARGS+1];          // +1 to keep the overflows
        struct inode *cur_dir;
};

int cmd_cd(struct super_block *, struct context *c);
int cmd_pwd(struct super_block *, struct context *c);
int cmd_ls(struct super_block *, struct context *c);
int cmd_lsr(struct super_block *, struct context *c);
int cmd_create(struct super_block *, struct context *c);
int cmd_stat(struct super_block *, struct context *c);
int cmd_rm(struct super_block *, struct context *c);
int cmd_mkdir(struct super_block *, struct context *c);

int cmd_cat(struct super_block *, struct context *c);
int cmd_write(struct super_block *, struct context *c);

int cmd_checkfs(struct super_block *, struct context *c);

#endif /* _TESTFS_H */
