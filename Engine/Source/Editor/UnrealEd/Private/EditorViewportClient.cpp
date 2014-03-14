// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "PreviewScene.h"
#include "EditorViewportClient.h"
#include "MouseDeltaTracker.h"
#include "CameraController.h"
#include "Editor/Matinee/Public/IMatinee.h"
#include "Editor/Matinee/Public/MatineeConstants.h"
#include "HighresScreenshot.h"
#include "EditorDragTools.h"

#define LOCTEXT_NAMESPACE "EditorViewportClient"

static TAutoConsoleVariable<int32> CVarAlignedOrthoZoom(
	TEXT("r.Editor.AlignedOrthoZoom"),
	1,
	TEXT("Only affects the editor ortho viewports.\n")
	TEXT(" 0: Each ortho viewport zoom in defined by the viewport width\n")
	TEXT(" 1: All ortho viewport zoom are locked to each other to allow axis lines to be aligned with each other."),
	ECVF_RenderThreadSafe);

float ComputeOrthoZoomFactor(const float ViewportWidth)
{
	float Ret = 1.0f;

	if(CVarAlignedOrthoZoom.GetValueOnGameThread())
	{
		// We want to have all ortho view ports scale the same way to have the axis aligned with each other.
		// So we take out the usual scaling of a view based on it's width.
		// That means when a view port is resized in x or y it shows more content, not the same content larger (for x) or has no effect (for y).
		// 500 is to get good results with existing view port settings.
		Ret = ViewportWidth / 500.0f;
	}

	return Ret;
}


namespace {
	static const float GridSize = 2048.0f;
	static const int32 CellSize = 16;
	static const float AutoViewportOrbitCameraTranslate = 256.0f;
	static const float LightRotSpeed = 0.22f;
}

namespace OrbitConstants
{
	const float OrbitPanSpeed = 1.0f;
	const float IntialLookAtDistance = 1024.f;
}

namespace FocusConstants
{
	const float TransitionTime = 0.25f;
}

/**
 * Cached off joystick input state
 */
class FCachedJoystickState
{
public:
	uint32 JoystickType;
	TMap <FKey, float> AxisDeltaValues;
	TMap <FKey, EInputEvent> KeyEventValues;
};



FViewportCameraTransform::FViewportCameraTransform()
	: TransitionCurve( new FCurveSequence( 0.0f, FocusConstants::TransitionTime, ECurveEaseFunction::CubicOut ) )
	, ViewLocation( FVector::ZeroVector )
	, DesiredLocation( FVector::ZeroVector )
	, ViewRotation( FRotator::ZeroRotator )
	, StartLocation( FVector::ZeroVector )
	, LookAt( FVector::ZeroVector )
	, OrthoZoom( DEFAULT_ORTHOZOOM )
{}

void FViewportCameraTransform::SetLocation( const FVector& Position )
{
	ViewLocation = Position;
	DesiredLocation = ViewLocation;
}

void FViewportCameraTransform::TransitionToLocation( const FVector& InDesiredLocation, bool bInstant )
{
	if( bInstant )
	{
		SetLocation( InDesiredLocation );
		TransitionCurve->JumpToEnd();
	}
	else
	{
		DesiredLocation = InDesiredLocation;
		StartLocation = ViewLocation;

		TransitionCurve->Play();
	}
}


bool FViewportCameraTransform::UpdateTransition()
{
	bool bIsAnimating = false;
	if( TransitionCurve->IsPlaying() || ViewLocation != DesiredLocation )
	{
		float LerpWeight = TransitionCurve->GetLerp();
		
		if( LerpWeight == 1.0f )
		{
			// Failsafe for the value not being exact on lerps
			ViewLocation = DesiredLocation;
		}
		else
		{
			ViewLocation = FMath::Lerp( StartLocation, DesiredLocation, LerpWeight );
		}

		
		bIsAnimating = true;
	}

	return bIsAnimating;
}

FMatrix FViewportCameraTransform::ComputeOrbitMatrix() const
{
	FTransform Transform =
	FTransform( -LookAt ) * 
	FTransform( FRotator(0,ViewRotation.Yaw,0) ) * 
	FTransform( FRotator(0, 0, ViewRotation.Pitch) ) *
	FTransform( FVector(0,(ViewLocation - LookAt).Size(), 0) );

	return Transform.ToMatrixNoScale() * FInverseRotationMatrix( FRotator(0,90.f,0) );
}

/**The Maximum Mouse/Camera Speeds Setting supported */
const uint32 FEditorViewportClient::MaxCameraSpeeds = 8;

/**

 * Static: Utility to get the actual mouse/camera speed (multiplier) based on the passed in setting.
 *
 * @param SpeedSetting	The desired speed setting
 */
float FEditorViewportClient::GetCameraSpeed(int32 SpeedSetting) const
{
	return 1.0f;
}


FEditorViewportClient::FEditorViewportClient(FPreviewScene* InPreviewScene)
	: ImmersiveDelegate()
	, VisibilityDelegate()
	, Viewport(NULL)
	, ViewTransform()
	, ViewportType(LVT_Perspective)
	, ViewState()
	, EngineShowFlags(ESFIM_Editor)
	, LastEngineShowFlags(ESFIM_Game)
	, ExposureSettings()
	, CurrentBufferVisualizationMode(NAME_None)
	, FramesSinceLastDraw(0)
	, ViewIndex(INDEX_NONE)
	, ViewFOV(EditorViewportDefs::DefaultPerspectiveFOVAngle)
	, FOVAngle(EditorViewportDefs::DefaultPerspectiveFOVAngle)
	, AspectRatio(1.777777f)
	, NearPlane(-1.0f)
	, bForcingUnlitForNewMap(false)
	, bInGameViewMode(false)
	, bWidgetAxisControlledByDrag(false)
	, bNeedsRedraw(true)
	, bNeedsLinkedRedraw(false)
	, bNeedsInvalidateHitProxy(false)
	, bUsingOrbitCamera(false)
	, bIsSimulateInEditorViewport( false )
	, bHasAudioFocus(false)
	, bShouldCheckHitProxy(false)
	, bUsesDrawHelper(true)
	, PerspViewModeIndex(VMI_Lit)
	, OrthoViewModeIndex(VMI_BrushWireframe)
	, bIsTracking( false )
	, bDraggingByHandle( false )
	, CurrentGestureDragDelta(FVector::ZeroVector)
	, CurrentGestureRotDelta(FRotator::ZeroRotator)
	, bDisableInput( false )
	, bDrawAxes(true)
	, bForceAudioRealtime(false)
	, bSetListenerPosition(false)
	, Widget(new FWidget)
	, MouseDeltaTracker(new FMouseDeltaTracker)
	, RecordingInterpEd(NULL)
	, bHasMouseMovedSinceClick(false)
	, CameraController( new FEditorCameraController() )
	, CameraUserImpulseData( new FCameraControllerUserImpulseData() )
	, TimeForForceRedraw( 0.0 )
	, FlightCameraSpeedScale( 1.0f )
	, bUseControllingActorViewInfo(false)
	, LastMouseX(0)
	, LastMouseY(0)
	, CachedMouseX(0)
	, CachedMouseY(0)
	, bIsRealtime(false)
	, bStoredRealtime(false)
	, bStoredShowFPS(false)
	, bStoredShowStats(false)
	, bShowFPS(false)
	, bShowStats(false)
	, PreviewScene(InPreviewScene)
	, bCameraLock(false)
	, bEnableSafeFrameOverride(false)
	, bShowAspectRatioBarsOverride(false)
	, bShowSafeFrameBoxOverride(false)
{

	ViewState.Allocate();

	// add this client to list of views, and remember the index
	ViewIndex = GEditor->AllViewportClients.Add(this);

	// Initialize the Cursor visibility struct
	RequiredCursorVisibiltyAndAppearance.bSoftwareCursorVisible = false;
	RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible = true;
	RequiredCursorVisibiltyAndAppearance.bDontResetCursor = false;
	RequiredCursorVisibiltyAndAppearance.bOverrideAppearance = false;
	RequiredCursorVisibiltyAndAppearance.RequiredCursor = EMouseCursor::Default;

	// Setup defaults for the common draw helper.
	DrawHelper.bDrawPivot = false;
	DrawHelper.bDrawWorldBox = false;
	DrawHelper.bDrawKillZ = false;
	DrawHelper.bDrawGrid = true;
	DrawHelper.GridColorAxis = FColor(160,160,160);
	DrawHelper.GridColorMajor = FColor(144,144,144);
	DrawHelper.GridColorMinor = FColor(128,128,128);
	DrawHelper.PerspectiveGridSize = GridSize;
	DrawHelper.NumCells = DrawHelper.PerspectiveGridSize / (CellSize * 2);

	// Most editor viewports do not want motion blur.
	EngineShowFlags.MotionBlur = 0;

	EngineShowFlags.SetSnap(1);

	SetViewMode(VMI_Lit);
}

FEditorViewportClient::~FEditorViewportClient()
{
	delete Widget;
	delete MouseDeltaTracker;

	delete CameraController;
	CameraController = NULL;

	delete CameraUserImpulseData;
	CameraUserImpulseData = NULL;

	if(Viewport)
	{
		UE_LOG(LogEditorViewport, Fatal, TEXT("Viewport != NULL in FLevelEditorViewportClient destructor."));
	}

	GEditor->AllViewportClients.Remove(this);

	// fix up the other viewport indices
	for (int32 ViewportIndex = ViewIndex; ViewportIndex < GEditor->AllViewportClients.Num(); ViewportIndex++)
	{
		GEditor->AllViewportClients[ViewportIndex]->ViewIndex = ViewportIndex;
	}

}

void FEditorViewportClient::RedrawRequested(FViewport* InViewport)
{
	bNeedsRedraw = true;
}

void FEditorViewportClient::RequestInvalidateHitProxy(FViewport* InViewport)
{
	bNeedsInvalidateHitProxy = true;
}

float FEditorViewportClient::GetOrthoUnitsPerPixel(const FViewport* Viewport) const
{
	const float SizeX = Viewport->GetSizeXY().X;

	// 15.0f was coming from the CAMERA_ZOOM_DIV marco, seems it was chosen arbitrarily
	return (GetOrthoZoom() / (SizeX * 15.f)) * ComputeOrthoZoomFactor(SizeX);
}

void FEditorViewportClient::SetViewLocationForOrbiting(const FVector& LookAtPoint, float DistanceToCamera )
{
	FMatrix Matrix = FTranslationMatrix(-GetViewLocation());
	Matrix = Matrix * FInverseRotationMatrix(GetViewRotation());
	FMatrix CamRotMat = Matrix.Inverse();
	FVector CamDir = FVector(CamRotMat.M[0][0],CamRotMat.M[0][1],CamRotMat.M[0][2]);
	SetViewLocation( LookAtPoint - DistanceToCamera * CamDir );
	SetLookAtLocation( LookAtPoint );
}

void FEditorViewportClient::SetInitialViewTransform( const FVector& ViewLocation, const FRotator& ViewRotation, float InOrthoZoom )
{
	SetViewLocation( ViewLocation );
	SetViewRotation( ViewRotation );

	// Make a look at location in front of the camera
	const FQuat CameraOrientation = FQuat::MakeFromEuler( GetViewRotation().Euler() );
	FVector Direction = CameraOrientation.RotateVector( FVector(1,0,0) );

	SetLookAtLocation( ViewLocation + Direction * OrbitConstants::IntialLookAtDistance ); 
}

void FEditorViewportClient::ToggleOrbitCamera( bool bEnableOrbitCamera )
{
	if( bUsingOrbitCamera != bEnableOrbitCamera )
	{
		bUsingOrbitCamera = bEnableOrbitCamera;

		// Convert orbit view to regular view
		FMatrix OrbitMatrix = ViewTransform.ComputeOrbitMatrix();
		OrbitMatrix = OrbitMatrix.Inverse();

		if( !bUsingOrbitCamera )
		{
			// Ensure that the view location and rotation is up to date to ensure smooth transition in and out of orbit mode
			SetViewRotation( OrbitMatrix.Rotator() );
		}
		else
		{
			bool bUpsideDown = (GetViewRotation().Pitch < -90.0f || GetViewRotation().Pitch > 90.0f || !FMath::IsNearlyZero( GetViewRotation().Roll, KINDA_SMALL_NUMBER ) );

			// if the camera is upside down compute the rotation differently to preserve pitch
			// otherwise the view will pop to right side up when transferring to orbit controls
			if( bUpsideDown )
			{
				FMatrix OrbitViewMatrix = FTranslationMatrix(-GetViewLocation());
				OrbitViewMatrix *= FInverseRotationMatrix(GetViewRotation());
				OrbitViewMatrix *= FRotationMatrix( FRotator(0,90.f,0) );

				FMatrix RotMat = FTranslationMatrix( -ViewTransform.GetLookAt() ) * OrbitViewMatrix;
				FMatrix RotMatInv = RotMat.Inverse();
				FRotator RollVec = RotMatInv.Rotator();
				FMatrix YawMat = RotMatInv * FInverseRotationMatrix( FRotator(0, 0, -RollVec.Roll));
				FMatrix YawMatInv = YawMat.Inverse();
				FRotator YawVec = YawMat.Rotator();
				FRotator rotYawInv = YawMatInv.Rotator();
				SetViewRotation( FRotator(-RollVec.Roll,YawVec.Yaw,0) );
			}
			else
			{
				SetViewRotation( OrbitMatrix.Rotator() );
			}
		}
		
		SetViewLocation( OrbitMatrix.GetOrigin() );
	}
}

void FEditorViewportClient::FocusViewportOnBox( const FBox& BoundingBox, bool bInstant /* = false */ )
{
	const FVector Position = BoundingBox.GetCenter();
	float Radius = BoundingBox.GetExtent().Size();

	const bool bEnable=false;
	ToggleOrbitCamera(bEnable);

	{
		if(!IsOrtho())
		{
		   /** 
		    * We need to make sure we are fitting the sphere into the viewport completely, so if the height of the viewport is less
		    * than the width of the viewport, we scale the radius by the aspect ratio in order to compensate for the fact that we have
		    * less visible vertically than horizontally.
		    */
		    if( AspectRatio > 1.0f )
		    {
			    Radius *= AspectRatio;
		    }

			/** 
			 * Now that we have a adjusted radius, we are taking half of the viewport's FOV,
			 * converting it to radians, and then figuring out the camera's distance from the center
			 * of the bounding sphere using some simple trig.  Once we have the distance, we back up
			 * along the camera's forward vector from the center of the sphere, and set our new view location.
			 */

			const float HalfFOVRadians = FMath::DegreesToRadians( ViewFOV / 2.0f);
			const float DistanceFromSphere = Radius / FMath::Tan( HalfFOVRadians );
			FVector CameraOffsetVector = ViewTransform.GetRotation().Vector() * -DistanceFromSphere;

			ViewTransform.SetLookAt(Position);		
			ViewTransform.TransitionToLocation( Position + CameraOffsetVector, bInstant );

		}
		else
		{
			// For ortho viewports just set the camera position to the center of the bounding volume.
			//SetViewLocation( Position );
			ViewTransform.TransitionToLocation( Position, bInstant );

			if( !(Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl)) )
			{
				/** 			
				* We also need to zoom out till the entire volume is in view.  The following block of code first finds the minimum dimension
				* size of the viewport.  It then calculates backwards from what the view size should be (The radius of the bounding volume),
				* to find the new OrthoZoom value for the viewport. The 15.0f is a fudge factor.
				*/
				float NewOrthoZoom;
				uint32 MinAxisSize = (AspectRatio > 1.0f) ? Viewport->GetSizeXY().Y : Viewport->GetSizeXY().X;
				float Zoom = Radius / (MinAxisSize / 2);

				NewOrthoZoom = Zoom * (Viewport->GetSizeXY().X*15.0f);
				NewOrthoZoom = FMath::Clamp<float>( NewOrthoZoom, MIN_ORTHOZOOM, MAX_ORTHOZOOM );
				ViewTransform.SetOrthoZoom( NewOrthoZoom );
			}
		}
	}

	// Tell the viewport to redraw itself.
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
//
// Configures the specified FSceneView object with the view and projection matrices for this viewport. 

