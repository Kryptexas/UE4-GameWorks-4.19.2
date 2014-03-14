// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EditorModeInterpolation : Editor mode for setting up interpolation sequences.

=============================================================================*/

#include "UnrealEd.h"
#include "EngineInterpolationClasses.h"
#include "InterpolationHitProxy.h"

#include "Editor/Matinee/Public/MatineeModule.h"
#include "Editor/Matinee/Public/IMatinee.h"

static const float	CurveHandleScale = 0.5f;

//////////////////////////////////////////////////////////////////////////
// FEdModeInterpEdit
//////////////////////////////////////////////////////////////////////////

FEdModeInterpEdit::FEdModeInterpEdit()
{
	ID = FBuiltinEditorModes::EM_InterpEdit;
	Name = NSLOCTEXT("EditorModes", "MatineeMode", "Matinee Mode");

	MatineeActor = NULL;
	InterpEd = NULL;

	Tools.Add( new FModeTool_InterpEdit() );
	SetCurrentTool( MT_InterpEdit );

	bLeavingMode = false;
}

FEdModeInterpEdit::~FEdModeInterpEdit()
{
}

bool FEdModeInterpEdit::InputKey( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event )
{
	// Enter key drops new key frames
	if( Key == EKeys::Enter &&
		( Event == IE_Pressed || Event == IE_Repeat ) &&
		( !ViewportClient->IsShiftPressed() &&
		  !ViewportClient->IsAltPressed() &&
		  !ViewportClient->IsCtrlPressed() ) )
	{
		if( InterpEd != NULL )
		{
			// Add a new key!
			InterpEd->AddKey();
		}

		return true;
	}

	return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
}

void FEdModeInterpEdit::Enter()
{
	FEdMode::Enter();

	// Disable Grouping while in InterpEdit mode
	bGroupingActiveSaved = GEditor->bGroupingActive;
	GEditor->bGroupingActive = false;
}

void FEdModeInterpEdit::Exit()
{
	MatineeActor = NULL;

	// If there is one, close the Interp Editor and clear pointers.
	if(InterpEd != NULL)
	{
		bLeavingMode = true; // This is so the editor being closed doesn't try and change the mode again!

		InterpEd->Close(true);

		bLeavingMode = false;
	}

	InterpEd = NULL;

	// Grouping is always disabled while in InterpEdit Mode, re-enable the saved value on exit
	GEditor->bGroupingActive = bGroupingActiveSaved;
	AGroupActor::SelectGroupsInSelection();

	FEdMode::Exit();
}

// We see if we have 
void FEdModeInterpEdit::ActorMoveNotify()
{
	if(!InterpEd)
		return;

	InterpEd->ActorModified();
}


void FEdModeInterpEdit::CamMoveNotify(FLevelEditorViewportClient* ViewportClient)
{
	if(!InterpEd)
		return;

	if( ViewportClient->AllowMatineePreview() )
	{
		InterpEd->CamMoved( ViewportClient->GetViewLocation(), ViewportClient->GetViewRotation() );
	}
}

void FEdModeInterpEdit::ActorPropChangeNotify()
{
	if(!InterpEd)
		return;

	InterpEd->ActorModified();
}

void FEdModeInterpEdit::UpdateSelectedActor()
{
	// would this be enough?
	InterpEd->ActorSelectionChange();
}

// Set the currently edited MatineeActor and open timeline window. Should always be called after we change to FBuiltinEditorModes::EM_InterpEdit.
void FEdModeInterpEdit::InitInterpMode(AMatineeActor* InMatineeActor)
{
	check(InMatineeActor);
	check(!InterpEd);

	MatineeActor = InMatineeActor;
	
	EToolkitMode::Type Mode = EToolkitMode::Standalone;
	IMatineeModule* MatineeModule = &FModuleManager::LoadModuleChecked<IMatineeModule>( "Matinee" );
	
	InterpEd = &(MatineeModule->CreateMatinee(Mode, TSharedPtr<IToolkitHost>(), InMatineeActor).Get());
}

void FEdModeInterpEdit::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(View, Viewport, PDI);

	check( GEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_InterpEdit) );

	if(InterpEd  && !InterpEd->Hide3DTrackView() && View->Family->EngineShowFlags.Splines)
	{
		InterpEd->DrawTracks3D(View, PDI);
	}
}

void FEdModeInterpEdit::DrawHUD(FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas)
{
	FEdMode::DrawHUD(ViewportClient,Viewport,View,Canvas);

	if(InterpEd)
	{
		InterpEd->DrawModeHUD(ViewportClient,Viewport,View,Canvas);
	}
}

bool FEdModeInterpEdit::AllowWidgetMove()
{
	FModeTool_InterpEdit* InterpTool = (FModeTool_InterpEdit*)FindTool(MT_InterpEdit);

	if(InterpTool->bMovingHandle)
	{
		return false;
	}
	else
	{
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////
// FModeTool_InterpEdit
//////////////////////////////////////////////////////////////////////////

FModeTool_InterpEdit::FModeTool_InterpEdit()
{
	ID = MT_InterpEdit;

	bMovingHandle = false;
	DragGroup = NULL;
	DragTrackIndex = false;
	DragKeyIndex = false;
	bDragArriving = false;
}

FModeTool_InterpEdit::~FModeTool_InterpEdit()
{
}

bool FModeTool_InterpEdit::MouseMove(FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,int32 x, int32 y)
{
	check( GEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_InterpEdit) );

	FEdModeInterpEdit* mode = (FEdModeInterpEdit*)GEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_InterpEdit);

	return 0;
}

