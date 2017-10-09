// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FlexMigrateContentCommandlet.h"
#include "Misc/Paths.h"
#include "FileHelpers.h"
#include "AssetRegistryModule.h"
#include "Engine/World.h"

#include "FlexStaticMesh.h"
#include "FlexAsset.h"

#include "Editor.h"

#include "Serialization/FindReferencersArchive.h"
#include "Serialization/ArchiveReplaceObjectRef.h"

#include "FlexParticleSpriteEmitter.h"
//#include "FlexParticleSystemComponent.h"


DEFINE_LOG_CATEGORY_STATIC(LogFlexMigrateContentCommandlet, Log, All);


/**
 * UFlexMigrateContentCommandlet
 *
 * Commandlet used to migrate content from non-plugin Flex UE4 integration
 */

UFlexMigrateContentCommandlet::UFlexMigrateContentCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

int32 UFlexMigrateContentCommandlet::Main(const FString& Params)
{
	bool bSuccess = true;

	UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("FlexMigrateContentCommandlet: ProjectFilePath = '%s'"), *FPaths::GetProjectFilePath());

#if 0
	TArray<FString> AllPackageFilenames;
	FEditorFileUtils::FindAllPackageFiles(AllPackageFilenames);
	for (int32 PackageIndex = 0; PackageIndex < AllPackageFilenames.Num(); PackageIndex++)
	{
		const FString& Filename = AllPackageFilenames[PackageIndex];

		if (FPaths::GetExtension(Filename, true) == FPackageName::GetMapPackageExtension())
		{
			UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("FlexMigrateContentCommandlet: Map = '%s'"), *Filename);
		}
	}
#endif
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// Update Registry Module
	AssetRegistryModule.Get().SearchAllAssets(true);

	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssetsByPath(FName("/Game"), AssetList, true);

	// load all assets
	for (auto AssetIt = AssetList.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		const FAssetData& AssetData = *AssetIt;
		UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("FlexMigrateContentCommandlet: load Asset - %s"), *AssetData.GetFullName());

		AssetData.GetPackage()->FullyLoad();
	}

	TMap<UObject*, UObject*> ReplacementMap;
	TArray<UPackage*> DirtiedPackages;

	// find StaticMeshes with FlexAsset property
	for (auto AssetIt = AssetList.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		const FAssetData& AssetData = *AssetIt;
		if (AssetData.AssetClass == UStaticMesh::StaticClass()->GetFName())
		{
			UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
			if (StaticMesh && StaticMesh->FlexAsset_DEPRECATED)
			{
				UObject* NewObject = MigrateStaticMesh(StaticMesh, DirtiedPackages);
				if (NewObject == nullptr)
				{
					UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("failed to migrate StaticMesh: %s"), *StaticMesh->GetFullName());
					bSuccess = false;
					break;
				}

				ReplacementMap.Add(StaticMesh, NewObject);
			}
		}
	}
#if 0
	for (TObjectIterator<UParticleSystemComponent> ComponentIt; ComponentIt; ++ComponentIt)
	{
		UParticleSystemComponent* ParticleSystemComponent = *ComponentIt;
		if (ParticleSystemComponent && ParticleSystemComponent->Template)
		{
			bool bIsFlexParticleSystem = false;
			for (auto EmitterIt = ParticleSystemComponent->Template->Emitters.CreateIterator(); EmitterIt; ++EmitterIt)
			{
				if ((*EmitterIt)->FlexContainerTemplate_DEPRECATED != nullptr)
				{
					bIsFlexParticleSystem = true;
					break;
				}
			}

			if (bIsFlexParticleSystem)
			{
				UObject* NewObject = MigrateParticleSystemComponent(ParticleSystemComponent, DirtiedPackages);
				if (NewObject == nullptr)
				{
					UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("failed to migrate ParticleSystemComponent: %s"), *ParticleSystemComponent->GetFullName());
					bSuccess = false;
					break;
				}

				ReplacementMap.Add(ParticleSystemComponent, NewObject);
			}
		}
	}
