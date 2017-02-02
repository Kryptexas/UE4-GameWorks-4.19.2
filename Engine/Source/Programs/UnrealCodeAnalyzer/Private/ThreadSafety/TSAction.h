// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Common.h"

THIRD_PARTY_INCLUDES_START
	#pragma pack(push, 8)
		#include "clang/Frontend/FrontendAction.h"
	#pragma pack(pop)
THIRD_PARTY_INCLUDES_END

namespace UnrealCodeAnalyzer
{
	class FTSConsumer;

	class FTSAction : public clang::ASTFrontendAction
	{
	public:
		virtual clang::ASTConsumer* CreateASTConsumer(clang::CompilerInstance& Compiler, clang::StringRef InFile) override;
		virtual bool BeginSourceFileAction(clang::CompilerInstance &CI, clang::StringRef Filename) override;
		virtual void EndSourceFileAction() override;

	private:
		FTSConsumer* Consumer;
		clang::StringRef Filename;
		clang::SourceManager* SourceManager;
	};
}
