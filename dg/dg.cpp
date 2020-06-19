
/*first line intentionally left blank.*/
/**********************************************************************************************************************/
/*included modules*/
/*project headers*/
/*standard headers*/
#include <cassert>
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtVisitor.h>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <llvm/Support/Casting.h>
#include <memory>
#include <string>
#include <vector>
/*LLVM headers*/
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/LLVM.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"
/*python headers*/
#include <Python.h>
/**********************************************************************************************************************/
/*used namespaces*/
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;
/**********************************************************************************************************************/
#if __clang_major__ <= 6
#define DEVI_GETLOCSTART getLocStart
#define DEVI_GETLOCEND getLocEnd
#elif __clang_major__ >= 8
#define DEVI_GETLOCSTART getBeginLoc
#define DEVI_GETLOCEND getEndLoc
#endif

#define RED "\033[1;31m"
#define CYAN "\033[1;36m"
#define GREEN "\033[1;32m"
#define BLUE "\033[1;34m"
#define BLACK "\033[1;30m"
#define BROWN "\033[1;33m"
#define MAGENTA "\033[1;35m"
#define GRAY "\033[1;37m"
#define DARKGRAY "\033[1;30m"
#define YELLOW "\033[1;33m"
#define NORMAL "\033[0m"
#define CLEAR "\033[2J"

#define  PRINT_WITH_COLOR(X,Y) \
  do{\
    printf(X Y NORMAL);\
  }while(0)

#define  PRINT_WITH_COLOR_LB(X,Y) \
  do{\
    printf(X "%s", Y);\
    printf(NORMAL"\n");\
  }while(0)
/**********************************************************************************************************************/
namespace {
static llvm::cl::OptionCategory DGCat("Obfuscator custom options");
}
/**********************************************************************************************************************/
class PyExec {
  public:
    PyExec(std::string __py_script_name, std::string __py_func_name, std::string __obj_path ) :
      py_script_name(__py_script_name), py_func_name(__py_func_name), obj_path(__obj_path) {}
    ~PyExec() {
      Py_Finalize();
    }

    int run(void) {
      Py_Initialize();

      int argc = 2;
      wchar_t* argv[2];
      argv[0] = Py_DecodeLocale(py_script_name.c_str(), 0);
      argv[1] = Py_DecodeLocale(obj_path.c_str(), 0);

      char buf[500];
      std::string bruiser_path;
      // @DEVI-linux-only
      int len = readlink("/proc/self/exe", buf, 499);
      if (len != -1) {
        buf[len] = '\0';
        bruiser_path = buf;
        auto index = bruiser_path.rfind("/");
        bruiser_path = bruiser_path.substr(0, index);
      }

      PySys_SetArgv(argc, argv);
      pName = PyUnicode_DecodeFSDefault(py_script_name.c_str());
      std::string command_1 = "import sys\nsys.path.append(\"" + bruiser_path + "/../bfd\")\n";
      std::string command_2 = "sys.path.append(\"" + bruiser_path + "/wasm\")\n";
      PyRun_SimpleString(command_1.c_str());
      PyRun_SimpleString(command_2.c_str());
      pModule = PyImport_Import(pName);
      Py_DECREF(pName);

      if (pModule != nullptr) {
        pFunc = PyObject_GetAttrString(pModule, py_func_name.c_str());
        if (pFunc && PyCallable_Check(pFunc)) {
          std::cout << GREEN << "function is callable." << NORMAL << "\n";
          pArgs = PyTuple_New(1);
          pValue = PyUnicode_FromString(obj_path.c_str());
          PyTuple_SetItem(pArgs, 0, pValue);
          //Py_DECREF(pArgs);
          //pArgs = nullptr;
          std::cout << BLUE << "calling python function..." << NORMAL << "\n";
          //pValue = PyObject_CallObject(pFunc, pArgs);
          pValue = PyObject_CallObject(pFunc, nullptr);
          if (pValue != nullptr) {
            std::cout << GREEN << "call finished successfully." << NORMAL << "\n";
          } else {
            Py_DECREF(pFunc);
            Py_DECREF(pModule);
            PyErr_Print();
            std::cout << RED << "Call failed." << NORMAL << "\n";
            return EXIT_FAILURE;
          }
        }
      else {
        if (PyErr_Occurred()) PyErr_Print();
        fprintf(stderr, "Cannot find function\"%s\"\n", py_func_name.c_str());
      }
      //Py_XDECREF(pFunc);
      //Py_DECREF(pModule);
    }
    else {
      PyErr_Print();
      fprintf(stderr, "Failed to load \"%ls\"\n", argv[0]);
      return 1;
    }
    //Py_Finalize();
    return 0;
    }

