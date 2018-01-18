/*
 * Copyright (c) 2017 Trail of Bits, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <deepstate/DeepState.hpp>

extern "C" {
#include "testfs.h"
#include "super.h"
#include "inode.h"
#include "dir.h"
#include "common.h"
#include "ops.h"
}

using namespace deepstate;

#define LENGTH 3

#define NUM_PATHS 2
#define PATH_LEN 3

#define DATA_LEN 2

#define NUM_FDS 3

static bool gIsOpen = false;
static constexpr int kFd = 99;
static char gFsPath[] = {'r', 'a', 'w', '.', 'f', 's', '\0'}; 
static std::vector<uint8_t> gFileData;
static long gFilePos = 0;

static char DataChar() {
  symbolic_char c;
  ASSUME ((c == 'x') || (c == 'y'));
  return c;
}

static void MakeNewData(char *data) {
  symbolic_unsigned l;
  ASSUME_GT(l, 0);
  ASSUME_LT(l, DATA_LEN+1);
  unsigned i;
  for (i = 0; i < l; i++) {
    data[i] = DataChar();
  }
  data[i] = 0;
}

/*
static char PathChar() {
  symbolic_char c;
  ASSUME ((c == 'a') || (c == 'b') || (c == 'A') || (c == '/'));
  return c;
}
*/

static void MakeNewPath(char *path) {
  symbolic_unsigned l;
  ASSUME_GT(l, 0);
  ASSUME_LT(l, PATH_LEN+1);
  int i, max_i = Pump(l);
  for (i = 0; i < max_i; i++) {
    path[i] = OneOf("aAbB/.");
  }
  path[i] = 0;
}

static int GetPath() {
  symbolic_unsigned path;
  ASSUME_LT(path, NUM_PATHS);
  return path;
}

static int GetFD() {
  symbolic_unsigned fd;
  ASSUME_LT(fd, NUM_FDS);
  return fd;
}

static int OpenFile(const char *path, int, ...) {
  ASSERT(!gIsOpen);
  ASSERT(path == gFsPath);
  gIsOpen = true;
  gFilePos = 0;
  // LOG(DEBUG) << "open(" << path << ") = " << kFd;
  return kFd;
}

static int fs_mkdir(const char *path) {
  return 0;
}

static int fs_open(const char *path, int, ...) {
  return 1;
}

static int CloseFile(int fd) {
  ASSERT(fd == kFd);
  gIsOpen = false;
  // LOG(DEBUG) << "close(" << fd << ")";
  return 0;
}

static int fs_close(int fd) {
  return 0;
}

static long SeekFile(int fd, long offset, int whence) {
  ASSERT(fd == kFd);
  const auto size = static_cast<long>(gFileData.size());
  switch (whence) {
    case SEEK_CUR: {
      auto pos = gFilePos + offset;
      if (0 > pos) {
        errno = EINVAL;
        return -1;
      } else {
        if (pos > size) {
          gFileData.resize(static_cast<size_t>(pos), 0);
        }
        gFilePos = pos;
        // LOG(DEBUG) << "lseek(" << fd << ", " << offset
        //            << ", SEEK_CUR) = " << pos;
        return pos;
      }
    }

    case SEEK_SET:
      if (0 > offset) {
        errno = EINVAL;
        return -1;
      } else {
        if (offset > size) {
          gFileData.resize(static_cast<size_t>(offset), 0);
        }
        gFilePos = offset;
        // LOG(DEBUG) << "lseek(" << fd << ", " << offset << ", SEEK_SET) = " << offset;
        return offset;
      }

    case SEEK_END: {
      auto pos = size + offset;
      if (0 > pos) {
        errno = EINVAL;
        return -1;
      } else {
        if (pos > size) {
          gFileData.resize(static_cast<size_t>(pos), 0);
        }
        gFilePos = pos;
        // LOG(DEBUG) << "lseek(" << fd << ", " << offset << ", SEEK_END) = " << pos;
        return pos;
      }
    }
    default:
      errno = EINVAL;
      return -1;
  }
}

static ssize_t WriteFile(int fd, const void *data_, size_t size) {
  ASSERT(fd == kFd);
  ASSERT(nullptr != data_);
  if (!size) {
    return 0;
  }

  auto data = reinterpret_cast<const uint8_t *>(data_);
  gFileData.reserve(gFileData.size() + size);
  auto pos_after_write = gFilePos + size;
  if (pos_after_write > gFileData.size()) {
    gFileData.resize(pos_after_write, 0);
  }

  // LOG(DEBUG) << "write(" << fd << ", " << data_ << ", "
  //            << size << ") @ " << gFilePos;

  for (auto i = 0UL; i < size; ++i) {
    gFileData[gFilePos++] = data[i];
  }

  ASSERT(gFilePos == static_cast<long>(pos_after_write));
  errno = 0;
  return static_cast<ssize_t>(size);
}

