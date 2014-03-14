// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineUserInterfaceClasses.h"
#include "EngineInterpolationClasses.h"

#include "SubtitleManager.h"
#include "Net/UnrealNetwork.h"
#include "Online.h"

#include "RenderCore.h"
#include "ColorList.h"
#include "Slate.h"

#include "IHeadMountedDisplay.h"
#include "SceneViewExtension.h"


DEFINE_LOG_CATEGORY(LogPlayerManagement);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

static TAutoConsoleVariable<int32> CVarViewportTest(
	TEXT("r.ViewportTest"),
	0,
	TEXT("Allows to test different viewport rectangle configuations (in game only) as they can happen when using Matinee/Editor.\n")
	TEXT("0: off(default)\n")
	TEXT("1..7: Various Configuations"),
	ECVF_RenderThreadSafe);

#endif


//////////////////////////////////////////////////////////////////////////
// Things used by ULocalPlayer::Exec
//@TODO: EXEC

bool GShouldLogOutAFrameOfMoveComponent = false;
bool GShouldLogOutAFrameOfSetBodyTransform = false;

extern int32 GetBoundFullScreenModeCVar();

//////////////////////////////////////////////////////////////////////////
// ULocalPlayer

FGameWorldContext::FGameWorldContext()
{

}

FGameWorldContext::FGameWorldContext( const AActor* InActor )
{
	SetPlayerController( GEngine->GetFirstLocalPlayerController( InActor->GetWorld() ) );
}

FGameWorldContext::FGameWorldContext( const class ULocalPlayer* InLocalPlayer )
{
	SetLocalPlayer( InLocalPlayer );
}

FGameWorldContext::FGameWorldContext( const class APlayerController* InPlayerController )
{
	SetPlayerController( InPlayerController );
}

FGameWorldContext::FGameWorldContext( const FGameWorldContext& InWorldContext )
{
	check(InWorldContext.GetLocalPlayer());
	SetLocalPlayer( InWorldContext.GetLocalPlayer() );	
}

UWorld* FGameWorldContext::GetWorld() const
{
	check( LocalPlayer.IsValid() );
	return LocalPlayer->GetWorld();	
}

ULocalPlayer* FGameWorldContext::GetLocalPlayer() const
{
	check( LocalPlayer.IsValid() );
	return LocalPlayer.Get();
}

APlayerController* FGameWorldContext::GetPlayerController() const
{
	check( LocalPlayer.IsValid() );
	return LocalPlayer->PlayerController;
}

bool FGameWorldContext::IsValid() const
{
	return LocalPlayer.IsValid();
}

void FGameWorldContext::SetLocalPlayer( const ULocalPlayer* InLocalPlayer )
{
	LocalPlayer = InLocalPlayer;
}

void FGameWorldContext::SetPlayerController( const APlayerController* InPlayerController )
{
	check( InPlayerController->IsLocalPlayerController() );
	LocalPlayer = CastChecked<ULocalPlayer>(InPlayerController->Player);
}

//////////////////////////////////////////////////////////////////////////
// ULocalPlayer

ULocalPlayer::ULocalPlayer(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void ULocalPlayer::PostInitProperties()
{
	Super::PostInitProperties();
	if ( !IsTemplate() )
	{
		ViewState.Allocate();
		
		if( GEngine->StereoRenderingDevice.IsValid() )
		{
			StereoViewState.Allocate();
		}
	}
}

void ULocalPlayer::PlayerAdded(UGameViewportClient* InViewportClient, int32 InControllerID)
{
	ViewportClient = InViewportClient;
	ControllerId = InControllerID;

	UClass* SpawnClass = GetOnlineSessionClass();
	OnlineSession = ConstructObject<UOnlineSession>(SpawnClass, this);
	if (OnlineSession)
	{
		OnlineSession->RegisterOnlineDelegates();
	}	
}

void ULocalPlayer::PlayerRemoved()
{
	// Clear all online delegates
	if (OnlineSession != NULL)
	{
		OnlineSession->ClearOnlineDelegates();
		OnlineSession = NULL;
	}
}

TSubclassOf<UOnlineSession> ULocalPlayer::GetOnlineSessionClass()
{
	return UOnlineSession::StaticClass();
}

void ULocalPlayer::HandleDisconnect(UWorld *World, UNetDriver *NetDriver)
{
	if (OnlineSession)
	{
		OnlineSession->HandleDisconnect(World, NetDriver);
	}
	else
	{
		// Let the engine cleanup this disconnect
		GEngine->HandleDisconnect(World, NetDriver);
	}
}

bool ULocalPlayer::SpawnPlayActor(const FString& URL,FString& OutError, UWorld* InWorld)
{
	check(InWorld);
	if ( InWorld->IsServer() )
	{
		FURL PlayerURL(NULL, *URL, TRAVEL_Absolute);

		// Get player nickname
		FString PlayerName = GetNickname();
		if (PlayerName.Len() > 0)
		{
			PlayerURL.AddOption(*FString::Printf(TEXT("Name=%s"), *PlayerName));
		}

		// Get player unique id
		TSharedPtr<FUniqueNetId> UniqueId = GetUniqueNetId();

		PlayerController = InWorld->SpawnPlayActor(this, ROLE_SimulatedProxy, PlayerURL, UniqueId, OutError, GEngine->GetGamePlayers(InWorld).Find(this));
	}
	else
	{
		UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
		// Statically bind to the specified player controller
		UClass* PCClass = GameEngine != NULL ?
			LoadClass<APlayerController>(NULL, *GameEngine->PendingLevelPlayerControllerClassName, NULL, LOAD_None, NULL) :
			NULL;
		if (PCClass == NULL)
		{
			// This failed to load so use the engine one as default
			PCClass = APlayerController::StaticClass();
			UE_LOG(LogPlayerManagement, Log, TEXT("PlayerController class for the pending level is %s"),*PCClass->GetFName().ToString());
		}
		// The PlayerController gets replicated from the client though the engine assumes that every Player always has
		// a valid PlayerController so we spawn a dummy one that is going to be replaced later.
		PlayerController = CastChecked<APlayerController>(InWorld->SpawnActor(PCClass));
		const int32 PlayerIndex=GEngine->GetGamePlayers(InWorld).Find(this);
		PlayerController->NetPlayerIndex = PlayerIndex;
	}
	return PlayerController != NULL;
}

void ULocalPlayer::SendSplitJoin()
{
	UNetDriver* NetDriver = NULL;

	UWorld* World = GetWorld();
	if (World)
	{
		NetDriver = World->GetNetDriver();
	}

	if (World == NULL || NetDriver == NULL || NetDriver->ServerConnection == NULL || NetDriver->ServerConnection->State != USOCK_Open)
	{
		UE_LOG(LogPlayerManagement, Warning, TEXT("SendSplitJoin(): Not connected to a server"));
	}
	else if (!bSentSplitJoin)
	{
		// make sure we don't already have a connection
		bool bNeedToSendJoin = false;
		if (PlayerController == NULL)
		{
			bNeedToSendJoin = true;
		}
		else if (NetDriver->ServerConnection->PlayerController != PlayerController)
		{
			bNeedToSendJoin = true;
			for (int32 i = 0; i < NetDriver->ServerConnection->Children.Num(); i++)
			{
				if (NetDriver->ServerConnection->Children[i]->PlayerController == PlayerController)
				{
					bNeedToSendJoin = false;
					break;
				}
			}
		}

		if (bNeedToSendJoin)
		{
			// use the default URL except for player name for splitscreen players
			FURL URL;
			URL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);

			// Send the player nickname at login
			FString PlayerName = GetNickname();
			if (PlayerName.Len() > 0)
			{
				URL.AddOption(*FString::Printf(TEXT("Name=%s"), *PlayerName));
			}

			// Send the player unique Id at login
			FUniqueNetIdRepl UniqueIdRepl(GetUniqueNetId());

			FString URLString = URL.ToString();
			FNetControlMessage<NMT_JoinSplit>::Send(NetDriver->ServerConnection, URLString, UniqueIdRepl);
			bSentSplitJoin = true;
		}
	}
}

