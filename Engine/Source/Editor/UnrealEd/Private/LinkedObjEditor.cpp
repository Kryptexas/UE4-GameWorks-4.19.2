// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnLinkedEdInterface.cpp: Base class for boxes-and-lines editing
=============================================================================*/

#include "UnrealEd.h"
#include "LinkedObjEditor.h"
#include "LinkedObjDrawUtils.h"

const static float	LinkedObjectEditor_ZoomIncrement = 0.1f;
const static float	LinkedObjectEditor_ZoomSpeed = 0.005f;
const static float	LinkedObjectEditor_ZoomNotchThresh = 0.007f;
const static int32	LinkedObjectEditor_ScrollBorderSize = 20;
const static float	LinkedObjectEditor_ScrollBorderSpeed = 400.f;


/*-----------------------------------------------------------------------------
	FLinkedObjViewportClient
-----------------------------------------------------------------------------*/

FLinkedObjViewportClient::FLinkedObjViewportClient( FLinkedObjEdNotifyInterface* InEdInterface )
	:	bAlwaysDrawInTick( false )
{
	// No funky rendering features
	EngineShowFlags.PostProcessing = 0;
    EngineShowFlags.HMDDistortion = 0;
	EngineShowFlags.DisableAdvancedFeatures();

	// This window will be 2D/canvas only, so set the viewport type to None
	ViewportType = LVT_None;

	EdInterface			= InEdInterface;

	Origin2D			= FIntPoint(0, 0);
	Zoom2D				= 1.0f;
	MinZoom2D			= 0.1f;
	MaxZoom2D			= 1.f;

	bMouseDown			= false;
	OldMouseX			= 0;
	OldMouseY			= 0;

	BoxStartX			= 0;
	BoxStartY			= 0;
	BoxEndX				= 0;
	BoxEndY				= 0;

	DeltaXFraction		= 0.0f;
	DeltaYFraction		= 0.0f;

	ScrollAccum			= FVector2D::ZeroVector;

	DistanceDragged		= 0;

	bTransactionBegun	= false;
	bMakingLine			= false;
	bMovingConnector	= false;
	bSpecialDrag		= false;
	bBoxSelecting		= false;
	bAllowScroll		= true;

	bHasMouseCapture	= false;
	bShowCursorOverride	= true;
	bIsScrolling		= false;

	MouseOverObject		= NULL;
	MouseOverTime		= 0.f;
	ToolTipDelayMS		= -1;
	GConfig->GetInt( TEXT( "ToolTips" ), TEXT( "DelayInMS" ), ToolTipDelayMS, GEditorUserSettingsIni );

	SpecialIndex		= 0;
	
	DesiredPanTime		= 0.f;

	NewX = NewY = 0;

	SetRealtime( false );
}

void FLinkedObjViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	Canvas->PushAbsoluteTransform(FScaleMatrix(Zoom2D) * FTranslationMatrix(FVector(Origin2D.X,Origin2D.Y,0)));
	{
		// Erase background
		Canvas->Clear( FColor(197,197,197) );

		EdInterface->DrawObjects( Viewport, Canvas );

		// Draw new line
		if(bMakingLine && !Canvas->IsHitTesting())
		{
			FIntPoint StartPoint = EdInterface->GetSelectedConnLocation(Canvas);
			FIntPoint EndPoint( (NewX - Origin2D.X)/Zoom2D, (NewY - Origin2D.Y)/Zoom2D );
			int32 ConnType = EdInterface->GetSelectedConnectorType();
			FColor LinkColor = EdInterface->GetMakingLinkColor();

			// Curves
			{
				float Tension;
				if(ConnType == LOC_INPUT || ConnType == LOC_OUTPUT)
				{
					Tension = FMath::Abs<int32>(StartPoint.X - EndPoint.X);
				}
				else
				{
					Tension = FMath::Abs<int32>(StartPoint.Y - EndPoint.Y);
				}


				if(ConnType == LOC_INPUT)
				{
					FLinkedObjDrawUtils::DrawSpline(Canvas, StartPoint, Tension*FVector2D(-1,0), EndPoint, Tension*FVector2D(-1,0), LinkColor, false);
				}
				else if(ConnType == LOC_OUTPUT)
				{
					FLinkedObjDrawUtils::DrawSpline(Canvas, StartPoint, Tension*FVector2D(1,0), EndPoint, Tension*FVector2D(1,0), LinkColor, false);
				}
				else if(ConnType == LOC_VARIABLE)
				{
					FLinkedObjDrawUtils::DrawSpline(Canvas, StartPoint, Tension*FVector2D(0,1), EndPoint, FVector2D::ZeroVector, LinkColor, false);
				}
				else
				{
					FLinkedObjDrawUtils::DrawSpline(Canvas, StartPoint, Tension*FVector2D(0,1), EndPoint, Tension*FVector2D(0,1), LinkColor, false);
				}
			}
		}

	}
	Canvas->PopTransform();

	Canvas->PushAbsoluteTransform(FTranslationMatrix(FVector(Origin2D.X,Origin2D.Y,0)));
	{
		// Draw the box select box
		if(bBoxSelecting)
		{
			int32 MinX = (FMath::Min(BoxStartX, BoxEndX) - BoxOrigin2D.X);
			int32 MinY = (FMath::Min(BoxStartY, BoxEndY) - BoxOrigin2D.Y);
			int32 MaxX = (FMath::Max(BoxStartX, BoxEndX) - BoxOrigin2D.X);
			int32 MaxY = (FMath::Max(BoxStartY, BoxEndY) - BoxOrigin2D.Y);
			FCanvasBoxItem BoxItem( FVector2D(MinX, MinY), FVector2D(BoxEndX-BoxStartX, BoxEndY-BoxStartY) );
			BoxItem.SetColor( FLinearColor::Red );
			Canvas->DrawItem( BoxItem );
		}
	}
	Canvas->PopTransform();
}

