#include "testfs.h"
#include "super.h"
#include "inode.h"
#include "dir.h"
#include "common.h"

static void
usage(char *progname)
{
        fprintf(stdout, "Usage: %s rawfile\n", progname);
        exit(1);
}

int
main(int argc, char *argv[])
{
        struct super_block *sb;
        int ret;

        if (argc != 2) {
                usage(argv[0]);
        }
		
        sb = testfs_make_super_block(argv[1]);
        testfs_make_inode_freemap(sb);
        testfs_make_block_freemap(sb);
        testfs_make_csum_table(sb);
        testfs_make_inode_blocks(sb);
        testfs_close_super_block(sb);

        ret = testfs_init_super_block(argv[1], 0, &sb);
        if (ret) {
                EXIT("testfs_init_super_block");
        }
        testfs_make_root_dir(sb);
        testfs_close_super_block(sb);
        return 0;
}
