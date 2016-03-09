// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorUISystem.h"
#include "VREditorMode.h"
#include "VREditorFloatingUI.h"
#include "VREditorDockableWindow.h"
#include "VREditorQuickMenu.h"
#include "VREditorRadialMenu.h"
#include "VREditorRadialMenuItem.h"
#include "VirtualHand.h"
#include "IHeadMountedDisplay.h"

// UI
#include "WidgetComponent.h"
#include "VREditorWidgetComponent.h"

// Content Browser
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"

// World Outliner
#include "Editor/SceneOutliner/Public/SceneOutliner.h"

// Actor Details, Modes
#include "LevelEditor.h"

#include "SLevelViewport.h"
#include "SScaleBox.h"
#include "SDPIScaler.h"
#include "SWidget.h"

namespace VREd
{
	static FAutoConsoleVariable ContentBrowserUIResolutionX( TEXT( "VREd.ContentBrowserUIResolutionX" ), 1920, TEXT( "Horizontal resolution to use for content browser UI render targets" ) );
	static FAutoConsoleVariable ContentBrowserUIResolutionY( TEXT( "VREd.ContentBrowserUIResolutionY" ), 1200, TEXT( "Vertical resolution to use for content browser UI render targets" ) );
	static FAutoConsoleVariable DefaultEditorUIResolutionX( TEXT( "VREd.DefaultEditorUIResolutionX" ), 1024, TEXT( "Horizontal resolution to use for VR editor UI render targets" ) );
	static FAutoConsoleVariable DefaultEditorUIResolutionY( TEXT( "VREd.DefaultEditorUIResolutionY" ), 1024, TEXT( "Vertical resolution to use for VR editor UI render targets" ) );
	static FAutoConsoleVariable QuickMenuUIResolutionX( TEXT( "VREd.QuickMenuUIResolutionX" ), 1024, TEXT( "Horizontal resolution to use for Quick Menu VR UI render targets" ) );
	static FAutoConsoleVariable QuickMenuUIResolutionY( TEXT( "VREd.QuickMenuUIResolutionY" ), 900, TEXT( "Vertical resolution to use for Quick Menu VR UI render targets" ) );
	static FAutoConsoleVariable ContentBrowserUISize( TEXT( "VREd.ContentBrowserUISize" ), 60.0f, TEXT( "How big content browser UIs should be" ) );
	static FAutoConsoleVariable EditorUISize( TEXT( "VREd.EditorUISize" ), 50.0f, TEXT( "How big editor UIs should be" ) );
	static FAutoConsoleVariable ContentBrowserUIScale( TEXT( "VREd.ContentBrowserUIScale" ), 2.0f, TEXT( "How much to scale up (or down) the content browser for VR" ) );
	static FAutoConsoleVariable EditorUIScale( TEXT( "VREd.EditorUIScale" ), 2.0f, TEXT( "How much to scale up (or down) editor UIs for VR" ) );
	static FAutoConsoleVariable EditorUIOffsetFromHand( TEXT( "VREd.EditorUIOffsetFromHand" ), 12.0f, TEXT( "How far to offset editor UIs from the origin of the hand mesh" ) );
	static FAutoConsoleVariable AssetEditorUIResolutionX( TEXT( "VREd.AssetEditorUIResolutionX" ), 1920, TEXT( "Horizontal resolution to use for VR editor asset editor UI render targets" ) );
	static FAutoConsoleVariable AssetEditorUIResolutionY( TEXT( "VREd.AssetEditorUIResolutionY" ), 1080, TEXT( "Vertical resolution to use for VR editor asset editor UI render targets" ) );
	static FAutoConsoleVariable RadialMenuFadeDelay( TEXT( "VREd.RadialMenuFadeDelay" ), 0.2f, TEXT( "The delay for the radial menu after selecting a button" ) );
	static FAutoConsoleVariable UIAbsoluteScrollSpeed( TEXT( "VREd.UIAbsoluteScrollSpeed" ), 8.0f, TEXT( "How fast the UI scrolls when dragging the touchpad" ) );
	static FAutoConsoleVariable UIRelativeScrollSpeed( TEXT( "VREd.UIRelativeScrollSpeed" ), 0.75f, TEXT( "How fast the UI scrolls when holding an analog stick" ) );
	static FAutoConsoleVariable MinUIScrollDeltaForInertia( TEXT( "VREd.MinUIScrollDeltaForInertia" ), 0.25f, TEXT( "Minimum amount of touch pad input before inertial UI scrolling kicks in" ) );
	static FAutoConsoleVariable MinDockDragDistance( TEXT( "VREd.MinDockDragDistance" ), 10.0f, TEXT( "Minimum amount of distance needed to drag dock from hand" ) );
	static FAutoConsoleVariable DoubleClickTime( TEXT( "VREd.DoubleClickTime" ), 0.25f, TEXT( "Minimum duration between clicks for a double click event to fire" ) );
	static FAutoConsoleVariable UIPressHapticFeedbackStrength( TEXT( "VREd.UIPressHapticFeedbackStrength" ), 0.4f, TEXT( "Strenth of haptic when clicking on the UI" ) );
	static FAutoConsoleVariable UIAssetEditorSummonedOnHandHapticFeedbackStrength( TEXT( "VREd.UIAssetEditorSummonedOnHandHapticFeedbackStrength" ), 1.0f, TEXT( "Strenth of haptic to play to remind a user which hand an asset editor was spawned on" ) );
	static FAutoConsoleVariable MaxDockWindowSize( TEXT( "VREd.MaxDockWindowSize" ), 250, TEXT( "Maximum size for dockable windows" ) );
	static FAutoConsoleVariable MinDockWindowSize( TEXT( "VREd.MinDockWindowSize" ), 40, TEXT( "Minimum size for dockable windows" ) );

	// Tutorial UI commands
	static FAutoConsoleVariable TutorialUIResolutionX( TEXT( "VREd.TutorialUI.Resolution.X" ), 1920, TEXT( "The X resolution for the tutorial UI panel" ) );
	static FAutoConsoleVariable TutorialUIResolutionY( TEXT( "VREd.TutorialUI.Resolution.Y" ), 1080, TEXT( "The Y resolution for the tutorial UI panel" ) );
	static FAutoConsoleVariable TutorialUISize( TEXT( "VREd.TutorialUI.Size" ), 200, TEXT( "The room space size for the tutorial UI panel" ) );
	static FAutoConsoleVariable TutorialUIYaw( TEXT( "VREd.TutorialUI.Yaw" ), 270, TEXT( "The yaw rotation for the tutorial UI panel" ) );
	static FAutoConsoleVariable TutorialUIPitch( TEXT( "VREd.TutorialUI.Pitch" ), 45, TEXT( "The pitch rotation for the tutorial UI panel" ) );
	static FAutoConsoleVariable TutorialUILocationX( TEXT( "VREd.TutorialUI.Location.X" ), 0, TEXT( "The X location for the tutorial UI panel" ) );
	static FAutoConsoleVariable TutorialUILocationY( TEXT( "VREd.TutorialUI.Location.Y" ), 200, TEXT( "The Y location for the tutorial UI panel" ) );
	static FAutoConsoleVariable TutorialUILocationZ( TEXT( "VREd.TutorialUI.Location.Z" ), 40, TEXT( "The Z location for the tutorial UI panel" ) );
}


