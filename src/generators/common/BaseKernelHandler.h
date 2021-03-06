#ifndef BASEKERNELHANDLER_H
#define BASEKERNELHANDLER_H
#include "core/OPParLoopData.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Refactoring.h"

namespace OP2 {
namespace matchers = clang::ast_matchers;

/// @brief Callback for perform the base modifications on op_par_loop_skeleton
///
/// Callback for perform the base modifications on op_par_loop_skeleton e.g.
/// change name, add user function decl, set args[], and modifications came
/// from numper of args.
class BaseKernelHandler : public matchers::MatchFinder::MatchCallback {
protected:
  std::map<std::string, clang::tooling::Replacements> *Replace;
  const ParLoop &loop;
  int handleParLoopDecl(const matchers::MatchFinder::MatchResult &Result);
  std::string handleArgsArrSetter();
  std::string handleIndsArr();
  std::string handleOPTimingRealloc();
  std::string handleOPDiagPrintf();
  int handleOPKernels(const matchers::MatchFinder::MatchResult &Result);
  int handleMPIWaitAllIfStmt(const matchers::MatchFinder::MatchResult &Result);

public:
  /// @brief Construct a BaseKernelHandler
  ///
  /// @param Replace Replacements map from the RefactoringTool where
  /// Replacements should added.
  ///
  /// @param loop The ParLoop that the file is currently generated.
  BaseKernelHandler(
      std::map<std::string, clang::tooling::Replacements> *Replace,
      const ParLoop &loop);
  // Static matchers handled by this class
  /// @brief Matcher for the op_par_loop_skeleton declaration
  static const matchers::DeclarationMatcher parLoopDeclMatcher;
  /// @brief Matcher for the declaration of nargs
  static const matchers::DeclarationMatcher nargsMatcher;
  /// @brief Matcher for the declaration of args array
  static const matchers::DeclarationMatcher argsArrMatcher;
  /// @brief Matcher for filling args array with op_args
  static const matchers::StatementMatcher argsArrSetterMatcher;
  /// @brief Matcher for op_timing_realloc call to change kernel id
  static const matchers::StatementMatcher opTimingReallocMatcher;
  /// @brief Matcher for the printf call in if(OPDiags > 2)
  static const matchers::StatementMatcher printfKernelNameMatcher;
  /// @brief Matcher for OP_kernels changes at the end of the kernel.
  static const matchers::StatementMatcher opKernelsSubscriptMatcher;
  /// @brief Matcher for the declaration of ninds
  static const matchers::DeclarationMatcher nindsMatcher;
  /// @brief Matcher for the declaration of inds array
  static const matchers::DeclarationMatcher indsArrMatcher;
  /// @brief Matcher for op_mpi_reduce call
  static const matchers::StatementMatcher opMPIReduceMatcher;
  /// @brief Matcher for the surrounding if statement of op_mpi_wait_all calls
  static const matchers::StatementMatcher opMPIWaitAllIfStmtMatcher;

  virtual void run(const matchers::MatchFinder::MatchResult &Result) override;
};

} // end of namespace OP2

#endif /* ifndef BASEKERNELHANDLER_H  */