ssize_t fs_write(int fs, const void *data, size_t size) {
  return 0;
}

static ssize_t ReadFile(int fd, void *data_, size_t size) {
  ASSERT(fd == kFd);
  ASSERT(nullptr != data_);
  if (!size) {
    return 0;
  }


  // LOG(DEBUG) << "read(" << fd << ", " << data_ << ", "
  //            << size << ") @ " << gFilePos;

  auto data = reinterpret_cast<uint8_t *>(data_);

  ssize_t num_read = 0;
  while (gFilePos < static_cast<ssize_t>(gFileData.size()) &&
         num_read < static_cast<ssize_t>(size)) {
    data[num_read++] = gFileData[gFilePos++];
  }

  errno = 0;
  return num_read;
}

static void InitFileOperations(void) {
  FOPS.open = OpenFile;
  FOPS.close = CloseFile;
  FOPS.seek = SeekFile;
  FOPS.write = WriteFile;
  FOPS.read = ReadFile;
}

static void CreateEmptyFileSystem(void) {
  return;
  LOG(INFO) << "Making super block";
  auto sb = testfs_make_super_block(gFsPath);

  LOG(INFO) << "Making inode free map";
  testfs_make_inode_freemap(sb);

  LOG(INFO) << "Making block free map";
  testfs_make_block_freemap(sb);

  LOG(INFO) << "Making checksum table";
  testfs_make_csum_table(sb);

  LOG(INFO) << "Making inode blocks";
  testfs_make_inode_blocks(sb);

  testfs_close_super_block(sb);
  LOG(INFO) << "Done creating empty file system.";

  ASSERT(!testfs_init_super_block(gFsPath, 0, &sb))
      << "Couldn't initialize super block";

  ASSERT(!testfs_make_root_dir(sb))
      << "Couldn't create root directory.";

  testfs_close_super_block(sb);
  LOG(INFO)
      << "Created root directory; File system initialized";
}

TEST(TestFs, FilesDirs) {
  InitFileOperations();
  CreateEmptyFileSystem();

  /*
  struct super_block *sb = nullptr;
  int ret = testfs_init_super_block(gFsPath, 0, &sb);
  ASSERT(!ret && sb != nullptr)
      << "Couldn't initialize super block";
  */

  char paths[NUM_PATHS][PATH_LEN+1] = {};
  bool used[NUM_PATHS] = {};
  char data[DATA_LEN+1] = {};
  int fds[NUM_FDS] = {};
  int fd, path = -1;
  for (int i = 0; i < NUM_FDS; i++) {
    fds[i] = -1;
  }

  for (int n = 0; n < LENGTH; n++) {
    OneOf(
      [n, &path, &paths, &used] {
        path = GetPath();
        ASSUME(!used[path]);
        MakeNewPath(paths[path]);
        printf("%d: paths[%d] = %s", 
               n, path, paths[path]);
        used[path] = true;
      },
      [n, &path, &paths, &used] {
        path = GetPath();
        ASSUME(used[path]);
        ASSUME_GT(strlen(paths[path]), 0);
        printf("%d: Mkdir(%s)",
               n, paths[path]);
        fs_mkdir(paths[path]);
        used[path] = false;
      },
      [n, &fd, &fds, &path, &paths, &used] {
        fd = GetFD();
        path = GetPath();
        ASSUME(used[path]);
        ASSUME_EQ(fds[fd], -1);
        ASSUME_GT(strlen(paths[path]), 0);
        printf("%d: fds[%d] = open(%s)", 
               n, fd, paths[path]);
        fds[fd] = fs_open(paths[path], 
                          O_CREAT|O_TRUNC);
        used[path] = false;
      },
      [n, &fd, &fds, &data] {
        MakeNewData(data);
        fd = GetFD();
        ASSUME_NE(fds[fd], -1);
        printf("%d: write(fds[%d],\"%s\")", 
               n, fd, data);
        fs_write(fds[fd], data, strlen(data));
      },
      [n, &fd, &fds] {
        ASSUME_NE(fds[fd], -1);
        printf("%d: close(fds[%d])", n, fd);
        fs_close(fds[fd]);
        fds[fd] = -1;
      });
  }

  //testfs_close_super_block(sb);
}

int main(int argc, char *argv[]) {
  DeepState_InitOptions(argc, argv);
  return DeepState_Run();
}