    void convertNPush(PyObject* pyobject) {}

    int64_t pyInt(PyObject* po) {return PyLong_AsLong(po);}

    double pyFloat(PyObject* po) {return PyFloat_AsDouble(po);}

    std::vector<PyObject*> pyList_unpack(PyObject* po) {
      std::vector<PyObject*> dummy;
      if (PyList_Check(po)) {
        int size = PyList_Size(po);
        for (int i = 0; i < size; ++i) {
          dummy.push_back(PyList_GetItem(po, i));
        }
      } else {
        PRINT_WITH_COLOR_LB(RED, "Not a PyList object.");
      }
      return dummy;
    }

    std::string pyString(PyObject* po) {
      return PyBytes_AsString(PyUnicode_AsEncodedString(PyObject_Repr(po), "utf-8", "surrogateescape"));
    }

    std::pair<std::vector<PyObject*>, std::vector<PyObject*>> pyDict_unpack(PyObject* po) {
      std::vector<PyObject*> Keys, Values;
      if (PyDict_Check(po)) {
        Keys = pyList_unpack(PyDict_Keys(po));
        Values = pyList_unpack(PyDict_Values(po));
      } else {
        PRINT_WITH_COLOR_LB(RED, "Not a PyDict object.");
      }
      return std::make_pair(Keys, Values);
    }

    char* pyBytes(PyObject* po) {
      char* dummy;
      if (PyBytes_Check(po)) {
        dummy = PyBytes_AsString(po);
        return dummy;
      } else {
        PRINT_WITH_COLOR_LB(RED, "Not a PyBytes object.");
      }
      return nullptr;
    }

    char* pyByteArray(PyObject* po) {
      char* dummy;
      if (PyByteArray_Check(po)) {
        dummy = PyByteArray_AsString(po);
        return dummy;
      } else {
        PRINT_WITH_COLOR_LB(RED, "Not a PyByteArray object.");
      }
      return nullptr;
    }

    int getAsCppStringVec(void) {
      PRINT_WITH_COLOR_LB(BLUE, "processing return result...");
      if (PyList_Check(pValue)) {
        std::cout << GREEN << "got a python list\n" << NORMAL;
        int list_length = PyList_Size(pValue);
        std::cout << BLUE << "length of list: " << list_length << NORMAL  <<"\n";
        for (int i = 0; i < list_length; ++i) {
          PyObject* pybytes = PyList_GetItem(pValue, i);
          if (pybytes == nullptr) PRINT_WITH_COLOR_LB(RED, "getascppstringvec:failed to get py list item.");
          PyObject* pyrepr = PyObject_Repr(pybytes);
          PyObject* pyunicode = PyUnicode_AsEncodedString(pyrepr, "utf-8", "surrogateescape");
          const char* dummy = PyBytes_AsString(pyunicode);
          //std::cout << RED << dummy << "\n" << NORMAL;
          hexobj_str.push_back(std::string(dummy));
        }
      }
      return 0;
    }

    int getAsCppByte_PyIntList(void) {
      if(PyList_Check(pValue)) {
        int list_length = PyList_Size(pValue);
        for(int i = 0; i < list_length; ++i) {
          PyObject* pybytes = PyList_GetItem(pValue, i);
          if (PyLong_Check(pybytes)) {
            text_section.push_back(PyLong_AsLong(pybytes));
          }
        }
      }
      return 0;
    }

