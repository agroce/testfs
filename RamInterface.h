#ifndef _RAMINTERFACE_H
#define _RAMINTERFACE_H

extern char gFsPath[];

int OpenFile(const char *path, int, ...);
int CloseFile(int fd);
long SeekFile(int fd, long offset, int whence);
ssize_t WriteFile(int fd, const void *data_, size_t size);
ssize_t ReadFile(int fd, void *data_, size_t size);
void InitFileOperations(void);
void CreateEmptyFileSystem(void);

#endif /* _RAMINTERFACE_H_ */