#endif
	for (TObjectIterator<UParticleSpriteEmitter> EmitterIt; EmitterIt; ++EmitterIt)
	{
		UParticleSpriteEmitter* ParticleEmitter = *EmitterIt;
		if (ParticleEmitter && ParticleEmitter->FlexContainerTemplate_DEPRECATED)
		{
			UObject* NewObject = MigrateParticleEmitter(ParticleEmitter, DirtiedPackages);
			if (NewObject == nullptr)
			{
				UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("failed to migrate ParticleEmitter: %s"), *ParticleEmitter->GetFullName());
				bSuccess = false;
				break;
			}

			ReplacementMap.Add(ParticleEmitter, NewObject);
		}
	}

	if (bSuccess && ReplacementMap.Num() > 0)
	{
		ForceReplaceReferences(ReplacementMap, DirtiedPackages);
	}

	// save all dirtied packages
	for (auto PackageIt = DirtiedPackages.CreateConstIterator(); PackageIt; ++PackageIt)
	{
		UPackage* Package = *PackageIt;
		check(Package->FileName != NAME_None);

		const FString PackageName = Package->FileName.ToString();
		const FString PackageExtension = Package->ContainsMap() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
		const FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, PackageExtension);

		UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("Saving Package '%s' to file '%s'"), *PackageName, *PackageFilename);

		//return UPackage::SavePackage(Package, nullptr, RF_Standalone, *PackageFileName, GError, nullptr, false, true, SAVE_NoError);
		GEditor->SavePackage(Package, nullptr, RF_Standalone, *PackageFilename, GError, nullptr, false, true, SAVE_NoError);
	}
	return bSuccess ? 0 : 1;
}

namespace
{
	template <class TR, class T>
	TR* PreMigrateObject(T* InObject)
	{
		UObject* InOuter = InObject->GetOuter();
		FName InObjectFName = InObject->GetFName();

		UPackage* TempPackage = GetTransientPackage();
		FString TempObjectName = MakeUniqueObjectName(TempPackage, T::StaticClass(), InObjectFName).ToString();

		if (InObject->Rename(*TempObjectName, TempPackage, REN_DontCreateRedirectors) == false)
			return nullptr;

		return Cast<TR>(StaticDuplicateObject(InObject, InOuter, InObjectFName, RF_AllFlags, TR::StaticClass()));
	}

	template <class T>
	T* PostMigrateObject(T* NewObject, TArray<UPackage*>& DirtiedPackages)
	{
		UPackage* Package = NewObject->GetOutermost();
		Package->SetDirtyFlag(true);
		DirtiedPackages.AddUnique(Package);
		return NewObject;
	}

	template <class TNO, typename TNP, class TOO, typename TOP>
	void MigratePropetry(TNO* NewObject, typename TNP* TNO::* NewProperty, TOO* OldObject, typename TOP* TOO::* OldProperty)
	{
		TOP* OldPropValue = OldObject->*OldProperty;
		TNP* NewPropValue = NewObject->*NewProperty = Cast<TNP>(OldPropValue);
		OldObject->*OldProperty = nullptr;

		UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("MigratePropetry: %s"), NewPropValue != nullptr ? *NewPropValue->GetFullName() : TEXT("null"));

		if (NewPropValue)
		{
			if (NewPropValue->IsIn(OldObject))
			{
				UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("MigratePropetry: change outer!"));
				NewPropValue->Rename(nullptr, NewObject, REN_DontCreateRedirectors);
			}
		}
	}
}

UObject* UFlexMigrateContentCommandlet::MigrateStaticMesh(UStaticMesh* StaticMesh, TArray<UPackage*>& DirtiedPackages)
{
	UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("MigrateStaticMesh: %s"), *StaticMesh->GetFullName());

	auto FSM = PreMigrateObject<UFlexStaticMesh>(StaticMesh);
	if (FSM == nullptr)
		return nullptr;

	MigratePropetry(FSM, &UFlexStaticMesh::FlexAsset, StaticMesh, &UStaticMesh::FlexAsset_DEPRECATED);

	return PostMigrateObject(FSM, DirtiedPackages);
}