    int getAsCppByte(void) {
      PRINT_WITH_COLOR_LB(BLUE, "processing return result...");
      std::vector<uint8_t> tempvec;
      if(PyList_Check(pValue)) {
        int list_length = PyList_Size(pValue);
        std::cout << BLUE << "length of list: " << list_length << NORMAL << "\n";
        for(int i = 0; i < list_length; ++i) {
          PyObject* pybytes = PyList_GetItem(pValue, i);
          if(PyList_Check(pybytes)) {
            int list_length_2 = PyList_Size(pybytes);
            for(int j = 0; j < list_length_2; ++j) {
              PyObject* dummy_int = PyList_GetItem(pybytes, j);
              if (PyLong_Check(dummy_int)) {
                unsigned char byte = PyLong_AsLong(dummy_int);
                tempvec.push_back(int(byte));
              }
            }
            //if (!tempvec.empty()) {hexobj.push_back(tempvec);}
            hexobj.push_back(tempvec);
            tempvec.clear();
          }
        }
      }
      return 0;
    }

    int getWasmModule(void) {
      return 0;
    }

    void killPyObj(void) {
      Py_DECREF(pValue);
    }

    void printHexObjs(void) {
        PRINT_WITH_COLOR_LB(YELLOW, "functions with a zero size will not be printed:");
        for (auto &iter : hexobj) {
          for (auto &iterer : iter) {
            std::cout << RED << std::hex << int(iterer) << " ";
          }
          std::cout << "\n" << NORMAL;
        }
    }

    std::vector<std::vector<uint8_t>> exportObjs(void) {return hexobj;}
    std::vector<std::string> exportStrings(void) {return hexobj_str;}
    std::vector<std::uint8_t> exportTextSection(void) {return text_section;}

    void getVarargs(std::vector<void*> _varargs) {varargs = _varargs;}

  private:
    std::string py_script_name;
    std::string py_func_name;
    std::string obj_path;
    PyObject* pName;
    PyObject* pModule;
    PyObject* pFunc;
    PyObject* pArgs;
    PyObject* pValue;
    std::vector<std::string> hexobj_str;
    std::vector<std::vector<uint8_t>> hexobj;
    std::vector<uint8_t> text_section;
    std::vector<void*> varargs;
};
/**********************************************************************************************************************/
SourceLocation SourceLocationHasMacro(SourceLocation sl, Rewriter &rewrite) {
  if (sl.isMacroID()) {
    return rewrite.getSourceMgr().getSpellingLoc(sl);
  } else {
    return sl;
  }
}

SourceLocation getSLSpellingLoc(SourceLocation sl, Rewriter &rewrite) {
  if (sl.isMacroID()) return rewrite.getSourceMgr().getSpellingLoc(sl);
  else return sl;
}
/**********************************************************************************************************************/
class StatementVisitor : public clang::ConstStmtVisitor<StatementVisitor> {
  public:
    StatementVisitor() = default;

    bool Visit(const Stmt* Node) {
      ConstStmtVisitor<StatementVisitor>::Visit(Node);
      return true;
    }

    bool VisitCallExpr(const CallExpr *Node) {
      llvm::outs() << "visited a callexpr node\n";
      return true;
    }

