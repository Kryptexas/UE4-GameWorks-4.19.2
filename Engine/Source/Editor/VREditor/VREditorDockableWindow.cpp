// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorDockableWindow.h"
#include "VREditorUISystem.h"
#include "VREditorMode.h"
#include "VREditorBaseUserWidget.h"
#include "WidgetComponent.h"

namespace VREd
{
	static FAutoConsoleVariable DockWindowThickness( TEXT( "VREd.DockWindowTickness" ), 1.0f, TEXT( "How thick the window is" ) );
	static FAutoConsoleVariable DockUIZBarOffset( TEXT( "VREd.DockUIZBarOffset" ), 2.0f, TEXT( "Z Distance between the selectionbar and the UI" ) );
}

AVREditorDockableWindow::AVREditorDockableWindow() 
	: Super()
{

	{
		UStaticMesh* WindowMesh = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/UI/SM_ContentWindow_01" ) );
			WindowMesh = ObjectFinder.Object;
			check( WindowMesh != nullptr );
		}

		WindowMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "WindowMesh" ) );
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
			static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/UI/SelectionBarMesh" ) );
			SelectionMesh = ObjectFinder.Object;
			check( SelectionMesh != nullptr );
		}

		SelectionMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "SelectionBarMesh" ) );
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

		UMaterial* HoverMaterial = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UMaterial> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TransformGizmoMaterial" ) );
			HoverMaterial = ObjectFinder.Object;
			check( HoverMaterial != nullptr );
		}
		HoverMaterialMID = UMaterialInstanceDynamic::Create( HoverMaterial, GetTransientPackage() );
		check( HoverMaterialMID != nullptr );
		SelectionMeshComponent->SetMaterial( 0, HoverMaterialMID );

		UMaterial* TranslucentHoverMaterial = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UMaterial> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TranslucentTransformGizmoMaterial" ) );
			TranslucentHoverMaterial = ObjectFinder.Object;
			check( TranslucentHoverMaterial != nullptr );
		}
		TranslucentHoverMID = UMaterialInstanceDynamic::Create( TranslucentHoverMaterial, GetTransientPackage() );
		check( TranslucentHoverMID != nullptr );
		SelectionMeshComponent->SetMaterial( 1, TranslucentHoverMID );
	}
}


void AVREditorDockableWindow::SetupWidgetComponent()
{
	Super::SetupWidgetComponent();

	SetSelectionBarColor( GetOwner().GetOwner().GetColor( FVREditorMode::EColors::UISelectionBarColor ) );
}


void AVREditorDockableWindow::TickManually( float DeltaTime )
{
	Super::TickManually( DeltaTime );

	if( WidgetComponent->IsVisible() )
	{
		// How big the selection bar should be
		const FVector SelectionBarSize( 1.0f, GetSize().X * 0.8f, 6.0f );	// @todo vreditor tweak

		const float WorldScaleFactor = GetOwner().GetOwner().GetWorldScaleFactor();
		const FVector AnimatedScale = CalculateAnimatedScale();

		const float WindowMeshSize = 100.0f;	// Size of imported mesh, we need to inverse compensate for
		const FVector WindowMeshScale = FVector(
			VREd::DockWindowThickness->GetFloat(),
			GetSize().X / WindowMeshSize,
			GetSize().Y / WindowMeshSize ) * AnimatedScale * WorldScaleFactor;
		WindowMeshComponent->SetRelativeScale3D( WindowMeshScale );

		const FVector SelectionMeshScale = SelectionBarSize * AnimatedScale * WorldScaleFactor;
		SelectionMeshComponent->SetRelativeScale3D( SelectionMeshScale );
		const FVector SelectionMeshRelativeLocation = FVector(
			0.0f,
			0.0f,
			-( GetSize().Y * 0.5f + SelectionBarSize.Z * 0.5f + VREd::DockUIZBarOffset->GetFloat() ) ) * AnimatedScale * WorldScaleFactor;
		SelectionMeshComponent->SetRelativeLocation( SelectionMeshRelativeLocation );
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

void AVREditorDockableWindow::OnEnterHover( const FHitResult& HitResult )
{
	if( SelectionMeshComponent == HitResult.GetComponent() )
	{
		SetSelectionBarColor( GetOwner().GetOwner().GetColor( FVREditorMode::EColors::HoverGizmoColor ) );
	}
}

void AVREditorDockableWindow::OnLeaveHover()
{
	SetSelectionBarColor( GetOwner().GetOwner().GetColor( FVREditorMode::EColors::UISelectionBarColor ) );
}

void AVREditorDockableWindow::SetSelectionBarColor( const FLinearColor& LinearColor )
{
	static FName StaticColorParameterName( "Color" );
	HoverMaterialMID->SetVectorParameterValue( StaticColorParameterName, LinearColor );
	TranslucentHoverMID->SetVectorParameterValue( StaticColorParameterName, LinearColor );
}