FVREditorUISystem::FVREditorUISystem( FVREditorMode& InitOwner )
	: Owner( InitOwner ),
	  FloatingUIs(),
	  QuickMenuUI( nullptr ),
	  QuickRadialMenu( nullptr ),
	  QuickMenuWidgetClass( nullptr ),
	  QuickRadialWidgetClass( nullptr ),
	  DraggingUI( nullptr ),
	  DraggingUIHandIndex( INDEX_NONE ),
	  DraggingUIOffsetTransform( FTransform::Identity ),
	  bPanelVisibilityToggle( false ),
	  RadialMenuHideDelayTime( 0.0f ),
	  bRefocusViewport(false)
{
	// Register to find out about VR events
	Owner.OnVRAction().AddRaw( this, &FVREditorUISystem::OnVRAction );
	Owner.OnVRHoverUpdate().AddRaw( this, &FVREditorUISystem::OnVRHoverUpdate );

	EditorUIPanels.SetNumZeroed( (int32)EEditorUIPanel::TotalCount );

	QuickMenuWidgetClass = LoadClass<UVREditorQuickMenu>( nullptr, TEXT( "/Engine/VREditor/UI/VRQuickMenu.VRQuickMenu_C" ) );
	check( QuickMenuWidgetClass != nullptr );

	QuickRadialWidgetClass = LoadClass<UVREditorRadialMenu>(nullptr, TEXT("/Engine/VREditor/UI/VRRadialQuickMenu.VRRadialQuickMenu_C"));
	check(QuickRadialWidgetClass != nullptr);

	TutorialWidgetClass = LoadClass<UVREditorBaseUserWidget>( nullptr, TEXT( "/Engine/VREditor/Tutorial/UI_VR_Tutorial_00.UI_VR_Tutorial_00_C" ) );
	check( TutorialWidgetClass != nullptr );

	// Load sounds
	StartDragUISound = LoadObject<USoundCue>( nullptr, TEXT( "/Engine/VREditor/Sounds/VR_negative_Cue" ) );
	check( StartDragUISound != nullptr );

	StopDragUISound = LoadObject<USoundCue>( nullptr, TEXT( "/Engine/VREditor/Sounds/VR_negative_Cue" ) );
	check( StopDragUISound != nullptr );

	HideUISound = LoadObject<USoundCue>( nullptr, TEXT( "/Engine/VREditor/Sounds/VR_close_Cue" ) );
	check( HideUISound != nullptr );

	ShowUISound = LoadObject<USoundCue>( nullptr, TEXT( "/Engine/VREditor/Sounds/VR_open_Cue" ) );
	check( ShowUISound != nullptr );

	// Create all of our UI panels
	CreateUIs();
}



FVREditorUISystem::~FVREditorUISystem()
{
	Owner.OnVRAction().RemoveAll( this );
	Owner.OnVRHoverUpdate().RemoveAll( this );

	CleanUpActorsBeforeMapChangeOrSimulate();

	QuickMenuWidgetClass = nullptr;
	QuickRadialWidgetClass = nullptr;
}


void FVREditorUISystem::AddReferencedObjects( FReferenceCollector& Collector )
{
	for( AVREditorFloatingUI* FloatingUIPtr : FloatingUIs )
	{
		Collector.AddReferencedObject( FloatingUIPtr );
	}
	Collector.AddReferencedObject( QuickMenuWidgetClass );
	Collector.AddReferencedObject( QuickRadialWidgetClass );
	Collector.AddReferencedObject( TutorialWidgetClass );
	Collector.AddReferencedObject( StartDragUISound );
	Collector.AddReferencedObject( StopDragUISound );
	Collector.AddReferencedObject( HideUISound );
	Collector.AddReferencedObject( ShowUISound );
}


void FVREditorUISystem::OnVRAction( FEditorViewportClient& ViewportClient, const FVRAction VRAction, const EInputEvent Event, bool& bIsInputCaptured, bool& bWasHandled )
{
	if( !bWasHandled )
	{

		if( VRAction.ActionType == EVRActionType::ConfirmRadialSelection )
		{
			FVirtualHand& Hand = Owner.GetVirtualHand( VRAction.HandIndex );
			if( Event == IE_Pressed )
			{
				UVREditorRadialMenu* RadialMenu = QuickRadialMenu->GetUserWidget<UVREditorRadialMenu>();
				if( IsShowingRadialMenu( VRAction.HandIndex ) )
				{
					RadialMenu->Update( Owner.GetVirtualHand( VRAction.HandIndex ) );
				}
				
				RadialMenu->SelectCurrentItem( Hand );
			}

			bWasHandled = true;
		}
		else if( VRAction.ActionType == EVRActionType::SelectAndMove_LightlyPressed )
		{
			FVirtualHand& Hand = Owner.GetVirtualHand( VRAction.HandIndex );
			FVector LaserPointerStart, LaserPointerEnd;
			if( Owner.GetLaserPointer( VRAction.HandIndex, LaserPointerStart, LaserPointerEnd ) )
			{
				FHitResult HitResult = Owner.GetHitResultFromLaserPointer( VRAction.HandIndex );
				if( HitResult.Actor.IsValid()  )
				{
					// Only allow clicks to our own widget components
					UWidgetComponent* WidgetComponent = Cast<UWidgetComponent>( HitResult.GetComponent() );
					if( WidgetComponent != nullptr && IsWidgetAnEditorUIWidget( WidgetComponent ) )
					{
						// Always mark the event as handled so that the editor doesn't try to select the widget component
						bWasHandled = true;

						if( Event != IE_Repeat )
						{
							// If the Modifier button is held down, treat this like a right click instead of a left click
							const bool bIsRightClicking =
								( Event == IE_Pressed && Hand.bIsModifierPressed ) ||
								( Event == IE_Released && Hand.bIsRightClickingOnUI );

							UWidgetComponent* WidgetComponent = Cast<UWidgetComponent>( HitResult.GetComponent() );
							if( WidgetComponent )
							{
								FVector2D LastLocalHitLocation = WidgetComponent->GetLastLocalHitLocation();

								FVector2D LocalHitLocation;
								WidgetComponent->GetLocalHitLocation( HitResult.ImpactPoint, LocalHitLocation );

								// If we weren't already hovering over this widget, then we'll reset the last hit location
								if( WidgetComponent != Hand.HoveringOverWidgetComponent )
								{
									LastLocalHitLocation = LocalHitLocation;

									if ( UVREditorWidgetComponent* VRWidgetComponent = Cast<UVREditorWidgetComponent>(Hand.HoveringOverWidgetComponent) )
									{
										VRWidgetComponent->SetIsHovering(false);
									}
								}

								FWidgetPath WidgetPathUnderFinger = FWidgetPath( WidgetComponent->GetHitWidgetPath( HitResult.ImpactPoint, /*bIgnoreEnabledStatus*/ false ) );
								if( WidgetPathUnderFinger.IsValid() )
								{
									TSet<FKey> PressedButtons;
									if( Event == IE_Pressed )
									{
										PressedButtons.Add( bIsRightClicking ? EKeys::RightMouseButton : EKeys::LeftMouseButton );
									}
									FPointerEvent PointerEvent(
										1 + VRAction.HandIndex,
										LocalHitLocation,
										LastLocalHitLocation,
										PressedButtons,
										bIsRightClicking ? EKeys::RightMouseButton : EKeys::LeftMouseButton,
										0.0f,	// Wheel delta
										FModifierKeysState() );

									Hand.HoveringOverWidgetComponent = WidgetComponent;

									if ( UVREditorWidgetComponent* VRWidgetComponent = Cast<UVREditorWidgetComponent>(WidgetComponent) )
									{
										VRWidgetComponent->SetIsHovering(true);
									}

									FReply Reply = FReply::Unhandled();
									if( Event == IE_Pressed )
									{
										Reply = FSlateApplication::Get().RoutePointerDownEvent( WidgetPathUnderFinger, PointerEvent );
										Hand.bIsClickingOnUI = true;
										Hand.bIsRightClickingOnUI = bIsRightClicking;
										bIsInputCaptured = true;

										// Play a haptic effect on press
										const float Strength = VREd::UIPressHapticFeedbackStrength->GetFloat();
										Owner.PlayHapticEffect(
											VRAction.HandIndex == VREditorConstants::LeftHandIndex ? Strength : 0.0f,
											VRAction.HandIndex == VREditorConstants::RightHandIndex ? Strength : 0.0f );
									}
									else if( Event == IE_Released )
									{
										Reply = FSlateApplication::Get().RoutePointerUpEvent( WidgetPathUnderFinger, PointerEvent );

										const double CurrentTime = FPlatformTime::Seconds();
										if( CurrentTime - Hand.LastClickReleaseTime <= VREd::DoubleClickTime->GetFloat() )
										{
											// Trigger a double click event!
											Reply = FSlateApplication::Get().RoutePointerDoubleClickEvent( WidgetPathUnderFinger, PointerEvent );
										}

										Hand.LastClickReleaseTime = CurrentTime;
									}
								}
							}
						}
					}
				}
			}

			if( Event == IE_Released )
			{
				bool bWasRightClicking = false;
				if( Hand.bIsClickingOnUI )
				{
					if( Hand.bIsRightClickingOnUI )
					{
						bWasRightClicking = true;
					}
					Hand.bIsClickingOnUI = false;
					Hand.bIsRightClickingOnUI = false;
					bIsInputCaptured = false;
				}

				if ( !bWasHandled )
				{
					TSet<FKey> PressedButtons;
					FPointerEvent PointerEvent(
						1 + VRAction.HandIndex,
						FVector2D::ZeroVector,
						FVector2D::ZeroVector,
						PressedButtons,
						bWasRightClicking ? EKeys::RightMouseButton : EKeys::LeftMouseButton,
						0.0f,	// Wheel delta
						FModifierKeysState() );

					FWidgetPath EmptyWidgetPath;

					Hand.bIsClickingOnUI = false;
					Hand.bIsRightClickingOnUI = false;
					FReply Reply = FSlateApplication::Get().RoutePointerUpEvent( EmptyWidgetPath, PointerEvent );
				}
			}
		}
	}

	// Stop dragging the dock if we are dragging a dock
	if (VRAction.ActionType == EVRActionType::SelectAndMove && Event == IE_Released)
	{
		// Put the Dock back on the hand it came from or leave it where it is in the room
		if (DraggingUI != nullptr && DraggingUIHandIndex == VRAction.HandIndex)
		{
			FVirtualHand& Hand = Owner.GetVirtualHand( VRAction.HandIndex );
			FVirtualHand& OtherHand = Owner.GetOtherHand( VRAction.HandIndex );
			const float Distance = FVector::Dist( Hand.HoverLocation, OtherHand.Transform.GetLocation() ) / Owner.GetWorldScaleFactor();
			bool bOnHand = true;
			if (Distance > VREd::MinDockDragDistance->GetFloat())
			{
				DraggingUI->SetDockedTo( AVREditorFloatingUI::EDockedTo::Room );
				bOnHand = false;
			}

			const int32 OtherHandIndex = Owner.GetOtherHandIndex( DraggingUIHandIndex );
			ShowEditorUIPanel( DraggingUI, OtherHandIndex, true, bOnHand, true );
			
			StopDraggingDockUI();
		}
	}
}


