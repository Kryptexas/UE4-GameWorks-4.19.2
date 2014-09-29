// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// CrossCompilerTool.cpp: Driver for testing compilation of an individual shader

#include "CrossCompilerTool.h"
#include "CrossCompiler.h"
#include "hlslcc.h"
#include "MetalBackend.h"
#include "GlslBackend.h"

#include "RequiredProgramMainCPPInclude.h"

DEFINE_LOG_CATEGORY(LogCrossCompilerTool);

IMPLEMENT_APPLICATION(CrossCompilerTool, "CrossCompilerTool");

namespace CCT
{
	static int32 Run(const FRunInfo& RunInfo)
	{
		ILanguageSpec* Language = nullptr;
		FCodeBackend* Backend = nullptr;

		int32 Flags = 0;

		Flags |= RunInfo.bRunCPP ? 0 : HLSLCC_NoPreprocess;
		Flags |= RunInfo.bCSE ? HLSLCC_ApplyCommonSubexpressionElimination : 0;

		FGlslLanguageSpec GlslLanguage(RunInfo.Target == HCT_FeatureLevelES2);
		FGlslCodeBackend GlslBackend(Flags);
		FMetalLanguageSpec MetalLanguage;
		FMetalCodeBackend MetalBackend(Flags);

		switch (RunInfo.BackEnd)
		{
		case CCT::FRunInfo::BE_Metal:
			Language = &MetalLanguage;
			Backend = &MetalBackend;
			break;

		case CCT::FRunInfo::BE_OpenGL:
			Language = &GlslLanguage;
			Backend = &GlslBackend;
			Flags |= HLSLCC_DX11ClipSpace;
			break;

		default:
			return 1;
		}

		FString HLSLShaderSource;
		if (!FFileHelper::LoadFileToString(HLSLShaderSource, *RunInfo.InputFile))
		{
			UE_LOG(LogCrossCompilerTool, Error, TEXT("Couldn't load Input file!"));
			return 1;
		}

		FString ShaderSource(TEXT(""));
		ANSICHAR* ErrorLog = 0;

		int32 Result = HlslCrossCompile(
			RunInfo.InputFile,
			HLSLShaderSource,
			RunInfo.Entry,
			RunInfo.Frequency,
			Backend,
			Language,
			Flags,
			RunInfo.Target,
			ShaderSource,
			&ErrorLog
			);

		if (ErrorLog)
		{
			FString OutError(ANSI_TO_TCHAR(ErrorLog));
			UE_LOG(LogCrossCompilerTool, Warning, TEXT("%s"), *OutError);
		}

		if (ShaderSource.Len() > 0)
		{
			UE_LOG(LogCrossCompilerTool, Display, TEXT("%s"), *ShaderSource);
			if (RunInfo.OutputFile.Len() > 0)
			{
				FFileHelper::SaveStringToFile(ShaderSource, *RunInfo.OutputFile);
			}
		}

		free(ErrorLog);

		return Result;
	}
}

INT32_MAIN_INT32_ARGC_TCHAR_ARGV()
{
	GEngineLoop.PreInit(ArgC, ArgV, TEXT("-NOPACKAGECACHE -Multiprocess"));

	TArray<FString> Tokens, Switches;
	FCommandLine::Parse(FCommandLine::Get(), Tokens, Switches);

	if (Tokens.Num() < 1)
	{
		UE_LOG(LogCrossCompilerTool, Error, TEXT("Missing input file!"));
		CCT::PrintUsage();
		return 1;
	}

	if (Tokens.Num() > 1)
	{
		UE_LOG(LogCrossCompilerTool, Warning,TEXT("Ignoring extra command line arguments!"));
	}

	CCT::FRunInfo RunInfo;
	if (!RunInfo.Setup(Tokens[0], Switches))
	{
		return 1;
	}

	return CCT::Run(RunInfo);
}
