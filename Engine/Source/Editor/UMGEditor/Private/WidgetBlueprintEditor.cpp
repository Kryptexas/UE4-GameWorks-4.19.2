// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetBlueprintEditor.h"

#define LOCTEXT_NAMESPACE "UMG"

FWidgetBlueprintEditor::FWidgetBlueprintEditor()
	: PreviewScene(FPreviewScene::ConstructionValues().AllowAudioPlayback(true).ShouldSimulatePhysics(true))
	, PreviewBlueprint(NULL)
{
}

FWidgetBlueprintEditor::~FWidgetBlueprintEditor()
{
	UWidgetBlueprint* Blueprint = GetWidgetBlueprintObj();
	if ( Blueprint )
	{
		Blueprint->OnChanged().RemoveAll(this);
	}
}

void FWidgetBlueprintEditor::OnBlueprintChanged(UBlueprint* InBlueprint)
{
	if ( InBlueprint )
	{
		UpdatePreview(InBlueprint, true);
	}
}

void FWidgetBlueprintEditor::InitWidgetBlueprintEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode)
{
	InitBlueprintEditor(Mode, InitToolkitHost, InBlueprints, bShouldOpenInDefaultsMode);

	UWidgetBlueprint* Blueprint = GetWidgetBlueprintObj();
	Blueprint->OnChanged().AddSP(this, &FWidgetBlueprintEditor::OnBlueprintChanged);

	UpdatePreview(GetWidgetBlueprintObj(), true);
}

void FWidgetBlueprintEditor::Tick(float DeltaTime)
{
	FBlueprintEditor::Tick(DeltaTime);

	// Tick the preview scene world.
	if ( !GIntraFrameDebuggingGameThread )
	{
		bool bIsSimulateEnabled = true;
		bool bIsRealTime = true;

		// Allow full tick only if preview simulation is enabled and we're not currently in an active SIE or PIE session
		if ( bIsSimulateEnabled && GEditor->PlayWorld == NULL && !GEditor->bIsSimulatingInEditor )
		{
			PreviewScene.GetWorld()->Tick(bIsRealTime ? LEVELTICK_All : LEVELTICK_TimeOnly, DeltaTime);
		}
		else
		{
			PreviewScene.GetWorld()->Tick(bIsRealTime ? LEVELTICK_ViewportsOnly : LEVELTICK_TimeOnly, DeltaTime);
		}
	}
}

void FWidgetBlueprintEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged)
{
	UpdatePreview(GetWidgetBlueprintObj(), true);
}

UWidgetBlueprint* FWidgetBlueprintEditor::GetWidgetBlueprintObj() const
{
	return Cast<UWidgetBlueprint>(GetBlueprintObj());
}

UUserWidget* FWidgetBlueprintEditor::GetPreview() const
{
	//// Note: The weak ptr can become stale if the actor is reinstanced due to a Blueprint change, etc. In that case we look to see if we can find the new instance in the preview world and then update the weak ptr.
	//if ( PreviewWidgetActorPtr.IsStale(true) )
	//{
	//	UWorld* PreviewWorld = PreviewWidgetActorPtr->GetWorld();
	//	for ( TActorIterator<AActor> It(PreviewWorld); It; ++It )
	//	{
	//		AActor* Actor = *It;
	//		if ( !Actor->IsPendingKillPending()
	//			&& Actor->GetClass()->ClassGeneratedBy == PreviewBlueprint )
	//		{
	//			PreviewActorPtr = Actor;
	//			break;
	//		}
	//	}
	//}

	return PreviewWidgetActorPtr.Get();
}

void FWidgetBlueprintEditor::DestroyPreview()
{
	UUserWidget* PreviewActor = GetPreview();
	if ( PreviewActor != NULL )
	{
		check(PreviewScene.GetWorld());

		PreviewActor->ClearFlags(RF_Standalone);
		PreviewActor->MarkPendingKill();
	}

	//if ( PreviewBlueprint != NULL )
	//{
	//	if ( PreviewBlueprint->SimpleConstructionScript != NULL
	//		&& PreviewActor == PreviewBlueprint->SimpleConstructionScript->GetComponentEditorActorInstance() )
	//	{
	//		// Ensure that all editable component references are cleared
	//		PreviewBlueprint->SimpleConstructionScript->ClearEditorComponentReferences();

	//		// Clear the reference to the preview actor instance
	//		PreviewBlueprint->SimpleConstructionScript->SetComponentEditorActorInstance(NULL);
	//	}

	//	PreviewBlueprint = NULL;
	//}
}

void FWidgetBlueprintEditor::UpdatePreview(UBlueprint* InBlueprint, bool bInForceFullUpdate)
{
	UUserWidget* PreviewActor = GetPreview();

	// Signal that we're going to be constructing editor components
	if ( InBlueprint != NULL && InBlueprint->SimpleConstructionScript != NULL )
	{
		InBlueprint->SimpleConstructionScript->BeginEditorComponentConstruction();
	}

	// If the Blueprint is changing
	if ( InBlueprint != PreviewBlueprint || bInForceFullUpdate )
	{
		// Destroy the previous actor instance
		DestroyPreview();

		// Save the Blueprint we're creating a preview for
		PreviewBlueprint = InBlueprint;

		PreviewActor = ConstructObject<UUserWidget>(PreviewBlueprint->GeneratedClass, PreviewScene.GetWorld()->GetCurrentLevel());
		PreviewActor->SetFlags(RF_Standalone);

		PreviewWidgetActorPtr = PreviewActor;
	}
}

#undef LOCTEXT_NAMESPACE