void FVREditorUISystem::OnVRHoverUpdate( FEditorViewportClient& ViewportClient, const int32 HandIndex, FVector& HoverImpactPoint, bool& bIsHoveringOverUI, bool& bWasHandled )
{
	FVirtualHand& Hand = Owner.GetVirtualHand( HandIndex );

	if( !bWasHandled && Hand.DraggingMode != EVREditorDraggingMode::DockableWindow )
	{
		FVector LaserPointerStart, LaserPointerEnd;
		if( Owner.GetLaserPointer( HandIndex, LaserPointerStart, LaserPointerEnd ) )
		{
			FHitResult HitResult = Owner.GetHitResultFromLaserPointer( HandIndex );
			if( HitResult.Actor.IsValid() )
			{
				// Only allow clicks to our own widget components
				UWidgetComponent* WidgetComponent = Cast<UWidgetComponent>( HitResult.GetComponent() );
				if( WidgetComponent != nullptr && IsWidgetAnEditorUIWidget( WidgetComponent ) )
				{
					FVector2D LastLocalHitLocation = WidgetComponent->GetLastLocalHitLocation();

					FVector2D LocalHitLocation;
					WidgetComponent->GetLocalHitLocation( HitResult.ImpactPoint, LocalHitLocation );

					// If we weren't already hovering over this widget, then we'll reset the last hit location
					if( WidgetComponent != Hand.HoveringOverWidgetComponent )
					{
						LastLocalHitLocation = LocalHitLocation;

						if ( UVREditorWidgetComponent* VRWidgetComponent = Cast<UVREditorWidgetComponent>(Hand.HoveringOverWidgetComponent) )
						{
							VRWidgetComponent->SetIsHovering(false);
						}
					}

					// @todo vreditor UI: There is a CursorRadius optional arg that we migth want to make use of wherever we call GetHitWidgetPath()!
					FWidgetPath WidgetPathUnderFinger = FWidgetPath( WidgetComponent->GetHitWidgetPath( HitResult.ImpactPoint, /*bIgnoreEnabledStatus*/ false ) );
					if ( WidgetPathUnderFinger.IsValid() )
					{
						FVirtualHand& Hand = Owner.GetVirtualHand(HandIndex);
						Hand.bIsHovering = true;
						Hand.HoverLocation = HitResult.ImpactPoint;

						TSet<FKey> PressedButtons;
						FPointerEvent PointerEvent(
							1 + HandIndex,
							LocalHitLocation,
							LastLocalHitLocation,
							LocalHitLocation - LastLocalHitLocation,
							PressedButtons,
							FModifierKeysState());

						FSlateApplication::Get().RoutePointerMoveEvent(WidgetPathUnderFinger, PointerEvent, false);

						bWasHandled = true;
						HoverImpactPoint = HitResult.ImpactPoint;
						Hand.HoveringOverWidgetComponent = WidgetComponent;
						bIsHoveringOverUI = true;

						if ( UVREditorWidgetComponent* VRWidgetComponent = Cast<UVREditorWidgetComponent>(WidgetComponent) )
						{
							VRWidgetComponent->SetIsHovering(true);
						}

						// Route the mouse scrolling
						if ( Hand.bIsTrackpadPositionValid[1] )
						{
							const bool bIsAbsolute = ( Owner.GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR );

							float ScrollDelta = 0.0f;
							if( Hand.bIsTouchingTrackpad || !bIsAbsolute )
							{
								if( bIsAbsolute )
								{
									const float ScrollSpeed = VREd::UIAbsoluteScrollSpeed->GetFloat();
									ScrollDelta = ( Hand.TrackpadPosition.Y - Hand.LastTrackpadPosition.Y ) * ScrollSpeed;
								}
								else
								{
									const float ScrollSpeed = VREd::UIRelativeScrollSpeed->GetFloat();
									ScrollDelta = Hand.TrackpadPosition.Y * ScrollSpeed;
								}
							}

							// If using a trackpad (Vive), invert scroll direction so that it feels more like scrolling on a mobile device
							if( Owner.GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR )
							{
								ScrollDelta *= -1.0f;
							}

							if( !FMath::IsNearlyZero( ScrollDelta ) )
							{
								FPointerEvent MouseWheelEvent(
									1 + HandIndex,
									LocalHitLocation,
									LastLocalHitLocation,
									PressedButtons,
									EKeys::MouseWheelAxis,
									ScrollDelta,
									FModifierKeysState() );

								FSlateApplication::Get().RouteMouseWheelOrGestureEvent(WidgetPathUnderFinger, MouseWheelEvent, nullptr);

								Hand.UIScrollVelocity = 0.0f;

								// Don't apply inertia unless the user dragged a decent amount this frame
								if( bIsAbsolute && FMath::Abs( ScrollDelta ) >= VREd::MinUIScrollDeltaForInertia->GetFloat() )
								{
									// Don't apply inertia if our data is sort of old
									const FTimespan CurrentTime = FTimespan::FromSeconds( FPlatformTime::Seconds() );
									if( CurrentTime - Hand.LastTrackpadPositionUpdateTime < FTimespan::FromSeconds( 1.0f / 30.0f ) )
									{
										//GWarn->Logf( TEXT( "INPUT: UIScrollVelocity=%0.2f" ), ScrollDelta );
										Hand.UIScrollVelocity = ScrollDelta;
									}
								}
							}
						}
						else
						{
							if( !FMath::IsNearlyZero( Hand.UIScrollVelocity ) )
							{
								// Apply UI scrolling inertia
								const float ScrollDelta = Hand.UIScrollVelocity;
								{
									FPointerEvent MouseWheelEvent(
										1 + HandIndex,
										LocalHitLocation,
										LastLocalHitLocation,
										PressedButtons,
										EKeys::MouseWheelAxis,
										ScrollDelta,
										FModifierKeysState() );

									FSlateApplication::Get().RouteMouseWheelOrGestureEvent( WidgetPathUnderFinger, MouseWheelEvent, nullptr );
								}

								// Apply damping
								FVector ScrollVelocityVector( Hand.UIScrollVelocity, 0.0f, 0.0f );
								const bool bVelocitySensitive = false;
								Owner.ApplyVelocityDamping( ScrollVelocityVector, bVelocitySensitive );
								Hand.UIScrollVelocity = ScrollVelocityVector.X;

								//GWarn->Logf( TEXT( "INERTIA: UIScrollVelocity==%0.2f  (DAMPING: UIScrollVelocity==%0.2f)" ), ScrollDelta, Hand.UIScrollVelocity );
							}
							else
							{
								Hand.UIScrollVelocity = 0.0f;
							}
						}
					}
				}
			}
		}
	}

	// If nothing was hovered, make sure we tell Slate about that
	if( !bWasHandled && Hand.HoveringOverWidgetComponent != nullptr )
	{
		if ( UVREditorWidgetComponent* VRWidgetComponent = Cast<UVREditorWidgetComponent>(Hand.HoveringOverWidgetComponent) )
		{
			VRWidgetComponent->SetIsHovering(false);
		}

		const FVector2D LastLocalHitLocation = Hand.HoveringOverWidgetComponent->GetLastLocalHitLocation();
		Hand.HoveringOverWidgetComponent = nullptr;

		TSet<FKey> PressedButtons;
		FPointerEvent PointerEvent(
			1 + HandIndex,
			LastLocalHitLocation,
			LastLocalHitLocation,
			FVector2D::ZeroVector,
			PressedButtons,
			FModifierKeysState() );
		FSlateApplication::Get().RoutePointerMoveEvent( FWidgetPath(), PointerEvent, false );
	}
}


