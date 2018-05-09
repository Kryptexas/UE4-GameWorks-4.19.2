// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LevelEditorMenuExtensions/LevelEditorMenuExtensions.h"
#include "Modules/ModuleManager.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Commands/UICommandList.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "LevelEditor.h"
#include "Editor.h"

#include "FlexActor.h"
#include "FlexComponent.h"


#define LOCTEXT_NAMESPACE "Flex"

//////////////////////////////////////////////////////////////////////////

FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors LevelEditorMenuExtenderDelegate;
FDelegateHandle LevelEditorExtenderDelegateHandle;

//////////////////////////////////////////////////////////////////////////
// FFlexLevelEditorMenuExtensions_Impl

class FFlexLevelEditorMenuExtensions_Impl
{
public:
	static void OnKeepFlexSimulationChanges()
	{
		// @todo simulate: There are lots of types of changes that can't be "kept", like attachment or newly-spawned actors.  This
		//    feature currently only supports propagating changes to regularly-editable properties on an instance of a PIE actor
		//    that still exists in the editor world.

		// Make sure we have some actors selected, and PIE is running
		if (GEditor->GetSelectedActorCount() > 0 && GEditor->PlayWorld != NULL)
		{
			int32 UpdatedActorCount = 0;
			int32 TotalCopiedPropertyCount = 0;
			FString FirstUpdatedActorLabel;
			{
				for (auto ActorIt(GEditor->GetSelectedActorIterator()); ActorIt; ++ActorIt)
				{
					auto* SimWorldActor = CastChecked<AActor>(*ActorIt);

					// Find our counterpart actor
					AActor* EditorWorldActor = EditorUtilities::GetEditorWorldCounterpartActor(SimWorldActor);
					if (EditorWorldActor != NULL)
					{
						// save flex actors' simulated positions, note does not support undo
						AFlexActor* FlexEditorActor = Cast<AFlexActor>(EditorWorldActor);
						AFlexActor* FlexSimActor = Cast<AFlexActor>(SimWorldActor);
						if (FlexEditorActor != NULL && FlexSimActor != NULL)
						{
							UFlexComponent* FlexEditorComponent = (UFlexComponent*)(FlexEditorActor->GetRootComponent());
							UFlexComponent* FlexSimComponent = (UFlexComponent*)(FlexSimActor->GetRootComponent());
							if (FlexEditorComponent != NULL && FlexSimComponent != NULL)
							{
								FlexEditorComponent->PreSimPositions.SetNum(FlexSimComponent->SimPositions.Num());

								for (int i = 0; i < FlexSimComponent->SimPositions.Num(); ++i)
									FlexEditorComponent->PreSimPositions[i] = FlexSimComponent->SimPositions[i];

								FlexEditorComponent->SavedRelativeLocation = FlexEditorComponent->RelativeLocation;
								FlexEditorComponent->SavedRelativeRotation = FlexEditorComponent->RelativeRotation;
								FlexEditorComponent->SavedTransform = FlexEditorComponent->GetComponentTransform();

								FlexEditorComponent->PreSimRelativeLocation = FlexSimComponent->RelativeLocation;
								FlexEditorComponent->PreSimRelativeRotation = FlexSimComponent->RelativeRotation;
								FlexEditorComponent->PreSimTransform = FlexSimComponent->GetComponentTransform();

								FlexEditorComponent->PreSimShapeTranslations = FlexSimComponent->PreSimShapeTranslations;
								FlexEditorComponent->PreSimShapeRotations = FlexSimComponent->PreSimShapeRotations;

								FlexEditorComponent->SendRenderTransform_Concurrent();

								// not currently used
								++UpdatedActorCount;
								++TotalCopiedPropertyCount;

								if (FirstUpdatedActorLabel.IsEmpty())
								{
									FirstUpdatedActorLabel = EditorWorldActor->GetActorLabel();
								}
							}
						}
					}
				}
			}
		}
	}

