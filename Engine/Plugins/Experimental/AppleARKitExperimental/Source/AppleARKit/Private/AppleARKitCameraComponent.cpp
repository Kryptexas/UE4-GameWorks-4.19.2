// AppleARKit
#include "AppleARKitCameraComponent.h"
#include "AppleARKit.h"
#include "AppleARKitSession.h"
#include "AppleARKitTransform.h"
#include "AppleARKitPlaneAnchor.h"

// UE4
#include "Misc/ScopeLock.h"
#include "EngineGlobals.h"
#include "IConsoleManager.h"
#include "DrawDebugHelpers.h"
#include "UnrealMathUtility.h"
#include "SceneView.h"

namespace {
	/** This is to prevent destruction of AppleARKit components while they are
	in the middle of being accessed by the render thread */
	FCriticalSection CritSect;	
} // anonymous namespace

int32 GAppleARKitLateUpdate = 1;
/** Console variable for specifying whether motion controller late update is used */
FAutoConsoleVariableRef CVarAppleARKitLateUpdate(
	TEXT("ar.EnableLateUpdate"),
	GAppleARKitLateUpdate,
	TEXT("This command allows you to specify whether the AppleARKit late update is applied.\n")
	TEXT(" 0: don't use late update\n")
	TEXT(" 1: use late update (default)"),
	ECVF_Default);

UAppleARKitCameraComponent::UAppleARKitCameraComponent()
{
	// Tick!
	PrimaryComponentTick.bCanEverTick = true;

	GameThreadTransform = FTransform::Identity;
	GameThreadOrientation = FQuat::Identity;
	GameThreadTranslation = FVector::ZeroVector;

	// // Tick as late as possible to catch the latest update before rendering
	// PrimaryComponentTick.TickGroup = TG_LastDemotable;
}

void UAppleARKitCameraComponent::BeginDestroy()
{
    Super::BeginDestroy();
	if (ViewExtension.IsValid())
	{
		{
			// This component could be getting accessed from the render thread so it needs to wait
			// before clearing MotionControllerComponent and allowing the destructor to continue
			FScopeLock ScopeLock(&CritSect);
			ViewExtension->AppleARKitComponent = nullptr;
		}

		ViewExtension.Reset();
	}
	
}

bool UAppleARKitCameraComponent::HitTestAtScreenPosition( const FVector2D ScreenPosition, int32 Flags, TArray< FAppleARKitHitTestResult >& OutResults )
{
	EAppleARKitHitTestResultType Types = (EAppleARKitHitTestResultType)Flags;

	// Check the session is running
	if ( !Session || !Session->IsRunning() )
	{
		UE_LOG( LogAppleARKit, Warning, TEXT("%s HitTestAtScreenPosition called while session not running"), *GetName() );
		return false;
	}

	// Get the raw ARKit space hit results
	bool bSuccess = Session->HitTestAtScreenPosition( ScreenPosition, Types, OutResults );

	// Apply the our parent transform to the hit results (if we're attached)
	if ( USceneComponent* ParentComponent = GetAttachParent() )
	{
		for ( FAppleARKitHitTestResult& Result : OutResults )
		{
			// Apply parent transfrom
			Result.Transform *= ParentComponent->GetComponentTransform();
		}
	}

	return bSuccess;
}

void UAppleARKitCameraComponent::SetOrientationAndPosition( const FRotator& Orientation, const FVector& Position )
{
	const bool bHasValidSession = (Session && Session->IsRunning());

	// First transform from world desired orientation & position to local space as we can't 
	FTransform WorldDesired = FTransform( Orientation, Position, GetComponentScale() );
	FTransform LocalDesired = GetAttachParent() ? WorldDesired * GetAttachParent()->GetComponentTransform().Inverse() : WorldDesired;

	// Calculate BaseTransform by figuring out the offset required to get from the current
	// camera transform to the desired one. This BaseTransform will then be pre-applied to
	// subsequent frames, by the Session, to correct the camera into the desired orientation
	// & position.
	FTransform BaseTransform = ( GetRelativeTransform() * (bHasValidSession ? Session->GetBaseTransform().Inverse() : FTransform::Identity) ).Inverse() * LocalDesired;

	// Force the new orientation & position to account for non-IOS platforms that won't receive new 
	// transforms during Tick with the new BaseTransform applied.
	SetWorldTransform( FTransform( Orientation, Position, GetComponentScale() ) );

	// Check the session is running
	if ( !bHasValidSession )
	{
		UE_LOG( LogAppleARKit, Warning, TEXT("%s SetOrientationAndPosition called while session not running"), *GetName() );
		return;
	}	

	// Pass new base transform to our session
	Session->SetBaseTransform( BaseTransform );
}

void UAppleARKitCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get a reference to the shared session
	Session = FAppleARKitExperimentalModule::Get().GetSession();
	check( Session != nullptr );

	// Start the shared session
	UE_LOG( LogAppleARKit, Log, TEXT("%s Starting Session %p ..."), *GetName(), &*Session );
	Session->Run();
}

void UAppleARKitCameraComponent::EndPlay( const EEndPlayReason::Type EndPlayReason )
{
	Super::EndPlay( EndPlayReason );

	// Stop the active session
	UE_LOG( LogAppleARKit, Log, TEXT("%s Stopping Session %p ..."), *GetName(), &*Session );
	Session->Pause();
}

