#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED
#include <cassert>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Lexer.h>
#include <clang/Tooling/CommonOptionsParser.h>

namespace OP2 {

inline std::string getSourceAsString(const clang::SourceRange d,
                                     const clang::SourceManager *sm) {
  clang::SourceLocation b(d.getBegin());
  clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(
      d.getEnd(), 0, *sm, clang::LangOptions()));
  return std::string(sm->getCharacterData(b),
                     sm->getCharacterData(e) - sm->getCharacterData(b));
}

inline std::string decl2str(const clang::Decl *d,
                            const clang::SourceManager *sm) {
  return getSourceAsString(d->getSourceRange(), sm);
}

inline std::vector<std::string>
getCommandlineArgs(clang::tooling::CommonOptionsParser &parser) {
  clang::tooling::CompilationDatabase &c = parser.getCompilations();
  clang::tooling::CompileCommand myC = c.getCompileCommands("")[0];
  std::vector<std::string> ToolCommandLine;
  std::copy(&myC.CommandLine[1], &myC.CommandLine[myC.CommandLine.size() - 1],
            std::back_inserter(ToolCommandLine));
  return ToolCommandLine;
}

// Just for parsing integers inside op_arg_dat()
inline int getIntValFromExpr(const clang::Expr *expr) {
  // check for - in case of direct Kernels
  if (const clang::UnaryOperator *idxOp =
          llvm::dyn_cast<clang::UnaryOperator>(expr)) {
    if (idxOp->getOpcode() == clang::UO_Minus) {
      if (const clang::IntegerLiteral *operand =
              llvm::dyn_cast<clang::IntegerLiteral>(idxOp->getSubExpr())) {
        int val = operand->getValue().getLimitedValue(INT_MAX);
        return -1 * val;
      } else {
        llvm::errs() << "Minus applied to Non-IntegerLiteral\n";
      }
    } else {
      llvm::errs() << "Unexpected Unary OP\n";
    }

  } else if (const clang::IntegerLiteral *intLit =
                 llvm::dyn_cast<clang::IntegerLiteral>(expr)) {
    int val = intLit->getValue().getLimitedValue(INT_MAX);
    if (val == INT_MAX) {
      llvm::errs() << "IntegerLiteral exceeds INT_MAX\n";
    }
    return val;
  }
  assert(false && "Failed to get integer literal from expression.");
  return INT_MAX;
}

template <typename T> inline const T *getExprAsDecl(const clang::Expr *expr) {
  if (const clang::DeclRefExpr *declRefExpr =
          llvm::dyn_cast<clang::DeclRefExpr>(expr->IgnoreCasts())) {
    const T *decl = llvm::dyn_cast<T>(declRefExpr->getFoundDecl());
    assert(decl);
    return decl;
  }
  if (const clang::DeclRefExpr *declRefExpr =
          llvm::dyn_cast<clang::DeclRefExpr>(*(expr->child_begin()))) {
    const T *decl = llvm::dyn_cast<T>(declRefExpr->getFoundDecl());
    assert(decl);
    return decl;
  }
  expr->dumpColor();
  assert(false && "Failed to get Decl from Expr.");
  return nullptr;
}

inline const clang::StringLiteral *getAsStringLiteral(const clang::Expr *expr) {
  if (auto str = llvm::dyn_cast<clang::StringLiteral>(expr))
    return str;

  auto cast = llvm::dyn_cast<clang::CastExpr>(expr);
  if (!cast)
    return nullptr;
  return llvm::dyn_cast<clang::StringLiteral>(cast->getSubExpr());
}

inline bool isStringLiteral(const clang::Expr &expr) {
  return getAsStringLiteral(&expr);
}

template <unsigned N>
clang::DiagnosticBuilder reportDiagnostic(
    const clang::ASTContext &Context, const clang::Expr *expr,
    const char (&FormatString)[N],
    clang::DiagnosticsEngine::Level level = clang::DiagnosticsEngine::Error) {
  clang::DiagnosticsEngine &DiagEngine = Context.getDiagnostics();
  auto DiagID = DiagEngine.getCustomDiagID(level, FormatString);
  auto SourceRange = expr->getSourceRange();
  auto report = DiagEngine.Report(SourceRange.getBegin(), DiagID);
  report.AddSourceRange({SourceRange, true});
  return report;
}

inline llvm::raw_ostream &debugs() {
#ifndef NDEBUG
  return llvm::errs();
#else
  return llvm::nulls();
#endif
}

template <typename F>
class MatchMaker : public clang::ast_matchers::MatchFinder::MatchCallback {
private:
  F matchFunction;

public:
  MatchMaker(F f) : matchFunction{f} {}
  virtual void
  run(const clang::ast_matchers::MatchFinder::MatchResult &Result) override {
    matchFunction(Result);
  }
};

template <typename F> MatchMaker<F> make_matcher(F matchFunction) {
  return MatchMaker<F>{matchFunction};
}

template <typename T>
const T *findParent(const clang::Stmt &stmt, clang::ASTContext &context) {
  auto vec = context.getParents(stmt);

  if (vec.empty())
    return nullptr;

  if (const T *t = vec[0].get<T>()) {
    return t;
  }

  const clang::Stmt *pStmt = vec[0].get<clang::Stmt>();
  if (pStmt) {
    return findParent<T>(*pStmt, context);
  }

  return nullptr;
}

} // namespace OP2
#endif // end of header guard