	static void OnClearFlexSimulationChanges()
	{
		if (GEditor->GetSelectedActorCount() > 0)
		{
			for (auto ActorIt(GEditor->GetSelectedActorIterator()); ActorIt; ++ActorIt)
			{
				// Find our counterpart actor
				AActor* EditorWorldActor = NULL;

				if (GEditor->PlayWorld != NULL)
				{
					// if PIE session active then find editor actor
					auto* SimWorldActor = CastChecked<AActor>(*ActorIt);
					EditorWorldActor = EditorUtilities::GetEditorWorldCounterpartActor(SimWorldActor);
				}
				else
				{
					EditorWorldActor = Cast<AActor>(*ActorIt);
				}

				if (EditorWorldActor != NULL)
				{
					// clear flex actors' simulated positions, note does not support undo
					AFlexActor* FlexEditorActor = Cast<AFlexActor>(EditorWorldActor);
					if (FlexEditorActor != NULL)
					{
						UFlexComponent* FlexEditorComponent = (UFlexComponent*)(FlexEditorActor->GetRootComponent());
						if (FlexEditorComponent != NULL && FlexEditorComponent->PreSimPositions.Num())
						{
							FlexEditorComponent->PreSimPositions.SetNum(0);
							FlexEditorComponent->PreSimShapeTranslations.SetNum(0);
							FlexEditorComponent->PreSimShapeRotations.SetNum(0);

							FlexEditorComponent->RelativeLocation = FlexEditorComponent->SavedRelativeLocation;
							FlexEditorComponent->RelativeRotation = FlexEditorComponent->SavedRelativeRotation;
							FlexEditorComponent->SetComponentToWorld(FlexEditorComponent->SavedTransform);

							FlexEditorComponent->SendRenderTransform_Concurrent();
						}
					}
				}
			}
		}
	}

	static void CreateFlexActionMenuEntries(FMenuBuilder& MenuBuilder)
	{
		MenuBuilder.BeginSection("Flex", NSLOCTEXT("LevelViewportContextMenu", "FlexHeading", "Flex"));
		{
			if (GEditor->PlayWorld != nullptr)
			{
				FUIAction Action_KeepFlexSimChanges(FExecuteAction::CreateStatic(&FFlexLevelEditorMenuExtensions_Impl::OnKeepFlexSimulationChanges));

				MenuBuilder.AddMenuEntry(
					LOCTEXT("KeepFlexSimulationChanges", "Keep Flex Simulation Changes"),
					LOCTEXT("KeepFlexSimulationChanges_Tooltip", "Saves the changes made to this actor in Simulate mode to the actor's default state."),
					FSlateIcon(),
					Action_KeepFlexSimChanges,
					NAME_None,
					EUserInterfaceActionType::Button
				);
			}

			FUIAction Action_ClearFlexSimChanges(FExecuteAction::CreateStatic(&FFlexLevelEditorMenuExtensions_Impl::OnClearFlexSimulationChanges));
			MenuBuilder.AddMenuEntry(
				LOCTEXT("ClearFlexSimulationChanges", "Clear Flex Simulation Changes"),
				LOCTEXT("ClearFlexSimulationChanges_Tooltip", "Discards any saved changes made to this actor in Simulate mode."),
				FSlateIcon(),
				Action_ClearFlexSimChanges,
				NAME_None,
				EUserInterfaceActionType::Button
			);
		}
		MenuBuilder.EndSection();
	}

	static TSharedRef<FExtender> OnExtendLevelEditorMenu(const TSharedRef<FUICommandList> CommandList, TArray<AActor*> SelectedActors)
	{
		TSharedRef<FExtender> Extender(new FExtender());

		// Only show these menu items if there's at least one Flex actor selected. Currently that means just AFlexActor.
		bool bFlexSelected = false;

		for (AActor* Actor : SelectedActors)
		{
			if (Actor->IsA(AFlexActor::StaticClass()))
			{
				bFlexSelected = true;
				break;
			}
		}

		if (bFlexSelected)
		{
			Extender->AddMenuExtension(
				"ActorSelectVisibilityLevels",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateStatic(&FFlexLevelEditorMenuExtensions_Impl::CreateFlexActionMenuEntries));

		}

		return Extender;
	}
};

//////////////////////////////////////////////////////////////////////////
// FFlexLevelEditorMenuExtensions

void FFlexLevelEditorMenuExtensions::InstallHooks()
{
	LevelEditorMenuExtenderDelegate = FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateStatic(&FFlexLevelEditorMenuExtensions_Impl::OnExtendLevelEditorMenu);

	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	auto& MenuExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
	MenuExtenders.Add(LevelEditorMenuExtenderDelegate);
	LevelEditorExtenderDelegateHandle = MenuExtenders.Last().GetHandle();
}

void FFlexLevelEditorMenuExtensions::RemoveHooks()
{
	// Remove level viewport context menu extenders
	if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetAllLevelViewportContextMenuExtenders().RemoveAll([&](const FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors& Delegate) {
			return Delegate.GetHandle() == LevelEditorExtenderDelegateHandle;
		});
	}
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
