// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintNativeCodeGenPCH.h"
#include "BlueprintNativeCodeGenCoordinator.h"
#include "NativeCodeGenCommandlineParams.h"
#include "Engine/Blueprint.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "ARFilter.h"
#include "PackageName.h"
#include "FileManager.h"
#include "AssetRegistryModule.h"

/*******************************************************************************
 * UBlueprintNativeCodeGenConfig
*******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintNativeCodeGenConfig::UBlueprintNativeCodeGenConfig(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

/*******************************************************************************
 * BlueprintNativeCodeGenCoordinatorImpl declaration
*******************************************************************************/

namespace BlueprintNativeCodeGenCoordinatorImpl
{
	static const UClass* TargetAssetTypes[] = {
		UBlueprint::StaticClass(),
		UUserDefinedStruct::StaticClass(),
		UUserDefinedEnum::StaticClass(),
	};

	static const int32 IncompatibleBlueprintTypeMask = (1 << BPTYPE_LevelScript) |
		(1 << BPTYPE_MacroLibrary) | (1 << BPTYPE_Const);

	static const FName BlueprintTypePropertyName("BlueprintType");
	static const FName ParentClassTagName("ParentClassPackage");
	static const FName AssetInterfacesTagName("ImplementedInterfaces");

	/**  */
	static IAssetRegistry& GetAssetRegistry();

	/**  */
	static void GatherAssetsToConvert(TArray<FAssetData>& ConversionQueueOut);

	/** */
	static void GatherParentAssets(const FAssetData& TargetAsset, TArray<FAssetData>& AssetsOut);

	/** */
	static void GatherInterfaceAssets(const FAssetData& TargetAsset, TArray<FAssetData>& AssetsOut);
}

/*******************************************************************************
 * FScopedAssetCollector
*******************************************************************************/

/**  */
struct FScopedAssetCollector
{
public:
	FScopedAssetCollector(const FARFilter& SharedFilter, const UClass* AssetType, TArray<FAssetData>& AssetsOut)
		: TargetType(AssetType)
	{
		if (FScopedAssetCollector** SpecializedCollector = SpecializedAssetCollectors.Find(TargetType))
		{
			(*SpecializedCollector)->GatherAssets(SharedFilter, AssetsOut);
		}
		else
		{
			GatherAssets(SharedFilter, AssetsOut);
		}
	}

protected:
	FScopedAssetCollector(const UClass* AssetType) : TargetType(AssetType) {}

	/**
	 * 
	 * 
	 * @param  SharedFilter    
	 * @param  AssetsOut    
	 */
	virtual void GatherAssets(const FARFilter& SharedFilter, TArray<FAssetData>& AssetsOut) const;

	const UClass* TargetType;
	static TMap<const UClass*, FScopedAssetCollector*> SpecializedAssetCollectors;
};
TMap<const UClass*, FScopedAssetCollector*> FScopedAssetCollector::SpecializedAssetCollectors;

//------------------------------------------------------------------------------
void FScopedAssetCollector::GatherAssets(const FARFilter& SharedFilter, TArray<FAssetData>& AssetsOut) const
{
	IAssetRegistry& AssetRegistry = BlueprintNativeCodeGenCoordinatorImpl::GetAssetRegistry();

	FARFilter ClassFilter;
	ClassFilter.Append(SharedFilter);
	ClassFilter.ClassNames.Add(TargetType->GetFName());

	if (ClassFilter.PackagePaths.Num() > 0)
	{
		// we want the union of specified package names and paths, not the 
		// intersection (how GetAssets() is set up to work), so here we query 
		// for both separately
		ClassFilter.PackageNames.Empty();
		AssetRegistry.GetAssets(ClassFilter, AssetsOut);
		
		if (SharedFilter.PackageNames.Num() > 0)
		{
			ClassFilter.PackageNames = SharedFilter.PackageNames;
			ClassFilter.PackageNames.Empty();
			AssetRegistry.GetAssets(ClassFilter, AssetsOut);
		}
	}
	else
	{
		AssetRegistry.GetAssets(ClassFilter, AssetsOut);
	}
}

/*******************************************************************************
 * FSpecializedAssetCollector<>
*******************************************************************************/

