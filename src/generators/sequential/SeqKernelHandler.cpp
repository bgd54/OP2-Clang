#include "SeqKernelHandler.h"
#include "core/utils.h"
#include "generators/common/handler.hpp"

namespace {
using namespace clang::ast_matchers;
const auto parLoopSkeletonCompStmtMatcher =
    compoundStmt(hasParent(functionDecl(hasName("op_par_loop_skeleton"))));
} // namespace

namespace OP2 {
using namespace clang::ast_matchers;
using namespace clang;

//___________________________________MATCHERS__________________________________
const DeclarationMatcher SeqKernelHandler::userFuncMatcher =
    functionDecl(hasName("skeleton"), isDefinition(), parameterCountIs(1))
        .bind("user_func");
const StatementMatcher SeqKernelHandler::funcCallMatcher =
    callExpr(callee(functionDecl(hasName("skeleton"), parameterCountIs(1))))
        .bind("func_call");
const DeclarationMatcher SeqKernelHandler::mapIdxDeclMatcher =
    varDecl(hasName("map0idx"), hasAncestor(parLoopSkeletonCompStmtMatcher))
        .bind("map_idx_decl");

//_________________________________CONSTRUCTORS________________________________
SeqKernelHandler::SeqKernelHandler(
    std::map<std::string, clang::tooling::Replacements> *Replace,
    const OP2Application &app, size_t idx)
    : Replace(Replace), application(app), loopIdx(idx) {}

//________________________________GLOBAL_HANDLER_______________________________
void SeqKernelHandler::run(const MatchFinder::MatchResult &Result) {
  if (!lineReplHandler<FunctionDecl, 1>(Result, Replace, "user_func", [this]() {
        return this->application.getParLoops()[loopIdx].getUserFuncInc();
      }))
    return; // if successfully handled return
  if (!lineReplHandler<CallExpr, 2>(Result, Replace, "func_call", [this]() {
        return this->application.getParLoops()[loopIdx].getFuncCall();
      }))
    return; // if successfully handled return
  if (!lineReplHandler<VarDecl, 2>(Result, Replace, "map_idx_decl", [this]() {
        return this->application.getParLoops()[loopIdx].getMapVarDecls();
      }))
    return; // if successfully handled return
}

//___________________________________HANDLERS__________________________________

} // namespace OP2
