// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"
#include "PersonaModule.h"
#include "Persona.h"
#include "BlueprintUtilities.h"
#include "AnimGraphDefinitions.h"
#include "Toolkits/ToolkitManager.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"
#include "PropertyEditorModule.h"
#include "SkeletalMeshSocketDetails.h"
#include "AnimNotifyDetails.h"
#include "AnimGraphNodeDetails.h"

IMPLEMENT_MODULE( FPersonaModule, Persona );

const FName PersonaAppName = FName(TEXT("PersonaApp"));


void FPersonaModule::StartupModule()
{
	MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);

	// Load all blueprint animnotifies from asset registry so they are available from drop downs in anim segment detail views
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		// Collect a full list of assets with the specified class
		TArray<FAssetData> AssetData;
		AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), AssetData);

		const FName BPParentClassName( TEXT( "ParentClass" ) );
		const FString BPAnimNotify( TEXT("Class'/Script/Engine.AnimNotify'" ));

		for (int32 AssetIndex = 0; AssetIndex < AssetData.Num(); ++AssetIndex)
		{
			bool FoundBPNotify = false;
			for (TMap<FName, FString>::TConstIterator TagIt(AssetData[ AssetIndex ].TagsAndValues); TagIt; ++TagIt)
			{
				FString TagValue = AssetData[ AssetIndex ].TagsAndValues.FindRef(BPParentClassName);
				if(TagValue == BPAnimNotify)
				{
					FoundBPNotify = true;
					break;
				}
			}

			if(FoundBPNotify)
			{
				FString BlueprintPath = AssetData[AssetIndex].ObjectPath.ToString();
				LoadObject<UBlueprint>(NULL, *BlueprintPath, NULL, 0, NULL);
			}
		}
	}
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyLayout( USkeletalMeshSocket::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FSkeletalMeshSocketDetails::MakeInstance ) );
		PropertyModule.RegisterCustomPropertyLayout( UAnimNotify::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FAnimNotifyDetails::MakeInstance ) );
		PropertyModule.RegisterCustomPropertyLayout( UAnimGraphNode_Base::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FAnimGraphNodeDetails::MakeInstance ) );

		PropertyModule.RegisterStructPropertyLayout( "InputScaleBias", FOnGetStructCustomizationInstance::CreateStatic( &FInputScaleBiasCustomization::MakeInstance ) );
		PropertyModule.RegisterStructPropertyLayout( "BoneReference", FOnGetStructCustomizationInstance::CreateStatic( &FBoneReferenceCustomization::MakeInstance ) );
	}
}

void FPersonaModule::ShutdownModule()
{
	MenuExtensibilityManager.Reset();
}


TSharedRef<IBlueprintEditor> FPersonaModule::CreatePersona( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, class USkeleton* Skeleton, class UAnimBlueprint* Blueprint, class UAnimationAsset* AnimationAsset, class USkeletalMesh * Mesh )
{
	TSharedRef< FPersona > NewPersona( new FPersona() );
	NewPersona->InitPersona( Mode, InitToolkitHost, Skeleton, Blueprint, AnimationAsset, Mesh );
	return NewPersona;
}
