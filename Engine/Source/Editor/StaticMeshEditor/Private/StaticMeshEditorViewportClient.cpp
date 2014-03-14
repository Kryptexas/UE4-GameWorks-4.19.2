// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "StaticMeshEditorModule.h"
#include "StaticMeshEditorActions.h"

#include "MouseDeltaTracker.h"
#include "ISocketManager.h"
#include "StaticMeshEditorViewportClient.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "StaticMeshResources.h"
#include "RawMesh.h"


#include "StaticMeshEditor.h"
#include "BusyCursor.h"
#include "MeshBuild.h"
#include "PreviewScene.h"
#include "ObjectTools.h"

#if WITH_PHYSX
#include "Editor/UnrealEd/Private/EditorPhysXSupport.h"
#endif

#define LOCTEXT_NAMESPACE "FStaticMeshEditorViewportClient"

#define HITPROXY_SOCKET	1

namespace {
	static const float LightRotSpeed = 0.22f;
	static const float StaticMeshEditor_RotateSpeed = 0.01f;
	static const float	StaticMeshEditor_TranslateSpeed = 0.25f;
	static const float GridSize = 2048.0f;
	static const int32 CellSize = 16;
	static const float AutoViewportOrbitCameraTranslate = 256.0f;

	static float AmbientCubemapIntensity = 0.4f;
}

FStaticMeshEditorViewportClient::FStaticMeshEditorViewportClient(TWeakPtr<IStaticMeshEditor> InStaticMeshEditor, FPreviewScene& InPreviewScene, UStaticMesh* InPreviewStaticMesh, UStaticMeshComponent* InPreviewStaticMeshComponent)
	: FEditorViewportClient(&InPreviewScene)
	, StaticMeshEditorPtr(InStaticMeshEditor)
{
	SimplygonLogo = LoadObject<UTexture2D>(NULL, TEXT("/Engine/EditorResources/SimplygonLogo.SimplygonLogo"), NULL, LOAD_None, NULL);

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

	SetViewMode(VMI_Lit);

	WidgetMode = FWidget::WM_None;

	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.SetSnap(0);
	EngineShowFlags.CompositeEditorPrimitives = true;
	OverrideNearClipPlane(1.0f);
	bUsingOrbitCamera = true;

	bShowSockets = true;
	bDrawUVs = false;
	bDrawNormals = false;
	bDrawTangents = false;
	bDrawBinormals = false;
	bShowPivot = false;
	bDrawAdditionalData = true;

	bManipulating = false;

	SetPreviewMesh(InPreviewStaticMesh, InPreviewStaticMeshComponent);
}

void FStaticMeshEditorViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	// Tick the preview scene world.
	if (!GIntraFrameDebuggingGameThread)
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

/**
 * A hit proxy class for sockets.
 */
struct HSMESocketProxy : public HHitProxy
{
	DECLARE_HIT_PROXY( );

	int32		SocketIndex;

	HSMESocketProxy(int32 InSocketIndex)
		:	HHitProxy( HPP_UI )
		,	SocketIndex( InSocketIndex )
	{}
};
IMPLEMENT_HIT_PROXY(HSMESocketProxy, HHitProxy);

bool FStaticMeshEditorViewportClient::InputWidgetDelta( FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale )
{
	bool bHandled = false;
	if (bManipulating)
	{
		UStaticMeshSocket* SelectedSocket = StaticMeshEditorPtr.Pin()->GetSelectedSocket();

		if(SelectedSocket && CurrentAxis != EAxisList::None)
		{
			UProperty* ChangedProperty = NULL;
			const FWidget::EWidgetMode MoveMode = GetWidgetMode();
			if(MoveMode == FWidget::WM_Rotate)
			{
				ChangedProperty = FindField<UProperty>( UStaticMeshSocket::StaticClass(), "RelativeRotation" );
				SelectedSocket->PreEditChange(ChangedProperty);

				FRotator CurrentRot = SelectedSocket->RelativeRotation;
				FRotator SocketWinding, SocketRotRemainder;
				CurrentRot.GetWindingAndRemainder(SocketWinding, SocketRotRemainder);

				const FQuat ActorQ = SocketRotRemainder.Quaternion();
				const FQuat DeltaQ = Rot.Quaternion();
				const FQuat ResultQ = DeltaQ * ActorQ;
				const FRotator NewSocketRotRem = FRotator( ResultQ );
				FRotator DeltaRot = NewSocketRotRem - SocketRotRemainder;
				DeltaRot.Normalize();

				SelectedSocket->RelativeRotation += DeltaRot;
				SelectedSocket->RelativeRotation.Pitch = FRotator::ClampAxis( SelectedSocket->RelativeRotation.Pitch );
				SelectedSocket->RelativeRotation.Yaw = FRotator::ClampAxis( SelectedSocket->RelativeRotation.Yaw );
				SelectedSocket->RelativeRotation.Roll = FRotator::ClampAxis( SelectedSocket->RelativeRotation.Roll );
			}
			else if(MoveMode == FWidget::WM_Translate)
			{
				ChangedProperty = FindField<UProperty>( UStaticMeshSocket::StaticClass(), "RelativeLocation" );
				SelectedSocket->PreEditChange(ChangedProperty);

				//FRotationMatrix SocketRotTM( SelectedSocket->RelativeRotation );
				//FVector SocketMove = SocketRotTM.TransformVector( Drag );

				SelectedSocket->RelativeLocation += Drag;
			}
			if ( ChangedProperty )
			{			
				FPropertyChangedEvent PropertyChangedEvent( ChangedProperty );
				SelectedSocket->PostEditChangeProperty(PropertyChangedEvent);
			}

			StaticMeshEditorPtr.Pin()->GetStaticMesh()->MarkPackageDirty();
		}
		Invalidate();		
		bHandled = true;
	}

	return bHandled;
}

