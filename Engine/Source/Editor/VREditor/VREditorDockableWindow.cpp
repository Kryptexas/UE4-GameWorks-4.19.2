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
	static FAutoConsoleVariable DockUISelectionBarVerticalOffset( TEXT( "VREd.DockUISelectionBarVerticalOffset" ), 2.0f, TEXT( "Z Distance between the selectionbar and the UI" ) );
	static FAutoConsoleVariable DockUICloseButtonOffsetFromCorner( TEXT( "VREd.DockUICloseButtonOffsetFromCorner" ), 2.0f, TEXT( "How far away from the corner (along each 2D axis) of the UI the close button draws" ) );
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

		SelectionBarMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "SelectionBarMesh" ) );
		SelectionBarMeshComponent->SetStaticMesh( SelectionMesh );
		SelectionBarMeshComponent->SetMobility( EComponentMobility::Movable );
		SelectionBarMeshComponent->AttachTo( RootComponent );

		SelectionBarMeshComponent->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
		SelectionBarMeshComponent->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Block );
		SelectionBarMeshComponent->SetCollisionResponseToChannel( ECC_EditorGizmo, ECollisionResponse::ECR_Block );

		SelectionBarMeshComponent->bGenerateOverlapEvents = false;
		SelectionBarMeshComponent->SetCanEverAffectNavigation( false );
		SelectionBarMeshComponent->bCastDynamicShadow = false;
		SelectionBarMeshComponent->bCastStaticShadow = false;
		SelectionBarMeshComponent->bAffectDistanceFieldLighting = false;

		UMaterial* HoverMaterial = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UMaterial> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TransformGizmoMaterial" ) );
			HoverMaterial = ObjectFinder.Object;
			check( HoverMaterial != nullptr );
		}
		SelectionBarMID = UMaterialInstanceDynamic::Create( HoverMaterial, GetTransientPackage() );
		check( SelectionBarMID != nullptr );
		SelectionBarMeshComponent->SetMaterial( 0, SelectionBarMID );

		UMaterial* TranslucentHoverMaterial = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UMaterial> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TranslucentTransformGizmoMaterial" ) );
			TranslucentHoverMaterial = ObjectFinder.Object;
			check( TranslucentHoverMaterial != nullptr );
		}
		SelectionBarTranslucentMID = UMaterialInstanceDynamic::Create( TranslucentHoverMaterial, GetTransientPackage() );
		check( SelectionBarTranslucentMID != nullptr );
		SelectionBarMeshComponent->SetMaterial( 1, SelectionBarTranslucentMID );
	}

	{
		UStaticMesh* CloseButtonMesh = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/UI/CloseButtonMesh" ) );
			CloseButtonMesh = ObjectFinder.Object;
			check( CloseButtonMesh != nullptr );
		}

		CloseButtonMeshComponent= CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "CloseButtonMesh" ) );
		CloseButtonMeshComponent->SetStaticMesh( CloseButtonMesh );
		CloseButtonMeshComponent->SetMobility( EComponentMobility::Movable );
		CloseButtonMeshComponent->AttachTo( RootComponent );

		CloseButtonMeshComponent->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
		CloseButtonMeshComponent->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Block );
		CloseButtonMeshComponent->SetCollisionResponseToChannel( ECC_EditorGizmo, ECollisionResponse::ECR_Block );

		CloseButtonMeshComponent->bGenerateOverlapEvents = false;
		CloseButtonMeshComponent->SetCanEverAffectNavigation( false );
		CloseButtonMeshComponent->bCastDynamicShadow = false;
		CloseButtonMeshComponent->bCastStaticShadow = false;
		CloseButtonMeshComponent->bAffectDistanceFieldLighting = false;

		UMaterial* HoverMaterial = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UMaterial> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TransformGizmoMaterial" ) );
			HoverMaterial = ObjectFinder.Object;
			check( HoverMaterial != nullptr );
		}
		CloseButtonMID = UMaterialInstanceDynamic::Create( HoverMaterial, GetTransientPackage() );
		check( CloseButtonMID != nullptr );
		CloseButtonMeshComponent->SetMaterial( 0, CloseButtonMID );

		UMaterial* TranslucentHoverMaterial = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UMaterial> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TranslucentTransformGizmoMaterial" ) );
			TranslucentHoverMaterial = ObjectFinder.Object;
			check( TranslucentHoverMaterial != nullptr );
		}
		CloseButtonTranslucentMID = UMaterialInstanceDynamic::Create( TranslucentHoverMaterial, GetTransientPackage() );
		check( CloseButtonTranslucentMID != nullptr );
		CloseButtonMeshComponent->SetMaterial( 1, CloseButtonTranslucentMID );
	}
}


