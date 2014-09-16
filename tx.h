#ifndef _TX_H
#define _TX_H

struct super_block;

typedef enum {TX_NONE, TX_WRITE, TX_CREATE, TX_RM, TX_UMOUNT } tx_type;
void testfs_tx_start(struct super_block *sb, tx_type type);
void testfs_tx_commit(struct super_block *sb, tx_type type);

#endif /* _TX_H */
