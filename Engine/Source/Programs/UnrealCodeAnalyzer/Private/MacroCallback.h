// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

THIRD_PARTY_INCLUDES_START
	#pragma pack(push, 8)
		#include "clang/Lex/PPCallbacks.h"
	#pragma pack(pop)
THIRD_PARTY_INCLUDES_END

namespace clang
{
	class MacroDirective;
	class Preprocessor;
	class SourceLocation;
	class SourceManager;
	class SourceRange;
	class Token;
}

namespace UnrealCodeAnalyzer
{
	class FMacroCallback : public clang::PPCallbacks
	{
	public:
		FMacroCallback(clang::StringRef Filename, clang::SourceManager& SourceManager, const clang::Preprocessor& Preprocessor);

		virtual void MacroExpands(const clang::Token &MacroNameTok, const clang::MacroDirective *MD, clang::SourceRange Range, const clang::MacroArgs *Args) override;
		virtual void MacroDefined(const clang::Token &MacroNameTok, const clang::MacroDirective *MD) override;
		virtual void If(clang::SourceLocation Loc, clang::SourceRange ConditionRange, ConditionValueKind ConditionValue) override;
		virtual void Elif(clang::SourceLocation Loc, clang::SourceRange ConditionRange, ConditionValueKind ConditionValue, clang::SourceLocation IfLoc) override;
		virtual void Ifdef(clang::SourceLocation Loc, const clang::Token &MacroNameTok, const clang::MacroDirective *MD) override;
		virtual void Ifndef(clang::SourceLocation Loc, const clang::Token &MacroNameTok, const clang::MacroDirective *MD) override;

		FPreHashedIncludeSetMap& GetIncludes()
		{
			return Includes;
		}
	private:

		bool IsMacroDefinedInCommandLine(clang::StringRef Macro);
		bool IsPredefinedMacro(clang::StringRef Macro);
		bool FMacroCallback::IsBuiltInMacro(const clang::MacroDirective* MacroDirective);

		clang::StringRef Filename;
		clang::SourceManager& SourceManager;
		const clang::Preprocessor& Preprocessor;

		TMap<FString, FString> MacroDefinitionToFile;

		FPreHashedIncludeSetMap Includes;

		static ELogVerbosity::Type LogVerbosity;
	};
}