bool FLinkedObjViewportClient::InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float /*AmountDepressed*/,bool /*Gamepad*/)
{
	bool bHandled = false;

	const bool bCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
	const bool bShiftDown = Viewport->KeyState(EKeys::LeftShift) || Viewport->KeyState(EKeys::RightShift);
	const bool bAltDown = Viewport->KeyState(EKeys::LeftAlt) || Viewport->KeyState(EKeys::RightAlt);

	const int32 HitX = Viewport->GetMouseX();
	const int32 HitY = Viewport->GetMouseY();
	
	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton) ? true : false;
	const bool MiddleMouseButtonDown = Viewport->KeyState(EKeys::MiddleMouseButton) ? true : false;
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton) ? true : false;
	const bool bAnyMouseButtonsDown = (LeftMouseButtonDown || MiddleMouseButtonDown || RightMouseButtonDown );
	const bool bAllMouseButtonsUp = !bAnyMouseButtonsDown;

	if (bAnyMouseButtonsDown && Event == IE_Pressed) {bHasMouseCapture = true;}
	else if (bAllMouseButtonsUp && Event == IE_Released) {bHasMouseCapture = false;}
	

	if ( !bHasMouseCapture )
	{
		Viewport->ShowCursor( true );
		Viewport->LockMouseToViewport( false );
	}

	static bool bDoubleClicking = false;

	static EInputEvent LastEvent = IE_Pressed;

	if( Key == EKeys::LeftMouseButton )
	{
		switch( Event )
		{
		case IE_Pressed:
		case IE_DoubleClick:
			{
				if (Event == IE_DoubleClick)
				{
					bDoubleClicking = true;
				}

				DeltaXFraction = 0.0f;
				DeltaYFraction = 0.0f;
				HHitProxy* HitResult = Viewport->GetHitProxy(HitX,HitY);

				if( HitResult )
				{
					// Handle click/double-click on line proxy
					if ( HitResult->IsA( HLinkedObjLineProxy::StaticGetType() ) )
					{
						// clicked on a line
						HLinkedObjLineProxy *LineProxy = (HLinkedObjLineProxy*)HitResult;
						if ( Event == IE_Pressed )
						{
							if( !EdInterface->HaveObjectsSelected() )
							{
								EdInterface->ClickedLine(LineProxy->Src,LineProxy->Dest);
							}
						}
						else if ( Event == IE_DoubleClick )
						{
							EdInterface->DoubleClickedLine( LineProxy->Src,LineProxy->Dest );
						}
					}
					// Handle click/double-click on object proxy
					else if( HitResult->IsA( HLinkedObjProxy::StaticGetType() ) )
					{
						UObject* Obj = ( (HLinkedObjProxy*)HitResult )->Obj;
						if( !bCtrlDown )
						{
							if( Event == IE_DoubleClick )
							{
								EdInterface->EmptySelection();
								EdInterface->AddToSelection( Obj );
								EdInterface->UpdatePropertyWindow();	
								EdInterface->DoubleClickedObject( Obj );
								bMouseDown = false;
								return true;
							}
							else if( !EdInterface->HaveObjectsSelected() )
							{
								// if there are no objects selected add this object. 
								// if objects are selected, we should not clear the selection until a mouse up occurs
								// since the user might be trying to pan. Panning should not break selection.
								EdInterface->AddToSelection( Obj );
								EdInterface->UpdatePropertyWindow();
							}
						}
					}
					// Handle click/double-click on connector proxy
					else if( HitResult->IsA(HLinkedObjConnectorProxy::StaticGetType()) )
					{
						HLinkedObjConnectorProxy* ConnProxy = (HLinkedObjConnectorProxy*)HitResult;
						EdInterface->SetSelectedConnector( ConnProxy->Connector );

						EdInterface->EmptySelection();
						EdInterface->UpdatePropertyWindow();

						if ( bAltDown )
						{
							// break the connectors
							EdInterface->AltClickConnector( ConnProxy->Connector );
						}
						else if( bCtrlDown )
						{
							// Begin moving a connector if ctrl+mouse click over a connector was done
							bMovingConnector = true;
							EdInterface->SetSelectedConnectorMoving( bMovingConnector );
						}
						else
						{
							if ( Event == IE_DoubleClick )
							{
								EdInterface->DoubleClickedConnector( ConnProxy->Connector );
							}
							else
							{
								bMakingLine = true;
								NewX = HitX;
								NewY = HitY;
							}
						}
					}
					// Handle click/double-click on special proxy
					else if( HitResult->IsA(HLinkedObjProxySpecial::StaticGetType()) )
					{
						HLinkedObjProxySpecial* SpecialProxy = (HLinkedObjProxySpecial*)HitResult;

						// Copy properties out of SpecialProxy first, in case it gets invalidated!
						int32 ProxyIndex = SpecialProxy->SpecialIndex;
						UObject* ProxyObj = SpecialProxy->Obj;

						FIntPoint MousePos( (HitX - Origin2D.X)/Zoom2D, (HitY - Origin2D.Y)/Zoom2D );

						// If object wasn't selected already OR 
						// we didn't handle it all in special click - change selection
						if( !EdInterface->IsInSelection( ProxyObj ) || 
							!EdInterface->SpecialClick(  MousePos.X, MousePos.Y, ProxyIndex, Viewport, ProxyObj ) )
						{
							bSpecialDrag = true;
							SpecialIndex = ProxyIndex;

							// Slightly quirky way of avoiding selecting the same thing again.
							if( !( EdInterface->GetNumSelected() == 1 && EdInterface->IsInSelection( ProxyObj ) ) )
							{
								EdInterface->EmptySelection();
								EdInterface->AddToSelection( ProxyObj );
								EdInterface->UpdatePropertyWindow();
							}

							// For supporting undo 
							EdInterface->BeginTransactionOnSelected();
							bTransactionBegun = true;
						}
					}
				}
				else
				{
					if( bCtrlDown && bAltDown )
					{
						BoxOrigin2D = Origin2D;
						BoxStartX = BoxEndX = HitX;
						BoxStartY = BoxEndY = HitY;

						bBoxSelecting = true;
					}
				}

				OldMouseX = HitX;
				OldMouseY = HitY;
				//default to not having moved yet
				bHasMouseMovedSinceClick = false;
				DistanceDragged = 0;
				bMouseDown = true;

				if( !bMakingLine && !bBoxSelecting && !bSpecialDrag && !(bCtrlDown && EdInterface->HaveObjectsSelected()) && bAllowScroll )
				{
				}
				else
				{
					Viewport->LockMouseToViewport(true);
				}

				Viewport->Invalidate();
			}
			break;

		case IE_Released:
			{
				if( bMakingLine )
				{
					Viewport->Invalidate();
					HHitProxy* HitResult = Viewport->GetHitProxy( HitX,HitY );
					if( HitResult )
					{
						if( HitResult->IsA(HLinkedObjConnectorProxy::StaticGetType()) )
						{
							HLinkedObjConnectorProxy* EndConnProxy = (HLinkedObjConnectorProxy*)HitResult;

							if( DistanceDragged < 4 )
							{
								HLinkedObjConnectorProxy* ConnProxy = (HLinkedObjConnectorProxy*)HitResult;
								bool bDoDeselect = EdInterface->ClickOnConnector( EndConnProxy->Connector.ConnObj, EndConnProxy->Connector.ConnType, EndConnProxy->Connector.ConnIndex );
								if( bDoDeselect && LastEvent != IE_DoubleClick )
								{
									EdInterface->EmptySelection();
									EdInterface->UpdatePropertyWindow();
								}
							}
							else if ( bAltDown )
							{
								EdInterface->AltClickConnector( EndConnProxy->Connector );
							}
							else
							{
								EdInterface->MakeConnectionToConnector( EndConnProxy->Connector );
							}
						}
						else if( HitResult->IsA(HLinkedObjProxy::StaticGetType()) )
						{
							UObject* Obj = ( (HLinkedObjProxy*)HitResult )->Obj;

							EdInterface->MakeConnectionToObject( Obj );
						}
					}
				}
				else if( bBoxSelecting )
				{
					// When box selecting, the region that user boxed can be larger than the size of the viewport
					// so we use the viewport as a max region and loop through the box, rendering different chunks of it
					// and reading back its hit proxy map to check for objects.
					TArray<UObject*> NewSelection;
					
					// Save the current origin since we will be modifying it.
					FVector2D SavedOrigin2D;
					
					SavedOrigin2D.X = Origin2D.X;
					SavedOrigin2D.Y = Origin2D.Y;

					// Calculate the size of the box and its extents.
					const int32 MinX = FMath::Min(BoxStartX, BoxEndX);
					const int32 MinY = FMath::Min(BoxStartY, BoxEndY);
					const int32 MaxX = FMath::Max(BoxStartX, BoxEndX) + 1;
					const int32 MaxY = FMath::Max(BoxStartY, BoxEndY) + 1;

					const int32 ViewX = Viewport->GetSizeXY().X-1;
					const int32 ViewY = Viewport->GetSizeXY().Y-1;

					const int32 BoxSizeX = MaxX - MinX;
					const int32 BoxSizeY = MaxY - MinY;

					const float BoxMinX = MinX-BoxOrigin2D.X;
					const float BoxMinY = MinY-BoxOrigin2D.Y;
					const float BoxMaxX = BoxMinX + BoxSizeX;
					const float BoxMaxY = BoxMinY + BoxSizeY;

					// Loop through 'tiles' of the box using the viewport size as our maximum tile size.
					int32 TestSizeX = FMath::Min(ViewX, BoxSizeX);
					int32 TestSizeY = FMath::Min(ViewY, BoxSizeY);

					float TestStartX = BoxMinX;
					float TestStartY = BoxMinY;

					while(TestStartX < BoxMaxX)
					{
						TestStartY = BoxMinY;
						TestSizeY = FMath::Min(ViewY, BoxSizeY);
				
						while(TestStartY < BoxMaxY)
						{
							// We read back the hit proxy map for the required region.
							Origin2D.X = -TestStartX;
							Origin2D.Y = -TestStartY;

							TArray<HHitProxy*> ProxyMap;
							Viewport->Invalidate();
							Viewport->InvalidateHitProxy();
							Viewport->GetHitProxyMap( FIntRect(0, 0, TestSizeX + 1, TestSizeY + 1), ProxyMap );

							// Todo: something is wrong here, we request the rectable bigger than we now process it.
							// This code will go away at some point so we don't change it for now

							// Find any keypoint hit proxies in the region - add the keypoint to selection.
							for( int32 Y=0; Y < TestSizeY; Y++ )
							{
								for( int32 X=0; X < TestSizeX; X++ )
								{
									HHitProxy* HitProxy = NULL;							
									int32 ProxyMapIndex = Y * TestSizeX + X; // calculate location in proxy map
									if( ProxyMapIndex < ProxyMap.Num() ) // If within range, grab the hit proxy from there
									{
										HitProxy = ProxyMap[ProxyMapIndex];
									}

									UObject* SelObject = NULL;

									// If we got one, add it to the NewSelection list.
									if( HitProxy )
									{

										if(HitProxy->IsA(HLinkedObjProxy::StaticGetType()))
										{
											SelObject = ((HLinkedObjProxy*)HitProxy)->Obj;
										}
										// Special case for the little resizer triangles in the bottom right corner of comment frames
										else if( HitProxy->IsA(HLinkedObjProxySpecial::StaticGetType()) )
										{
											SelObject = NULL;
										}

										if( SelObject )
										{
											// Don't want to call AddToSelection here because that might invalidate the display and we'll crash.
											NewSelection.AddUnique( SelObject );
										}
									}
								}
							}

							TestStartY += ViewY;
							TestSizeY = FMath::Min(ViewY, FMath::Trunc(BoxMaxY - TestStartY));
						}

						TestStartX += ViewX;
						TestSizeX = FMath::Min(ViewX, FMath::Trunc(BoxMaxX - TestStartX));
					}

		
					// restore the original viewport settings
					Origin2D.X = SavedOrigin2D.X;
					Origin2D.Y = SavedOrigin2D.Y;

					// If shift is down, don't empty, just add to selection.
					if( !bShiftDown )
					{
						EdInterface->EmptySelection();
					}
					
					// Iterate over array adding each to selection.
					for( int32 i=0; i<NewSelection.Num(); i++ )
					{
						EdInterface->AddToSelection( NewSelection[i] );
					}

					EdInterface->UpdatePropertyWindow();
				}
				else
				{
					HHitProxy*	HitResult = Viewport->GetHitProxy(HitX,HitY);

					// If mouse didn't really move since last time, and we released over empty space, deselect everything.
					if( !HitResult && DistanceDragged < 4 )
					{
						NewX = HitX;
						NewY = HitY;
						const bool bDoDeselect = EdInterface->ClickOnBackground() && !bCtrlDown;
						if( bDoDeselect && LastEvent != IE_DoubleClick )
						{
							EdInterface->EmptySelection();
							EdInterface->UpdatePropertyWindow();
						}
					}
					// If we've hit something and not really dragged anywhere, attempt to select
					// whatever has been hit
					else if ( HitResult && DistanceDragged < 4 )
					{
						// Released on a line
						if ( HitResult->IsA(HLinkedObjLineProxy::StaticGetType()) )
						{
							HLinkedObjLineProxy *LineProxy = (HLinkedObjLineProxy*)HitResult;
							EdInterface->ClickedLine( LineProxy->Src,LineProxy->Dest );
						}
						// Released on an object
						else if( HitResult->IsA(HLinkedObjProxy::StaticGetType()) )
						{
							UObject* Obj = (( HLinkedObjProxy*)HitResult )->Obj;

							if( !bCtrlDown )
							{
								EdInterface->EmptySelection();
								EdInterface->AddToSelection(Obj);
							}
							else if( EdInterface->IsInSelection( Obj ) )
							{
								// Remove the object from selection its in the selection and this isn't the initial click where the object was added
								EdInterface->RemoveFromSelection( Obj );
							}
							// At this point, the user CTRL-clicked on an unselected kismet object.
							// Since the the user isn't dragging, add the object the selection.
							else
							{
								// The user is trying to select multiple objects at once
								EdInterface->AddToSelection( Obj );
							}
							if (!bDoubleClicking)
							{
								EdInterface->UpdatePropertyWindow();
							}
						}
					}
					else if( bCtrlDown && DistanceDragged >= 4 )
					{
						EdInterface->PositionSelectedObjects();
					}
				}

				if( bTransactionBegun )
				{
					EdInterface->EndTransactionOnSelected();
					bTransactionBegun = false;
				}

				bMouseDown = false;
				bMakingLine = false;
				// Mouse was released, stop moving the connector
				bMovingConnector = false;
				EdInterface->SetSelectedConnectorMoving( bMovingConnector );
				bSpecialDrag = false;
				bBoxSelecting = false;

				Viewport->LockMouseToViewport( false );
				Viewport->Invalidate();

				bDoubleClicking = false;
			}
			break;
		}

		bHandled = true;
	}
	else if( Key == EKeys::RightMouseButton )
	{
		switch( Event )
		{
		case IE_Pressed:
			{
				NewX = Viewport->GetMouseX();
				NewY = Viewport->GetMouseY();
				DeltaXFraction = 0.0f;
				DeltaYFraction = 0.0f;
				DistanceDragged = 0;
			}
			break;

		case IE_Released:
			{
				Viewport->Invalidate();

				if( bTransactionBegun )
				{
					EdInterface->EndTransactionOnSelected();
					bTransactionBegun = false;
				}

				if(bMakingLine || Viewport->KeyState(EKeys::LeftMouseButton))
					break;

				// If right clicked and dragged - don't pop up menu. Have to click and release in roughly the same spot.
				if( FMath::Abs(HitX - NewX) + FMath::Abs(HitY - NewY) > 4 || DistanceDragged > 4)
					break;

				HHitProxy*	HitResult = Viewport->GetHitProxy(HitX,HitY);

				if(!HitResult)
				{
					EdInterface->OpenNewObjectMenu();
				}
				else
				{
					if( HitResult->IsA(HLinkedObjConnectorProxy::StaticGetType()) )
					{
						HLinkedObjConnectorProxy* ConnProxy = (HLinkedObjConnectorProxy*)HitResult;

						// First select the connector and deselect any objects.
						EdInterface->SetSelectedConnector( ConnProxy->Connector );
						EdInterface->EmptySelection();
						EdInterface->UpdatePropertyWindow();
						Viewport->Invalidate();

						// Then open connector options menu.
						EdInterface->OpenConnectorOptionsMenu();
					}
					else if( HitResult->IsA(HLinkedObjProxy::StaticGetType()) )
					{
						// When right clicking on an unselected object, select it only before opening menu.
						UObject* Obj = ((HLinkedObjProxy*)HitResult)->Obj;

						if( !EdInterface->IsInSelection(Obj) )
						{
							EdInterface->EmptySelection();
							EdInterface->AddToSelection(Obj);
							EdInterface->UpdatePropertyWindow();
							Viewport->Invalidate();
						}
					
						EdInterface->OpenObjectOptionsMenu();
					}
				}
			}
			break;
		}

		bHandled = true;
	}
	else if ( (Key == EKeys::MouseScrollDown || Key == EKeys::MouseScrollUp) && Event == IE_Pressed )
	{
		// Mousewheel up/down zooms in/out.
		const float DeltaZoom = (Key == EKeys::MouseScrollDown ? -LinkedObjectEditor_ZoomIncrement : LinkedObjectEditor_ZoomIncrement );

		if( (DeltaZoom < 0.f && Zoom2D > MinZoom2D) ||
			(DeltaZoom > 0.f && Zoom2D < MaxZoom2D) )
		{
			//Default zooming to center of the viewport
			float CenterOfZoomX = Viewport->GetSizeXY().X*0.5f;
			float CenterOfZoomY= Viewport->GetSizeXY().Y*0.5f;
			if (GetDefault<ULevelEditorViewportSettings>()->bCenterZoomAroundCursor)
			{
				//center of zoom is now around the mouse
				CenterOfZoomX = OldMouseX;
				CenterOfZoomY = OldMouseY;
			}
			//Old offset
			const float ViewCenterX = (CenterOfZoomX - Origin2D.X)/Zoom2D;
			const float ViewCenterY = (CenterOfZoomY - Origin2D.Y)/Zoom2D;

			//change zoom ratio
			Zoom2D = FMath::Clamp<float>(Zoom2D+DeltaZoom,MinZoom2D,MaxZoom2D);

			//account for new offset
			float DrawOriginX = ViewCenterX - (CenterOfZoomX/Zoom2D);
			float DrawOriginY = ViewCenterY - (CenterOfZoomY/Zoom2D);

			//move origin by delta we've calculated
			Origin2D.X = -(DrawOriginX * Zoom2D);
			Origin2D.Y = -(DrawOriginY * Zoom2D);

			EdInterface->ViewPosChanged();
			Viewport->Invalidate();
		}

		bHandled = true;
	}
	// Handle the user pressing the first thumb mouse button
	else if ( Key == EKeys::ThumbMouseButton && Event == IE_Pressed && !bMouseDown )
	{
		// Attempt to set the navigation history for the ed interface back an entry
		EdInterface->NavigationHistoryBack();

		bHandled = true;
	}
	// Handle the user pressing the second thumb mouse button
	else if ( Key == EKeys::ThumbMouseButton2 && Event == IE_Pressed && !bMouseDown )
	{
		// Attempt to set the navigation history for the ed interface forward an entry
		EdInterface->NavigationHistoryForward();

		bHandled = true;
	}
	else if ( Event == IE_Pressed )
	{
		// Bookmark support
		TCHAR CurChar = 0;
		if( Key == EKeys::Zero )		CurChar = '0';
		else if( Key == EKeys::One )	CurChar = '1';
		else if( Key == EKeys::Two )	CurChar = '2';
		else if( Key == EKeys::Three )	CurChar = '3';
		else if( Key == EKeys::Four )	CurChar = '4';
		else if( Key == EKeys::Five )	CurChar = '5';
		else if( Key == EKeys::Six )	CurChar = '6';
		else if( Key == EKeys::Seven )	CurChar = '7';
		else if( Key == EKeys::Eight )	CurChar = '8';
		else if( Key == EKeys::Nine )	CurChar = '9';

		if( ( CurChar >= '0' && CurChar <= '9' ) && !bAltDown && !bShiftDown && !bMouseDown)
		{
			// Determine the bookmark index based on the input key
			const int32 BookmarkIndex = CurChar - '0';

			// CTRL+# will set a bookmark, # will jump to it.
			if( bCtrlDown )
			{
				EdInterface->SetBookmark( BookmarkIndex );
			}
			else
			{
				EdInterface->JumpToBookmark( BookmarkIndex );
			}

			bHandled = true;
		}
	}

	if(EdInterface->EdHandleKeyInput(Viewport, Key, Event))
	{
		bHandled = true;
	}

	LastEvent = Event;

	// Hide and lock mouse cursor if we're capturing mouse input.
	// But don't hide mouse cursor if drawing lines and other special cases.
	bool bCanLockMouse = (Key == EKeys::LeftMouseButton) || (Key == EKeys::RightMouseButton);
	if ( bHasMouseCapture && bCanLockMouse )
	{
		//Update cursor and lock to window if invisible
		bool bShowCursor = UpdateCursorVisibility ();
		bool bDraggingObject = (bCtrlDown && EdInterface->HaveObjectsSelected());
		Viewport->LockMouseToViewport( !bShowCursor || bDraggingObject || bMakingLine || bBoxSelecting || bSpecialDrag);
	}

	// Handle viewport screenshot.
	InputTakeScreenshot( Viewport, Key, Event );

	return bHandled;
}

