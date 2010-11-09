//===----------- VDAGToDAG.cpp - A dag to dag inst selector for VTM -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the VTM target.
//
//===----------------------------------------------------------------------===//

#include "VTM.h"
#include "VTargetMachine.h"
#include "VRegisterInfo.h"
#include "llvm/Intrinsics.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

//===----------------------------------------------------------------------===//
// Instruction Selector Implementation
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
/// VTMDAGToDAGISel - VTM specific code to select VTM instructions for
/// SelectionDAG operations.
namespace {
class VDAGToDAGISel : public SelectionDAGISel {
public:
  VDAGToDAGISel(VTargetMachine &TM, CodeGenOpt::Level OptLevel)
    : SelectionDAGISel(TM, OptLevel) {}

  virtual void PostprocessISelDAG();

  virtual const char *getPassName() const {
    return "VTM DAG->DAG Pattern Instruction Selection";
  }

  // Include the pieces autogenerated from the target description.
#include "VGenDAGISel.inc"

private:
  SDNode *Select(SDNode *N);
  SDNode *SelectADD(SDNode *Node);

  // Walk the DAG after instruction selection, fixing register class issues.
  void FixRegisterClasses(SelectionDAG &DAG);

  const VInstrInfo &getInstrInfo() {
    return *static_cast<const VTargetMachine&>(TM).getInstrInfo();
  }
  const VRegisterInfo *getRegisterInfo() {
    return static_cast<const VTargetMachine&>(TM).getRegisterInfo();
  }
};
}  // end anonymous namespace

FunctionPass *llvm::createVISelDag(VTargetMachine &TM,
                                          CodeGenOpt::Level OptLevel) {
  return new VDAGToDAGISel(TM, OptLevel);
}

void VDAGToDAGISel::PostprocessISelDAG() {
}

SDNode *VDAGToDAGISel::SelectADD(SDNode *N) {
  //N->getValueType(0)
  SDValue Ops[] = { N->getOperand(0), N->getOperand(1), N->getOperand(2)};
  unsigned OpC = 0;
  switch (N->getValueType(0).getSizeInBits()) {
  case 1:   OpC = VTM::VOpAddi1; break;
  case 8:   OpC = VTM::VOpAddi8; break;
  case 16:  OpC = VTM::VOpAddi16; break;
  case 32:  OpC = VTM::VOpAddi32; break;
  case 64:  OpC = VTM::VOpAddi64; break;
  default:
    assert(0 && "Bad value type!");
    OpC = VTM:: INSTRUCTION_LIST_END; break;
  }
  
  return CurDAG->SelectNodeTo(N, OpC, N->getVTList(), Ops, 3);
}

SDNode *VDAGToDAGISel::Select(SDNode *N) {
  if (N->isMachineOpcode())
    return NULL;   // Already selected.

  switch (N->getOpcode()) {
  default: break;
  case VTMISD::ADDDAG:
    return SelectADD(N);
  //case VTMISD::InArg: {
  //  SDValue ArgIdx = N->getOperand(1);
  //  int64_t Val = cast<ConstantSDNode>(ArgIdx)->getZExtValue();
  //  ArgIdx = CurDAG->getTargetConstant(Val, ArgIdx.getValueType());

  //  SDValue Ops[] = { ArgIdx, N->getOperand(0) };
  //  return CurDAG->SelectNodeTo(N, VTM::VTMArgi16t, N->getVTList(), Ops, 2);
  //}
  }

  return SelectCode(N);
}