FSceneView* FEditorViewportClient::CalcSceneView(FSceneViewFamily* ViewFamily)
{
	FSceneViewInitOptions ViewInitOptions;

	const FVector& ViewLocation = GetViewLocation();
	const FRotator& ViewRotation = GetViewRotation();

	const FIntPoint ViewportSizeXY = Viewport->GetSizeXY();

	FIntRect ViewRect = FIntRect(0, 0, ViewportSizeXY.X, ViewportSizeXY.Y);
	ViewInitOptions.SetViewRectangle(ViewRect);

	// no matter how we are drawn (forced or otherwise), reset our time here
	TimeForForceRedraw = 0.0;

	const ELevelViewportType EffectiveViewportType = GetViewportType();
	const bool bConstrainAspectRatio = bUseControllingActorViewInfo && ControllingActorViewInfo.bConstrainAspectRatio;

	ViewInitOptions.ViewMatrix = FTranslationMatrix(-GetViewLocation());	
	if (EffectiveViewportType == LVT_Perspective)
	{
		if (bUsingOrbitCamera)
		{
			ViewInitOptions.ViewMatrix = ViewTransform.ComputeOrbitMatrix();
		}
		else
		{
			ViewInitOptions.ViewMatrix = ViewInitOptions.ViewMatrix * FInverseRotationMatrix(ViewRotation);
		}

		ViewInitOptions.ViewMatrix = ViewInitOptions.ViewMatrix * FMatrix(
			FPlane(0,	0,	1,	0),
			FPlane(1,	0,	0,	0),
			FPlane(0,	1,	0,	0),
			FPlane(0,	0,	0,	1));

		float MinZ = GetNearClipPlane();
		float MaxZ = MinZ;
		float MatrixFOV = ViewFOV * (float)PI / 360.0f;

		if( bConstrainAspectRatio )
		{
			ViewInitOptions.ProjectionMatrix = FReversedZPerspectiveMatrix(
				MatrixFOV,
				MatrixFOV,
				1.0f,
				AspectRatio,
				MinZ,
				MaxZ
				);

			ViewInitOptions.SetConstrainedViewRectangle(Viewport->CalculateViewExtents(AspectRatio, ViewRect));
		}
		else
		{
			float XAxisMultiplier;
			float YAxisMultiplier;
			const EAspectRatioAxisConstraint AspectRatioAxisConstraint = GetDefault<ULevelEditorViewportSettings>()->AspectRatioAxisConstraint;

			if (((ViewportSizeXY.X > ViewportSizeXY.Y) && (AspectRatioAxisConstraint == AspectRatio_MajorAxisFOV)) || (AspectRatioAxisConstraint == AspectRatio_MaintainXFOV))
			{
				//if the viewport is wider than it is tall
				XAxisMultiplier = 1.0f;
				YAxisMultiplier = ViewportSizeXY.X / (float)ViewportSizeXY.Y;
			}
			else
			{
				//if the viewport is taller than it is wide
				XAxisMultiplier = ViewportSizeXY.Y / (float)ViewportSizeXY.X;
				YAxisMultiplier = 1.0f;
			}

			ViewInitOptions.ProjectionMatrix = FReversedZPerspectiveMatrix (
				MatrixFOV,
				MatrixFOV,
				XAxisMultiplier,
				YAxisMultiplier,
				MinZ,
				MaxZ
				);
		}
	}
	else
	{
		float ZScale = 0.5f / HALF_WORLD_MAX;
		float ZOffset = HALF_WORLD_MAX;

		//The divisor for the matrix needs to match the translation code.
		const float Zoom = GetOrthoUnitsPerPixel(Viewport);

		float OrthoWidth = Zoom * ViewportSizeXY.X / 2.0f;
		float OrthoHeight = Zoom * ViewportSizeXY.Y / 2.0f;

		if (EffectiveViewportType == LVT_OrthoXY)
		{
			ViewInitOptions.ViewMatrix = ViewInitOptions.ViewMatrix * FMatrix(
				FPlane(1,	0,	0,					0),
				FPlane(0,	-1,	0,					0),
				FPlane(0,	0,	-1,					0),
				FPlane(0,	0,	-ViewLocation.Z,	1));
		}
		else if (EffectiveViewportType == LVT_OrthoXZ)
		{
			ViewInitOptions.ViewMatrix = ViewInitOptions.ViewMatrix * FMatrix(
				FPlane(1,	0,	0,					0),
				FPlane(0,	0,	-1,					0),
				FPlane(0,	1,	0,					0),
				FPlane(0,	0,	-ViewLocation.Y,	1));
		}
		else if (EffectiveViewportType == LVT_OrthoYZ)
		{
			ViewInitOptions.ViewMatrix = ViewInitOptions.ViewMatrix * FMatrix(
				FPlane(0,	0,	1,					0),
				FPlane(1,	0,	0,					0),
				FPlane(0,	1,	0,					0),
				FPlane(0,	0,	ViewLocation.X,		1));
		}
		else if (EffectiveViewportType == LVT_OrthoFreelook)
		{
			ViewInitOptions.ViewMatrix = ViewInitOptions.ViewMatrix * FInverseRotationMatrix(ViewRotation);
			ViewInitOptions.ViewMatrix = ViewInitOptions.ViewMatrix * FMatrix(
				FPlane(0,	0,	 1,	0),
				FPlane(1,	0,	 0,	0),
				FPlane(0,	1,	 0,	0),
				FPlane(0,	0,	 0,	1));

			//@TODO: Note: This code needs to be kept in sync with the code in ULocalPlayer::GetProjectionData
			check(bUseControllingActorViewInfo);

			const float EffectiveAspectRatio = bConstrainAspectRatio ? AspectRatio : (ViewportSizeXY.X / (float)ViewportSizeXY.Y);
			const float YScale = 1.0f / EffectiveAspectRatio;
			OrthoWidth = ControllingActorViewInfo.OrthoWidth / 2.0f;
			OrthoHeight = (ControllingActorViewInfo.OrthoWidth / 2.0f) * YScale;

			const float NearViewPlane = ViewInitOptions.ViewMatrix.GetOrigin().Z;
			const float FarPlane = NearViewPlane - 2.0f*WORLD_MAX;

			const float InverseRange = 1.0f / (FarPlane - NearViewPlane);
			ZScale = -2.0f * InverseRange;
			ZOffset = -(FarPlane + NearViewPlane) * InverseRange;
		}
		else
		{
			// Unknown viewport type
			check(false);
		}

		ViewInitOptions.ProjectionMatrix = FReversedZOrthoMatrix(
			OrthoWidth,
			OrthoHeight,
			ZScale,
			ZOffset
			);

		if (bConstrainAspectRatio)
		{
			ViewInitOptions.SetConstrainedViewRectangle(Viewport->CalculateViewExtents(AspectRatio, ViewRect));
		}
	}

	ViewInitOptions.ViewFamily = ViewFamily;
	ViewInitOptions.SceneViewStateInterface = ViewState.GetReference();
	ViewInitOptions.ViewElementDrawer = this;

	ViewInitOptions.BackgroundColor = GetBackgroundColor();

	ViewInitOptions.EditorViewBitflag = (uint64)1 << ViewIndex, // send the bit for this view - each actor will check it's visibility bits against this

	// for ortho views to steal perspective view origin
	ViewInitOptions.OverrideLODViewOrigin = FVector::ZeroVector;
	ViewInitOptions.bUseFauxOrthoViewPos = true;

	FSceneView* View = new FSceneView(ViewInitOptions);

	ViewFamily->Views.Add(View);

	View->StartFinalPostprocessSettings(ViewLocation);

	OverridePostProcessSettings( *View );

	View->EndFinalPostprocessSettings();

	return View;
}

/** Determines if the new MoveCanvas movement should be used 
 * @return - true if we should use the new drag canvas movement.  Returns false for combined object-camera movement and marquee selection
 */
bool FLevelEditorViewportClient::ShouldUseMoveCanvasMovement()
{
	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton) ? true : false;
	const bool MiddleMouseButtonDown = Viewport->KeyState(EKeys::MiddleMouseButton) ? true : false;
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton) ? true : false;
	const bool bMouseButtonDown = (LeftMouseButtonDown || MiddleMouseButtonDown || RightMouseButtonDown );

	const bool AltDown = IsAltPressed();
	const bool ShiftDown = IsShiftPressed();
	const bool ControlDown = IsCtrlPressed();

	//if we're using the new move canvas mode, we're in an ortho viewport, and the mouse is down
	if (GetDefault<ULevelEditorViewportSettings>()->bPanMovesCanvas && IsOrtho() && bMouseButtonDown)
	{
		//MOVING CAMERA
		if ( !MouseDeltaTracker->UsingDragTool() && AltDown == false && ShiftDown == false && ControlDown == false && (Widget->GetCurrentAxis() == EAxisList::None) && (LeftMouseButtonDown ^ RightMouseButtonDown))
		{
			return true;
		}

		//OBJECT MOVEMENT CODE
		if ( ( AltDown == false && ShiftDown == false && ( LeftMouseButtonDown ^ RightMouseButtonDown ) ) && 
			( ( GetWidgetMode() == FWidget::WM_Translate && Widget->GetCurrentAxis() != EAxisList::None ) ||
			( GetWidgetMode() == FWidget::WM_TranslateRotateZ && Widget->GetCurrentAxis() != EAxisList::ZRotation &&  Widget->GetCurrentAxis() != EAxisList::None ) ) )
		{
			return true;
		}


		//ALL other cases hide the mouse
		return false;
	}
	else
	{
		//current system - do not show cursor when mouse is down
		return false;
	}
}


void FEditorViewportClient::ReceivedFocus(FViewport* InViewport)
{
	// Viewport has changed got to reset the cursor as it could of been left in any state
	UpdateRequiredCursorVisibility();
	ApplyRequiredCursorVisibility( true );

	// Force a cursor update to make sure its returned to default as it could of been left in any state and wont update itself till an action is taken
	SetRequiredCursorOverride( true , EMouseCursor::Default );
	FSlateApplication::Get().QueryCursor();

	if( IsMatineeRecordingWindow() )
	{
		// Allow the joystick to be used for matinee capture
		InViewport->CaptureJoystickInput( true );
	}
	
}

void FEditorViewportClient::LostFocus(FViewport* InViewport)
{
	StopTracking();	
}

void FEditorViewportClient::Tick(float DeltaTime)
{
	ConditionalCheckHoveredHitProxy();

	// Update show flags
	UpdateLightingShowFlags();

	const bool bIsAnimating = ViewTransform.UpdateTransition();

	if( !bIsAnimating )
	{
		// Update any real-time camera movement
		UpdateCameraMovement( DeltaTime );

		UpdateMouseDelta();
		
		UpdateGestureDelta();
	}


	// refresh ourselves if animating or told to from another view
	if ( bIsAnimating || ( TimeForForceRedraw != 0.0 && FPlatformTime::Seconds() > TimeForForceRedraw ) )
	{
		Invalidate();
	}
}

namespace ViewportDeadZoneConstants
{
	enum
	{
		NO_DEAD_ZONE,
		STANDARD_DEAD_ZONE
	};
};

float GetFilteredDelta(const float DefaultDelta, const uint32 DeadZoneType, const float StandardDeadZoneSize)
{
	if (DeadZoneType == ViewportDeadZoneConstants::NO_DEAD_ZONE)
	{
		return DefaultDelta;
	}
	else
	{
		//can't be one or normalizing won't work
		check(FMath::IsWithin<float>(StandardDeadZoneSize, 0.0f, 1.0f));
		//standard dead zone
		float ClampedAbsValue = FMath::Clamp(FMath::Abs(DefaultDelta), StandardDeadZoneSize, 1.0f);
		float NormalizedClampedAbsValue = (ClampedAbsValue - StandardDeadZoneSize)/(1.0f-StandardDeadZoneSize);
		float ClampedSignedValue = (DefaultDelta >= 0.0f) ? NormalizedClampedAbsValue : -NormalizedClampedAbsValue;
		return ClampedSignedValue;
	}
}


/**Applies Joystick axis control to camera movement*/
void FEditorViewportClient::UpdateCameraMovementFromJoystick(const bool bRelativeMovement, FCameraControllerConfig& InConfig)
{
	for(TMap<int32,FCachedJoystickState*>::TConstIterator JoystickIt(JoystickStateMap);JoystickIt;++JoystickIt)
	{
		FCachedJoystickState* JoystickState = JoystickIt.Value();
		check(JoystickState);
		for(TMap<FKey,float>::TConstIterator AxisIt(JoystickState->AxisDeltaValues);AxisIt;++AxisIt)
		{
			FKey Key = AxisIt.Key();
			float UnfilteredDelta = AxisIt.Value();
			const float StandardDeadZone = CameraController->GetConfig().ImpulseDeadZoneAmount;

			if (bRelativeMovement)
			{
				//XBOX Controller
				if (Key == EKeys::Gamepad_LeftX)
				{
					CameraUserImpulseData->MoveRightLeftImpulse += GetFilteredDelta(UnfilteredDelta, ViewportDeadZoneConstants::STANDARD_DEAD_ZONE, StandardDeadZone) * InConfig.TranslationMultiplier;
				}
				else if (Key == EKeys::Gamepad_LeftY)
				{
					CameraUserImpulseData->MoveForwardBackwardImpulse += GetFilteredDelta(UnfilteredDelta, ViewportDeadZoneConstants::STANDARD_DEAD_ZONE, StandardDeadZone) * InConfig.TranslationMultiplier;
				}
				else if (Key == EKeys::Gamepad_RightX)
				{
					float DeltaYawImpulse = GetFilteredDelta(UnfilteredDelta, ViewportDeadZoneConstants::STANDARD_DEAD_ZONE, StandardDeadZone) * InConfig.RotationMultiplier * (InConfig.bInvertX ? -1.0f : 1.0f);
					CameraUserImpulseData->RotateYawImpulse += DeltaYawImpulse;
					InConfig.bForceRotationalPhysics |= (DeltaYawImpulse != 0.0f);
				}
				else if (Key == EKeys::Gamepad_RightY)
				{
					float DeltaPitchImpulse = GetFilteredDelta(UnfilteredDelta, ViewportDeadZoneConstants::STANDARD_DEAD_ZONE, StandardDeadZone) * InConfig.RotationMultiplier * (InConfig.bInvertY ? -1.0f : 1.0f);
					CameraUserImpulseData->RotatePitchImpulse -= DeltaPitchImpulse;
					InConfig.bForceRotationalPhysics |= (DeltaPitchImpulse != 0.0f);
				}
				else if (Key == EKeys::Gamepad_LeftTriggerAxis)
				{
					CameraUserImpulseData->MoveUpDownImpulse -= GetFilteredDelta(UnfilteredDelta, ViewportDeadZoneConstants::STANDARD_DEAD_ZONE, StandardDeadZone) * InConfig.TranslationMultiplier;
				}
				else if (Key == EKeys::Gamepad_RightTriggerAxis)
				{
					CameraUserImpulseData->MoveUpDownImpulse += GetFilteredDelta(UnfilteredDelta, ViewportDeadZoneConstants::STANDARD_DEAD_ZONE, StandardDeadZone) * InConfig.TranslationMultiplier;
				}
			}
		}
		if (bRelativeMovement)
		{
			for(TMap<FKey,EInputEvent>::TConstIterator KeyIt(JoystickState->KeyEventValues);KeyIt;++KeyIt)
			{
				FKey Key = KeyIt.Key();
				EInputEvent KeyState = KeyIt.Value();

				const bool bPressed = (KeyState==IE_Pressed);
				const bool bRepeat = (KeyState == IE_Repeat);

				if ((Key == EKeys::Gamepad_LeftShoulder) && (bPressed || bRepeat))
				{
					CameraUserImpulseData->ZoomOutInImpulse +=  InConfig.ZoomMultiplier;
				}
				else if ((Key == EKeys::Gamepad_RightShoulder) && (bPressed || bRepeat))
				{
					CameraUserImpulseData->ZoomOutInImpulse -= InConfig.ZoomMultiplier;
				}
				else if (RecordingInterpEd)
				{
					bool bRepeatAllowed = RecordingInterpEd->IsRecordMenuChangeAllowedRepeat();
					if ((Key == EKeys::Gamepad_DPad_Up) && bPressed)
					{
						const bool bNextMenuItem = false;
						RecordingInterpEd->ChangeRecordingMenu(bNextMenuItem);
						bRepeatAllowed = false;
					}
					else if ((Key == EKeys::Gamepad_DPad_Down) && bPressed)
					{
						const bool bNextMenuItem = true;
						RecordingInterpEd->ChangeRecordingMenu(bNextMenuItem);
						bRepeatAllowed = false;
					}
					else if ((Key == EKeys::Gamepad_DPad_Right) && (bPressed || (bRepeat && bRepeatAllowed)))
					{
						const bool bIncrease= true;
						RecordingInterpEd->ChangeRecordingMenuValue(this, bIncrease);
					}
					else if ((Key == EKeys::Gamepad_DPad_Left) && (bPressed || (bRepeat && bRepeatAllowed)))
					{
						const bool bIncrease= false;
						RecordingInterpEd->ChangeRecordingMenuValue(this, bIncrease);
					}
					else if ((Key == EKeys::Gamepad_RightThumbstick) && (bPressed))
					{
						const bool bIncrease= true;
						RecordingInterpEd->ResetRecordingMenuValue(this);
					}
					else if ((Key == EKeys::Gamepad_LeftThumbstick) && (bPressed))
					{
						RecordingInterpEd->ToggleRecordMenuDisplay();
					}
					else if ((Key == EKeys::Gamepad_FaceButton_Bottom) && (bPressed))
					{
						RecordingInterpEd->ToggleRecordInterpValues();
					}
					else if ((Key == EKeys::Gamepad_FaceButton_Right) && (bPressed))
					{
						if (!RecordingInterpEd->GetMatineeActor()->bIsPlaying)
						{
							bool bLoop = true;
							bool bForward = true;
							RecordingInterpEd->StartPlaying(bLoop, bForward);
						}
						else
						{
							RecordingInterpEd->StopPlaying();
						}
					}

					if (!bRepeatAllowed)
					{
						//only respond to this event ONCE
						JoystickState->KeyEventValues.Remove(Key);
					}
				}
				if (bPressed)
				{
					//instantly set to repeat to stock rapid flickering until the time out
					JoystickState->KeyEventValues.Add(Key, IE_Repeat);
				}
			}
		}
	}
}

EMouseCursor::Type FEditorViewportClient::GetCursor(FViewport* InViewport,int32 X,int32 Y)
{
	EMouseCursor::Type MouseCursor = EMouseCursor::Default;

	bool bMoveCanvasMovement = ShouldUseMoveCanvasMovement();
	
	if (RequiredCursorVisibiltyAndAppearance.bOverrideAppearance &&
		RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible)
	{
		MouseCursor = RequiredCursorVisibiltyAndAppearance.RequiredCursor;
	}
	else if( MouseDeltaTracker->UsingDragTool() )
	{
		MouseCursor = EMouseCursor::Default;
	}
	else if (!RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible)
	{
		MouseCursor = EMouseCursor::None;
	}
	//only camera movement gets the hand icon
	else if (bMoveCanvasMovement && (Widget->GetCurrentAxis() == EAxisList::None) && bHasMouseMovedSinceClick)
	{
		//We're grabbing the canvas so the icon should look "grippy"
		MouseCursor = EMouseCursor::GrabHandClosed;
	}
	else if (bMoveCanvasMovement && 
		bHasMouseMovedSinceClick &&
		( GetWidgetMode() == FWidget::WM_Translate || GetWidgetMode() == FWidget::WM_TranslateRotateZ ) )
	{
		MouseCursor = EMouseCursor::CardinalCross;
	}
	//wyisyg mode
	else if (IsUsingAbsoluteTranslation() && bHasMouseMovedSinceClick)
	{
		MouseCursor = EMouseCursor::CardinalCross;
	}
	// Don't select widget axes by mouse over while they're being controlled by a mouse drag.
	else if( InViewport->IsCursorVisible() && !bWidgetAxisControlledByDrag )
	{
		// allow editor modes to override cursor
		EMouseCursor::Type EditorModeCursor = EMouseCursor::Default;
		if(GEditorModeTools().GetCursor(EditorModeCursor))
		{
			MouseCursor = EditorModeCursor;
		}
		else
		{
			HHitProxy* HitProxy = InViewport->GetHitProxy(X,Y);

			// Change the mouse cursor if the user is hovering over something they can interact with.
			if( HitProxy && !bUsingOrbitCamera )
			{
				MouseCursor = HitProxy->GetMouseCursor();
				bShouldCheckHitProxy = true;
			}
			else
			{
				// Turn off widget highlight if there currently is one
				if( Widget->GetCurrentAxis() != EAxisList::None )
				{
					SetCurrentWidgetAxis( EAxisList::None );
					Invalidate( false, false );
				}			

				// Turn off any hover effects as we are no longer over them.
				// @todo Viewport Cleanup

	/*
				if( HoveredObjects.Num() > 0 )
				{
					ClearHoverFromObjects();
					Invalidate( false, false );
				}			*/
			}
		}
	}



	CachedMouseX = X;
	CachedMouseY = Y;

	return MouseCursor;
}

bool FEditorViewportClient::IsOrtho() const
{
	return !IsPerspective();
}

bool FEditorViewportClient::IsPerspective() const
{
	return (GetViewportType() == LVT_Perspective);
}

