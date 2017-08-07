#pragma once

// Unreal
#include "Camera/CameraComponent.h"
#include "SceneViewExtension.h"

// AppleARKit
#include "AppleARKitSession.h"
#include "AppleARKitCameraComponent.generated.h"

/**
 * A specialized version of a camera component that updates its transform
 * on tick, using a ARKit tracking session.
 */
UCLASS( ClassGroup=AppleARKit, BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent) )
class APPLEARKIT_API UAppleARKitCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:

	// Default constructor
	UAppleARKitCameraComponent();

	void BeginDestroy() override;

	/**
	 * Searches the last processed frame for anchors corresponding to a point in the captured image.
	 *
	 * A 2D point in the captured image's coordinate space can refer to any point along a line segment
	 * in the 3D coordinate space. Hit-testing is the process of finding anchors of a frame located along this line segment.
	 *
	 * NOTE: The hit test locations are reported in the game world coordinate space. For hit test results
	 * in ARKit space, you're after UAppleARKitSession::HitTestAtScreenPosition
	 * 
	 * @param ScreenPosition The viewport pixel coordinate of the trace origin.
	 */
	UFUNCTION( BlueprintCallable, Category="AppleARKit|Session" )
	bool HitTestAtScreenPosition( const FVector2D ScreenPosition, UPARAM(DisplayName="Flags", meta=(Bitmask, BitmaskEnum=EAppleARKitHitTestResultType)) int32 Types, TArray< FAppleARKitHitTestResult >& OutResults );
    
	/** 
	 * This lets you pass a 'corrected' position & orientation you want the camera to assume (in 
	 * world space). The offset from the camera's current transform to this new desired transform is 
	 * then passed to the FAppleARKitSession via SetBaseTransform, correcting / calibrating further 
	 * poses received from ARKit.
	 *
	 * This can be used for things like 'tap to place' where you want to move the game world into 
	 * camera view, without actually moving the world (the camera moves instead).
	 *
	 * @see FAppleARKitSession::SetBaseTransform
	 */
	UFUNCTION( BlueprintCallable, Category="AppleARKit|Camera" )
	void SetOrientationAndPosition( const FRotator& WorldOrientation, const FVector& WorldPosition );

	// AActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay( const EEndPlayReason::Type EndPlayReason ) override;
	virtual void TickComponent( float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction ) override;

private:

	/** View extension object that can persist on the render thread */
	class FAppleARKitViewExtension : public ISceneViewExtension, public TSharedFromThis<FAppleARKitViewExtension, ESPMode::ThreadSafe>
	{
	public:
		FAppleARKitViewExtension(UAppleARKitCameraComponent* InAppleARKitComponent);
		virtual ~FAppleARKitViewExtension() {}

		/** ISceneViewExtension interface */
		virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) final {}
		virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) final;
		virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) final {};
        virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) final;
        virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) final {};
		virtual int32 GetPriority() const override { return -10; }

	private:

		friend class UAppleARKitCameraComponent;

		UAppleARKitCameraComponent* AppleARKitComponent;
		UAppleARKitSession* Session;
	};
	TSharedPtr< FAppleARKitViewExtension, ESPMode::ThreadSafe > ViewExtension;

	// Session
	UPROPERTY( Transient )
	UAppleARKitSession* Session;

	// Copy of the GT frame used to generate the camera's matrices;
	FTransform GameThreadTransform;
	FQuat GameThreadOrientation;
	FVector GameThreadTranslation;	

	// The last frame applied to this camera
	double LastUpdateTimestamp = -1.0;
};