void FStaticMeshEditorViewportClient::TrackingStarted( const struct FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge )
{
	if( !bManipulating && bIsDraggingWidget )
	{
		if( GetWidgetMode() == FWidget::WM_Rotate )
		{
			GEditor->BeginTransaction( LOCTEXT("FStaticMeshEditorViewportClient_RotateSocket", "Rotate Socket" ) );
		}
		else
		{
			if( InInputState.IsLeftMouseButtonPressed() && (Widget->GetCurrentAxis() & EAxisList::XYZ) )
			{
				const bool bAltDown = InInputState.IsAltButtonPressed();
				if ( bAltDown )
				{
					// Rather than moving/rotating the selected socket, copy it and move the copy instead
					StaticMeshEditorPtr.Pin()->DuplicateSelectedSocket();
				}
			}

			GEditor->BeginTransaction( LOCTEXT("FStaticMeshEditorViewportClient_TranslateSocket", "Translate Socket" ) );
		}	
		
		bManipulating = true;
	}

}

FWidget::EWidgetMode FStaticMeshEditorViewportClient::GetWidgetMode() const
{
	UStaticMeshSocket* SelectedSocket = StaticMeshEditorPtr.Pin()->GetSelectedSocket();
	if( SelectedSocket )
	{
		return WidgetMode;
	}

	return FWidget::WM_None;
}

void FStaticMeshEditorViewportClient::TrackingStopped()
{
	if( bManipulating )
	{
		bManipulating = false;
		GEditor->EndTransaction();
	}
}

FVector FStaticMeshEditorViewportClient::GetWidgetLocation() const
{
	UStaticMeshSocket* SelectedSocket = StaticMeshEditorPtr.Pin()->GetSelectedSocket();
	if( SelectedSocket )
	{
		FMatrix SocketTM;
		SelectedSocket->GetSocketMatrix(SocketTM, StaticMeshComponent);

		return SocketTM.GetOrigin();
	}

	return FVector::ZeroVector;
}

FMatrix FStaticMeshEditorViewportClient::GetWidgetCoordSystem() const 
{
	UStaticMeshSocket* SelectedSocket = StaticMeshEditorPtr.Pin()->GetSelectedSocket();
	if( SelectedSocket )
	{
		//FMatrix SocketTM;
		//SelectedSocket->GetSocketMatrix(SocketTM, StaticMeshComponent);

		return FRotationMatrix( SelectedSocket->RelativeRotation );
	}

	return FMatrix::Identity;
}

bool FStaticMeshEditorViewportClient::ShouldOrbitCamera() const
{
	if (GetDefault<ULevelEditorViewportSettings>()->bUseUE3OrbitControls)
	{
		// this editor orbits always if ue3 orbit controls are enabled
		return true;
	}

	return FEditorViewportClient::ShouldOrbitCamera();
}

