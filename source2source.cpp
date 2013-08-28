//-------------------------------------------------------------------------
//
// rewritersample.cpp: Source-to-source transformation sample with Clang,
// using Rewriter - the code rewriting interface.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//
#include <cstdio>
#include <string>
#include <sstream>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Parse/Parser.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace std;


// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
public:
    MyASTVisitor(Rewriter &R)
        : TheRewriter(R)
    {}

    bool VisitStmt(Stmt *s) {
        // Only care about If statements.
        if (isa<IfStmt>(s)) {
            IfStmt *IfStatement = cast<IfStmt>(s);
            Stmt *Then = IfStatement->getThen();

            TheRewriter.InsertText(Then->getLocStart(),
                                   "// the 'if' part\n",
                                   true, true);

            Stmt *Else = IfStatement->getElse();
            if (Else)
                TheRewriter.InsertText(Else->getLocStart(),
                                       "// the 'else' part\n",
                                       true, true);
        }

        return true;
    }

    bool VisitFunctionDecl(FunctionDecl *f) {
        // Only function definitions (with bodies), not declarations.
        if (f->hasBody()) {
            Stmt *FuncBody = f->getBody();

            // Type name as string
            QualType QT = f->getResultType();
            string TypeStr = QT.getAsString();

            // Function name
            DeclarationName DeclName = f->getNameInfo().getName();
            string FuncName = DeclName.getAsString();

            // Add comment before
            stringstream SSBefore;
            SSBefore << "// Begin function " << FuncName << " returning "
                     << TypeStr << "\n";
            SourceLocation ST = f->getSourceRange().getBegin();
            TheRewriter.InsertText(ST, SSBefore.str(), true, true);

            // And after
            stringstream SSAfter;
            SSAfter << "\n// End function " << FuncName << "\n";
            ST = FuncBody->getLocEnd().getLocWithOffset(1);
            TheRewriter.InsertText(ST, SSAfter.str(), true, true);
        }

        return true;
    }

private:
    void AddBraces(Stmt *s);

    Rewriter &TheRewriter;
};


// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer
{
public:
    MyASTConsumer(Rewriter &R)
        : Visitor(R)
    {}

    // Override the method that gets called for each parsed top-level
    // declaration.
    virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
        for (DeclGroupRef::iterator b = DR.begin(), e = DR.end();
             b != e; ++b)
            // Traverse the declaration using our AST visitor.
            Visitor.TraverseDecl(*b);
        return true;
    }

private:
    MyASTVisitor Visitor;
};


int main (int argc, char* argv[])
{
    using clang::CompilerInstance;
    using clang::TargetOptions;
    using clang::TargetInfo;
    using clang::FileEntry;
    using clang::Token;
    using clang::ASTContext;
    using clang::ASTConsumer;
    using clang::Parser;
    using clang::DiagnosticOptions;
    using clang::TextDiagnosticPrinter;

    CompilerInstance ci;
    DiagnosticOptions diagnosticOptions;
    TextDiagnosticPrinter *pTextDiagnosticPrinter =
        new TextDiagnosticPrinter(
            llvm::outs(),
            &diagnosticOptions,
            true);
    ci.createDiagnostics(argc, argv);

    llvm::IntrusiveRefCntPtr<TargetOptions> pto( new TargetOptions());
    pto->Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *pti = TargetInfo::CreateTargetInfo(ci.getDiagnostics(), pto.getPtr());
    ci.setTarget(pti);

    ci.createFileManager();
    ci.createSourceManager(ci.getFileManager());

    ci.getLangOpts().GNUMode = 1;
    ci.getLangOpts().CXXExceptions = 1;
    ci.getLangOpts().RTTI = 1;
    ci.getLangOpts().Bool = 1;
    ci.getLangOpts().CPlusPlus = 1;

    ci.getHeaderSearchOpts().AddPath("/usr/include/c++/4.4.4",
        clang::frontend::Angled,
        false,
        false,
        false);
    ci.getHeaderSearchOpts().AddPath("/usr/include/c++/4.4.4/i686-redhat-linux/",
        clang::frontend::Angled,
        false,
        false,
        false);
    ci.getHeaderSearchOpts().AddPath("/usr/include/c++/4.4.4/backward",
        clang::frontend::Angled,
        false,
        false,
        false);
    ci.getHeaderSearchOpts().AddPath("/usr/local/include",
        clang::frontend::Angled,
        false,
        false,
        false);
    ci.getHeaderSearchOpts().AddPath("/opt/pkg/llvm/lib/clang/3.3/include",
        clang::frontend::Angled,
        false,
        false,
        false);
    ci.getHeaderSearchOpts().AddPath("/usr/include",
        clang::frontend::Angled,
        false,
        false,
        false);

    clang::CompilerInvocation::setLangDefaults(ci.getLangOpts(), clang::IK_CXX);

    ci.createPreprocessor();
    ci.getPreprocessorOpts().UsePredefines = false;

    ci.getPreprocessor().getBuiltinInfo().InitializeBuiltins(
        ci.getPreprocessor().getIdentifierTable(),
        ci.getPreprocessor().getLangOpts());


    ci.createASTContext();

    Rewriter TheRewriter;
    TheRewriter.setSourceMgr(ci.getSourceManager(),ci.getLangOpts());

    MyASTConsumer *astConsumer = new MyASTConsumer(TheRewriter);
    ci.setASTConsumer(astConsumer);

    const FileEntry *pFile = ci.getFileManager().getFile("foobar.cpp");

    ci.getSourceManager().createMainFileID(pFile);
    ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(),
                                             &ci.getPreprocessor());
    clang::ParseAST(ci.getPreprocessor(), astConsumer, ci.getASTContext());

    const RewriteBuffer *RewriteBuf =
        TheRewriter.getRewriteBufferFor(ci.getSourceManager().getMainFileID());

    llvm::outs() << string(RewriteBuf->begin(), RewriteBuf->end());

    ci.getDiagnosticClient().EndSourceFile();

    return 0;
}