bool FEditorViewportClient::IsAspectRatioConstrained() const
{
	return bUseControllingActorViewInfo && ControllingActorViewInfo.bConstrainAspectRatio;
}

ELevelViewportType FEditorViewportClient::GetViewportType() const
{
	ELevelViewportType EffectiveViewportType = ViewportType;
	if (bUseControllingActorViewInfo)
	{
		EffectiveViewportType = (ControllingActorViewInfo.ProjectionMode == ECameraProjectionMode::Perspective) ? LVT_Perspective : LVT_OrthoFreelook;
	}
	return EffectiveViewportType;
}

void FEditorViewportClient::SetViewportType( ELevelViewportType InViewportType )
{
	ViewportType = InViewportType;

	// Changing the type may also change the active view mode; re-apply that now
	ApplyViewMode(GetViewMode(), IsPerspective(), EngineShowFlags);

	// We might have changed to an orthographic viewport; if so, update any viewport links
	UpdateLinkedOrthoViewports(true);

	Invalidate();
}

bool FEditorViewportClient::IsActiveViewportType(ELevelViewportType InViewportType) const
{
	return GetViewportType() == InViewportType;
}

// Updates real-time camera movement.  Should be called every viewport tick!
void FEditorViewportClient::UpdateCameraMovement( float DeltaTime )
{
	// We only want to move perspective cameras around like this
	if( Viewport != NULL && IsPerspective() && !ShouldOrbitCamera() )
	{
		const bool bEnable = false;
		ToggleOrbitCamera(bEnable);

        const bool bIsUsingTrackpad = FSlateApplication::Get().IsUsingTrackpad();
        
		// Certain keys are only available while the flight camera input mode is active
		const bool bUsingFlightInput = IsFlightCameraInputModeActive();

		// Do we want to use the regular arrow keys for flight input?
		// Because the arrow keys are used for nudging actors, we'll only do this while the ALT key is up
		const bool bAltDown = IsAltPressed();
		const bool bRemapArrowKeys = !bAltDown;

		// Do we want to remap the various WASD keys for flight input?
		const bool bRemapWASDKeys =
			(!bAltDown && !IsCtrlPressed() && !IsShiftPressed()) &&
			(GetDefault<ULevelEditorViewportSettings>()->FlightCameraControlType == WASD_Always ||
			( bUsingFlightInput &&
			( GetDefault<ULevelEditorViewportSettings>()->FlightCameraControlType == WASD_RMBOnly && (Viewport->KeyState(EKeys::RightMouseButton ) ||Viewport->KeyState(EKeys::MiddleMouseButton) || Viewport->KeyState(EKeys::LeftMouseButton) || bIsUsingTrackpad ) ) ) ) &&
			!MouseDeltaTracker->UsingDragTool();

		//reset impulses if we're using WASD keys
		CameraUserImpulseData->MoveForwardBackwardImpulse = 0.0f;
		CameraUserImpulseData->MoveRightLeftImpulse = 0.0f;
		CameraUserImpulseData->MoveUpDownImpulse = 0.0f;
		CameraUserImpulseData->ZoomOutInImpulse = 0.0f;
		CameraUserImpulseData->RotateYawImpulse = 0.0f;
		CameraUserImpulseData->RotatePitchImpulse = 0.0f;
		CameraUserImpulseData->RotateRollImpulse = 0.0f;

		// Forward/back
		if( ( bRemapWASDKeys && Viewport->KeyState( EKeys::W ) ) ||
			( bRemapArrowKeys && Viewport->KeyState( EKeys::Up ) ) ||
			Viewport->KeyState( EKeys::NumPadEight ) )
		{
			CameraUserImpulseData->MoveForwardBackwardImpulse += 1.0f;
		}
		if( ( bRemapWASDKeys && Viewport->KeyState( EKeys::S ) ) ||
			( bRemapArrowKeys && Viewport->KeyState( EKeys::Down ) ) ||
			Viewport->KeyState( EKeys::NumPadTwo ) )
		{
			CameraUserImpulseData->MoveForwardBackwardImpulse -= 1.0f;
		}

		// Right/left
		if( ( bRemapWASDKeys && Viewport->KeyState( EKeys::D ) ) ||
			( bRemapArrowKeys && Viewport->KeyState( EKeys::Right ) ) ||
			Viewport->KeyState( EKeys::NumPadSix ) )
		{
			CameraUserImpulseData->MoveRightLeftImpulse += 1.0f;
		}
		if( ( bRemapWASDKeys && Viewport->KeyState( EKeys::A ) ) ||
			( bRemapArrowKeys && Viewport->KeyState( EKeys::Left ) ) ||
			Viewport->KeyState( EKeys::NumPadFour ) )
		{
			CameraUserImpulseData->MoveRightLeftImpulse -= 1.0f;
		}

		// Up/down
		if( ( bRemapWASDKeys && Viewport->KeyState( EKeys::E ) ) ||
			Viewport->KeyState( EKeys::PageUp ) || Viewport->KeyState( EKeys::NumPadNine ) || Viewport->KeyState( EKeys::Add ) )
		{
			CameraUserImpulseData->MoveUpDownImpulse += 1.0f;
		}
		if( ( bRemapWASDKeys && Viewport->KeyState( EKeys::Q ) ) ||
			Viewport->KeyState( EKeys::PageDown ) || Viewport->KeyState( EKeys::NumPadSeven ) || Viewport->KeyState( EKeys::Subtract ) )
		{
			CameraUserImpulseData->MoveUpDownImpulse -= 1.0f;
		}

		// Zoom FOV out/in
		if( ( bRemapWASDKeys && Viewport->KeyState( EKeys::Z ) ) ||
			Viewport->KeyState( EKeys::NumPadOne ) )
		{
			CameraUserImpulseData->ZoomOutInImpulse += 1.0f;
		}
		if( ( bRemapWASDKeys && Viewport->KeyState( EKeys::C ) ) ||
			Viewport->KeyState( EKeys::NumPadThree ) )
		{
			CameraUserImpulseData->ZoomOutInImpulse -= 1.0f;
		}

		if (!CameraController->IsRotating())
		{
			CameraController->GetConfig().bForceRotationalPhysics = false;
		}

		bool bIgnoreJoystickControls = false;
		//if we're playing back (without recording), stop input from being processed
		if (RecordingInterpEd && RecordingInterpEd->GetMatineeActor())
		{
			if (RecordingInterpEd->GetMatineeActor()->bIsPlaying && !RecordingInterpEd->IsRecordingInterpValues())
			{
				bIgnoreJoystickControls = true;
			}

			CameraController->GetConfig().bPlanarCamera = (RecordingInterpEd->GetCameraMovementScheme() == MatineeConstants::ECameraScheme::CAMERA_SCHEME_PLANAR_CAM);
		}

		//Now update for cached joystick info (relative movement first)
		UpdateCameraMovementFromJoystick(true, CameraController->GetConfig());

		//if we're not playing any cinematics right now
		if (!bIgnoreJoystickControls)
		{
			//Now update for cached joystick info (absolute movement second)
			UpdateCameraMovementFromJoystick(false, CameraController->GetConfig());
		}


		FVector NewViewLocation = GetViewLocation();
		FRotator NewViewRotation = GetViewRotation();
		FVector NewViewEuler = GetViewRotation().Euler();
		float NewViewFOV = ViewFOV;

		// We'll combine the regular camera speed scale (controlled by viewport toolbar setting) with
		// the flight camera speed scale (controlled by mouse wheel).
		const float CameraSpeed = GetCameraSpeed(GetDefault<ULevelEditorViewportSettings>()->CameraSpeed);
		const float FinalCameraSpeedScale = FlightCameraSpeedScale * CameraSpeed;

		// Only allow FOV recoil if flight camera mode is currently inactive.
		const bool bAllowRecoilIfNoImpulse = (!bUsingFlightInput) && (!IsMatineeRecordingWindow());

		// Update the camera's position, rotation and FOV
		float EditorMovementDeltaUpperBound = 1.0f;	// Never "teleport" the camera further than a reasonable amount after a large quantum

#if UE_BUILD_DEBUG
		// Editor movement is very difficult in debug without this, due to hitching
		// It is better to freeze movement during a hitch than to fly off past where you wanted to go 
		// (considering there will be further hitching trying to get back to where you were)
		EditorMovementDeltaUpperBound = .15f;
#endif

		CameraController->UpdateSimulation(
			*CameraUserImpulseData,
			FMath::Min(DeltaTime, EditorMovementDeltaUpperBound),
			bAllowRecoilIfNoImpulse,
			FinalCameraSpeedScale,
			NewViewLocation,
			NewViewEuler,
			NewViewFOV );


		// We'll zero out rotation velocity modifier after updating the simulation since these actions
		// are always momentary -- that is, when the user mouse looks some number of pixels,
		// we increment the impulse value right there
		{
			CameraUserImpulseData->RotateYawVelocityModifier = 0.0f;
			CameraUserImpulseData->RotatePitchVelocityModifier = 0.0f;
			CameraUserImpulseData->RotateRollVelocityModifier = 0.0f;
		}


		// Check for rotation difference within a small tolerance, ignoring winding
		if( !GetViewRotation().GetDenormalized().Equals( FRotator::MakeFromEuler( NewViewEuler ).GetDenormalized(), SMALL_NUMBER ) )
		{
			NewViewRotation = FRotator::MakeFromEuler( NewViewEuler );
		}

		if( !NewViewLocation.Equals( GetViewLocation(), SMALL_NUMBER ) ||
			NewViewRotation != GetViewRotation() ||
			!FMath::IsNearlyEqual( NewViewFOV, ViewFOV, float(SMALL_NUMBER) ) )
		{
			// Something has changed!
			const bool bInvalidateChildViews=true;

			// When flying the camera around the hit proxies dont need to be invalidated since we are flying around and not clicking on anything
			const bool bInvalidateHitProxies=!IsFlightCameraActive();
			Invalidate(bInvalidateChildViews,bInvalidateHitProxies);

			// Update the FOV
			ViewFOV = NewViewFOV;

			// Actually move/rotate the camera
			MoveViewportPerspectiveCamera(
				NewViewLocation - GetViewLocation(),
				NewViewRotation - GetViewRotation() );
		}
	}
}

/**
 * Forcibly disables lighting show flags if there are no lights in the scene, or restores lighting show
 * flags if lights are added to the scene.
 */
void FEditorViewportClient::UpdateLightingShowFlags()
{
	bool bViewportNeedsRefresh = false;

	if( bForcingUnlitForNewMap && !bInGameViewMode && IsPerspective() )
	{
		// We'll only use default lighting for viewports that are viewing the main world
		if (GWorld != NULL && GetScene() != NULL && GetScene()->GetWorld() != NULL && GetScene()->GetWorld() == GWorld )
		{
			// Check to see if there are any lights in the scene
			bool bAnyLights = GetScene()->HasAnyLights();
			if (bAnyLights)
			{
				// Is unlit mode currently enabled?  We'll make sure that all of the regular unlit view
				// mode show flags are set (not just EngineShowFlags.Lighting), so we don't disrupt other view modes
				if (GetViewMode() == VMI_Unlit)
				{
					// We have lights in the scene now so go ahead and turn lighting back on
					// designer can see what they're interacting with!
					SetViewMode(VMI_Lit);
				}

				// No longer forcing lighting to be off
				bForcingUnlitForNewMap = false;
			}
			else
			{
				// Is lighting currently enabled?
				if (GetViewMode() != VMI_Unlit)
				{
					// No lights in the scene, so make sure that lighting is turned off so the level
					// designer can see what they're interacting with!
					SetViewMode(VMI_Unlit);

				}
			}
		}
	}
}

bool FEditorViewportClient::GetActiveSafeFrame(float& OutAspectRatio) const
{
	ACameraActor* SelectedCamera = NULL;

	if ( ( EngineShowFlags.SafeFrames || bEnableSafeFrameOverride ) && !IsOrtho() )
	{
		for (FSelectionIterator It(GEditor->GetSelectedActorIterator()) ; It ; ++It)
		{
			ACameraActor* Camera = Cast<ACameraActor>(*It);
			if (Camera != NULL && Camera->CameraComponent->bConstrainAspectRatio)
			{
				if (SelectedCamera != NULL)
				{
					// multiple cameras selected
					return false;
				}
				SelectedCamera = Camera;
				OutAspectRatio = Camera->CameraComponent->AspectRatio;
			}
		}
	}

	return SelectedCamera != NULL;
}

void FEditorViewportClient::DrawSafeFrames(FViewport& InViewport, FSceneView& View, FCanvas& Canvas)
{
	float FixedAspectRatio;
	if (GetActiveSafeFrame(FixedAspectRatio))
	{
		// Get the size of the viewport
		const int32 SizeX = InViewport.GetSizeXY().X;
		const int32 SizeY = InViewport.GetSizeXY().Y;
		float ActualAspectRatio = (float)SizeX / (float)SizeY;

		float SafeWidth = SizeX;
		float SafeHeight = SizeY;
		FSlateRect SafeRect(0.0f, 0.0f, SizeX, SizeY);

		const bool bShouldDisplayAspectRatio = (!bEnableSafeFrameOverride || bShowAspectRatioBarsOverride);
		const bool bShouldDisplaySafeFrameBox = (!bEnableSafeFrameOverride || bShowSafeFrameBoxOverride);

		if( bShouldDisplayAspectRatio )
		{
			if (FixedAspectRatio < ActualAspectRatio )
			{
				// vertical bars required on left and right
				SafeWidth = FixedAspectRatio * SizeY;
				float CorrectedHalfWidth = SafeWidth * 0.5f;
				float CentreX = SizeX * 0.5f;
				float X1 = CentreX - CorrectedHalfWidth;
				float X2 = CentreX + CorrectedHalfWidth;
				SafeRect = FSlateRect(X1, 0.0f, X2, SizeY);

				FCanvasLineItem LineItem;
				LineItem.SetColor( FLinearColor(0.0f, 0.0f, 0.0f, 0.75f) );
				DrawSafeFrameQuad(Canvas, FVector2D(0.0f, 0.0f), FVector2D(X1, SizeY));
				LineItem.Draw( &Canvas, FVector2D(X1, 0.0f), FVector2D(X1, SizeY) );

				DrawSafeFrameQuad(Canvas, FVector2D(X2, 0.0f), FVector2D(SizeX, SizeY));
				LineItem.Draw( &Canvas, FVector2D(X2, 0.0f), FVector2D(X2, SizeY) );
			}
			else
			{
				// horizontal bars required on top and bottom
				SafeHeight = SizeX / FixedAspectRatio;
				float CorrectedHalfHeight = SafeHeight * 0.5f;
				float CentreY = SizeY * 0.5f;
				float Y1 = CentreY - CorrectedHalfHeight;
				float Y2 = CentreY + CorrectedHalfHeight;
				SafeRect = FSlateRect(0.0f, Y1, SizeX, Y2);

				FCanvasLineItem LineItem;
				LineItem.SetColor( FLinearColor(0.0f, 0.0f, 0.0f, 0.75f) );
				DrawSafeFrameQuad(Canvas, FVector2D(0.0f, 0.0f), FVector2D(SizeX, Y1));
				LineItem.Draw( &Canvas, FVector2D(0.0f, Y1), FVector2D(SizeX, Y1) );
				DrawSafeFrameQuad(Canvas, FVector2D(0.0f, Y2), FVector2D(SizeX, SizeY));
				LineItem.Draw( &Canvas, FVector2D(0.0f, Y2), FVector2D(SizeX, Y2) );
			}
		}

		if( bShouldDisplaySafeFrameBox )
		{
			// Draw a frame inside the safe rect padded inwards by a given percentage
			const float SafePadding = 0.075f;
			FSlateRect InnerRect = SafeRect.InsetBy(FMargin(0.5f * SafePadding * SafeRect.GetSize().Size()));
			FCanvasBoxItem BoxItem( FVector2D(InnerRect.Left, InnerRect.Top), InnerRect.GetSize() );
			BoxItem.SetColor( FLinearColor(0.0f, 0.0f, 0.0f, 0.5f) );
			Canvas.DrawItem( BoxItem );
		}
	}
}

void FEditorViewportClient::DrawSafeFrameQuad( FCanvas &Canvas, FVector2D V1, FVector2D V2 )
{
	static const FLinearColor SafeFrameColor(0.0f, 0.0f, 0.0f, 1.0f);
	FCanvasUVTri UVTriItem;
	UVTriItem.V0_Pos = FVector2D(V1.X, V1.Y);
	UVTriItem.V1_Pos = FVector2D(V2.X, V1.Y);
	UVTriItem.V2_Pos = FVector2D(V1.X, V2.Y);		
	FCanvasTriangleItem TriItem( UVTriItem, GWhiteTexture );
	UVTriItem.V0_Pos = FVector2D(V2.X, V1.Y);
	UVTriItem.V1_Pos = FVector2D(V2.X, V2.Y);
	UVTriItem.V2_Pos = FVector2D(V1.X, V2.Y);
	TriItem.TriangleList.Add( UVTriItem );
	TriItem.SetColor( SafeFrameColor );
	TriItem.Draw( &Canvas );
}

