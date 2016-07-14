// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorFloatingUI.h"
#include "VREditorUISystem.h"
#include "VREditorBaseUserWidget.h"
#include "VREditorMode.h"
#include "WidgetComponent.h"
#include "VREditorWidgetComponent.h"
#include "SLevelViewport.h"	// For tab manager tricks


namespace VREd
{
	static FAutoConsoleVariable UIFadeSpeed( TEXT( "VREd.UIFadeSpeed" ), 6.0f, TEXT( "How fast UI should fade in and out" ) );
	static FAutoConsoleVariable UIBrightness( TEXT( "VREd.UIBrightness" ), 0.3f, TEXT( "How bright the UI should be" ) );
	static FAutoConsoleVariable UIOnHandRotationOffset( TEXT( "VREd.UIOnHandRotationOffset" ), 45.0f, TEXT( "Rotation offset for UI that's docked to your hand, to make it more comfortable to hold" ) );
}


AVREditorFloatingUI::AVREditorFloatingUI()
	: LocalRotation( FRotator( 90.0f, 180.0f, 0.0f ) ),
	  RelativeOffset( FVector::ZeroVector ),
	  SlateWidget( nullptr ),
	  UserWidget( nullptr ),
	  WidgetComponent( nullptr ),
	  Scale( 1.0f ),
	  Resolution( 0, 0 ),
	  Owner( nullptr ),
	  DockedTo( EDockedTo::Nothing ),
	  PreviousDockedTo( EDockedTo::Nothing ),
	  UserWidgetClass( nullptr ),
	  bShouldBeVisible(),
	  FadeAlpha( 1.0f ),
	  bCollisionOnShowUI( true ),
	  FadeDelay( 0.0f ),
	  InitialScale( 1.0f ),
	  bIsMoving( false ),
	  MoveToTransform( FTransform() ),
	  StartMoveToTransform( FTransform() ),
	  MoveToAlpha( 0.0f ),
	  MoveToTime( 0.0f ),
	  MoveToResultDock( EDockedTo::Nothing )
{
	const bool bTransient = true;
	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>( TEXT( "SceneComponent" ), bTransient );
	check( SceneComponent != nullptr );
	this->RootComponent = SceneComponent;

	WidgetComponent = CreateDefaultSubobject<UVREditorWidgetComponent>( TEXT( "WidgetComponent" ), bTransient );
	WidgetComponent->SetEditTimeUsable(true);
	WidgetComponent->SetupAttachment( SceneComponent );

	InitialScale = Scale;
}


void AVREditorFloatingUI::SetupWidgetComponent()
{
	WidgetComponent->SetTwoSided( false );	// No VR UI is two-sided

	if( SlateWidget.IsValid() )
	{
		// Slate UIs have bogus opacity in their texture's alpha, so ignore texture alpha for VR
		WidgetComponent->SetOpacityFromTexture( 0.0f );	// Slate UIs have bogus opacity in their texture's alpha, so ignore texture alpha for VR
		WidgetComponent->SetBackgroundColor( FLinearColor::Black );
		WidgetComponent->SetBlendMode( EWidgetBlendMode::Opaque );
	}
	else
	{
		WidgetComponent->SetOpacityFromTexture( 1.0f );
		WidgetComponent->SetBackgroundColor( FLinearColor::Transparent );
		WidgetComponent->SetBlendMode( EWidgetBlendMode::Masked );
	}

	// @todo vreditor: Ideally we use automatic mip map generation, otherwise the UI looks too crunchy at a distance.
	// However, I tried this and on D3D11 the mips are all black.
	WidgetComponent->SetDrawSize( FVector2D( Resolution.X, Resolution.Y ) );		// NOTE: Must be called before RegisterComponent() because collision data will be created during registration

	// NOTE: Must be called *after* RegisterComponent() because UWidgetComponent nulls out Widget if no WidgetClass is set (WidgetClass is protected and there is no accessor)
	if( SlateWidget.IsValid() )
	{
		WidgetComponent->SetSlateWidget( SlateWidget.ToSharedRef() );
	}
	else
	{
		// @todo vreditor unreal: Ideally we would do this in the constructor and not again after.  Due to an apparent bug in UMG, 
		// we need to re-create the widget in lock-step with the WidgetComponent, otherwise input doesn't function correctly on the 
		// widget after the widget component is destroyed and recreated with the same user widget.
		check( UserWidgetClass != nullptr );
		UserWidget = CreateWidget<UVREditorBaseUserWidget>( GetWorld(), UserWidgetClass );
		check( UserWidget != nullptr );
		UserWidget->SetOwner( this );

		WidgetComponent->SetWidget( UserWidget );
	}

	// @todo vreditor: Is this useful?
	//WidgetComponent->SetMaxInteractionDistance( 10000.0f );

	// Default to visible
	ShowUI( true );

	// Set initial opacity
	UpdateFadingState( 0.0f );

	// Set initial transform
	UpdateTransformIfDocked();
}