void FVREditorUISystem::Tick( FEditorViewportClient* ViewportClient, const float DeltaTime )
{
	if ( bRefocusViewport )
	{
		bRefocusViewport = false;
		FSlateApplication::Get().SetUserFocus(0, ViewportClient->GetEditorViewportWidget());
	}

	// Figure out if one hand is "aiming toward" the other hand.  We'll fade in a UI on the hand being
	// aimed at when the user does this.
	if( QuickMenuUI != nullptr )
	{
		int32 HandIndexWithQuickMenu = INDEX_NONE;
		if( QuickMenuUI->IsUIVisible() )
		{
			HandIndexWithQuickMenu = QuickMenuUI->GetDockedTo() == AVREditorFloatingUI::EDockedTo::LeftArm ? VREditorConstants::LeftHandIndex : VREditorConstants::RightHandIndex;
		}

		const float WorldScaleFactor = Owner.GetWorldScaleFactor();

		int32 HandIndexThatNeedsQuickMenu = INDEX_NONE;
		for( int32 HandIndex = 0; HandIndex < VREditorConstants::NumVirtualHands; ++HandIndex )
		{
			bool bShowQuickMenuOnArm = false;

			const FVirtualHand& Hand = Owner.GetVirtualHand( HandIndex );
			const FVirtualHand& OtherHand = Owner.GetOtherHand( HandIndex );
			const int32 OtherHandIndex = Owner.GetOtherHandIndex( HandIndex );		

			// @todo vreditor tweak: Weird to hard code this here.  Probably should be an accessor on the hand itself, and based on the actual device type
			const FTransform UICapsuleTransform = OtherHand.Transform;
			const FVector UICapsuleStart = FVector( -9.0f, 0.0f, 0.0f ) * WorldScaleFactor;
			const FVector UICapsuleEnd = FVector( -18.0f, 0.0f, 0.0f ) * WorldScaleFactor;
			const float UICapsuleLocalRadius = 6.0f * WorldScaleFactor;
			const float MinDistanceToUICapsule = 8.0f * WorldScaleFactor;	// @todo vreditor tweak
			const FVector UIForwardVector = FVector::UpVector;
			const float MinDotForAimingAtOtherHand = 0.25f;	// @todo vreditor tweak

			if( Owner.IsHandAimingTowardsCapsule( HandIndex, UICapsuleTransform, UICapsuleStart, UICapsuleEnd, UICapsuleLocalRadius, MinDistanceToUICapsule, UIForwardVector, MinDotForAimingAtOtherHand ) )
			{
				bShowQuickMenuOnArm = true;
			}

			if( bShowQuickMenuOnArm )
			{
				HandIndexThatNeedsQuickMenu = OtherHandIndex;
			}
		}

		if( QuickMenuUI->IsUIVisible() )
		{
			// If we don't need a quick menu, or if a different hand needs to spawn it, then kill the existing menu
			if( HandIndexThatNeedsQuickMenu == INDEX_NONE || HandIndexThatNeedsQuickMenu != HandIndexWithQuickMenu  )
			{
				// Despawn
				Owner.GetVirtualHand( HandIndexWithQuickMenu ).bHasUIOnForearm = false;
				QuickMenuUI->ShowUI( false );
			}
		}

		if( HandIndexThatNeedsQuickMenu != INDEX_NONE && !QuickMenuUI->IsUIVisible() )
		{
			const AVREditorFloatingUI::EDockedTo DockTo = ( HandIndexThatNeedsQuickMenu == VREditorConstants::LeftHandIndex ) ? AVREditorFloatingUI::EDockedTo::LeftArm : AVREditorFloatingUI::EDockedTo::RightArm;
			QuickMenuUI->SetDockedTo( DockTo );
			QuickMenuUI->ShowUI( true );
			Owner.GetVirtualHand( HandIndexThatNeedsQuickMenu ).bHasUIOnForearm = true;
		}
	}

	// If the user is moving the analog stick, try to spawn the radial menu for that hand
	if( Owner.GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift )
	{
		for( int32 HandIndex = 0; HandIndex < VREditorConstants::NumVirtualHands; ++HandIndex )
		{
			const FVirtualHand& Hand = Owner.GetVirtualHand( HandIndex );
			const FVirtualHand& OtherHand = Owner.GetOtherHand( HandIndex );
			bool bShouldShowRadialMenu = false;

			const float MinJoystickOffsetBeforeRadialMenu = 0.15f;	// @todo vreditor twea

			const bool bOtherHandAlreadyHasRadialMenu =
				IsShowingRadialMenu( Owner.GetOtherHandIndex( HandIndex ) );

			if( !Hand.bHasUIInFront &&
				Hand.bIsTrackpadPositionValid[ 0 ] &&
				Hand.bIsTrackpadPositionValid[ 1 ] &&
				Hand.TrackpadPosition.Size() > MinJoystickOffsetBeforeRadialMenu &&
				GetDraggingDockUIHandIndex() != HandIndex &&
				!bOtherHandAlreadyHasRadialMenu )
			{
				bShouldShowRadialMenu = true;
			}

			if( bShouldShowRadialMenu )
			{
				TryToSpawnRadialMenu( HandIndex );
			}
			else
			{
				// Close it
				HideRadialMenu( HandIndex );
			}
		}
	}
	else
	{
		// Close the radial menu if it was not updated for a while
		RadialMenuHideDelayTime += DeltaTime;
		if( RadialMenuHideDelayTime >= VREd::RadialMenuFadeDelay->GetFloat() )
		{
			HideRadialMenu( QuickRadialMenu->GetDockedTo() == AVREditorFloatingUI::EDockedTo::LeftHand ? VREditorConstants::LeftHandIndex : VREditorConstants::RightHandIndex );
		}
	}

	// Tick all of our floating UIs
	for( AVREditorFloatingUI* FloatingUIPtr : FloatingUIs )
	{
		if( FloatingUIPtr != nullptr )
		{
			FloatingUIPtr->TickManually( DeltaTime );
		}
	}
}

