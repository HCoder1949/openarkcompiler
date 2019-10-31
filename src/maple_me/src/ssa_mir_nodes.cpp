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
#include "ssa_mir_nodes.h"
#include "opcode_info.h"
#include "mir_function.h"
#include "printing.h"
#include "ssa_tab.h"

namespace maple {
void GenericSSAPrint(MIRModule &mod, const StmtNode &stmtNode, int32 indent, StmtsSSAPart &stmtsSSAPart) {
  stmtNode.Dump(mod, indent);
  // print SSAPart
  Opcode op = stmtNode.GetOpCode();
  AccessSSANodes *ssaPart = stmtsSSAPart.SSAPartOf(stmtNode);
  switch (op) {
    case OP_maydassign:
    case OP_dassign: {
      mod.GetOut() << " ";
      ssaPart->GetSSAVar()->Dump(&mod);
      ssaPart->DumpMayDefNodes(mod);
      return;
    }
    case OP_regassign: {
      mod.GetOut() << "  ";
      ssaPart->GetSSAVar()->Dump(&mod);
      return;
    }
    case OP_iassign: {
      ssaPart->DumpMayDefNodes(mod);
      return;
    }
    case OP_throw:
    case OP_retsub:
    case OP_return: {
      ssaPart->DumpMayUseNodes(mod);
      mod.GetOut() << '\n';
      return;
    }
    default: {
      if (kOpcodeInfo.IsCallAssigned(op)) {
        ssaPart->DumpMayUseNodes(mod);
        ssaPart->DumpMustDefNodes(mod);
        ssaPart->DumpMayDefNodes(mod);
      } else if (kOpcodeInfo.HasSSADef(op) && kOpcodeInfo.HasSSAUse(op)) {
        ssaPart->DumpMayUseNodes(mod);
        mod.GetOut() << '\n';
        ssaPart->DumpMayDefNodes(mod);
      }
      return;
    }
  }
}

static MapleMap<OStIdx, MayDefNode> *SSAGenericGetMayDefsFromVersionSt(VersionSt &vst, StmtsSSAPart &stmtsSSAPart,
                                                                       std::unordered_set<VersionSt*> &visited) {
  if (vst.IsInitVersion() || visited.find(&vst) != visited.end()) {
    return nullptr;
  }
  visited.insert(&vst);
  if (vst.GetDefType() == VersionSt::kPhi) {
    PhiNode *phi = vst.GetPhi();
    for (size_t i = 0; i < phi->GetPhiOpnds().size(); i++) {
      VersionSt *vsym = phi->GetPhiOpnd(i);
      MapleMap<OStIdx, MayDefNode> *mayDefs = SSAGenericGetMayDefsFromVersionSt(*vsym, stmtsSSAPart, visited);
      if (mayDefs != nullptr) {
        return mayDefs;
      }
    }
  } else if (vst.GetDefType() == VersionSt::kMayDef) {
    MayDefNode *maydef = vst.GetMayDef();
    return &stmtsSSAPart.GetMayDefNodesOf(*maydef->GetStmt());
  }
  return nullptr;
}

MapleMap<OStIdx, MayDefNode> *SSAGenericGetMayDefsFromVersionSt(VersionSt &sym, StmtsSSAPart &stmtsSSAPart) {
  std::unordered_set<VersionSt*> visited;
  return SSAGenericGetMayDefsFromVersionSt(sym, stmtsSSAPart, visited);
}

bool HasMayUseOpnd(const BaseNode &baseNode, SSATab &func) {
  const StmtNode &stmtNode = static_cast<const StmtNode&>(baseNode);
  if (kOpcodeInfo.HasSSAUse(stmtNode.GetOpCode())) {
    MapleMap<OStIdx, MayUseNode> &mayUses = func.GetStmtsSSAPart().GetMayUseNodesOf(stmtNode);
    if (!mayUses.empty()) {
      return true;
    }
  }
  for (size_t i = 0; i < baseNode.NumOpnds(); i++) {
    if (HasMayUseOpnd(*baseNode.Opnd(i), func)) {
      return true;
    }
  }
  return false;
}
}  // namespace maple
