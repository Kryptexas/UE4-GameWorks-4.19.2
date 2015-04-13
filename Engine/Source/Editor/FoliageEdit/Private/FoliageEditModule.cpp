// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "FoliageEditModule.h"

const FName FoliageEditAppIdentifier = FName(TEXT("FoliageEdApp"));

#include "FoliageEdMode.h"
#include "PropertyEditing.h"
#include "FoliageTypeDetails.h"
#include "ProceduralFoliageComponent.h"
#include "ProceduralFoliageComponentVisualizer.h"
#include "ProceduralFoliageComponentDetails.h"
#include "ActorFactoryProceduralFoliage.h"
#include "ComponentVisualizer.h"
#include "ProceduralFoliageVolume.h"
#include "ProceduralFoliageBlockingVolume.h"
#include "ProceduralFoliageComponent.h"
#include "FoliageTypeObjectCustomization.h"

/**
 * Foliage Edit Mode module
 */
class FFoliageEditModule : public IFoliageEditModule
{
public:

	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule() override
	{
		FEditorModeRegistry::Get().RegisterMode<FEdModeFoliage>(
			FBuiltinEditorModes::EM_Foliage,
			NSLOCTEXT("EditorModes", "FoliageMode", "Foliage"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.FoliageMode", "LevelEditor.FoliageMode.Small"),
			true, 400
			);

		// Register the details customizer
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout("FoliageType", FOnGetDetailCustomizationInstance::CreateStatic(&FFoliageTypeDetails::MakeInstance));
		PropertyModule.RegisterCustomPropertyTypeLayout("FoliageTypeObject", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFoliageTypeObjectCustomization::MakeInstance));
		
		GUnrealEd->RegisterComponentVisualizer(UProceduralFoliageComponent::StaticClass()->GetFName(), MakeShareable(new FProceduralFoliageComponentVisualizer));

		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditor.RegisterCustomClassLayout("ProceduralFoliageComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FProceduralFoliageComponentDetails::MakeInstance));

		// Actor Factories
		auto ProceduralFoliageVolumeFactory = NewObject<UActorFactoryProceduralFoliage>();
		GEditor->ActorFactories.Add(ProceduralFoliageVolumeFactory);

#if WITH_EDITOR
		// Volume placeability
		if (!GetDefault<UEditorExperimentalSettings>()->bProceduralFoliage)
		{
			AProceduralFoliageVolume::StaticClass()->ClassFlags |= CLASS_NotPlaceable;
			AProceduralFoliageBlockingVolume::StaticClass()->ClassFlags |= CLASS_NotPlaceable;
		}

		SubscribeEvents();
#endif
	}

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule() override
	{
		FEditorModeRegistry::Get().UnregisterMode(FBuiltinEditorModes::EM_Foliage);

		if (!UObjectInitialized())
		{
			return;
		}

#if WITH_EDITOR
		UnsubscribeEvents();
#endif

		// Unregister the details customization
		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyModule.UnregisterCustomClassLayout("FoliageType");
			PropertyModule.NotifyCustomizationModuleChanged();
		}
	}

#if WITH_EDITOR

	void OnLevelActorDeleted(AActor* Actor)
	{
		if (AProceduralFoliageVolume* ProceduralFoliageVolume = Cast<AProceduralFoliageVolume>(Actor))
		{
			if (UProceduralFoliageComponent* ProceduralComponent = ProceduralFoliageVolume->ProceduralComponent)
			{
				ProceduralComponent->RemoveProceduralContent();
			}
		}
	}

	void SubscribeEvents()
	{
		GEngine->OnLevelActorDeleted().Remove(OnLevelActorDeletedDelegateHandle);
		OnLevelActorDeletedDelegateHandle = GEngine->OnLevelActorDeleted().AddRaw(this, &FFoliageEditModule::OnLevelActorDeleted);

		auto ExperimentalSettings = GetMutableDefault<UEditorExperimentalSettings>();
		ExperimentalSettings->OnSettingChanged().Remove(OnExperimentalSettingChangedDelegateHandle);
		OnExperimentalSettingChangedDelegateHandle =  ExperimentalSettings->OnSettingChanged().AddRaw(this, &FFoliageEditModule::HandleExperimentalSettingChanged);
	}

	void UnsubscribeEvents()
	{
		GEngine->OnLevelActorDeleted().Remove(OnLevelActorDeletedDelegateHandle);
		GetMutableDefault<UEditorExperimentalSettings>()->OnSettingChanged().Remove(OnExperimentalSettingChangedDelegateHandle);
	}

	void HandleExperimentalSettingChanged(FName PropertyName)
	{
		if (GetDefault<UEditorExperimentalSettings>()->bProceduralFoliage)
		{
			AProceduralFoliageVolume::StaticClass()->ClassFlags &= ~CLASS_NotPlaceable;
			AProceduralFoliageBlockingVolume::StaticClass()->ClassFlags &= ~CLASS_NotPlaceable;
		}
		else
		{
			AProceduralFoliageVolume::StaticClass()->ClassFlags |= CLASS_NotPlaceable;
			AProceduralFoliageBlockingVolume::StaticClass()->ClassFlags |= CLASS_NotPlaceable;
		}
	}

	FDelegateHandle OnLevelActorDeletedDelegateHandle;
	FDelegateHandle OnExperimentalSettingChangedDelegateHandle;
#endif
};

IMPLEMENT_MODULE( FFoliageEditModule, FoliageEdit );