template<class AssetType>
struct FSpecializedAssetCollector : public FScopedAssetCollector
{
public:
	FSpecializedAssetCollector() : FScopedAssetCollector(AssetType::StaticClass())
	{
		SpecializedAssetCollectors.Add(AssetType::StaticClass(), this);
	}

	~FSpecializedAssetCollector()
	{
		SpecializedAssetCollectors.Remove(AssetType::StaticClass());
	}

	// FScopedAssetCollector interface
	virtual void GatherAssets(const FARFilter& SharedFilter, TArray<FAssetData>& AssetsOut) const override;
	// End FScopedAssetCollector interface
};

//------------------------------------------------------------------------------
template<>
void FSpecializedAssetCollector<UBlueprint>::GatherAssets(const FARFilter& SharedFilter, TArray<FAssetData>& AssetsOut) const
{
	FARFilter BlueprintFilter;
	BlueprintFilter.Append(SharedFilter);

	if (UByteProperty* TypeProperty = FindField<UByteProperty>(TargetType, BlueprintNativeCodeGenCoordinatorImpl::BlueprintTypePropertyName))
	{
		ensure(TypeProperty->HasAnyPropertyFlags(CPF_AssetRegistrySearchable));

		UEnum* TypeEnum = TypeProperty->Enum;
		if (ensure(TypeEnum != nullptr))
		{
			for (int32 BlueprintType = 0; BlueprintType < TypeEnum->GetMaxEnumValue(); ++BlueprintType)
			{
				if (!TypeEnum->IsValidEnumValue(BlueprintType))
				{
					continue;
				}

				// explicitly ignore unsupported Blueprint conversion types
				if ((BlueprintNativeCodeGenCoordinatorImpl::IncompatibleBlueprintTypeMask & (1 << BlueprintType)) != 0)
				{
					continue;
				}
				BlueprintFilter.TagsAndValues.Add(TypeProperty->GetFName(), TypeEnum->GetEnumName(BlueprintType));
			}
		}
	}

	const int32 StartingIndex = AssetsOut.Num();
	FScopedAssetCollector::GatherAssets(BlueprintFilter, AssetsOut);
	const int32 EndingIndex   = AssetsOut.Num();

	// have to ensure that we convert the entire inheritance chain
	for (int32 BlueprintIndex = StartingIndex; BlueprintIndex < EndingIndex; ++BlueprintIndex)
	{
		BlueprintNativeCodeGenCoordinatorImpl::GatherParentAssets(AssetsOut[BlueprintIndex], AssetsOut);
		BlueprintNativeCodeGenCoordinatorImpl::GatherInterfaceAssets(AssetsOut[BlueprintIndex], AssetsOut);
		// @TODO: Collect UserDefinedStructs (maybe enums?)... maybe put off till later, once the BP is loaded?
	}
}

/*******************************************************************************
 * BlueprintNativeCodeGenCoordinatorImpl definition
*******************************************************************************/

//------------------------------------------------------------------------------
static IAssetRegistry& BlueprintNativeCodeGenCoordinatorImpl::GetAssetRegistry()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	return AssetRegistryModule.Get();
}

//------------------------------------------------------------------------------
static void BlueprintNativeCodeGenCoordinatorImpl::GatherAssetsToConvert(TArray<FAssetData>& ConversionQueueOut)
{
	IAssetRegistry& AssetRegistry = GetAssetRegistry();
	// have to make sure that the asset registry is fully populated
	AssetRegistry.SearchAllAssets(/*bSynchronousSearch =*/true);

	FARFilter AssetFilter;
	AssetFilter.bRecursivePaths = true;

	const UBlueprintNativeCodeGenConfig* ConfigSettings = GetDefault<UBlueprintNativeCodeGenConfig>();
	IFileManager& FileManager = IFileManager::Get();

	for (const FString& PackagePath : ConfigSettings->PackagesToAlwaysConvert)
	{
		const FString RelativePath = FPackageName::LongPackageNameToFilename(PackagePath);
		if (FileManager.DirectoryExists(*RelativePath))
		{
			AssetFilter.PackagePaths.Add(*PackagePath);
		}
		else
		{
			AssetFilter.PackageNames.Add(*PackagePath);
		}
	}

	// will be utilized if UBlueprint is an entry in TargetAssetTypes (auto registers
	FSpecializedAssetCollector<UBlueprint> BlueprintAssetCollector;

	const int32 TargetClassCount = sizeof(TargetAssetTypes) / sizeof(TargetAssetTypes[0]);
	for (int32 ClassIndex = 0; ClassIndex < TargetClassCount; ++ClassIndex)
	{
		const UClass* TargetClass = TargetAssetTypes[ClassIndex];
		FScopedAssetCollector AssetCollector(AssetFilter, TargetClass, ConversionQueueOut);
	}
}

