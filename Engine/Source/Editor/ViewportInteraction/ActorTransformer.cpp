// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ActorTransformer.h"
#include "ActorViewportTransformable.h"
#include "ViewportWorldInteraction.h"
#include "Engine/Selection.h"


void UActorTransformer::Init( UViewportWorldInteraction* InitViewportWorldInteraction )
{
	Super::Init( InitViewportWorldInteraction );

	// Find out about selection changes
	USelection::SelectionChangedEvent.AddUObject( this, &UActorTransformer::OnActorSelectionChanged );
}


void UActorTransformer::Shutdown()
{
	USelection::SelectionChangedEvent.RemoveAll( this );

	Super::Shutdown();
}


void UActorTransformer::OnActorSelectionChanged( UObject* ChangedObject )
{
	TArray<TUniquePtr<FViewportTransformable>> NewTransformables;

	USelection* ActorSelectionSet = GEditor->GetSelectedActors();

	static TArray<UObject*> SelectedActorObjects;
	SelectedActorObjects.Reset();
	ActorSelectionSet->GetSelectedObjects( AActor::StaticClass(), /* Out */ SelectedActorObjects );

	for( UObject* SelectedActorObject : SelectedActorObjects )
	{
		AActor* SelectedActor = CastChecked<AActor>( SelectedActorObject );

		FActorViewportTransformable* Transformable = new FActorViewportTransformable();

		NewTransformables.Add( TUniquePtr<FViewportTransformable>( Transformable ) );

		Transformable->ActorWeakPtr = SelectedActor;
		Transformable->StartTransform = Transformable->LastTransform = Transformable->TargetTransform = Transformable->UnsnappedTargetTransform = Transformable->InterpolationSnapshotTransform = SelectedActor->GetTransform();
	}

	ViewportWorldInteraction->SetTransformables( MoveTemp( NewTransformables ) );
}

