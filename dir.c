#include "testfs.h"
#include "super.h"
#include "inode.h"
#include "block.h"
#include "dir.h"
#include "tx.h"

#include "fslice.h"

/* reads next dirent, updates offset to next dirent in directory */
/* allocates memory, caller should free */
struct dirent *
testfs_next_dirent(struct inode *dir, int *offset)
{
        int ret;
        struct dirent d, *dp;

        assert(dir);
        assert(testfs_inode_get_type(dir) == I_DIR);
        if (*offset >= testfs_inode_get_size(dir))
                return NULL;
        ret = testfs_read_data(dir, *offset, (char *)&d, sizeof(struct dirent));
        if (ret < 0)
                return NULL;
        assert(d.d_name_len > 0);
        dp = malloc(sizeof(struct dirent) + d.d_name_len);
        if (!dp)
                return NULL;
        *dp = d;
        *offset += sizeof(struct dirent);
        ret = testfs_read_data(dir, *offset, D_NAME(dp), d.d_name_len);
        if (ret < 0) {
                free(dp);
                return NULL;
        }
        *offset += d.d_name_len;
        return dp;
}

/* returns dirent associated with inode_nr in dir.
 * returns NULL on error.
 * allocates memory, caller should free. */
static struct dirent *
testfs_find_dirent(struct inode *dir, int inode_nr)
{
        struct dirent *d;
        int offset = 0;

        assert(dir);
        assert(testfs_inode_get_type(dir) == I_DIR);
        assert(inode_nr >= 0);
        for (; (d = testfs_next_dirent(dir, &offset)); free(d)) {
                if (d->d_inode_nr == inode_nr)
                        return d;
        }
        return NULL;
}

/* return 0 on success.
 * return negative value on error. */
static int
testfs_write_dirent(struct inode *dir, char *name, int len, int inode_nr,
                    int offset)
{
        int ret;
        struct dirent *d = malloc(sizeof(struct dirent) + len);
        
        if (!d)
                return -ENOMEM;
        assert(inode_nr >= 0);
        d->d_name_len = len;
        d->d_inode_nr = inode_nr;
        strcpy(D_NAME(d), name);
        ret = testfs_write_data(dir, offset, (char *)d, 
                                sizeof(struct dirent) + len);
        free(d);
        return ret;
}

/* return 0 on success.
 * return negative value on error. */
static int
testfs_add_dirent(struct inode *dir, char *name, int inode_nr)
{
        struct dirent *d;
        int p_offset = 0, offset = 0;
        int found = 0;
        int ret = 0;
        int len = strlen(name) + 1;
        fslice_name(name, len);

        assert(dir);
        assert(testfs_inode_get_type(dir) == I_DIR);
        assert(name);
        for (; ret == 0 && found == 0; free(d)) {
                p_offset = offset;
                if ((d = testfs_next_dirent(dir, &offset)) == NULL)
                        break;
                if ((d->d_inode_nr >= 0) && (strcmp(D_NAME(d), name) == 0)) {
                        ret = -EEXIST;
                        continue;
                }
                if ((d->d_inode_nr >= 0) || (d->d_name_len != len))
                        continue;
                found = 1;
        }
        if (ret < 0)
                return ret;
        assert(found || (p_offset == testfs_inode_get_size(dir)));
        return testfs_write_dirent(dir, name, len, inode_nr, p_offset);
}


/* returns negative value if name within dir is not empty */
static int
testfs_remove_dirent_allowed(struct super_block *sb, int inode_nr)
{
        struct inode *dir;
        int offset = 0;
        struct dirent *d;
        int ret = 0;

        dir = testfs_get_inode(sb, inode_nr);
        if (testfs_inode_get_type(dir) != I_DIR)
                goto out;
        for (; ret == 0 && (d = testfs_next_dirent(dir, &offset)); free(d)) {
                if ((d->d_inode_nr < 0) || (strcmp(D_NAME(d), ".") == 0) || 
                    (strcmp(D_NAME(d), "..") == 0))
                        continue;
                ret = -ENOTEMPTY;
        }
out:
        testfs_put_inode(dir);
        return ret;
}

/* returns inode_nr of dirent removed.
   returns negative value if name is not found */
