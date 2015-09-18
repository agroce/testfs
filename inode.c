#include "testfs.h"
#include "super.h"
#include "block.h"
#include "inode.h"
#include "list.h"
#include "csum.h"

#include "fslice.h"

/* inode flags */
#define I_FLAGS_DIRTY     0x1

struct inode {
        int i_flags;
        struct dinode in;
        int i_nr;
        struct hlist_node hnode; /* keep these structures in a hash table */
        int i_count;
        struct super_block *sb;
};

static struct hlist_head *inode_hash_table = NULL;

#define INODE_HASH_SHIFT 8

#define inode_hashfn(nr)	\
	hash_int((unsigned int)nr, INODE_HASH_SHIFT)

static const int inode_hash_size = (1 << INODE_HASH_SHIFT);

void
inode_hash_init(void)
{
        int i;

        inode_hash_table = malloc(inode_hash_size * sizeof(struct hlist_head));
        if (!inode_hash_table) {
                EXIT("malloc");
        }
        for (i = 0; i < inode_hash_size; i++) {
                INIT_HLIST_HEAD(&inode_hash_table[i]);
        }
}

void
inode_hash_destroy(void)
{
        int i;
        assert(inode_hash_table);
        for (i = 0; i < inode_hash_size; i++) {
                assert(hlist_empty(&inode_hash_table[i]));
        }
        free(inode_hash_table);
}

static struct inode *
inode_hash_find(struct super_block *sb, int inode_nr)
{
        struct hlist_node *elem;
        struct inode *in;

        hlist_for_each_entry(in, elem, 
                             &inode_hash_table[inode_hashfn(inode_nr)], hnode) {
                if ((in->sb == sb) && (in->i_nr == inode_nr)) {
                        return in;
                }
        }
	return NULL;
}

static void
inode_hash_insert(struct inode *in)
{
        INIT_HLIST_NODE(&in->hnode);
        hlist_add_head(&in->hnode, 
                       &inode_hash_table[inode_hashfn(in->i_nr)]);
}

static void
inode_hash_remove(struct inode *in)
{
        hlist_del(&in->hnode);
}

static int
testfs_inode_to_block_nr(struct inode *in)
{
        int block_nr = in->i_nr / INODES_PER_BLOCK;
        assert(block_nr >= 0);
        assert(block_nr < NR_INODE_BLOCKS);
        return block_nr;
}

static int
testfs_inode_to_block_offset(struct inode *in)
{
        int block_offset = (in->i_nr % INODES_PER_BLOCK) * 
                sizeof(struct dinode);
        assert(block_offset >= 0);
        assert(block_offset < BLOCK_SIZE);
        return block_offset;
}

static void
testfs_read_inode_block(struct inode *in, char *block)
{
        int block_nr = testfs_inode_to_block_nr(in);
        read_blocks(in->sb, block, in->sb->sb.inode_blocks_start + block_nr, 1);
}

static void
testfs_write_inode_block(struct inode *in, char *block)
{
        int block_nr = testfs_inode_to_block_nr(in);
        write_blocks(in->sb, block, 
                     in->sb->sb.inode_blocks_start + block_nr, 1);
}

/* given logical block number, read physical block
 * return physical block number.
 * returns 0 if physical block does not exist.
 * returns negative value on other errors. */
static int
testfs_get_block(struct inode *in, char *block, int log_block_nr)
{
        int phy_block_nr;

        assert(log_block_nr >= 0);
        if (log_block_nr < NR_DIRECT_BLOCKS) {
                phy_block_nr = in->in.i_block_nr[log_block_nr];
                goto read_block;
        }
        log_block_nr -= NR_DIRECT_BLOCKS;
        if (log_block_nr >= NR_INDIRECT_BLOCKS)
                return -EFBIG;
        if (in->in.i_indirect == 0)
                return 0;
        read_blocks(in->sb, block, in->in.i_indirect, 1);
        phy_block_nr = ((int *)block)[log_block_nr];
read_block:
        if (phy_block_nr > 0)
                read_blocks(in->sb, block, phy_block_nr, 1);
        return phy_block_nr;
}

