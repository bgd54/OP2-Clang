#ifndef OP2WRITEABLEREFACTORINGTOOL_HPP
#define OP2WRITEABLEREFACTORINGTOOL_HPP

// clang Tooling includes
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"

namespace OP2 {
/// @brief Base class for RefactoringTools in op2-clang. Adds utility to write
/// output to files or streams.
class OP2WriteableRefactoringTool : public clang::tooling::RefactoringTool {
public:
  /// @brief Generate output filename.
  ///
  /// @param Entry The input file that processed
  ///
  /// @return output filename (e.g. xxx_op.cpp, xxx_kernel.cpp)
  virtual std::string
  getOutputFileName(const clang::FileEntry *Entry) const = 0;

  OP2WriteableRefactoringTool(
      clang::tooling::CommonOptionsParser &optionsParser)
      : clang::tooling::RefactoringTool(optionsParser.getCompilations(),
                                        optionsParser.getSourcePathList()) {}
  OP2WriteableRefactoringTool(
      const clang::tooling::CompilationDatabase &Compilations,
      const std::vector<std::string> Sources)
      : clang::tooling::RefactoringTool(Compilations, Sources) {}

  /// @brief Create the output files based on the replacements.
  virtual void writeOutReplacements() {
    // Set up the Rewriter (For this we need a SourceManager)
    llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> DiagOpts =
        new clang::DiagnosticOptions();
    clang::DiagnosticsEngine Diagnostics(
        llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs>(
            new clang::DiagnosticIDs()),
        &*DiagOpts, new clang::TextDiagnosticPrinter(llvm::errs(), &*DiagOpts),
        true);
    clang::SourceManager Sources(Diagnostics, getFiles());

    // Apply all replacements to a rewriter.
    clang::Rewriter Rewrite(Sources, clang::LangOptions());
    formatAndApplyAllReplacements(getReplacements(), Rewrite, "LLVM");

    // Query the rewriter for all the files it has rewritten, dumping their new
    // contents to output files.
    for (clang::Rewriter::buffer_iterator I = Rewrite.buffer_begin(),
                                          E = Rewrite.buffer_end();
         I != E; ++I) {

      std::string filename =
          getOutputFileName(Sources.getFileEntryForID(I->first));
      std::error_code ec;
      llvm::raw_fd_ostream outfile{llvm::StringRef(filename), ec,
                                   llvm::sys::fs::F_Text };

      I->second.write(outfile);
    }
  }

  virtual void writeReplacementsTo(llvm::raw_ostream &os) {
    // Set up the Rewriter (For this we need a SourceManager)
    llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> DiagOpts =
        new clang::DiagnosticOptions();
    clang::DiagnosticsEngine Diagnostics(
        llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs>(
            new clang::DiagnosticIDs()),
        &*DiagOpts, new clang::TextDiagnosticPrinter(llvm::errs(), &*DiagOpts),
        true);
    clang::SourceManager Sources(Diagnostics, getFiles());

    // Apply all replacements to a rewriter.
    clang::Rewriter Rewrite(Sources, clang::LangOptions());
    formatAndApplyAllReplacements(getReplacements(), Rewrite, "LLVM");

    // Query the rewriter for all the files it has rewritten, dumping their new
    // contents to output files.
    for (clang::Rewriter::buffer_iterator I = Rewrite.buffer_begin(),
                                          E = Rewrite.buffer_end();
         I != E; ++I) {
      I->second.write(os);
    }
  }

  void addReplacementTo(std::string fileName, clang::tooling::Replacement repl,
                        std::string diagMessage) {
    llvm::Error err = getReplacements()[fileName].add(repl);
    if (err) { // TODO proper error checking
      llvm::outs() << "Some Error occured during adding replacement for "
                   << diagMessage << "\n";
    }
  }

  virtual ~OP2WriteableRefactoringTool() = default;
};

} // namespace OP2

#endif /* ifndef OP2WRITEABLEREFACTORINGTOOL_HPP */