void AVREditorFloatingUI::SetSlateWidget( FVREditorUISystem& InitOwner, const TSharedRef<SWidget>& InitSlateWidget, const FIntPoint InitResolution, const float InitScale, const EDockedTo InitDockedTo )
{
	Owner = &InitOwner;

	SlateWidget = InitSlateWidget;

	Resolution = InitResolution;
	check( Resolution.X > 0 && Resolution.Y > 0 );

	Scale = InitScale;
	InitialScale = Scale;

	DockedTo = InitDockedTo;

	SetupWidgetComponent();
}


void AVREditorFloatingUI::SetUMGWidget( FVREditorUISystem& InitOwner, TSubclassOf<UVREditorBaseUserWidget> InitUserWidgetClass, const FIntPoint InitResolution, const float InitScale, const EDockedTo InitDockedTo )
{
	Owner = &InitOwner;

	check( InitUserWidgetClass != nullptr );
	UserWidgetClass = InitUserWidgetClass;

	Resolution = InitResolution;
	check( Resolution.X > 0 && Resolution.Y > 0 );

	Scale = InitScale;
	InitialScale = Scale;

	DockedTo = InitDockedTo;

	SetupWidgetComponent();
}


void AVREditorFloatingUI::Destroyed()
{
	if( WidgetComponent != nullptr )
	{
		// NOTE: We're nulling out widgets so that we don't have to wait for a GC to free up Slate resources (avoid shutdown crash)
		WidgetComponent->SetSlateWidget( nullptr );
		WidgetComponent->SetWidget( nullptr );
		WidgetComponent = nullptr;
	}

	this->SlateWidget = nullptr;

	// @todo vreditor unreal: UMG has a bug that prevents you from re-using the user widget for a new widget component
	// after a previous widget component that was using it was destroyed
	if( UserWidget != nullptr )
	{
		UserWidget->MarkPendingKill();
		UserWidget = nullptr;
	}

	Super::Destroyed();
}


void AVREditorFloatingUI::TickManually( float DeltaTime )
{
	Super::Tick( DeltaTime );

	// Update fading state
	UpdateFadingState( DeltaTime );

	if ( bIsMoving )
	{
		TickMoveTo( DeltaTime );
	}
	else
	{
		// Update transform
		UpdateTransformIfDocked();
	}
}


void AVREditorFloatingUI::UpdateFadingState( const float DeltaTime )
{
	if ( FadeDelay > 0.f )
	{
		FadeDelay -= DeltaTime;
	}
	else
	{
		if( bShouldBeVisible.GetValue() )
		{
			FadeAlpha += VREd::UIFadeSpeed->GetFloat() * DeltaTime;
		}
		else
		{
			FadeAlpha -= VREd::UIFadeSpeed->GetFloat() * DeltaTime;
		}
		FadeAlpha = FMath::Clamp( FadeAlpha, 0.0f, 1.0f );

		if( FadeAlpha > 0.0f + KINDA_SMALL_NUMBER )
		{
			// At least a little bit visible
			if( bHidden )
			{
				SetActorHiddenInGame( false );
				WidgetComponent->SetVisibility( true );
				FadeDelay = 0.0f;
			}
		}

		if( FadeAlpha >= 1.0f - KINDA_SMALL_NUMBER )
		{
			// Fully visible
		}
		else if( FadeAlpha <= 0.0f + KINDA_SMALL_NUMBER )
		{
			// Fully invisible
			if( !bHidden )
			{
				SetActorHiddenInGame( true );
				WidgetComponent->SetVisibility( false );
				FadeDelay = 0.0f;
			}
		}

 		// Set material color
		// NOTE: We intentionally make the UI quite dark here, so it doesn't bloom out in HDR
		const float UIBrightness = FadeAlpha * VREd::UIBrightness->GetFloat();
		WidgetComponent->SetTintColorAndOpacity( FLinearColor( UIBrightness, UIBrightness, UIBrightness ).CopyWithNewOpacity( FadeAlpha ) );
	}
}
	

