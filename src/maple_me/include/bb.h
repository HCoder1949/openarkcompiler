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
#ifndef MAPLE_ME_INCLUDE_BB_H
#define MAPLE_ME_INCLUDE_BB_H
#include "utils.h"
#include "mpl_number.h"
#include "ptr_list_ref.h"
#include "orig_symbol.h"
#include "ver_symbol.h"
#include "ssa.h"

namespace maple {
class MeStmt;  // circular dependency exists, no other choice
class MeVarPhiNode;  // circular dependency exists, no other choice
class MeRegPhiNode;  // circular dependency exists, no other choice
class PaiassignMeStmt;
class IRMap;  // circular dependency exists, no other choice
enum BBKind {
  kBBUnknown,  // uninitialized
  kBBCondGoto,
  kBBGoto,  // unconditional branch
  kBBFallthru,
  kBBReturn,
  kBBAfterGosub,  // the BB that follows a gosub, as it is an entry point
  kBBSwitch,
  kBBInvalid
};

enum BBAttr : uint32 {
  kBBAttrIsEntry = utils::bit_field_v<1>,
  kBBAttrIsExit = utils::bit_field_v<2>,
  kBBAttrWontExit = utils::bit_field_v<3>,
  kBBAttrIsTry = utils::bit_field_v<4>,
  kBBAttrIsTryEnd = utils::bit_field_v<5>,
  kBBAttrIsJSCatch = utils::bit_field_v<6>,
  kBBAttrIsJSFinally = utils::bit_field_v<7>,
  kBBAttrIsCatch = utils::bit_field_v<8>,
  kBBAttrIsJavaFinally = utils::bit_field_v<9>,
  kBBAttrArtificial = utils::bit_field_v<10>,
  kBBAttrIsInLoop = utils::bit_field_v<11>,
  kBBAttrIsInLoopForEA = utils::bit_field_v<12>
};

constexpr uint32 kBBVectorInitialSize = 2;
using StmtNodes = PtrListRef<StmtNode>;
using MeStmts = PtrListRef<MeStmt>;

class BB {
 public:
  using BBId = utils::Index<BB>;

  BB(MapleAllocator *alloc, MapleAllocator *versAlloc, BBId id)
      : id(id),
        bbLabel(0),
        pred(kBBVectorInitialSize, nullptr, alloc->Adapter()),
        succ(kBBVectorInitialSize, nullptr, alloc->Adapter()),
        phiList(versAlloc->Adapter()),
        mevarPhiList(alloc->Adapter()),
        meregPhiList(alloc->Adapter()),
        mevarPaiList(alloc->Adapter()),
        frequency(0),
        kind(kBBUnknown),
        attributes(0) {
    pred.pop_back();
    pred.pop_back();
    succ.pop_back();
    succ.pop_back();
  }

  BB(MapleAllocator *alloc, MapleAllocator *versAlloc, BBId id, StmtNode *firstStmt, StmtNode *lastStmt)
      : id(id),
        bbLabel(0),
        pred(kBBVectorInitialSize, nullptr, alloc->Adapter()),
        succ(kBBVectorInitialSize, nullptr, alloc->Adapter()),
        phiList(versAlloc->Adapter()),
        mevarPhiList(alloc->Adapter()),
        meregPhiList(alloc->Adapter()),
        mevarPaiList(alloc->Adapter()),
        frequency(0),
        kind(kBBUnknown),
        attributes(0),
        stmtNodeList(firstStmt, lastStmt) {
    pred.pop_back();
    pred.pop_back();
    succ.pop_back();
    succ.pop_back();
  }

  bool GetAttributes(uint32 attrKind) const {
    return (attributes & attrKind) != 0;
  }

  void SetAttributes(uint32 attrKind) {
    attributes |= attrKind;
  }

  void ClearAttributes(uint32 attrKind) {
    attributes &= (~attrKind);
  }

  virtual bool IsGoto() const {
    return kind == kBBGoto;
  }

  virtual bool IsFuncEntry() const {
    return false;
  }

  virtual bool AddBackEndTry() const {
    return GetAttributes(kBBAttrIsTryEnd);
  }

  void Dump(MIRModule *mod);
  void DumpHeader(MIRModule *mod);
  void DumpPhi(MIRModule* mod);
  void DumpBBAttribute(MIRModule *mod);
  std::string StrAttribute() const;

  void AddPredBB(BB *predVal) {
    ASSERT(predVal != nullptr, "null ptr check");
    pred.push_back(predVal);
    predVal->succ.push_back(this);
  }