void FLinkedObjViewportClient::MouseMove(FViewport* Viewport, int32 X, int32 Y)
{
	int32 DeltaX = X - OldMouseX;
	int32 DeltaY = Y - OldMouseY;
	OldMouseX = X;
	OldMouseY = Y;


	// Do mouse-over stuff (if mouse button is not held).
	OnMouseOver( X, Y );
}

void FLinkedObjViewportClient::CapturedMouseMove( FViewport* InViewport, int32 InMouseX, int32 InMouseY )
{
	FEditorViewportClient::CapturedMouseMove(InViewport, InMouseX, InMouseY);
	
	// This is a hack put in place to make the slate material editor work ok
	// Because this runs InputAxis through Tick, any ShowCursor handling is overwritten	during OnMouseMove.
	// As a result, this hack redoes the ShowCursor part during OnMouseMove as well.
	// It should be removed when either we use the SGraphPanel or when reworking the input scheme
	if (bIsScrolling)
	{
		Viewport->ShowCursor(bShowCursorOverride);
	}
}

/** Handle mouse over events */
void FLinkedObjViewportClient::OnMouseOver( int32 X, int32 Y )
{
	// Do mouse-over stuff (if mouse button is not held).
	UObject *NewMouseOverObject = NULL;
	int32 NewMouseOverConnType = -1;
	int32 NewMouseOverConnIndex = INDEX_NONE;
	HHitProxy*	HitResult = NULL;

	if(!bMouseDown || bMakingLine)
	{
		HitResult = Viewport->GetHitProxy(X,Y);
	}

	if( HitResult )
	{
		if( HitResult->IsA(HLinkedObjProxy::StaticGetType()) )
		{
			NewMouseOverObject = ((HLinkedObjProxy*)HitResult)->Obj;
		}
		else if( HitResult->IsA(HLinkedObjConnectorProxy::StaticGetType()) )
		{
			NewMouseOverObject = ((HLinkedObjConnectorProxy*)HitResult)->Connector.ConnObj;
			NewMouseOverConnType = ((HLinkedObjConnectorProxy*)HitResult)->Connector.ConnType;
			NewMouseOverConnIndex = ((HLinkedObjConnectorProxy*)HitResult)->Connector.ConnIndex;

			if( !EdInterface->ShouldHighlightConnector(((HLinkedObjConnectorProxy*)HitResult)->Connector) )
			{
				NewMouseOverConnType = -1;
				NewMouseOverConnIndex = INDEX_NONE;
			}
		}
		else if (HitResult->IsA(HLinkedObjLineProxy::StaticGetType()) && !bMakingLine)		// don't mouse-over lines when already creating a line
		{
			HLinkedObjLineProxy *LineProxy = (HLinkedObjLineProxy*)HitResult;
			NewMouseOverObject = LineProxy->Src.ConnObj;
			NewMouseOverConnType = LineProxy->Src.ConnType;
			NewMouseOverConnIndex = LineProxy->Src.ConnIndex;
		}

	}

	if(	NewMouseOverObject != MouseOverObject || 
		NewMouseOverConnType != MouseOverConnType ||
		NewMouseOverConnIndex != MouseOverConnIndex )
	{
		MouseOverObject = NewMouseOverObject;
		MouseOverConnType = NewMouseOverConnType;
		MouseOverConnIndex = NewMouseOverConnIndex;
		MouseOverTime = FPlatformTime::Seconds();

		Viewport->InvalidateDisplay();
		EdInterface->OnMouseOver(MouseOverObject);
	}
}


