// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
//#include "EnginePhysicsClasses.h"

DEFINE_LOG_CATEGORY_STATIC(LogEditorPhysMode, Log, All);

FPhysicsManipulationEdMode::FPhysicsManipulationEdMode()
{
	ID = FBuiltinEditorModes::EM_Physics;
	Name = NSLOCTEXT("EditorModes", "PhysicsMode", "Physics Mode");

	HandleComp = ConstructObject<UPhysicsHandleComponent>(UPhysicsHandleComponent::StaticClass(), GetTransientPackage(), NAME_None, RF_NoFlags);
}

FPhysicsManipulationEdMode::~FPhysicsManipulationEdMode()
{
	HandleComp = NULL;
}

void FPhysicsManipulationEdMode::Enter()
{
	HandleComp->RegisterComponentWithWorld(GetWorld());
}

void FPhysicsManipulationEdMode::Exit()
{
	HandleComp->UnregisterComponent();
}

void FPhysicsManipulationEdMode::PeekAtSelectionChangedEvent( UObject* ItemUndergoingChange )
{
	USelection* Selection = GEditor->GetSelectedActors();

	if (ItemUndergoingChange != NULL && ItemUndergoingChange->IsSelected())
	{
		AActor* SelectedActor = Cast<AActor>(ItemUndergoingChange);
		if (SelectedActor != NULL)
		{
			UPrimitiveComponent* PC = SelectedActor->GetRootPrimitiveComponent();
			if (PC != NULL && PC->BodyInstance.bSimulatePhysics)
			{
				GEditorModeTools().ActivateMode(ID, false);
				return;
			}
		}
	}
	else if (ItemUndergoingChange != NULL && !ItemUndergoingChange->IsA(USelection::StaticClass()))
	{
		GEditorModeTools().DeactivateMode(ID);
	}
}

bool FPhysicsManipulationEdMode::InputDelta( FLevelEditorViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale )
{
	UE_LOG(LogEditorPhysMode, Warning, TEXT("Mouse: %s InDrag: %s  InRot: %s"), *GEditor->MouseMovement.ToString(), *InDrag.ToString(), *InRot.ToString());

	const float GrabMoveSpeed = 1.0f;
	const float GrabRotateSpeed = 1.0f;

	if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
	{
		HandleTargetLocation += InDrag * GrabMoveSpeed;
		HandleTargetRotation += InRot;

		HandleComp->SetTargetLocation(HandleTargetLocation);
		HandleComp->SetTargetRotation(HandleTargetRotation);

		return true;
	}
	else
	{
		return FEdMode::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
	}
}

bool FPhysicsManipulationEdMode::StartTracking( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport )
{
	UE_LOG(LogEditorPhysMode, Warning, TEXT("Start Tracking"));

	FVector GrabLocation(0,0,0);
	UPrimitiveComponent* ComponentToGrab = NULL;
	
	USelection* Selection = GEditor->GetSelectedActors();

	for (int32 i=0; i<Selection->Num(); ++i)
	{
		AActor* SelectedActor = Cast<AActor>(Selection->GetSelectedObject(i));

		if (SelectedActor != NULL)
		{
			UPrimitiveComponent* PC = SelectedActor->GetRootPrimitiveComponent();

			if (PC != NULL && PC->BodyInstance.bSimulatePhysics)
			{
				ComponentToGrab = PC;
				
				HandleTargetLocation = SelectedActor->GetActorLocation();
				HandleTargetRotation = SelectedActor->GetActorRotation();
				break;
			}

			if (ComponentToGrab != NULL) { break; }
		}
	}

	if (ComponentToGrab != NULL)
	{
		HandleComp->GrabComponent(ComponentToGrab, NAME_None, ComponentToGrab->GetOwner()->GetActorLocation(), true);
	}

	return FEdMode::StartTracking(InViewportClient, InViewport);
}

bool FPhysicsManipulationEdMode::EndTracking( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport )
{
	UE_LOG(LogEditorPhysMode, Warning, TEXT("End Tracking"));

	HandleComp->ReleaseComponent();

	return FEdMode::EndTracking(InViewportClient, InViewport);
}

void FPhysicsManipulationEdMode::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject(HandleComp);
}
