#ifndef _POSIXTFS_H
#define _POSIXTFS_H

int tfs_mkdir(struct super_block *sb, const char *path);
int tfs_rmdir(struct super_block *sb, const char *path);
int tfs_unlink(struct super_block *sb, const char *path);
int tfs_create(struct super_block *sb, const char *path);
int tfs_write(struct super_block *sb, const char* path, char* data);
int tfs_checkfs(struct super_block *sb);
int tfs_ls(struct super_block *sb);
int tfs_lsr(struct super_block *sb);
int tfs_stat(struct super_block *sb, const char *path);
int tfs_cat(struct super_block *sb, const char *path);
#endif /* _POSIXTFS_H_ */