void FEditorViewportClient::UpdateMouseDelta()
{
	FVector DragDelta = FVector::ZeroVector;

	bool bNeedMovementSnapping = true;
	if( Widget->GetCurrentAxis() != EAxisList::None && bNeedMovementSnapping )
	{
		DragDelta = MouseDeltaTracker->GetDeltaSnapped();
	}
	else
	{
		DragDelta = MouseDeltaTracker->GetDelta();
	}

	GEditor->MouseMovement += FVector( FMath::Abs(DragDelta.X), FMath::Abs(DragDelta.Y), FMath::Abs(DragDelta.Z) );

	if( Viewport )
	{
		if( !DragDelta.IsNearlyZero() )
		{
			const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton);
			const bool MiddleMouseButtonDown = Viewport->KeyState(EKeys::MiddleMouseButton);
			const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton);
			const bool bIsUsingTrackpad = FSlateApplication::Get().IsUsingTrackpad();

			const bool bIsNonOrbitMiddleMouse = MiddleMouseButtonDown && !IsAltPressed();

			for( int32 x = 0 ; x < 3 ; ++x )
			{
				FVector WkDelta( 0.f, 0.f, 0.f );
				WkDelta[x] = DragDelta[x];
				if( !WkDelta.IsZero() )
				{
					// Convert the movement delta into drag/rotation deltas
					FVector Drag;
					FRotator Rot;
					FVector Scale;
					EAxisList::Type CurrentAxis = Widget->GetCurrentAxis();
					if ( IsOrtho() && ( LeftMouseButtonDown || bIsUsingTrackpad ) && RightMouseButtonDown )
					{
						bWidgetAxisControlledByDrag = false;
						Widget->SetCurrentAxis( EAxisList::None );
						MouseDeltaTracker->ConvertMovementDeltaToDragRot( this, WkDelta, Drag, Rot, Scale );
						Widget->SetCurrentAxis( CurrentAxis );
						CurrentAxis = EAxisList::None;
					}
					else
					{
						//if Absolute Translation, and not just moving the camera around
						if (IsUsingAbsoluteTranslation())
						{
							// Compute a view.
							FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
								Viewport,
								GetScene(),
								EngineShowFlags)
								.SetRealtimeUpdate( IsRealtime() ) );
							FSceneView* View = CalcSceneView( &ViewFamily );

							MouseDeltaTracker->AbsoluteTranslationConvertMouseToDragRot(View, this, Drag, Rot, Scale);
							//only need to go through this loop once
							x = 3;
						} 
						else
						{
							MouseDeltaTracker->ConvertMovementDeltaToDragRot( this, WkDelta, Drag, Rot, Scale );
						}
					}

					const bool bInputHandledByGizmos = InputWidgetDelta( Viewport, CurrentAxis, Drag, Rot, Scale );
					
					if( !Rot.IsZero() )
					{
						Widget->UpdateDeltaRotation();
					}

					if( !bInputHandledByGizmos )
					{
						if ( ShouldOrbitCamera() )
						{
							FVector TempDrag;
							FRotator TempRot;
							InputAxisForOrbit( Viewport, DragDelta, TempDrag, TempRot );
						}
						else
						{
							// Disable orbit camera
							const bool bEnable=false;
							ToggleOrbitCamera(bEnable);

							if( ShouldPanCamera() )
							{
								if( !IsOrtho())
								{
									const float CameraSpeed = GetCameraSpeed(GetDefault<ULevelEditorViewportSettings>()->CameraSpeed);
									Drag *= CameraSpeed;
								}
								MoveViewportCamera( Drag, Rot );
							}
						}
					}
			

					// Clean up
					MouseDeltaTracker->ReduceBy( WkDelta );
				}
			}

			Invalidate( false, false );
		}
	}
}


static bool IsOrbitRotationMode( FViewport* Viewport )
{
	bool	LeftMouseButton = Viewport->KeyState(EKeys::LeftMouseButton),
		MiddleMouseButton = Viewport->KeyState(EKeys::MiddleMouseButton),
		RightMouseButton = Viewport->KeyState(EKeys::RightMouseButton);
	return LeftMouseButton && !MiddleMouseButton && !RightMouseButton ;
}

static bool IsOrbitPanMode( FViewport* Viewport )
{
	bool	LeftMouseButton = Viewport->KeyState(EKeys::LeftMouseButton),
		MiddleMouseButton = Viewport->KeyState(EKeys::MiddleMouseButton),
		RightMouseButton = Viewport->KeyState(EKeys::RightMouseButton);

	bool bAltPressed = Viewport->KeyState(EKeys::LeftAlt) || Viewport->KeyState(EKeys::RightAlt);

	return  (MiddleMouseButton && !LeftMouseButton && !RightMouseButton) || (!bAltPressed && MiddleMouseButton );
}

static bool IsOrbitZoomMode( FViewport* Viewport )
{	
	bool	LeftMouseButton = Viewport->KeyState(EKeys::LeftMouseButton),
		MiddleMouseButton = Viewport->KeyState(EKeys::MiddleMouseButton),
		RightMouseButton = Viewport->KeyState(EKeys::RightMouseButton);

	 return RightMouseButton || (LeftMouseButton && MiddleMouseButton);
}


void FEditorViewportClient::InputAxisForOrbit(FViewport* InViewport, const FVector& DragDelta, FVector& Drag, FRotator& Rot)
{
	// Ensure orbit is enabled
	const bool bEnable=true;
	ToggleOrbitCamera(bEnable);

	FRotator TempRot = GetViewRotation();

	SetViewRotation( FRotator(0,90,0) );
	ConvertMovementToOrbitDragRot(DragDelta, Drag, Rot);
	SetViewRotation( TempRot );

	Drag.X = DragDelta.X;

	bool	LeftMouseButton = InViewport->KeyState(EKeys::LeftMouseButton),
		MiddleMouseButton = InViewport->KeyState(EKeys::MiddleMouseButton),
		RightMouseButton = InViewport->KeyState(EKeys::RightMouseButton);

	if ( IsOrbitRotationMode( InViewport ) )
	{
		SetViewRotation( GetViewRotation() + FRotator( Rot.Pitch, -Rot.Yaw, Rot.Roll ) );
	}
	else if ( IsOrbitPanMode( InViewport ) )
	{
		FVector DeltaLocation = FVector(-Drag.X, 0, Drag.Z);

		FVector LookAt = ViewTransform.GetLookAt();

		FMatrix RotMat =
			FTranslationMatrix( -LookAt ) *
			FRotationMatrix( FRotator(0,GetViewRotation().Yaw,0) ) * 
			FRotationMatrix( FRotator(0, 0, GetViewRotation().Pitch));

		FVector TransformedDelta = RotMat.Inverse().TransformVector(DeltaLocation);

		SetViewLocation( GetViewLocation() + TransformedDelta );
		SetLookAtLocation( GetLookAtLocation() + TransformedDelta );
	}
	else if ( IsOrbitZoomMode( InViewport ) )
	{
		FMatrix OrbitMatrix = ViewTransform.ComputeOrbitMatrix().Inverse();

		FVector DeltaLocation = FVector(0, Drag.X+ -Drag.Y, 0);

		FVector LookAt = ViewTransform.GetLookAt();

		// Orient the delta down the view direction towards the look at
		FMatrix RotMat =
			FTranslationMatrix( -LookAt ) *
			FRotationMatrix( FRotator(0,GetViewRotation().Yaw,0) ) * 
			FRotationMatrix( FRotator(0, 0, GetViewRotation().Pitch));

		FVector TransformedDelta = RotMat.Inverse().TransformVector(DeltaLocation);

		SetViewLocation( OrbitMatrix.GetOrigin() + TransformedDelta );
	}
}

/**
 * forces a cursor update and marks the window as a move has occurred
 */
void FEditorViewportClient::MarkMouseMovedSinceClick()
{
	if (!bHasMouseMovedSinceClick )
	{
		bHasMouseMovedSinceClick = true;
		//if we care about the cursor
		if (Viewport->IsCursorVisible() && Viewport->HasMouseCapture())
		{
			//force a refresh
			Viewport->UpdateMouseCursor(true);
		}
	}
}

/** Determines whether this viewport is currently allowed to use Absolute Movement */
bool FEditorViewportClient::IsUsingAbsoluteTranslation() const
{
	bool bIsHotKeyAxisLocked = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
	bool bCameraLockedToWidget = Viewport->KeyState(EKeys::LeftShift) || Viewport->KeyState(EKeys::RightShift);
	// Screen-space movement must always use absolute translation
	bool bScreenSpaceTransformation = Widget && (Widget->GetCurrentAxis() == EAxisList::Screen);
	bool bAbsoluteMovementEnabled = GetDefault<ULevelEditorViewportSettings>()->bUseAbsoluteTranslation || bScreenSpaceTransformation;
	bool bCurrentWidgetSupportsAbsoluteMovement = FWidget::AllowsAbsoluteTranslationMovement( GetWidgetMode() ) || bScreenSpaceTransformation;
	bool bWidgetActivelyTrackingAbsoluteMovement = Widget && (Widget->GetCurrentAxis() != EAxisList::None);

	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton);
	const bool MiddleMouseButtonDown = Viewport->KeyState(EKeys::MiddleMouseButton);
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton);

	const bool bAnyMouseButtonsDown = (LeftMouseButtonDown || MiddleMouseButtonDown || RightMouseButtonDown);

	return (!bCameraLockedToWidget && !bIsHotKeyAxisLocked && bAbsoluteMovementEnabled && bCurrentWidgetSupportsAbsoluteMovement && bWidgetActivelyTrackingAbsoluteMovement && !IsOrtho() && bAnyMouseButtonsDown);
}

void FEditorViewportClient::SetMatineeRecordingWindow (IMatineeBase* InInterpEd)
{
	RecordingInterpEd = InInterpEd;
	if (CameraController)
	{
		FCameraControllerConfig Config = CameraController->GetConfig();
		RecordingInterpEd->LoadRecordingSettings(OUT Config);
		CameraController->SetConfig(Config);
	}
}

bool FEditorViewportClient::IsFlightCameraActive() const
{
	bool bIsFlightMovementKey = 
		( Viewport->KeyState( EKeys::W )
		|| Viewport->KeyState( EKeys::S )
		|| Viewport->KeyState( EKeys::A )
		|| Viewport->KeyState( EKeys::D )
		|| Viewport->KeyState( EKeys::E )
		|| Viewport->KeyState( EKeys::Q )
		|| Viewport->KeyState( EKeys::Z )
		|| Viewport->KeyState( EKeys::C ) );

	// Movement key pressed and automatic movement enabled
	bIsFlightMovementKey &= GetDefault<ULevelEditorViewportSettings>()->FlightCameraControlType == WASD_Always;

	// Not using automatic movement but the flight camera is active
	bIsFlightMovementKey |= IsFlightCameraInputModeActive() && (GetDefault<ULevelEditorViewportSettings>()->FlightCameraControlType == WASD_RMBOnly );

	return
		!(Viewport->KeyState( EKeys::LeftControl ) || Viewport->KeyState( EKeys::RightControl ) ) &&
		!(Viewport->KeyState( EKeys::LeftShift ) || Viewport->KeyState( EKeys::RightShift ) ) &&
		!(Viewport->KeyState( EKeys::LeftAlt ) || Viewport->KeyState( EKeys::RightAlt ) ) &&
		bIsFlightMovementKey;
}


bool FEditorViewportClient::InputKey(FViewport* InViewport, int32 ControllerId, FKey Key, EInputEvent Event, float/*AmountDepressed*/, bool/*Gamepad*/)
{
	if (bDisableInput)
	{
		return true;
	}

	const int32	HitX = InViewport->GetMouseX();
	const int32	HitY = InViewport->GetMouseY();

	FCachedJoystickState* JoystickState = GetJoystickState(ControllerId);
	if (JoystickState)
	{
		JoystickState->KeyEventValues.Add(Key, Event);
	}

	FInputEventState InputState( InViewport, Key, Event );

	const bool bWasCursorVisible = InViewport->IsCursorVisible();
	const bool bWasSoftwareCursorVisible = InViewport->IsSoftwareCursorVisible();

	const bool AltDown = InputState.IsAltButtonPressed();
	const bool ShiftDown = InputState.IsShiftButtonPressed();
	const bool ControlDown = InputState.IsCtrlButtonPressed();

	RequiredCursorVisibiltyAndAppearance.bDontResetCursor = false;
	UpdateRequiredCursorVisibility();

	bool bHandled = false;

	if( bWasCursorVisible != RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible || bWasSoftwareCursorVisible != RequiredCursorVisibiltyAndAppearance.bSoftwareCursorVisible )
	{
		bHandled = true;	
	}

	
	// Compute a view.
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		InViewport,
		GetScene(),
		EngineShowFlags )
		.SetRealtimeUpdate( IsRealtime() ) );
	FSceneView* View = CalcSceneView( &ViewFamily );


	if (!InputState.IsAnyMouseButtonDown())
	{
		bHasMouseMovedSinceClick = false;
	}


	// Start tracking if any mouse button is down and it was a tracking event (MouseButton/Ctrl/Shift/Alt):
	if ( InputState.IsAnyMouseButtonDown() 
		&& (Event == IE_Pressed || Event == IE_Released)
		&& (InputState.IsMouseButtonEvent() || InputState.IsCtrlButtonEvent() || InputState.IsAltButtonEvent() || InputState.IsShiftButtonEvent() ) )		
	{
		StartTrackingDueToInput( InputState, *View );
		return true;
	}

	
	// If we are tracking and no mouse button is down and this input event released the mouse button stop tracking and process any clicks if necessary
	if ( bIsTracking && !InputState.IsAnyMouseButtonDown() && InputState.IsMouseButtonEvent() )
	{
		// Handle possible mouse click viewport
		ProcessClickInViewport( InputState, *View );

		// Stop tracking if no mouse button is down
		StopTracking();

		bHandled |= true;
	}


	if ( Event == IE_DoubleClick )
	{
		ProcessDoubleClickInViewport( InputState, *View );
		return true;
	}

	if( ( Key == EKeys::MouseScrollUp || Key == EKeys::MouseScrollDown || Key == EKeys::Add || Key == EKeys::Subtract ) && (Event == IE_Pressed || Event == IE_Repeat ) && IsOrtho() )
	{
		OnOrthoZoom( InputState );
		bHandled |= true;
	}
	else if( ( Key == EKeys::MouseScrollUp || Key == EKeys::MouseScrollDown ) && Event == IE_Pressed && IsPerspective() )
	{
		// If flight camera input is active, then the mouse wheel will control the speed of camera
		// movement
		if( IsFlightCameraInputModeActive() )
		{
			OnChangeCameraSpeed( InputState );
		}
		else
		{
			OnDollyPerspectiveCamera( InputState );
		}

		bHandled |= true;
	}
	else if( IsFlightCameraActive() )
	{
		// Flight camera control is active, so simply absorb the key.  The camera will update based
		// on currently pressed keys (Viewport->KeyState) in the Tick function.

		//mark "externally moved" so context menu doesn't come up
		MouseDeltaTracker->SetExternalMovement();
		bHandled |= true;
	}
	
	//apply the visibility and set the cursor positions
	ApplyRequiredCursorVisibility( true );
	return bHandled;
}


void FEditorViewportClient::StopTracking()
{
	if( bIsTracking )
	{
		//cache the axis of any widget we might be moving
		EAxisList::Type DraggingAxis = EAxisList::None;
		if( Widget != NULL )
		{
			DraggingAxis = Widget->GetCurrentAxis();
		}

		MouseDeltaTracker->EndTracking( this );

		Widget->SetCurrentAxis( EAxisList::None );
		Invalidate( true, true );

	/*	if ( !MouseDeltaTracker->UsingDragTool() )
		{
			Widget->SetCurrentAxis( AXIS_None );
			Invalidate( true, true );
		}*/

		SetRequiredCursorOverride( false );

		bWidgetAxisControlledByDrag = false;
		bIsTracking = false;	


	}

	bHasMouseMovedSinceClick = false;
}

bool FEditorViewportClient::IsInImmersiveViewport() const
{
	return ImmersiveDelegate.IsBound() ? ImmersiveDelegate.Execute() : false ;
}

void FEditorViewportClient::StartTrackingDueToInput( const struct FInputEventState& InputState, FSceneView& View )
{
	// Check to see if the current event is a modifier key and that key was already in the
	// same state.
	EInputEvent Event = InputState.GetInputEvent();
	FViewport* InputStateViewport = InputState.GetViewport();
	FKey Key = InputState.GetKey();

	bool bIsRedundantModifierEvent = 
		( InputState.IsAltButtonEvent() && ( ( Event != IE_Released ) == IsAltPressed() ) ) ||
		( InputState.IsCtrlButtonEvent() && ( ( Event != IE_Released ) == IsCtrlPressed() ) ) ||
		( InputState.IsShiftButtonEvent() && ( ( Event != IE_Released ) == IsShiftPressed() ) );

	if( MouseDeltaTracker->UsingDragTool() && InputState.IsLeftMouseButtonPressed() && Event != IE_Released )
	{
		bIsRedundantModifierEvent = true;
	}

	const int32	HitX = InputStateViewport->GetMouseX();
	const int32	HitY = InputStateViewport->GetMouseY();


	//First mouse down, note where they clicked
	LastMouseX = HitX;
	LastMouseY = HitY;

	// Only start (or restart) tracking mode if the current event wasn't a modifier key that
	// was already pressed or released.
	if( !bIsRedundantModifierEvent )
	{
		const bool bWasTracking = bIsTracking;

		// Stop current tracking
		if ( bIsTracking )
		{
			MouseDeltaTracker->EndTracking( this );
			bIsTracking = false;
		}

		bDraggingByHandle = (Widget->GetCurrentAxis() != EAxisList::None);

		if( Event == IE_Pressed )
		{
			// Tracking initialization:
			GEditor->MouseMovement = FVector::ZeroVector;
		}

		// Start new tracking. Potentially reset the widget so that StartTracking can pick a new axis.
		if ( !bDraggingByHandle || InputState.IsCtrlButtonPressed() ) 
		{
			bWidgetAxisControlledByDrag = false;
			Widget->SetCurrentAxis( EAxisList::None );
		}
		const bool bNudge = false;
		MouseDeltaTracker->StartTracking( this, HitX, HitY, InputState, bNudge, !bWasTracking );
		bIsTracking = true;

		//if we are using a widget to drag by axis ensure the cursor is correct
		if( bDraggingByHandle == true )
		{
			//reset the flag to say we used a drag modifier	if we are using the widget handle
			if( bWidgetAxisControlledByDrag == false )
			{
				MouseDeltaTracker->ResetUsedDragModifier();
			}

			SetRequiredCursorOverride( true , EMouseCursor::CardinalCross );
		}

		//only reset the initial point when the mouse is actually clicked
		if (InputState.IsAnyMouseButtonDown() && Widget)
		{
			Widget->ResetInitialTranslationOffset();				
		}

		//Don't update the cursor visibility if we don't have focus or mouse capture 
		if( InputStateViewport->HasFocus() ||  InputStateViewport->HasMouseCapture())
		{
			//Need to call this one more time as the axis variable for the widget has just been updated
			UpdateRequiredCursorVisibility();
		}
	}
	ApplyRequiredCursorVisibility( true );
}



void FEditorViewportClient::ProcessClickInViewport( const FInputEventState& InputState, FSceneView& View )
{
	// Ignore actor manipulation if we're using a tool
	if ( !MouseDeltaTracker->UsingDragTool() )
	{
		EInputEvent Event = InputState.GetInputEvent();
		FViewport* InputStateViewport = InputState.GetViewport();
		FKey Key = InputState.GetKey();

		const int32	HitX = InputStateViewport->GetMouseX();
		const int32	HitY = InputStateViewport->GetMouseY();
		
		FIntPoint CurrentMousePos(HitX,HitY);

		// Calc the raw delta from the mouse to detect if there was any movement
		FVector RawMouseDelta = MouseDeltaTracker->GetScreenDelta();

		// Note: We are using raw mouse movement to double check distance moved in low performance situations.  In low performance situations its possible 
		// that we would get a mouse down and a mouse up before the next tick where GEditor->MouseMovment has not been updated.  
		// In that situation, legitimate drags are incorrectly considered clicks
		bool bNoMouseMovment = RawMouseDelta.SizeSquared() < MOUSE_CLICK_DRAG_DELTA && GEditor->MouseMovement.SizeSquared() < MOUSE_CLICK_DRAG_DELTA;

		// If the mouse haven't moved too far, treat the button release as a click.
		if( bNoMouseMovment && !MouseDeltaTracker->WasExternalMovement() )
		{
			HHitProxy* HitProxy = InputStateViewport->GetHitProxy(HitX,HitY);

			// When clicking, the cursor should always appear at the location of the click and not move out from undere the user
			InputStateViewport->SetPreCaptureMousePosFromSlateCursor();
			ProcessClick(View,HitProxy,Key,Event,HitX,HitY);
		}
	}

}