void ULocalPlayer::FinishDestroy()
{
	if ( !IsTemplate() )
	{
		ViewState.Destroy();
		StereoViewState.Destroy();
	}
	Super::FinishDestroy();
}

void ULocalPlayer::GetViewPoint(FMinimalViewInfo& OutViewInfo)
{
	if (PlayerController != NULL)
	{
		if (PlayerController->PlayerCameraManager != NULL)
		{
			OutViewInfo = PlayerController->PlayerCameraManager->CameraCache.POV;
			OutViewInfo.FOV = PlayerController->PlayerCameraManager->GetFOVAngle();
			PlayerController->GetPlayerViewPoint(/*out*/ OutViewInfo.Location, /*out*/ OutViewInfo.Rotation);
		}
		else
		{
			PlayerController->GetPlayerViewPoint(/*out*/ OutViewInfo.Location, /*out*/ OutViewInfo.Rotation);
		}
	}

    // allow HMDs to override fov
    if (GEngine->HMDDevice.IsValid() && GEngine->IsStereoscopic3D())
    {
        float HmdFov = GEngine->HMDDevice->GetFieldOfView();
        if (HmdFov > 0)
        {
            OutViewInfo.FOV = HmdFov;
        }
    }
}

FSceneView* ULocalPlayer::CalcSceneView( class FSceneViewFamily* ViewFamily, 
	FVector& OutViewLocation, 
	FRotator& OutViewRotation, 
	FViewport* Viewport, 
	class FViewElementDrawer* ViewDrawer,
	EStereoscopicPass StereoPass)
{
	if ((PlayerController == NULL) || (Size.X <= 0.f) || (Size.Y <= 0.f) || (Viewport == NULL))
	{
		return NULL;
	}

	FSceneViewInitOptions ViewInitOptions;

	// get the projection data
	if (GetProjectionData(Viewport, StereoPass, /*inout*/ ViewInitOptions) == false)
	{
		// Return NULL if this we didn't get back the info we needed
		return NULL;
	}
	
	// return if we have an invalid view rect
	if (!ViewInitOptions.IsValidViewRectangle())
	{
		return NULL;
	}

	// Get the viewpoint...technically doing this twice
	// but it makes GetProjectionData better
	FMinimalViewInfo ViewInfo;
	GetViewPoint(ViewInfo);
	
	OutViewLocation = ViewInfo.Location;
	OutViewRotation = ViewInfo.Rotation;

	if (PlayerController->PlayerCameraManager != NULL)
	{
		// Apply screen fade effect to screen.
		if (PlayerController->PlayerCameraManager->bEnableFading)
		{
			ViewInitOptions.OverlayColor = PlayerController->PlayerCameraManager->FadeColor.ReinterpretAsLinear();
			ViewInitOptions.OverlayColor.A = FMath::Clamp(PlayerController->PlayerCameraManager->FadeAmount,0.0f,1.0f);
		}

		// Do color scaling if desired.
		if (PlayerController->PlayerCameraManager->bEnableColorScaling)
		{
			ViewInitOptions.ColorScale = FLinearColor(
				PlayerController->PlayerCameraManager->ColorScale.X,
				PlayerController->PlayerCameraManager->ColorScale.Y,
				PlayerController->PlayerCameraManager->ColorScale.Z
				);
		}

		// Was there a camera cut this frame?
		ViewInitOptions.bInCameraCut = PlayerController->PlayerCameraManager->bGameCameraCutThisFrame;
	}

	// Fill out the rest of the view init options
	ViewInitOptions.ViewFamily = ViewFamily;
	ViewInitOptions.SceneViewStateInterface = ((StereoPass != eSSP_RIGHT_EYE) ? ViewState.GetReference() : StereoViewState.GetReference());
	ViewInitOptions.ViewActor = PlayerController->GetViewTarget();
	ViewInitOptions.ViewElementDrawer = ViewDrawer;
	ViewInitOptions.BackgroundColor = FLinearColor::Black;
	ViewInitOptions.LODDistanceFactor = PlayerController->LocalPlayerCachedLODDistanceFactor;
	ViewInitOptions.StereoPass = StereoPass;
	ViewInitOptions.WorldToMetersScale = PlayerController->GetWorldSettings()->WorldToMeters;
	PlayerController->BuildHiddenComponentList(OutViewLocation, /*out*/ ViewInitOptions.HiddenPrimitives);

	FSceneView* View = new FSceneView(ViewInitOptions);
    
    View->ViewLocation = OutViewLocation;
    View->ViewRotation = OutViewRotation;

	//@TODO: SPLITSCREEN: This call will have an issue with splitscreen, as the show flags are shared across the view family
	EngineShowFlagOrthographicOverride(View->IsPerspectiveProjection(), ViewFamily->EngineShowFlags);

	check( PlayerController->GetWorld() );

    for( int ViewExt = 0; ViewExt < ViewFamily->ViewExtensions.Num(); ViewExt++ )
    {
        ViewFamily->ViewExtensions[ViewExt]->SetupView(*View);
    }

	ViewFamily->Views.Add(View);

	{
		View->StartFinalPostprocessSettings(OutViewLocation);

		//	CAMERA OVERRIDE
		//	NOTE: Matinee works through this channel
		View->OverridePostProcessSettings(ViewInfo.PostProcessSettings, ViewInfo.PostProcessBlendWeight);

		View->EndFinalPostprocessSettings();
	}

	// Upscaling
	{
		float LocalScreenPercentage = View->FinalPostProcessSettings.ScreenPercentage;

		float Fraction = 1.0f;

		// apply ScreenPercentage
		if (LocalScreenPercentage != 100.f)
		{
			Fraction = FMath::Clamp(LocalScreenPercentage / 100.0f, 0.1f, 4.0f);
		}

		// Window full screen mode with upscaling
		bool bFullscreen = false;
		if (GEngine && GEngine->GameViewport && GEngine->GameViewport->GetWindow().IsValid())
		{
			bFullscreen = GEngine->GameViewport->GetWindow()->GetWindowMode() != EWindowMode::Windowed;
		}

		if (bFullscreen)
		{
			int32 WindowModeType = GetBoundFullScreenModeCVar();

			// CVar mode 2 is fullscreen with upscale
			if(WindowModeType == 2)
			{
				FIntPoint WindowSize = Viewport->GetSizeXY();

				// allow only upscaling
				float FractionX = FMath::Clamp((float)GSystemResolution.ResX / WindowSize.X, 0.1f, 4.0f);
				float FractionY = FMath::Clamp((float)GSystemResolution.ResY / WindowSize.Y, 0.1f, 4.0f);

				// maintain a pixel aspect ratio of 1:1 for easier internal computations
				Fraction *= FMath::Max(FractionX, FractionY);
			}
		}

		// Upscale if needed
		if (Fraction != 1.0f)
		{
			// compute the view rectangle with the ScreenPercentage applied
			const FIntRect ScreenPercentageAffectedViewRect = ViewInitOptions.GetConstrainedViewRect().Scale(Fraction);
			View->SetScaledViewRect(ScreenPercentageAffectedViewRect);
		}
	}

	return View;
}