void FVREditorUISystem::Render( const FSceneView* SceneView, FViewport* Viewport, FPrimitiveDrawInterface* PDI )
{
	// ...
}

void FVREditorUISystem::CreateUIs()
{
	const FIntPoint DefaultResolution( VREd::DefaultEditorUIResolutionX->GetInt(), VREd::DefaultEditorUIResolutionY->GetInt() );

	const bool bIsVREditorDemo = FParse::Param( FCommandLine::Get(), TEXT( "VREditorDemo" ) );	// @todo vreditor: Remove this when no longer needed
																								
	// @todo vreditor: Tweak
	if ( QuickMenuWidgetClass != nullptr )
	{
		const FIntPoint Resolution( VREd::QuickMenuUIResolutionX->GetInt(), VREd::QuickMenuUIResolutionY->GetInt() );

		const bool bWithSceneComponent = false;
		QuickMenuUI = GetOwner().SpawnTransientSceneActor< AVREditorFloatingUI >(TEXT("QuickMenu"), bWithSceneComponent);
		QuickMenuUI->SetUMGWidget( *this, QuickMenuWidgetClass, Resolution, 30.0f, AVREditorFloatingUI::EDockedTo::Nothing );
		QuickMenuUI->ShowUI( false );
		QuickMenuUI->SetRelativeOffset( FVector( -11.0f, 0.0f, 3.0f ) );
		FloatingUIs.Add( QuickMenuUI );
	}

	// Create the radial UI
	if ( QuickRadialWidgetClass != nullptr )
	{
		TSharedPtr<SWidget> WidgetToDraw;
		QuickRadialMenu = GetOwner().SpawnTransientSceneActor< AVREditorFloatingUI >(TEXT("QuickRadialmenu"), false);
		QuickRadialMenu->SetUMGWidget( *this, QuickRadialWidgetClass, DefaultResolution, 40.0f, AVREditorFloatingUI::EDockedTo::Nothing );
		QuickRadialMenu->ShowUI( false );
		QuickRadialMenu->SetActorEnableCollision( false );
		QuickRadialMenu->SetRelativeOffset( FVector( 10.0f, 0.0f, 10.f ) );
		QuickRadialMenu->SetCollisionOnShow( false );
		FloatingUIs.Add(QuickRadialMenu);
	}

	// Make some editor UIs!
	{
		{
			const FIntPoint Resolution( VREd::ContentBrowserUIResolutionX->GetInt(), VREd::ContentBrowserUIResolutionY->GetInt() );

			IContentBrowserSingleton& ContentBrowserSingleton = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>( "ContentBrowser" ).Get();;

			// @todo vreditor UI: We've turned off a LOT of content browser features that are clunky to use in VR right (pop ups, text fields, etc.)
			FContentBrowserConfig Config;
			Config.bCanShowClasses = false;
			Config.bCanShowRealTimeThumbnails = false;
			Config.InitialAssetViewType = EAssetViewType::Tile;
			Config.bCanShowDevelopersFolder = false;
			Config.bCanShowFolders = false;
			Config.bUsePathPicker = false;
			Config.bCanShowFilters = false;
			Config.bCanShowAssetSearch = false;
			Config.bUseSourcesView = true;
			Config.bExpandSourcesView = true;
			Config.ThumbnailLabel = EThumbnailLabel::NoLabel;
			Config.ThumbnailScale = 0.4f;

			if( bIsVREditorDemo )
			{
				Config.bUsePathPicker = false;
				Config.bCanShowFilters = false;
				Config.bShowAssetPathTree = false;
				Config.bAlwaysShowCollections = true;
				Config.SelectedCollectionName.Name = FName( "Meshes" );	// @todo vreditor: hard-coded collection name and type
				Config.SelectedCollectionName.Type = ECollectionShareType::CST_Shared;
			}
			else
			{
				Config.bCanShowFilters = true;
				Config.bUsePathPicker = true;
				Config.bShowAssetPathTree = true;
				Config.bAlwaysShowCollections = false;
			}

			Config.bShowBottomToolbar = false;
			Config.bCanShowLockButton = false;
			TSharedRef<SWidget> ContentBrowser = ContentBrowserSingleton.CreateContentBrowser( "VRContentBrowser", nullptr, &Config );

			TSharedRef<SWidget> WidgetToDraw =
				SNew( SDPIScaler )
				.DPIScale( VREd::ContentBrowserUIScale->GetFloat() )
				[
					ContentBrowser
				]
			;

			const bool bWithSceneComponent = false;
			AVREditorFloatingUI* ContentBrowserUI = GetOwner().SpawnTransientSceneActor< AVREditorDockableWindow >( TEXT( "ContentBrowserUI" ), bWithSceneComponent );
			ContentBrowserUI->SetSlateWidget( *this, WidgetToDraw, Resolution, VREd::ContentBrowserUISize->GetFloat(), AVREditorFloatingUI::EDockedTo::Nothing );
			ContentBrowserUI->ShowUI( false );
			FloatingUIs.Add( ContentBrowserUI );

			EditorUIPanels[ (int32)EEditorUIPanel::ContentBrowser ] = ContentBrowserUI;
		}

		{
			FSceneOutlinerModule& SceneOutlinerModule = FModuleManager::Get().LoadModuleChecked<FSceneOutlinerModule>( "SceneOutliner" );

			SceneOutliner::FInitializationOptions InitOptions;
			InitOptions.Mode = ESceneOutlinerMode::ActorBrowsing;

			TSharedRef<ISceneOutliner> SceneOutliner = SceneOutlinerModule.CreateSceneOutliner( InitOptions, FOnActorPicked() /* Not used for outliner when in browsing mode */ );
			TSharedRef<SWidget> WidgetToDraw =
				SNew( SDPIScaler )
				.DPIScale( VREd::EditorUIScale->GetFloat() )
				[
					SceneOutliner
				]
			;

			const bool bWithSceneComponent = false;
			AVREditorFloatingUI* WorldOutlinerUI = GetOwner().SpawnTransientSceneActor< AVREditorDockableWindow >( TEXT( "WorldOutlinerUI" ), bWithSceneComponent );
			WorldOutlinerUI->SetSlateWidget( *this, WidgetToDraw, DefaultResolution, VREd::EditorUISize->GetFloat(), AVREditorFloatingUI::EDockedTo::Nothing );
			WorldOutlinerUI->ShowUI( false );
			FloatingUIs.Add( WorldOutlinerUI );

			EditorUIPanels[ (int32)EEditorUIPanel::WorldOutliner ] = WorldOutlinerUI;
		}

		{
			const TSharedRef< ILevelEditor >& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" ).GetFirstLevelEditor().ToSharedRef();

			const FName TabIdentifier = NAME_None;	// No tab for us!
			TSharedRef<SWidget> ActorDetails = LevelEditor->CreateActorDetails( TabIdentifier );

			TSharedRef<SWidget> WidgetToDraw =
				SNew( SDPIScaler )
				.DPIScale( VREd::EditorUIScale->GetFloat() )
				[
					ActorDetails
				]
			;

			const bool bWithSceneComponent = false;
			AVREditorFloatingUI* ActorDetailsUI = GetOwner().SpawnTransientSceneActor< AVREditorDockableWindow >( TEXT( "ActorDetailsUI" ), bWithSceneComponent );
			ActorDetailsUI->SetSlateWidget( *this, WidgetToDraw, DefaultResolution, VREd::EditorUISize->GetFloat(), AVREditorFloatingUI::EDockedTo::Nothing );
			ActorDetailsUI->ShowUI( false );
			FloatingUIs.Add( ActorDetailsUI );

			EditorUIPanels[ (int32)EEditorUIPanel::ActorDetails ] = ActorDetailsUI;
		}

		{
			const TSharedRef< ILevelEditor >& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" ).GetFirstLevelEditor().ToSharedRef();

			TSharedRef<SWidget> Modes = LevelEditor->CreateToolBox();

			TSharedRef<SWidget> WidgetToDraw =
				SNew( SDPIScaler )
				.DPIScale( VREd::EditorUIScale->GetFloat() )
				[
					Modes
				]
			;

			const bool bWithSceneComponent = false;
			AVREditorFloatingUI* ModesUI = GetOwner().SpawnTransientSceneActor< AVREditorDockableWindow >( TEXT( "ModesUI" ), bWithSceneComponent );
			ModesUI->SetSlateWidget( *this, WidgetToDraw, DefaultResolution, VREd::EditorUISize->GetFloat(), AVREditorFloatingUI::EDockedTo::Nothing );
			ModesUI->ShowUI( false );
			FloatingUIs.Add( ModesUI );

			// @todo vreditor placement: This is required to force the modes UI to refresh -- otherwise it looks empty
			GLevelEditorModeTools().ActivateMode( FBuiltinEditorModes::EM_Placement );

			EditorUIPanels[ (int32)EEditorUIPanel::Modes ] = ModesUI;			
		}

		// Create the tutorial dockable window
		{
			const bool bShowAtStart = !bIsVREditorDemo;

			AVREditorFloatingUI* TutorialUI = GetOwner().SpawnTransientSceneActor< AVREditorDockableWindow >( TEXT( "TutorialUI" ), false );
			TutorialUI->SetUMGWidget( *this, TutorialWidgetClass, FIntPoint( VREd::TutorialUIResolutionX->GetFloat(), VREd::TutorialUIResolutionY->GetFloat() ), VREd::TutorialUISize->GetFloat(), bShowAtStart ? AVREditorFloatingUI::EDockedTo::Room : AVREditorFloatingUI::EDockedTo::Nothing );
			if( bShowAtStart )
			{
				TutorialUI->SetRelativeOffset( FVector( VREd::TutorialUILocationX->GetFloat(), VREd::TutorialUILocationY->GetFloat(), VREd::TutorialUILocationZ->GetFloat() ) );
				TutorialUI->SetLocalRotation( FRotator( VREd::TutorialUIPitch->GetFloat(), VREd::TutorialUIYaw->GetFloat(), 0 ) );
			}
			
			TutorialUI->ShowUI( bShowAtStart );

			FloatingUIs.Add( TutorialUI );

			EditorUIPanels[ (int32)EEditorUIPanel::Tutorial ] = TutorialUI;		
		}

		{
			const FIntPoint Resolution(VREd::AssetEditorUIResolutionX->GetInt(), VREd::AssetEditorUIResolutionY->GetInt());

			const bool bWithSceneComponent = false;
			AVREditorDockableWindow* TabManagerUI = GetOwner().SpawnTransientSceneActor< AVREditorDockableWindow >(TEXT("AssetEditor"), bWithSceneComponent);
			TabManagerUI->SetSlateWidget(*this, SNullWidget::NullWidget, Resolution, VREd::EditorUISize->GetFloat(), AVREditorFloatingUI::EDockedTo::Nothing);
			TabManagerUI->ShowUI( false );

			// @todo vreditor: Could use "Hovering" instead for better performance with many UIs, but needs to be manually refreshed in some cases
			TabManagerUI->GetWidgetComponent()->SetDrawingPolicy(EVREditorWidgetDrawingPolicy::Always);

			FloatingUIs.Add(TabManagerUI);

			EditorUIPanels[ (int32)EEditorUIPanel::AssetEditor ] = TabManagerUI;

			TSharedPtr<SWindow> TabManagerWindow = TabManagerUI->GetWidgetComponent()->GetSlateWindow();
			TSharedRef<SWindow> TabManagerWindowRef = TabManagerWindow.ToSharedRef();
			ProxyTabManager = MakeShareable(new FProxyTabmanager(TabManagerWindowRef));

			ProxyTabManager->OnTabOpened.Add(FOnTabEvent::FDelegate::CreateRaw(this, &FVREditorUISystem::OnProxyTabLaunched));
			ProxyTabManager->OnAttentionDrawnToTab.Add(FOnTabEvent::FDelegate::CreateRaw(this, &FVREditorUISystem::OnAttentionDrawnToTab));

			// We're going to start stealing tabs from the global tab manager inserting them into the world instead.
			FGlobalTabmanager::Get()->SetProxyTabManager(ProxyTabManager);
		}
	}
}

