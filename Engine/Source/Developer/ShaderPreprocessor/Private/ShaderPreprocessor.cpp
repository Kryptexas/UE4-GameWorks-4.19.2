// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShaderPreprocessor.h"
#include "mcpp.h"
#include "ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, ShaderPreprocessor);

/**
 * Append defines to an MCPP command line.
 * @param OutOptions - Upon return contains MCPP command line parameters as a string appended to the current string.
 * @param Definitions - Definitions to add.
 */
static void AddMcppDefines(FString& OutOptions, const TMap<FString,FString>& Definitions)
{
	for (TMap<FString,FString>::TConstIterator It(Definitions); It; ++It)
	{
		OutOptions += FString::Printf(TEXT(" -D%s=%s"), *(It.Key()), *(It.Value()));
	}
}

/**
 * Filter MCPP errors.
 * @param ErrorMsg - The error message.
 * @returns true if the message is valid and has not been filtered out.
 */
static bool FilterMcppError(const FString& ErrorMsg)
{
	const TCHAR* SubstringsToFilter[] =
	{
		TEXT("with no newline, supplemented newline"),
		TEXT("Converted [CR+LF] to [LF]")
	};
	const int32 FilteredSubstringCount = ARRAY_COUNT(SubstringsToFilter);

	for (int32 SubstringIndex = 0; SubstringIndex < FilteredSubstringCount; ++SubstringIndex)
	{
		if (ErrorMsg.Contains(SubstringsToFilter[SubstringIndex]))
		{
			return false;
		}
	}
	return true;
}

/**
 * Parses MCPP error output.
 * @param ShaderOutput - Shader output to which to add errors.
 * @param McppErrors - MCPP error output.
 */
