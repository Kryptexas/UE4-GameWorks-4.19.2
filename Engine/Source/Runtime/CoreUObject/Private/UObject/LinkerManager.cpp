// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinkerManager.h: Unreal object linker manager
=============================================================================*/
#include "CoreUObjectPrivate.h"
#include "LinkerManager.h"

FLinkerManager& FLinkerManager::Get()
{
	static TAutoPtr<FLinkerManager> Singleton(new FLinkerManager());
	return *Singleton.GetOwnedPointer();
}

bool FLinkerManager::Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FParse::Command(&Cmd, TEXT("LinkerLoadList")))
	{
		UE_LOG(LogLinker, Display, TEXT("ObjectLoaders: %d"), ObjectLoaders.Num());
		UE_LOG(LogLinker, Display, TEXT("LoadersWithNewImports: %d"), LoadersWithNewImports.Num());

#if UE_BUILD_DEBUG
		UE_LOG(LogLinker, Display, TEXT("LiveLinkers: %d"), LiveLinkers.Num());
		for (auto Linker : LiveLinkers)
		{
			UE_LOG(LogLinker, Display, TEXT("%s"), *Linker->Filename);
		}
#endif
		return true;
	}
	else if (FParse::Command(&Cmd, TEXT("LINKERS")))
	{
		Ar.Logf(TEXT("Linkers:"));
		for (auto Linker : ObjectLoaders)
		{
			int32 NameSize = 0;
			for (int32 j = 0; j < Linker->NameMap.Num(); j++)
			{
				if (Linker->NameMap[j] != NAME_None)
				{
					NameSize += FNameEntry::GetSize(*Linker->NameMap[j].ToString());
				}
			}
			Ar.Logf
				(
				TEXT("%s (%s): Names=%i (%iK/%iK) Imports=%i (%iK) Exports=%i (%iK) Gen=%i Bulk=%i"),
				*Linker->Filename,
				*Linker->LinkerRoot->GetFullName(),
				Linker->NameMap.Num(),
				Linker->NameMap.Num() * sizeof(FName) / 1024,
				NameSize / 1024,
				Linker->ImportMap.Num(),
				Linker->ImportMap.Num() * sizeof(FObjectImport) / 1024,
				Linker->ExportMap.Num(),
				Linker->ExportMap.Num() * sizeof(FObjectExport) / 1024,
				Linker->Summary.Generations.Num(),
#if WITH_EDITOR
				Linker->BulkDataLoaders.Num()
#else
				0
#endif // WITH_EDITOR
				);
		}

		return true;
	}
	return false;
}