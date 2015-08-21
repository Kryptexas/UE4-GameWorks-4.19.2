// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnitTestFrameworkEditorPCH.h"
#include "UnitTestFrameworkEditorModule.h"
#include "Engine/World.h"
#include "UTFUnitTestInterface.h"
#include "UTFEditorAutomationTests.h"

/*******************************************************************************
 * 
 ******************************************************************************/

namespace UnitTestFrameworkEditorModuleImpl
{
	/**  */
	static void AddUnitTestingRegistryTags(const UWorld* World, TArray<UObject::FAssetRegistryTag>& TagsOut);
}

//------------------------------------------------------------------------------
static void UnitTestFrameworkEditorModuleImpl::AddUnitTestingRegistryTags(const UWorld* World, TArray<UObject::FAssetRegistryTag>& TagsOut)
{
	if (ULevel* Level = World->PersistentLevel)
	{
		for (AActor* Actor : Level->Actors)
		{
			if (Actor == nullptr)
			{
				continue;
			}

			UClass* ActorClass = Actor->GetClass();
			if (ActorClass->ImplementsInterface(UUTFUnitTestInterface::StaticClass()))
			{
				TagsOut.Add(UObject::FAssetRegistryTag(FUTFEditorAutomationTests::UnitTestAssetTag, TEXT("TRUE"), UObject::FAssetRegistryTag::TT_Hidden));
				break;
			}
		}
	}
}

/*******************************************************************************
 * FUnitTestFrameworkEditorModule
 ******************************************************************************/

/** 
 * 
 */
class FUnitTestFrameworkEditorModule : public IUnitTestFrameworkEditorModuleInterface
{
public:
	virtual void StartupModule() override
	{
		GetWorldAssetTagsHandle = FWorldDelegates::GetAssetTags.AddStatic(UnitTestFrameworkEditorModuleImpl::AddUnitTestingRegistryTags);
	}

	virtual void ShutdownModule() override
	{
		FWorldDelegates::GetAssetTags.Remove(GetWorldAssetTagsHandle);
		GetWorldAssetTagsHandle.Reset();
	}

private: 
	FDelegateHandle GetWorldAssetTagsHandle;
};
IMPLEMENT_MODULE(FUnitTestFrameworkEditorModule, UnitTestFrameworkEditor);