/**
 * @return		true if the key was handled by this editor mode tool.
 */
bool FModeTool_InterpEdit::InputKey(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	check( GEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_InterpEdit) );

	FEdModeInterpEdit* mode = (FEdModeInterpEdit*)GEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_InterpEdit);
	if( !mode->InterpEd )
	{
		// Abort cleanly on InerpEd not yet being assigned.  This can occasionally be the case when receiving
		// modifier key release events when changing into interp edit mode.
		return false;
	}

	bool bCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
	bool bAltDown = Viewport->KeyState(EKeys::LeftAlt) || Viewport->KeyState(EKeys::RightAlt);
	bool bShiftDown = Viewport->KeyState(EKeys::LeftShift) || Viewport->KeyState(EKeys::RightShift);

	if( Key == EKeys::LeftMouseButton )
	{

		if( Event == IE_Pressed)
		{
			int32 HitX = ViewportClient->Viewport->GetMouseX();
			int32 HitY = ViewportClient->Viewport->GetMouseY();
			HHitProxy*	HitResult = ViewportClient->Viewport->GetHitProxy(HitX, HitY);

			if(HitResult)
			{
				if( HitResult->IsA(HInterpTrackKeypointProxy::StaticGetType()) )
				{
					HInterpTrackKeypointProxy* KeyProxy = (HInterpTrackKeypointProxy*)HitResult;
					UInterpGroup* Group = KeyProxy->Group;
					UInterpTrack* Track = KeyProxy->Track;
					int32 KeyIndex = KeyProxy->KeyIndex;

					// Using the CTRL modifier invokes multi-select keyframe selection.
					if( bCtrlDown )
					{
						// If key is already selected, deselect the key.
						if( mode->InterpEd->KeyIsInSelection(Group, Track, KeyIndex) )
						{
							mode->InterpEd->RemoveKeyFromSelection(Group, Track, KeyIndex);
							mode->InterpEd->InvalidateTrackWindowViewports();
						}
						// Otherwise, select the key while preserving previous selection.
						else
						{
							// This will invalidate the display - so we must not access the KeyProxy after this!
							mode->InterpEd->SelectTrack( Group, Track, false);
							mode->InterpEd->AddKeyToSelection(Group, Track, KeyIndex, !bShiftDown);
						}
					}
					else
					{
						mode->InterpEd->SelectTrack( Group, Track );
						// NOTE: Clear previously-selected tracks because ctrl is not down. 
						mode->InterpEd->ClearKeySelection();
						mode->InterpEd->AddKeyToSelection(Group, Track, KeyIndex, !bShiftDown);
					}
				}
				else if( HitResult->IsA(HInterpTrackKeyHandleProxy::StaticGetType()) )
				{
					// If we clicked on a 3D track handle, remember which key.
					HInterpTrackKeyHandleProxy* KeyProxy = (HInterpTrackKeyHandleProxy*)HitResult;
					DragGroup = KeyProxy->Group;
					DragTrackIndex = KeyProxy->TrackIndex;
					DragKeyIndex = KeyProxy->KeyIndex;
					bDragArriving = KeyProxy->bArriving;

					bMovingHandle = true;

					mode->InterpEd->BeginDrag3DHandle(DragGroup, DragTrackIndex);
				}
			}
		}
		else if( Event == IE_Released)
		{
			if(bMovingHandle)
			{
				mode->InterpEd->EndDrag3DHandle();
				bMovingHandle = false;
			}
		}
	}

	// Handle keys
	if( Event == IE_Pressed )
	{
		if( Key == EKeys::Delete )
		{
			// Swallow 'Delete' key to avoid deleting stuff when trying to interpolate it!
			return true;
		}
		else if( mode->InterpEd->ProcessKeyPress( Key, bCtrlDown, bAltDown ) )
		{
			return true;
		}
	}

	return FModeTool::InputKey(ViewportClient, Viewport, Key, Event);
}

/**
 * @return		true if the delta was handled by this editor mode tool.
 */
bool FModeTool_InterpEdit::InputDelta(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	check( GEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_InterpEdit) );

	FEdModeInterpEdit* mode = (FEdModeInterpEdit*)GEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_InterpEdit);
	check(mode->InterpEd);

	bool bShiftDown = InViewport->KeyState(EKeys::LeftShift) || InViewport->KeyState(EKeys::RightShift);

	FVector InputDeltaDrag( InDrag );

	// If we are grabbing a 'handle' on the movement curve, pass that info to Matinee
	if(bMovingHandle)
	{
		//if we're using inverted panning
		if ((InViewportClient->ViewportType != LVT_Perspective) && InViewportClient->ShouldUseMoveCanvasMovement())
		{
			InputDeltaDrag = -InputDeltaDrag;
		}


		mode->InterpEd->Move3DHandle( DragGroup, DragTrackIndex, DragKeyIndex, bDragArriving, InputDeltaDrag * (1.f/CurveHandleScale) );

		return 1;
	}
	// If shift is downOnly do 'move initial position' if dragging the widget
	else if(bShiftDown && InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
	{
		mode->InterpEd->MoveInitialPosition( InputDeltaDrag, InRot );

		return 1;
	}

	InViewportClient->Viewport->Invalidate();

	return 0;
}

void FModeTool_InterpEdit::SelectNone()
{
	check( GEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_InterpEdit) );

	FEdModeInterpEdit* mode = (FEdModeInterpEdit*)GEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_InterpEdit);
}
