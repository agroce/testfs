#include <deepstate/DeepState.hpp>
#include "RamInterface.h"

using namespace deepstate;

extern "C" {
#include "testfs.h"
#include "super.h"
#include "inode.h"
#include "dir.h"
#include "common.h"
#include "ops.h"
}

char gFsPath[] = {'r', 'a', 'w', '.', 'f', 's', '\0'};

static bool gIsOpen = false;
static constexpr int kFd = 99;
static std::vector<uint8_t> gFileData;
static long gFilePos = 0;

int OpenFile(const char *path, int, ...) {
  ASSERT(!gIsOpen);
  ASSERT(path == gFsPath);
  gIsOpen = true;
  gFilePos = 0;
  // LOG(DEBUG) << "open(" << path << ") = " << kFd;
  return kFd;
}

int CloseFile(int fd) {
  ASSERT(fd == kFd);
  gIsOpen = false;
  // LOG(DEBUG) << "close(" << fd << ")";
  return 0;
}

long SeekFile(int fd, long offset, int whence) {
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

ssize_t WriteFile(int fd, const void *data_, size_t size) {
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

ssize_t ReadFile(int fd, void *data_, size_t size) {
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

void InitFileOperations(void) {
  FOPS.open = OpenFile;
  FOPS.close = CloseFile;
  FOPS.seek = SeekFile;
  FOPS.write = WriteFile;
  FOPS.read = ReadFile;
}

void CreateEmptyFileSystem(void) {
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