void FStaticMeshEditorViewportClient::Draw(const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);

	if( bShowSockets )
	{
		for(int32 i=0; i < StaticMesh->Sockets.Num(); i++)
		{
			UStaticMeshSocket* Socket = StaticMesh->Sockets[i];
			if(Socket)
			{
				FMatrix SocketTM;
				Socket->GetSocketMatrix(SocketTM, StaticMeshComponent);
				PDI->SetHitProxy( new HSMESocketProxy(i) );
				DrawWireDiamond(PDI, SocketTM, 5.f, FColor(255,128,128), SDPG_Foreground );
				PDI->SetHitProxy( NULL );
			}
		}
	}

	// Draw any edges that are currently selected by the user
	if( SelectedEdgeIndices.Num() > 0 )
	{
		for(int32 VertexIndex = 0; VertexIndex < SelectedEdgeVertices.Num(); VertexIndex += 2)
		{
			FVector EdgeVertices[ 2 ];
			EdgeVertices[ 0 ] = SelectedEdgeVertices[VertexIndex];
			EdgeVertices[ 1 ] = SelectedEdgeVertices[VertexIndex + 1];

			PDI->DrawLine(
				StaticMeshComponent->ComponentToWorld.TransformPosition( EdgeVertices[ 0 ] ),
				StaticMeshComponent->ComponentToWorld.TransformPosition( EdgeVertices[ 1 ] ),
				FColor( 255, 255, 0 ),
				SDPG_World );
		}
	}


	if( bDrawNormals || bDrawTangents || bDrawBinormals )
	{
		FStaticMeshLODResources& LODModel = StaticMesh->RenderData->LODResources[StaticMeshEditorPtr.Pin()->GetCurrentLODIndex()];
		FIndexArrayView Indices = LODModel.IndexBuffer.GetArrayView();
		uint32 NumIndices = Indices.Num();

		FMatrix LocalToWorldInverseTranspose = StaticMeshComponent->ComponentToWorld.ToMatrixWithScale().Inverse().GetTransposed();
		for (uint32 i = 0; i < NumIndices; i++)
		{
			const FVector& VertexPos = LODModel.PositionVertexBuffer.VertexPosition( Indices[i] );

			const FVector WorldPos = StaticMeshComponent->ComponentToWorld.TransformPosition( VertexPos );
			const FVector& Normal = LODModel.VertexBuffer.VertexTangentZ( Indices[i] ); 
			const FVector& Binormal = LODModel.VertexBuffer.VertexTangentY( Indices[i] ); 
			const FVector& Tangent = LODModel.VertexBuffer.VertexTangentX( Indices[i] ); 

			const float Len = 5.0f;

			if( bDrawNormals )
			{
				PDI->DrawLine( WorldPos, WorldPos+LocalToWorldInverseTranspose.TransformVector( Normal ).SafeNormal() * Len, FLinearColor( 0.0f, 1.0f, 0.0f), SDPG_World );
			}

			if( bDrawTangents )
			{
				PDI->DrawLine( WorldPos, WorldPos+LocalToWorldInverseTranspose.TransformVector( Tangent ).SafeNormal() * Len, FLinearColor( 1.0f, 0.0f, 0.0f), SDPG_World );
			}

			if( bDrawBinormals )
			{
				PDI->DrawLine( WorldPos, WorldPos+LocalToWorldInverseTranspose.TransformVector( Binormal ).SafeNormal() * Len, FLinearColor( 0.0f, 0.0f, 1.0f), SDPG_World );
			}
		}	
	}


	if( bShowPivot )
	{
		FUnrealEdUtils::DrawWidget(View, PDI, StaticMeshComponent->ComponentToWorld.ToMatrixWithScale(), 0, 0, EAxisList::All, EWidgetMovementMode::WMM_Translate, false);
	}

	if( bDrawAdditionalData )
	{
		const TArray<UAssetUserData*>* UserDataArray = StaticMesh->GetAssetUserDataArray();
		if (UserDataArray != NULL)
		{
			for (int32 AdditionalDataIndex = 0; AdditionalDataIndex < UserDataArray->Num(); ++AdditionalDataIndex)
			{
				if ((*UserDataArray)[AdditionalDataIndex] != NULL)
				{
					(*UserDataArray)[AdditionalDataIndex]->Draw(PDI, View);
				}
			}
		}
	}
}

static void DrawAngles(FCanvas* Canvas, int32 XPos, int32 YPos, EAxisList::Type ManipAxis, FWidget::EWidgetMode MoveMode, const FRotator& Rotation, const FVector& Translation)
{
	FString OutputString(TEXT(""));
	if (MoveMode == FWidget::WM_Rotate && Rotation.IsZero() == false)
	{
		//Only one value moves at a time
		const FVector EulerAngles = Rotation.Euler();
		if (ManipAxis == EAxisList::X)
		{
			OutputString += FString::Printf(TEXT("Roll: %0.2f"), EulerAngles.X);
		}
		else if (ManipAxis == EAxisList::Y)
		{
			OutputString += FString::Printf(TEXT("Pitch: %0.2f"), EulerAngles.Y);
		}
		else if (ManipAxis == EAxisList::Z)
		{
			OutputString += FString::Printf(TEXT("Yaw: %0.2f"), EulerAngles.Z);
		}
	}
	else if (MoveMode == FWidget::WM_Translate && Translation.IsZero() == false)
	{
		//Only one value moves at a time
		if (ManipAxis == EAxisList::X)
		{
			OutputString += FString::Printf(TEXT(" %0.2f"), Translation.X);
		}
		else if (ManipAxis == EAxisList::Y)
		{
			OutputString += FString::Printf(TEXT(" %0.2f"), Translation.Y);
		}
		else if (ManipAxis == EAxisList::Z)
		{
			OutputString += FString::Printf(TEXT(" %0.2f"), Translation.Z);
		}
	}

	if (OutputString.Len() > 0)
	{
		FCanvasTextItem TextItem( FVector2D(XPos, YPos), FText::FromString( OutputString ), GEngine->GetSmallFont(), FLinearColor::White );
		Canvas->DrawItem( TextItem );
	}
}