static bool ParseMcppErrors(FShaderCompilerOutput& ShaderOutput, const FString& McppErrors)
{
	bool bSuccess = true;
	if (McppErrors.Len() > 0)
	{
		TArray<FString> Lines;
		McppErrors.ParseIntoArray(&Lines, TEXT("\n"), true);
		for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
		{
			const FString& Line = Lines[LineIndex];
			int32 SepIndex1 = Line.Find(TEXT(":"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 2);
			int32 SepIndex2 = Line.Find(TEXT(":"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SepIndex1 + 1);
			if (SepIndex1 != INDEX_NONE && SepIndex2 != INDEX_NONE)
			{
				FString Filename = Line.Left(SepIndex1);
				FString LineNumStr = Line.Mid(SepIndex1 + 1, SepIndex2 - SepIndex1 - 1);
				FString Message = Line.Mid(SepIndex2 + 1, Line.Len() - SepIndex2 - 1);
				if (Filename.Len() && LineNumStr.Len() && LineNumStr.IsNumeric() && Message.Len())
				{
					while (++LineIndex < Lines.Num() && Lines[LineIndex].Len() && Lines[LineIndex].StartsWith(TEXT(" "),ESearchCase::CaseSensitive))
					{
						Message += FString(TEXT("\n")) + Lines[LineIndex];
					}
					--LineIndex;
					Message = Message.Trim().TrimTrailing();

					// Ignore the warning about files that don't end with a newline.
					if (FilterMcppError(Message))
					{
						FShaderCompilerError* CompilerError = new(ShaderOutput.Errors) FShaderCompilerError;
						CompilerError->ErrorFile = GetRelativeShaderFilename(Filename);
						CompilerError->ErrorLineString = LineNumStr;
						CompilerError->StrippedErrorMessage = Message;
						bSuccess = false;
					}
				}

			}
		}
	}
	return bSuccess;
}

/**
 * Helper class used to load shader source files for MCPP.
 */
class FMcppFileLoader
{
public:
	/** Initialization constructor. */
	explicit FMcppFileLoader(const FShaderCompilerInput& InShaderInput)
		: ShaderInput(InShaderInput)
	{
		FString ShaderDir = FPlatformProcess::ShaderDir();

		InputShaderFile = ShaderDir / FPaths::GetCleanFilename(ShaderInput.SourceFilename);
		if (FPaths::GetExtension(InputShaderFile) != TEXT("usf"))
		{
			InputShaderFile += TEXT(".usf");
		}

		FString InputShaderSource;
		if (LoadShaderSourceFile(*ShaderInput.SourceFilename,InputShaderSource))
		{
			InputShaderSource = FString::Printf(TEXT("%s\n#line 1\n%s"), *ShaderInput.SourceFilePrefix, *InputShaderSource);
			CachedFileContents.Add(GetRelativeShaderFilename(InputShaderFile),StringToArray<ANSICHAR>(*InputShaderSource, InputShaderSource.Len()));
		}
	}

	/** Returns the input shader filename to pass to MCPP. */
	const FString& GetInputShaderFilename() const
	{
		return InputShaderFile;
	}

	/** Retrieves the MCPP file loader interface. */
	file_loader GetMcppInterface()
	{
		file_loader Loader;
		Loader.get_file_contents = GetFileContents;
		Loader.user_data = (void*)this;
		return Loader;
	}

private:
	/** Holder for shader contents (string + size). */
	typedef TArray<ANSICHAR> FShaderContents;

	/** MCPP callback for retrieving file contents. */
	static int GetFileContents(void* InUserData, const ANSICHAR* InFilename, const ANSICHAR** OutContents, size_t* OutContentSize)
	{
		FMcppFileLoader* This = (FMcppFileLoader*)InUserData;
		FString Filename = GetRelativeShaderFilename(ANSI_TO_TCHAR(InFilename));

		FShaderContents* CachedContents = This->CachedFileContents.Find(Filename);
		if (!CachedContents)
		{
			FString FileContents;
			if (This->ShaderInput.Environment.IncludeFileNameToContentsMap.Contains(Filename))
			{
				FileContents = This->ShaderInput.Environment.IncludeFileNameToContentsMap.FindRef(Filename);
			}
			else
			{
				LoadShaderSourceFile(*Filename,FileContents);
			}

			if (FileContents.Len() > 0)
			{
				CachedContents = &This->CachedFileContents.Add(Filename,StringToArray<ANSICHAR>(*FileContents, FileContents.Len()));
			}
		}

		if (OutContents)
		{
			*OutContents = CachedContents ? CachedContents->GetTypedData() : NULL;
		}
		if (OutContentSize)
		{
			*OutContentSize = CachedContents ? CachedContents->Num() : 0;
		}

		return !!CachedContents;
	}

	/** Shader input data. */
	const FShaderCompilerInput& ShaderInput;
	/** File contents are cached as needed. */
	TMap<FString,FShaderContents> CachedFileContents;
	/** The input shader filename. */
	FString InputShaderFile;
};

/**
 * Preprocess a shader.
 * @param OutPreprocessedShader - Upon return contains the preprocessed source code.
 * @param ShaderOutput - ShaderOutput to which errors can be added.
 * @param ShaderInput - The shader compiler input.
 * @param AdditionalDefines - Additional defines with which to preprocess the shader.
 * @returns true if the shader is preprocessed without error.
 */
bool PreprocessShader(
	FString& OutPreprocessedShader,
	FShaderCompilerOutput& ShaderOutput,
	const FShaderCompilerInput& ShaderInput,
	const FShaderCompilerDefinitions& AdditionalDefines
	)
{
	FString McppOptions;
	FString McppOutput, McppErrors;
	ANSICHAR* McppOutAnsi = NULL;
	ANSICHAR* McppErrAnsi = NULL;
	bool bSuccess = false;

	// MCPP is not threadsafe.
	static FCriticalSection McppCriticalSection;
	FScopeLock McppLock(&McppCriticalSection);

	FMcppFileLoader FileLoader(ShaderInput);

	AddMcppDefines(McppOptions, ShaderInput.Environment.GetDefinitions());
	AddMcppDefines(McppOptions, AdditionalDefines.GetDefinitionMap());

	int32 Result = mcpp_run(
		TCHAR_TO_ANSI(*McppOptions),
		TCHAR_TO_ANSI(*FileLoader.GetInputShaderFilename()),
		&McppOutAnsi,
		&McppErrAnsi,
		FileLoader.GetMcppInterface()
		);

	McppOutput = McppOutAnsi;
	McppErrors = McppErrAnsi;

	if (ParseMcppErrors(ShaderOutput, McppErrors))
	{
		// exchange strings
		FMemory::Memswap( &OutPreprocessedShader, &McppOutput, sizeof(FString) );
		bSuccess = true;
	}

	return bSuccess;
}