class UObject* UFlexMigrateContentCommandlet::MigrateParticleEmitter(class UParticleSpriteEmitter* ParticleEmitter, TArray<UPackage*>& DirtiedPackages)
{
	UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("MigrateParticleEmitter: %s"), *ParticleEmitter->GetFullName());

	auto FPE = PreMigrateObject<UFlexParticleSpriteEmitter>(ParticleEmitter);
	if (FPE == nullptr)
		return nullptr;

	if (FPE->EmitterName != UFlexParticleSpriteEmitter::DefaultEmitterName)
	{
		FPE->EmitterName = FName(*(FString(TEXT("Flex ")) + ParticleEmitter->EmitterName.ToString()));
	}

	MigratePropetry(FPE, &UFlexParticleSpriteEmitter::FlexContainerTemplate, static_cast<UParticleEmitter*>(ParticleEmitter), &UParticleEmitter::FlexContainerTemplate_DEPRECATED);
	FPE->Phase = ParticleEmitter->Phase_DEPRECATED;
	FPE->bLocalSpace = ParticleEmitter->bLocalSpace_DEPRECATED;
	FPE->Mass = ParticleEmitter->Mass_DEPRECATED;
	FPE->InertialScale = ParticleEmitter->InertialScale_DEPRECATED;
	MigratePropetry(FPE, &UFlexParticleSpriteEmitter::FlexFluidSurfaceTemplate, static_cast<UParticleEmitter*>(ParticleEmitter), &UParticleEmitter::FlexFluidSurfaceTemplate_DEPRECATED);

	return PostMigrateObject(FPE, DirtiedPackages);
}

#if 0
class UObject* UFlexMigrateContentCommandlet::MigrateParticleSystemComponent(class UParticleSystemComponent* ParticleSystemComponent, TArray<UPackage*>& DirtiedPackages)
{
	UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("MigrateParticleSystemComponent: %s"), *ParticleSystemComponent->GetFullName());

	auto FPSC = PreMigrateObject<UFlexParticleSystemComponent>(ParticleSystemComponent);
	if (FPSC == nullptr)
		return nullptr;

	MigratePropetry(FPSC, &UFlexParticleSystemComponent::FlexFluidSurfaceOverride, ParticleSystemComponent, &UParticleSystemComponent::FlexFluidSurfaceOverride_DEPRECATED);

	return PostMigrateObject(FPSC, DirtiedPackages);
}
#endif