bool FEditorViewportClient::IsAltPressed() const
{
	return Viewport->KeyState(EKeys::LeftAlt) || Viewport->KeyState(EKeys::RightAlt);
}

bool FEditorViewportClient::IsCtrlPressed() const
{
	return Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
}

bool FEditorViewportClient::IsShiftPressed() const
{
	return Viewport->KeyState(EKeys::LeftShift) || Viewport->KeyState(EKeys::RightShift);
}


void FEditorViewportClient::ProcessDoubleClickInViewport( const struct FInputEventState& InputState, FSceneView& View )
{
	// Stop current tracking
	if ( bIsTracking )
	{
		MouseDeltaTracker->EndTracking( this );
		bIsTracking = false;
	}

	FViewport* InputStateViewport = InputState.GetViewport();
	EInputEvent Event = InputState.GetInputEvent();
	FKey Key = InputState.GetKey();

	const int32	HitX = InputStateViewport->GetMouseX();
	const int32	HitY = InputStateViewport->GetMouseY();

	MouseDeltaTracker->StartTracking( this, HitX, HitY, InputState );
	bIsTracking = true;
	GEditor->MouseMovement = FVector::ZeroVector;
	HHitProxy*	HitProxy = InputStateViewport->GetHitProxy(HitX,HitY);
	ProcessClick(View,HitProxy,Key,Event,HitX,HitY);
	MouseDeltaTracker->EndTracking( this );
	bIsTracking = false;

	// This needs to be set to false to allow the axes to update
	bWidgetAxisControlledByDrag = false;
	MouseDeltaTracker->ResetUsedDragModifier();
	RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible = true;
	RequiredCursorVisibiltyAndAppearance.bSoftwareCursorVisible = false;
	ApplyRequiredCursorVisibility();
}


/** Determines if the new MoveCanvas movement should be used 
 * @return - true if we should use the new drag canvas movement.  Returns false for combined object-camera movement and marquee selection
 */
bool FEditorViewportClient::ShouldUseMoveCanvasMovement() const
{
	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton) ? true : false;
	const bool MiddleMouseButtonDown = Viewport->KeyState(EKeys::MiddleMouseButton) ? true : false;
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton) ? true : false;
	const bool bMouseButtonDown = (LeftMouseButtonDown || MiddleMouseButtonDown || RightMouseButtonDown );

	const bool AltDown = IsAltPressed();
	const bool ShiftDown = IsShiftPressed();
	const bool ControlDown = IsCtrlPressed();

	//if we're using the new move canvas mode, we're in an ortho viewport, and the mouse is down
	if (GetDefault<ULevelEditorViewportSettings>()->bPanMovesCanvas && IsOrtho() && bMouseButtonDown)
	{
		//MOVING CAMERA
		if ( !MouseDeltaTracker->UsingDragTool() && AltDown == false && ShiftDown == false && ControlDown == false && (Widget->GetCurrentAxis() == EAxisList::None) && (LeftMouseButtonDown ^ RightMouseButtonDown))
		{
			return true;
		}

		//OBJECT MOVEMENT CODE
		if ( ( AltDown == false && ShiftDown == false && ( LeftMouseButtonDown ^ RightMouseButtonDown ) ) && 
			( ( GetWidgetMode() == FWidget::WM_Translate && Widget->GetCurrentAxis() != EAxisList::None ) ||
			( GetWidgetMode() == FWidget::WM_TranslateRotateZ && Widget->GetCurrentAxis() != EAxisList::ZRotation &&  Widget->GetCurrentAxis() != EAxisList::None ) ) )
		{
			return true;
		}


		//ALL other cases hide the mouse
		return false;
	}
	else
	{
		//current system - do not show cursor when mouse is down
		return false;
	}
}

void FEditorViewportClient::DrawAxes(FViewport* InViewport, FCanvas* Canvas, const FRotator* InRotation, EAxisList::Type InAxis)
{
	FMatrix ViewTM = FMatrix::Identity;
	if ( bUsingOrbitCamera)
	{
		ViewTM = FRotationMatrix( ViewTransform.ComputeOrbitMatrix().Inverse().Rotator() );
	}
	else
	{
		ViewTM = FRotationMatrix( GetViewRotation() );
	}


	if( InRotation )
	{
		ViewTM = FRotationMatrix( *InRotation );
	}

	const int32 SizeX = InViewport->GetSizeXY().X;
	const int32 SizeY = InViewport->GetSizeXY().Y;

	const FIntPoint AxisOrigin( 30, SizeY - 30 );
	const float AxisSize = 25.f;

	UFont* Font = GEngine->GetSmallFont();
	int32 XL, YL;
	StringSize(Font, XL, YL, TEXT("Z"));

	FVector AxisVec;
	FIntPoint AxisEnd;
	FCanvasLineItem LineItem;
	FCanvasTextItem TextItem( FVector2D::ZeroVector, FText::GetEmpty(), Font, FLinearColor::White );
	if( ( InAxis & EAxisList::X ) ==  EAxisList::X)
	{
		AxisVec = AxisSize * ViewTM.InverseTransformVector( FVector(1,0,0) );
		AxisEnd = AxisOrigin + FIntPoint( AxisVec.Y, -AxisVec.Z );
		LineItem.SetColor( FLinearColor::Red );
		TextItem.SetColor( FLinearColor::Red );
		LineItem.Draw( Canvas, AxisOrigin, AxisEnd );
		TextItem.Text = LOCTEXT("XAxis","X");
		TextItem.Draw( Canvas, FVector2D(AxisEnd.X + 2, AxisEnd.Y - 0.5*YL) );
	}

	if( ( InAxis & EAxisList::Y ) == EAxisList::Y)
	{
		AxisVec = AxisSize * ViewTM.InverseTransformVector( FVector(0,1,0) );
		AxisEnd = AxisOrigin + FIntPoint( AxisVec.Y, -AxisVec.Z );
		LineItem.SetColor( FLinearColor::Green );
		TextItem.SetColor( FLinearColor::Green );
		LineItem.Draw( Canvas, AxisOrigin, AxisEnd );
		TextItem.Text = LOCTEXT("YAxis","Y");
		TextItem.Draw( Canvas, FVector2D(AxisEnd.X + 2, AxisEnd.Y - 0.5*YL) );
		
	}

	if( ( InAxis & EAxisList::Z ) == EAxisList::Z )
	{
		AxisVec = AxisSize * ViewTM.InverseTransformVector( FVector(0,0,1) );
		AxisEnd = AxisOrigin + FIntPoint( AxisVec.Y, -AxisVec.Z );
		LineItem.SetColor( FLinearColor::Blue );
		TextItem.SetColor( FLinearColor::Blue );
		LineItem.Draw( Canvas, AxisOrigin, AxisEnd );
		TextItem.Text = LOCTEXT("ZAxis","Z");
		TextItem.Draw( Canvas, FVector2D(AxisEnd.X + 2, AxisEnd.Y - 0.5*YL) );
	}
}

/** Convert the specified number (in cm or unreal units) into a readable string with relevant si units */
FString UnrealUnitsToSiUnits(float UnrealUnits)
{
	// Put it in mm to start off with
	UnrealUnits *= 10.f;

	const int32 OrderOfMagnitude = FMath::Trunc(FMath::LogX(10.0f, UnrealUnits));

	// Get an exponent applied to anything >= 1,000,000,000mm (1000km)
	const int32 Exponent = (OrderOfMagnitude - 6)  / 3;
	const FString ExponentString = Exponent > 0 ? FString::Printf(TEXT("e+%d"), Exponent*3) : TEXT("");

	float ScaledNumber = UnrealUnits;

	// Factor the order of magnitude into thousands and clamp it to km
	const int32 OrderOfThousands = OrderOfMagnitude / 3;
	if (OrderOfThousands != 0)
	{
		// Scale units to m or km (with the order of magnitude in 1000s)
		ScaledNumber /= FMath::Pow(1000.f, OrderOfThousands);
	}

	// Round to 2 S.F.
	const TCHAR* Approximation = TEXT("");
	{
		const int32 ScaledOrder = OrderOfMagnitude % (FMath::Max(OrderOfThousands, 1) * 3);
		const float RoundingDivisor = FMath::Pow(10.f, ScaledOrder) / 10.f;
		const int32 Rounded = FMath::Trunc(ScaledNumber / RoundingDivisor) * RoundingDivisor;
		if (ScaledNumber - Rounded > KINDA_SMALL_NUMBER)
		{
			ScaledNumber = Rounded;
			Approximation = TEXT("~");
		}
	}

	if (OrderOfMagnitude <= 2)
	{
		// Always show cm not mm
		ScaledNumber /= 10;
	}

	static const TCHAR* UnitText[] = { TEXT("cm"), TEXT("m"), TEXT("km") };
	if (FMath::Fmod(ScaledNumber, 1.f) > KINDA_SMALL_NUMBER)
	{
		return FString::Printf(TEXT("%s%.1f%s%s"), Approximation, ScaledNumber, *ExponentString, UnitText[FMath::Min(OrderOfThousands, 2)]);
	}
	else
	{
		return FString::Printf(TEXT("%s%d%s%s"), Approximation, FMath::Trunc(ScaledNumber), *ExponentString, UnitText[FMath::Min(OrderOfThousands, 2)]);
	}
}

void FEditorViewportClient::DrawScaleUnits(FViewport* InViewport, FCanvas* Canvas, const FSceneView& InView)
{
	const float UnitsPerPixel = GetOrthoUnitsPerPixel(InViewport);

	// Find the closest power of ten to our target width
	static const int32 ApproxTargetMarkerWidthPx = 100;
	const float SegmentWidthUnits = FMath::Pow(10.f, FMath::Round(FMath::LogX(10.f, UnitsPerPixel * ApproxTargetMarkerWidthPx)));

	const FString DisplayText = UnrealUnitsToSiUnits(SegmentWidthUnits);
	
	UFont* Font = GEngine->GetTinyFont();
	int32 TextWidth, TextHeight;
	StringSize(Font, TextWidth, TextHeight, *DisplayText);

	// Origin is the bottom left of the scale
	const FIntPoint StartPoint(80, InViewport->GetSizeXY().Y - 30);
	const FIntPoint EndPoint = StartPoint + FIntPoint(SegmentWidthUnits / UnitsPerPixel, 0);

	// Sort out the color for the text and widget
	FLinearColor HSVBackground = InView.BackgroundColor.LinearRGBToHSV();
	const int32 Sign = (0.5f - HSVBackground.B) / FMath::Abs(HSVBackground.B - 0.5f);
	HSVBackground.B = HSVBackground.B + Sign*0.4f;
	const FLinearColor SegmentColor = HSVBackground.HSVToLinearRGB();

	const FIntPoint VerticalTickOffset(0, -3);

	// Draw the scale
	FCanvasLineItem LineItem;
	LineItem.SetColor(SegmentColor);
	LineItem.Draw(Canvas, StartPoint, StartPoint + VerticalTickOffset);
	LineItem.Draw(Canvas, StartPoint, EndPoint);
	LineItem.Draw(Canvas, EndPoint, EndPoint + VerticalTickOffset);

	// Draw the text
	FCanvasTextItem TextItem(EndPoint + FIntPoint(-(TextWidth + 3), -TextHeight), FText::FromString(DisplayText), Font, SegmentColor);
	TextItem.Draw(Canvas);
}

void FEditorViewportClient::OnOrthoZoom( const struct FInputEventState& InputState, float Scale )
{
	FViewport* InputStateViewport = InputState.GetViewport();
	FKey Key = InputState.GetKey();

	// Scrolling the mousewheel up/down zooms the orthogonal viewport in/out.
	int32 Delta = 25 * Scale;
	if( Key == EKeys::MouseScrollUp || Key == EKeys::Add )
	{
		Delta *= -1;
	}

	//Extract current state
	int32 ViewportWidth = InputStateViewport->GetSizeXY().X;
	int32 ViewportHeight = InputStateViewport->GetSizeXY().Y;

	FVector OldOffsetFromCenter;

	const bool bCenterZoomAroundCursor = GetDefault<ULevelEditorViewportSettings>()->bCenterZoomAroundCursor && (Key == EKeys::MouseScrollDown || Key == EKeys::MouseScrollUp );

	if (bCenterZoomAroundCursor)
	{
		//Y is actually backwards, but since we're move the camera opposite the cursor to center, we negate both
		//therefore the x is negated
		//X Is backwards, negate it
		//default to viewport mouse position
		int32 CenterX = InputStateViewport->GetMouseX();
		int32 CenterY = InputStateViewport->GetMouseY();
		if (ShouldUseMoveCanvasMovement())
		{
			//use virtual mouse while dragging (normal mouse is clamped when invisible)
			CenterX = LastMouseX;
			CenterY = LastMouseY;
		}
		int32 DeltaFromCenterX = -(CenterX - (ViewportWidth>>1));
		int32 DeltaFromCenterY =  (CenterY - (ViewportHeight>>1));
		switch( GetViewportType() )
		{
		case LVT_OrthoXY:
			OldOffsetFromCenter.Set(DeltaFromCenterX, -DeltaFromCenterY, 0.0f);
			break;
		case LVT_OrthoXZ:
			OldOffsetFromCenter.Set(DeltaFromCenterX, 0.0f, DeltaFromCenterY);
			break;
		case LVT_OrthoYZ:
			OldOffsetFromCenter.Set(0.0f, DeltaFromCenterX, DeltaFromCenterY);
			break;
		case LVT_OrthoFreelook:
			//@TODO: CAMERA: How to handle this
			break;
		}
	}

	//save off old zoom
	const float OldUnitsPerPixel = GetOrthoUnitsPerPixel(Viewport);

	//update zoom based on input
	SetOrthoZoom( GetOrthoZoom() + (GetOrthoZoom() / CAMERA_ZOOM_DAMPEN) * Delta );
	SetOrthoZoom( FMath::Clamp<float>( GetOrthoZoom(), MIN_ORTHOZOOM, MAX_ORTHOZOOM ) );

	if (bCenterZoomAroundCursor)
	{
		//This is the equivalent to moving the viewport to center about the cursor, zooming, and moving it back a proportional amount towards the cursor
		FVector FinalDelta = (GetOrthoUnitsPerPixel(Viewport) - OldUnitsPerPixel)*OldOffsetFromCenter;

		//now move the view location proportionally
		SetViewLocation( GetViewLocation() + FinalDelta );
	}

	const bool bInvalidateViews = true;

	// Update linked ortho viewport movement based on updated zoom and view location, 
	UpdateLinkedOrthoViewports( bInvalidateViews );

	const bool bInvalidateHitProxies = true;

	Invalidate( bInvalidateViews, bInvalidateHitProxies );

	//mark "externally moved" so context menu doesn't come up
	MouseDeltaTracker->SetExternalMovement();
}

void FEditorViewportClient::OnDollyPerspectiveCamera( const FInputEventState& InputState )
{
	FKey Key = InputState.GetKey();

	// Scrolling the mousewheel up/down moves the perspective viewport forwards/backwards.
	FVector Drag(0,0,0);

	const FRotator& ViewRotation = GetViewRotation();
	Drag.X = FMath::Cos( ViewRotation.Yaw * PI / 180.f ) * FMath::Cos( ViewRotation.Pitch * PI / 180.f );
	Drag.Y = FMath::Sin( ViewRotation.Yaw * PI / 180.f ) * FMath::Cos( ViewRotation.Pitch * PI / 180.f );
	Drag.Z = FMath::Sin( ViewRotation.Pitch * PI / 180.f );

	if( Key == EKeys::MouseScrollDown )
	{
		Drag = -Drag;
	}

	const float CameraSpeed = GetCameraSpeed(GetDefault<ULevelEditorViewportSettings>()->MouseScrollCameraSpeed);
	Drag *= CameraSpeed * 32.f;

	const bool bDollyCamera = true;
	MoveViewportCamera( Drag, FRotator::ZeroRotator, bDollyCamera );
	Invalidate( true, true );

	FEditorDelegates::OnDollyPerspectiveCamera.Broadcast(Drag, ViewIndex);
}

void FEditorViewportClient::OnChangeCameraSpeed( const struct FInputEventState& InputState )
{
	const float MinCameraSpeedScale = 0.1f;
	const float MaxCameraSpeedScale = 10.0f;

	FKey Key = InputState.GetKey();

	// Adjust and clamp the camera speed scale
	if( Key == EKeys::MouseScrollUp )
	{
		if( FlightCameraSpeedScale >= 2.0f )
		{
			FlightCameraSpeedScale += 0.5f;
		}
		else if( FlightCameraSpeedScale >= 1.0f )
		{
			FlightCameraSpeedScale += 0.2f;
		}
		else
		{
			FlightCameraSpeedScale += 0.1f;
		}
	}
	else
	{
		if( FlightCameraSpeedScale > 2.49f )
		{
			FlightCameraSpeedScale -= 0.5f;
		}
		else if( FlightCameraSpeedScale >= 1.19f )
		{
			FlightCameraSpeedScale -= 0.2f;
		}
		else
		{
			FlightCameraSpeedScale -= 0.1f;
		}
	}

	FlightCameraSpeedScale = FMath::Clamp( FlightCameraSpeedScale, MinCameraSpeedScale, MaxCameraSpeedScale );

	if( FMath::IsNearlyEqual( FlightCameraSpeedScale, 1.0f, 0.01f ) )
	{
		// Snap to 1.0 if we're really close to that
		FlightCameraSpeedScale = 1.0f;
	}
}

void FEditorViewportClient::AddReferencedObjects( FReferenceCollector& Collector )
{
	if( PreviewScene )
	{
		PreviewScene->AddReferencedObjects( Collector );
	}
}

FSceneInterface* FEditorViewportClient::GetScene() const
{
	UWorld* World = GetWorld();
	if( World )
	{
		return World->Scene;
	}
	
	return NULL;
}

UWorld* FEditorViewportClient::GetWorld() const
{
	UWorld* OutWorldPtr = NULL;
	// If we have a valid scene get its world
	if( PreviewScene )
	{
		OutWorldPtr = PreviewScene->GetWorld();
	}
	if ( OutWorldPtr == NULL )
	{
		OutWorldPtr = GWorld;
	}
	return OutWorldPtr;
}


