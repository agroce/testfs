#include <assert.h>
#include "super.h"
#include "tx.h"

char *tx_type_array[] = {"TX_NONE",
                         "TX_WRITE",
                         "TX_CREATE",
                         "TX_RM",
                         "TX_UMOUNT"};

void
testfs_tx_start(struct super_block *sb, tx_type type)
{
        assert(sb->tx_in_progress == TX_NONE);
        sb->tx_in_progress = type;
}

void
testfs_tx_commit(struct super_block *sb, tx_type type)
{
        assert(sb->tx_in_progress == type);
        sb->tx_in_progress = TX_NONE;
}
