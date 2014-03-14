// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AssetRegistryModule.h"

UObjectLibrary::UObjectLibrary(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsFullyLoaded = false;
	bUseWeakReferences = false;
}

#if WITH_EDITOR
void UObjectLibrary::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// If a base class is set, null out any references to Objects not of that class.
	if(ObjectBaseClass != NULL)
	{
		for(int32 i=0; i<Objects.Num(); i++)
		{
			if(Objects[i] != NULL && !Objects[i]->IsA(ObjectBaseClass))
			{
				Objects[i] = NULL;
			}
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

void UObjectLibrary::UseWeakReferences(bool bSetUseWeak)
{
	if (bSetUseWeak == bUseWeakReferences)
	{
		return;
	}

	bUseWeakReferences = bSetUseWeak;

	if (bUseWeakReferences)
	{
		// Convert existing strong references
		for(int32 i=0; i<Objects.Num(); i++)
		{
			if(Objects[i] != NULL)
			{
				WeakObjects.AddUnique(Objects[i]);
			}
		}
		Objects.Empty();
	}
	else
	{
		// Convert existing wewak references
		for(int32 i=0; i<WeakObjects.Num(); i++)
		{
			if(WeakObjects[i].Get() != NULL)
			{
				Objects.AddUnique(WeakObjects[i].Get());
			}
		}
		WeakObjects.Empty();
	}
}

bool UObjectLibrary::AddObject(UObject *NewObject)
{
	if (NewObject == NULL)
	{
		return false;
	}

	if (ObjectBaseClass != NULL)
	{
		if (!NewObject->IsA(ObjectBaseClass))
		{
			return false;
		}
	}

	if (bUseWeakReferences)
	{
		if (WeakObjects.Contains(NewObject))
		{
			return false;
		}

		if (WeakObjects.Add(NewObject) != INDEX_NONE)
		{
			Modify();
			return true;
		}
	}
	else
	{
		if (Objects.Contains(NewObject))
		{
			return false;
		}

		if (Objects.Add(NewObject) != INDEX_NONE)
		{
			Modify();
			return true;
		}
	}
	
	return false;
}

bool UObjectLibrary::RemoveObject(UObject *ObjectToRemove)
{
	if (bUseWeakReferences)
	{
		if (Objects.Remove(ObjectToRemove) != 0)
		{
			Modify();
			return true;
		}
	}
	else
	{
		if (WeakObjects.Remove(ObjectToRemove) != 0)
		{
			Modify();
			return true;
		}
	}

	return false;
}

int32 UObjectLibrary::LoadAssetsFromPath(const FString& Path)
{
	int32 Count = 0;
	TArray<UObject*> LoadedObjects;
	if (EngineUtils::FindOrLoadAssetsByPath(Path, LoadedObjects))
	{
		for (int32 i = 0; i < LoadedObjects.Num(); ++i)
		{
			UObject* Object = LoadedObjects[i];

			if (Object == NULL || (ObjectBaseClass && !Object->IsA(ObjectBaseClass)))
			{
				// Incorrect type, skip
				continue;
			}
		
			AddObject(Object);
			Count++;
		}
	}
	return Count;
}

int32 UObjectLibrary::LoadBlueprintsFromPath(const FString& Path, UClass *ParentClass)
{
	int32 Count = 0;
	TArray<UObject*> LoadedObjects;
	if (EngineUtils::FindOrLoadAssetsByPath(Path, LoadedObjects))
	{
		for (int32 i = 0; i < LoadedObjects.Num(); ++i)
		{
			UBlueprintCore* Blueprint = Cast<UBlueprintCore>(LoadedObjects[i]);

			if (Blueprint && Blueprint->GeneratedClass)
			{
				UClass* Class = Blueprint->GeneratedClass;

				if (Class == NULL || (ParentClass && !Class->IsChildOf(ParentClass)))
				{
					continue;
				}
		
				AddObject(Class);
				Count++;
			}
		}
	}
	return Count;
}

int32 UObjectLibrary::LoadAssetDataFromPath(const FString& Path)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

#if WITH_EDITOR
	// Cooked data has the asset data already set up
	TArray<FString> ScanPaths;
	ScanPaths.Add(Path);
	AssetRegistry.ScanPathsSynchronous(ScanPaths);
#endif

	AssetRegistry.GetAssetsByPath(FName(*Path), AssetDataList, true);
	for(int32 AssetIdx=0; AssetIdx<AssetDataList.Num(); AssetIdx++)
	{
		FAssetData& Data = AssetDataList[AssetIdx];
		if (ObjectBaseClass && !Data.GetClass()->IsChildOf(ObjectBaseClass))
		{
			// Remove if class is wrong
			AssetDataList.RemoveAt(AssetIdx);
			AssetIdx--;
			continue;
		}
	}

	return AssetDataList.Num();
}

int32 UObjectLibrary::LoadBlueprintAssetDataFromPath(const FString& Path, UClass *ParentClass)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

#if WITH_EDITOR
	// Cooked data has the asset data already set up
	TArray<FString> ScanPaths;
	ScanPaths.Add(Path);
	AssetRegistry.ScanPathsSynchronous(ScanPaths);
#endif

	AssetRegistry.GetAssetsByPath(FName(*Path), AssetDataList, true);
	for(int32 AssetIdx=0; AssetIdx<AssetDataList.Num(); AssetIdx++)
	{
		FAssetData& Data = AssetDataList[AssetIdx];
		if (!Data.GetClass()->IsChildOf(UBlueprintCore::StaticClass()))
		{
			// Remove if class is wrong
			AssetDataList.RemoveAt(AssetIdx);
			AssetIdx--;
			continue;
		}

		const FString* LoadedParentClass = Data.TagsAndValues.Find("ParentClass");
		if (LoadedParentClass && !LoadedParentClass->IsEmpty())
		{
			UClass* Class = FindObject<UClass>(ANY_PACKAGE, **LoadedParentClass);
			if ( Class == NULL )
			{
				Class = LoadObject<UClass>(ANY_PACKAGE, **LoadedParentClass);
			}
			if (!Class || (ParentClass && !Class->IsChildOf(ParentClass)))
			{
				// Remove if class is wrong
				AssetDataList.RemoveAt(AssetIdx);
				AssetIdx--;
				continue;
			}
		}
		else
		{
				// Remove if class is wrong
				AssetDataList.RemoveAt(AssetIdx);
				AssetIdx--;
				continue;
		}
	}

	return AssetDataList.Num();
}

