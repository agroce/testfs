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

#include <fcntl.h>

#include <deepstate/DeepState.hpp>

using namespace deepstate;

extern "C" {
#include "super.h"
#include "posixtfs.h"
#include "dir.h"
}

#define LENGTH 1
#define NUM_PATHS 2
#define PATH_LEN 2
#define DATA_LEN 2
#define NUM_FDS 1

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

static int GetFD() {
  symbolic_unsigned fd;
  ASSUME_LT(fd, NUM_FDS);
  return fd;
}

TEST(TestFs, FilesDirs) {
  char storage[MAX_STORAGE];

  memset(storage, 0, MAX_STORAGE);
  
  struct super_block *sb = testfs_make_super_block(storage);
  ASSERT(sb != nullptr)
      << "Couldn't initialize super block";

  LOG(INFO) << "Making inode free map";
  testfs_make_inode_freemap(sb);

  LOG(INFO) << "Making block free map";
  testfs_make_block_freemap(sb);

  LOG(INFO) << "Making checksum table";
  testfs_make_csum_table(sb);

  LOG(INFO) << "Making inode blocks";
  testfs_make_inode_blocks(sb);

  testfs_close_super_block(sb);  

  ASSERT(!testfs_init_super_block(storage, 0, &sb))
    << "Couldn't initialize super block";

  ASSERT(!testfs_make_root_dir(sb))
      << "Couldn't create root directory.";

  testfs_close_super_block(sb);
  LOG(INFO)
      << "Created root directory; File system initialized";

  ASSERT(!testfs_init_super_block(storage, 0, &sb))
    << "Couldn't initialize super block";

  struct inode *root = testfs_get_inode(sb, 0);
  ASSERT(root != NULL) << "Root is null!";

  LOG(INFO) << "Checking the initial file system...";  
  tfs_checkfs(sb);

  char path[PATH_LEN+1];
  char data[DATA_LEN+1] = {};
  int fds[NUM_FDS] = {};
  int fd;
  for (int i = 0; i < NUM_FDS; i++) {
    fds[i] = -1;
  }

  for (int n = 0; n < LENGTH; n++) {
    OneOf(
      [n, sb, &path] {
        MakeNewPath(path);
        printf("STEP %d: tfs_mkdir(sb, \"%s\");",
               n, path);
        tfs_mkdir(sb, path);
      },
      [n, sb] {
        printf("STEP %d: tfs_ls(sb);");
        tfs_ls(sb);
      },
      [n, sb] {
        printf("STEP %d: tfs_lsr(sb);");
        tfs_lsr(sb);
      },            
      [n, sb, &fd, &fds, &path] {
        fd = GetFD();
	MakeNewPath(path);
        ASSUME_EQ(fds[fd], -1);
        printf("STEP %d: fds[%d] = open(sb, \"%s\", O_CREAT|O_TRUNC);", 
               n, fd, path);
        fds[fd] = tfs_open(sb, path, O_CREAT|O_TRUNC);
      },
      [n, sb, &fd, &fds, &data] {
        MakeNewData(data);
        fd = GetFD();
        ASSUME_NE(fds[fd], -1);
        printf("STEP %d: write(sb, fds[%d],\"%s\");", 
               n, fd, data);
        tfs_write(sb, fds[fd], data, strlen(data));
      },
      [n, sb, &fd, &fds] {
	fd = GetFD();
        ASSUME_NE(fds[fd], -1);
        printf("STEP %d: close(sb, fds[%d]);", n, fd);
        tfs_close(sb, fds[fd]);
        fds[fd] = -1;
      });
    
    LOG(INFO) << "Checking the file system...";
    tfs_checkfs(sb);
  }

  testfs_close_super_block(sb);
}

#ifndef LIBFUZZER
int main(int argc, char *argv[]) {
  DeepState_InitOptions(argc, argv);
  return DeepState_Run();
}
#endif