void AVREditorDockableWindow::SetupWidgetComponent()
{
	Super::SetupWidgetComponent();

	SetSelectionBarColor( GetOwner().GetOwner().GetColor( FVREditorMode::EColors::UISelectionBarColor ) );
	SetCloseButtonColor( GetOwner().GetOwner().GetColor( FVREditorMode::EColors::UICloseButtonColor ) );
}


void AVREditorDockableWindow::TickManually( float DeltaTime )
{
	Super::TickManually( DeltaTime );

	if( WidgetComponent->IsVisible() )
	{
		const float WorldScaleFactor = GetOwner().GetOwner().GetWorldScaleFactor();
		const FVector AnimatedScale = CalculateAnimatedScale();

		// Update the window border mesh
		{
			const float WindowMeshSize = 100.0f;	// Size of imported mesh, we need to inverse compensate for

			const FVector WindowMeshScale = FVector(
				VREd::DockWindowThickness->GetFloat(),
				GetSize().X / WindowMeshSize,
				GetSize().Y / WindowMeshSize ) * AnimatedScale * WorldScaleFactor;
			WindowMeshComponent->SetRelativeScale3D( WindowMeshScale );			
		}

		// Update the selection bar
		{
			// How big the selection bar should be
			const FVector SelectionBarSize( 1.0f, GetSize().X * 0.8f, 6.0f );	// @todo vreditor tweak

			const FVector SelectionBarScale = SelectionBarSize * AnimatedScale * WorldScaleFactor;
			SelectionBarMeshComponent->SetRelativeScale3D( SelectionBarScale );
			const FVector SelectionBarRelativeLocation = FVector(
				0.0f,
				0.0f,
				-( GetSize().Y * 0.5f + SelectionBarSize.Z * 0.5f + VREd::DockUISelectionBarVerticalOffset->GetFloat() ) ) * AnimatedScale * WorldScaleFactor;
			SelectionBarMeshComponent->SetRelativeLocation( SelectionBarRelativeLocation );
		}

		// Update the close button
		{
			// How big the close button should be
			const FVector CloseButtonSize( 1.0f, 3.0f, 3.0f );	// @todo vreditor tweak

			const FVector CloseButtonScale = CloseButtonSize * AnimatedScale * WorldScaleFactor;
			CloseButtonMeshComponent->SetRelativeScale3D( CloseButtonScale );
			const FVector CloseButtonRelativeLocation = FVector(
				0.0f,
				-( GetSize().X * 0.5f + CloseButtonSize.Y * 0.5f + VREd::DockUICloseButtonOffsetFromCorner->GetFloat() ),
				( GetSize().Y * 0.5f + CloseButtonSize.Z * 0.5f + VREd::DockUICloseButtonOffsetFromCorner->GetFloat() ) ) * AnimatedScale * WorldScaleFactor;
			CloseButtonMeshComponent->SetRelativeLocation( CloseButtonRelativeLocation );
		}
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
	if( SelectionBarMeshComponent == HitResult.GetComponent() )
	{
		SetSelectionBarColor( GetOwner().GetOwner().GetColor( FVREditorMode::EColors::HoverGizmoColor ) );
	}

	if( CloseButtonMeshComponent == HitResult.GetComponent() )
	{
		SetCloseButtonColor( GetOwner().GetOwner().GetColor( FVREditorMode::EColors::HoverGizmoColor ) );
	}
}

void AVREditorDockableWindow::OnLeaveHover()
{
	SetSelectionBarColor( GetOwner().GetOwner().GetColor( FVREditorMode::EColors::UISelectionBarColor ) );
	SetCloseButtonColor( GetOwner().GetOwner().GetColor( FVREditorMode::EColors::UICloseButtonColor ) );
}


void AVREditorDockableWindow::SetSelectionBarColor( const FLinearColor& LinearColor )
{
	static FName StaticColorParameterName( "Color" );
	SelectionBarMID->SetVectorParameterValue( StaticColorParameterName, LinearColor );
	SelectionBarTranslucentMID->SetVectorParameterValue( StaticColorParameterName, LinearColor );
}


void AVREditorDockableWindow::SetCloseButtonColor( const FLinearColor& LinearColor )
{
	static FName StaticColorParameterName( "Color" );
	CloseButtonMID->SetVectorParameterValue( StaticColorParameterName, LinearColor );
	CloseButtonTranslucentMID->SetVectorParameterValue( StaticColorParameterName, LinearColor );
}
