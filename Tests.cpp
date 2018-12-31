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
#include "block.h"
}

#define LENGTH 20
#define PATH_LEN 10
#define DATA_LEN 2

#define MAX_RESET 5

static void MakeNewData(char *data) {
  unsigned l = DeepState_UIntInRange(1, DATA_LEN);
  /* We need to use AssignString since we don't want null strings here. */
  DeepState_AssignCStr(data, Pump(l, DATA_LEN-1), "xy", 2);
}

static void MakeNewPath(char *path) {
  DeepState_AssignCStrMax(path, PATH_LEN, "aAbB/.", 6);
}

TEST(TestFs, FilesDirs) {
  char storage[MAX_STORAGE];

  memset(storage, 0, MAX_STORAGE);

  set_reset_countdown(-1);
  
  struct super_block *sb = testfs_make_super_block(storage);
  ASSERT(sb != nullptr)
    << "Couldn't initialize super block";

  LOG(TRACE) << "Making inode free map";
  testfs_make_inode_freemap(sb);

  LOG(TRACE) << "Making block free map";
  testfs_make_block_freemap(sb);

  LOG(TRACE) << "Making checksum table";
  testfs_make_csum_table(sb);

  LOG(TRACE) << "Making inode blocks";
  testfs_make_inode_blocks(sb);

  testfs_close_super_block(sb);  

  ASSERT(!testfs_init_super_block(storage, 0, &sb))
    << "Couldn't initialize super block";

  ASSERT(!testfs_make_root_dir(sb))
    << "Couldn't create root directory.";

  testfs_close_super_block(sb);
  LOG(TRACE) << "Created root directory; File system initialized";

  ASSERT(!testfs_init_super_block(storage, 0, &sb))
    << "Couldn't initialize super block";

  struct inode *root = testfs_get_inode(sb, 0);
  ASSERT(root != NULL) << "Root is null!";

  LOG(TRACE) << "Checking the initial file system...";  
  tfs_checkfs(sb);

  char path[PATH_LEN+1];
  char data[DATA_LEN+1] = {};
  int r;

  for (int n = 0; n < LENGTH; n++) {
    OneOf(
	  [&r, n, sb, &path] {
	    MakeNewPath(path);
	    LOG(TRACE) <<
	      "STEP " << n << ": tfs_mkdir(sb, \"" << path << "\");";
	    r = tfs_mkdir(sb, path);
	    LOG(TRACE) <<
	      "RESULT " << n << ": tfs_mkdir(sb, \"" << path << "\") = " << r;
	  },
	  [&r, n, sb, &path] {
	    MakeNewPath(path);
	    LOG(TRACE) <<
	      "STEP " << n << ": tfs_rmdir(sb, \"" << path << "\");";
	    r = tfs_rmdir(sb, path);
	    LOG(TRACE) <<
	      "RESULT " << n << ": tfs_rmdir(sb, \"" << path << "\") = " << r;
	  },
	  [&r, n, sb] {
	    LOG(TRACE) << "STEP " << n << ": tfs_ls(sb);";
	    r = tfs_ls(sb);
	    LOG(TRACE) << "RESULT " << n << ": tfs_ls(sb) = " << r;
	  },
	  [&r, n, sb] {
	    LOG(TRACE) << "STEP " << n << ": tfs_lsr(sb);";
	    r = tfs_lsr(sb);
	    LOG(TRACE) << "RESULT " << n << ": tfs_lsr(sb) = " << r;
	  },
	  [&r, n, sb, &path] {
	    MakeNewPath(path);
	    LOG(TRACE) <<
	      "STEP " << n << ": tfs_create(sb, \"" << path << "\");";
	    r = tfs_create(sb, path);
	    LOG(TRACE) <<
	      "RESULT " << n << ": tfs_create(sb, \"" << path << "\") = " << r;
	  },
	  [&r, n, sb, &path, &data] {
	    MakeNewPath(path);
	    MakeNewData(data);
	    LOG(TRACE) <<
	      "STEP " << n << ": tfs_write(sb, \"" << path << "\", \"" <<
	      data << "\");";
	    r = tfs_write(sb, path, data);
	    LOG(TRACE) <<
	      "RESULT " << n << ": tfs_write(sb, \"" << path << "\", \"" <<
	      data << "\") = " << r;
	  },
	  [&r, n, sb, &path] {
	    MakeNewPath(path);
	    LOG(TRACE) << "STEP " << n << ": tfs_stat(sb);";
	    r = tfs_stat(sb, path);
	    LOG(TRACE) << "RESULT " << n << ": tfs_stat(sb) = " << r;
	  },
	  [&r, n, sb, &path] {
	    MakeNewPath(path);
	    LOG(TRACE) << "STEP " << n << ": tfs_cat(sb);";
	    r = tfs_cat(sb, path);
	    LOG(TRACE) << "RESULT " << n << ": tfs_cat(sb) = " << r;
	  },
	  [n, sb] {
	    ASSUME_EQ(get_reset_countdown(), -1); // Only one reset at a time
	    int k = DeepState_IntInRange(1, MAX_RESET);
	    LOG(TRACE) << "STEP " << n << ": set_reset_countdown(" << k << ");";
	    set_reset_countdown(k);
	  });

    if (get_reset_countdown() == 0) {
      LOG(TRACE) << "Reset took place during operation.";
      set_reset_countdown(-1);
      ASSERT(!testfs_init_super_block(storage, 0, &sb))
	<< "Couldn't initialize super block";
    }
    
    LOG(TRACE) << "Checking the file system...";
    tfs_checkfs(sb);
  }

  testfs_close_super_block(sb);
}