void AVREditorFloatingUI::UpdateTransformIfDocked()
{
	if( DockedTo != EDockedTo::Nothing && DockedTo != EDockedTo::Dragging )
	{
		FTransform NewTransform;
		
		if( DockedTo == EDockedTo::LeftHand )
		{
			const bool bOnArm = false;
			NewTransform = MakeUITransformLockedToHand( VREditorConstants::LeftHandIndex, bOnArm );
		}
		else if( DockedTo == EDockedTo::RightHand )
		{
			const bool bOnArm = false;
			NewTransform = MakeUITransformLockedToHand( VREditorConstants::RightHandIndex, bOnArm );
		}
		else if( DockedTo == EDockedTo::LeftArm )
		{
			const bool bOnArm = true;
			NewTransform = MakeUITransformLockedToHand( VREditorConstants::LeftHandIndex, bOnArm );
		}
		else if( DockedTo == EDockedTo::RightArm )
		{
			const bool bOnArm = true;
			NewTransform = MakeUITransformLockedToHand( VREditorConstants::RightHandIndex, bOnArm );
		}
		else if( DockedTo == EDockedTo::Room)
		{
			NewTransform = MakeUITransformLockedToRoom();
		}
		else if( DockedTo == EDockedTo::Custom )
		{
			NewTransform = MakeCustomUITransform();
		}
		else
		{
			check( 0 );
		}

		SetTransform( NewTransform );
	}
}


FVector AVREditorFloatingUI::CalculateAnimatedScale() const
{
	const float AnimationOvershootAmount = 0.7f;	// @todo vreditor tweak
	const float EasedAlpha = FVREditorMode::OvershootEaseOut( FadeAlpha, AnimationOvershootAmount );

	// Animate vertically more than horizontally; just looks a little better
	const float ZScale = FMath::Max( 0.001f, EasedAlpha );
	const float YScale = FMath::Max( 0.001f, 0.7f + 0.3f * EasedAlpha );

	FVector AnimatedScale = FVector( 1.0f, YScale, ZScale );
	AnimatedScale.Y *= YScale;
	AnimatedScale.Z *= ZScale;

	return AnimatedScale;
}


void AVREditorFloatingUI::SetTransform( const FTransform& Transform )
{
	const FVector AnimatedScale = CalculateAnimatedScale();
	
	FTransform AnimatedTransform = Transform;
	AnimatedTransform.SetScale3D( AnimatedTransform.GetScale3D() * AnimatedScale );

	const float Aspect = (float)Resolution.X / (float)Resolution.Y;

	RootComponent->SetWorldLocation( AnimatedTransform.GetLocation() );
	RootComponent->SetWorldRotation( AnimatedTransform.GetRotation() );
	WidgetComponent->SetWorldScale3D( FVector( 1.0f / AnimatedTransform.GetScale3D().X, 1.0f / (float)Resolution.X, 1.0f / (float)Resolution.Y / Aspect ) * AnimatedTransform.GetScale3D() );
}


void AVREditorFloatingUI::SetRelativeOffset( const FVector& InRelativeOffset )
{
	RelativeOffset = InRelativeOffset;
}

void AVREditorFloatingUI::SetLocalRotation( const FRotator& InLocalRotation )
{
	LocalRotation = InLocalRotation;
}

void AVREditorFloatingUI::SetCollisionOnShow( const bool bInCollisionOnShow )
{
	bCollisionOnShowUI = bInCollisionOnShow;
}

float AVREditorFloatingUI::GetInitialScale() const
{
	return InitialScale;
}

FTransform AVREditorFloatingUI::MakeUITransformLockedToHand( const int32 HandIndex, const bool bOnArm )
{
	return MakeUITransformLockedToHand( HandIndex, bOnArm, RelativeOffset, LocalRotation );
}