void FVREditorUISystem::CleanUpActorsBeforeMapChangeOrSimulate()
{
	for( AVREditorFloatingUI* FloatingUIPtr : FloatingUIs )
	{
		if( FloatingUIPtr != nullptr )
		{
			FloatingUIPtr->Destroy( false, false );
			FloatingUIPtr = nullptr;
		}
	}

	FloatingUIs.Reset();
	EditorUIPanels.Reset();
	QuickRadialMenu = nullptr;
	QuickMenuUI = nullptr;

	ProxyTabManager.Reset();

	// Remove the proxy tab manager, we don't want to steal tabs any more.
	FGlobalTabmanager::Get()->SetProxyTabManager(TSharedPtr<FProxyTabmanager>());
}


bool FVREditorUISystem::IsWidgetAnEditorUIWidget( const UActorComponent* WidgetComponent ) const
{
	if( WidgetComponent != nullptr && WidgetComponent->IsA( UWidgetComponent::StaticClass() ) )
	{
		for( AVREditorFloatingUI* FloatingUIPtr : FloatingUIs )
		{
			if( FloatingUIPtr != nullptr )
			{
				if( WidgetComponent == FloatingUIPtr->GetWidgetComponent() )
				{
					return true;
				}
			}
		}
	}

	return false;
}


bool FVREditorUISystem::IsShowingEditorUIPanel( const EEditorUIPanel EditorUIPanel ) const
{
	AVREditorFloatingUI* Panel = EditorUIPanels[ (int32)EditorUIPanel ];
	if( Panel != nullptr )
	{
		return Panel->IsUIVisible();
	}

	return false;
}


void FVREditorUISystem::ShowEditorUIPanel( const UWidgetComponent* WidgetComponent, const int32 HandIndex, const bool bShouldShow, const bool OnHand, const bool bRefreshQuickMenu )
{
	AVREditorFloatingUI* Panel = nullptr;
	for( AVREditorFloatingUI* CurrentPanel : EditorUIPanels )
	{
		if( CurrentPanel->GetWidgetComponent() == WidgetComponent )
		{
			Panel = CurrentPanel;
			break;
		}
	}

	ShowEditorUIPanel( Panel, HandIndex, bShouldShow, OnHand, bRefreshQuickMenu );
}