void FStaticMeshEditorViewportClient::DrawCanvas( FViewport& InViewport, FSceneView& View, FCanvas& Canvas )
{
#if TODO_STATICMESH
	if ( StaticMesh->bHasBeenSimplified && SimplygonLogo && SimplygonLogo->Resource )
	{
		const float LogoSizeX = 64.0f;
		const float LogoSizeY = 40.65f;
		const float Padding = 6.0f;
		const float LogoX = Viewport->GetSizeXY().X - Padding - LogoSizeX;
		const float LogoY = Viewport->GetSizeXY().Y - Padding - LogoSizeY;

		Canvas->DrawTile(
			LogoX,
			LogoY,
			LogoSizeX,
			LogoSizeY,
			0.0f,
			0.0f,
			1.0f,
			1.0f,
			FLinearColor::White,
			SimplygonLogo->Resource,
			SE_BLEND_Opaque );
	}
#endif // #if TODO_STATICMESH


	const int32 HalfX = Viewport->GetSizeXY().X/2;
	const int32 HalfY = Viewport->GetSizeXY().Y/2;

	// Draw socket names if desired.
	if( bShowSockets )
	{
		for(int32 i=0; i<StaticMesh->Sockets.Num(); i++)
		{
			UStaticMeshSocket* Socket = StaticMesh->Sockets[i];
			if(Socket!=NULL)
			{
				FMatrix SocketTM;
				Socket->GetSocketMatrix(SocketTM, StaticMeshComponent);
				const FVector SocketPos	= SocketTM.GetOrigin();
				const FPlane proj		= View.Project( SocketPos );
				if(proj.W > 0.f)
				{
					const int32 XPos = HalfX + ( HalfX * proj.X );
					const int32 YPos = HalfY + ( HalfY * (proj.Y * -1) );

					FCanvasTextItem TextItem( FVector2D( XPos, YPos ), FText::FromString( Socket->SocketName.ToString() ), GEngine->GetSmallFont(), FLinearColor(FColor(255,196,196)) );
					Canvas.DrawItem( TextItem );	

					if (bManipulating && StaticMeshEditorPtr.Pin()->GetSelectedSocket() == Socket)
					{
						//Figure out the text height
						FTextSizingParameters Parameters(GEngine->GetSmallFont(), 1.0f, 1.0f);
						UCanvas::CanvasStringSize(Parameters, *Socket->SocketName.ToString());
						int32 YL = FMath::Trunc(Parameters.DrawYL);

						DrawAngles(&Canvas, XPos, YPos + YL, 
							Widget->GetCurrentAxis(), 
							GetWidgetMode(),
							Socket->RelativeRotation,
							Socket->RelativeLocation);
					}
				}
			}
		}
	}

	int32 YPos = 6;
	int32 CurrentLODLevel = StaticMeshEditorPtr.Pin()->GetCurrentLODLevel();
	if (CurrentLODLevel == 0)
	{
		// NOTE: Does not account for MaxDrawDistanceScale.
		float DistanceSquared = (StaticMeshComponent->Bounds.Origin - View.ViewMatrices.ViewOrigin).SizeSquared();
		const float LODFactorDistanceSquared = DistanceSquared * FMath::Square(View.LODDistanceFactor);
		while (CurrentLODLevel < MAX_STATIC_MESH_LODS && LODFactorDistanceSquared >= FMath::Square(StaticMesh->RenderData->LODDistance[CurrentLODLevel+1]))
		{
			CurrentLODLevel++;
		}
	}
	else
	{
		CurrentLODLevel -= 1;
	}

	Canvas.DrawShadowedString(
		6,
		YPos,
		*FText::Format( NSLOCTEXT("UnrealEd", "LOD_F", "LOD:  {0}"), FText::AsNumber(CurrentLODLevel) ).ToString(),
		GEngine->GetSmallFont(),
		FLinearColor::White
		);
	YPos += 18;

	Canvas.DrawShadowedString(
		6,
		YPos,
		*FText::Format( NSLOCTEXT("UnrealEd", "Triangles_F", "Triangles:  {0}"), FText::AsNumber(StaticMeshEditorPtr.Pin()->GetNumTriangles(CurrentLODLevel)) ).ToString(),
		GEngine->GetSmallFont(),
		FLinearColor::White
		);
	YPos += 18;

	Canvas.DrawShadowedString(
		6,
		YPos,
		*FText::Format( NSLOCTEXT("UnrealEd", "Vertices_F", "Vertices:  {0}"), FText::AsNumber(StaticMeshEditorPtr.Pin()->GetNumVertices(CurrentLODLevel)) ).ToString(),
		GEngine->GetSmallFont(),
		FLinearColor::White
		);
	YPos += 18;

	Canvas.DrawShadowedString(
		6,
		YPos,
		*FText::Format( NSLOCTEXT("UnrealEd", "UVChannels_F", "UV Channels:  {0}"), FText::AsNumber(StaticMeshEditorPtr.Pin()->GetNumUVChannels(CurrentLODLevel)) ).ToString(),
		GEngine->GetSmallFont(),
		FLinearColor::White
		);
	YPos += 18;

	Canvas.DrawShadowedString(
		6,
		YPos,
		*FText::Format( NSLOCTEXT("UnrealEd", "ApproxSize_F", "Approx Size: {0}x{1}x{2}"),
			FText::AsNumber(int32(StaticMesh->GetBounds().BoxExtent.X * 2.0f)), // x2 as artists wanted length not radius
			FText::AsNumber(int32(StaticMesh->GetBounds().BoxExtent.Y * 2.0f)),
			FText::AsNumber(int32(StaticMesh->GetBounds().BoxExtent.Z * 2.0f)) ).ToString(),
		GEngine->GetSmallFont(),
		FLinearColor::White
		);
	YPos += 18;

	// Show the number of collision primitives if we are drawing collision.
	if(EngineShowFlags.Collision && StaticMesh->BodySetup)
	{
		Canvas.DrawShadowedString(
			6,
			YPos,
			*FText::Format( NSLOCTEXT("UnrealEd", "NumPrimitives_F", "Num Primitives:  {0}"), FText::AsNumber(StaticMesh->BodySetup->AggGeom.GetElementCount()) ).ToString(),
			GEngine->GetSmallFont(),
			FLinearColor::White
			);
		YPos += 18;
	}

	if(bDrawUVs)
	{
		DrawUVsForMesh(Viewport, &Canvas, YPos);
	}
}

