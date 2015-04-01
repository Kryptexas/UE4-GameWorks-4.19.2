// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinkerManager.h: Unreal object linker manager
=============================================================================*/

#pragma once

#include "Linker.h"

class FLinkerManager : private FSelfRegisteringExec
{
public:

	static FLinkerManager& Get();

	FORCEINLINE TSet<FLinkerLoad*>& GetLoaders()
	{
		return ObjectLoaders;
	}

	FORCEINLINE TSet<FLinkerLoad*>& GetLoadersWithNewImports()
	{
		return LoadersWithNewImports;
	}

#if UE_BUILD_DEBUG
	FORCEINLINE TArray<FLinkerLoad*>& GetLiveLinkers()
	{
		return LiveLinkers;
	}
#endif

	// FSelfRegisteringExec interface
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

private:

	/** Map of packages to their open linkers **/
	TSet<FLinkerLoad*> ObjectLoaders;
	/** List of loaders that have new imports **/
	TSet<FLinkerLoad*> LoadersWithNewImports;
#if UE_BUILD_DEBUG
	/** List of all the existing linker loaders **/
	TArray<FLinkerLoad*> LiveLinkers;
#endif
};