void FEditorViewportClient::SetupViewForRendering( FSceneViewFamily& ViewFamily, FSceneView& View )
{
	if(ViewFamily.EngineShowFlags.Wireframe)
	{
		// Wireframe color is emissive-only, and mesh-modifying materials do not use material substitution, hence...
		View.DiffuseOverrideParameter = FVector4(0.f, 0.f, 0.f, 0.f);
		View.SpecularOverrideParameter = FVector4(0.f, 0.f, 0.f, 0.f);
	}
	else if (ViewFamily.EngineShowFlags.OverrideDiffuseAndSpecular)
	{
		View.DiffuseOverrideParameter = FVector4(GEngine->LightingOnlyBrightness.R, GEngine->LightingOnlyBrightness.G, GEngine->LightingOnlyBrightness.B, 0.0f);
		View.SpecularOverrideParameter = FVector4(.1f, .1f, .1f, 0.0f);
	}
	else if( ViewFamily.EngineShowFlags.ReflectionOverride)
	{
		View.DiffuseOverrideParameter = FVector4(0.f, 0.f, 0.f, 0.f);
		View.SpecularOverrideParameter = FVector4(1, 1, 1, 0.0f);
		View.NormalOverrideParameter = FVector4(0, 0, 1, 0.0f);
		View.RoughnessOverrideParameter = FVector2D(0.0f, 0.0f);
	}

	if (!ViewFamily.EngineShowFlags.Diffuse)
	{
		View.DiffuseOverrideParameter = FVector4(0.f, 0.f, 0.f, 0.f);
	}

	if (!ViewFamily.EngineShowFlags.Specular)
	{
		View.SpecularOverrideParameter = FVector4(0.f, 0.f, 0.f, 0.f);
	}

	View.CurrentBufferVisualizationMode = CurrentBufferVisualizationMode;
}

void FEditorViewportClient::Draw(FViewport* InViewport, FCanvas* Canvas)
{
	FViewport* ViewportBackup = Viewport;
	Viewport = InViewport ? InViewport : Viewport;

	// Determine whether we should use world time or real time based on the scene.
	float TimeSeconds;
	float RealTimeSeconds;
	float DeltaTimeSeconds;

	UWorld* World = GWorld;
	if (( GetScene() != World->Scene) || (IsRealtime() == true))
	{
		// Use time relative to start time to avoid issues with float vs double
		TimeSeconds = GCurrentTime - GStartTime;
		RealTimeSeconds = GCurrentTime - GStartTime;
		DeltaTimeSeconds = GDeltaTime;
	}
	else
	{
		TimeSeconds = World->GetTimeSeconds();
		RealTimeSeconds = World->GetRealTimeSeconds();
		DeltaTimeSeconds = World->GetDeltaSeconds();
	}

	// Setup a FSceneViewFamily/FSceneView for the viewport.
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		Canvas->GetRenderTarget(),
		GetScene(),
		EngineShowFlags)
		.SetWorldTimes( TimeSeconds, DeltaTimeSeconds, RealTimeSeconds )
		.SetRealtimeUpdate( IsRealtime() ));

	ViewFamily.EngineShowFlags = EngineShowFlags;
	EngineShowFlagOverride(ESFIM_Editor, GetViewMode(), ViewFamily.EngineShowFlags, CurrentBufferVisualizationMode);
	EngineShowFlagOrthographicOverride(IsPerspective(), ViewFamily.EngineShowFlags);

	ViewFamily.ExposureSettings = ExposureSettings;

	FSceneView* View = CalcSceneView( &ViewFamily );

	SetupViewForRendering(ViewFamily,*View);

	if (IsAspectRatioConstrained())
	{
		// Clear the background to black if the aspect ratio is constrained, as the scene view won't write to all pixels.
		Canvas->Clear(FLinearColor::Black);
	}
	
	GetRendererModule().BeginRenderingViewFamily(Canvas,&ViewFamily);

	DrawCanvas( *Viewport, *View, *Canvas );

	if (EngineShowFlags.SafeFrames || bEnableSafeFrameOverride )
	{
		DrawSafeFrames(*Viewport, *View, *Canvas);
	}

	// Remove temporary debug lines.
	// Possibly a hack. Lines may get added without the scene being rendered etc.
	if (World->LineBatcher != NULL && (World->LineBatcher->BatchedLines.Num() || World->LineBatcher->BatchedPoints.Num()))
	{
		World->LineBatcher->Flush();
	}

	if (World->ForegroundLineBatcher != NULL && (World->ForegroundLineBatcher->BatchedLines.Num() || World->ForegroundLineBatcher->BatchedPoints.Num()))
	{
		World->ForegroundLineBatcher->Flush();
	}

	
	// Draw the widget.
	if (Widget)
	{
		Widget->DrawHUD( Canvas );
	}

	// Axes indicators
	if( bDrawAxes && !ViewFamily.EngineShowFlags.Game )
	{
		switch (GetViewportType())
		{
		case LVT_OrthoXY:
			{
				const FRotator XYRot(-90.0f, -90.0f, 0.0f);
				DrawAxes(Viewport, Canvas, &XYRot, EAxisList::XY);
				DrawScaleUnits(Viewport, Canvas, *View);
				break;
			}
		case LVT_OrthoXZ:
			{
				const FRotator XZRot(0.0f, -90.0f, 0.0f);
				DrawAxes(Viewport, Canvas, &XZRot, EAxisList::XZ);
				DrawScaleUnits(Viewport, Canvas, *View);
				break;
			}
		case LVT_OrthoYZ:
			{
				const FRotator YZRot(0.0f, 0.0f, 0.0f);
				DrawAxes(Viewport, Canvas, &YZRot, EAxisList::YZ);
				DrawScaleUnits(Viewport, Canvas, *View);
				break;
			}
		default:
			{
				DrawAxes(Viewport, Canvas);
				break;
			}
		}
	}

	UDebugDrawService::Draw(ViewFamily.EngineShowFlags, Viewport, View, Viewport->GetDebugCanvas());

	// Frame rate display
	int32 NextYPos = 75;
	if( IsRealtime() && ShouldShowFPS() )
	{
		const int32 XPos = FMath::Max( 10, (int32)Viewport->GetSizeXY().X - 90 );
		NextYPos = DrawFPSCounter( Viewport, Viewport->GetDebugCanvas(), XPos, NextYPos );
	}

	// Stats display
	if( IsRealtime() && ShouldShowStats() )
	{
		const int32 XPos = 4;
		TArray< FDebugDisplayProperty > EmptyPropertyArray;
		DrawStatsHUD( World, Viewport, Viewport->GetDebugCanvas(), NULL, EmptyPropertyArray, GetViewLocation(), GetViewRotation() );
	}

	if(!IsRealtime())
	{
		// Wait for the rendering thread to finish drawing the view before returning.
		// This reduces the apparent latency of dragging the viewport around.
		FlushRenderingCommands();
	}

	Viewport = ViewportBackup;
}

void FEditorViewportClient::Draw(const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	// Draw the drag tool.
	MouseDeltaTracker->Render3DDragTool( View, PDI );

	// Draw the widget.
	Widget->Render( View, PDI, this );

	if( bUsesDrawHelper )
	{
		DrawHelper.Draw( View, PDI );
	}

	// This viewport was just rendered, reset this value.
	FramesSinceLastDraw = 0;
}

FLinearColor FEditorViewportClient::GetBackgroundColor() const
{
	FLinearColor BackgroundColor = FColor(55, 55, 55);

	return BackgroundColor;
}

void FEditorViewportClient::SetCameraSetup(
	const FVector& LocationForOrbiting, 
	const FRotator& InOrbitRotation, 
	const FVector& InOrbitZoom, 
	const FVector& InOrbitLookAt, 
	const FVector& InViewLocation, 
	const FRotator &InViewRotation )
{
	if( bUsingOrbitCamera )
	{
		SetViewRotation( InOrbitRotation );
		SetViewLocation( InViewLocation + InOrbitZoom );
		SetLookAtLocation( InOrbitLookAt );
	}
	else
	{
		SetViewLocation( InViewLocation );
		SetViewRotation( InViewRotation );
	}

	
	// Save settings for toggling between orbit and unlocked camera
	DefaultOrbitLocation = InViewLocation;
	DefaultOrbitRotation = InOrbitRotation;
	DefaultOrbitZoom = InOrbitZoom;
	DefaultOrbitLookAt = InOrbitLookAt;
}

// Determines which axis InKey and InDelta most refer to and returns
// a corresponding FVector.  This vector represents the mouse movement
// translated into the viewports/widgets axis space.
//
// @param InNudge		If 1, this delta is coming from a keyboard nudge and not the mouse

FVector FEditorViewportClient::TranslateDelta( FKey InKey, float InDelta, bool InNudge )
{
	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton);
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton);
	const bool bIsUsingTrackpad = FSlateApplication::Get().IsUsingTrackpad();

	FVector vec(0.0f, 0.0f, 0.0f);

	float X = InKey == EKeys::MouseX ? InDelta : 0.f;
	float Y = InKey == EKeys::MouseY ? InDelta : 0.f;

	switch( GetViewportType() )
	{
	case LVT_OrthoXY:
	case LVT_OrthoXZ:
	case LVT_OrthoYZ:
		{
			LastMouseX += X;
			LastMouseY -= Y;

			if ((X != 0.0f) || (Y!=0.0f))
			{
				MarkMouseMovedSinceClick();
			}

			//only invert x,y if we're moving the camera
			if( ShouldUseMoveCanvasMovement() )
			{
				if(Widget->GetCurrentAxis() == EAxisList::None) 
				{
					X = -X;
					Y = -Y;
				}
			}

			//update the position
			Viewport->SetSoftwareCursorPosition( FVector2D( LastMouseX, LastMouseY ) );
			//UE_LOG(LogEditorViewport, Log, *FString::Printf( TEXT("can:%d %d") , LastMouseX , LastMouseY ));
			//change to grab hand
			SetRequiredCursorOverride( true , EMouseCursor::CardinalCross );
			//update and apply cursor visibility
			UpdateAndApplyCursorVisibility();

			FWidget::EWidgetMode WidgetMode = GetWidgetMode();
			bool bIgnoreOrthoScaling = (WidgetMode == FWidget::WM_Scale) && (Widget->GetCurrentAxis() != EAxisList::None);

			if( InNudge || bIgnoreOrthoScaling )
			{
				vec = FVector( X, Y, 0.f );
			}
			else
			{
				const float UnitsPerPixel = GetOrthoUnitsPerPixel(Viewport);
				vec = FVector( X * UnitsPerPixel, Y * UnitsPerPixel, 0.f );

				if( Widget->GetCurrentAxis() == EAxisList::None )
				{
					switch( GetViewportType() )
					{
					case LVT_OrthoXY:
						vec.Y *= -1.0f;
						break;
					case LVT_OrthoXZ:
						vec = FVector( X * UnitsPerPixel, 0.f, Y * UnitsPerPixel );
						break;
					case LVT_OrthoYZ:
						vec = FVector( 0.f, X * UnitsPerPixel, Y * UnitsPerPixel );
						break;
					}
				}
			}
		}
		break;

	case LVT_OrthoFreelook://@TODO: CAMERA: Not sure what to do here
	case LVT_Perspective:
		// Update the software cursor position
		Viewport->SetSoftwareCursorPosition( FVector2D(Viewport->GetMouseX() , Viewport->GetMouseY() ) );
		vec = FVector( X, Y, 0.f );
		break;

	default:
		check(0);		// Unknown viewport type
		break;
	}

	if( IsOrtho() && ((LeftMouseButtonDown || bIsUsingTrackpad) && RightMouseButtonDown) && Y != 0.f )
	{
		vec = FVector(0,0,Y);
	}

	return vec;
}

bool FEditorViewportClient::InputAxis(FViewport* InViewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	if (bDisableInput)
	{
		return true;
	}

	const bool bMouseButtonDown = InViewport->KeyState( EKeys::LeftMouseButton ) || InViewport->KeyState( EKeys::MiddleMouseButton ) || InViewport->KeyState( EKeys::RightMouseButton );
	const bool bLightMoveDown = InViewport->KeyState(EKeys::L);

	// Look at which axis is being dragged and by how much
	const float DragX = (Key == EKeys::MouseX) ? Delta : 0.f;
	const float DragY = (Key == EKeys::MouseY) ? Delta : 0.f;

	if( bLightMoveDown && bMouseButtonDown && PreviewScene )
	{
		FRotator LightDir = PreviewScene->GetLightDirection();

		LightDir.Yaw += -DragX * LightRotSpeed;
		LightDir.Pitch += -DragY * LightRotSpeed;

		PreviewScene->SetLightDirection( LightDir );
		Invalidate();
	}
	else
	{
		/**Save off axis commands for future camera work*/
		FCachedJoystickState* JoystickState = GetJoystickState(ControllerId);
		if (JoystickState)
		{
			JoystickState->AxisDeltaValues.Add(Key, Delta);
		}

		if( bIsTracking	)
		{
			// Accumulate and snap the mouse movement since the last mouse button click.
			MouseDeltaTracker->AddDelta( this, Key, Delta, 0 );
		}
	}

	// If we are using a drag tool, paint the viewport so we can see it update.
	if( MouseDeltaTracker->UsingDragTool() )
	{
		Invalidate( false, false );
	}

	return true;
}

static float AdjustGestureCameraRotation(float Delta, float AdjustLimit, float DeltaCutoff)
{
	const float AbsDelta = FMath::Abs(Delta);
	const float Scale = AbsDelta * (1.0f / AdjustLimit);
	if (AbsDelta > 0.0f && AbsDelta <= AdjustLimit)
	{
		return Delta * Scale;
	}
	const bool bIsUsingTrackpad = FSlateApplication::Get().IsUsingTrackpad();
	return bIsUsingTrackpad ? Delta : FMath::Clamp(Delta, -DeltaCutoff, DeltaCutoff);
}

bool FEditorViewportClient::InputGesture(FViewport* InViewport, EGestureEvent::Type GestureType, const FVector2D& GestureDelta)
{
	if (bDisableInput)
	{
		return true;
	}

	const FRotator& ViewRotation = GetViewRotation();

	const bool LeftMouseButtonDown = InViewport->KeyState(EKeys::LeftMouseButton);

	const ELevelViewportType ViewportType = GetViewportType();

	const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();

	switch( ViewportType )
	{
	case LVT_OrthoXY:
	case LVT_OrthoXZ:
	case LVT_OrthoYZ:
		{
			if (GestureType == EGestureEvent::Scroll)
			{
				const float UnitsPerPixel = GetOrthoUnitsPerPixel(Viewport);
				
				// GestureDelta is in window pixel coords.  Adjust for ortho units.
				FVector2D AdjustedGestureDelta = GestureDelta * UnitsPerPixel;
		
				switch( ViewportType )
				{
					case LVT_OrthoXY:
						CurrentGestureDragDelta += FVector(-AdjustedGestureDelta.X, -AdjustedGestureDelta.Y, 0);
						break;
					case LVT_OrthoXZ:
						CurrentGestureDragDelta += FVector(-AdjustedGestureDelta.X, 0, AdjustedGestureDelta.Y);
						break;
					case LVT_OrthoYZ:
						CurrentGestureDragDelta += FVector(0, -AdjustedGestureDelta.X, AdjustedGestureDelta.Y);
						break;
				}
			}
			else if (GestureType == EGestureEvent::Magnify)
			{
				OnOrthoZoom(FInputEventState(InViewport, EKeys::MouseScrollDown, IE_Released), -10.0f * GestureDelta.X);
			}
		}
		break;

	case LVT_Perspective:
	case LVT_OrthoFreelook:
		{
			if (GestureType == EGestureEvent::Scroll)
			{
				if( LeftMouseButtonDown )
				{
					// Pan left/right/up/down
					
					CurrentGestureDragDelta.X += GestureDelta.X * -FMath::Sin( ViewRotation.Yaw * PI / 180.f );
					CurrentGestureDragDelta.Y += GestureDelta.X *  FMath::Cos( ViewRotation.Yaw * PI / 180.f );
					CurrentGestureDragDelta.Z += -GestureDelta.Y;
				}
				else
				{
					// Change viewing angle

					CurrentGestureRotDelta.Yaw += AdjustGestureCameraRotation( GestureDelta.X, 20.0f, 35.0f ) * -0.35f;
					CurrentGestureRotDelta.Pitch += AdjustGestureCameraRotation( GestureDelta.Y, 20.0f, 35.0f ) * 0.35f;
				}
			}
		}
		break;

	default:
		// Not a 3D viewport receiving this gesture.  Could be a canvas window.  Bail out.
		return false;
	}

	//mark "externally moved" so context menu doesn't come up
	MouseDeltaTracker->SetExternalMovement();

	return true;
}

void FEditorViewportClient::UpdateGestureDelta()
{
	if( CurrentGestureDragDelta != FVector::ZeroVector || CurrentGestureRotDelta != FRotator::ZeroRotator )
	{
		MoveViewportCamera( CurrentGestureDragDelta, CurrentGestureRotDelta, false );
		
		Invalidate();
		
		CurrentGestureDragDelta = FVector::ZeroVector;
		CurrentGestureRotDelta = FRotator::ZeroRotator;
	}
}

// Converts a generic movement delta into drag/rotation deltas based on the viewport and keys held down

void FEditorViewportClient::ConvertMovementToDragRot(const FVector& InDelta,
													 FVector& InDragDelta,
													 FRotator& InRotDelta) const
{
	const FRotator& ViewRotation = GetViewRotation();

	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton);
	const bool MiddleMouseButtonDown = Viewport->KeyState(EKeys::MiddleMouseButton);
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton);
	const bool bIsUsingTrackpad = FSlateApplication::Get().IsUsingTrackpad();

	InDragDelta = FVector::ZeroVector;
	InRotDelta = FRotator::ZeroRotator;

	switch( GetViewportType() )
	{
	case LVT_OrthoXY:
	case LVT_OrthoXZ:
	case LVT_OrthoYZ:
		{
			if( ( LeftMouseButtonDown || bIsUsingTrackpad ) && RightMouseButtonDown )
			{
				// Both mouse buttons change the ortho viewport zoom.
				InDragDelta = FVector(0,0,InDelta.Z);
			}
			else if( RightMouseButtonDown )
			{
				// @todo: set RMB to move opposite to the direction of drag, in other words "grab and pull".
				InDragDelta = InDelta;
			}
			else if( LeftMouseButtonDown )
			{
				// LMB moves in the direction of the drag.
				InDragDelta = InDelta;
			}
		}
		break;

	case LVT_Perspective:
	case LVT_OrthoFreelook:
		{
			const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();

			if( LeftMouseButtonDown && !RightMouseButtonDown )
			{
				// Move forward and yaw

				InDragDelta.X = InDelta.Y * FMath::Cos( ViewRotation.Yaw * PI / 180.f );
				InDragDelta.Y = InDelta.Y * FMath::Sin( ViewRotation.Yaw * PI / 180.f );

				InRotDelta.Yaw = InDelta.X * ViewportSettings->MouseSensitivty;
			}
			else if( MiddleMouseButtonDown || bIsUsingTrackpad || ( ( LeftMouseButtonDown || bIsUsingTrackpad ) && RightMouseButtonDown ) )
			{
				// Pan left/right/up/down

				InDragDelta.X = InDelta.X * -FMath::Sin( ViewRotation.Yaw * PI / 180.f );
				InDragDelta.Y = InDelta.X *  FMath::Cos( ViewRotation.Yaw * PI / 180.f );
				InDragDelta.Z = InDelta.Y;
			}
			else if( RightMouseButtonDown && !LeftMouseButtonDown )
			{
				// Change viewing angle

				InRotDelta.Yaw = InDelta.X * ViewportSettings->MouseSensitivty;
				InRotDelta.Pitch = InDelta.Y * ViewportSettings->MouseSensitivty;
			}
		}
		break;

	default:
		check(0);	// unknown viewport type
		break;
	}
}