void FStaticMeshEditorViewportClient::DrawUVsForMesh(FViewport* InViewport, FCanvas* InCanvas, int32 InTextYPos )
{
	//use the overriden LOD level
	const uint32 LODLevel = FMath::Clamp(StaticMeshComponent->ForcedLodModel - 1, 0, StaticMesh->RenderData->LODResources.Num() - 1);

	int32 UVChannel = StaticMeshEditorPtr.Pin()->GetCurrentUVChannel();

	DrawUVs(InViewport, InCanvas, InTextYPos, LODLevel, UVChannel, SelectedEdgeTexCoords[UVChannel], StaticMeshComponent->StaticMesh->RenderData, NULL);
}

void FStaticMeshEditorViewportClient::MouseMove(FViewport* Viewport,int32 x, int32 y)
{
	FEditorViewportClient::MouseMove(Viewport,x,y);
}

bool FStaticMeshEditorViewportClient::InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event,float AmountDepressed,bool Gamepad)
{
	bool bHandled = false;

	const int32 HitX = Viewport->GetMouseX();
	const int32 HitY = Viewport->GetMouseY();
	const bool bAltDown = Viewport->KeyState(EKeys::LeftAlt) || Viewport->KeyState(EKeys::RightAlt);
	const bool bCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
	const bool bShiftDown = Viewport->KeyState(EKeys::LeftShift) || Viewport->KeyState(EKeys::RightShift);

	if( Event == IE_Pressed) 
	{
		if(Key == EKeys::SpaceBar)
		{
			if( StaticMeshEditorPtr.Pin()->GetSelectedSocket() && !Widget->IsDragging() )
			{
				const FWidget::EWidgetMode MoveMode = GetWidgetMode();
				if(MoveMode == FWidget::WM_Rotate)
				{
					WidgetMode = FWidget::WM_Translate;
				}
				else if(MoveMode == FWidget::WM_Translate)
				{
					WidgetMode = FWidget::WM_Rotate;
				}
				Invalidate();
			}
			bHandled = true;
		}
	}


	if( !bHandled )
	{
		bHandled = FEditorViewportClient::InputKey( Viewport, ControllerId, Key, Event, AmountDepressed, false );
	}

	// Handle viewport screenshot.
	bHandled |= InputTakeScreenshot( Viewport, Key, Event );

	return bHandled;
}

bool FStaticMeshEditorViewportClient::InputAxis(FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	return FEditorViewportClient::InputAxis(Viewport,ControllerId,Key,Delta,DeltaTime,NumSamples,bGamepad);
}