static int
testfs_remove_dirent(struct super_block *sb, struct inode *dir, char *name)
{
        struct dirent *d;
        int p_offset, offset = 0;
        int inode_nr = -1;
        int ret = -ENOENT;

        assert(dir);
        assert(name);
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                return -EINVAL;
        }
        for (; inode_nr == -1; free(d)) {
                p_offset = offset;
                if ((d = testfs_next_dirent(dir, &offset)) == NULL)
                        break;
                //fslice_name(D_NAME(d), d->d_name_len);
                if ((d->d_inode_nr < 0) || (strcmp(D_NAME(d), name) != 0))
                        continue;
                /* found the dirent */
                inode_nr = d->d_inode_nr;
                if ((ret = testfs_remove_dirent_allowed(sb, inode_nr)) < 0)
                        continue; /* this will break out of the loop */
                d->d_inode_nr = -1;
                ret = testfs_write_data(dir, p_offset, (char *)d, 
                                        sizeof(struct dirent) + d->d_name_len);
                if (ret >= 0)
                        ret = inode_nr;
        }
        return ret;
}

static int
testfs_create_empty_dir(struct super_block *sb, int p_inode_nr,
                        struct inode *cdir)
{
        int ret;

        assert(testfs_inode_get_type(cdir) == I_DIR);
        ret = testfs_add_dirent(cdir, ".", testfs_inode_get_nr(cdir));
        if (ret < 0)
                return ret;
        ret = testfs_add_dirent(cdir, "..", p_inode_nr);
        if (ret < 0) {
                testfs_remove_dirent(sb, cdir, ".");
                return ret;
        }
        return 0;
}

static int
testfs_create_file_or_dir(struct super_block *sb, struct inode *dir,
                          inode_type type, char *name)
{
        int ret = 0;
        struct inode *in;
        int inode_nr;

        if (dir) {
                inode_nr = testfs_dir_name_to_inode_nr(dir, name);
                if (inode_nr >= 0)
                        return -EEXIST;
        }
        testfs_tx_start(sb, TX_CREATE);
        /* first create inode */
        ret = testfs_create_inode(sb, type, &in);
        if (ret < 0) {
                goto fail;
        }
        inode_nr = testfs_inode_get_nr(in);
        if (type == I_DIR) { /* create directory */
                int p_inode_nr = dir ? testfs_inode_get_nr(dir) : inode_nr;
                ret = testfs_create_empty_dir(sb, p_inode_nr, in);
                if (ret < 0)
                        goto out;
        }
        /* then add directory entry */
        if (dir) {
                if ((ret = testfs_add_dirent(dir, name, inode_nr)) < 0)
                        goto out;
                testfs_sync_inode(dir);
        }
        testfs_sync_inode(in);
        testfs_put_inode(in);
        testfs_tx_commit(sb, TX_CREATE);
        return 0;
out:
        testfs_remove_inode(in);
fail:
        testfs_tx_commit(sb, TX_CREATE);
        return ret;
}

static int
testfs_pwd(struct super_block *sb, struct inode *in)
{
        int p_inode_nr;
        struct inode *p_in;
        struct dirent *d;
        int ret;

        assert(in);
        assert(testfs_inode_get_nr(in) >= 0);
        p_inode_nr = testfs_dir_name_to_inode_nr(in, "..");
        assert(p_inode_nr >= 0);
        if (p_inode_nr == testfs_inode_get_nr(in)) {
                printf("/");
                return 1;
        }
        p_in = testfs_get_inode(sb, p_inode_nr);
        d = testfs_find_dirent(p_in, testfs_inode_get_nr(in));
        assert(d);
        ret = testfs_pwd(sb, p_in);
        testfs_put_inode(p_in);
        printf("%s%s", ret == 1 ? "" : "/", D_NAME(d));
        free(d);
        return 0;
}

/* returns negative value if name is not found */
int
testfs_dir_name_to_inode_nr(struct inode *dir, char *name)
{
        struct dirent *d;
        int offset = 0;
        int ret = -ENOENT;

        assert(dir);
        assert(name);
        assert(testfs_inode_get_type(dir) == I_DIR);
        for (; ret < 0 && (d = testfs_next_dirent(dir, &offset)); free(d)) {
                //fslice_name(D_NAME(d), d->d_name_len);
                if ((d->d_inode_nr < 0) || (strcmp(D_NAME(d), name) != 0))
                        continue;
                ret = d->d_inode_nr;
        }
        return ret;
}

int
testfs_make_root_dir(struct super_block *sb)
{
        return testfs_create_file_or_dir(sb, NULL, I_DIR, NULL);
}

int
cmd_cd(struct super_block *sb, struct context *c)
{
        int inode_nr;
        struct inode *dir_inode;

        if (c->nargs != 2) {
                return -EINVAL;
        }
        inode_nr = testfs_dir_name_to_inode_nr(c->cur_dir, c->cmd[1]);
        if (inode_nr < 0)
                return inode_nr;
        dir_inode = testfs_get_inode(sb, inode_nr);
        if (testfs_inode_get_type(dir_inode) != I_DIR) {
                testfs_put_inode(dir_inode);
                return -ENOTDIR;
        }
        testfs_put_inode(c->cur_dir);
        c->cur_dir = dir_inode;
        return 0;
}