    bool VisitIfStmt(const IfStmt* Node) {
      llvm::outs() << "visited an IfStmt\n";
      return true;
    }
};
/**********************************************************************************************************************/
class FuncDeclMatchCallback : public MatchFinder::MatchCallback {
public:
  explicit FuncDeclMatchCallback(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &MR) {
    const FunctionDecl *FD = MR.Nodes.getNodeAs<clang::FunctionDecl>("funcdecl");
    if (false == FD->isThisDeclarationADefinition()) return void();
    if (false == MR.Context->getSourceManager().isInMainFile(FD->getLocation())) return void();
    if (true == MR.Context->getSourceManager().isInSystemHeader(FD->getLocation())) return void();

    auto Body = FD->getBody();
    if (nullptr == Body) return void();

    if (Body->child_begin() == Body->child_end()) return void();

    StatementVisitor SV;
    /* SV.Visit(Body); */

    for (auto &iter : Body->children()) {
      /* iter->dumpColor(); */
      clang::Stmt::StmtClass SC = iter->getStmtClass();

      if (clang::Stmt::StmtClass::CallExprClass == SC) {
        const CallExpr *CE = llvm::dyn_cast<CallExpr>(iter);
        llvm::outs() << CE->getDirectCallee()->getName().str() << "\t";
        for (auto &&iter : CE->arguments()) {
          llvm::outs() << Rewrite.getRewrittenText(iter->getSourceRange()) << "\t";
        }
        llvm::outs() << "\n";
      }
    }
  }

private:
  Rewriter &Rewrite;
};
/**********************************************************************************************************************/
class PPInclusion : public PPCallbacks
{
public:
  explicit PPInclusion (SourceManager *SM, Rewriter *Rewrite) : SM(*SM), Rewrite(*Rewrite) {}

  virtual void MacroDefined(const Token &MacroNameTok, const MacroDirective *MD) {}

  virtual void MacroExpands (const Token &MacroNameTok, const MacroDefinition &MD, SourceRange Range, const MacroArgs *Args) {}

  virtual void  InclusionDirective (SourceLocation HashLoc, const Token &IncludeTok,
      StringRef FileName, bool IsAngled, CharSourceRange FilenameRange, const FileEntry *File,
      StringRef SearchPath, StringRef RelativePath, const clang::Module *Imported) {}

private:
  const SourceManager &SM;
  Rewriter &Rewrite;
};
/**********************************************************************************************************************/
/**
 * @brief A Clang Diagnostic Consumer that does nothing.
 */
class BlankDiagConsumer : public clang::DiagnosticConsumer
{
  public:
    BlankDiagConsumer() = default;
    virtual ~BlankDiagConsumer() {}
    virtual void HandleDiagnostic(DiagnosticsEngine::Level DiagLevel, const Diagnostic &Info) override {}
};
/**********************************************************************************************************************/
class DGASTConsumer : public ASTConsumer {
public:
  DGASTConsumer(Rewriter &R)
      : funcDeclHandler(R) {
    Matcher.addMatcher(functionDecl().bind("funcdecl"), &funcDeclHandler);
  }

  void HandleTranslationUnit(ASTContext &Context) override {
    Matcher.matchAST(Context);
  }

private:
  FuncDeclMatchCallback funcDeclHandler;
  MatchFinder Matcher;
};
/**********************************************************************************************************************/
class DGFrontendAction : public ASTFrontendAction {
public:
  DGFrontendAction() {}
  ~DGFrontendAction() {
    delete BDCProto;
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
#if __clang_major__ >= 10
    CI.getPreprocessor().addPPCallbacks(std::make_unique<PPInclusion>(&CI.getSourceManager(), &TheRewriter));
#elif
    CI.getPreprocessor().addPPCallbacks(llvm::make_unique<PPInclusion>(&CI.getSourceManager(), &TheRewriter));
#endif
    DiagnosticsEngine &DE = CI.getPreprocessor().getDiagnostics();
    DE.setClient(BDCProto, false);
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
#if __clang_major__ >= 10
    return std::make_unique<DGASTConsumer>(TheRewriter);
#elif
    return llvm::make_unique<MyASTConsumer>(TheRewriter);
#endif
  }

private:
  BlankDiagConsumer* BDCProto = new BlankDiagConsumer;
  Rewriter TheRewriter;
};
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/*Main*/
int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, DGCat);
  const std::vector<std::string> &SourcePathList = op.getSourcePathList();
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());
  return Tool.run(newFrontendActionFactory<DGFrontendAction>().get());
}
/**********************************************************************************************************************/
/*last line intentionally left blank.*/