bool ULocalPlayer::GetPixelBoundingBox(const FBox& ActorBox, FVector2D& OutLowerLeft, FVector2D& OutUpperRight, const FVector2D* OptionalAllotedSize)
{
		//@TODO: CAMERA: This has issues with aspect-ratio constrained cameras
	if ((ViewportClient != NULL) && (ViewportClient->Viewport != NULL) && (PlayerController != NULL))
	{
		// get the projection data
		FSceneViewProjectionData ProjectionData;
        if (GetProjectionData(ViewportClient->Viewport, eSSP_FULL, /*out*/ ProjectionData) == false)
		{
			return false;
		}

		// if we passed in an optional size, use it for the viewrect
		FIntRect ViewRect = ProjectionData.GetConstrainedViewRect();
		if (OptionalAllotedSize != NULL)
		{
			ViewRect.Min = FIntPoint(0,0);
			ViewRect.Max = FIntPoint(OptionalAllotedSize->X, OptionalAllotedSize->Y);
		}

		// transform the box
		const int32 NumOfVerts = 8;
		FVector Vertices[NumOfVerts] = 
		{
			FVector(ActorBox.Min),
			FVector(ActorBox.Min.X, ActorBox.Min.Y, ActorBox.Max.Z),
			FVector(ActorBox.Min.X, ActorBox.Max.Y, ActorBox.Min.Z),
			FVector(ActorBox.Max.X, ActorBox.Min.Y, ActorBox.Min.Z),
			FVector(ActorBox.Max.X, ActorBox.Max.Y, ActorBox.Min.Z),
			FVector(ActorBox.Max.X, ActorBox.Min.Y, ActorBox.Max.Z),
			FVector(ActorBox.Min.X, ActorBox.Max.Y, ActorBox.Max.Z),
			FVector(ActorBox.Max)
		};

		// create the view projection matrix
		const FMatrix ViewProjectionMatrix = ProjectionData.ViewMatrix * ProjectionData.ProjectionMatrix;

		int SuccessCount = 0;
		OutLowerLeft = FVector2D(FLT_MAX, FLT_MAX);
		OutUpperRight = FVector2D(FLT_MIN, FLT_MIN);
		for (int i = 0; i < NumOfVerts; ++i)
		{
			//grab the point in screen space
			const FVector4 ScreenPoint = ViewProjectionMatrix.TransformFVector4( FVector4( Vertices[i], 1.0f) );

			if (ScreenPoint.W > 0.0f)
			{
				float InvW = 1.0f / ScreenPoint.W;
				FVector2D PixelPoint = FVector2D( ViewRect.Min.X + (0.5f + ScreenPoint.X * 0.5f * InvW) * ViewRect.Width(),
												  ViewRect.Min.Y + (0.5f - ScreenPoint.Y * 0.5f * InvW) * ViewRect.Height());
			
				PixelPoint.X = FMath::Clamp<float>(PixelPoint.X, 0, ViewRect.Width());
				PixelPoint.Y = FMath::Clamp<float>(PixelPoint.Y, 0, ViewRect.Height());

				OutLowerLeft.X = FMath::Min(OutLowerLeft.X, PixelPoint.X);
				OutLowerLeft.Y = FMath::Min(OutLowerLeft.Y, PixelPoint.Y);

				OutUpperRight.X = FMath::Max(OutUpperRight.X, PixelPoint.X);
				OutUpperRight.Y = FMath::Max(OutUpperRight.Y, PixelPoint.Y);

				++SuccessCount;
			}
		}

		// make sure we are calculating with more than one point;
		return SuccessCount >= 2;
	}
	else
	{
		return false;
	}
}

