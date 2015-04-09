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

	FORCEINLINE void GetLoaders(TSet<FLinkerLoad*>& OutLoaders)
	{
		FScopeLock ObjectLoadersLock(&ObjectLoadersCritical);
		OutLoaders = ObjectLoaders;
	}

	FORCEINLINE void GetLoadersAndEmpty(TSet<FLinkerLoad*>& OutLoaders)
	{
		FScopeLock ObjectLoadersLock(&ObjectLoadersCritical);
		OutLoaders = ObjectLoaders;
		OutLoaders.Empty();
	}

	FORCEINLINE void AddLoader(FLinkerLoad* LinkerLoad)
	{
		FScopeLock ObjectLoadersLock(&ObjectLoadersCritical);
		ObjectLoaders.Add(LinkerLoad);
	}

	FORCEINLINE void RemoveLoader(FLinkerLoad* LinkerLoad)
	{
		FScopeLock ObjectLoadersLock(&ObjectLoadersCritical);
		ObjectLoaders.Remove(LinkerLoad);
	}

	FORCEINLINE void EmptyLoaders(FLinkerLoad* LinkerLoad)
	{
		FScopeLock ObjectLoadersLock(&ObjectLoadersCritical);
		ObjectLoaders.Empty();
	}

	FORCEINLINE void GetLoadersWithNewImports(TSet<FLinkerLoad*>& OutLoaders)
	{
		FScopeLock ObjectLoadersLock(&LoadersWithNewImportsCritical);
		OutLoaders = LoadersWithNewImports;
	}

	FORCEINLINE void GetLoadersWithNewImportsAndEmpty(TSet<FLinkerLoad*>& OutLoaders)
	{
		FScopeLock ObjectLoadersLock(&LoadersWithNewImportsCritical);
		OutLoaders = LoadersWithNewImports;
		OutLoaders.Empty();
	}

	FORCEINLINE void AddLoaderWithNewImports(FLinkerLoad* LinkerLoad)
	{
		FScopeLock ObjectLoadersLock(&LoadersWithNewImportsCritical);
		LoadersWithNewImports.Add(LinkerLoad);
	}

	FORCEINLINE void RemoveLoaderWithNewImports(FLinkerLoad* LinkerLoad)
	{
		FScopeLock ObjectLoadersLock(&LoadersWithNewImportsCritical);
		LoadersWithNewImports.Remove(LinkerLoad);
	}

	FORCEINLINE void EmptyLoadersWithNewImports(FLinkerLoad* LinkerLoad)
	{
		FScopeLock ObjectLoadersLock(&LoadersWithNewImportsCritical);
		LoadersWithNewImports.Empty();
	}

#if UE_BUILD_DEBUG
	FORCEINLINE TArray<FLinkerLoad*>& GetLiveLinkers()
	{
		return LiveLinkers;
	}
#endif

	// FSelfRegisteringExec interface
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	/** Empty the loaders */
	void ResetLoaders(UObject* InPkg);

	/**
	* Dissociates all linker import and forced export object references. This currently needs to
	* happen as the referred objects might be destroyed at any time.
	*/
	void DissociateImportsAndForcedExports();

private:

	/** Map of packages to their open linkers **/
	TSet<FLinkerLoad*> ObjectLoaders;
	FCriticalSection ObjectLoadersCritical;

	/** List of loaders that have new imports **/
	TSet<FLinkerLoad*> LoadersWithNewImports;
	FCriticalSection LoadersWithNewImportsCritical;
#if UE_BUILD_DEBUG
	/** List of all the existing linker loaders **/
	TArray<FLinkerLoad*> LiveLinkers;
#endif
	
};