  // This is to help new bb to keep some flags from original bb after logically splitting.
  void CopyFlagsAfterSplit(const BB &bb) {
    bb.GetAttributes(kBBAttrIsTry) ? SetAttributes(kBBAttrIsTry) : ClearAttributes(kBBAttrIsTry);
    bb.GetAttributes(kBBAttrIsTryEnd) ? SetAttributes(kBBAttrIsTryEnd) : ClearAttributes(kBBAttrIsTryEnd);
    bb.GetAttributes(kBBAttrIsExit) ? SetAttributes(kBBAttrIsExit) : ClearAttributes(kBBAttrIsExit);
    bb.GetAttributes(kBBAttrWontExit) ? SetAttributes(kBBAttrWontExit) : ClearAttributes(kBBAttrWontExit);
  }

  BBId GetBBId() const {
    return id;
  }

  void SetBBId(BBId idx) {
    id = idx;
  }

  uint32 UintID() const {
    return static_cast<uint32>(id);
  }

  StmtNode *GetTheOnlyStmtNode() const;
  bool IsEmpty() const {
    return stmtNodeList.empty();
  }

  void SetFirst(StmtNode *stmt) {
    stmtNodeList.update_front(stmt);
  }

  void SetLast(StmtNode *stmt) {
    stmtNodeList.update_back(stmt);
  }

  StmtNode *GetFirst() {
    return &(stmtNodeList.front());
  }

  StmtNode *GetLast() {
    return &(stmtNodeList.back());
  }

  void SetFirstMe(MeStmt *stmt);
  void SetLastMe(MeStmt *stmt);
  MeStmt *GetLastMe();
  bool IsInList(const MapleVector<BB*> &bbList) const;
  bool IsPredBB(const BB &bb) const {
    // if this is a pred of bb return true;
    // otherwise return false;
    return IsInList(bb.pred);
  }

  bool IsSuccBB(const BB &bb) const {
    return IsInList(bb.succ);
  }
  void DumpMeBB(IRMap &irMap);

  void AddSuccBB(BB *succPara) {
    succ.push_back(succPara);
    succPara->pred.push_back(this);
  }

  void ReplacePred(const BB *old, BB *newPred);
  void ReplaceSucc(const BB *old, BB *newSucc);
  void ReplaceSuccOfCommonEntryBB(const BB *old, BB *newSucc);
  void AddStmtNode(StmtNode *stmt);
  void PrependStmtNode(StmtNode *stmt);
  void RemoveStmtNode(StmtNode *stmt);
  void RemoveLastStmt();
  void InsertStmtBefore(StmtNode *stmt, StmtNode *newStmt);
  void ReplaceStmt(StmtNode *stmt, StmtNode *newStmt);
  int RemoveBBFromVector(MapleVector<BB*> &bbVec) const;
  void RemoveBBFromPred(BB *bb);
  void RemoveBBFromSucc(BB *bb);

  void RemovePred(BB *predBB) {
    predBB->RemoveBBFromSucc(this);
    RemoveBBFromPred(predBB);
  }

  void RemoveSucc(BB *succBB) {
    succBB->RemoveBBFromPred(this);
    RemoveBBFromSucc(succBB);
  }

  void InsertPai(BB &bb, PaiassignMeStmt &s) {
    if (mevarPaiList.find(&bb) == mevarPaiList.end()) {
      std::vector<PaiassignMeStmt*> tmp;
      mevarPaiList[&bb] = tmp;
    }
    mevarPaiList[&bb].push_back(&s);
  }

  MapleMap<BB*, std::vector<PaiassignMeStmt*>>& GetPaiList() {
    return mevarPaiList;
  }

  bool IsMeStmtEmpty() const {
    return meStmtList.empty();
  }

  void FindReachableBBs(std::vector<bool> &visitedBBs) const;
  void FindWillExitBBs(std::vector<bool> &visitedBBs) const;
  const PhiNode *PhiofVerStInserted(const VersionSt &versionSt) const;
  void InsertPhi(MapleAllocator *alloc, VersionSt *versionSt);
  void PrependMeStmt(MeStmt *meStmt);
  void RemoveMeStmt(MeStmt *meStmt);
  void AddMeStmtFirst(MeStmt *meStmt);
  void AddMeStmtLast(MeStmt *meStmt);
  void InsertMeStmtBefore(MeStmt *meStmt, MeStmt *inStmt);
  void InsertMeStmtAfter(MeStmt *meStmt, MeStmt *inStmt);
  void InsertMeStmtLastBr(MeStmt *inStmt);
  void ReplaceMeStmt(MeStmt *stmt, MeStmt *newStmt);
  void DumpMeVarPhiList(IRMap *irMap);
  void DumpMeRegPhiList(IRMap *irMap);
  void DumpMeVarPaiList(IRMap *irMap);
  StmtNodes &GetStmtNodes() {
    return stmtNodeList;
  }

  const StmtNodes &GetStmtNodes() const {
    return stmtNodeList;
  }

  MeStmts &GetMeStmts() {
    return meStmtList;
  }

  const MeStmts &GetMeStmts() const {
    return meStmtList;
  }

  virtual ~BB() = default;

  LabelIdx GetBBLabel() const {
    return bbLabel;
  }