void FEditorViewportClient::ConvertMovementToOrbitDragRot(const FVector& InDelta,
																 FVector& InDragDelta,
																 FRotator& InRotDelta) const
{
	const FRotator& ViewRotation = GetViewRotation();

	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton);
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton);
	const bool MiddleMouseButtonDown = Viewport->KeyState(EKeys::MiddleMouseButton);
	const bool bIsUsingTrackpad = FSlateApplication::Get().IsUsingTrackpad();

	InDragDelta = FVector::ZeroVector;
	InRotDelta = FRotator::ZeroRotator;

	const float YawRadians = FMath::DegreesToRadians( ViewRotation.Yaw );

	switch( GetViewportType() )
	{
	case LVT_OrthoXY:
	case LVT_OrthoXZ:
	case LVT_OrthoYZ:
		{
			if( ( LeftMouseButtonDown || bIsUsingTrackpad ) && RightMouseButtonDown )
			{
				// Change ortho zoom.
				InDragDelta = FVector(0,0,InDelta.Z);
			}
			else if( RightMouseButtonDown )
			{
				// Move camera.
				InDragDelta = InDelta;
			}
			else if( LeftMouseButtonDown )
			{
				// Move actors.
				InDragDelta = InDelta;
			}
		}
		break;

	case LVT_Perspective:
		{
			const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();

			if( IsOrbitRotationMode( Viewport ) )
			{
				// Change the viewing angle
				InRotDelta.Yaw = InDelta.X * ViewportSettings->MouseSensitivty;
				InRotDelta.Pitch = InDelta.Y * ViewportSettings->MouseSensitivty;
			}
			else if( IsOrbitPanMode( Viewport ) )
			{
				// Pan left/right/up/down
				InDragDelta.X = InDelta.X * -FMath::Sin( YawRadians );
				InDragDelta.Y = InDelta.X *  FMath::Cos( YawRadians );
				InDragDelta.Z = InDelta.Y;
			}
			else if( IsOrbitZoomMode( Viewport ) )
			{
				// Zoom in and out.
				InDragDelta.X = InDelta.Y * FMath::Cos( YawRadians );
				InDragDelta.Y = InDelta.Y* FMath::Sin( YawRadians );
			}
		}
		break;

	default:
		check(0);	// unknown viewport type
		break;
	}
}

bool FEditorViewportClient::ShouldPanCamera() const
{
	const bool bIsCtrlDown = IsCtrlPressed();
	
	const bool bLeftMouseButtonDown = Viewport->KeyState( EKeys::LeftMouseButton );
	const bool bRightMouseButtonDown = Viewport->KeyState( EKeys::RightMouseButton );
	const bool bIsMarqueeSelect = false;

	const bool bOrthoRotateObjectMode = IsOrtho() && IsCtrlPressed() && bRightMouseButtonDown && !bLeftMouseButtonDown;
	// Pan the camera if not marquee selecting or the left and right mouse buttons are down
	return !bOrthoRotateObjectMode && !bIsCtrlDown && (!bIsMarqueeSelect || (bLeftMouseButtonDown && bRightMouseButtonDown) );
}

TSharedPtr<FDragTool> FEditorViewportClient::MakeDragTool(EDragTool::Type)
{
	return MakeShareable( new FDragTool );
}

bool FEditorViewportClient::ShouldOrbitCamera() const
{
	if( bCameraLock )
	{
		return true;
	}
	else
	{
		bool bDesireOrbit = false;

		if (!GetDefault<ULevelEditorViewportSettings>()->bUseUE3OrbitControls)
		{
			bDesireOrbit = IsAltPressed() && !IsCtrlPressed() && !IsShiftPressed();
		}
		else
		{
			bDesireOrbit = Viewport->KeyState(EKeys::U) || Viewport->KeyState(EKeys::L);
		}

		return bDesireOrbit && !IsFlightCameraInputModeActive() && !IsOrtho();
	}
}

/** Returns true if perspective flight camera input mode is currently active in this viewport */
bool FEditorViewportClient::IsFlightCameraInputModeActive() const
{
	if( (Viewport != NULL) && IsPerspective() )
	{
		if( CameraController != NULL )
		{
			const bool bLeftMouseButtonDown = Viewport->KeyState( EKeys::LeftMouseButton );
			const bool bMiddleMouseButtonDown = Viewport->KeyState( EKeys::MiddleMouseButton );
			const bool bRightMouseButtonDown = Viewport->KeyState( EKeys::RightMouseButton );
			const bool bIsUsingTrackpad = FSlateApplication::Get().IsUsingTrackpad();
			
			const bool bIsNonOrbitMiddleMouse = bMiddleMouseButtonDown && !IsAltPressed();

			const bool bIsMouseLooking =
				Widget->GetCurrentAxis() == EAxisList::None &&
				( bLeftMouseButtonDown || bMiddleMouseButtonDown || bRightMouseButtonDown || bIsUsingTrackpad ) &&
				!IsCtrlPressed() && !IsShiftPressed() && !IsAltPressed();

			return bIsMouseLooking;
		}
	}

	return false;
}

bool FEditorViewportClient::IsMovingCamera() const
{
	return bUsingOrbitCamera || IsFlightCameraActive();
}

/** True if the window is maximized or floating */
bool FEditorViewportClient::IsVisible() const
{
	bool bIsVisible = false;

	if( VisibilityDelegate.IsBound() )
	{
		// Call the visibility delegate to see if our parent viewport and layout configuration says we arevisible
		bIsVisible = VisibilityDelegate.Execute();
	}

	return bIsVisible;
}


void FEditorViewportClient::GetViewportDimensions( FIntPoint& OutOrigin, FIntPoint& Outize )
{
	OutOrigin = FIntPoint(0,0);
	if ( Viewport != NULL )
	{
		Outize.X = Viewport->GetSizeXY().X;
		Outize.Y = Viewport->GetSizeXY().Y;
	}
	else
	{
		Outize = FIntPoint(0,0);
	}
}

void FEditorViewportClient::UpdateAndApplyCursorVisibility()
{
	UpdateRequiredCursorVisibility();
	ApplyRequiredCursorVisibility();	
}

void FEditorViewportClient::UpdateRequiredCursorVisibility()
{
	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton) ? true : false;
	const bool MiddleMouseButtonDown = Viewport->KeyState(EKeys::MiddleMouseButton) ? true : false;
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton) ? true : false;
	const bool bMouseButtonDown = (LeftMouseButtonDown || MiddleMouseButtonDown || RightMouseButtonDown );
	const bool bIsUsingTrackpad = FSlateApplication::Get().IsUsingTrackpad();

	bool AltDown = IsAltPressed();
	bool ShiftDown = IsShiftPressed();
	bool ControlDown = IsCtrlPressed();

	if (GetViewportType() == LVT_None)
	{
		RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible = true;
		RequiredCursorVisibiltyAndAppearance.bSoftwareCursorVisible = false;
		return;
	}

	//if we're using the new move canvas mode, we're in an ortho viewport, and the mouse is down
	if (IsOrtho() && bMouseButtonDown && !MouseDeltaTracker->UsingDragTool())
	{
		//Translating an object, but NOT moving the camera AND the object (shift)
		if ( ( AltDown == false && ShiftDown == false && ( LeftMouseButtonDown ^ RightMouseButtonDown ) ) && 
			( ( GetWidgetMode() == FWidget::WM_Translate && Widget->GetCurrentAxis() != EAxisList::None ) ||
			(  GetWidgetMode() == FWidget::WM_TranslateRotateZ && Widget->GetCurrentAxis() != EAxisList::ZRotation &&  Widget->GetCurrentAxis() != EAxisList::None ) ) )
		{
			RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible = false;
			RequiredCursorVisibiltyAndAppearance.bSoftwareCursorVisible = true;								
			SetRequiredCursorOverride( true , EMouseCursor::CardinalCross );
			return;
		}

		if (GetDefault<ULevelEditorViewportSettings>()->bPanMovesCanvas)
		{
			bool bMovingCamera = RightMouseButtonDown && GetCurrentWidgetAxis() == EAxisList::None;
			bool bIsZoomingCamera = bMovingCamera && ( LeftMouseButtonDown || bIsUsingTrackpad ) && RightMouseButtonDown;
			//moving camera without  zooming
			if ( bMovingCamera && !bIsZoomingCamera )
			{
				// Always turn the hardware cursor on before turning the software cursor off
				// so the hardware cursor will be be set where the software cursor was
				RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible = !bHasMouseMovedSinceClick;
				RequiredCursorVisibiltyAndAppearance.bSoftwareCursorVisible = bHasMouseMovedSinceClick;
				SetRequiredCursorOverride( true , EMouseCursor::GrabHand );
				return;
			}
			RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible = false;
			RequiredCursorVisibiltyAndAppearance.bSoftwareCursorVisible = false;
			return;
		}
	}

	//if Absolute Translation and not just moving the camera around
	if (IsUsingAbsoluteTranslation() && !MouseDeltaTracker->UsingDragTool() )
	{
		//If we are dragging something we should hide the hardware cursor and show the s/w one
		RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible = false;
		RequiredCursorVisibiltyAndAppearance.bSoftwareCursorVisible = true;
		SetRequiredCursorOverride( true , EMouseCursor::CardinalCross );
	}
	else
	{
		// Calc the raw delta from the mouse since we started dragging to detect if there was any movement
		FVector RawMouseDelta = MouseDeltaTracker->GetScreenDelta();

		if (bMouseButtonDown && (RawMouseDelta.SizeSquared() >= MOUSE_CLICK_DRAG_DELTA || IsFlightCameraActive()) && !MouseDeltaTracker->UsingDragTool())
		{
			//current system - do not show cursor when mouse is down
			RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible = false;
			RequiredCursorVisibiltyAndAppearance.bSoftwareCursorVisible = false;
			return;
		}

		if( MouseDeltaTracker->UsingDragTool() )
		{
			RequiredCursorVisibiltyAndAppearance.bOverrideAppearance = false;
		}

		RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible = true;
		RequiredCursorVisibiltyAndAppearance.bSoftwareCursorVisible = false;
	}
}

void FEditorViewportClient::ApplyRequiredCursorVisibility( bool bUpdateSoftwareCursorPostion )
{
	if( RequiredCursorVisibiltyAndAppearance.bDontResetCursor == true )
	{
		Viewport->SetPreCaptureMousePosFromSlateCursor();
	}
	bool bOldCursorVisibility = Viewport->IsCursorVisible();
	bool bOldSoftwareCursorVisibility = Viewport->IsSoftwareCursorVisible();

	Viewport->ShowCursor( RequiredCursorVisibiltyAndAppearance.bHardwareCursorVisible );
	Viewport->ShowSoftwareCursor( RequiredCursorVisibiltyAndAppearance.bSoftwareCursorVisible );
	if( bUpdateSoftwareCursorPostion == true )
	{
		//if we made the software cursor visible set its position
		if( bOldSoftwareCursorVisibility != Viewport->IsSoftwareCursorVisible() )
		{			
			Viewport->SetSoftwareCursorPosition( FVector2D(Viewport->GetMouseX() , Viewport->GetMouseY() ) );			
		}
	}
}


void FEditorViewportClient::SetRequiredCursorOverride( bool WantOverride, EMouseCursor::Type RequiredCursor )
{
	RequiredCursorVisibiltyAndAppearance.bOverrideAppearance = WantOverride;
	RequiredCursorVisibiltyAndAppearance.RequiredCursor = RequiredCursor;
}

EAxisList::Type FEditorViewportClient::GetCurrentWidgetAxis() const
{
	return Widget->GetCurrentAxis();
}

void FEditorViewportClient::SetCurrentWidgetAxis( EAxisList::Type InAxis )
{
	Widget->SetCurrentAxis( InAxis );
}

float FEditorViewportClient::GetNearClipPlane() const
{
	return (NearPlane < 0.0f) ? GNearClippingPlane : NearPlane;
}

void FEditorViewportClient::OverrideNearClipPlane(float InNearPlane)
{
	NearPlane = InNearPlane;
}

void FEditorViewportClient::MoveViewportCamera(const FVector& InDrag, const FRotator& InRot, bool bDollyCamera )
{


	switch( GetViewportType() )
	{
	case LVT_OrthoXY:
	case LVT_OrthoXZ:
	case LVT_OrthoYZ:
		{
			const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton);
			const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton);
			const bool bIsUsingTrackpad = FSlateApplication::Get().IsUsingTrackpad();

			if( ( LeftMouseButtonDown || bIsUsingTrackpad ) && RightMouseButtonDown )
			{
				SetOrthoZoom( GetOrthoZoom() + (GetOrthoZoom() / CAMERA_ZOOM_DAMPEN) * InDrag.Z );
				SetOrthoZoom( FMath::Clamp<float>( GetOrthoZoom(), MIN_ORTHOZOOM, MAX_ORTHOZOOM ) );	
			}
			else
			{
				SetViewLocation( GetViewLocation() + InDrag );
			}

			// Update any linked orthographic viewports.
			UpdateLinkedOrthoViewports();
		}
		break;

	case LVT_OrthoFreelook:
		//@TODO: CAMERA: Not sure how to handle this
		break;

	case LVT_Perspective:
		{
			// If the flight camera is active, we'll update the rotation impulse data for that instead
			// of rotating the camera ourselves here
			if( IsFlightCameraInputModeActive() && CameraController->GetConfig().bUsePhysicsBasedRotation )
			{
				const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();

				// NOTE: We damp the rotation for impulse input since the camera controller will
				//	apply its own rotation speed
				const float VelModRotSpeed = 900.0f;
				const FVector RotEuler = InRot.Euler();

				CameraUserImpulseData->RotateRollVelocityModifier += VelModRotSpeed * RotEuler.X / ViewportSettings->MouseSensitivty;
				CameraUserImpulseData->RotatePitchVelocityModifier += VelModRotSpeed * RotEuler.Y / ViewportSettings->MouseSensitivty;
				CameraUserImpulseData->RotateYawVelocityModifier += VelModRotSpeed * RotEuler.Z / ViewportSettings->MouseSensitivty;
			}
			else
			{
				MoveViewportPerspectiveCamera( InDrag, InRot, bDollyCamera );
			}
		}
		break;
	}
}

bool FEditorViewportClient::ShouldLockPitch() const
{
	return CameraController->GetConfig().bLockedPitch;
}


void FEditorViewportClient::CheckHoveredHitProxy( HHitProxy* HoveredHitProxy )
{
	const EAxisList::Type SaveAxis = Widget->GetCurrentAxis();
	EAxisList::Type NewAxis = EAxisList::None;

	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton) ? true : false;
	const bool MiddleMouseButtonDown = Viewport->KeyState(EKeys::MiddleMouseButton) ? true : false;
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton) ? true : false;
	const bool bMouseButtonDown = (LeftMouseButtonDown || MiddleMouseButtonDown || RightMouseButtonDown );

	// Change the mouse cursor if the user is hovering over something they can interact with.
	if( HoveredHitProxy )
	{
		if( HoveredHitProxy->IsA(HWidgetAxis::StaticGetType() ) && !bUsingOrbitCamera && !bMouseButtonDown )
		{
			// In the case of the widget mode being overridden we can have a hit proxy
			// from the previous mode with an inappropriate axis for rotation.
			EAxisList::Type ProxyAxis = ((HWidgetAxis*)HoveredHitProxy)->Axis;
			if ( !IsOrtho() || GetWidgetMode() != FWidget::WM_Rotate
				|| ProxyAxis == EAxisList::X || ProxyAxis == EAxisList::Y || ProxyAxis == EAxisList::Z )
			{
				NewAxis = ProxyAxis;
			}
			else
			{
				switch( GetViewportType() )
				{
				case LVT_OrthoXY:
					NewAxis = EAxisList::Z;
					break;
				case LVT_OrthoXZ:
					NewAxis = EAxisList::Y;
					break;
				case LVT_OrthoYZ:
					NewAxis = EAxisList::X;
					break;
				default:
					break;
				}
			}
		}


		// If the current axis on the widget changed, repaint the viewport.
		if( NewAxis != SaveAxis )
		{
			SetCurrentWidgetAxis( NewAxis );

			Invalidate( false, false );
		}
	}

}

void FEditorViewportClient::ConditionalCheckHoveredHitProxy()
{
	// If it has been decided that there is more important things to do than check hit proxies, then don't check them.
	if( !bShouldCheckHitProxy || bWidgetAxisControlledByDrag == true )
	{
		return;
	}

	HHitProxy* HitProxy = Viewport->GetHitProxy(CachedMouseX,CachedMouseY);

	CheckHoveredHitProxy( HitProxy );

	// We need to set this to false here as if mouse is moved off viewport fast, it will keep doing CheckHoveredOverHitProxy for this viewport when it should not.
	bShouldCheckHitProxy = false;
}

/** Moves a perspective camera */
void FEditorViewportClient::MoveViewportPerspectiveCamera( const FVector& InDrag, const FRotator& InRot, bool bDollyCamera )
{
	check( IsPerspective() );

	FVector ViewLocation = GetViewLocation();
	FRotator ViewRotation = GetViewRotation();

	if ( ShouldLockPitch() )
	{
		// Update camera Rotation
		ViewRotation += FRotator( InRot.Pitch, InRot.Yaw, InRot.Roll );

		// normalize to -180 to 180
		ViewRotation.Pitch = FRotator::NormalizeAxis(ViewRotation.Pitch);
		// Make sure its withing  +/- 90 degrees.
		ViewRotation.Pitch = FMath::Clamp( ViewRotation.Pitch, -90.f, 90.f );		
	}
	else
	{
		//when not constraining the pitch (matinee feature) we need to rotate differently to avoid a gimbal lock
		const FRotator PitchRot(InRot.Pitch, 0, 0);
		const FRotator LateralRot(0, InRot.Yaw, InRot.Roll);

		//update lateral rotation
		ViewRotation += LateralRot;

		//update pitch separately using quaternions
		const FQuat ViewQuat = ViewRotation.Quaternion();
		const FQuat PitchQuat = PitchRot.Quaternion();
		const FQuat ResultQuat = ViewQuat * PitchQuat;

		//get our correctly rotated ViewRotation
		ViewRotation = ResultQuat.Rotator();	
	}

	// Update camera Location
	ViewLocation += InDrag;
	
	if( !bDollyCamera )
	{
		const float DistanceToCurrentLookAt = FVector::Dist( GetViewLocation() , GetLookAtLocation() );

		const FQuat CameraOrientation = FQuat::MakeFromEuler( ViewRotation.Euler() );
		FVector Direction = CameraOrientation.RotateVector( FVector(1,0,0) );

		SetLookAtLocation( ViewLocation + Direction * DistanceToCurrentLookAt );
	}

	SetViewLocation( ViewLocation );
	SetViewRotation( ViewRotation );

	PerspectiveCameraMoved();

}

