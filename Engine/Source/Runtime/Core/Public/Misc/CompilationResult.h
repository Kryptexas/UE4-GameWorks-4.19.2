// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/*=============================================================================
	CompilationResult.h:  Defines an enum for return codes between build tools.
=============================================================================*/

/**
 * Enumerates possible results of a compilation operation.
 *
 * This enum has to be compatible with the one defined in the
 * UE4\Engine\Source\Programs\UnrealBuildTool\System\ExternalExecution.cs file
 * to keep communication between UHT, UBT and Editor compiling processes valid.
 */
namespace ECompilationResult
{
	enum Type
	{
		/** Compilation succeeded */
		Succeeded = 0,
		/** Compilation failed because generated code changed which was not supported */
		FailedDueToHeaderChange = 1,
		/** Compilation failed due to compilation errors */
		OtherCompilationError,
		/** Compilation is not supported in the current build */
		Unsupported,
		/** Unknown error */
		Unknown
	};

	/**
	* Converts ECompilationResult enum to string.
	*/
	static FORCEINLINE const TCHAR* ToString(ECompilationResult::Type Result)
	{
		switch (Result)
		{
		case ECompilationResult::Succeeded:
			return TEXT("Succeeded");
		case ECompilationResult::FailedDueToHeaderChange:
			return TEXT("FailedDueToHeaderChange");
		case ECompilationResult::OtherCompilationError:
			return TEXT("OtherCompilationError");
		case ECompilationResult::Unsupported:
			return TEXT("Unsupported");
		};
		return TEXT("Unknown");
	}
}