static int
testfs_allocate_block(struct inode *in, char *block, int log_block_nr)
{
        char indirect[BLOCK_SIZE];
        int phy_block_nr;

        assert(log_block_nr >= 0);
        phy_block_nr = testfs_get_block(in, block, log_block_nr);
        if (phy_block_nr != 0)
                return phy_block_nr;
        if (log_block_nr < NR_DIRECT_BLOCKS) {
                phy_block_nr = testfs_alloc_block(in->sb, block);
                if (phy_block_nr < 0)
                        return phy_block_nr;
                in->in.i_block_nr[log_block_nr] = phy_block_nr;
                in->i_flags |= I_FLAGS_DIRTY;
                return phy_block_nr;
        }
        log_block_nr -= NR_DIRECT_BLOCKS;
        assert(log_block_nr < NR_INDIRECT_BLOCKS);
        if (in->in.i_indirect == 0) {
                phy_block_nr = testfs_alloc_block(in->sb, indirect);
                if (phy_block_nr < 0)
                        return phy_block_nr;
                in->in.i_indirect = phy_block_nr;
                in->i_flags |= I_FLAGS_DIRTY;
        } else {
                read_blocks(in->sb, indirect, in->in.i_indirect, 1);
        }
        phy_block_nr = testfs_alloc_block(in->sb, block);
        if (phy_block_nr > 0)
                ((int *)indirect)[log_block_nr] = phy_block_nr;
        write_blocks(in->sb, indirect, in->in.i_indirect, 1);
        return phy_block_nr;
}

struct inode *
testfs_get_inode(struct super_block *sb, int inode_nr)
{
        char block[BLOCK_SIZE];
        int block_offset;
        struct inode *in;

        in = inode_hash_find(sb, inode_nr);
        if (in) {
                in->i_count++;
                return in;
        }
        if ((in = calloc(1, sizeof(struct inode))) == NULL) {
                EXIT("calloc");
        }
        in->i_flags = 0;
        in->i_nr = inode_nr;
        in->sb = sb;
        in->i_count = 1;
        testfs_read_inode_block(in, block);
        block_offset = testfs_inode_to_block_offset(in);
        memcpy(&in->in, block + block_offset, sizeof(struct dinode));
        inode_hash_insert(in);
        return in;
}

void
testfs_sync_inode(struct inode *in)
{
        char block[BLOCK_SIZE];
        int block_offset;

        assert(in->i_flags & I_FLAGS_DIRTY);
        testfs_read_inode_block(in, block);
        block_offset = testfs_inode_to_block_offset(in);
        memcpy(block + block_offset, &in->in, sizeof(struct dinode));
        testfs_write_inode_block(in, block);
        in->i_flags &= ~I_FLAGS_DIRTY;
}

void
testfs_put_inode(struct inode *in)
{
        assert((in->i_flags & I_FLAGS_DIRTY) == 0);
        if (--in->i_count == 0) {
                inode_hash_remove(in);
                free(in);
        }
}

inline int
testfs_inode_get_size(struct inode *in)
{
        return in->in.i_size;
}

inline inode_type
testfs_inode_get_type(struct inode *in)
{
        return in->in.i_type;
}

inline int
testfs_inode_get_nr(struct inode *in)
{
        return in->i_nr;
}

inline struct super_block *
testfs_inode_get_sb(struct inode *in)
{
        return in->sb;
}

/* returns negative value on error */
int
testfs_create_inode(struct super_block *sb, inode_type type, struct inode **inp)
{
        struct inode *in;
        int inode_nr = testfs_get_inode_freemap(sb);

        if (inode_nr < 0) {
                return inode_nr;
        }
        in = testfs_get_inode(sb, inode_nr);
        in->in.i_type = type;
        in->i_flags |= I_FLAGS_DIRTY;
        *inp = in;
        return 0;
}

void
testfs_remove_inode(struct inode *in)
{
        testfs_truncate_data(in, 0);
        /* zero the inode */
        bzero(&in->in, sizeof(struct dinode));
        in->i_flags |= I_FLAGS_DIRTY;
        testfs_put_inode_freemap(in->sb, in->i_nr);
        testfs_sync_inode(in);
        testfs_put_inode(in);
}

/* read data from inode in, from start to start+size, into buf[size].
 * return 0 on success.
 * return negative value on error. */
int
testfs_read_data(struct inode *in, int start, char *buf, const int size)
{
        char block[BLOCK_SIZE];
        int b_offset = start % BLOCK_SIZE; /* src offset in block for copy */
        int buf_offset = 0; /* dst offset in buf for copy */
        int done = 0;
        
        assert(buf);
        assert((start + size) <= in->in.i_size);
        do {
                int block_nr = (start + buf_offset)/BLOCK_SIZE;
                int copy_size;

                block_nr = testfs_get_block(in, block, block_nr);
                if (block_nr < 0)
                        return block_nr;
                assert(block_nr > 0);
                if ((size - buf_offset) <= (BLOCK_SIZE - b_offset)) {
                        copy_size = size - buf_offset;
                        done = 1;
                } else {
                        copy_size = BLOCK_SIZE - b_offset;
                }
                memcpy(buf + buf_offset, block + b_offset, copy_size);
                buf_offset += copy_size;
                b_offset = 0;
        } while (!done);
        //fslice_data(buf, size);
        return 0;
}

/* write data from buf[size] to inode in, from start to start+size.
 * return 0 on success.
 * return negative value on error. */
