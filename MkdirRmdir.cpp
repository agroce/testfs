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

using namespace deepstate;

extern "C" {
#include "super.h"
#include "posixtfs.h"
#include "dir.h"
#include "inode.h"
}

#define PATH_LEN 1

static void MakeNewPath(char *path) {
  symbolic_unsigned l;
  ASSUME_GT(l, 0);
  ASSUME_LT(l, PATH_LEN+1);
  int i, max_i = Pump(l);
  for (i = 0; i < max_i; i++) {
    path[i] = OneOf("ab/");
  }
  path[i] = 0;
}

TEST(TestFs, MkdirRmdir) {
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

  MakeNewPath(path);
  tfs_mkdir(sb, path);

  MakeNewPath(path);
  tfs_rmdir(sb, path);

  testfs_close_super_block(sb);
}

#ifndef LIBFUZZER
int main(int argc, char *argv[]) {
  DeepState_InitOptions(argc, argv);
  return DeepState_Run();
}
#endif