void FStaticMeshEditorViewportClient::ProcessClick(class FSceneView& InView, class HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	if( HitProxy && HitProxy->IsA( HSMESocketProxy::StaticGetType() ) )
	{
		HSMESocketProxy* SocketProxy = (HSMESocketProxy*)HitProxy;

		UStaticMeshSocket* Socket = NULL;

		if(SocketProxy->SocketIndex < StaticMesh->Sockets.Num())
		{
			Socket = StaticMesh->Sockets[SocketProxy->SocketIndex];
		}

		if(Socket)
		{
			StaticMeshEditorPtr.Pin()->SetSelectedSocket(Socket);
		}
		

		Invalidate();
	}
	else if(!HitProxy)
	{
		const bool bCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
		const bool bShiftDown = Viewport->KeyState(EKeys::LeftShift) || Viewport->KeyState(EKeys::RightShift);

		if(!bCtrlDown && !bShiftDown)
		{
			SelectedEdgeIndices.Empty();
		}

		// Check to see if we clicked on a mesh edge
		if( StaticMeshComponent != NULL && Viewport->GetSizeXY().X > 0 && Viewport->GetSizeXY().Y > 0 )
		{
			FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues( Viewport, GetScene(), EngineShowFlags ));
			FSceneView* View = CalcSceneView(&ViewFamily);
			FViewportClick ViewportClick(View, this, Key, Event, HitX, HitY);

			const FVector ClickLineStart( ViewportClick.GetOrigin() );
			const FVector ClickLineEnd( ViewportClick.GetOrigin() + ViewportClick.GetDirection() * HALF_WORLD_MAX );

			// Don't bother doing a line check as there is only one mesh in the SME and it makes fuzzy selection difficult
			// 	FHitResult CheckResult( 1.0f );
			// 	if( StaticMeshComponent->LineCheck(
			// 			CheckResult,	// In/Out: Result
			// 			ClickLineEnd,	// Target
			// 			ClickLineStart,	// Source
			// 			FVector::ZeroVector,	// Extend
			// 			TRACE_ComplexCollision ) )	// Trace flags
			{
				// @todo: Should be in screen space ideally
				const float WorldSpaceMinClickDistance = 100.0f;

				float ClosestEdgeDistance = FLT_MAX;
				TArray< int32 > ClosestEdgeIndices;
				FVector ClosestEdgeVertices[ 2 ];

				const uint32 LODLevel = FMath::Clamp( StaticMeshComponent->ForcedLodModel - 1, 0, StaticMeshComponent->StaticMesh->GetNumLODs() - 1 );
				FRawMesh RawMesh;
				StaticMeshComponent->StaticMesh->SourceModels[LODLevel].RawMeshBulkData->LoadRawMesh(RawMesh);

				const int32 RawEdgeCount = RawMesh.WedgeIndices.Num() - 1; 
				const int32 NumFaces = RawMesh.WedgeIndices.Num() / 3;
				int32 NumBackFacingTriangles = 0;
				for(int32 FaceIndex = 0; FaceIndex < NumFaces; ++FaceIndex)
				{
					// We disable edge selection where all adjoining triangles are back face culled and the 
					// material is not two-sided. This prevents edges that are back-face culled from being selected.
					bool bIsBackFacing = false;
					bool bIsTwoSided = false;
					UMaterialInterface* Material = StaticMeshComponent->GetMaterial(RawMesh.FaceMaterialIndices[FaceIndex]);
					if (Material && Material->GetMaterial())
					{
						bIsTwoSided = Material->IsTwoSided();
					}
					if(!bIsTwoSided)
					{
						// Check whether triangle if back facing 
						const FVector A = RawMesh.GetWedgePosition( FaceIndex * 3);
						const FVector B = RawMesh.GetWedgePosition( FaceIndex * 3 + 1);
						const FVector C = RawMesh.GetWedgePosition( FaceIndex * 3 + 2);
								
						// Compute the per-triangle normal
						const FVector BA = A - B;
						const FVector CA = A - C;
						const FVector TriangleNormal = (CA ^ BA).SafeNormal();

						// Transform the view position from world to component space
						const FVector ComponentSpaceViewOrigin = StaticMeshComponent->ComponentToWorld.InverseTransformPosition( View->ViewMatrices.ViewOrigin);
								
						// Determine which side of the triangle's plane that the view position lies on.
						bIsBackFacing = (FVector::PointPlaneDist( ComponentSpaceViewOrigin,  A, TriangleNormal)  < 0.0f);
					}
						
					for( int32 VertIndex = 0; VertIndex < 3; ++VertIndex )
					{
						const int32 EdgeIndex = FaceIndex * 3 + VertIndex;
						const int32 EdgeIndex2 = FaceIndex * 3 + ((VertIndex + 1) % 3);

						FVector EdgeVertices[ 2 ];
						EdgeVertices[0]	= RawMesh.GetWedgePosition(EdgeIndex);
						EdgeVertices[1] = RawMesh.GetWedgePosition(EdgeIndex2);

						// First check to see if this edge is already in our "closest to click" list.
						// Most edges are shared by two faces in our raw triangle data set, so we want
						// to select (or deselect) both of these edges that the user clicks on (what
						// appears to be) a single edge
						if( ClosestEdgeIndices.Num() > 0 &&
							( ( EdgeVertices[ 0 ].Equals( ClosestEdgeVertices[ 0 ] ) && EdgeVertices[ 1 ].Equals( ClosestEdgeVertices[ 1 ] ) ) ||
							( EdgeVertices[ 0 ].Equals( ClosestEdgeVertices[ 1 ] ) && EdgeVertices[ 1 ].Equals( ClosestEdgeVertices[ 0 ] ) ) ) )
						{
							// Edge overlaps the closest edge we have so far, so just add it to the list
							ClosestEdgeIndices.Add( EdgeIndex );
							// Increment the number of back facing triangles if the adjoining triangle 
							// is back facing and isn't two-sided
							if(bIsBackFacing && !bIsTwoSided)
							{
								++NumBackFacingTriangles;
							}
						}
						else
						{
							FVector WorldSpaceEdgeStart( StaticMeshComponent->ComponentToWorld.TransformPosition( EdgeVertices[ 0 ] ) );
							FVector WorldSpaceEdgeEnd( StaticMeshComponent->ComponentToWorld.TransformPosition( EdgeVertices[ 1 ] ) );

							// Determine the mesh edge that's closest to the ray cast through the eye towards the click location
							FVector ClosestPointToEdgeOnClickLine;
							FVector ClosestPointToClickLineOnEdge;
							FMath::SegmentDistToSegment(
								ClickLineStart,
								ClickLineEnd,
								WorldSpaceEdgeStart,
								WorldSpaceEdgeEnd,
								ClosestPointToEdgeOnClickLine,
								ClosestPointToClickLineOnEdge );

							// Compute the minimum distance (squared)
							const float MinDistanceToEdgeSquared = ( ClosestPointToClickLineOnEdge - ClosestPointToEdgeOnClickLine ).SizeSquared();

							if( MinDistanceToEdgeSquared <= WorldSpaceMinClickDistance )
							{
								if( MinDistanceToEdgeSquared <= ClosestEdgeDistance )
								{
									// This is the closest edge to the click line that we've found so far!
									ClosestEdgeDistance = MinDistanceToEdgeSquared;
									ClosestEdgeVertices[ 0 ] = EdgeVertices[ 0 ];
									ClosestEdgeVertices[ 1 ] = EdgeVertices[ 1 ];

									ClosestEdgeIndices.Reset();
									ClosestEdgeIndices.Add( EdgeIndex );

									// Reset the number of back facing triangles.
									NumBackFacingTriangles = (bIsBackFacing && !bIsTwoSided) ? 1 : 0;
								}
							}
						}
					}
				}

				// Did the user click on an edge? Edges must also have at least one adjoining triangle 
				// which isn't back face culled (for one-sided materials)
				if( ClosestEdgeIndices.Num() > 0 && ClosestEdgeIndices.Num() > NumBackFacingTriangles)
				{
					for( int32 CurIndex = 0; CurIndex < ClosestEdgeIndices.Num(); ++CurIndex )
					{
						const int32 CurEdgeIndex = ClosestEdgeIndices[ CurIndex ];

						if( bCtrlDown )
						{
							// Toggle selection
							if( SelectedEdgeIndices.Contains( CurEdgeIndex ) )
							{
								SelectedEdgeIndices.Remove( CurEdgeIndex );
							}
							else
							{
								SelectedEdgeIndices.Add( CurEdgeIndex );
							}
						}
						else
						{
							// Append to selection
							SelectedEdgeIndices.Add( CurEdgeIndex );
						}
					}

					// Reset cached vertices and uv coordinates.
					SelectedEdgeVertices.Reset();
					for(int32 TexCoordIndex = 0; TexCoordIndex < MAX_STATIC_TEXCOORDS; ++TexCoordIndex)
					{
						SelectedEdgeTexCoords[TexCoordIndex].Reset();
					}

					for(FSelectedEdgeSet::TIterator SelectionIt( SelectedEdgeIndices ); SelectionIt; ++SelectionIt)
					{
						const uint32 EdgeIndex = *SelectionIt;
						const uint32 FaceIndex = EdgeIndex / 3;

						const uint32 WedgeIndex = FaceIndex * 3 + (EdgeIndex % 3);
						const uint32 WedgeIndex2 = FaceIndex * 3 + ((EdgeIndex + 1) % 3);

						// Cache edge vertices in local space.
						FVector EdgeVertices[ 2 ];
						EdgeVertices[ 0 ] = RawMesh.GetWedgePosition(WedgeIndex);
						EdgeVertices[ 1 ] = RawMesh.GetWedgePosition(WedgeIndex2);

						SelectedEdgeVertices.Add(EdgeVertices[0]);
						SelectedEdgeVertices.Add(EdgeVertices[1]);

						// Cache UV
						for(int32 TexCoordIndex = 0; TexCoordIndex < MAX_STATIC_TEXCOORDS; ++TexCoordIndex)
						{
							if( RawMesh.WedgeTexCoords[TexCoordIndex].Num() > 0)
							{
								FVector2D UVIndex1, UVIndex2;
								UVIndex1 = RawMesh.WedgeTexCoords[TexCoordIndex][WedgeIndex];
								UVIndex2 = RawMesh.WedgeTexCoords[TexCoordIndex][WedgeIndex2];
								SelectedEdgeTexCoords[TexCoordIndex].Add(UVIndex1);
								SelectedEdgeTexCoords[TexCoordIndex].Add(UVIndex2);
							}
						}
					}
				}
			}
		}
	}

	Invalidate();
}