bool ULocalPlayer::GetPixelPoint(const FVector& InPoint, FVector2D& OutPoint, const FVector2D* OptionalAllotedSize)
{
	//@TODO: CAMERA: This has issues with aspect-ratio constrained cameras
	if ((ViewportClient != NULL) && (ViewportClient->Viewport != NULL) && (PlayerController != NULL))
	{
		// get the projection data
		FSceneViewProjectionData ProjectionData;
		if (GetProjectionData(ViewportClient->Viewport, eSSP_FULL, /*inout*/ ProjectionData) == false)
		{
			return false;
		}

		// if we passed in an optional size, use it for the viewrect
		FIntRect ViewRect = ProjectionData.GetConstrainedViewRect();
		if (OptionalAllotedSize != NULL)
		{
			ViewRect.Min = FIntPoint(0,0);
			ViewRect.Max = FIntPoint(OptionalAllotedSize->X, OptionalAllotedSize->Y);
		}

		// create the view projection matrix
		const FMatrix ViewProjectionMatrix = ProjectionData.ViewMatrix * ProjectionData.ProjectionMatrix;

		//@TODO: CAMERA: Validate this code!
		// grab the point in screen space
		const FVector4 ScreenPoint = ViewProjectionMatrix.TransformFVector4( FVector4( InPoint, 1.0f) );

		if (ScreenPoint.W > 0.0f)
		{
			float InvW = 1.0f / ScreenPoint.W;
			OutPoint = FVector2D( ViewRect.Min.X + (0.5f + ScreenPoint.X * 0.5f * InvW) * ViewRect.Width(),
								  ViewRect.Min.Y + (0.5f - ScreenPoint.Y * 0.5f * InvW) * ViewRect.Height());

			return true;
		}
	}
	return false;
}

