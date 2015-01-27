// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace CommandletHelpers
{
	UNREALED_API FProcHandle CreateCommandletProcess(const TCHAR* CommandletName, const TCHAR* ProjectPath, const TCHAR* Parms, bool bLaunchDetached, bool bLaunchHidden, bool bLaunchReallyHidden, uint32* OutProcessID, int32 PriorityModifier, const TCHAR* OptionalWorkingDirectory, void* PipeWrite);
}