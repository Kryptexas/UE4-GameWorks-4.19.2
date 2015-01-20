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
#include "ProceduralFoliageActor.h"
#include "ProceduralFoliageComponent.h"

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

		GUnrealEd->RegisterComponentVisualizer(UProceduralFoliageComponent::StaticClass()->GetFName(), MakeShareable(new FProceduralFoliageComponentVisualizer));

		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditor.RegisterCustomClassLayout("ProceduralFoliageComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FProceduralFoliageComponentDetails::MakeInstance));

		// Actor Factories
		UActorFactoryProceduralFoliage* ProceduralFoliageActorFactory = ConstructObject<UActorFactoryProceduralFoliage>(UActorFactoryProceduralFoliage::StaticClass());
		GEditor->ActorFactories.Add(ProceduralFoliageActorFactory);

#if WITH_EDITOR
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
		if (AProceduralFoliageActor* PFA = Cast<AProceduralFoliageActor>(Actor))
		{
			if (UProceduralFoliageComponent* ProceduralComponent = PFA->ProceduralComponent)
			{
				ProceduralComponent->RemoveProceduralContent();
			}
		}
	}

	void SubscribeEvents()
	{
		GEngine->OnLevelActorDeleted().Remove(OnLevelActorDeletedDelegateHandle);
		OnLevelActorDeletedDelegateHandle = GEngine->OnLevelActorDeleted().AddRaw(this, &FFoliageEditModule::OnLevelActorDeleted);
	}

	void UnsubscribeEvents()
	{
		GEngine->OnLevelActorDeleted().Remove(OnLevelActorDeletedDelegateHandle);
	}

	FDelegateHandle OnLevelActorDeletedDelegateHandle;
#endif
};

IMPLEMENT_MODULE( FFoliageEditModule, FoliageEdit );
