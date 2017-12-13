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

static constexpr int kFd = 99;
static char gFsPath[] = {'r', 'a', 'w', '.', 'f', 's', '\0'}; 
static std::vector<char> gFileData;
static long gFilePos = 0;

static int OpenFile(const char *path, int, ...) {
  ASSERT(path == gFsPath);
  return kFd;
}

static int CloseFile(int fd) {
  ASSERT(fd == kFd);
  return 0;
}

static long SeekFile(int fd, long offset, int whence) {
  ASSERT(fd == kFd);
  const auto size = static_cast<long>(gFileData.size());
  switch (whence) {
    case SEEK_CUR: {
      auto pos = gFilePos + offset;
      if (0 > pos || pos > size) {
        errno = EINVAL;
        return -1;
      } else {
        gFilePos = pos;
        return pos;
      }
    }

    case SEEK_SET:
      if (0 > offset || offset > size) {
        errno = EINVAL;
        return -1;
      } else {
        gFilePos = offset;
        return offset;
      }

    case SEEK_END: {
      auto pos = size - offset;
      if (0 > pos || pos > size) {
        errno = EINVAL;
        return -1;
      } else {
        gFilePos = pos;
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

  auto data = reinterpret_cast<const char *>(data_);
  gFileData.reserve(gFileData.size() + size);
  auto pos_after_write = (gFileData.size() - gFilePos) + size;
  if (pos_after_write > gFileData.size()) {
    gFileData.resize(pos_after_write);
  }
  for (auto i = 0UL; i < size; ++i) {
    gFileData[gFilePos++] = data[i];
  }
  ASSERT(gFilePos == static_cast<long>(pos_after_write));
  return static_cast<long>(size);
}

static long ReadFile(int fd, void *data_, unsigned long size) {
  ASSERT(fd == kFd);
  ASSERT(nullptr != data_);
  if (!size) {
    return 0;
  }

  auto data = reinterpret_cast<char *>(data_);

  long num_read = 0;
  while (gFilePos < static_cast<long>(gFileData.size())) {
    data[num_read++] = gFileData[gFilePos++];
  }
  return num_read;
}

TEST(TestFs, Initialize) {
  FOPS.open = OpenFile;
  FOPS.close = CloseFile;
  FOPS.seek = SeekFile;
  FOPS.write = WriteFile;
  FOPS.read = ReadFile;

  struct super_block *sb;
  int ret;
  
  sb = testfs_make_super_block(gFsPath);
  testfs_make_inode_freemap(sb);
  testfs_make_block_freemap(sb);
  testfs_make_csum_table(sb);
  testfs_make_inode_blocks(sb);
  testfs_close_super_block(sb);
  
  ret = testfs_init_super_block(gFsPath, 0, &sb);
  ASSERT(!ret)
      << "Couldn't initialize super block";

  ret = testfs_make_root_dir(sb);
  ASSERT(!ret)
      << "Couldn't create root directory.";

  testfs_close_super_block(sb);
}

int main(int argc, char *argv[]) {
  return DeepState_Run();
}
