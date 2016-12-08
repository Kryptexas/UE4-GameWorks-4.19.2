// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace SCT
{
	class FRunInfo
	{
	public:
		FString InputFile1;
		FString InputFile2;
		FString OutputFile;
		int32 GameVersion;

		FRunInfo();
		bool Setup(const TArray<FString>& InOptions, const TArray<FString>& InSwitches);
	};

	void PrintUsage();
}

DECLARE_LOG_CATEGORY_EXTERN(LogShaderCacheTool, Log, All);
