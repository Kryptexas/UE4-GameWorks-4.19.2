// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Common.h"
#include "TSVisitor.h"

THIRD_PARTY_INCLUDES_START
	#pragma pack(push, 8)
		#include "clang/AST/ASTConsumer.h"
	#pragma pack(pop)
THIRD_PARTY_INCLUDES_END

namespace UnrealCodeAnalyzer
{
	class FTSConsumer : public clang::ASTConsumer
	{
	public:
		explicit FTSConsumer(clang::ASTContext* Context, clang::StringRef InFile);

		virtual void HandleTranslationUnit(clang::ASTContext &Ctx) override;

	private:
		FTSVisitor Visitor;
	};
}
