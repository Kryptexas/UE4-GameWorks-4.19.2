// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorDockableWindow.h"
#include "VREditorUISystem.h"
#include "VREditorMode.h"
#include "VREditorBaseUserWidget.h"
#include "WidgetComponent.h"

namespace VREd
{
	static FAutoConsoleVariable DockUIZBarOffset( TEXT( "VREd.DockUIZBarOffset" ), 0.5f, TEXT( "Z Distance between the selectionbar and the UI" ) );
}

AVREditorDockableWindow::AVREditorDockableWindow() 
	: Super(),
	SelectionBarScale( FVector(1.0f, 1.0f, 1.0f) )
{
	UStaticMesh* SelectionMesh = nullptr;
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/SM_ContentWindow_01" ) );
		SelectionMesh = ObjectFinder.Object; 
		check( SelectionMesh != nullptr );
	}

	SelectionBar = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "SelectionBar" ) ); 
	SelectionBar->SetStaticMesh( SelectionMesh );
	SelectionBar->SetMobility( EComponentMobility::Movable );
	SelectionBar->AttachTo( RootComponent );

	SelectionBar->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
	SelectionBar->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Block );
	SelectionBar->SetCollisionResponseToChannel( ECC_EditorGizmo, ECollisionResponse::ECR_Block );

	SelectionBar->bGenerateOverlapEvents = false;
	SelectionBar->SetCanEverAffectNavigation( false );
	SelectionBar->bCastDynamicShadow = false;
	SelectionBar->bCastStaticShadow = false;
	SelectionBar->bAffectDistanceFieldLighting = false;
}

void AVREditorDockableWindow::TickManually( float DeltaTime )
{
	Super::TickManually( DeltaTime );

	if( WidgetComponent->IsVisible() )
	{
		const FVector AnimatedScale = CalculateAnimatedScale();
		SelectionBar->SetWorldScale3D( ( FVector( 0.001f, 1.0 / (float)Resolution.X, 1.0f / (float)Resolution.Y ) * ( Size *  GetOwner().GetOwner().GetWorldScaleFactor() ) ) * 10 * AnimatedScale );
	}
} 

void AVREditorDockableWindow::UpdateRelativeRoomTransform()
{
	const FTransform RoomToWorld = GetOwner().GetOwner().GetRoomTransform();
	const FTransform WorldToRoom = RoomToWorld.Inverse();

	const FTransform WindowToWorldTransform = GetActorTransform();
	const FTransform WindowToRoomTransform = WindowToWorldTransform * WorldToRoom;

	const FVector RoomSpaceWindowLocation = WindowToRoomTransform.GetLocation() / GetOwner().GetOwner().GetWorldScaleFactor();
	const FQuat RoomSpaceWindowRotation = WindowToRoomTransform.GetRotation();

	RelativeOffset = RoomSpaceWindowLocation;
	LocalRotation = RoomSpaceWindowRotation.Rotator();
}