  void SetBBLabel(LabelIdx idx) {
    bbLabel = idx;
  }

  uint32 GetFrequency() const {
    return frequency;
  }

  void SetFrequency(uint32 f) {
    frequency = f;
  }

  BBKind GetKind() const {
    return kind;
  }

  void SetKind(BBKind bbKind) {
    kind = bbKind;
  }

  MapleVector<BB*> &GetPred() {
    return pred;
  }

  const MapleVector<BB*> &GetPred() const {
    return pred;
  }

  const BB *GetPred(size_t cnt) const {
    ASSERT(cnt < pred.size(), "out of range in BB::GetPred");
    return pred.at(cnt);
  }

  BB *GetPred(size_t cnt) {
    ASSERT(cnt < pred.size(), "out of range in BB::GetPred");
    return pred.at(cnt);
  }

  void SetPred(size_t cnt, BB *pp) {
    CHECK_FATAL(cnt < pred.size(), "out of range in BB::SetPred");
    pred[cnt] = pp;
  }

  MapleVector<BB*> &GetSucc() {
    return succ;
  }

  const MapleVector<BB*> &GetSucc() const {
    return succ;
  }

  const BB *GetSucc(size_t cnt) const {
    ASSERT(cnt < succ.size(), "out of range in BB::GetSucc");
    return succ.at(cnt);
  }

  BB *GetSucc(size_t cnt) {
    ASSERT(cnt < succ.size(), "out of range in BB::GetSucc");
    return succ.at(cnt);
  }

  void SetSucc(size_t cnt, BB *ss) {
    CHECK_FATAL(cnt < succ.size(), "out of range in BB::SetSucc");
    succ[cnt] = ss;
  }

  const MapleMap<const OriginalSt*, PhiNode> &GetPhiList() const {
    return phiList;
  }
  MapleMap<const OriginalSt*, PhiNode> &GetPhiList() {
    return phiList;
  }
  void ClearPhiList() {
    phiList.clear();
  }

  MapleMap<OStIdx, MeVarPhiNode*> &GetMevarPhiList() {
    return mevarPhiList;
  }

  const MapleMap<OStIdx, MeVarPhiNode*> &GetMevarPhiList() const {
    return mevarPhiList;
  }

  MapleMap<OStIdx, MeRegPhiNode*> &GetMeregphiList() {
    return meregPhiList;
  }

  const MapleMap<OStIdx, MeRegPhiNode*> &GetMeregphiList() const {
    return meregPhiList;
  }

 private:
  BBId id;
  LabelIdx bbLabel;       // the BB's label
  MapleVector<BB*> pred;  // predecessor list
  MapleVector<BB*> succ;  // successor list
  MapleMap<const OriginalSt*, PhiNode> phiList;
  MapleMap<OStIdx, MeVarPhiNode*> mevarPhiList;
  MapleMap<OStIdx, MeRegPhiNode*> meregPhiList;
  MapleMap<BB*, std::vector<PaiassignMeStmt*>> mevarPaiList;
  uint32 frequency;
  BBKind kind;
  uint32 attributes;
  StmtNodes stmtNodeList;
  MeStmts meStmtList;
};

using BBId = BB::BBId;

class SCCOfBBs {
 public:
  SCCOfBBs(uint32 index, BB *bb, MapleAllocator *alloc)
      : id(index),
        entry(bb),
        bbs(alloc->Adapter()),
        predSCC(std::less<SCCOfBBs*>(), alloc->Adapter()),
        succSCC(std::less<SCCOfBBs*>(), alloc->Adapter()) {}
  ~SCCOfBBs() = default;
  void Dump();
  void Verify(MapleVector<SCCOfBBs*> &sccOfBB);
  void SetUp(MapleVector<SCCOfBBs*> &sccOfBB);
  bool HasCycle() const;
  void AddBBNode(BB *bb) {
    bbs.push_back(bb);
  }
  void Clear() {
    bbs.clear();
  }
  uint32 GetID() const {
    return id;
  }
  const MapleVector<BB*> &GetBBs() const {
    return bbs;
  }
  const MapleSet<SCCOfBBs*> &GetSucc() const {
    return succSCC;
  }
  const MapleSet<SCCOfBBs*> &GetPred() const {
    return predSCC;
  }
  bool HasPred() const {
    return (predSCC.size() != 0);
  }
  BB *GetEntry() {
    return entry;
  }
 private:
  uint32 id;
  BB *entry;
  MapleVector<BB*> bbs;
  MapleSet<SCCOfBBs*> predSCC;
  MapleSet<SCCOfBBs*> succSCC;
};
}  // namespace maple

namespace std {
template <>
struct hash<maple::BBId> {
  size_t operator()(const maple::BBId &x) const {
    return x;
  }
};
}  // namespace std

#endif  // MAPLE_ME_INCLUDE_BB_H