bool ULocalPlayer::GetProjectionData(FViewport* Viewport, EStereoscopicPass StereoPass, FSceneViewProjectionData& ProjectionData)
{
	// If the actor
	if ((Viewport == NULL) || (PlayerController == NULL) || (Viewport->GetSizeXY().X == 0) || (Viewport->GetSizeXY().Y == 0))
	{
		return false;
	}

	int32 X = FMath::Trunc(Origin.X * Viewport->GetSizeXY().X);
	int32 Y = FMath::Trunc(Origin.Y * Viewport->GetSizeXY().Y);
	uint32 SizeX = FMath::Trunc(Size.X * Viewport->GetSizeXY().X);
	uint32 SizeY = FMath::Trunc(Size.Y * Viewport->GetSizeXY().Y);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	// We expect some size to avoid problems with the view rect manipulation
	if(SizeX > 50 && SizeY > 50)
	{
		int32 Value = CVarViewportTest.GetValueOnGameThread();

		if(Value)
		{
			int InsetX = SizeX / 4;
			int InsetY = SizeY / 4;

			// this allows to test various typical view port situations (todo: split screen)
			switch(Value)
			{
				case 1: X += InsetX; Y += InsetY; SizeX -= InsetX * 2; SizeY -= InsetY * 2;break;
				case 2: Y += InsetY; SizeY -= InsetY * 2; break;
				case 3: X += InsetX; SizeX -= InsetX * 2; break;
				case 4: SizeX /= 2; SizeY /= 2; break;
				case 5: SizeX /= 2; SizeY /= 2; X += SizeX;	break;
				case 6: SizeX /= 2; SizeY /= 2; Y += SizeY; break;
				case 7: SizeX /= 2; SizeY /= 2; X += SizeX; Y += SizeY; break;
			}
		}
	}
#endif

	FIntRect UnconstrainedRectangle = FIntRect(X, Y, X+SizeX, Y+SizeY);

	ProjectionData.SetViewRectangle(UnconstrainedRectangle);

	// Get the viewpoint.
	FMinimalViewInfo ViewInfo;
	GetViewPoint(/*out*/ ViewInfo);

    // If stereo rendering is enabled, update the size and offset appropriately for this pass
    const bool bNeedStereo = GEngine->IsStereoscopic3D() && (StereoPass != eSSP_FULL);
    if( bNeedStereo )
    {
        GEngine->StereoRenderingDevice->AdjustViewRect(StereoPass, X, Y, SizeX, SizeY);
    }

	// scale distances for cull distance purposes by the ratio of our current FOV to the default FOV
	PlayerController->LocalPlayerCachedLODDistanceFactor = ViewInfo.FOV / FMath::Max<float>(0.01f, (PlayerController->PlayerCameraManager != NULL) ? PlayerController->PlayerCameraManager->DefaultFOV : 90.f);
	
    FVector StereoViewLocation = ViewInfo.Location;
    if (GEngine->IsStereoscopic3D() && StereoPass != eSSP_FULL)
    {
        GEngine->StereoRenderingDevice->CalculateStereoViewOffset(StereoPass, ViewInfo.Rotation, GetWorld()->GetWorldSettings()->WorldToMeters, StereoViewLocation);
    }

    // Create the view matrix
    ProjectionData.ViewMatrix = FTranslationMatrix(-StereoViewLocation);
    ProjectionData.ViewMatrix = ProjectionData.ViewMatrix * FInverseRotationMatrix(ViewInfo.Rotation);
    ProjectionData.ViewMatrix = ProjectionData.ViewMatrix * FMatrix(
        FPlane(0,	0,	1,	0),
        FPlane(1,	0,	0,	0),
        FPlane(0,	1,	0,	0),
        FPlane(0,	0,	0,	1));

	if( !bNeedStereo )
	{
		// Create the projection matrix (and possibly constrain the view rectangle)
		if (ViewInfo.bConstrainAspectRatio)
		{		
			// Enforce a particular aspect ratio for the render of the scene. 
			// Results in black bars at top/bottom etc.
			ProjectionData.SetConstrainedViewRectangle(ViewportClient->Viewport->CalculateViewExtents(ViewInfo.AspectRatio, UnconstrainedRectangle));
	
			if (ViewInfo.ProjectionMode == ECameraProjectionMode::Orthographic)
			{
				const float YScale = 1.0f / ViewInfo.AspectRatio;
	
				const float OrthoWidth = ViewInfo.OrthoWidth / 2.0f;
				const float OrthoHeight = ViewInfo.OrthoWidth / 2.0f * YScale;
				const float NearPlane = ProjectionData.ViewMatrix.GetOrigin().Z;
				const float FarPlane = NearPlane - 2.0f*WORLD_MAX;
				const float InverseRange = 1.0f / (FarPlane - NearPlane);
				const float ZScale = -2.0f * InverseRange;
				const float ZOffset = -(FarPlane + NearPlane) * InverseRange;

				ProjectionData.ProjectionMatrix = FReversedZOrthoMatrix(
					OrthoWidth,
					OrthoHeight,
					ZScale,
					ZOffset
					);
			}
			else
			{
				ProjectionData.ProjectionMatrix = FReversedZPerspectiveMatrix( 
					ViewInfo.FOV * (float)PI / 360.0f,
					ViewInfo.AspectRatio,
					1.0f,
					GNearClippingPlane );
			}
		}
		else 
		{
			float MatrixFOV = ViewInfo.FOV * (float)PI / 360.0f;
			float XAxisMultiplier;
			float YAxisMultiplier;

			// if x is bigger, and we're respecting x or major axis, AND mobile isn't forcing us to be Y axis aligned
			if (((SizeX > SizeY) && (AspectRatioAxisConstraint == AspectRatio_MajorAxisFOV)) || (AspectRatioAxisConstraint == AspectRatio_MaintainXFOV) || (ViewInfo.ProjectionMode == ECameraProjectionMode::Orthographic))
			{
				//if the viewport is wider than it is tall
				XAxisMultiplier = 1.0f;
				YAxisMultiplier = SizeX / (float)SizeY;
			}
			else
			{
				//if the viewport is taller than it is wide
				XAxisMultiplier = SizeY / (float)SizeX;
				YAxisMultiplier = 1.0f;
			}
	
			if (ViewInfo.ProjectionMode == ECameraProjectionMode::Orthographic)
			{
				const float NearPlane = ProjectionData.ViewMatrix.GetOrigin().Z;
				const float FarPlane = NearPlane - 2.0f*WORLD_MAX;
	
				const float OrthoWidth = ViewInfo.OrthoWidth / 2.0f * XAxisMultiplier;
				const float OrthoHeight = (ViewInfo.OrthoWidth / 2.0f) / YAxisMultiplier;
	
				const float InverseRange = 1.0f / (FarPlane - NearPlane);
				const float ZScale = -2.0f * InverseRange;
				const float ZOffset = -(FarPlane + NearPlane) * InverseRange;

				ProjectionData.ProjectionMatrix = FReversedZOrthoMatrix(
					OrthoWidth, 
					OrthoHeight,
					ZScale,
					ZOffset
					);		
			}
			else
			{
				ProjectionData.ProjectionMatrix = FReversedZPerspectiveMatrix(
					MatrixFOV,
					MatrixFOV,
					XAxisMultiplier,
					YAxisMultiplier,
					GNearClippingPlane,
					GNearClippingPlane
					);
			}
		}
	}
	else
	{
		// Let the stereoscopic rendering device handle creating its own projection matrix, as needed
		ProjectionData.ProjectionMatrix = GEngine->StereoRenderingDevice->GetStereoProjectionMatrix(StereoPass, ViewInfo.FOV);

		// calculate the out rect
		ProjectionData.SetViewRectangle(FIntRect(X, Y, X + SizeX, Y + SizeY));
	}
	
	
	return true;
}

