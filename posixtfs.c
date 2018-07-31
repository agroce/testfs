#include <libgen.h>
#include "super.h"
#include "testfs.h"
#include "inode.h"
#include "dir.h"
#include "posixtfs.h"

int tfs_mkdir(struct super_block *sb, const char *path) {
  struct context c;
  char *cpath = malloc((strlen(path)+1)*sizeof(char));
  char **components = malloc(strlen(path)*sizeof(char *));
  int lpos = 0;
  strcpy(cpath, path);
  char *dir = dirname(cpath);
  components[lpos] = malloc((strlen(path)+1)*sizeof(char));  
  strcpy(components[lpos], basename(cpath));
  //printf("PATH %d: %s\n", lpos, components[lpos]);  
  lpos++;
  while (!(strcmp(dir,"/") == 0) && !(strcmp(dir,".") == 0)) {
    components[lpos] = malloc((strlen(path)+1)*sizeof(char));
    strcpy(components[lpos], basename(cpath));
    //printf("PATH %d: %s\n", lpos, components[lpos]);
    lpos++;    
    cpath = dirname(cpath);
    dir = basename(cpath);
  }
  c.cur_dir = testfs_get_inode(sb, 0);
  for (int pos = lpos-1; pos > 1; pos--) {
    //printf("%d: %s\n", lpos, components[pos]);
    c.cur_dir = testfs_get_inode(sb, testfs_dir_name_to_inode_nr(c.cur_dir, components[pos]));
  }
  c.cmd[1] = components[0];
  //printf("NAME: %s\n", c.cmd[1]);
  c.nargs = 2;
  return cmd_mkdir(sb, &c);  
  return 0;
}

int tfs_open(struct super_block *sb, const char *path, int size, ...) {
  return 1;
}

int tfs_close(struct super_block *sb, int fd) {
  return 0;
}

ssize_t tfs_write(struct super_block *sb, int fd, char *data, size_t size) {
  return 0;
}

int tfs_checkfs(struct super_block * sb) {
  struct context c;
  c.nargs = 1;
  return cmd_checkfs(sb, &c);
}

int tfs_ls(struct super_block * sb) {
  struct context c;
  c.nargs = 1;
  c.cur_dir = testfs_get_inode(sb, 0);  
  return cmd_ls(sb, c);
}

int tfs_lsr(struct super_block * sb) {
  struct context c;
  c.nargs = 1;
  c.cur_dir = testfs_get_inode(sb, 0);  
  return cmd_lsr(sb, c);
}
