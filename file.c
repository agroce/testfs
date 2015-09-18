#include "testfs.h"
#include "inode.h"
#include "dir.h"
#include "tx.h"
#include "fslice.h"


int
cmd_cat(struct super_block *sb, struct context *c)
{
        char *buf;
        int inode_nr;
        struct inode *in;
        int ret = 0;
        int sz;
        int i;

        if (c->nargs < 2) {
                return -EINVAL;
        }    
        for (i = 1; ret == 0 && i < c->nargs; i++ )
        {
                inode_nr = testfs_dir_name_to_inode_nr(c->cur_dir, c->cmd[i]);
                if (inode_nr < 0)
                        return inode_nr;
                in = testfs_get_inode(sb, inode_nr);
                if (testfs_inode_get_type(in) == I_DIR) {
                        ret = -EISDIR;
                        goto out;
                }
                sz = testfs_inode_get_size(in);
                if (sz > 0) {
                        buf = malloc(sz + 1);
                        if (!buf) {
                                ret = -ENOMEM;
                                goto out;
                        }
                        testfs_read_data(in, 0, buf, sz);
                        buf[sz] = 0;
                        printf("%s\n", buf);
                        free(buf);
                }
out:
                testfs_put_inode(in);                
        }

        return ret;
}

int
cmd_write(struct super_block *sb, struct context *c)
{
        int inode_nr;
        struct inode *in;
        int size;
        int ret = 0;
        char * filename = c->cmd[1];
        char * content = c->cmd[2];

        if (c->nargs != 3) {
                return -EINVAL;
        }
        inode_nr = testfs_dir_name_to_inode_nr(c->cur_dir, filename);
        if (inode_nr < 0)
                return inode_nr;
        in = testfs_get_inode(sb, inode_nr);
        if (testfs_inode_get_type(in) == I_DIR) {
                ret = -EISDIR;
                goto out;
        }
        size = strlen(content);
        testfs_tx_start(sb, TX_WRITE);
        ret = testfs_write_data(in, 0, content, size);
        if (ret >= 0) {
                testfs_truncate_data(in, size);
        }
        testfs_sync_inode(in);
        testfs_tx_commit(sb, TX_WRITE);
out:
        testfs_put_inode(in);
        return ret;
}

