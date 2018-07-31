#include "super.h"
#include "posixtfs.h"
#include "dir.h"
#include "inode.h"

int main() {
  int r;
  
  char storage[MAX_STORAGE];

  memset(storage, 0, MAX_STORAGE);
  
  struct super_block *sb = testfs_make_super_block(storage);

  testfs_make_inode_freemap(sb);

  testfs_make_block_freemap(sb);

  testfs_make_csum_table(sb);

  testfs_make_inode_blocks(sb);

  testfs_close_super_block(sb);  

  r = testfs_init_super_block(storage, 0, &sb);
  assert(r == 0);
  
  r = testfs_make_root_dir(sb);
  assert(r == 0);

  testfs_close_super_block(sb);

  r = testfs_init_super_block(storage, 0, &sb);
  assert(r == 0);  

  struct inode *root = testfs_get_inode(sb, 0);
  assert (root != NULL);

  tfs_checkfs(sb);
}
