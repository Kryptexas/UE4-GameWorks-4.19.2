// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorDockableWindow.h"
#include "VREditorUISystem.h"
#include "VREditorMode.h"
#include "VREditorBaseUserWidget.h"
#include "WidgetComponent.h"

namespace VREd
{
	static FAutoConsoleVariable DockWindowTickness( TEXT( "VREd.DockWindowTickness" ), 0.003f, TEXT( "Z Distance between the selectionbar and the UI" ) );
	static FAutoConsoleVariable DockUIZBarOffset( TEXT( "VREd.DockUIZBarOffset" ), 1.0f, TEXT( "Z Distance between the selectionbar and the UI" ) );
}

AVREditorDockableWindow::AVREditorDockableWindow() 
	: Super(),
	SelectionMeshScale( FVector(0.6f, 2.0f, 1.2f) )
{

	{
		UStaticMesh* WindowMesh = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/SM_ContentWindow_01" ) );
			WindowMesh = ObjectFinder.Object;
			check( WindowMesh != nullptr );
		}

		WindowMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "SelectionBar" ) );
		WindowMeshComponent->SetStaticMesh( WindowMesh );
		WindowMeshComponent->SetMobility( EComponentMobility::Movable );
		WindowMeshComponent->AttachTo( RootComponent );

		WindowMeshComponent->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
		WindowMeshComponent->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Block );
		WindowMeshComponent->SetCollisionResponseToChannel( ECC_EditorGizmo, ECollisionResponse::ECR_Block );

		WindowMeshComponent->bGenerateOverlapEvents = false;
		WindowMeshComponent->SetCanEverAffectNavigation( false );
		WindowMeshComponent->bCastDynamicShadow = false;
		WindowMeshComponent->bCastStaticShadow = false;
		WindowMeshComponent->bAffectDistanceFieldLighting = false;
	}

	{
		UStaticMesh* SelectionMesh = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/UniformScaleHandle" ) );
			SelectionMesh = ObjectFinder.Object;
			check( SelectionMesh != nullptr );
		}

		SelectionMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "SelectionMesh" ) );
		SelectionMeshComponent->SetStaticMesh( SelectionMesh );
		SelectionMeshComponent->SetMobility( EComponentMobility::Movable );
		SelectionMeshComponent->AttachTo( RootComponent );

		SelectionMeshComponent->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
		SelectionMeshComponent->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Block );
		SelectionMeshComponent->SetCollisionResponseToChannel( ECC_EditorGizmo, ECollisionResponse::ECR_Block );

		SelectionMeshComponent->bGenerateOverlapEvents = false;
		SelectionMeshComponent->SetCanEverAffectNavigation( false );
		SelectionMeshComponent->bCastDynamicShadow = false;
		SelectionMeshComponent->bCastStaticShadow = false;
		SelectionMeshComponent->bAffectDistanceFieldLighting = false;
	}

}

void AVREditorDockableWindow::TickManually( float DeltaTime )
{
	Super::TickManually( DeltaTime );

	if( WidgetComponent->IsVisible() )
	{
		const FVector AnimatedScale = CalculateAnimatedScale();
		const float WordScaleFactor = GetOwner().GetOwner().GetWorldScaleFactor();
		const FVector SelectionMeshWorldScale = SelectionMeshScale * WordScaleFactor;
		SelectionMeshComponent->SetWorldScale3D( SelectionMeshWorldScale * AnimatedScale );

		const FVector WindowMeshWorldScale = FVector( VREd::DockWindowTickness->GetFloat() , 1.0 / (float)Resolution.X, 1.0f / (float)Resolution.Y  ) * ( Size *  GetOwner().GetOwner().GetWorldScaleFactor() ) * AnimatedScale;
		WindowMeshComponent->SetWorldScale3D( WindowMeshWorldScale * 10 );	
		
		const FVector NewRelativeLocation = FVector( 0.f, 0.f, -(((WindowMeshWorldScale.Z * 0.5f) * 1000) + ((SelectionMeshWorldScale.Z * 0.5) * 10) + (VREd::DockUIZBarOffset->GetFloat() * WordScaleFactor)) );
		SelectionMeshComponent->SetRelativeLocation( NewRelativeLocation );
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