bool FLinkedObjViewportClient::InputAxis(FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	bShowCursorOverride = true;
	bIsScrolling = false;

	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton) ? true : false;
	const bool MiddleMouseButtonDown = Viewport->KeyState(EKeys::MiddleMouseButton) ? true : false;
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton) ? true : false;
	const bool bMouseButtonDown = (LeftMouseButtonDown || MiddleMouseButtonDown || RightMouseButtonDown );

	bool bCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
	bool bShiftDown = Viewport->KeyState(EKeys::LeftShift) || Viewport->KeyState(EKeys::RightShift);

	// DeviceDelta is not constricted to mouse locks
	float DeviceDeltaX = (Key == EKeys::MouseX) ? Delta : 0;
	float DeviceDeltaY = (Key == EKeys::MouseY) ? -Delta : 0;

	// Mouse variables represent the actual (potentially constrained) location of the mouse
	int32 MouseX = Viewport->GetMouseX();	
	int32 MouseY = Viewport->GetMouseY();
	int32 MouseDeltaX = MouseX - OldMouseX;
	int32 MouseDeltaY = MouseY - OldMouseY;

	// Accumulate delta fractions, since these will get dropped when truncated to int32.
	DeltaXFraction += MouseDeltaX * (1.f/Zoom2D) - int32(MouseDeltaX * (1.f/Zoom2D));
	DeltaYFraction += MouseDeltaY * (1.f/Zoom2D) - int32(MouseDeltaY * (1.f/Zoom2D));
	int32 DeltaXAdd = int32(DeltaXFraction);
	int32 DeltaYAdd = int32(DeltaYFraction);
	DeltaXFraction -= DeltaXAdd;
	DeltaYFraction -= DeltaYAdd;

	bool bLeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton);
	bool bRightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton);

	if( Key == EKeys::MouseX || Key == EKeys::MouseY )
	{
		DistanceDragged += FMath::Abs(MouseDeltaX) + FMath::Abs(MouseDeltaY);
	}

	// If holding both buttons, we are zooming.
	if(bLeftMouseButtonDown && bRightMouseButtonDown)
	{
		if(Key == EKeys::MouseY)
		{
			//Always zoom around center for two button zoom
			float CenterOfZoomX = Viewport->GetSizeXY().X*0.5f;
			float CenterOfZoomY= Viewport->GetSizeXY().Y*0.5f;

			const float ZoomDelta = -Zoom2D * Delta * LinkedObjectEditor_ZoomSpeed;

			const float ViewCenterX = (CenterOfZoomX - (float)Origin2D.X)/Zoom2D;
			const float ViewCenterY = (CenterOfZoomY - (float)Origin2D.Y)/Zoom2D;

			Zoom2D = FMath::Clamp<float>(Zoom2D+ZoomDelta,MinZoom2D,MaxZoom2D);

			// We have a 'notch' around 1.f to make it easy to get back to normal zoom factor.
			if( FMath::Abs(Zoom2D - 1.f) < LinkedObjectEditor_ZoomNotchThresh )
			{
				Zoom2D = 1.f;
			}

			const float DrawOriginX = ViewCenterX - (CenterOfZoomX/Zoom2D);
			const float DrawOriginY = ViewCenterY - (CenterOfZoomY/Zoom2D);

			Origin2D.X = -FMath::Round(DrawOriginX * Zoom2D);
			Origin2D.Y = -FMath::Round(DrawOriginY * Zoom2D);

			EdInterface->ViewPosChanged();
			Viewport->Invalidate();
		}
	}
	else if(bLeftMouseButtonDown || bRightMouseButtonDown)
	{
		bool bInvalidate = false;
		if(bMakingLine)
		{
			NewX = MouseX;
			NewY = MouseY;
			bInvalidate = true;
		}
		else if(bBoxSelecting)
		{
			BoxEndX = MouseX + (BoxOrigin2D.X - Origin2D.X);
			BoxEndY = MouseY + (BoxOrigin2D.Y - Origin2D.Y);
			bInvalidate = true;
		}
		else if(bSpecialDrag)
		{
			FIntPoint MousePos( (MouseX - Origin2D.X)/Zoom2D, (MouseY - Origin2D.Y)/Zoom2D );
			EdInterface->SpecialDrag( MouseDeltaX * (1.f/Zoom2D) + DeltaXAdd, MouseDeltaY * (1.f/Zoom2D) + DeltaYAdd, MousePos.X, MousePos.Y, SpecialIndex );
			bInvalidate = true;
		}
		else if( bCtrlDown && EdInterface->HaveObjectsSelected() )
		{
			EdInterface->MoveSelectedObjects( MouseDeltaX * (1.f/Zoom2D) + DeltaXAdd, MouseDeltaY * (1.f/Zoom2D) + DeltaYAdd );

			// If haven't started a transaction, and moving some stuff, and have moved mouse far enough, start transaction now.
			if(!bTransactionBegun && DistanceDragged > 4)
			{
				EdInterface->BeginTransactionOnSelected();
				bTransactionBegun = true;
			}
			bInvalidate = true;
		}
		else if( bCtrlDown && bMovingConnector )
		{
			// A connector is being moved.  Calculate the delta it should move
			const float InvZoom2D = 1.f/Zoom2D;
			int32 DX = MouseDeltaX * InvZoom2D + DeltaXAdd;
			int32 DY = MouseDeltaY * InvZoom2D + DeltaYAdd;
			EdInterface->MoveSelectedConnLocation(DX,DY);
			bInvalidate = true;
		}
		else if(bAllowScroll && bHasMouseCapture)
		{
			//Default to using device delta
			int32 DeltaXForScroll;
			int32 DeltaYForScroll;
			if (GetDefault<ULevelEditorViewportSettings>()->bPanMovesCanvas)
			{
				//override to stay with the mouse. it's pixel accurate.
				DeltaXForScroll = DeviceDeltaX;
				DeltaYForScroll = DeviceDeltaY;
				if (DeltaXForScroll || DeltaYForScroll)
				{
					MarkMouseMovedSinceClick();
				}
				//assign both so the updates work right (now) AND the assignment at the bottom works (after).
				OldMouseX = MouseX = OldMouseX + DeviceDeltaX;
				OldMouseY = MouseY = OldMouseY + DeviceDeltaY;

				bShowCursorOverride = UpdateCursorVisibility();
				bIsScrolling = true;
				UpdateMousePosition();
			}
			else
			{
				DeltaXForScroll = -DeviceDeltaX;
				DeltaYForScroll = -DeviceDeltaY;
			}

			Origin2D.X += DeltaXForScroll;
			Origin2D.Y += DeltaYForScroll;
			EdInterface->ViewPosChanged();
			bInvalidate = true;
		}

		OnMouseOver( MouseX, MouseY );

		if ( bInvalidate )
		{
			Viewport->InvalidateDisplay();
		}
	}

	//save the latest mouse position
	OldMouseX = MouseX;
	OldMouseY = MouseY;

	return true;
}