void ULocalPlayer::ApplyWorldOffset(FVector InOffset)
{
	ViewState.GetReference()->ApplyWorldOffset(InOffset);
}

bool ULocalPlayer::HandleDNCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	// Create a pending Note actor (only in PIE)
	if( PlayerController )
	{
		FString Comment = FString(Cmd);
		int32 NewNoteIndex = GEngine->PendingDroppedNotes.AddZeroed();
		FDropNoteInfo& NewNote = GEngine->PendingDroppedNotes[NewNoteIndex];

		// Use the pawn's location if we have one
		if( PlayerController->GetPawnOrSpectator() != NULL )
		{
			NewNote.Location = PlayerController->GetPawnOrSpectator()->GetActorLocation();
		}
		else
		{
			// No pawn, so just use the camera's location
			FRotator CameraRotation;
			PlayerController->GetPlayerViewPoint(NewNote.Location, CameraRotation);
		}

		NewNote.Rotation = PlayerController->GetControlRotation();
		NewNote.Comment = Comment;
		UE_LOG(LogPlayerManagement, Log, TEXT("Note Dropped: (%3.2f,%3.2f,%3.2f) - '%s'"), NewNote.Location.X, NewNote.Location.Y, NewNote.Location.Z, *NewNote.Comment);
	}
	return true;
}


bool ULocalPlayer::HandleExitCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	// If there is no viewport it was already closed.
	if( ViewportClient->Viewport )
	{
		ViewportClient->CloseRequested(ViewportClient->Viewport);
	}
	return true;
}

bool ULocalPlayer::HandlePauseCommand( const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld )
{
	Super::Exec(InWorld, TEXT("Pause"),Ar);

	FSlateApplication::Get().ResetToDefaultInputSettings();

	return true;
}

bool ULocalPlayer::HandleListMoveBodyCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	GShouldLogOutAFrameOfSetBodyTransform = true;
	return true;
}

bool ULocalPlayer::HandleListAwakeBodiesCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	ListAwakeRigidBodies(true, GetWorld());
	return true;
}


bool ULocalPlayer::HandleListSimBodiesCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	ListAwakeRigidBodies(false, GetWorld());
	return true;
}

bool ULocalPlayer::HandleMoveComponentTimesCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	GShouldLogOutAFrameOfMoveComponent = true;
	return true;
}

bool ULocalPlayer::HandleListSkelMeshesCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	// Iterate over all skeletal mesh components and create mapping from skeletal mesh to instance.
	TMultiMap<USkeletalMesh*,USkeletalMeshComponent*> SkeletalMeshToInstancesMultiMap;
	for( TObjectIterator<USkeletalMeshComponent> It; It; ++It )
	{
		USkeletalMeshComponent* SkeletalMeshComponent = *It;
		USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->SkeletalMesh;

		if( !SkeletalMeshComponent->IsTemplate() )
		{
			SkeletalMeshToInstancesMultiMap.Add( SkeletalMesh, SkeletalMeshComponent );
		}
	}

	// Retrieve player location for distance checks.
	FVector PlayerLocation = FVector::ZeroVector;
	if( PlayerController && PlayerController->GetPawn() )
	{
		PlayerLocation = PlayerController->GetPawn()->GetActorLocation();
	}

	// Iterate over multi-map and dump information sorted by skeletal mesh.
	for( TObjectIterator<USkeletalMesh> It; It; ++It )
	{
		// Look up array of instances associated with this key/ skeletal mesh.
		USkeletalMesh* SkeletalMesh = *It;
		TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
		SkeletalMeshToInstancesMultiMap.MultiFind( SkeletalMesh, SkeletalMeshComponents );

		if( SkeletalMesh && SkeletalMeshComponents.Num() )
		{
			// Dump information about skeletal mesh.
			FSkeletalMeshResource* SkelMeshResource = SkeletalMesh->GetResourceForRendering(GRHIFeatureLevel);
			check(SkelMeshResource->LODModels.Num());
			UE_LOG(LogPlayerManagement, Log, TEXT("%5i Vertices for LOD 0 of %s"),SkelMeshResource->LODModels[0].NumVertices,*SkeletalMesh->GetFullName());

			// Dump all instances.
			for( int32 InstanceIndex=0; InstanceIndex<SkeletalMeshComponents.Num(); InstanceIndex++ )
			{
				USkeletalMeshComponent* SkeletalMeshComponent = SkeletalMeshComponents[InstanceIndex];					
				check(SkeletalMeshComponent);
				UWorld* World = SkeletalMeshComponent->GetWorld();
				check(World);
				float TimeSinceLastRender = World->GetTimeSeconds() - SkeletalMeshComponent->LastRenderTime;

				UE_LOG(LogPlayerManagement, Log, TEXT("%s%2i  Component    : %s"), 
					(TimeSinceLastRender > 0.5) ? TEXT(" ") : TEXT("*"), 
					InstanceIndex,
					*SkeletalMeshComponent->GetFullName() );
				if( SkeletalMeshComponent->GetOwner() )
				{
					UE_LOG(LogPlayerManagement, Log, TEXT("     Owner        : %s"),*SkeletalMeshComponent->GetOwner()->GetFullName());
				}
				UE_LOG(LogPlayerManagement, Log, TEXT("     LastRender   : %f"), TimeSinceLastRender);
				UE_LOG(LogPlayerManagement, Log, TEXT("     CullDistance : %f   Distance: %f   Location: (%7.1f,%7.1f,%7.1f)"), 
					SkeletalMeshComponent->CachedMaxDrawDistance,	
					FVector::Dist( PlayerLocation, SkeletalMeshComponent->Bounds.Origin ),
					SkeletalMeshComponent->Bounds.Origin.X,
					SkeletalMeshComponent->Bounds.Origin.Y,
					SkeletalMeshComponent->Bounds.Origin.Z );
			}
		}
	}
	return true;
}

