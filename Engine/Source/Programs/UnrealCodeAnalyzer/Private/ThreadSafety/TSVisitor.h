// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Common.h"

THIRD_PARTY_INCLUDES_START
	#pragma pack(push, 8)
		#include "clang/AST/RecursiveASTVisitor.h"
	#pragma pack(pop)
THIRD_PARTY_INCLUDES_END

namespace UnrealCodeAnalyzer
{
	class FTSVisitor : public clang::RecursiveASTVisitor<FTSVisitor>
	{
	public:
		explicit FTSVisitor(clang::ASTContext* InContext, clang::StringRef InFile);
		bool VisitCXXConstructorDecl(clang::CXXConstructorDecl* Decl);
		bool TraverseCXXMethodDecl(clang::CXXMethodDecl* Decl);
		bool VisitDeclRefExpr(clang::DeclRefExpr* Expr);
		bool VisitCallExpr(clang::CallExpr* Expr);

	private:
		bool InheritsFromUObject(clang::CXXConstructorDecl* Decl);
		bool IsGlobalVariable(clang::VarDecl* VarDecl);

		clang::SourceManager& SourceManager;
		TArray<clang::CXXMethodDecl*> FunctionDeclarationsStack;
		TSet<clang::Decl*> VisitedDecls;
	};
}