EMouseCursor::Type FLinkedObjViewportClient::GetCursor(FViewport* Viewport, int32 X, int32 Y)
{
	bool bLeftDown  = Viewport->KeyState(EKeys::LeftMouseButton) ? true : false;
	bool bRightDown = Viewport->KeyState(EKeys::RightMouseButton) ? true : false;

	//if we're allowed to scroll, we are in "canvas move mode" and ONLY one mouse button is down
	if ((bAllowScroll) && GetDefault<ULevelEditorViewportSettings>()->bPanMovesCanvas && (bLeftDown ^ bRightDown))
	{
		const bool bCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
	
		//double check there is no other overriding operation (other than panning)
		if (!bMakingLine && !bBoxSelecting && !bSpecialDrag)
		{
			if ((bCtrlDown && EdInterface->HaveObjectsSelected()))
			{
				return EMouseCursor::CardinalCross;
			}
			else if (bHasMouseMovedSinceClick)
			{
				return EMouseCursor::Hand;
			}
		}
	}

	return EMouseCursor::Default;
}


/**
 * Sets the cursor to be visible or not.  Meant to be called as the mouse moves around in "move canvas" mode (not just on button clicks)
 */
bool FLinkedObjViewportClient::UpdateCursorVisibility (void)
{
	bool bShowCursor = ShouldCursorBeVisible();

	bool bCursorWasVisible = Viewport->IsCursorVisible() ;
	Viewport->ShowCursor( bShowCursor);

	//first time showing the cursor again.  Update old mouse position so there isn't a jump as well.
	if (!bCursorWasVisible && bShowCursor)
	{
		OldMouseX = Viewport->GetMouseX();
		OldMouseY = Viewport->GetMouseY();
	}

	return bShowCursor;
}

