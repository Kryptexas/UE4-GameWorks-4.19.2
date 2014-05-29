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
			if (bHasBlueprintClasses)
			{			
				if(Objects[i] != NULL)
				{
					UClass* BlueprintClass = Cast<UClass>(Objects[i]);

					if (!BlueprintClass)
					{
						// Only blueprints
						Objects[i] = NULL;
					} 
					else if (!BlueprintClass->IsChildOf(ObjectBaseClass))
					{
						// Wrong base class
						Objects[i] = NULL;
					}
				}
			}
			else
			{
				if(Objects[i] != NULL && !Objects[i]->IsA(ObjectBaseClass))
				{
					Objects[i] = NULL;
				}
			}
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

class UObjectLibrary* UObjectLibrary::CreateLibrary(UClass* InBaseClass, bool bInHasBlueprintClasses, bool bInUseWeak)
{
	UObjectLibrary *NewLibrary = ConstructObject<UObjectLibrary>(UObjectLibrary::StaticClass());

	NewLibrary->ObjectBaseClass = InBaseClass;
	NewLibrary->bHasBlueprintClasses = bInHasBlueprintClasses;
	NewLibrary->UseWeakReferences(bInUseWeak);

	return NewLibrary;
}

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
		// Convert existing weak references
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
		if (bHasBlueprintClasses)
		{
			UClass* BlueprintClass = Cast<UClass>(NewObject);

			if (!BlueprintClass)
			{
				// Only blueprints
				return false;
			} 
			else if (!BlueprintClass->IsChildOf(ObjectBaseClass))
			{
				// Wrong base class
				return false;
			}
		}
		else
		{
			if (!NewObject->IsA(ObjectBaseClass))
			{
				return false;
			}
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

int32 UObjectLibrary::LoadAssetsFromPaths(TArray<FString> Paths)
{
	int32 Count = 0;

	if (bIsFullyLoaded)
	{
		// We already ran this
		return 0; 
	}

	bIsFullyLoaded = true;
	
	for (int PathIndex = 0; PathIndex < Paths.Num(); PathIndex++)
	{
		TArray<UObject*> LoadedObjects;
		FString Path = Paths[PathIndex];
		if (EngineUtils::FindOrLoadAssetsByPath(Path, LoadedObjects, bHasBlueprintClasses ? EngineUtils::ATL_Class : EngineUtils::ATL_Regular))
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
	}
	return Count;
}

int32 UObjectLibrary::LoadBlueprintsFromPaths(TArray<FString> Paths)
{
	int32 Count = 0;

	if (!bHasBlueprintClasses)
	{
		return 0;
	}

	if (bIsFullyLoaded)
	{
		// We already ran this
		return 0; 
	}

	bIsFullyLoaded = true;

	for (int PathIndex = 0; PathIndex < Paths.Num(); PathIndex++)
	{
		TArray<UObject*> LoadedObjects;
		FString Path = Paths[PathIndex];

		if (EngineUtils::FindOrLoadAssetsByPath(Path, LoadedObjects, EngineUtils::ATL_Class))
		{
			for (int32 i = 0; i < LoadedObjects.Num(); ++i)
			{
				auto Class = Cast<UBlueprintGeneratedClass>(LoadedObjects[i]);

				if (Class == NULL || (ObjectBaseClass && !Class->IsChildOf(ObjectBaseClass)))
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

int32 UObjectLibrary::LoadAssetDataFromPaths(TArray<FString> Paths)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

#if WITH_EDITOR
	// Cooked data has the asset data already set up
	TArray<FString> ScanPaths;
	ScanPaths.Append(Paths);
	AssetRegistry.ScanPathsSynchronous(ScanPaths);
#endif

	FARFilter ARFilter;
	if ( ObjectBaseClass )
	{
		ARFilter.ClassNames.Add(ObjectBaseClass->GetFName());

		// Add any old names to the list in case things haven't been resaved
		TArray<FName> OldNames = ULinkerLoad::FindPreviousNamesForClass(ObjectBaseClass->GetPathName(), false);

		ARFilter.ClassNames.Append(OldNames);

		ARFilter.bRecursiveClasses = true;
	}

	for (int PathIndex = 0; PathIndex < Paths.Num(); PathIndex++)
	{
		ARFilter.PackagePaths.Add(FName(*Paths[PathIndex]));
	}

	ARFilter.bRecursivePaths = true;
	ARFilter.bIncludeOnlyOnDiskAssets = true;

	AssetRegistry.GetAssets(ARFilter, AssetDataList);

	return AssetDataList.Num();
}

int32 UObjectLibrary::LoadBlueprintAssetDataFromPaths(TArray<FString> Paths)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	if (!bHasBlueprintClasses)
	{
		return 0;
	}

#if WITH_EDITOR
	// Cooked data has the asset data already set up
	TArray<FString> ScanPaths;
	ScanPaths.Append(Paths);
	AssetRegistry.ScanPathsSynchronous(ScanPaths);
#endif

	FARFilter ARFilter;
	ARFilter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	
	for (int PathIndex = 0; PathIndex < Paths.Num(); PathIndex++)
	{
		ARFilter.PackagePaths.Add(FName(*Paths[PathIndex]));
	}
	
	ARFilter.bRecursivePaths = true;
	ARFilter.bIncludeOnlyOnDiskAssets = true;

	/* GetDerivedClassNames doesn't work yet
	if ( ObjectBaseClass )
	{
		TArray<FName> SearchClassNames;
		TSet<FName> ExcludedClassNames;
		TSet<FName> DerivedClassNames;
		SearchClassNames.Add(ObjectBaseClass->GetFName());
		AssetRegistry.GetDerivedClassNames(SearchClassNames, ExcludedClassNames, DerivedClassNames);

		const FName GeneratedClassName = FName(TEXT("GeneratedClass"));
		for ( auto DerivedClassIt = DerivedClassNames.CreateConstIterator(); DerivedClassIt; ++DerivedClassIt )
		{
			ARFilter.TagsAndValues.Add(GeneratedClassName, (*DerivedClassIt).ToString());
		}
	}*/

	AssetRegistry.GetAssets(ARFilter, AssetDataList);

	for(int32 AssetIdx=0; AssetIdx<AssetDataList.Num(); AssetIdx++)
	{
		FAssetData& Data = AssetDataList[AssetIdx];
		
		const FString* LoadedParentClass = Data.TagsAndValues.Find("ParentClass");
		if (LoadedParentClass && !LoadedParentClass->IsEmpty())
		{
			UClass* Class = FindObject<UClass>(ANY_PACKAGE, **LoadedParentClass);
			if ( Class == NULL )
			{
				Class = LoadObject<UClass>(NULL, **LoadedParentClass);
			}
			if (!Class || (ObjectBaseClass && !Class->IsChildOf(ObjectBaseClass)))
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

		UObject *LoadedObject = NULL;
			
		if (!bHasBlueprintClasses)
		{
			LoadedObject = Data.GetAsset();

			checkSlow(!LoadedObject || !ObjectBaseClass || LoadedObject->IsA(ObjectBaseClass));
		}
		else
		{
			UPackage* Package = Data.GetPackage();
			if (Package)
			{
				TArray<UObject*> ObjectsInPackage;
				GetObjectsWithOuter(Package, ObjectsInPackage);
				for (UObject* PotentialBPGC : ObjectsInPackage)
				{
					if (auto LoadedBPGC = Cast<UBlueprintGeneratedClass>(PotentialBPGC))
					{
						checkSlow(!ObjectBaseClass || LoadedBPGC->IsChildOf(ObjectBaseClass));
						LoadedObject = LoadedBPGC;
						break; //there is usually only one BGPC in a package
					}
				}
			}
		}

		if (LoadedObject)
		{
			AddObject(LoadedObject);
			Count++;
		}
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
