#ifndef CUDAKERNELHANDLER_H
#define CUDAKERNELHANDLER_H
#include "core/OPParLoopData.h"
#include "core/op2_clang_core.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Refactoring.h"

namespace OP2 {
namespace matchers = clang::ast_matchers;

/// @brief Callback for perform the specific modifications for CUDA
/// kernels on skeleton
class CUDAKernelHandler : public matchers::MatchFinder::MatchCallback {
protected:
  std::map<std::string, clang::tooling::Replacements> *Replace;
  const clang::tooling::CompilationDatabase &Compilations;
  const OP2Application &application;
  const size_t loopIdx;
  OP2Optimizations op2Flags;

  std::string getCUDAFuncDefinition();
  std::string getLocalVarDecls();
  template <bool GEN_CONSTANTS = false> std::string getReductArrsToDevice();
  std::string getHostReduction();
  std::string genLocalArrDecl();
  std::string genLocalArrInit();
  std::string genRedForstmt();
  std::string genFuncCall();
  std::string genCUDAkernelLaunch();
  std::string getMapIdxDecls();
  std::string getMapIdxInits();
  std::string genWriteIncrement();
  std::string genStrideDecls();
  std::string genStrideInit();

public:
  /// @brief Construct a CUDAKernelHandler
  ///
  /// @param Replace Replacements map from the RefactoringTool where
  /// Replacements should added.
  /// @param app Application collected data
  /// @param idx index of the currently generated loop
  CUDAKernelHandler(
      std::map<std::string, clang::tooling::Replacements> *Replace,
      const clang::tooling::CompilationDatabase &Compilations,
      const OP2Application &app, size_t idx, OP2Optimizations flags);
  // Static matchers handled by this class
  static const matchers::DeclarationMatcher cudaFuncMatcher;
  static const matchers::StatementMatcher cudaFuncCallMatcher;
  static const matchers::DeclarationMatcher declLocalRedArrMatcher;
  static const matchers::StatementMatcher initLocalRedArrMatcher;
  static const matchers::StatementMatcher opReductionMatcher;
  static const matchers::StatementMatcher updateRedArrsOnHostMatcher;
  static const matchers::StatementMatcher setReductionArraysToArgsMatcher;
  static const matchers::StatementMatcher setConstantArraysToArgsMatcher;
  static const matchers::DeclarationMatcher arg0hDeclMatcher;
  static const matchers::DeclarationMatcher mapidxDeclMatcher;
  static const matchers::DeclarationMatcher strideDeclMatcher;
  static const matchers::StatementMatcher strideInitMatcher;
  static const matchers::StatementMatcher mapidxInitMatcher;
  static const matchers::StatementMatcher incrementWriteMatcher;
  static const matchers::StatementMatcher constantHandlingMatcher;
  static const matchers::StatementMatcher reductHandlingMatcher;
  static const matchers::StatementMatcher mvRedArrsDtoHostMatcher;

  virtual void run(const matchers::MatchFinder::MatchResult &Result) override;
};

} // end of namespace OP2

#endif /* ifndef CUDAKERNELHANDLER_H  */