void FVREditorUISystem::ShowEditorUIPanel( const EEditorUIPanel EditorUIPanel, const int32 HandIndex, const bool bShouldShow, const bool OnHand, const bool bRefreshQuickMenu )
{
	AVREditorFloatingUI* Panel = EditorUIPanels[ (int32)EditorUIPanel ];
	ShowEditorUIPanel( Panel, HandIndex, bShouldShow, OnHand, bRefreshQuickMenu );
}

void FVREditorUISystem::ShowEditorUIPanel( AVREditorFloatingUI* Panel, const int32 HandIndex, const bool bShouldShow, const bool OnHand, const bool bRefreshQuickMenu )
{
	if( Panel != nullptr )
	{
		AVREditorFloatingUI::EDockedTo DockedTo = Panel->GetDockedTo();
		if( OnHand || DockedTo == AVREditorFloatingUI::EDockedTo::Nothing )
		{
			// Hide any panels that are already shown on this hand
			for( int32 PanelIndex = 0; PanelIndex < (int32)EEditorUIPanel::TotalCount; ++PanelIndex )
			{
				AVREditorFloatingUI* OtherPanel = EditorUIPanels[PanelIndex];
				if( OtherPanel != nullptr && OtherPanel->IsUIVisible() && OtherPanel->GetDockedTo() != AVREditorFloatingUI::EDockedTo::Room )
				{
					const uint32 OtherPanelDockedToHandIndex = OtherPanel->GetDockedTo() == AVREditorFloatingUI::EDockedTo::LeftHand ? VREditorConstants::LeftHandIndex : VREditorConstants::RightHandIndex;
					if( OtherPanelDockedToHandIndex == HandIndex )
					{
						OtherPanel->ShowUI( false );
						OtherPanel->SetDockedTo( AVREditorFloatingUI::EDockedTo::Nothing );
						Owner.GetVirtualHand( OtherPanelDockedToHandIndex ).bHasUIInFront = false;
					}
				}
			}
			
			const AVREditorFloatingUI::EDockedTo NewDockedTo = HandIndex == VREditorConstants::LeftHandIndex ? AVREditorFloatingUI::EDockedTo::LeftHand : AVREditorFloatingUI::EDockedTo::RightHand;
			Panel->SetDockedTo( NewDockedTo );
			
			if (bShouldShow)
			{
				Panel->SetScale( Panel->GetInitialScale() );
			}

			const FVector EditorUIRelativeOffset( Panel->GetSize().Y * 0.5f + VREd::EditorUIOffsetFromHand->GetFloat(), 0.0f, 0.0f );
			Panel->SetRelativeOffset( EditorUIRelativeOffset );
			Panel->SetLocalRotation( FRotator( 90.0f, 180.0f, 0.0f ) );
		}

		Panel->ShowUI( bShouldShow );

		if (Panel->GetDockedTo() == AVREditorFloatingUI::EDockedTo::LeftHand || Panel->GetDockedTo() == AVREditorFloatingUI::EDockedTo::RightHand)
		{
			Owner.GetVirtualHand( HandIndex ).bHasUIInFront = bShouldShow;
		}

		UGameplayStatics::PlaySound2D( Owner.GetWorld(), bShouldShow ? ShowUISound : HideUISound );

		if (bRefreshQuickMenu && QuickMenuUI)
		{
			QuickMenuUI->GetUserWidget<UVREditorQuickMenu>()->RefreshUI();
		}
	}
}


bool FVREditorUISystem::IsShowingRadialMenu( const int32 HandIndex ) const
{
	int32 DockedToHandIndex = INDEX_NONE;
	DockedToHandIndex = QuickRadialMenu->GetDockedTo() == AVREditorFloatingUI::EDockedTo::LeftHand ? VREditorConstants::LeftHandIndex : VREditorConstants::RightHandIndex;
	return DockedToHandIndex == HandIndex && !QuickRadialMenu->bHidden;
}


void FVREditorUISystem::UpdateRadialMenu( const int32 HandIndex )
{
	if(QuickRadialMenu->bHidden)
	{
		QuickRadialMenu->ShowUI( true, false );
	}

	if(!QuickRadialMenu->bHidden)
	{
		RadialMenuHideDelayTime = 0.0f;
		QuickRadialMenu->GetUserWidget<UVREditorRadialMenu>()->Update( Owner.GetVirtualHand( HandIndex ) );
	}
}


void FVREditorUISystem::TryToSpawnRadialMenu( const int32 HandIndex )
{
	FVirtualHand& Hand = Owner.GetVirtualHand( HandIndex );

	int32 DockedToHandIndex = INDEX_NONE;
	if( !QuickRadialMenu->bHidden )
	{
		DockedToHandIndex = QuickRadialMenu->GetDockedTo() == AVREditorFloatingUI::EDockedTo::LeftHand ? VREditorConstants::LeftHandIndex : VREditorConstants::RightHandIndex;
	}

	bool bNeedsSpawn =
		( QuickRadialMenu->bHidden || DockedToHandIndex != HandIndex ) &&
		Hand.DraggingMode != EVREditorDraggingMode::ActorsAtLaserImpact &&	// Don't show radial menu if the hand is busy dragging something around
		Hand.DraggingMode != EVREditorDraggingMode::ActorsFreely &&
		Hand.DraggingMode != EVREditorDraggingMode::ActorsWithGizmo &&
		Hand.DraggingMode != EVREditorDraggingMode::AssistingDrag &&
		DraggingUIHandIndex != HandIndex &&
		!Hand.bIsHoveringOverUI;	// Don't show radial menu when aiming at a UI  (too much clutter)

	UVREditorRadialMenu* RadialMenu = QuickRadialMenu->GetUserWidget<UVREditorRadialMenu>();
	// We need to update the trackpad position in the radialmenu before checking if it can be used
	RadialMenu->Update( Hand );
	if( RadialMenu && RadialMenu->IsInMenuRadius() )
	{
		bNeedsSpawn = false;
	}

	if( bNeedsSpawn )
	{
		DockedToHandIndex = HandIndex;

		const AVREditorFloatingUI::EDockedTo DockedTo = DockedToHandIndex == VREditorConstants::LeftHandIndex ? AVREditorFloatingUI::EDockedTo::LeftHand : AVREditorFloatingUI::EDockedTo::RightHand;
		QuickRadialMenu->SetDockedTo( DockedTo );
		QuickRadialMenu->ShowUI( true );
	}
}


void FVREditorUISystem::HideRadialMenu( const int32 HandIndex )
{
	if( IsShowingRadialMenu( HandIndex ) )
	{
		UVREditorRadialMenu* RadialMenu = QuickRadialMenu->GetUserWidget<UVREditorRadialMenu>();
		QuickRadialMenu->ShowUI( false, true, VREd::RadialMenuFadeDelay->GetFloat() );
	}
}


FTransform FVREditorUISystem::MakeDockableUITransformOnLaser( AVREditorDockableWindow* InitDraggingDockUI, const int32 HandIndex, const float DockSelectDistance ) const
{
	const FVirtualHand& Hand = Owner.GetVirtualHand( HandIndex );

	const FVector NewLocation = Hand.Transform.GetLocation() + ( Hand.Transform.GetRotation().Vector().GetSafeNormal() * DockSelectDistance );
	
	FRotator NewRotation = ( Hand.Transform.GetLocation() - NewLocation ).ToOrientationRotator();
	NewRotation.Roll = -Hand.Transform.GetRotation().Rotator().Roll;
	
	const FTransform LaserImpactToWorld( NewRotation, NewLocation );
	return LaserImpactToWorld;
}


FTransform FVREditorUISystem::MakeDockableUITransform( AVREditorDockableWindow* InitDraggingDockUI, const int32 HandIndex, const float DockSelectDistance ) const
{
	const FTransform UIOnLaserToWorld = MakeDockableUITransformOnLaser( DraggingUI, HandIndex, DockSelectDistance );
	const FTransform UIToUIOnLaser = DraggingUIOffsetTransform;
	
	const FTransform UIToWorld = UIToUIOnLaser * UIOnLaserToWorld;
	return UIToWorld;
}


