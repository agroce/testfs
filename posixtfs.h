#ifndef _POSIXTFS_H
#define _POSIXTFS_H

int tfs_mkdir(struct super_block *sb, const char *path);
int tfs_open(struct super_block *sb, const char *path, int, ...);
int tfs_close(struct super_block *sb, int fd);
ssize_t tfs_write(struct super_block *sb, int fd, char* data, size_t size);
int tfs_checkfs(struct super_block * sb);
int tfs_ls(struct super_block * sb);
int tfs_lsr(struct super_block * sb);

#endif /* _POSIXTFS_H_ */