int
cmd_pwd(struct super_block *sb, struct context *c)
{
        if (c->nargs != 1) {
                return -EINVAL;
        }
        testfs_pwd(sb, c->cur_dir);
        printf("\n");
        return 0;
}

static int
testfs_ls(struct inode *in, int recursive)
{
        int offset = 0;
        struct dirent *d;

        for (; (d = testfs_next_dirent(in, &offset)); free(d)) {
                struct inode *cin;

                if (d->d_inode_nr < 0)
                        continue;
                cin = testfs_get_inode(testfs_inode_get_sb(in), d->d_inode_nr);
                printf("%s%s\n", D_NAME(d), 
                       (testfs_inode_get_type(cin) == I_DIR) ? "/":"");
                if (recursive && testfs_inode_get_type(cin) == I_DIR &&
                    (strcmp(D_NAME(d), ".") != 0) && 
                    (strcmp(D_NAME(d), "..") != 0)) {
                        testfs_ls(cin, recursive);
                }
                testfs_put_inode(cin);
        }
        return 0;
}

int
cmd_ls(struct super_block *sb, struct context *c)
{
        int inode_nr;
        struct inode *in;
        char *cdir = ".";

        if (c->nargs != 1 && c->nargs != 2) {
                return -EINVAL;
        }
        if (c->nargs == 2) {
                cdir = c->cmd[1];
        }
        assert(c->cur_dir);
        inode_nr = testfs_dir_name_to_inode_nr(c->cur_dir, cdir);
        if (inode_nr < 0)
                return inode_nr;
        in = testfs_get_inode(sb, inode_nr);
        testfs_ls(in, 0);
        testfs_put_inode(in);
        return 0;
}

int
cmd_lsr(struct super_block *sb, struct context *c)
{
        int inode_nr;
        struct inode *in;
        char *cdir = ".";

        if (c->nargs != 1 && c->nargs != 2) {
                return -EINVAL;
        }
        if (c->nargs == 2) {
                cdir = c->cmd[1];
        }
        assert(c->cur_dir);
        inode_nr = testfs_dir_name_to_inode_nr(c->cur_dir, cdir);
        if (inode_nr < 0)
                return inode_nr;
        in = testfs_get_inode(sb, inode_nr);
        testfs_ls(in, 1);
        testfs_put_inode(in);
        return 0;
}

int
cmd_create(struct super_block *sb, struct context *c)
{
        if (c->nargs != 2) {
                return -EINVAL;
        }
        return testfs_create_file_or_dir(sb, c->cur_dir, I_FILE, c->cmd[1]);
}

int
cmd_stat(struct super_block *sb, struct context *c)
{
        int inode_nr;
        struct inode *in;
        int i;

        if (c->nargs < 2) {
                return -EINVAL;
        }   
        for (i = 1; i < c->nargs; i++ )
        {
                inode_nr = testfs_dir_name_to_inode_nr(c->cur_dir, c->cmd[i]);
                if (inode_nr < 0)
                        return inode_nr;
                in = testfs_get_inode(sb, inode_nr);
                printf("%s: i_nr = %d, i_type = %d, i_size = %d\n", c->cmd[i], 
                       testfs_inode_get_nr(in), testfs_inode_get_type(in), 
                       testfs_inode_get_size(in));
                testfs_put_inode(in);
        }
        return 0;
}

int
cmd_rm(struct super_block *sb, struct context *c)
{
        int inode_nr;
        struct inode *in;

        if (c->nargs != 2) {
                return -EINVAL;
        }
        testfs_tx_start(sb, TX_RM);
        inode_nr = testfs_remove_dirent(sb, c->cur_dir, c->cmd[1]);
        if (inode_nr < 0) {
                testfs_tx_commit(sb, TX_RM);
                return inode_nr;
        }
        in = testfs_get_inode(sb, inode_nr);
        testfs_remove_inode(in);
        testfs_sync_inode(c->cur_dir);
        testfs_tx_commit(sb, TX_RM);
        return 0;
}

int
cmd_mkdir(struct super_block *sb, struct context *c)
{
        if (c->nargs != 2) {
                return -EINVAL;
        }
        return testfs_create_file_or_dir(sb, c->cur_dir, I_DIR, c->cmd[1]);
}
