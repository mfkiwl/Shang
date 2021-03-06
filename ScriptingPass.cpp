//===- ScriptingPass.cpp - Pass Applies Script to Pogram Data ---*- C++ -*-===//
//
//                      The Shang HLS frameowrk                               //
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the ScriptingPass, a MachineFunctionPass that allow user
// manipulate some data such as current generated RTL module with external script
//
//===----------------------------------------------------------------------===//

#include "vtm/VFInfo.h"
#include "vtm/Passes.h"
#include "vtm/Utilities.h"
#include "vtm/VerilogModuleAnalysis.h"

#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Target/TargetData.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/Format.h"
#define DEBUG_TYPE "vtm-scripting-pass"
#include "llvm/Support/Debug.h"

using namespace llvm;

namespace {
struct ScriptingPass : public MachineFunctionPass {
  static char ID;
  std::string PassName, GlobalScript, FunctionScript;
  TargetData *TD;

  ScriptingPass(const char *Name, const char *FScript, const char *GScript)
    : MachineFunctionPass(ID), PassName(Name),
      GlobalScript(GScript), FunctionScript(FScript), TD(0) {
    initializeVerilogModuleAnalysisPass(*PassRegistry::getPassRegistry());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const {
    MachineFunctionPass::getAnalysisUsage(AU);
    AU.addRequired<VerilogModuleAnalysis>();
    AU.setPreservesAll();
  }

  const char *getPassName() const { return PassName.c_str(); }

  bool doInitialization(Module &M);
  bool doFinalization(Module &M);

  bool runOnMachineFunction(MachineFunction &MF);
};
}

char ScriptingPass::ID = 0;

static void printConstant(raw_ostream &OS, uint64_t Val, Type* Ty,
                          TargetData *TD) {
  OS << '\'';
  if (TD->getTypeSizeInBits(Ty) == 1)
    OS << (Val ? '1' : '0');
  else {
    std::string FormatS =
      "%0" + utostr_32(TD->getTypeStoreSize(Ty) * 2) + "llx";
    OS << "0x" << format(FormatS.c_str(), Val);
  }
  OS << '\'';
}


static void ExtractConstant(raw_ostream &OS, Constant *C, TargetData *TD) {
  if (ConstantInt *CI = dyn_cast<ConstantInt>(C)) {
    printConstant(OS, CI->getZExtValue(), CI->getType(), TD);
    return;
  }

  if (isa<ConstantPointerNull>(C)) {
    printConstant(OS, 0, C->getType(), TD);
    return;
  }

  if (ConstantDataSequential *CDS = dyn_cast<ConstantDataSequential>(C)) {
    ExtractConstant(OS, CDS->getElementAsConstant(0), TD);
    for (unsigned i = 1, e = CDS->getNumElements(); i != e; ++i) {
      OS << ", ";
      ExtractConstant(OS, CDS->getElementAsConstant(i), TD);
    }
    return;
  }

  if (ConstantArray *CA = dyn_cast<ConstantArray>(C)) {
    ExtractConstant(OS, cast<Constant>(CA->getOperand(0)), TD);
    for (unsigned i = 1, e = CA->getNumOperands(); i != e; ++i) {
      OS << ", ";
      ExtractConstant(OS, cast<Constant>(CA->getOperand(i)), TD);
    }
    return;
  }

  llvm_unreachable("Unsupported constant type to bind to script engine!");
  OS << '0';
}

bool llvm::runScriptOnGlobalVariables(Module &M, TargetData *TD,
                                      const std::string &ScriptToRun,
                                      SMDiagnostic Err) {
  // Put the global variable information to the script engine.
  if (!runScriptStr("GlobalVariables = {}\n", Err))
    llvm_unreachable("Cannot create globalvariable table in scripting pass!");

  std::string Script;
  raw_string_ostream SS(Script);
  // Push the global variable information into the script engine.
  for (Module::global_iterator GI = M.global_begin(), E = M.global_end();
       GI != E; ++GI ){
    GlobalVariable *GV = GI;

    // GlobalVariable information:
    // GVInfo {
    //   bool isLocal
    //   unsigned AddressSpace
    //   unsigned Alignment
    //   unsigned NumElems
    //   unsigned ElemSize
    //   table Initializer.
    //}

    SS << "GlobalVariables." << VBEMangle(GV->getName()) << " = { ";
    SS << "isLocal = " << GV->hasLocalLinkage() << ", ";
    SS << "AddressSpace = " << GV->getType()->getAddressSpace() << ", ";
    SS << "Alignment = " << GV->getAlignment() << ", ";
    Type *Ty = cast<PointerType>(GV->getType())->getElementType();
    // The element type of a scalar is the type of the scalar.
    Type *ElemTy = Ty;
    unsigned NumElem = 1;
    // Try to expand multi-dimension array to single dimension array.
    while (const ArrayType *AT = dyn_cast<ArrayType>(ElemTy)) {
      ElemTy = AT->getElementType();
      NumElem *= AT->getNumElements();
    }
    SS << "NumElems = " << NumElem << ", ";

    SS << "ElemSize = " << TD->getTypeStoreSizeInBits(ElemTy) << ", ";

    // The initialer table: Initializer = { c0, c1, c2, ... }
    SS << "Initializer = ";
    if (!GV->hasInitializer())
      SS << "nil";
    else {
      Constant *C = GV->getInitializer();

      SS << "{ ";
      if (C->isNullValue()) {
        Constant *Null = Constant::getNullValue(ElemTy);

        ExtractConstant(SS, Null, TD);
        for (unsigned i = 1; i < NumElem; ++i) {
          SS << ", ";
          ExtractConstant(SS, Null, TD);
        }
      } else
        ExtractConstant(SS, C, TD);

      SS << "}";
    }

    SS << '}';

    SS.flush();
    if (!runScriptStr(Script, Err)) {
      llvm_unreachable("Cannot create globalvariable infomation!");
    }
    Script.clear();
  }

  // Run the script against the GlobalVariables table.
  return runScriptStr(ScriptToRun, Err);
}

void llvm::bindFunctionInfoToScriptEngine(MachineFunction &MF, TargetData &TD,
                                          VASTModule *Module) {
  SMDiagnostic Err;
  // Push the function information into the script engine.
  // FuncInfo {
  //   String Name,
  //   unsinged ReturnSize,
  //   ArgTable : { ArgName = Size, ArgName = Size ... }
  // }

  const Function *F = MF.getFunction();

  std::string Script;
  raw_string_ostream SS(Script);

  SS << "FuncInfo = { ";
  SS << "Name = '" << F->getName() << "', ";

  SS << "ReturnSize = ";
  if (F->getReturnType()->isVoidTy())
    SS << '0';
  else
    SS << TD.getTypeStoreSizeInBits(F->getReturnType());
  SS << ", ";

  SS << "Args = { ";

  if (F->arg_size()) {
    Function::const_arg_iterator I = F->arg_begin();
    SS << "{ Name = '" << I->getName() << "', Size = "
      << TD.getTypeStoreSizeInBits(I->getType()) << "}";
    ++I;

    for (Function::const_arg_iterator E = F->arg_end(); I != E; ++I)
      SS << " , { Name = '" << I->getName() << "', Size = "
      << TD.getTypeStoreSizeInBits(I->getType()) << "}";
  }

  SS << "} }";

  SS.flush();
  if (!runScriptStr(Script, Err))
    llvm_unreachable("Cannot create function infomation!");
  Script.clear();

  bindToScriptEngine("CurModule", Module);
}

bool ScriptingPass::doInitialization(Module &M) {
  TD = getAnalysisIfAvailable<TargetData>();
  assert(TD && "TD not avaialbe?");

  SMDiagnostic Err;
  if (!runScriptOnGlobalVariables(M, TD, GlobalScript, Err))
    report_fatal_error("In Scripting pass[" + PassName + "]:\n"
                       + Err.getMessage());

  return false;
}

bool ScriptingPass::doFinalization(Module &M) {
  return false;
}

bool ScriptingPass::runOnMachineFunction(MachineFunction &MF) {
  VASTModule *Module = getAnalysis<VerilogModuleAnalysis>().getModule();
  bindFunctionInfoToScriptEngine(MF, *TD, Module);

  SMDiagnostic Err;
  if (!runScriptStr(FunctionScript, Err))
    report_fatal_error("In Scripting pass[" + PassName + "]:\n"
                       + Err.getMessage());

  return false;
}

Pass *llvm::createScriptingPass(const char *Name, const char *FScript,
                                const char *GScript) {
  return new ScriptingPass(Name, FScript, GScript);
}