/* TODO: on error, unallocate blocks */
int
testfs_write_data(struct inode *in, int start, char *buf, const int size)
{
        char block[BLOCK_SIZE];
        int b_offset = start % BLOCK_SIZE;  /* dst offset in block for copy */
        int buf_offset = 0; /* src offset in buf for copy */
        int done = 0;
        
        fslice_data(buf, size);

        assert(buf);
        assert(start <= in->in.i_size);
        do {
                int block_nr = (start + buf_offset)/BLOCK_SIZE;
                int copy_size;
                int csum;

                block_nr = testfs_allocate_block(in, block, block_nr);
                if (block_nr < 0) {
                        int orig_size = in->in.i_size;
                        in->in.i_size = MAX(orig_size, start + buf_offset);
                        in->i_flags |= I_FLAGS_DIRTY;
                        testfs_truncate_data(in, orig_size);
                        return block_nr;
                }
                assert(block_nr > 0);
                if ((size - buf_offset) <= (BLOCK_SIZE - b_offset)) {
                        copy_size = size - buf_offset;
                        done = 1;
                } else {
                        copy_size = BLOCK_SIZE - b_offset;
                }
                memcpy(block + b_offset, buf + buf_offset, copy_size);
                csum = testfs_calculate_csum(block, BLOCK_SIZE);
                write_blocks(in->sb, block, block_nr, 1);
                testfs_put_csum(in->sb, block_nr, csum);
                buf_offset += copy_size;
                b_offset = 0;
        } while (!done);
        in->in.i_size = MAX(in->in.i_size, start + size);
        in->i_flags |= I_FLAGS_DIRTY;
        return 0;
}

void
testfs_truncate_data(struct inode *in, const int size)
{
        int i;
        int s_block_nr;
        int e_block_nr;

        if (in->in.i_size <= size)
                return;
        s_block_nr = DIVROUNDUP(size, BLOCK_SIZE);
        e_block_nr = DIVROUNDUP(in->in.i_size, BLOCK_SIZE);

        /* remove direct blocks */
        for (i = s_block_nr; i < e_block_nr && i < NR_DIRECT_BLOCKS; i++) {
                assert(in->in.i_block_nr[i] > 0);
                testfs_free_block(in->sb, in->in.i_block_nr[i]);
                in->in.i_block_nr[i] = 0;
                in->i_flags |= I_FLAGS_DIRTY;
        }
        s_block_nr -= NR_DIRECT_BLOCKS;
        s_block_nr = MAX(s_block_nr, 0);
        e_block_nr -= NR_DIRECT_BLOCKS;

        if (e_block_nr > 0) { /* remove indirect blocks */
                char block[BLOCK_SIZE];
                assert(in->in.i_indirect > 0);
                read_blocks(in->sb, block, in->in.i_indirect, 1);
                for (i = s_block_nr; i < e_block_nr && i < NR_INDIRECT_BLOCKS;
                     i++) {
                        int block_nr = ((int *)block)[i];
                        assert(block_nr > 0);
                        testfs_free_block(in->sb, block_nr);
                        ((int *)block)[i] = 0;
                }
                if (s_block_nr == 0) {
                        testfs_free_block(in->sb, in->in.i_indirect);
                        in->in.i_indirect = 0;
                        in->i_flags |= I_FLAGS_DIRTY;
                } else {
                        write_blocks(in->sb, block, in->in.i_indirect, 1);
                }
        } else {
                assert(in->in.i_indirect == 0);
        }
        in->in.i_size = size;
        in->i_flags |= I_FLAGS_DIRTY;
}

int
testfs_check_inode(struct super_block *sb, struct bitmap *b_freemap,
                   struct inode *in)
{
        int size = 0;
        int i;
        char block[BLOCK_SIZE];

        for (i = 0; i < NR_DIRECT_BLOCKS; i++) {
                int block_nr = in->in.i_block_nr[i];
                if (block_nr == 0)
                        return size;
                size += BLOCK_SIZE;
                
                /* verify checksum */
                testfs_verify_csum(sb, block_nr);
                
                /* mark block freemap */
                block_nr -= sb->sb.data_blocks_start;
                bitmap_mark(b_freemap, block_nr);
        }
        if (!in->in.i_indirect) {
                return size;
        }
        bitmap_mark(b_freemap, in->in.i_indirect - sb->sb.data_blocks_start);
        read_blocks(in->sb, block, in->in.i_indirect, 1);
        for (i = 0; i < NR_INDIRECT_BLOCKS; i++) {
                int block_nr = ((int *)block)[i];
                if (block_nr == 0)
                        return size;
                size += BLOCK_SIZE;
                block_nr -= sb->sb.data_blocks_start;
                bitmap_mark(b_freemap, block_nr);
        }
        return size;
}