FSceneView* FStaticMeshEditorViewportClient::CalcSceneView(FSceneViewFamily* ViewFamily)
{
	FSceneView* SceneView = FEditorViewportClient::CalcSceneView(ViewFamily);
	FFinalPostProcessSettings::FCubemapEntry& CubemapEntry = *new(SceneView->FinalPostProcessSettings.ContributingCubemaps) FFinalPostProcessSettings::FCubemapEntry;
	CubemapEntry.AmbientCubemap = GUnrealEd->GetThumbnailManager()->AmbientCubemap;
	CubemapEntry.AmbientCubemapTintMulScaleValue = FLinearColor::White * AmbientCubemapIntensity;
	return SceneView;
}

void FStaticMeshEditorViewportClient::SetPreviewMesh(UStaticMesh* InStaticMesh, UStaticMeshComponent* InStaticMeshComponent)
{
	StaticMesh = InStaticMesh;
	StaticMeshComponent = InStaticMeshComponent;

	// set camera
	FRotator ThumbnailAngle;
	float ThumbnailDistance = 0;
	USceneThumbnailInfo* ThumbnailInfo = Cast<USceneThumbnailInfo>(StaticMesh->ThumbnailInfo);
	if ( !ThumbnailInfo )
	{
		ThumbnailInfo = USceneThumbnailInfo::StaticClass()->GetDefaultObject<USceneThumbnailInfo>();
	}

	ThumbnailAngle.Pitch = ThumbnailInfo->OrbitPitch;
	ThumbnailAngle.Yaw = ThumbnailInfo->OrbitYaw;
	ThumbnailAngle.Roll = 0;
	ThumbnailDistance = ThumbnailInfo->OrbitZoom;

	float CameraY = StaticMesh->GetBounds().SphereRadius / (75.0f * (float)PI / 360.0f);
	SetCameraSetup(
		FVector::ZeroVector, 
		ThumbnailAngle,
		FVector(0.0f, CameraY + ThumbnailDistance - AutoViewportOrbitCameraTranslate, 0.0f), 
		StaticMesh->GetBounds().Origin, 
		-FVector(0, CameraY, 0), 
		FRotator(0,90.f,0) );
}


