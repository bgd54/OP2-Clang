#include "OMP4KernelHandler.h"
#include "generators/common/ASTMatchersExtension.h"
#include "generators/common/handler.hpp"
#include "clang/AST/StmtOpenMP.h"
#include "generators/openmp4/OMP4UserFuncTransformator.hpp"

namespace {
using namespace clang::ast_matchers;
const auto parLoopSkeletonCompStmtMatcher =
    compoundStmt(hasParent(functionDecl(hasName("op_par_loop_skeleton"))));
} // namespace

namespace OP2 {
using namespace clang::ast_matchers;
using namespace clang;
//___________________________________MATCHERS__________________________________

const StatementMatcher OMP4KernelHandler::locRedVarMatcher =
    declStmt(containsDeclaration(0, varDecl(hasName("arg0_l"))),
             hasParent(parLoopSkeletonCompStmtMatcher))
        .bind("local_reduction_variable");

const StatementMatcher OMP4KernelHandler::locRedToArgMatcher =
    binaryOperator(
        hasOperatorName("="),
        hasRHS(ignoringImpCasts(declRefExpr(to(varDecl(hasName("arg0_l")))))),
        hasParent(parLoopSkeletonCompStmtMatcher))
        .bind("loc_red_to_arg_assignment");

const StatementMatcher OMP4KernelHandler::ompParForMatcher =
    ompParallelForDirective().bind(
        "ompParForDir"); // FIXME check if it is in the main file.

const DeclarationMatcher OMP4KernelHandler::userFuncMatcher =
    functionDecl(hasName("skeleton_OMP4"), isDefinition(), parameterCountIs(1))
        .bind("user_func_OMP4");

const StatementMatcher OMP4KernelHandler::funcCallMatcher =
    callExpr(callee(functionDecl(hasName("skeleton_OMP4"), parameterCountIs(1))))
        .bind("func_call_OMP4");

        
const DeclarationMatcher OMP4KernelHandler::mapIdxDeclMatcher =
    varDecl(hasName("mapStart"), hasAncestor(parLoopSkeletonCompStmtMatcher))
        .bind("mapStart_decl");


//_________________________________CONSTRUCTORS________________________________
OMP4KernelHandler::OMP4KernelHandler(const clang::tooling::CompilationDatabase &Compilations,
    std::map<std::string, clang::tooling::Replacements> *Replace,
    const ParLoop &loop, const OP2Application &application, const size_t loopIdx)
    : Compilations(Compilations), Replace(Replace), loop(loop), application(application), loopIdx(loopIdx) {}

//________________________________GLOBAL_HANDLER_______________________________
void OMP4KernelHandler::run(const MatchFinder::MatchResult &Result) {

  if (!lineReplHandler<FunctionDecl, 1>(Result, Replace, "user_func_OMP4",  [this]() {
        std::string retStr="";
        const ParLoop &loop = this->application.getParLoops()[loopIdx];
        std::string hostFuncText = loop.getUserFuncInc();
        std::vector<std::string> path = {"/tmp/omp4.cpp"};
        

        loop.dumpFuncTextTo(path[0]);

        std::string SOAresult =
            OMP4UserFuncTransformator(Compilations, loop, application, const_list, kernel_arg_name, path).run();
        if (SOAresult != "")
          hostFuncText = SOAresult;
        retStr = hostFuncText.substr(hostFuncText.find("{")) + "}\n "+ this->AssignbackReduction() + "}\n";
        return this->getmappedFunc() +  retStr;
      }))
    return; // if successfully handled return

  if (!lineReplHandler<VarDecl, 9>(Result, Replace, "mapStart_decl",  [this]() {
        return this->DevicePointerDecl();
      }))
    return; // if successfully handled return

  if (!lineReplHandler<DeclStmt, 1>(
          Result, Replace, "local_reduction_variable",
          std::bind(&OMP4KernelHandler::handleRedLocalVarDecl, this)))
    return;
  if (!HANDLER(CallExpr, 2, "func_call_OMP4", OMP4KernelHandler::handleFuncCall))
    return; // if successfully handled return
  if (!lineReplHandler<BinaryOperator, 7>(
          Result, Replace, "loc_red_to_arg_assignment",
          std::bind(&OMP4KernelHandler::handlelocRedToArgAssignment, this)))
    return; // if successfully handled return
  if (!HANDLER(OMPParallelForDirective, 0, "ompParForDir",
               OMP4KernelHandler::handleOMPParLoop)) {
    return;
  }
}
//___________________________________HANDLERS__________________________________

std::string OMP4KernelHandler::handleFuncCall() {
  std::string funcCall = "";
  llvm::raw_string_ostream ss(funcCall);
  std::map<std::string,std::string> arg2data;
  ss << loop.getName() << "_omp4_kernel(";
  for(size_t i = 0; i < loop.getNumArgs(); ++i){
      if(!loop.getArg(i).isGBL){
        if(!loop.getArg(i).isDirect() && arg2data[loop.getArg(i).opMap] == ""){
          ss << "map" << i << ",\n"; 
          ss << "map" << i << "size,\n";
        }
        if(arg2data[loop.getArg(i).opDat] == ""){
          ss << "data" << i << ",\n";
          ss << "data" << i << "size,\n";
        }
      } else {
        ss << "&arg" << i << "_l,\n";
      } 
      arg2data[loop.getArg(i).opMap] = std::to_string(i);
      arg2data[loop.getArg(i).opDat] = std::to_string(i);
  }
  if(loop.isDirect()){
    ss << "set->size,\n part_size != 0 ? (set->size - 1) / part_size + 1: (set->size - 1) / nthread,\n nthread);";
  } else {
    ss << "col_reord,\n set_size1,\n start,\n end,\n part_size != 0 ? (end - start - 1) / part_size + 1: (end - start - 1) / nthread,\n nthread);";
  }
  return ss.str();
}

std::string OMP4KernelHandler::getmappedFunc(){
  std::string mappedfunc = "";
  llvm::raw_string_ostream ss(mappedfunc);
  std::map<std::string,std::string> arg2data;
  ss << "void " << loop.getName() << "_omp4_kernel(";
  for(size_t i = 0; i < loop.getNumArgs(); ++i){
      if(!loop.getArg(i).isGBL){
        if(!loop.getArg(i).isDirect() && arg2data[loop.getArg(i).opMap] == ""){
          ss << "int *map" << i << ",\n"; 
          ss << "int map" << i << "size,\n";
        }
        if(arg2data[loop.getArg(i).opDat] == ""){
          ss << loop.getArg(i).type << " *" << "data" << i << ",\n";
          ss << "int " << "data" << i << "size,\n";
        }
      } else {
        ss << loop.getArg(i).type << " *" << "arg"  << i << ", ";
      }
      arg2data[loop.getArg(i).opMap] = std::to_string(i);
      arg2data[loop.getArg(i).opDat] = std::to_string(i);
  }

  if(!loop.isDirect()){
    ss << "int *col_reord, int set_size1, int start, int end, int num_teams, int nthread) {\n";
  } else {
    ss << " int count, int num_teams, int nthread) {\n";
  }

  for (size_t i = 0; i < loop.getNumArgs(); ++i){
    if(loop.getArg(i).isGBL){
      ss << loop.getArg(i).type << " arg" << i << "_l = *arg" << i << ";\n";
    }
  }

  // ss << "#pragma omp distribute parallel for schedule(static, 1)\n";
  ss << "#pragma omp target teams distribute parallel for schedule(static, 1) num_teams(num_teams) thread_limit(nthread) ";
  ss << "map(to: ";

  // int j =0, int k = 0;
  arg2data.clear();
  int  j = 0, k = 0;
  for(size_t i = 0; i < loop.getNumArgs(); ++i){
        if(!loop.getArg(i).isGBL){
          if(!loop.getArg(i).isDirect() && arg2data[loop.getArg(i).opMap] == ""){
            if(k|j) {ss << ", "; k = 0; j =0;}
            ss << "map" << i << "[0:"; 
            ss << "map" << i << "size]";
            j++;
          }
          
          if(arg2data[loop.getArg(i).opDat] == ""){
            if(k|j) {ss << ", "; j = 0; j = 0;}
            ss << "data" << i << "[0: ";
            ss << "data" << i << "size] ";
            k++;
          }
        } 
        arg2data[loop.getArg(i).opMap] = std::to_string(i);
        arg2data[loop.getArg(i).opDat] = std::to_string(i);
  }

  if(!loop.isDirect()){
    ss << " ,col_reord[0 : set_size1]) ";
  } else {
    ss << ") ";
  }

  if(const_list.size() != 0){
    ss << " map(to : ";
    for(auto it = const_list.begin(); it < const_list.end(); it++){
        ss << *it;
        for (auto it_g = application.constants.begin(); it_g != application.constants.end(); it_g++){
          if(it_g->name == *it && it_g->size > 1){
            ss << "[0:" << it_g->size -1<< "]"; 
          }
        }
        if(it != const_list.end() - 1){
          ss << ", ";
        }
    }
    ss << ") ";
  }

  for (size_t i = 0; i < loop.getNumArgs(); ++i){
    if(loop.getArg(i).isGBL){
      ss << " map(tofrom:"  << " arg" << i << "_l) reduction(";
      if(loop.getArg(i).accs == 3)
        ss << "+";
      else if(loop.getArg(i).accs == 4)
        ss << "min";
      else if(loop.getArg(i).accs == 5)
        ss << "max";

      ss << ": " << " arg" << i << "_l)";
    }
  }

  ss << "\n";
  
  if(!loop.isDirect()){
    ss << "for ( int e=start; e<end; e++ ){\n";
    ss << "int n_op = col_reord[e];\n";
  } else {
    ss << "for ( int n_op=0; n_op<count; n_op++ ){\n";
  }


  arg2data.clear();
  for(size_t i = 0; i < loop.getNumArgs(); ++i){
      if(!loop.getArg(i).isGBL){
        if(!loop.getArg(i).isDirect()){
          if(arg2data[loop.getArg(i).opMap] == ""){
            arg2data[loop.getArg(i).opMap] = "map" + std::to_string(i);
            ss << "int " << loop.getArg(i).opDat << "_map" << i << "idx"  << "= map" << i << "[n_op + set_size1 * " <<  loop.getArg(i).idx << "];\n"; 
          } else {
            ss << "int " << loop.getArg(i).opDat  << "_map" << i << "idx" << " = " << arg2data[loop.getArg(i).opMap] << "[n_op + set_size1 * " <<  loop.getArg(i).idx << "];\n";
          }
          if(arg2data[loop.getArg(i).opDat]== "")
            arg2data[loop.getArg(i).opDat] = "data" + std::to_string(i);
        }
    }
  }



  // for (size_t i = 0; i < loop.getNumArgs(); ++i) {
  //   if(!loop.getArg(i).isDirect()){
  //     ss << "int " << loop.getArg(i).opDat << "_map" << i << "idx = map0[n_op + set_size1 + " << loop.getArg(i).idx << "];\n";
  //   }
  // }


  ss << "//variable mapping\n";
  for (size_t i = 0; i < loop.getNumArgs(); ++i){
    ss << kernel_arg_name[i];
    if(!loop.getArg(i).isDirect() && !loop.getArg(i).isGBL){
      ss << " = &" << arg2data[loop.getArg(i).opDat] << "[" << loop.getArg(i).dim << " * " <<  loop.getArg(i).opDat << "_map" << i << "idx];\n";
    } else if (!loop.getArg(i).isGBL) {
      ss << " = &" << "data" << i << "[" << loop.getArg(i).dim << " * n_op];\n";
    }
    else if(loop.getArg(i).isGBL){
      ss << " = &arg" << i << "_l;\n";
    }
  }

  return ss.str();
}

std::string OMP4KernelHandler::DevicePointerDecl(){
  std::string DPD = "";
  llvm::raw_string_ostream ss(DPD);
  std::map<std::string,std::string> arg2data;
  for(size_t i = 0; i < loop.getNumArgs(); ++i){
      if(!loop.getArg(i).isGBL){
        if(!loop.getArg(i).isDirect() && arg2data[loop.getArg(i).opMap] == ""){
          ss << "int *map" << i << "= arg" << i << ".map_data_d;\n"; 
          ss << "int map" << i << "size = arg" << i << ".map->dim * set_size1;\n\n";
        }
        if(arg2data[loop.getArg(i).opDat] == ""){
          ss << loop.getArg(i).type << " *" << "data" << i << " = ";
          ss << "(" << loop.getArg(i).type << "*)" <<"arg" << i << ".data_d;\n";

          ss << "int " << "data" << i << "size" << " = getSetSizeFromOpArg(&arg" << i;
          ss << ") * arg" << i << ".dat->dim;\n\n";
        }
      }
      arg2data[loop.getArg(i).opMap] = std::to_string(i);
      arg2data[loop.getArg(i).opDat] = std::to_string(i);
  }
  return ss.str();
}

std::string OMP4KernelHandler::handleRedLocalVarDecl() {
  std::string s;
  llvm::raw_string_ostream os(s);
  for (size_t ind = 0; ind < loop.getNumArgs(); ++ind) {
    const OPArg &arg = loop.getArg(ind);
    if (arg.isReduction()) {
      os << arg.type << " arg" << ind << "_l = *(" + arg.type + " *)arg" << ind
         << ".data;\n";
    }
  }
  return os.str();
}

std::string OMP4KernelHandler::AssignbackReduction() {
  std::string s;
  llvm::raw_string_ostream os(s);
  for (size_t ind = 0; ind < loop.getNumArgs(); ++ind) {
    const OPArg &arg = loop.getArg(ind);
    if (arg.isReduction()) {
      os << "*arg" << ind << "= " << "arg" << ind << "_l;\n";
    }
  }
  return os.str();
}

std::string OMP4KernelHandler::handlelocRedToArgAssignment() {
  std::string s;
  llvm::raw_string_ostream os(s);
  for (size_t ind = 0; ind < loop.getNumArgs(); ++ind) {
    const OPArg &arg = loop.getArg(ind);
    if (arg.isReduction()) {
      os << "*((" + arg.type + " *)arg" << ind << ".data) = arg" << ind
         << "_l;\n";
    }
  }
  return os.str();
}

std::string OMP4KernelHandler::handleOMPParLoop() {
  std::string plusReds, minReds, maxReds;
  llvm::raw_string_ostream os(plusReds);
  llvm::raw_string_ostream osMin(minReds);
  llvm::raw_string_ostream osMax(maxReds);
  for (size_t ind = 0; ind < loop.getNumArgs(); ++ind) {
    const OPArg &arg = loop.getArg(ind);
    if (arg.isReduction()) {
      switch (arg.accs) {
      case OP2::OP_INC:
        os << "arg" << ind << "_l, ";
        break;
      case OP2::OP_MAX:
        osMax << "arg" << ind << "_l, ";
        break;
      case OP2::OP_MIN:
        osMin << "arg" << ind << "_l, ";
        break;
      default:
        // error if this is a reduction it must be one of OP_MIN, OP_MAX or
        // OP_INC
        assert(!arg.isReduction() ||
               (arg.accs == OP2::OP_INC || arg.accs == OP2::OP_MAX ||
                arg.accs == OP2::OP_MIN));
      }
    }
  }
  if (os.str().length() > 0) {
    plusReds =
        " reduction(+:" + plusReds.substr(0, plusReds.length() - 2) + ")";
  }
  if (osMin.str().length() > 0) {
    minReds = " reduction(min:" + minReds.substr(0, minReds.length() - 2) + ")";
  }
  if (osMax.str().length() > 0) {
    maxReds = " reduction(max:" + maxReds.substr(0, maxReds.length() - 2) + ")";
  }
  return "omp parallel for " + plusReds + " " + minReds + " " + maxReds;
}

} // namespace OP2