//------------------------------------------------------------------------------
static void BlueprintNativeCodeGenCoordinatorImpl::GatherParentAssets(const FAssetData& TargetAsset, TArray<FAssetData>& AssetsOut)
{
	IAssetRegistry& AssetRegistry = GetAssetRegistry();

	if (const FString* ParentClassPkg = TargetAsset.TagsAndValues.Find(ParentClassTagName))
	{
		TArray<FAssetData> ParentPackages;
		AssetRegistry.GetAssetsByPackageName(**ParentClassPkg, ParentPackages);

		// should either be 0 (when the parent is a native class), or 1 (the parent is another asset) 
		ensure(ParentPackages.Num() <= 1);

		for (const FAssetData& ParentData : ParentPackages)
		{
			AssetsOut.AddUnique(ParentData); // keeps us from converting parents that were already flagged for conversion
			GatherParentAssets(ParentData, AssetsOut);
			GatherInterfaceAssets(ParentData, AssetsOut);
		}
	}
}

//------------------------------------------------------------------------------
static void BlueprintNativeCodeGenCoordinatorImpl::GatherInterfaceAssets(const FAssetData& TargetAsset, TArray<FAssetData>& AssetsOut)
{
	IAssetRegistry& AssetRegistry = GetAssetRegistry();

	if (const FString* InterfacesListPtr = TargetAsset.TagsAndValues.Find(AssetInterfacesTagName))
	{
		static const FString InterfaceClassLabel(TEXT("(Interface="));
		static const FString BlueprintClassName = UBlueprintGeneratedClass::StaticClass()->GetName();
		static const FString ClassPathDelimiter = TEXT("'");
		static const FString PackageDelimiter   = TEXT(".");

		FString InterfacesList = *InterfacesListPtr;
		do
		{
			const int32 SearchIndex = InterfacesList.Find(InterfaceClassLabel);
			if (SearchIndex != INDEX_NONE)
			{
				int32 InterfaceClassIndex = SearchIndex + InterfaceClassLabel.Len();
				InterfacesList = InterfacesList.Mid(InterfaceClassIndex);

				if (InterfacesList.StartsWith(BlueprintClassName))
				{
					InterfaceClassIndex = BlueprintClassName.Len();

					const int32 ClassEndIndex = InterfacesList.Find(ClassPathDelimiter, ESearchCase::IgnoreCase, ESearchDir::FromStart, InterfaceClassIndex+1);
					const FString ClassPath = InterfacesList.Mid(InterfaceClassIndex+1, ClassEndIndex - InterfaceClassIndex);

					FString ClassPackagePath, ClassName;
					if (ClassPath.Split(PackageDelimiter, &ClassPackagePath, &ClassName))
					{
						TArray<FAssetData> InterfacePackages;
						AssetRegistry.GetAssetsByPackageName(*ClassPackagePath, InterfacePackages);

						// should find a single asset (since we know this is for a Blueprint generated class)
						ensure(InterfacePackages.Num() == 1);

						for (const FAssetData& Interface : InterfacePackages)
						{
							// AddUnique() keeps us from converting interfaces that were already flagged for conversion
							AssetsOut.AddUnique(Interface); 
						}
					}
				}
			}
			else
			{
				break;
			}
		} while (!InterfacesList.IsEmpty());
	}
}

/*******************************************************************************
 * FBlueprintNativeCodeGenCoordinator
*******************************************************************************/

//------------------------------------------------------------------------------
FBlueprintNativeCodeGenCoordinator::FBlueprintNativeCodeGenCoordinator(const FNativeCodeGenCommandlineParams& CommandlineParams)
{
	BlueprintNativeCodeGenCoordinatorImpl::GatherAssetsToConvert(ConversionQueue);
}