/**
 * Given that we're in "move canvas" mode, set the snap back visible mouse position to clamp to the viewport
 */
void FLinkedObjViewportClient::UpdateMousePosition(void)
{
	const int32 SizeX = Viewport->GetSizeXY().X;
	const int32 SizeY = Viewport->GetSizeXY().Y;

	int32 ClampedMouseX = FMath::Clamp<int32>(OldMouseX, 0, SizeX);
	int32 ClampedMouseY = FMath::Clamp<int32>(OldMouseY, 0, SizeY);

	Viewport->SetMouse(ClampedMouseX, ClampedMouseY);
}

/** Determines if the cursor should presently be visible
 * @return - true if the cursor should remain visible
 */
bool FLinkedObjViewportClient::ShouldCursorBeVisible(void)
{
	bool bLeftDown  = Viewport->KeyState(EKeys::LeftMouseButton) ? true : false;
	bool bRightDown = Viewport->KeyState(EKeys::RightMouseButton) ? true : false;
	bool bCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);

	const int32 SizeX = Viewport->GetSizeXY().X;
	const int32 SizeY = Viewport->GetSizeXY().Y;

	bool bInViewport = FMath::IsWithin<int32>(OldMouseX, 1, SizeX-1) && FMath::IsWithin<int32>(OldMouseY, 1, SizeY-1);

	//both mouse button zoom hides mouse as well
	bool bShowMouseOnScroll = (!bAllowScroll) || GetDefault<ULevelEditorViewportSettings>()->bPanMovesCanvas && (bLeftDown ^ bRightDown) && bInViewport;
	//if scrolling isn't allowed, or we're in "inverted" pan mode, lave the mouse visible
	bool bHideCursor = !bMakingLine && !bBoxSelecting && !bSpecialDrag && !(bCtrlDown && EdInterface->HaveObjectsSelected()) && !bShowMouseOnScroll;

	return !bHideCursor;

}
/** 
 *	See if cursor is in 'scroll' region around the edge, and if so, scroll the view automatically. 
 *	Returns the distance that the view was moved.
 */
