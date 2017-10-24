// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#if !UE_BUILD_SHIPPING

#include "CoreMinimal.h"
#include "ToolMode.h"

namespace BuildPatchTool
{
	class FAutomationToolModeFactory
	{
	public:
		static IToolModeRef Create(IBuildPatchServicesModule& BpsInterface);
	};
}

#endif // !UE_BUILD_SHIPPING