void FStaticMeshEditorViewportClient::SetDrawUVOverlay()
{
	bDrawUVs = !bDrawUVs;
	Invalidate();
}

bool FStaticMeshEditorViewportClient::IsSetDrawUVOverlayChecked() const
{
	return bDrawUVs;
}

void FStaticMeshEditorViewportClient::SetShowNormals()
{
	bDrawNormals = !bDrawNormals;
	Invalidate();
}

bool FStaticMeshEditorViewportClient::IsSetShowNormalsChecked() const
{
	return bDrawNormals;
}

void FStaticMeshEditorViewportClient::SetShowTangents()
{
	bDrawTangents = !bDrawTangents;
	Invalidate();
}

bool FStaticMeshEditorViewportClient::IsSetShowTangentsChecked() const
{
	return bDrawTangents;
}

void FStaticMeshEditorViewportClient::SetShowBinormals()
{
	bDrawBinormals = !bDrawBinormals;
	Invalidate();
}

bool FStaticMeshEditorViewportClient::IsSetShowBinormalsChecked() const
{
	return bDrawBinormals;
}

void FStaticMeshEditorViewportClient::SetShowSockets()
{
	bShowSockets = !bShowSockets;
	Invalidate();
}
bool FStaticMeshEditorViewportClient::IsSetShowSocketsChecked() const
{
	return bShowSockets;
}

void FStaticMeshEditorViewportClient::SetShowPivot()
{
	bShowPivot = !bShowPivot;
	Invalidate();
}

bool FStaticMeshEditorViewportClient::IsSetShowPivotChecked() const
{
	return bShowPivot;
}

void FStaticMeshEditorViewportClient::SetDrawAdditionalData()
{
	bDrawAdditionalData = !bDrawAdditionalData;
	Invalidate();
}

bool FStaticMeshEditorViewportClient::IsSetDrawAdditionalData() const
{
	return bDrawAdditionalData;
}

TSet< int32 >& FStaticMeshEditorViewportClient::GetSelectedEdges()
{ 
	return SelectedEdgeIndices;
}

void FStaticMeshEditorViewportClient::OnSocketSelectionChanged( UStaticMeshSocket* SelectedSocket )
{
	if( SelectedSocket && WidgetMode == FWidget::WM_None )
	{
		WidgetMode = FWidget::WM_Translate;
	}

	Invalidate();
}

void FStaticMeshEditorViewportClient::ResetCamera()
{
	FocusViewportOnBox( StaticMeshComponent->Bounds.GetBox() );
	Invalidate();
}
#undef LOCTEXT_NAMESPACE 
