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
}


AVREditorFloatingUI::AVREditorFloatingUI()
	: Scale( 1.0f ),
	  DockedTo( EDockedTo::Nothing ),
	  PreviousDockedTo( EDockedTo::Nothing ),
	  SlateWidget( nullptr ),
	  UserWidgetClass( nullptr ),
	  UserWidget( nullptr ),
	  Resolution( 0, 0 ),
	  WidgetComponent( nullptr ),
	  bShouldBeVisible(),
	  FadeAlpha( 1.0f ),
	  bRotateToHead( false ),
	  LocalRotation( FRotator( 90.0f, 180.0f, 0.0f ) ),
	  bCollisionOnShowUI( true ),
	  FadeDelay( 0.0f )
{
	const bool bTransient = true;
	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>( TEXT( "SceneComponent" ), bTransient );
	check( SceneComponent != nullptr );
	this->RootComponent = SceneComponent;

	WidgetComponent = CreateDefaultSubobject<UVREditorWidgetComponent>( TEXT( "WidgetComponent" ), bTransient );
	WidgetComponent->SetEditTimeUsable(true);
	WidgetComponent->AttachTo( SceneComponent );

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

	// Update transform
	UpdateTransformIfDocked();
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
	if( DockedTo != EDockedTo::Nothing )
	{
		FTransform NewTransform;
		
		if( DockedTo == EDockedTo::LeftHand )
		{
			const bool bOnArm = true;
			NewTransform = MakeUITransformLockedToHand( VREditorConstants::LeftHandIndex, bOnArm );
		}
		else if( DockedTo == EDockedTo::RightHand )
		{
			const bool bOnArm = true;
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
		else if (DockedTo == EDockedTo::Head)
		{
			NewTransform = MakeUITransformLockedToHead();
		}
		else if( DockedTo == EDockedTo::Custom )
		{
			NewTransform = MakeCustomUITransform();
		}
		else
		{
			check( 0 );
		}

		if( bRotateToHead )
		{
			RotateToHead( NewTransform );
		}

		SetTransform( NewTransform );
	}
}

// @todo vreditor: Move this to FMath so we can use it everywhere
// NOTE: OvershootAmount is usually between 0.5 and 2.0, but can go lower and higher for extreme overshots
template<class T>
static T OvershootEaseOut( T Alpha, const float OvershootAmount = 1.0f )
{
	Alpha--;
	return 1.0f - ( ( Alpha * ( ( OvershootAmount + 1 ) * Alpha + OvershootAmount ) + 1 ) - 1.0f );
}


FVector AVREditorFloatingUI::CalculateAnimatedScale() const
{
	const float UIAnimationOvershootAmount = 0.7f;	// @todo vreditor tweak
	const float EasedAlpha = OvershootEaseOut( FadeAlpha, UIAnimationOvershootAmount );

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


void AVREditorFloatingUI::SetRotateToHead( const bool bInitRotateToHead )
{
	bRotateToHead = bInitRotateToHead;
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
	const float WorldScaleFactor = Owner->GetOwner().GetWorldScaleFactor();

	const FTransform& HandTransform = Owner->GetOwner().GetVirtualHand( HandIndex ).Transform;

	FVector UILocation;
	FRotator UIRotation;
	FTransform OnArmTransform;
	
	FTransform LocalTransform( LocalRotation.Quaternion(), FVector::ZeroVector );

	if( bOnArm )
	{
		OnArmTransform = LocalTransform * HandTransform;

		const FVector HandForward = HandTransform.GetUnitAxis( EAxis::X );	
		const FVector HandRight = HandTransform.GetUnitAxis( EAxis::Y );
		const FVector HandUp = HandTransform.GetUnitAxis( EAxis::Z );

		UILocation =
			OnArmTransform.GetLocation() +
			HandForward * RelativeOffset.X * WorldScaleFactor +
			HandRight * RelativeOffset.Y * WorldScaleFactor +
			HandUp * RelativeOffset.Z * WorldScaleFactor;
	}

	if ( !bRotateToHead )
	{
		UIRotation = ( OnArmTransform.GetRotation() ).Rotator();
	}

	const FTransform UITransform =
		FTransform(
			UIRotation,
			UILocation,
			FVector( Scale * WorldScaleFactor ) );

	return UITransform;
}

void AVREditorFloatingUI::RotateToHead( FTransform& Transform )
{
	FRotator UIRotation;
	const FVector HeadToUIDirection = ( Owner->GetOwner().GetHeadTransform().GetLocation() - Transform.GetLocation() ).GetSafeNormal();
	UIRotation = HeadToUIDirection.ToOrientationRotator();
	UIRotation.Roll = 0.0f;

	Transform.SetRotation( FQuat( UIRotation ) );
}

FTransform AVREditorFloatingUI::MakeUITransformLockedToRoom()
{
	FVector UILocation;
	const float WorldScaleFactor = Owner->GetOwner().GetWorldScaleFactor();
	const FTransform RoomTransform = Owner->GetOwner().GetRoomTransform();
	
	const FVector RoomForward = RoomTransform.GetUnitAxis( EAxis::X );
	const FVector RoomRight = RoomTransform.GetUnitAxis( EAxis::Y );
	const FVector RoomUp = RoomTransform.GetUnitAxis( EAxis::Z );

	UILocation =
		RoomTransform.GetLocation() +
		RoomForward * RelativeOffset.X * WorldScaleFactor +
		RoomRight * RelativeOffset.Y * WorldScaleFactor +
		RoomUp * RelativeOffset.Z * WorldScaleFactor;

	return FTransform(
		LocalRotation + RoomTransform.GetRotation().Rotator(),
		UILocation,
		FVector( Scale * WorldScaleFactor ) );
}

FTransform AVREditorFloatingUI::MakeUITransformLockedToHead()
{
	FTransform Result;
	const float WorldScaleFactor = Owner->GetOwner().GetWorldScaleFactor();

	FVector HeadWorldLocation = Owner->GetOwner().GetHeadTransform().GetLocation();
	FVector RoomWorldLocation = Owner->GetOwner().GetRoomTransform().GetLocation();
	FVector NewLocation = FVector( HeadWorldLocation.X, HeadWorldLocation.Y, RoomWorldLocation.Z );
	
	Result.SetLocation( NewLocation + ( RelativeOffset * WorldScaleFactor ) );
	Result.SetScale3D( FVector( Scale * WorldScaleFactor ) );
	
	return Result;
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

		const TSharedRef<SViewport>& ViewportWidget = Owner->GetOwner().GetLevelViewportPossessedForVR().GetViewportWidget().Pin().ToSharedRef();
		if ( bShow )
		{
			WidgetComponent->RegisterHitTesterWithViewport(ViewportWidget);
		}
		else
		{
			WidgetComponent->UnregisterHitTesterWithViewport(ViewportWidget);
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