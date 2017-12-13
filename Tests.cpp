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

#define LENGTH 4

static bool gIsOpen = false;
static constexpr int kFd = 99;
static char gFsPath[] = {'r', 'a', 'w', '.', 'f', 's', '\0'}; 
static std::vector<uint8_t> gFileData;
static long gFilePos = 0;

static int OpenFile(const char *path, int, ...) {
  ASSERT(!gIsOpen);
  ASSERT(path == gFsPath);
  gIsOpen = true;
  gFilePos = 0;
  // LOG(DEBUG) << "open(" << path << ") = " << kFd;
  return kFd;
}

static int CloseFile(int fd) {
  ASSERT(fd == kFd);
  gIsOpen = false;
  // LOG(DEBUG) << "close(" << fd << ")";
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

static long WriteFile(int fd, const void *data_, unsigned long size) {
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
  return static_cast<long>(size);
}

static long ReadFile(int fd, void *data_, unsigned long size) {
  ASSERT(fd == kFd);
  ASSERT(nullptr != data_);
  if (!size) {
    return 0;
  }


  // LOG(DEBUG) << "read(" << fd << ", " << data_ << ", "
  //            << size << ") @ " << gFilePos;

  auto data = reinterpret_cast<uint8_t *>(data_);

  long num_read = 0;
  while (gFilePos < static_cast<long>(gFileData.size()) &&
         num_read < static_cast<long>(size)) {
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

TEST(TestFs, Initialize) {
  InitFileOperations();
  CreateEmptyFileSystem();

  struct super_block *sb = nullptr;
  int ret = testfs_init_super_block(gFsPath, 0, &sb);
  ASSERT(!ret && sb != nullptr)
      << "Couldn't initialize super block";

  LOG(INFO)
      << "Getting inode for root directory";
  struct inode *root_dir_inode = testfs_get_inode(sb, 0); /* root dir */

  struct context context = {};

  auto dir_name = DeepState_CStr(3);
  //char dir_name[] = {'h', 'i', '\0'};
  LOG(INFO)
      << "Creating directory /" << dir_name;
  context.cur_dir = root_dir_inode;
  context.cmd[1] = dir_name;
  context.nargs = 2;
  ASSERT(!cmd_mkdir(sb, &context))
      << "Could not create directory /" << dir_name;

  // This will do a `printf`.
  context.cur_dir = root_dir_inode;
  context.cmd[1] = dir_name;
  context.nargs = 2;
  ASSERT(!cmd_stat(sb, &context))
      << "Could not state directory /" << dir_name;

  testfs_put_inode(root_dir_inode);
  testfs_close_super_block(sb);
}

int main(int argc, char *argv[]) {
  return DeepState_Run();
}