FTransform AVREditorFloatingUI::MakeUITransformLockedToHand( const int32 HandIndex, const bool bOnArm, const FVector& InRelativeOffset, const FRotator& InLocalRotation )
{
	const float WorldScaleFactor = Owner->GetOwner().GetWorldScaleFactor();

	FTransform UIToHandTransform( InLocalRotation, InRelativeOffset * WorldScaleFactor );
	if (!bOnArm)
	{
		UIToHandTransform *= FTransform( FRotator( VREd::UIOnHandRotationOffset->GetFloat(), 0.0f, 0.0f ).Quaternion(), FVector::ZeroVector );
	}

	const FTransform HandToWorldTransform = Owner->GetOwner().GetVirtualHand( HandIndex ).Transform;

	FTransform UIToWorldTransform = UIToHandTransform * HandToWorldTransform;
	UIToWorldTransform.SetScale3D( FVector( Scale * WorldScaleFactor ) );

	return UIToWorldTransform;
}

void AVREditorFloatingUI::MoveTo( const FTransform& ResultTransform, const float TotalMoveToTime, const EDockedTo ResultDock )
{
	MoveToTime = TotalMoveToTime;
	bIsMoving = true;
	StartMoveToTransform = GetActorTransform();
	MoveToTransform = ResultTransform;
	MoveToAlpha = 0.0f;
	MoveToResultDock = ResultDock;
}

void AVREditorFloatingUI::StopMoveTo()
{
	bIsMoving = false;
	SetTransform( MoveToTransform );
	MoveToTransform = FTransform();
}

FTransform AVREditorFloatingUI::MakeUITransformLockedToRoom()
{
	const float WorldScaleFactor = Owner->GetOwner().GetWorldScaleFactor();

	const FTransform UIToRoomTransform( LocalRotation, RelativeOffset * WorldScaleFactor );

	const FTransform RoomToWorldTransform = Owner->GetOwner().GetRoomTransform();

	FTransform UIToWorldTransform = UIToRoomTransform * RoomToWorldTransform;
	UIToWorldTransform.SetScale3D( FVector( Scale * WorldScaleFactor ) );

	return UIToWorldTransform;
}

void AVREditorFloatingUI::TickMoveTo( const float DeltaTime )
{
	const float WorldScaleFactor = Owner->GetOwner().GetWorldScaleFactor();

	MoveToAlpha += DeltaTime;
	const float LerpTime = MoveToTime;
	if (MoveToAlpha > MoveToTime)
	{
		MoveToAlpha = MoveToTime;
		bIsMoving = false;
		SetDockedTo( MoveToResultDock );
	}

	const float CurrentALpha = MoveToAlpha / LerpTime;
	const FVector NewLocation = FMath::Lerp( StartMoveToTransform.GetLocation(), MoveToTransform.GetLocation(), CurrentALpha );
	const FQuat NewRotation = FMath::Lerp( StartMoveToTransform.GetRotation(), MoveToTransform.GetRotation(), CurrentALpha );
	FTransform NewTransform( NewRotation, NewLocation, FVector( Scale * WorldScaleFactor ) );

	SetTransform( NewTransform );
}

void AVREditorFloatingUI::SetDockedTo( const EDockedTo NewDockedTo )
{
	PreviousDockedTo = DockedTo;
	DockedTo = NewDockedTo;
	UpdateTransformIfDocked();
}


void AVREditorFloatingUI::ShowUI( const bool bShow, const bool bAllowFading, const float InitFadeDelay )
{
	if( !bShouldBeVisible.IsSet() || bShow != bShouldBeVisible.GetValue() )
	{
		bShouldBeVisible = bShow;

		if( bCollisionOnShowUI )
		{
			SetActorEnableCollision( bShow );
		}

		if( !bAllowFading )
		{
			SetActorHiddenInGame( !bShow );
			WidgetComponent->SetVisibility( bShow );
			FadeAlpha = bShow ? 1.0f : 0.0f;
		}

		FadeDelay = InitFadeDelay;
	}
}


FVector2D AVREditorFloatingUI::GetSize() const
{
	const float Aspect = (float)Resolution.X / (float)Resolution.Y;
	return FVector2D( Scale, Scale / Aspect );
}

float AVREditorFloatingUI::GetScale() const
{
	return Scale;
}

void AVREditorFloatingUI::SetScale( const float NewSize )
{
	Scale = NewSize;

	const float WorldScaleFactor = Owner->GetOwner().GetWorldScaleFactor();
	const FVector NewScale( Scale * WorldScaleFactor );
	const float Aspect = (float)Resolution.X / (float)Resolution.Y;
	WidgetComponent->SetWorldScale3D( FVector( 1.0f / NewScale.X, 1.0f / (float)Resolution.X, 1.0f / (float)Resolution.Y / Aspect ) * NewScale );
}