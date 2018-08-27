#include <libgen.h>
#include "super.h"
#include "testfs.h"
#include "inode.h"
#include "dir.h"
#include "posixtfs.h"

int put_context_at_dir(struct super_block *sb, const char *path, struct context *c) {
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
  c->cur_dir = testfs_get_inode(sb, 0);
  for (int pos = lpos-1; pos > 1; pos--) {
    //printf("%d: %s\n", lpos, components[pos]);
    int nr = testfs_dir_name_to_inode_nr(c->cur_dir, components[pos]);
    free(components[pos]);
    if (nr < 0) {
      return -1;
    }
    c->cur_dir = testfs_get_inode(sb, nr);
  }
  char *p = malloc(strlen(components[0])+1*sizeof(char));
  strcpy(p, components[0]);
  free(cpath);
  free(components);
  c->cmd[1] = p;
  printf("SET CONTEXT\n");
  return 0;
}

int tfs_mkdir(struct super_block *sb, const char *path) {
  struct context c;
  int r;
  if (put_context_at_dir(sb, path, &c) < 0) {
    return -1;
  }
  c.nargs = 2;
  r = cmd_mkdir(sb, &c);
  printf("COMPLETED MKDIR\n");
  free(c.cmd[1]);
  return r;
}

int tfs_rmdir(struct super_block *sb, const char *path) {
  struct context c;
  int r;
  if (put_context_at_dir(sb, path, &c) < 0) {
    return -1;
  }
  c.nargs = 2;
  r = cmd_rm(sb, &c);
  free(c.cmd[1]);
  return r;
}

int tfs_unlink(struct super_block *sb, const char *path) {
  struct context c;
  int r;
  if (put_context_at_dir(sb, path, &c) < 0) {
    return -1;
  }
  c.nargs = 2;
  r = cmd_rm(sb, &c);
  free(c.cmd[1]);
  return r;  
}

int tfs_create(struct super_block *sb, const char *path) {
  struct context c;
  int r;
  if (put_context_at_dir(sb, path, &c) < 0) {
    return -1;
  }
  c.nargs = 2;
  r = cmd_create(sb, &c);
  free(c.cmd[1]);
  return r;
}

int tfs_write(struct super_block *sb, const char *path, char *data) {
  struct context c;
  int r;
  if (put_context_at_dir(sb, path, &c) < 0) {
    return -1;
  }
  c.cmd[2] = data;
  c.nargs = 3;	  
  r = cmd_write(sb, &c);
  free(c.cmd[1]);
  return r;
}

int tfs_stat(struct super_block *sb, const char *path) {
  struct context c;
  int r;
  if (put_context_at_dir(sb, path, &c) < 0) {
    return -1;
  }
  c.nargs = 2;
  r = cmd_stat(sb, &c);
  free(c.cmd[1]);  
  return r;
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
  return cmd_ls(sb, &c);
}

int tfs_lsr(struct super_block * sb) {
  struct context c;
  c.nargs = 1;
  c.cur_dir = testfs_get_inode(sb, 0);  
  return cmd_lsr(sb, &c);
}

int tfs_open(struct super_block *sb, const char *path, int size, ...) {
  return 1;
}

int tfs_close(struct super_block *sb, int fd) {
  return 0;
}
