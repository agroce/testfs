#ifndef _INODE_H
#define _INODE_H

#include "list.h"
#include "bitmap.h"

typedef enum {I_NONE, I_FILE, I_DIR} inode_type;

#define NR_DIRECT_BLOCKS 4
#define NR_INDIRECT_BLOCKS (BLOCK_SIZE/sizeof(int))

struct dinode {
        inode_type i_type;                      /* 0x00 */
        int i_size;                             /* 0x04 */
        int i_mod_time;                         /* 0x08 */
        int i_block_nr[NR_DIRECT_BLOCKS];       /* 0x0C */
        int i_indirect;                         /* 0x1C */
};

#define INODES_PER_BLOCK (BLOCK_SIZE/(sizeof(struct dinode)))

void inode_hash_init(void);
void inode_hash_destroy(void);
struct inode *testfs_get_inode(struct super_block *sb, int inode_nr);
void testfs_sync_inode(struct inode *in);
void testfs_put_inode(struct inode *in);
int testfs_inode_get_size(struct inode *in);
inode_type testfs_inode_get_type(struct inode *in);
int testfs_inode_get_nr(struct inode *in);
struct super_block *testfs_inode_get_sb(struct inode *in);
int testfs_create_inode(struct super_block *sb, inode_type type,
                        struct inode **inp);
void testfs_remove_inode(struct inode *in);
int testfs_read_data(struct inode *in, int start, char *buf, const int size);
int testfs_write_data(struct inode *in, int start, char *name, const int size);
void testfs_truncate_data(struct inode *in, const int size);
int testfs_check_inode(struct super_block *sb, struct bitmap *b_freemap,
                       struct inode *in);

#endif /* _INODE_H */