bool UFlexMigrateContentCommandlet::ForceReplaceReferences(const TMap<UObject*, UObject*>& ReplacementMap, TArray<UPackage*>& DirtiedPackages)
{
	TArray<UObject*> ReplaceableObjects;
	ReplacementMap.GenerateKeyArray(ReplaceableObjects);

	// Find all the properties (and their corresponding objects) that refer to any of the objects to be replaced
	TMap< UObject*, TSet<UProperty*> > ReferencingPropertiesMap;
	for (FObjectIterator ObjIter; ObjIter; ++ObjIter)
	{
		UObject* CurObject = *ObjIter;

		// Unless the "object to replace with" is null, ignore any of the objects to replace to themselves
		if (ReplacementMap.Find(CurObject) == nullptr)
		{
			// Find the referencers of the objects to be replaced
			FFindReferencersArchive FindRefsArchive(CurObject, ReplaceableObjects);

			// Inform the object referencing any of the objects to be replaced about the properties that are being forcefully
			// changed, and store both the object doing the referencing as well as the properties that were changed in a map (so that
			// we can correctly call PostEditChange later)
			TMap<UObject*, int32> CurNumReferencesMap;
			TMultiMap<UObject*, UProperty*> CurReferencingPropertiesMMap;
			if (FindRefsArchive.GetReferenceCounts(CurNumReferencesMap, CurReferencingPropertiesMMap) > 0)
			{
				auto& CurReferencedProperties = ReferencingPropertiesMap.Add(CurObject);
				for (auto MMapIter = CurReferencingPropertiesMMap.CreateConstIterator(); MMapIter; ++MMapIter)
				{
					CurReferencedProperties.Add(MMapIter->Value);
				}

				if (CurReferencedProperties.Num() > 0)
				{
					UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("Object %s has properties to replace:"), *CurObject->GetFullName());

					for (auto RefPropIter = CurReferencedProperties.CreateConstIterator(); RefPropIter; ++RefPropIter)
					{
						UProperty* Prop = *RefPropIter;

						auto LogProperty([&](UObject* ObjectToReplace)
						{
							auto FindResult = ReplacementMap.Find(ObjectToReplace);
							UObject* ReplaceObject = FindResult ? *FindResult : nullptr;

							UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("--- Prop %s = %s -> %s"), *Prop->GetName(), ObjectToReplace ? *ObjectToReplace->GetFullName() : TEXT("null"), ReplaceObject ? *ReplaceObject->GetFullName() : TEXT("null"));

						});

						if (UObjectProperty* ObjectProp = Cast<UObjectProperty>(Prop))
						{
							UProperty* OwnerProp = ObjectProp->GetOwnerProperty();
							if (OwnerProp != ObjectProp)
							{
								if (UArrayProperty* ArrayProp = Cast<UArrayProperty>(OwnerProp))
								{
									check(ArrayProp->Inner == Prop);
									FScriptArrayHelper_InContainer ArrayHelper(ArrayProp, CurObject);
									for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
									{
										UObject* Object = ObjectProp->GetObjectPropertyValue(ArrayHelper.GetRawPtr(Index));
										LogProperty(Object);
									}
								}
							}
							else
							{
								for (int32 Index = 0; Index < ObjectProp->ArrayDim; ++Index)
								{
									UObject* Object = ObjectProp->GetPropertyValue_InContainer(CurObject, Index);
									LogProperty(Object);
								}
							}
						}

						CurObject->PreEditChange(Prop);
					}
				}
				else
				{
					CurObject->PreEditChange(nullptr);
				}
			}
		}
	}

	// Iterate over the map of referencing objects/changed properties, forcefully replacing the references and
	for (auto MapIter = ReferencingPropertiesMap.CreateConstIterator(); MapIter; ++MapIter)
	{
		UObject* CurReplaceObj = MapIter.Key();

		FArchiveReplaceObjectRef<UObject> ReplaceAr(CurReplaceObj, ReplacementMap, false, true, false);
	}

	// Now alter the referencing objects the change has completed via PostEditChange, 
	// this is done in a separate loop to prevent reading of data that we want to overwrite
	for (auto MapIter = ReferencingPropertiesMap.CreateConstIterator(); MapIter; ++MapIter)
	{
		UObject* CurReplaceObj = MapIter.Key();
		const auto& RefPropSet = MapIter.Value();

		if (RefPropSet.Num() > 0)
		{
			for (auto RefPropIter = RefPropSet.CreateConstIterator(); RefPropIter; ++RefPropIter)
			{
				FPropertyChangedEvent PropertyEvent(*RefPropIter, EPropertyChangeType::Redirected);
				CurReplaceObj->PostEditChangeProperty(PropertyEvent);
			}
		}
		else
		{
			FPropertyChangedEvent PropertyEvent(nullptr, EPropertyChangeType::Redirected);
			CurReplaceObj->PostEditChangeProperty(PropertyEvent);
		}

		if (!CurReplaceObj->HasAnyFlags(RF_Transient) && CurReplaceObj->GetOutermost() != GetTransientPackage())
		{
			if (!CurReplaceObj->RootPackageHasAnyFlags(PKG_CompiledIn))
			{
				CurReplaceObj->MarkPackageDirty();
				DirtiedPackages.AddUnique(CurReplaceObj->GetOutermost());
			}
		}
	}

	return true;
}