FIntPoint FLinkedObjViewportClient::DoScrollBorder(float DeltaTime)
{
	FIntPoint Result( 0, 0 );

	if (bAllowScroll)
	{
		const int32 PosX = Viewport->GetMouseX();
		const int32 PosY = Viewport->GetMouseY();
		const int32 SizeX = Viewport->GetSizeXY().X;
		const int32 SizeY = Viewport->GetSizeXY().Y;

		DeltaTime = FMath::Clamp(DeltaTime, 0.01f, 1.0f);

		if(PosX < LinkedObjectEditor_ScrollBorderSize)
		{
			ScrollAccum.X += (1.f - ((float)PosX/(float)LinkedObjectEditor_ScrollBorderSize)) * LinkedObjectEditor_ScrollBorderSpeed * DeltaTime;
		}
		else if(PosX > SizeX - LinkedObjectEditor_ScrollBorderSize)
		{
			ScrollAccum.X -= ((float)(PosX - (SizeX - LinkedObjectEditor_ScrollBorderSize))/(float)LinkedObjectEditor_ScrollBorderSize) * LinkedObjectEditor_ScrollBorderSpeed * DeltaTime;
		}
		else
		{
			ScrollAccum.X = 0.f;
		}

		float ScrollY = 0.f;
		if(PosY < LinkedObjectEditor_ScrollBorderSize)
		{
			ScrollAccum.Y += (1.f - ((float)PosY/(float)LinkedObjectEditor_ScrollBorderSize)) * LinkedObjectEditor_ScrollBorderSpeed * DeltaTime;
		}
		else if(PosY > SizeY - LinkedObjectEditor_ScrollBorderSize)
		{
			ScrollAccum.Y -= ((float)(PosY - (SizeY - LinkedObjectEditor_ScrollBorderSize))/(float)LinkedObjectEditor_ScrollBorderSize) * LinkedObjectEditor_ScrollBorderSpeed * DeltaTime;
		}
		else
		{
			ScrollAccum.Y = 0.f;
		}

		// Apply integer part of ScrollAccum to origin, and save the rest.
		const int32 MoveX = FMath::Floor(ScrollAccum.X);
		Origin2D.X += MoveX;
		ScrollAccum.X -= MoveX;

		const int32 MoveY = FMath::Floor(ScrollAccum.Y);
		Origin2D.Y += MoveY;
		ScrollAccum.Y -= MoveY;

		// Update the box selection if necessary
		if (bBoxSelecting)
		{
			BoxEndX += MoveX;
			BoxEndY += MoveY;
		}

		// If view has changed, notify the app and redraw the viewport.
		if( FMath::Abs<int32>(MoveX) > 0 || FMath::Abs<int32>(MoveY) > 0 )
		{
			EdInterface->ViewPosChanged();
			Viewport->Invalidate();
		}
		Result = FIntPoint(MoveX, MoveY);
	}

	return Result;
}

