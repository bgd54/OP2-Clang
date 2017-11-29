#ifndef OMPREFACTORINGTOOL_H
#define OMPREFACTORINGTOOL_H

#include "../OPParLoopData.h"
#include "GeneratorBase.hpp"
#include "OMPKernelHandler.h"
#include "SeqKernelHandler.h"

namespace OP2 {
/// \brief Utility of generate OpenMP kernel based on ParLoop information.
///
///
class OMPRefactoringTool : public OP2KernelGeneratorBase {
  static const std::string skeletons[2];

  /// @brief Handler for OpenMP kernel specific modifications.
  ///
  OMPKernelHandler ompKernelHandler;
  /// @brief Handler for modifications same as sequential case.
  ///
  SeqKernelHandler seqKernelHandler;

public:
  /// @brief Construct a refactoring tool to generate the OpenMP kernel.
  ///
  /// @param Compilations The CompilationDatabase which contains the copmile
  /// commandlines.
  /// @param loop The ParLoop containing informations about the op_par_loop.
  /// @param PCHContainerOps The PCHContainerOperation for loading and creating
  /// clang modules
  // TODO: Modify to get right skeletons... and Database..
  OMPRefactoringTool(
      const clang::tooling::CompilationDatabase &Compilations,
      const ParLoop &loop,
      std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps =
          std::make_shared<clang::PCHContainerOperations>());

  /// @brief Adding OpenMP specific Matchers and handlers.
  ///   Called from OP2KernelGeneratorBase::GenerateKernelFile()
  ///
  /// @param MatchFinder used by the RefactoringTool
  virtual void addGeneratorSpecificMatchers(clang::ast_matchers::MatchFinder &);

  static constexpr const char *_postfix = "kernel";
  static constexpr unsigned numParams = 1;
  static constexpr const char *commandlineParams[numParams] = {"-fopenmp"};

  virtual ~OMPRefactoringTool() = default;
};

} // namespace OP2

#endif