bool ULocalPlayer::HandleListPawnComponentsCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	for( TObjectIterator<APawn> It; It; ++It )
	{
		APawn *Pawn = *It;	
		UE_LOG(LogPlayerManagement, Log, TEXT("Components for pawn: %s (collision component: %s)"),*Pawn->GetName(),*Pawn->GetRootComponent()->GetName());

		TArray<UActorComponent*> Components;
		Pawn->GetComponents(Components);

		for (int32 CompIdx = 0; CompIdx < Components.Num(); CompIdx++)
		{
			UActorComponent *Comp = Components[CompIdx];
			if (Comp->IsRegistered())
			{
				UE_LOG(LogPlayerManagement, Log, TEXT("  %d: %s"),CompIdx,*Comp->GetName());
			}
		}
	}
	return true;
}


bool ULocalPlayer::HandleExecCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	TCHAR Filename[512];
	if( FParse::Token( Cmd, Filename, ARRAY_COUNT(Filename), 0 ) )
	{
		ExecMacro( Filename, Ar );
	}
	return true;
}

bool ULocalPlayer::HandleToggleDrawEventsCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if( GEmitDrawEvents )
	{
		GEmitDrawEvents = false;
		UE_LOG(LogPlayerManagement, Warning, TEXT("Draw events are now DISABLED"));
	}
	else
	{
		GEmitDrawEvents = true;
		UE_LOG(LogPlayerManagement, Warning, TEXT("Draw events are now ENABLED"));
	}
#endif
	return true;
}

bool ULocalPlayer::HandleToggleStreamingVolumesCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	if (FParse::Command(&Cmd, TEXT("ON")))
	{
		GetWorld()->DelayStreamingVolumeUpdates( 0 );
	}
	else if (FParse::Command(&Cmd, TEXT("OFF")))
	{
		GetWorld()->DelayStreamingVolumeUpdates( INDEX_NONE );
	}
	else
	{
		if( GetWorld()->StreamingVolumeUpdateDelay == INDEX_NONE )
		{
			GetWorld()->DelayStreamingVolumeUpdates( 0 );
		}
		else
		{
			GetWorld()->DelayStreamingVolumeUpdates( INDEX_NONE );
		}
	}
	return true;
}

bool ULocalPlayer::HandleCancelMatineeCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	// allow optional parameter for initial time in the matinee that this won't work (ie, 
	// 'cancelmatinee 5' won't do anything in the first 5 seconds of the matinee)
	float InitialNoSkipTime = FCString::Atof(Cmd);

	// is the player in cinematic mode?
	if (PlayerController->bCinematicMode)
	{
		TArray<UWorld*> MatineeActorWorldsThatSkipped;
		// if so, look for all active matinees that has this Player in a director group
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			AMatineeActor* MatineeActor = Cast<AMatineeActor>(*It);

			if( MatineeActor )
			{
				// is it currently playing (and skippable)?
				if (MatineeActor->bIsPlaying && MatineeActor->bIsSkippable && (MatineeActor->bClientSideOnly || MatineeActor->GetWorld()->IsServer()))
				{
					for (int32 GroupIndex = 0; GroupIndex < MatineeActor->GroupInst.Num(); GroupIndex++)
					{
						// is the PC the group actor?
						if (MatineeActor->GroupInst[GroupIndex]->GetGroupActor() == PlayerController)
						{
							const float RightBeforeEndTime = 0.1f;
							// make sure we aren';t already at the end (or before the allowed skip time)
							if ((MatineeActor->InterpPosition < MatineeActor->MatineeData->InterpLength - RightBeforeEndTime) && 
								(MatineeActor->InterpPosition >= InitialNoSkipTime))
							{
								// skip to end
								MatineeActor->SetPosition(MatineeActor->MatineeData->InterpLength - RightBeforeEndTime, true);
								MatineeActorWorldsThatSkipped.AddUnique( MatineeActor->GetWorld() );
							}
						}
					}
				}
			}
		}

		if (MatineeActorWorldsThatSkipped.Num() != 0 )
		{
			for (int iActor = 0; iActor < MatineeActorWorldsThatSkipped.Num() ; iActor++)
			{
				AGameMode* const GameMode = MatineeActorWorldsThatSkipped[ iActor ]->GetAuthGameMode();
				if (GameMode)
				{
					GameMode->MatineeCancelled();
				}
			}
		}
	}
	return true;
}


bool ULocalPlayer::Exec(UWorld* InWorld, const TCHAR* Cmd,FOutputDevice& Ar)
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		// Override a few commands in PIE
		if( FParse::Command(&Cmd,TEXT("DN")) )
		{
			return HandleDNCommand( Cmd, Ar );
		}

		if( FParse::Command(&Cmd,TEXT("CloseEditorViewport")) 
		||	FParse::Command(&Cmd,TEXT("Exit")) 
		||	FParse::Command(&Cmd,TEXT("Quit")))
		{
			return HandleExitCommand( Cmd, Ar );
		}

		if( FParse::Command(&Cmd,TEXT("FocusNextPIEWindow")))
		{
			GEngine->FocusNextPIEWorld(InWorld);
			return true;
		}
		if( FParse::Command(&Cmd,TEXT("FocusLastPIEWindow")))
		{
			GEngine->FocusNextPIEWorld(InWorld, true);
			return true;
		}

		if( FParse::Command(&Cmd,TEXT("Pause") ))
		{
			return HandlePauseCommand( Cmd, Ar, InWorld );
		}
	}
