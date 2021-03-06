// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SYZYGY_REFINERY_TYPES_UNITTEST_UTIL_H_
#define SYZYGY_REFINERY_TYPES_UNITTEST_UTIL_H_

#include "base/containers/hash_tables.h"
#include "gtest/gtest.h"
#include "syzygy/refinery/core/address.h"

namespace testing {

class PdbCrawlerVTableTestBase : public testing::Test {
 protected:
  virtual void GetVFTableRVAs(
      const wchar_t* pdb_path_str,
      base::hash_set<refinery::RelativeAddress>* vftable_rvas) = 0;
  void PerformGetVFTableRVAsTest(const wchar_t* pdb_path_str,
                                 const wchar_t* dll_path_str);
};

}  // namespace testing

#endif  // SYZYGY_REFINERY_TYPES_UNITTEST_UTIL_H_