void FEditorViewportClient::EnableCameraLock(bool bEnable)
{
	bCameraLock = bEnable;

	if(bCameraLock)
	{
		SetViewLocation( DefaultOrbitLocation + DefaultOrbitZoom );
		SetViewRotation( DefaultOrbitRotation );
		SetLookAtLocation( DefaultOrbitLookAt );
	}
	else
	{
		ToggleOrbitCamera( false );
	}

	bUsingOrbitCamera = bCameraLock;
}

FCachedJoystickState* FEditorViewportClient::GetJoystickState(const uint32 InControllerID)
{
	FCachedJoystickState* CurrentState = JoystickStateMap.FindRef(InControllerID);
	if (CurrentState == NULL)
	{
		/** Create new joystick state for cached input*/
		CurrentState = new FCachedJoystickState();
		CurrentState->JoystickType = 0;
		JoystickStateMap.Add(InControllerID, CurrentState);
	}

	return CurrentState;
}


void FEditorViewportClient::OverrideSafeFrameDisplay( bool bEnableOverrides, bool bShowAspectRatioBars, bool bShowSafeFrameBox )
{
	bEnableSafeFrameOverride = bEnableOverrides;
	bShowAspectRatioBarsOverride = bShowAspectRatioBars;
	bShowSafeFrameBoxOverride = bShowSafeFrameBox;
	Invalidate(false,false);
}

void FEditorViewportClient::SetCameraLock()
{
	EnableCameraLock(!bCameraLock);
	Invalidate();
}

bool FEditorViewportClient::IsCameraLocked() const
{
	return bCameraLock;
}

void FEditorViewportClient::SetShowGrid()
{
	DrawHelper.bDrawGrid = !DrawHelper.bDrawGrid;	
	Invalidate();
}

bool FEditorViewportClient::IsSetShowGridChecked() const
{
	return DrawHelper.bDrawGrid;
}

void FEditorViewportClient::SetShowBounds(bool bShow)
{
	EngineShowFlags.Bounds = bShow;
}

void FEditorViewportClient::ToggleShowBounds()
{
	EngineShowFlags.Bounds = 1 - EngineShowFlags.Bounds;
	Invalidate();
}

bool FEditorViewportClient::IsSetShowBoundsChecked() const
{
	return EngineShowFlags.Bounds;
}

void FEditorViewportClient::SetShowCollision()
{
	EngineShowFlags.Collision = !EngineShowFlags.Collision;
	Invalidate();
}

bool FEditorViewportClient::IsSetShowCollisionChecked() const
{
	return EngineShowFlags.Collision;
}

void FEditorViewportClient::SetRealtimePreview()
{
	SetRealtime(!IsRealtime());
	Invalidate();
}

void FEditorViewportClient::SetViewMode(EViewModeIndex InViewModeIndex)
{
	if (IsPerspective())
	{
		PerspViewModeIndex = InViewModeIndex;
		ApplyViewMode(PerspViewModeIndex, true, EngineShowFlags);
	}
	else
	{
		OrthoViewModeIndex = InViewModeIndex;
		ApplyViewMode(OrthoViewModeIndex, false, EngineShowFlags);
	}

	Invalidate();
}

void FEditorViewportClient::SetViewModes(const EViewModeIndex InPerspViewModeIndex, const EViewModeIndex InOrthoViewModeIndex)
{
	PerspViewModeIndex = InPerspViewModeIndex;
	OrthoViewModeIndex = InOrthoViewModeIndex;

	if (IsPerspective())
	{
		ApplyViewMode(PerspViewModeIndex, true, EngineShowFlags);
	}
	else
	{
		ApplyViewMode(OrthoViewModeIndex, false, EngineShowFlags);
	}

	Invalidate();
}

EViewModeIndex FEditorViewportClient::GetViewMode() const
{
	return (IsPerspective()) ? PerspViewModeIndex : OrthoViewModeIndex;
}

void FEditorViewportClient::Invalidate(bool bInvalidateChildViews, bool bInvalidateHitProxies)
{
	if ( Viewport )
	{
		if ( bInvalidateHitProxies )
		{
			// Invalidate hit proxies and display pixels.
			Viewport->Invalidate();
		}
		else
		{
			// Invalidate only display pixels.
			Viewport->InvalidateDisplay();
		}

		// If this viewport is a view parent . . .
		if ( bInvalidateChildViews &&
			ViewState.GetReference()->IsViewParent() )
		{
			GEditor->InvalidateChildViewports( ViewState.GetReference(), bInvalidateHitProxies );	
		}
	}
}

void FEditorViewportClient::OnJoystickPlugged(const uint32 InControllerID, const uint32 InType, const uint32 bInConnected)
{
	FCachedJoystickState* CurrentState = JoystickStateMap.FindRef(InControllerID);
	//joystick is now disabled, delete if needed
	if (!bInConnected)
	{
		JoystickStateMap.Remove(InControllerID);
		delete CurrentState;
	}
	else
	{
		if (CurrentState == NULL)
		{
			/** Create new joystick state for cached input*/
			CurrentState = new FCachedJoystickState();
			CurrentState->JoystickType = InType;
			JoystickStateMap.Add(InControllerID, CurrentState);
		}
	}
}

void FEditorViewportClient::MouseMove(FViewport* InViewport,int32 x, int32 y)
{
	check(IsInGameThread());
}

void FEditorViewportClient::CapturedMouseMove( FViewport* InViewport, int32 InMouseX, int32 InMouseY )
{
	UpdateRequiredCursorVisibility();
	ApplyRequiredCursorVisibility();
}

void FEditorViewportClient::OpenScreenshot( FString SourceFilePath )
{
	FPlatformProcess::ExploreFolder( *( FPaths::GetPath( SourceFilePath ) ) );
}

void FEditorViewportClient::TakeScreenshot(FViewport* InViewport, bool bInValidatViewport)
{
	// The old method for taking screenshots does this for us on mousedown, so we do not have
	//	to do this for all situations.
	if( bInValidatViewport )
	{
		// We need to invalidate the viewport in order to generate the correct pixel buffer for picking.
		Invalidate( false, true );
	}

	// Redraw the viewport so we don't end up with clobbered data from other viewports using the same frame buffer.
	InViewport->Draw();

	// Default the result to fail it will be set to  SNotificationItem::CS_Success if saved ok
	SNotificationItem::ECompletionState SaveResultState = SNotificationItem::CS_Fail;
	// The string we will use to tell the user the result of the save
	FText ScreenshotSaveResultText;
	FString HyperLinkString;

	// Read the contents of the viewport into an array.
	TArray<FColor> Bitmap;
	if( InViewport->ReadPixels(Bitmap) )
	{
		check(Bitmap.Num() == InViewport->GetSizeXY().X * InViewport->GetSizeXY().Y);

		// Create screenshot folder if not already present.
		if ( IFileManager::Get().MakeDirectory( *FPaths::ScreenShotDir(), true ) )
		{
			// Save the contents of the array to a bitmap file.
			FString ScreenshotSaveName = "";
			if ( FFileHelper::CreateBitmap(*(FPaths::ScreenShotDir() / TEXT("ScreenShot")),InViewport->GetSizeXY().X,InViewport->GetSizeXY().Y,Bitmap.GetTypedData(),NULL,&IFileManager::Get(),&ScreenshotSaveName) )
			{
				// Setup the string with the path and name of the file
				ScreenshotSaveResultText = NSLOCTEXT( "UnrealEd", "ScreenshotSavedAs", "Screenshot capture saved as" );					
				HyperLinkString = FPaths::ConvertRelativePathToFull( ScreenshotSaveName );	
				// Flag success
				SaveResultState = SNotificationItem::CS_Success;
			}
			else
			{
				// Failed to save the bitmap
				ScreenshotSaveResultText = NSLOCTEXT( "UnrealEd", "ScreenshotFailedBitmap", "Screenshot failed, unable to save" );									
			}
		}
		else
		{
			// Failed to make save directory
			ScreenshotSaveResultText = NSLOCTEXT( "UnrealEd", "ScreenshotFailedFolder", "Screenshot capture failed, unable to create save directory (see log)" );					
			UE_LOG(LogEditorViewport, Warning, TEXT("Failed to create directory %s"), *FPaths::ConvertRelativePathToFull(FPaths::ScreenShotDir()));
		}
	}
	else
	{
		// Failed to read the image from the viewport
		ScreenshotSaveResultText = NSLOCTEXT( "UnrealEd", "ScreenshotFailedViewport", "Screenshot failed, unable to read image from viewport" );					
	}

	// Inform the user of the result of the operation
	FNotificationInfo Info( ScreenshotSaveResultText );
	Info.ExpireDuration = 5.0f;
	Info.bUseSuccessFailIcons = false;
	Info.bUseLargeFont = false;
	if ( !HyperLinkString.IsEmpty() )
	{
		Info.Hyperlink = FSimpleDelegate::CreateRaw(this, &FEditorViewportClient::OpenScreenshot, HyperLinkString );
		Info.HyperlinkText = FText::FromString( HyperLinkString );
	}

	TWeakPtr<SNotificationItem> SaveMessagePtr;
	SaveMessagePtr = FSlateNotificationManager::Get().AddNotification(Info);
	SaveMessagePtr.Pin()->SetCompletionState(SaveResultState);
}

/**
 * Implements screenshot capture for editor viewports.
 */
bool FEditorViewportClient::InputTakeScreenshot(FViewport* InViewport, FKey Key, EInputEvent Event)
{
	const bool F9Down = InViewport->KeyState(EKeys::F9);

	// Whether or not we accept the key press
	bool bHandled = false;

	if ( F9Down )
	{
		if ( Key == EKeys::LeftMouseButton )
		{
			if( Event == IE_Pressed )
			{
				// We need to invalidate the viewport in order to generate the correct pixel buffer for picking.
				Invalidate( false, true );
			}
			else if( Event == IE_Released )
			{
				TakeScreenshot(InViewport,false);
			}
			bHandled = true;
		}
	}

	return bHandled;
}

void FEditorViewportClient::TakeHighResScreenShot()
{
	if(Viewport)
	{
		Viewport->TakeHighResScreenShot();
	}
}

void FEditorViewportClient::ProcessScreenShots(FViewport* InViewport)
{
	if (GIsDumpingMovie || FScreenshotRequest::IsScreenshotRequested() || GIsHighResScreenshot)
	{
		FString ScreenShotName = FScreenshotRequest::GetFilename();
		TArray<FColor> Bitmap;
		if( GetViewportScreenShot(InViewport, Bitmap) )
		{
			FIntPoint BitmapSize = InViewport->GetSizeXY();
			FIntRect SourceRect = GIsHighResScreenshot ? GetHighResScreenshotConfig().CaptureRegion : FIntRect(0, 0, GScreenshotResolutionX, GScreenshotResolutionY);
			bool bWriteAlpha = false;

			// If this is a high resolution screenshot and we are using the masking feature,
			// Get the results of the mask rendering pass and insert into the alpha channel of the screenshot.
			if (GIsHighResScreenshot && GetHighResScreenshotConfig().bMaskEnabled)
			{
				bWriteAlpha = GetHighResScreenshotConfig().MergeMaskIntoAlpha(Bitmap);
			}

			FFileHelper::CreateBitmap(*ScreenShotName, BitmapSize.X, BitmapSize.Y, Bitmap.GetTypedData(), &SourceRect, &IFileManager::Get(), NULL, bWriteAlpha);
		}
		
		// Done with the request
		FScreenshotRequest::Reset();

		// Re-enable screen messages - if we are NOT capturing a movie
		GAreScreenMessagesEnabled = GScreenMessagesRestoreState;

		InViewport->InvalidateHitProxy();
	}
}

void FEditorViewportClient::DrawBoundingBox(FBox &Box, FCanvas* InCanvas, const FSceneView* InView, const FViewport* InViewport, const FLinearColor& InColor, const bool bInDrawBracket, const FString &InLabelText)
{
	FVector BoxCenter, BoxExtents;
	Box.GetCenterAndExtents( BoxCenter, BoxExtents );

	// Project center of bounding box onto screen.
	const FVector4 ProjBoxCenter = InView->WorldToScreen(BoxCenter);
		
	// Do nothing if behind camera
	if( ProjBoxCenter.W > 0.f )
	{
		// Project verts of world-space bounding box onto screen and take their bounding box
		const FVector Verts[8] = {	FVector( 1, 1, 1),
			FVector( 1, 1,-1),
			FVector( 1,-1, 1),
			FVector( 1,-1,-1),
			FVector(-1, 1, 1),
			FVector(-1, 1,-1),
			FVector(-1,-1, 1),
			FVector(-1,-1,-1) };

		const int32 HalfX = 0.5f * InViewport->GetSizeXY().X;
		const int32 HalfY = 0.5f * InViewport->GetSizeXY().Y;

		FVector2D ScreenBoxMin(1000000000, 1000000000);
		FVector2D ScreenBoxMax(-1000000000, -1000000000);

		for(int32 j=0; j<8; j++)
		{
			// Project vert into screen space.
			const FVector WorldVert = BoxCenter + (Verts[j]*BoxExtents);
			FVector2D PixelVert;
			if(InView->ScreenToPixel(InView->WorldToScreen(WorldVert),PixelVert))
			{
				// Update screen-space bounding box with with transformed vert.
				ScreenBoxMin.X = FMath::Min<int32>(ScreenBoxMin.X, PixelVert.X);
				ScreenBoxMin.Y = FMath::Min<int32>(ScreenBoxMin.Y, PixelVert.Y);

				ScreenBoxMax.X = FMath::Max<int32>(ScreenBoxMax.X, PixelVert.X);
				ScreenBoxMax.Y = FMath::Max<int32>(ScreenBoxMax.Y, PixelVert.Y);
			}
		}


		FCanvasLineItem LineItem( FVector2D( 0.0f, 0.0f ), FVector2D( 0.0f, 0.0f ) );
		LineItem.SetColor( InColor );
		if( bInDrawBracket )
		{
			// Draw a bracket when considering the non-current level.
			const float DeltaX = ScreenBoxMax.X - ScreenBoxMin.X;
			const float DeltaY = ScreenBoxMax.X - ScreenBoxMin.X;
			const FIntPoint Offset( DeltaX * 0.2f, DeltaY * 0.2f );

			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMin.X, ScreenBoxMin.Y), FVector2D(ScreenBoxMin.X + Offset.X, ScreenBoxMin.Y) );
			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMin.X, ScreenBoxMax.Y), FVector2D(ScreenBoxMin.X + Offset.X, ScreenBoxMax.Y) );

			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMax.X, ScreenBoxMin.Y), FVector2D(ScreenBoxMax.X - Offset.X, ScreenBoxMin.Y) );
			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMax.X, ScreenBoxMax.Y), FVector2D(ScreenBoxMax.X - Offset.X, ScreenBoxMax.Y) );

			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMin.X, ScreenBoxMin.Y), FVector2D(ScreenBoxMin.X, ScreenBoxMin.Y + Offset.Y) );
			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMax.X, ScreenBoxMin.Y), FVector2D(ScreenBoxMax.X, ScreenBoxMin.Y + Offset.Y) );

			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMin.X, ScreenBoxMax.Y), FVector2D(ScreenBoxMin.X, ScreenBoxMax.Y - Offset.Y) );
			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMax.X, ScreenBoxMax.Y), FVector2D(ScreenBoxMax.X, ScreenBoxMax.Y - Offset.Y) );
		}
		else
		{
			// Draw a box when considering the current level.
			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMin.X, ScreenBoxMin.Y), FVector2D(ScreenBoxMin.X, ScreenBoxMax.Y) );
			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMin.X, ScreenBoxMax.Y), FVector2D(ScreenBoxMax.X, ScreenBoxMax.Y) );
			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMax.X, ScreenBoxMax.Y), FVector2D(ScreenBoxMax.X, ScreenBoxMin.Y) );
			LineItem.Draw( InCanvas, FVector2D(ScreenBoxMax.X, ScreenBoxMin.Y), FVector2D(ScreenBoxMin.X, ScreenBoxMin.Y) );
		}


		if (InLabelText.Len() > 0)
		{
			FCanvasTextItem TextItem( FVector2D( ScreenBoxMin.X + ((ScreenBoxMax.X - ScreenBoxMin.X) * 0.5f),ScreenBoxMin.Y), FText::FromString( InLabelText ), GEngine->GetMediumFont(), InColor );
			TextItem.bCentreX = true;
			InCanvas->DrawItem( TextItem );
		}		
	}
}

void FEditorViewportClient::SetGameView(bool bGameViewEnable)
{
	// backup this state as we want to preserve it
	bool bCompositeEditorPrimitives = EngineShowFlags.CompositeEditorPrimitives;

	// defaults
	FEngineShowFlags GameFlags(ESFIM_Game);
	FEngineShowFlags EditorFlags(ESFIM_Editor);
	{
		// likely we can take the existing state
		if(EngineShowFlags.Game)
		{
			GameFlags = EngineShowFlags;
			EditorFlags = LastEngineShowFlags;
		}
		else if(LastEngineShowFlags.Game)
		{
			GameFlags = LastEngineShowFlags;
			EditorFlags = EngineShowFlags;
		}
	}

	// toggle between the game and engine flags
	if(bGameViewEnable)
	{
		EngineShowFlags = GameFlags;
		LastEngineShowFlags = EditorFlags;
	}
	else
	{
		EngineShowFlags = EditorFlags;
		LastEngineShowFlags = GameFlags;
	}

	// maintain this state
	EngineShowFlags.CompositeEditorPrimitives = bCompositeEditorPrimitives;
	LastEngineShowFlags.CompositeEditorPrimitives = bCompositeEditorPrimitives;

	EngineShowFlags.SelectionOutline = bGameViewEnable ? false : GetDefault<ULevelEditorViewportSettings>()->bUseSelectionOutline;

	ApplyViewMode(GetViewMode(), IsPerspective(), EngineShowFlags);

	bInGameViewMode = bGameViewEnable;

	Invalidate();
}
#undef LOCTEXT_NAMESPACE 