#endif // WITH_EDITOR

// NOTE: all of these can probably be #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST) out

	if( FParse::Command(&Cmd,TEXT("LISTMOVEBODY")) )
	{
		return HandleListMoveBodyCommand( Cmd, Ar );
	}
#if WITH_PHYSX
	// This will list all awake rigid bodies
	else if( FParse::Command(&Cmd,TEXT("LISTAWAKEBODIES")) )
	{
		return HandleListAwakeBodiesCommand( Cmd, Ar );	
	}
	// This will list all simulating rigid bodies
	else if( FParse::Command(&Cmd,TEXT("LISTSIMBODIES")) )
	{
		return HandleListSimBodiesCommand( Cmd, Ar );		
	}
#endif
	else if( FParse::Command(&Cmd, TEXT("MOVECOMPTIMES")) )
	{
		return HandleMoveComponentTimesCommand( Cmd, Ar );	
	}
	else if( FParse::Command(&Cmd,TEXT("LISTSKELMESHES")) )
	{
		return HandleListSkelMeshesCommand( Cmd, Ar );
	}
	else if ( FParse::Command(&Cmd,TEXT("LISTPAWNCOMPONENTS")) )
	{
		return HandleListPawnComponentsCommand( Cmd, Ar );	
	}
	else if( FParse::Command(&Cmd,TEXT("EXEC")) )
	{
		return HandleExecCommand( Cmd, Ar );
	}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	else if( FParse::Command(&Cmd,TEXT("TOGGLEDRAWEVENTS")) )
	{
		return HandleToggleDrawEventsCommand( Cmd, Ar );
	}
#endif
	else if( FParse::Command(&Cmd,TEXT("TOGGLESTREAMINGVOLUMES")) )
	{
		return HandleToggleStreamingVolumesCommand( Cmd, Ar );
	}
	// @hack: This is a test matinee skipping function, quick and dirty to see if it's good enough for
	// gameplay. Will fix up better when we have some testing done!
	else if (FParse::Command(&Cmd, TEXT("CANCELMATINEE")))
	{
		return HandleCancelMatineeCommand( Cmd, Ar );	
	}
	else if(ViewportClient && ViewportClient->Exec( InWorld, Cmd,Ar))
	{
		return true;
	}
	else if ( Super::Exec( InWorld, Cmd, Ar ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ULocalPlayer::ExecMacro( const TCHAR* Filename, FOutputDevice& Ar )
{
	// make sure Binaries is specified in the filename
	FString FixedFilename;
	if (!FCString::Stristr(Filename, TEXT("Binaries")))
	{
		FixedFilename = FString(TEXT("../../Binaries/")) + Filename;
		Filename = *FixedFilename;
	}

	FString Text;
	if (FFileHelper::LoadFileToString(Text, Filename))
	{
		UE_LOG(LogPlayerManagement, Log, TEXT("Execing %s"), Filename);
		const TCHAR* Data = *Text;
		FString Line;
		while( FParse::Line(&Data, Line) )
		{
			Exec(GetWorld(), *Line, Ar);
		}
	}
	else
	{
		UE_SUPPRESS(LogExec, Warning, Ar.Logf(*FString::Printf( TEXT("Can't find file '%s'"), Filename) ));
	}
}

void ULocalPlayer::SetControllerId( int32 NewControllerId )
{
	if ( ControllerId != NewControllerId )
	{
		UE_LOG(LogPlayerManagement, Log, TEXT("%s changing ControllerId from %i to %i"), *GetFName().ToString(), ControllerId, NewControllerId);

		int32 CurrentControllerId = ControllerId;

		// set this player's ControllerId to -1 so that if we need to swap controllerIds with another player we don't
		// re-enter the function for this player.
		ControllerId = -1;

		// see if another player is already using this ControllerId; if so, swap controllerIds with them
		GEngine->SwapControllerId(this, CurrentControllerId, NewControllerId);
		ControllerId = NewControllerId;
	}
}

FString ULocalPlayer::GetNickname() const
{
	IOnlineIdentityPtr OnlineIdentityInt = Online::GetIdentityInterface();
	if (OnlineIdentityInt.IsValid())
	{
		return OnlineIdentityInt->GetPlayerNickname(ControllerId);
	}

	return TEXT("");
}

TSharedPtr<FUniqueNetId> ULocalPlayer::GetUniqueNetId() const
{
	IOnlineIdentityPtr OnlineIdentityInt = Online::GetIdentityInterface();
	if (OnlineIdentityInt.IsValid())
	{
		TSharedPtr<FUniqueNetId> UniqueId = OnlineIdentityInt->GetUniquePlayerId(ControllerId);
		if (UniqueId.IsValid())
		{
			return UniqueId;
		}
	}

	return NULL;
}

UWorld* ULocalPlayer::GetWorld() const
{
	UWorld* World = NULL;
	if (PlayerController != NULL)
	{
		World = PlayerController->GetWorld();
	}

	return World;
}


void ULocalPlayer::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	ULocalPlayer* This = CastChecked<ULocalPlayer>(InThis);

	FSceneViewStateInterface* Ref = This->ViewState.GetReference();

	if(Ref)
	{
		Ref->AddReferencedObjects(Collector);
	}

	UPlayer::AddReferencedObjects(This, Collector);
}