AVREditorFloatingUI* FVREditorUISystem::StartDraggingDockUI( AVREditorDockableWindow* InitDraggingDockUI, const int32 HandIndex, const float DockSelectDistance )
{
	AVREditorFloatingUI::EDockedTo DockTo = InitDraggingDockUI->GetDockedTo();
	if( DockTo == AVREditorFloatingUI::EDockedTo::LeftHand || DockTo == AVREditorFloatingUI::EDockedTo::RightHand )
	{
		Owner.GetOtherHand( HandIndex ).bHasUIInFront = false;
	}

	DraggingUIHandIndex = HandIndex;

	FTransform UIToWorld = InitDraggingDockUI->GetActorTransform();
	UIToWorld.SetScale3D( FVector( 1.0f ) );
	const FTransform WorldToUI = UIToWorld.Inverse();

	const FTransform UIOnLaserToWorld = MakeDockableUITransformOnLaser( InitDraggingDockUI, HandIndex, DockSelectDistance );
	const FTransform UIOnLaserToUI = UIOnLaserToWorld * WorldToUI;
	const FTransform UIToUIOnLaser = UIOnLaserToUI.Inverse();
	DraggingUIOffsetTransform = UIToUIOnLaser;

	DraggingUI = InitDraggingDockUI;
	DraggingUI->SetDockedTo( AVREditorFloatingUI::EDockedTo::Dragging );

	UGameplayStatics::PlaySound2D( Owner.GetWorld(), StartDragUISound );

	return DraggingUI;
}

AVREditorDockableWindow* FVREditorUISystem::GetDraggingDockUI() const
{
	return DraggingUI;
}

int32 FVREditorUISystem::GetDraggingDockUIHandIndex() const
{
	return DraggingUIHandIndex;
}

void FVREditorUISystem::StopDraggingDockUI()
{
	DraggingUI = nullptr;
	FVirtualHand& Hand = Owner.GetVirtualHand( DraggingUIHandIndex );
	Hand.DraggingMode = EVREditorDraggingMode::Nothing;
	DraggingUIHandIndex = INDEX_NONE;

	UGameplayStatics::PlaySound2D( Owner.GetWorld(), StopDragUISound );
}

bool FVREditorUISystem::IsDraggingDockUI()
{
	return DraggingUI != nullptr &&  DraggingUI->GetDockedTo() == AVREditorFloatingUI::EDockedTo::Nothing;
}

void FVREditorUISystem::TogglePanelsVisibility()
{
	bPanelVisibilityToggle = !bPanelVisibilityToggle;
	
	UGameplayStatics::PlaySound2D( Owner.GetWorld(), bPanelVisibilityToggle ? ShowUISound : HideUISound );

	bool bFirstCheckOnHand = true;
	AVREditorFloatingUI* PanelOnHand = nullptr;
	for( AVREditorFloatingUI* Panel : EditorUIPanels )
	{
		if( Panel != nullptr && Panel->IsUIVisible() != bPanelVisibilityToggle )
		{
			bool bShouldSetNewVisibility = true;
			const AVREditorFloatingUI::EDockedTo DockedTo = Panel->GetDockedTo();
			if( DockedTo == AVREditorFloatingUI::EDockedTo::LeftHand ||  DockedTo == AVREditorFloatingUI::EDockedTo::RightHand  )
			{
				// Prevent the panel from spawning on the hand if there is already a UI on the hand
				// Iterate through all panels to check if there is already a panel on the hand, but only once and if we haven't found one yet
				if( bFirstCheckOnHand && PanelOnHand == nullptr )
				{
					bFirstCheckOnHand = false;
					for(AVREditorFloatingUI* OtherPanel : EditorUIPanels)
					{
						if( OtherPanel != Panel && OtherPanel->IsUIVisible() && ( OtherPanel->GetDockedTo() == AVREditorFloatingUI::EDockedTo::LeftHand || OtherPanel->GetDockedTo() == AVREditorFloatingUI::EDockedTo::RightHand ) )
						{
							PanelOnHand = OtherPanel;
							break;
						}
					}
				}

				if( !PanelOnHand )
				{
					const uint32 HandIndex = Panel->GetDockedTo() == AVREditorFloatingUI::EDockedTo::LeftHand ? VREditorConstants::LeftHandIndex : VREditorConstants::RightHandIndex;
					Owner.GetVirtualHand( HandIndex ).bHasUIInFront = bPanelVisibilityToggle;
					PanelOnHand = Panel;
				}
				else
				{
					bShouldSetNewVisibility = false;
				}
			}
			else if ( DockedTo == AVREditorFloatingUI::EDockedTo::Nothing )
			{
				bShouldSetNewVisibility = false;
			}

			if( bShouldSetNewVisibility )
			{
				Panel->ShowUI( bPanelVisibilityToggle );
			}
		}
	}

	if (QuickMenuUI)
	{
		QuickMenuUI->GetUserWidget<UVREditorQuickMenu>()->RefreshUI();
	}
}

float FVREditorUISystem::GetMaxDockWindowSize() const
{
	return VREd::MaxDockWindowSize->GetFloat();
}

float FVREditorUISystem::GetMinDockWindowSize() const
{
	return VREd::MinDockWindowSize->GetFloat();
}

void FVREditorUISystem::OnProxyTabLaunched(TSharedPtr<SDockTab> NewTab)
{
	ShowAssetEditor();
}

void FVREditorUISystem::OnAttentionDrawnToTab(TSharedPtr<SDockTab> NewTab)
{
	ShowAssetEditor();
}

void FVREditorUISystem::ShowAssetEditor()
{
	bRefocusViewport = true;

	// A tab was opened, so make sure the "Asset" UI is visible.  That's where the user can interact
	// with the newly-opened tab
	if ( !IsShowingEditorUIPanel(EEditorUIPanel::AssetEditor) )
	{
		// Always spawn on a hand.  But which hand?  Well, we'll choose the hand that isn't actively clicking on something using a laser.
		const int32 HandIndex = Owner.GetVirtualHand(VREditorConstants::LeftHandIndex).bIsClickingOnUI ? VREditorConstants::RightHandIndex : VREditorConstants::LeftHandIndex;	// Hand that did not clicked with a laser
		const bool bShouldShow = true;
		const bool bShowOnHand = true;
		ShowEditorUIPanel(EEditorUIPanel::AssetEditor, HandIndex, bShouldShow, bShowOnHand);

		// Play haptic effect so user knows to look at their hand that now has UI on it!
		const float Strength = VREd::UIAssetEditorSummonedOnHandHapticFeedbackStrength->GetFloat();
		Owner.PlayHapticEffect(
			HandIndex == VREditorConstants::LeftHandIndex ? Strength : 0.0f,
			HandIndex == VREditorConstants::RightHandIndex ? Strength : 0.0f);
	}
}

void FVREditorUISystem::TogglePanelVisibility( const EEditorUIPanel EditorUIPanel )
{
	AVREditorFloatingUI* Panel = EditorUIPanels[(int32)EditorUIPanel];
	if (Panel != nullptr)
	{
		const bool bIsShowing = Panel->IsUIVisible();
		const int32 HandIndexWithQuickMenu = QuickMenuUI->GetDockedTo() == AVREditorFloatingUI::EDockedTo::LeftArm ? VREditorConstants::LeftHandIndex : VREditorConstants::RightHandIndex;
		if( Panel->GetDockedTo() == AVREditorFloatingUI::EDockedTo::Room )
		{
			Panel->ShowUI( false );
			Panel->SetDockedTo( AVREditorFloatingUI::EDockedTo::Nothing );
		}
		else
		{
			ShowEditorUIPanel( Panel, HandIndexWithQuickMenu, !bIsShowing, true );
		}
	}
}