/**
 * Sets whether or not the viewport should be invalidated in Tick().
 */
void FLinkedObjViewportClient::SetRedrawInTick(bool bInAlwaysDrawInTick)
{
	bAlwaysDrawInTick = bInAlwaysDrawInTick;
}

void FLinkedObjViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	// Auto-scroll display if moving/drawing etc. and near edge.
	bool bCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
	if(	bMouseDown )		
	{
		// If holding both buttons, we are zooming.
		if(Viewport->KeyState(EKeys::RightMouseButton) && Viewport->KeyState(EKeys::LeftMouseButton))
		{
		}
		else if(bMakingLine || bBoxSelecting)
		{
			DoScrollBorder(DeltaSeconds);
		}
		else if(bSpecialDrag)
		{
			FIntPoint Delta = DoScrollBorder(DeltaSeconds);
			if(Delta.Size() > 0)
			{
				EdInterface->SpecialDrag( -Delta.X * (1.f/Zoom2D), -Delta.Y * (1.f/Zoom2D), 0, 0, SpecialIndex ); // TODO fix mouse position in this case.
			}
		}
		else if(bCtrlDown && EdInterface->HaveObjectsSelected())
		{
			FIntPoint Delta = DoScrollBorder(DeltaSeconds);

			// In the case of dragging boxes around, we move them as well when dragging at the edge of the screen.
			EdInterface->MoveSelectedObjects( -Delta.X * (1.f/Zoom2D), -Delta.Y * (1.f/Zoom2D) );

			DistanceDragged += ( FMath::Abs<int32>(Delta.X) + FMath::Abs<int32>(Delta.Y) );

			if(!bTransactionBegun && DistanceDragged > 4)
			{
				EdInterface->BeginTransactionOnSelected();
				bTransactionBegun = true;
			}
		}
	}

	// Pan to DesiredOrigin2D within DesiredPanTime seconds.
	if( DesiredPanTime > 0.f )
	{
		Origin2D.X = FMath::Lerp( Origin2D.X, DesiredOrigin2D.X, FMath::Min(DeltaSeconds/DesiredPanTime,1.f) );
		Origin2D.Y = FMath::Lerp( Origin2D.Y, DesiredOrigin2D.Y, FMath::Min(DeltaSeconds/DesiredPanTime,1.f) );
		DesiredPanTime -= DeltaSeconds;
		Viewport->InvalidateDisplay();
	}
	else if ( bAlwaysDrawInTick  )
	{
		Viewport->InvalidateDisplay();
	}

	if (MouseOverObject 
		&& ToolTipDelayMS > 0
		&& (FPlatformTime::Seconds() -  MouseOverTime) > ToolTipDelayMS * .001f)
	{
		// Redraw so that tooltips can be displayed
		Viewport->InvalidateDisplay();
	}
}
