//===------------ HWAtom.cpp - Translate LLVM IR to HWAtom  -----*- C++ -*-===//
//
//                            The Verilog Backend
//
// Copyright: 2010 by Hongbin Zheng. all rights reserved.
// IMPORTANT: This software is supplied to you by Hongbin Zheng in consideration
// of your agreement to the following terms, and your use, installation, 
// modification or redistribution of this software constitutes acceptance
// of these terms.  If you do not agree with these terms, please do not use, 
// install, modify or redistribute this software. You may not redistribute, 
// install copy or modify this software without written permission from 
// Hongbin Zheng. 
//
//===----------------------------------------------------------------------===//
//
// This file implement the HWAtom class, which represent the basic atom
// operation in hardware.
//
//===----------------------------------------------------------------------===//

#include "vbe/HWAtom.h"

#include "llvm/Support/Debug.h"
#include "llvm/Analysis/LoopInfo.h"

#include "vbe/ResourceConfig.h"

using namespace llvm;
using namespace esyn;


HWAtom::~HWAtom() {}

void HWAtom::dump() const {
  print(dbgs());
  dbgs() << '\n';
}

void HWAConst::print(raw_ostream &OS) const {
  OS << "Const (" << getValue() << ")";
}

void HWASigned::print(raw_ostream &OS) const {
  OS << "signed (";
  WriteAsOperand(OS, &getDep(0)->getValue(), false);
  OS << ")";
}

void HWARegister::print(raw_ostream &OS) const {
  OS << "Register: ";
  WriteAsOperand(OS, &Val, false);
  OS << " using ";
  WriteAsOperand(OS, &getDep(0)->getValue(), false);
}

void ExecStage::getScheduleMap(ScheduleMapType &Atoms) const {
  for (ExecStage::const_iterator I = begin(), E = end(); I != E; ++I) {
    HWAtom *A = *I;
    Atoms.insert(std::make_pair<unsigned, HWAtom*>(A->getSlot(), A));
  }
}

void ExecStage::print(raw_ostream &OS) const {
  OS << "State: ";
  WriteAsOperand(OS, getBasicBlock(), false);
  OS << "\n";
  //for (HWAState::const_iterator I = begin(), E = end(); I != E; ++I) {
  //  (*I)->print(OS.indent(2));
  //  OS << "\n";
  //}
  unsigned oldSlot = 0;

  std::multimap<unsigned, HWAtom*> Atoms;
  getScheduleMap(Atoms);
  for (std::multimap<unsigned, HWAtom*>::iterator I = Atoms.begin(),
      E = Atoms.end(); I != E; ++I) {
    HWAtom *A = I->second;
    if (A->getSlot() != oldSlot) {
      oldSlot = A->getSlot();
      OS << "Cycle: " << oldSlot << "\n";
    }
    A->print(OS.indent(2));
    OS << " at "<< A->getSlot() << "\n";
  }

  const HWAPostBind &ExitRoot = getExitRoot();
  ExitRoot.print(OS.indent(2));
  OS << " at "<< ExitRoot.getSlot() << "\n";
}

void HWAPreBind::print(raw_ostream &OS) const {
  OS << getValue() << " Res: " << SubClassData << '\n';
}

void HWAPostBind::print(raw_ostream &OS) const {
  OS << getValue() << " PostBind: " << SubClassData;
}

void HWAEntryRoot::print(raw_ostream &OS) const {
  WriteAsOperand(OS, &getValue(), false);
  OS << " Entry";
}