int32 UObjectLibrary::LoadAssetsFromAssetData()
{
	int32 Count = 0;

	if (bIsFullyLoaded)
	{
		// We already ran this
		return 0; 
	}

	bIsFullyLoaded = true;

	for(int32 AssetIdx=0; AssetIdx<AssetDataList.Num(); AssetIdx++)
	{
		FAssetData& Data = AssetDataList[AssetIdx];

		UObject *LoadedObject = Data.GetAsset();

		if (!LoadedObject)
		{
			continue;
		}

		if (LoadedObject->IsA(UBlueprintCore::StaticClass()) && (!ObjectBaseClass || !ObjectBaseClass->IsChildOf(UBlueprintCore::StaticClass())))
		{
			// If we're looking for non-blueprints and are a blueprint, look for generated class
			UBlueprintCore* LoadedBlueprint = Cast<UBlueprintCore>(LoadedObject);
			LoadedObject = LoadedBlueprint->GeneratedClass;
		}

		checkSlow(!ObjectBaseClass || LoadedObject->IsA(ObjectBaseClass));

		AddObject(LoadedObject);
		Count++;
	}

	return Count;
}

void UObjectLibrary::ClearLoaded()
{
	bIsFullyLoaded = false;
	AssetDataList.Empty();
	Objects.Empty();
	WeakObjects.Empty();
}

void UObjectLibrary::GetAssetDataList(TArray<FAssetData>& OutAssetData)
{
	OutAssetData = AssetDataList;
}
