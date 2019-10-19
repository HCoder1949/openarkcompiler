/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#ifndef MAPLE_ME_INCLUDE_ME_OPTION_H
#define MAPLE_ME_INCLUDE_ME_OPTION_H
#include <vector>
#include "mempool.h"
#include "mempool_allocator.h"
#include "types_def.h"
#include <string>

namespace maple {
class MeOption {
 public:
  explicit MeOption(MemPool &memPool) : optionAlloc(&memPool) {}

  void ParseOptions(int argc, char **argv, std::string &fileName);
  ~MeOption() {}

  void DumpUsage();
  static bool DumpPhase(const std::string &phase);
  static std::unordered_set<std::string> dumpPhases;
  static constexpr int kLevelZero = 0;
  static constexpr int kLevelOne = 1;
  static constexpr int kLevelTwo = 2;
  static constexpr int kLevelThree = 3;
  static bool dumpAfter;
  static constexpr int kRangeArrayLen = 2;
  static unsigned long range[kRangeArrayLen];
  static bool useRange;
  static std::string dumpFunc;
  static bool quiet;
  static bool setCalleeHasSideEffect;
  static bool noSteensgaard;
  static bool noTBAA;
  static uint8 aliasAnalysisLevel;
  static bool noDot;
  static bool stmtNum;
  static uint8 optLevel;
  static bool ignoreIPA;
  static bool lessThrowAlias;
  static bool finalFieldAlias;
  static bool regreadAtReturn;
  void SplitPhases(const std::string &str, std::unordered_set<std::string> &set);
  void GetRange(const std::string &str);

  std::unordered_set<std::string> &GetSkipPhases() {
    return skipPhases;
  }

 private:
  std::unordered_set<std::string> skipPhases;
  MapleAllocator optionAlloc;
};

#ifndef DEBUGFUNC
#define DEBUGFUNC(f)                                                         \
  (MeOption::dumpPhases.find(PhaseName()) != MeOption::dumpPhases.end() && \
   (MeOption::dumpFunc.compare("*") == 0 || f->GetName().find(MeOption::dumpFunc.c_str()) != std::string::npos))
#endif
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_OPTION_H
