// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "TSAction.h"
#include "TSConsumer.h"

namespace UnrealCodeAnalyzer
{

	clang::ASTConsumer* FTSAction::CreateASTConsumer(clang::CompilerInstance& Compiler, clang::StringRef InFile)
	{
		Filename = InFile;
		SourceManager = &Compiler.getSourceManager();
		Consumer = new FTSConsumer(&Compiler.getASTContext(), InFile);
		return Consumer;
	}

	bool FTSAction::BeginSourceFileAction(clang::CompilerInstance &CI, clang::StringRef Filename)
	{
		return true;
	}

	void FTSAction::EndSourceFileAction()
	{
	}

}