void UAppleARKitCameraComponent::TickComponent( float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	// Check the session is running
	if ( !Session || !Session->IsRunning() )
	{
		UE_LOG( LogAppleARKit, Warning, TEXT("%s ticked while session not running"), *GetName() );
		return;
	}

	// Get current frame
	Session->UpdateCurrentGameThreadFrame();
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > CurrentFrame = Session->GetCurrentFrame_GameThread();
	if ( CurrentFrame.IsValid() && LastUpdateTimestamp != CurrentFrame->Timestamp )
	{
		// Skip identity transform poses that can be returned from the API when tracking is lost
		if ( !CurrentFrame->Camera.Transform.Equals( FTransform::Identity ) )
		{
			UE_LOG( LogAppleARKit, Verbose, TEXT("Setting %s transform from session (%p) CurrentFrame (%f) to: %s"), *GetName(), &*Session, CurrentFrame->Timestamp, *CurrentFrame->Camera.Transform.ToString() );

			// Update transform
			GameThreadTransform = CurrentFrame->Camera.Transform;
			GameThreadOrientation = CurrentFrame->Camera.Orientation;
			GameThreadTranslation = CurrentFrame->Camera.Translation;

			SetRelativeTransform( CurrentFrame->Camera.Transform );
		}

		// Update field of view
		if ( CurrentFrame->Camera.GetHorizontalFieldOfView() > 0 && GEngine && GEngine->GameViewport )
		{
			// Get viewport size
			FVector2D ViewportSize;
			GEngine->GameViewport->GetViewportSize( ViewportSize );

			// Get FOV for viewport size
			float NewFieldOfView = CurrentFrame->Camera.GetHorizontalFieldOfViewForScreen( EAppleARKitBackgroundFitMode::Fill, ViewportSize.X, ViewportSize.Y );

			// Has it changed?
			if ( FieldOfView != NewFieldOfView )
			{
				UE_LOG( LogAppleARKit, Verbose, TEXT("Setting %s horizontal FOV from session (%p) CurrentFrame (%f) to: %f"), *GetName(), &*Session, CurrentFrame->Timestamp, NewFieldOfView );

				// Update FOV
				SetFieldOfView( NewFieldOfView );
			}
		}
        if (!GAppleARKitLateUpdate)
        {
            Session->MarshallRenderThreadFrame(CurrentFrame);
        }

		LastUpdateTimestamp = CurrentFrame->Timestamp;
	}

	if (!ViewExtension.IsValid() && GEngine)
	{
		ViewExtension = FSceneViewExtensions::NewExtension<FAppleARKitViewExtension>(this);
	}

// #if ARKIT_SUPPORT

// 	// Draw plane anchors
// 	TMap< FGuid, UAppleARKitAnchor* > Anchors = Session->GetAnchors();
// 	for ( auto It = Anchors.CreateConstIterator(); It; ++It )
// 	{
// 		if ( UAppleARKitPlaneAnchor* PlaneAnchor = Cast< UAppleARKitPlaneAnchor >( It.Value() ) )
// 		{
// 			// Build local space transform
// 			FTransform PlaneWorldTransform = FTransform( PlaneAnchor->GetCenter() ) * PlaneAnchor->GetTransform() * Session->GetBaseTransform();

// 			// Fold in parent transform
// 			if ( USceneComponent* ParentComponent = GetAttachParent() )
// 			{
// 				PlaneWorldTransform *= ParentComponent->ComponentToWorld;
// 			}

// 			// Draw plane
// 			DrawDebugBox( GetWorld(), PlaneWorldTransform.GetTranslation(), PlaneAnchor->GetExtent() * PlaneWorldTransform.GetScale3D() / 2.0f, PlaneWorldTransform.GetRotation(), FColor::Green, /*bPersistentLines=*/false, /*LifeTime=*/-1.f, /*DepthPriority=*/0, /*Thickness=*/3.f);
// 		}
// 	}

// #endif
}

UAppleARKitCameraComponent::FAppleARKitViewExtension::FAppleARKitViewExtension(const FAutoRegister& AutoRegister, UAppleARKitCameraComponent* InAppleARKitComponent)
:FSceneViewExtensionBase(AutoRegister)
{
	check(InAppleARKitComponent);
	AppleARKitComponent = InAppleARKitComponent;
	Session = AppleARKitComponent->Session;
}

void UAppleARKitCameraComponent::FAppleARKitViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	check(IsInGameThread());
	
	// Set our base information for use in the render thread
	InView.BaseHmdLocation = AppleARKitComponent->GameThreadTranslation;
	InView.BaseHmdOrientation = AppleARKitComponent->GameThreadOrientation;
}


void UAppleARKitCameraComponent::FAppleARKitViewExtension::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	if (GAppleARKitLateUpdate)
	{
		check(IsInRenderingThread());

		TSharedPtr<FAppleARKitFrame, ESPMode::ThreadSafe> NewestFrame = Session->GetMostRecentFrame();

		// Skip identity transform poses that can be returned from the API when tracking is lost
        if (!NewestFrame.IsValid() || NewestFrame->Camera.Transform.Equals(FTransform::Identity))
		{
			return;
		}

		FVector CurHmdPosition = NewestFrame->Camera.Translation;
		FQuat CurHmdOrientation = NewestFrame->Camera.Orientation;
        
        FQuat DeltaOrient = InView.BaseHmdOrientation.Inverse() * CurHmdOrientation;
        InView.ViewRotation = FRotator(InView.ViewRotation.Quaternion() * DeltaOrient);
        
        FQuat LocalDeltaControlOrientation = InView.ViewRotation.Quaternion() * CurHmdOrientation.Inverse();
        FVector DeltaPosition = CurHmdPosition - InView.BaseHmdLocation;
        InView.ViewLocation += LocalDeltaControlOrientation.RotateVector(DeltaPosition);
        
		InView.UpdateViewMatrix();

		Session->SetRenderThreadFrame(NewestFrame);
	}
}
