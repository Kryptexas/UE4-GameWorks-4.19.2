// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PersonaPrivatePCH.h"

#include "PreviewScene.h"
#include "SAnimationEditorViewport.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "SAnimViewportToolBar.h"
#include "AnimViewportShowCommands.h"
#include "AnimGraphDefinitions.h"
#include "AnimationEditorViewportClient.h"
#include "Runtime/Engine/Classes/Components/ReflectionCaptureComponent.h"
#include "Runtime/Engine/Classes/Components/SphereReflectionCaptureComponent.h"
#include "UnrealWidget.h"
#include "MouseDeltaTracker.h"
#include "ScopedTransaction.h"

namespace {
	// Value from UE3
	static const float AnimationEditorViewport_RotateSpeed = 0.02f;
	// Value from UE3
	static const float AnimationEditorViewport_TranslateSpeed = 0.25f;
	// follow camera feature
	static const float FollowCamera_InterpSpeed = 4.f;
	static const float FollowCamera_InterpSpeed_Z = 1.f;

	// @todo double define - fix it
	const float FOVMin = 5.f;
	const float FOVMax = 170.f;
}

/**
 * A hit proxy class for sockets in the Persona viewport.
 */
struct HPersonaSocketProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	FSelectedSocketInfo SocketInfo;

	explicit HPersonaSocketProxy( FSelectedSocketInfo InSocketInfo )
		:	SocketInfo( InSocketInfo )
	{}
};
IMPLEMENT_HIT_PROXY( HPersonaSocketProxy, HHitProxy );

/**
 * A hit proxy class for sockets in the Persona viewport.
 */
struct HPersonaBoneProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	FName BoneName;

	explicit HPersonaBoneProxy(const FName & InBoneName)
		:	BoneName( InBoneName )
	{}
};
IMPLEMENT_HIT_PROXY( HPersonaBoneProxy, HHitProxy );

#define LOCTEXT_NAMESPACE "FAnimationViewportClient"

/////////////////////////////////////////////////////////////////////////
// FAnimationViewportClient

FAnimationViewportClient::FAnimationViewportClient( FPreviewScene& InPreviewScene, TWeakPtr<FPersona> InPersonaPtr )
	: FEditorViewportClient(&InPreviewScene)
	, PersonaPtr( InPersonaPtr )
	, bManipulating(false)
	, bInTransaction(false)
	, AnimationPlaybackScale(1.0f)
	, SelectedWindActor(NULL)
	, PrevWindStrength(0.0f)
	, GravityScaleSliderValue(0.25f)
{
	// load config
	ConfigOption = UPersonaOptions::StaticClass()->GetDefaultObject<UPersonaOptions>();
	check (ConfigOption);

	// DrawHelper set up
	DrawHelper.PerspectiveGridSize = HALF_WORLD_MAX1;
	DrawHelper.AxesLineThickness = ConfigOption->bHighlightOrigin ? 1.0f : 0.0f;
	DrawHelper.bDrawGrid = ConfigOption->bShowGrid;

	LocalAxesMode = static_cast<ELocalAxesMode::Type>(ConfigOption->DefaultLocalAxesSelection);

	WidgetMode = FWidget::WM_Rotate;

	SetSelectedBackgroundColor(ConfigOption->ViewportBackgroundColor, false);

	EngineShowFlags.Game = 0;
	EngineShowFlags.ScreenSpaceReflections = 1;
	EngineShowFlags.ReflectionEnvironmentLightmapMixing = 0;
	EngineShowFlags.AmbientOcclusion = 1;
	EngineShowFlags.SetSnap(0);

	SetRealtime(true);
	if(GEditor->PlayWorld)
	{
		SetRealtime(false,true); // We are PIE, don't start in realtime mode
	}

	// set light options 
	PreviewScene->DirectionalLight->SetRelativeLocation(FVector(-1024.f, 1024.f, 2048.f));
	PreviewScene->DirectionalLight->SetRelativeScale3D(FVector(15.f));
	PreviewScene->DirectionalLight->Mobility = EComponentMobility::Stationary;
	PreviewScene->DirectionalLight->DynamicShadowDistanceStationaryLight = 3000.f;
	PreviewScene->DirectionalLight->bPrecomputedLightingIsValid = false;
	PreviewScene->SetLightBrightness(4.f);
	//InPreviewScene->SetSkyBrightness(0.5f);
	PreviewScene->DirectionalLight->InvalidateLightingCache();
	PreviewScene->DirectionalLight->RecreateRenderState_Concurrent();

	// A background sky sphere
	EditorSkyComp = ConstructObject<UStaticMeshComponent>(UStaticMeshComponent::StaticClass());
	UStaticMesh * StaticMesh = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/MapTemplates/Sky/SM_SkySphere.SM_SkySphere"), NULL, LOAD_None, NULL);
	check (StaticMesh);
	EditorSkyComp->SetStaticMesh( StaticMesh );
	UMaterial* SkyMaterial= LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorMaterials/PersonaSky.PersonaSky"), NULL, LOAD_None, NULL);
	check (SkyMaterial);
	EditorSkyComp->SetMaterial(0, SkyMaterial);
	const float SkySphereScale = 1000.f;
	const FTransform SkyTransform(FRotator(0,0,0), FVector(0,0,0), FVector(SkySphereScale));
	PreviewScene->AddComponent(EditorSkyComp, SkyTransform);

	// now add height fog component

	EditorHeightFogComponent = ConstructObject<UExponentialHeightFogComponent>(UExponentialHeightFogComponent::StaticClass());
	
	EditorHeightFogComponent->FogDensity=0.00075f;
	EditorHeightFogComponent->FogInscatteringColor=FLinearColor(3.f,4.f,6.f,0.f)*0.3f;
	EditorHeightFogComponent->DirectionalInscatteringExponent=16.f;
	EditorHeightFogComponent->DirectionalInscatteringColor=FLinearColor(1.1f,0.9f,0.538427f,0.f);
	EditorHeightFogComponent->FogHeightFalloff=0.01f;
	EditorHeightFogComponent->StartDistance=15000.f;
	const FTransform FogTransform(FRotator(0,0,0), FVector(3824.f,34248.f,50000.f), FVector(80.f));
	PreviewScene->AddComponent(EditorHeightFogComponent, FogTransform);

	// add capture component for reflection
 	USphereReflectionCaptureComponent* CaptureComponent = ConstructObject<USphereReflectionCaptureComponent>(USphereReflectionCaptureComponent::StaticClass());
 
 	const FTransform CaptureTransform(FRotator(0,0,0), FVector(0.f,0.f,100.f), FVector(1.f));
 	PreviewScene->AddComponent(CaptureComponent, CaptureTransform);
	CaptureComponent->SetCaptureIsDirty();
	CaptureComponent->UpdateReflectionCaptureContents(PreviewScene->GetWorld());

	// now add floor
 	UStaticMesh* FloorMesh = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorMeshes/PhAT_FloorBox.PhAT_FloorBox"), NULL, LOAD_None, NULL);
 	check(FloorMesh);
 	EditorFloorComp = ConstructObject<UStaticMeshComponent>(UStaticMeshComponent::StaticClass());
 	EditorFloorComp->SetStaticMesh( FloorMesh );
 	PreviewScene->AddComponent(EditorFloorComp, FTransform::Identity);
 	EditorFloorComp->SetRelativeScale3D(FVector(3.f, 3.f, 1.f));
	UMaterial* Material= LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorMaterials/PersonaFloorMat.PersonaFloorMat"), NULL, LOAD_None, NULL);
	check(Material);
	EditorFloorComp->SetMaterial( 0, Material );

	// set visibility
	EditorSkyComp->SetVisibility(ConfigOption->bShowSky);
	EditorHeightFogComponent->SetVisibility(ConfigOption->bShowSky);
 	EditorFloorComp->SetVisibility(ConfigOption->bShowFloor);

	ViewFOV = FMath::Clamp<float>(ConfigOption->ViewFOV, FOVMin, FOVMax);

	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.CompositeEditorPrimitives = true;

	// set camera mode
	bCameraFollow = false;

	//wind actor's initial position, rotation and strength
	PrevWindLocation = FVector(100, 100, 100);
	PrevWindRotation = FRotator(0,0,0);
	PrevWindStrength = 0.2f;

	bDrawUVs = false;
	UVChannelToDraw = 0;

	// Set audio mute option
	UWorld* World = PreviewScene->GetWorld();
	if(World)
	{
		World->bAllowAudioPlayback = !ConfigOption->bMuteAudio;
	}
}

FLinearColor FAnimationViewportClient::GetBackgroundColor() const
{
	return SelectedHSVColor.HSVToLinearRGB();
}

FSceneView* FAnimationViewportClient::CalcSceneView(FSceneViewFamily* ViewFamily)
{
	float AmbientCubemapIntensity = 0.4f;

	FSceneView* SceneView = FEditorViewportClient::CalcSceneView(ViewFamily);
	FFinalPostProcessSettings::FCubemapEntry& CubemapEntry = *new(SceneView->FinalPostProcessSettings.ContributingCubemaps) FFinalPostProcessSettings::FCubemapEntry;
	CubemapEntry.AmbientCubemap = GUnrealEd->GetThumbnailManager()->AmbientCubemap;
	CubemapEntry.AmbientCubemapTintMulScaleValue = FLinearColor::White * AmbientCubemapIntensity;
	return SceneView;
}

void FAnimationViewportClient::SetSelectedBackgroundColor(const FLinearColor & RGBColor, bool bSave/*=true*/)
{
	SelectedHSVColor = RGBColor.LinearRGBToHSV(); 

	// save to config
	if (bSave)
	{
		ConfigOption->SetViewportBackgroundColor(RGBColor);
	}

	// only consider V from HSV
	// just inversing works against for mid range brightness
	// V being from 0-0.3 (almost white), 0.31-1 (almost black)
	if (SelectedHSVColor.B < 0.3f)
	{
		DrawHelper.GridColorAxis = FColor(230,230,230);
		DrawHelper.GridColorMajor = FColor(180,180,180);
		DrawHelper.GridColorMinor = FColor(128,128,128);
	}
	else
	{
		DrawHelper.GridColorAxis = FColor(20,20,20);
		DrawHelper.GridColorMajor = FColor(60,60,60);
		DrawHelper.GridColorMinor = FColor(128,128,128);
	}

	Invalidate();
}

void FAnimationViewportClient::SetBackgroundColor( FLinearColor InColor )
{
	SetSelectedBackgroundColor(InColor);
}

float FAnimationViewportClient::GetBrightnessValue() const
{
	return SelectedHSVColor.B;
}

void FAnimationViewportClient::SetBrightnessValue( float Value )
{
	SelectedHSVColor.B = Value;
	SetSelectedBackgroundColor(SelectedHSVColor.HSVToLinearRGB());
}

void FAnimationViewportClient::OnToggleShowGrid()
{
	FEditorViewportClient::SetShowGrid();

	ConfigOption->SetShowGrid(DrawHelper.bDrawGrid);
}

bool FAnimationViewportClient::IsShowingGrid() const
{
	return FEditorViewportClient::IsSetShowGridChecked();
}

void FAnimationViewportClient::OnToggleHighlightOrigin()
{
	DrawHelper.AxesLineThickness = 1.0f - DrawHelper.AxesLineThickness;

	ConfigOption->SetHighlightOrigin( DrawHelper.AxesLineThickness != 0.0f );
}

bool FAnimationViewportClient::IsHighlightingOrigin() const
{
	return ( DrawHelper.AxesLineThickness != 0.0f );
}

void FAnimationViewportClient::OnToggleShowFloor()
{
	if (EditorFloorComp)
	{
		EditorFloorComp->SetVisibility(!EditorFloorComp->bVisible);
		Invalidate();
	}

	ConfigOption->SetShowFloor(EditorFloorComp->bVisible);
}

bool FAnimationViewportClient::IsShowingFloor() const
{
	return (EditorFloorComp)? EditorFloorComp->bVisible : false;
}

void FAnimationViewportClient::OnToggleShowSky()
{
	if (EditorSkyComp)
	{
		bool bNewVisibility = !EditorSkyComp->bVisible;
		EditorSkyComp->SetVisibility(bNewVisibility);
		EditorHeightFogComponent->SetVisibility(bNewVisibility);
		Invalidate();
	}

	ConfigOption->SetShowSky(EditorSkyComp->bVisible);
}

bool FAnimationViewportClient::IsShowingSky() const
{
	return (EditorSkyComp)? EditorSkyComp->bVisible : false;
}

void FAnimationViewportClient::OnToggleMuteAudio()
{
	UWorld* World = PreviewScene->GetWorld();

	if(World)
	{
		bool bNewAllowAudioPlayback = !World->AllowAudioPlayback();
		World->bAllowAudioPlayback = bNewAllowAudioPlayback;

		ConfigOption->SetMuteAudio(!bNewAllowAudioPlayback);
	}
}

bool FAnimationViewportClient::IsAudioMuted() const
{
	UWorld* World = PreviewScene->GetWorld();

	return (World) ? !World->AllowAudioPlayback() : false;
}

void FAnimationViewportClient::SetCameraFollow()
{
	bCameraFollow = !bCameraFollow;
	
	if( bCameraFollow )
	{

		EnableCameraLock(false);

		if (PreviewSkelMeshComp.IsValid())
		{
			FBoxSphereBounds Bound = PreviewSkelMeshComp.Get()->CalcBounds(FTransform::Identity);
			SetViewLocationForOrbiting(Bound.Origin);
		}
	}
	else
	{
		FocusViewportOnPreviewMesh();
		Invalidate();
	}
}

bool FAnimationViewportClient::IsSetCameraFollowChecked() const
{
	return bCameraFollow;
}

void FAnimationViewportClient::SetPreviewMeshComponent(UDebugSkelMeshComponent* InPreviewSkelMeshComp)
{
	PreviewSkelMeshComp = InPreviewSkelMeshComp;

	UpdateCameraSetup();
}

void FAnimationViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);

	if (PreviewSkelMeshComp.IsValid() && PreviewSkelMeshComp->SkeletalMesh)
	{
		// Can't have both bones of interest and sockets of interest set
		check( !(PreviewSkelMeshComp->BonesOfInterest.Num() && PreviewSkelMeshComp->SocketsOfInterest.Num() ) )

		// if we have BonesOfInterest, draw sub set of the bones only
		if ( PreviewSkelMeshComp->BonesOfInterest.Num() > 0 )
		{
			DrawMeshSubsetBones(PreviewSkelMeshComp.Get(), PreviewSkelMeshComp->BonesOfInterest, PDI);
		}
		// otherwise, if we display bones, display
		if ( PreviewSkelMeshComp->bDisplayBones )
		{
			DrawMeshBones(PreviewSkelMeshComp.Get(), PDI);
		}
		if ( PreviewSkelMeshComp->bDisplayRawAnimation )
		{
			DrawMeshBonesCompressedAnimation(PreviewSkelMeshComp.Get(), PDI);
		}
		if ( PreviewSkelMeshComp->NonRetargetedSpaceBases.Num() > 0 )
		{
			DrawMeshBonesNonRetargetedAnimation(PreviewSkelMeshComp.Get(), PDI);
		}
		if( PreviewSkelMeshComp->bDisplayAdditiveBasePose )
		{
			DrawMeshBonesAdditiveBasePose(PreviewSkelMeshComp.Get(), PDI);
		}

		// Display normal vectors of each simulation vertex
		if ( PreviewSkelMeshComp->bDisplayClothingNormals )
		{
			PreviewSkelMeshComp->DrawClothingNormals(PDI);
		}

		// Display tangent spaces of each graphical vertex
		if ( PreviewSkelMeshComp->bDisplayClothingTangents )
		{
			PreviewSkelMeshComp->DrawClothingTangents(PDI);
		}

		// Display collision volumes of current selected cloth
		if ( PreviewSkelMeshComp->bDisplayClothingCollisionVolumes )
		{
			PreviewSkelMeshComp->DrawClothingCollisionVolumes(PDI);
		}

		// Display socket hit points
		if ( PreviewSkelMeshComp->bDrawSockets )
		{
			if ( PreviewSkelMeshComp->bSkeletonSocketsVisible && PreviewSkelMeshComp->SkeletalMesh->Skeleton )
			{
				DrawSockets( PreviewSkelMeshComp.Get()->SkeletalMesh->Skeleton->Sockets, PDI, true );
			}

			if ( PreviewSkelMeshComp->bMeshSocketsVisible )
			{
				DrawSockets( PreviewSkelMeshComp.Get()->SkeletalMesh->GetMeshOnlySocketList(), PDI, false );
			}
		}

		// If we have a socket of interest, draw the widget
		if ( PreviewSkelMeshComp->SocketsOfInterest.Num() == 1 )
		{
			USkeletalMeshSocket* Socket = PreviewSkelMeshComp->SocketsOfInterest[0].Socket;

			bool bSocketIsOnSkeleton = PreviewSkelMeshComp->SocketsOfInterest[0].bSocketIsOnSkeleton;

			TArray<USkeletalMeshSocket*> SocketAsArray;
			SocketAsArray.Add( Socket );
			DrawSockets( SocketAsArray, PDI, false );
		}
	}

	if ( PersonaPtr.IsValid() && PreviewSkelMeshComp.IsValid() )
	{
		// select all nodes, and if skeletalcontrol, allow them to draw 
		const FGraphPanelSelectionSet SelectedNodes = PersonaPtr.Pin()->GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UAnimGraphNode_SkeletalControlBase* Node = Cast<UAnimGraphNode_SkeletalControlBase>(*NodeIt);

			if (Node)
			{
				Node->Draw(PDI, PreviewSkelMeshComp.Get());
			}
		}
	}
}

void FAnimationViewportClient::DrawCanvas( FViewport& InViewport, FSceneView& View, FCanvas& Canvas )
{
	if (PreviewSkelMeshComp.IsValid())
	{

		// Display bone names
		if (PreviewSkelMeshComp->bShowBoneNames)
		{
			ShowBoneNames(&Canvas, &View);
		}

		// Display info
		DisplayInfo(&Canvas, &View, IsShowingMeshStats());

		// Draw name of selected bone
		if (PreviewSkelMeshComp->BonesOfInterest.Num() == 1)
		{
			const FIntPoint ViewPortSize = Viewport->GetSizeXY();
			const int32 HalfX = ViewPortSize.X/2;
			const int32 HalfY = ViewPortSize.Y/2;

			int32 BoneIndex = PreviewSkelMeshComp->BonesOfInterest[0];
			const FName BoneName = PreviewSkelMeshComp->SkeletalMesh->RefSkeleton.GetBoneName(BoneIndex);

			FMatrix BoneMatrix = PreviewSkelMeshComp->GetBoneMatrix(BoneIndex);
			const FPlane proj = View.Project(BoneMatrix.GetOrigin());
			if (proj.W > 0.f)
			{
				const int32 XPos = HalfX + ( HalfX * proj.X );
				const int32 YPos = HalfY + ( HalfY * (proj.Y * -1) );

				FQuat BoneQuat = PreviewSkelMeshComp->GetBoneQuaternion(BoneName);
				FVector Loc = PreviewSkelMeshComp->GetBoneLocation(BoneName);
				FCanvasTextItem TextItem( FVector2D( XPos, YPos), FText::FromString( BoneName.ToString() ), GEngine->GetSmallFont(), FLinearColor::White );
				Canvas.DrawItem( TextItem );				
			}
		}

		// Draw name of selected socket
		if (PreviewSkelMeshComp->SocketsOfInterest.Num() == 1)
		{
			USkeletalMeshSocket* Socket = PreviewSkelMeshComp->SocketsOfInterest[0].Socket;

			FMatrix SocketMatrix;
			Socket->GetSocketMatrix( SocketMatrix, PreviewSkelMeshComp.Get() );
			const FVector SocketPos	= SocketMatrix.GetOrigin();

			const FPlane proj = View.Project( SocketPos );
			if(proj.W > 0.f)
			{
				const FIntPoint ViewPortSize = Viewport->GetSizeXY();
				const int32 HalfX = ViewPortSize.X/2;
				const int32 HalfY = ViewPortSize.Y/2;

				const int32 XPos = HalfX + ( HalfX * proj.X );
				const int32 YPos = HalfY + ( HalfY * (proj.Y * -1) );
				FCanvasTextItem TextItem( FVector2D( XPos, YPos), FText::FromString( Socket->SocketName.ToString() ), GEngine->GetSmallFont(), FLinearColor::White );
				Canvas.DrawItem( TextItem );				
			}
		}

		if (bDrawUVs)
		{
			DrawUVsForMesh(Viewport, &Canvas, 1.0f);
		}
	}
}

void FAnimationViewportClient::DrawUVsForMesh(FViewport* InViewport, FCanvas* InCanvas, int32 InTextYPos)
{
	//use the overriden LOD level
	const uint32 LODLevel = FMath::Clamp(PreviewSkelMeshComp->ForcedLodModel - 1, 0, PreviewSkelMeshComp->SkeletalMesh->LODInfo.Num() - 1);

	TArray<FVector2D> SelectedEdgeTexCoords; //No functionality in Persona for this (yet?)

	DrawUVs(InViewport, InCanvas, InTextYPos, LODLevel, UVChannelToDraw, SelectedEdgeTexCoords, NULL, &PreviewSkelMeshComp->GetSkeletalMeshResource()->LODModels[LODLevel] );
}

void FAnimationViewportClient::Tick(float DeltaSeconds) 
{
	FEditorViewportClient::Tick(DeltaSeconds);

	// @todo fixme
	if (bCameraFollow && PreviewSkelMeshComp.IsValid())
	{
		// if camera isn't lock, make the mesh bounds to be center
		FSphere BoundSphere = GetCameraTarget();

		// need to interpolate from ViewLocation to Origin
		SetCameraTargetLocation(BoundSphere, DeltaSeconds);
	}

	if (!GIntraFrameDebuggingGameThread)
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds*AnimationPlaybackScale);
	}
}

void FAnimationViewportClient::SetCameraTargetLocation(const FSphere &BoundSphere, float DeltaSeconds)
{
	FVector OldViewLoc = GetViewLocation();
	FMatrix EpicMat = FTranslationMatrix(-GetViewLocation());
	EpicMat = EpicMat * FInverseRotationMatrix(GetViewRotation());
	FMatrix CamRotMat = EpicMat.Inverse();
	FVector CamDir = FVector(CamRotMat.M[0][0],CamRotMat.M[0][1],CamRotMat.M[0][2]);
	FVector NewViewLocation = BoundSphere.Center - BoundSphere.W * 2 * CamDir;

	NewViewLocation.X = FMath::FInterpTo(OldViewLoc.X, NewViewLocation.X, DeltaSeconds, FollowCamera_InterpSpeed);
	NewViewLocation.Y = FMath::FInterpTo(OldViewLoc.Y, NewViewLocation.Y, DeltaSeconds, FollowCamera_InterpSpeed);
	NewViewLocation.Z = FMath::FInterpTo(OldViewLoc.Z, NewViewLocation.Z, DeltaSeconds, FollowCamera_InterpSpeed_Z);

	SetViewLocation( NewViewLocation );
}

void FAnimationViewportClient::ShowBoneNames( FCanvas* Canvas, FSceneView* View )
{
	if (!PreviewSkelMeshComp.IsValid() || !PreviewSkelMeshComp->MeshObject)
	{
		return;
	}

	//Most of the code taken from FASVViewportClient::Draw() in AnimSetViewerMain.cpp
	FSkeletalMeshResource* SkelMeshResource = PreviewSkelMeshComp->GetSkeletalMeshResource();
	check(SkelMeshResource);
	const int32 LODIndex = FMath::Clamp(PreviewSkelMeshComp->PredictedLODLevel, 0, SkelMeshResource->LODModels.Num()-1);
	FStaticLODModel& LODModel = SkelMeshResource->LODModels[ LODIndex ];

	const int32 HalfX = Viewport->GetSizeXY().X/2;
	const int32 HalfY = Viewport->GetSizeXY().Y/2;

	for (int32 i=0; i< LODModel.RequiredBones.Num(); i++)
	{
		const int32 BoneIndex = LODModel.RequiredBones[i];

		// If previewing a specific chunk, only show the bone names that belong to it
		if ((PreviewSkelMeshComp->ChunkIndexPreview >= 0) && !LODModel.Chunks[PreviewSkelMeshComp->ChunkIndexPreview].BoneMap.Contains(BoneIndex))
		{
			continue;
		}

		const FColor BoneColor = FColor::White;
		if (BoneColor.A != 0)
		{
			const FVector BonePos = PreviewSkelMeshComp->ComponentToWorld.TransformPosition(PreviewSkelMeshComp->SpaceBases[BoneIndex].GetLocation());

			const FPlane proj = View->Project(BonePos);
			if (proj.W > 0.f)
			{
				const int32 XPos = HalfX + ( HalfX * proj.X );
				const int32 YPos = HalfY + ( HalfY * (proj.Y * -1) );

				const FName BoneName = PreviewSkelMeshComp->SkeletalMesh->RefSkeleton.GetBoneName(BoneIndex);
				const FString BoneString = FString::Printf( TEXT("%d: %s"), BoneIndex, *BoneName.ToString() );
				FCanvasTextItem TextItem( FVector2D( XPos, YPos), FText::FromString( BoneString ), GEngine->GetSmallFont(), BoneColor );
				Canvas->DrawItem( TextItem );
			}
		}
	}
}

void FAnimationViewportClient::DisplayInfo(FCanvas* Canvas, FSceneView* View, bool bDisplayAllInfo)
{
	int32 CurXOffset = 5;
	int32 CurYOffset = 60;

	int32 XL, YL;
	StringSize( GEngine->GetSmallFont(),  XL, YL, TEXT("L") );
	FString InfoString;

	// it is weird, but unless it's completely black, it's too bright, so just making it white if only black
	const FLinearColor TextColor = (SelectedHSVColor.B < 0.3f) ? FLinearColor::White : FLinearColor::Black;

	// if not valid skeletalmesh
	if (!PreviewSkelMeshComp.IsValid() || !PreviewSkelMeshComp->SkeletalMesh)
	{
		return;
	}

	if (PreviewSkelMeshComp->SkeletalMesh->MorphTargets.Num() > 0)
	{
		FColor HeadlineColour(255,83,0);
		FColor SubHeadlineColour(202,66,0);

		int32 SubHeadingIndent = CurXOffset + 10;

		TArray<UMaterial*> ProcessedMaterials;
		TArray<UMaterial*> MaterialsThatNeedMorphFlagOn;
		TArray<UMaterial*> MaterialsThatNeedSaving;

		for (int i = 0; i < PreviewSkelMeshComp->GetNumMaterials(); ++i)
		{
			if (UMaterialInterface* MaterialInterface = PreviewSkelMeshComp->GetMaterial(i))
			{
				UMaterial* Material = MaterialInterface->GetMaterial();
				if ((Material != nullptr) && !ProcessedMaterials.Contains(Material))
				{
					ProcessedMaterials.Add(Material);
					if (!Material->GetUsageByFlag(MATUSAGE_MorphTargets))
					{
						MaterialsThatNeedMorphFlagOn.Add(Material);
					}
					else if (Material->IsUsageFlagDirty(MATUSAGE_MorphTargets))
					{
						MaterialsThatNeedSaving.Add(Material);
					}
				}
			}
		}

		if (MaterialsThatNeedMorphFlagOn.Num() > 0)
		{
			InfoString = FString::Printf( *LOCTEXT("MorphSupportNeeded", "The following materials need morph support ('Used with Morph Targets' in material editor):").ToString() );
			Canvas->DrawShadowedString( CurXOffset, CurYOffset, *InfoString, GEngine->GetSmallFont(), HeadlineColour );

			CurYOffset += YL + 2;

			for(auto Iter = MaterialsThatNeedMorphFlagOn.CreateIterator(); Iter; ++Iter)
			{
				UMaterial* Material = (*Iter);
				InfoString = FString::Printf(TEXT("%s"), *Material->GetPathName());
				Canvas->DrawShadowedString( SubHeadingIndent, CurYOffset, *InfoString, GEngine->GetSmallFont(), SubHeadlineColour );
				CurYOffset += YL + 2;
			}
			CurYOffset += 2;
		}

		if (MaterialsThatNeedSaving.Num() > 0)
		{
			InfoString = FString::Printf( *LOCTEXT("MaterialsNeedSaving", "The following materials need saving to fully support morph targets:").ToString() );
			Canvas->DrawShadowedString( CurXOffset, CurYOffset, *InfoString, GEngine->GetSmallFont(), HeadlineColour );

			CurYOffset += YL + 2;

			for(auto Iter = MaterialsThatNeedSaving.CreateIterator(); Iter; ++Iter)
			{
				UMaterial* Material = (*Iter);
				InfoString = FString::Printf(TEXT("%s"), *Material->GetPathName());
				Canvas->DrawShadowedString( SubHeadingIndent, CurYOffset, *InfoString, GEngine->GetSmallFont(), SubHeadlineColour );
				CurYOffset += YL + 2;
			}
			CurYOffset += 2;
		}
	}

	if (PreviewSkelMeshComp->IsUsingInGameBounds())
	{
		if (!PreviewSkelMeshComp->CheckIfBoundsAreCorrrect())
		{
			if( PreviewSkelMeshComp->GetPhysicsAsset() == NULL )
			{
				InfoString = FString::Printf( *LOCTEXT("NeedToSetupPhysicsAssetForAccurateBounds", "You may need to setup Physics Asset to use more accurate bounds").ToString() );
			}
			else
			{
				InfoString = FString::Printf( *LOCTEXT("NeedToSetupBoundsInPhysicsAsset", "You need to setup bounds in Physics Asset to include whole mesh").ToString() );
			}
			Canvas->DrawShadowedString( CurXOffset, CurYOffset, *InfoString, GEngine->GetSmallFont(), TextColor );
			CurYOffset += YL + 2;
		}
	}

	if (bDisplayAllInfo && PreviewSkelMeshComp != NULL && PreviewSkelMeshComp->MeshObject != NULL)
	{
		FSkeletalMeshResource* SkelMeshResource = PreviewSkelMeshComp->GetSkeletalMeshResource();
		check(SkelMeshResource);

		// Draw stats about the mesh
		const FBoxSphereBounds& SkelBounds = PreviewSkelMeshComp->Bounds;
		const FPlane ScreenPosition = View->Project(SkelBounds.Origin);

		const int32 HalfX = Viewport->GetSizeXY().X/2;
		const int32 HalfY = Viewport->GetSizeXY().Y/2;

		const float ScreenRadius = FMath::Max((float)HalfX * View->ViewMatrices.ProjMatrix.M[0][0], (float)HalfY * View->ViewMatrices.ProjMatrix.M[1][1]) * SkelBounds.SphereRadius / FMath::Max(ScreenPosition.W,1.0f);
		const float LODFactor = ScreenRadius / 320.0f;

		int32 NumBonesInUse;
		int32 NumBonesMappedToVerts; 
		int32 NumChunksInUse;
		int32 NumSectionsInUse;
		FString WeightUsage;

		const int32 LODIndex = FMath::Clamp(PreviewSkelMeshComp->PredictedLODLevel, 0, SkelMeshResource->LODModels.Num()-1);
		FStaticLODModel& LODModel = SkelMeshResource->LODModels[ LODIndex ];

		NumBonesInUse = LODModel.RequiredBones.Num();
		NumBonesMappedToVerts = LODModel.ActiveBoneIndices.Num();
		NumChunksInUse = LODModel.Chunks.Num();
		NumSectionsInUse = LODModel.Sections.Num();

		InfoString = FString::Printf( TEXT("LOD [%d] Bones:%d (Mapped to Vertices:%d) Polys:%d"), 
			LODIndex, 
			NumBonesInUse,	   
			NumBonesMappedToVerts,
			LODModel.GetTotalFaces());

		Canvas->DrawShadowedString( CurXOffset, CurYOffset, *InfoString, GEngine->GetSmallFont(), TextColor );

		InfoString = FString::Printf( TEXT("(Display Factor:%3.2f, FOV:%3.0f)"), LODFactor, ViewFOV);
		CurYOffset += YL + 2;
		Canvas->DrawShadowedString( CurXOffset, CurYOffset, *InfoString, GEngine->GetSmallFont(), TextColor );

		CurYOffset += 1; // --

		uint32 TotalRigidVertices = 0;
		uint32 TotalSoftVertices = 0;
		for(int32 ChunkIndex = 0;ChunkIndex < LODModel.Chunks.Num();ChunkIndex++)
		{
			int32 ChunkRigidVerts = LODModel.Chunks[ChunkIndex].GetNumRigidVertices();
			int32 ChunkSoftVerts = LODModel.Chunks[ChunkIndex].GetNumSoftVertices();

			InfoString = FString::Printf( TEXT(" [Chunk %d] Verts:%d (Rigid:%d Soft:%d) Bones:%d"), 
				ChunkIndex,
				ChunkRigidVerts + ChunkSoftVerts,
				ChunkRigidVerts,
				ChunkSoftVerts,
				LODModel.Chunks[ChunkIndex].BoneMap.Num()
				);

			CurYOffset += YL + 2;
			Canvas->DrawShadowedString( CurXOffset, CurYOffset, *InfoString, GEngine->GetSmallFont(), TextColor*0.8f );

			TotalRigidVertices += ChunkRigidVerts;
			TotalSoftVertices += ChunkSoftVerts;
		}

		InfoString = FString::Printf( TEXT("TOTAL Verts:%d (Rigid:%d Soft:%d)"), 
			LODModel.NumVertices,
			TotalRigidVertices,
			TotalSoftVertices );

		CurYOffset += 1; // --


		CurYOffset += YL + 2;
		Canvas->DrawShadowedString( CurXOffset, CurYOffset, *InfoString, GEngine->GetSmallFont(), TextColor );

		InfoString = FString::Printf( TEXT("Chunks:%d Sections:%d %s"), 
			NumChunksInUse, 
			NumSectionsInUse,
			*WeightUsage
			);

		CurYOffset += YL + 2;
		Canvas->DrawShadowedString( CurXOffset, CurYOffset, *InfoString, GEngine->GetSmallFont(), TextColor );
		
		if (PreviewSkelMeshComp->BonesOfInterest.Num() > 0)
		{
			int32 BoneIndex = PreviewSkelMeshComp->BonesOfInterest[0];
			const FName BoneName = PreviewSkelMeshComp->SkeletalMesh->RefSkeleton.GetBoneName(BoneIndex);
			FTransform LocalTransform = PreviewSkelMeshComp->LocalAtoms[BoneIndex];
			FTransform ComponentTransform = PreviewSkelMeshComp->SpaceBases[BoneIndex];

			CurYOffset += YL + 2;						
			InfoString = FString::Printf(TEXT("Local :%s"), *LocalTransform.ToString());
			Canvas->DrawShadowedString( CurXOffset, CurYOffset, *InfoString, GEngine->GetSmallFont(), TextColor );

			CurYOffset += YL + 2;
			InfoString = FString::Printf(TEXT("Component :%s"), *ComponentTransform.ToString());
			Canvas->DrawShadowedString( CurXOffset, CurYOffset, *InfoString, GEngine->GetSmallFont(), TextColor );
		}

		CurYOffset += YL + 2;
		InfoString = FString::Printf(TEXT("Approximate Size: %ix%ix%i"), 
			FMath::Round(PreviewSkelMeshComp->Bounds.BoxExtent.X * 2.0f),
			FMath::Round(PreviewSkelMeshComp->Bounds.BoxExtent.Y * 2.0f),
			FMath::Round(PreviewSkelMeshComp->Bounds.BoxExtent.Z * 2.0f));
		Canvas->DrawShadowedString( CurXOffset, CurYOffset, *InfoString, GEngine->GetSmallFont(), TextColor );
	}
}

int32 FAnimationViewportClient::FindSelectedBone() const
{
	int32 SelectedBone = -1;
	if (PreviewSkelMeshComp.IsValid() && (PreviewSkelMeshComp->BonesOfInterest.Num() == 1) )
	{
		SelectedBone = PreviewSkelMeshComp->BonesOfInterest[0];
	}
	return SelectedBone;
}

USkeletalMeshSocket* FAnimationViewportClient::FindSelectedSocket() const
{
	USkeletalMeshSocket* SelectedSocket = NULL;

	if (PreviewSkelMeshComp.IsValid() && (PreviewSkelMeshComp->SocketsOfInterest.Num() == 1) )
	{
		SelectedSocket = PreviewSkelMeshComp->SocketsOfInterest[0].Socket;
	}

	return SelectedSocket;
}

TWeakObjectPtr<AWindDirectionalSource> FAnimationViewportClient::FindSelectedWindActor() const
{
	return SelectedWindActor;
}

void FAnimationViewportClient::ProcessClick(class FSceneView& View, class HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	if ( HitProxy )
	{
		if ( HitProxy->IsA( HPersonaSocketProxy::StaticGetType() ) )
		{
			HPersonaSocketProxy* SocketProxy = ( HPersonaSocketProxy* )HitProxy;

			// Tell Persona that the socket has been selected - this will sort out the skeleton tree, etc.
			if ( PersonaPtr.IsValid() )
			{
				PersonaPtr.Pin()->SetSelectedSocket( SocketProxy->SocketInfo );
			}
		}
		else if ( HitProxy->IsA( HPersonaBoneProxy::StaticGetType() ) )
		{
			HPersonaBoneProxy* BoneProxy = ( HPersonaBoneProxy* )HitProxy;

			// Tell Persona that the bone has been selected - this will sort out the skeleton tree, etc.
			if ( PersonaPtr.IsValid() )
			{
				PersonaPtr.Pin()->SetSelectedBone( PreviewSkelMeshComp.Get()->SkeletalMesh->Skeleton, BoneProxy->BoneName );
			}
		}
		else if ( HitProxy->IsA( HActor::StaticGetType() ) )
		{
			HActor* ActorHitProxy = static_cast<HActor*>(HitProxy);
			AWindDirectionalSource* WindActor = Cast<AWindDirectionalSource>(ActorHitProxy->Actor);

			if( WindActor )
			{
				//clear previously selected things
				PersonaPtr.Pin()->DeselectAll();
				SelectedWindActor = WindActor;
			}
		}
	}
	else
	{
		// Clicking outside of a hit proxy should de-select things
		if ( PersonaPtr.IsValid() )
		{
			PersonaPtr.Pin()->DeselectAll();
		}
	}
}

bool FAnimationViewportClient::InputWidgetDelta( FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale )
{
	// Get some useful info about buttons being held down
	const bool bCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
	const bool bShiftDown = Viewport->KeyState(EKeys::LeftShift) || Viewport->KeyState(EKeys::RightShift);
	const bool bMouseButtonDown = Viewport->KeyState( EKeys::LeftMouseButton ) || Viewport->KeyState( EKeys::MiddleMouseButton ) || Viewport->KeyState( EKeys::RightMouseButton );

	bool bHandled = false;

	if ( bManipulating && CurrentAxis != EAxisList::None )
	{
		bHandled = true;

		int32 BoneIndex = FindSelectedBone();
		USkeletalMeshSocket* SelectedSocket = FindSelectedSocket();
		TWeakObjectPtr<AWindDirectionalSource> WindActor = FindSelectedWindActor();

		FAnimNode_ModifyBone* SkelControl = NULL;

		if ( BoneIndex >= 0 )
		{
			//Get the skeleton control manipulating this bone
			SkelControl = &(PreviewSkelMeshComp->PreviewInstance->SingleBoneController);
		}

		if ( SkelControl || SelectedSocket )
		{
			FTransform CurrentSkelControlTM(
				SelectedSocket ? SelectedSocket->RelativeRotation : SkelControl->Rotation,
				SelectedSocket ? SelectedSocket->RelativeLocation : SkelControl->Translation);

			FTransform BaseTM;

			if ( SelectedSocket )
			{
				BaseTM = SelectedSocket->GetSocketTransform( PreviewSkelMeshComp.Get() );
			}
			else
			{
				BaseTM = PreviewSkelMeshComp->GetBoneTransform( BoneIndex );
			}

			// Remove SkelControl's orientation from BoneMatrix, as we need to translate/rotate in the non-SkelControlled space
			BaseTM = BaseTM.InverseSafe() * CurrentSkelControlTM;

			if (WidgetMode == FWidget::WM_Rotate)
			{
				FVector RotAxis;
				float RotAngle;
				Rot.Quaternion().ToAxisAndAngle( RotAxis, RotAngle );

				FVector4 BoneSpaceAxis = BaseTM.TransformVector( RotAxis );

				//Calculate the new delta rotation
				FQuat DeltaQuat( BoneSpaceAxis, RotAngle );

				FRotator NewRotation = ( CurrentSkelControlTM * FTransform( DeltaQuat ) ).Rotator();

				if ( SelectedSocket )
				{
					SelectedSocket->RelativeRotation = NewRotation;
				}
				else
				{
					SkelControl->Rotation = NewRotation;
				}
			}
			else if (WidgetMode == FWidget::WM_Translate)
			{
				FVector4 BoneSpaceOffset = BaseTM.TransformVector(Drag);

				if (SelectedSocket)
				{
					SelectedSocket->RelativeLocation += BoneSpaceOffset;
				}
				else
				{
					SkelControl->Translation += BoneSpaceOffset;
				}
			}
			else if (WidgetMode == FWidget::WM_Scale)
			{
				//@TODO: ANIMATION: Add scaling support here
			}
		}
		else if( WindActor.IsValid() )
		{
			if (WidgetMode == FWidget::WM_Rotate)
			{
				FTransform WindTransform = WindActor->GetTransform();

				FRotator NewRotation = ( WindTransform * FTransform( Rot ) ).Rotator();

				WindActor->SetActorRotation( NewRotation );
			}
			else
			{
				FVector Location = WindActor->GetActorLocation();
				Location += Drag;
				WindActor->SetActorLocation(Location);
			}
		}

		Viewport->Invalidate();
	}

	return bHandled;
}


void FAnimationViewportClient::TrackingStarted( const struct FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge )
{
	int32 BoneIndex = FindSelectedBone();
	USkeletalMeshSocket* SelectedSocket = FindSelectedSocket();
	TWeakObjectPtr<AWindDirectionalSource> WindActor = FindSelectedWindActor();

	if( (BoneIndex >= 0 || SelectedSocket || WindActor.IsValid()) && bIsDraggingWidget )
	{
		bool bValidAxis = false;
		FVector WorldAxisDir;

		if ( PreviewSkelMeshComp->SocketsOfInterest.Num() == 1 && InInputState.IsLeftMouseButtonPressed() && (Widget->GetCurrentAxis() & EAxisList::XYZ) != 0 )
		{
			const bool bAltDown = InInputState.IsAltButtonPressed();

			if ( bAltDown && PersonaPtr.IsValid() )
			{
				// Rather than moving/rotating the selected socket, copy it and move the copy instead
				PersonaPtr.Pin()->DuplicateAndSelectSocket( PreviewSkelMeshComp->SocketsOfInterest[0] );
			}

			// Socket movement is transactional - we want undo/redo and saving of it
			USkeletalMeshSocket* Socket = PreviewSkelMeshComp->SocketsOfInterest[0].Socket;

			if ( Socket && bInTransaction == false )
			{
				if (WidgetMode == FWidget::WM_Rotate )
				{
					GEditor->BeginTransaction( LOCTEXT("AnimationEditorViewport_RotateSocket", "Rotate Socket" ) );
				}
				else
				{
					GEditor->BeginTransaction( LOCTEXT("AnimationEditorViewport_TranslateSocket", "Translate Socket" ) );
				}

				Socket->SetFlags( RF_Transactional );	// Undo doesn't work without this!
				Socket->Modify();
				bInTransaction = true;
			}
		}

		bManipulating = true;
	}
}

void FAnimationViewportClient::TrackingStopped() 
{
	if (bManipulating)
	{
		// Socket movement is transactional - we want undo/redo and saving of it
		if ( bInTransaction )
		{
			GEditor->EndTransaction();
			bInTransaction = false;
		}

		bManipulating = false;
	}
	Invalidate();
}

FWidget::EWidgetMode FAnimationViewportClient::GetWidgetMode() const
{
	if ((PreviewSkelMeshComp != NULL) && ((PreviewSkelMeshComp->BonesOfInterest.Num() == 1) || (PreviewSkelMeshComp->SocketsOfInterest.Num() == 1)))
	{
		return WidgetMode;
	}
	else if (SelectedWindActor.IsValid())
	{
		return WidgetMode;
	}
	else
	{
		return FWidget::WM_None;
	}
}

FVector FAnimationViewportClient::GetWidgetLocation() const
{
	if( PreviewSkelMeshComp->BonesOfInterest.Num() > 0 )
	{
		int32 BoneIndex = PreviewSkelMeshComp->BonesOfInterest.Last();
		const FName BoneName = PreviewSkelMeshComp->SkeletalMesh->RefSkeleton.GetBoneName(BoneIndex);

		FMatrix BoneMatrix = PreviewSkelMeshComp->GetBoneMatrix(BoneIndex);

		return BoneMatrix.GetOrigin();
	}
	else if( PreviewSkelMeshComp->SocketsOfInterest.Num() > 0 )
	{
		USkeletalMeshSocket* Socket = PreviewSkelMeshComp->SocketsOfInterest.Last().Socket;

 		FMatrix SocketMatrix;
		Socket->GetSocketMatrix( SocketMatrix, PreviewSkelMeshComp.Get() );

		return SocketMatrix.GetOrigin();
	}
	else if( SelectedWindActor.IsValid() )
	{
		return SelectedWindActor.Get()->GetActorLocation();
	}

	return FVector::ZeroVector;
}

FMatrix FAnimationViewportClient::GetWidgetCoordSystem() const
{
	const bool bIsLocal = GetWidgetCoordSystemSpace() == COORD_Local;

	if( bIsLocal )
	{
		if ( PreviewSkelMeshComp->BonesOfInterest.Num() > 0 )
		{
			int32 BoneIndex = PreviewSkelMeshComp->BonesOfInterest.Last();

			FMatrix BoneMatrix = PreviewSkelMeshComp->GetBoneMatrix(BoneIndex);

			return BoneMatrix.RemoveTranslation();
		}
		else if( PreviewSkelMeshComp->SocketsOfInterest.Num() > 0 )
		{
			USkeletalMeshSocket* Socket = PreviewSkelMeshComp->SocketsOfInterest.Last().Socket;

			FMatrix SocketMatrix;
			Socket->GetSocketMatrix( SocketMatrix, PreviewSkelMeshComp.Get() );

			return SocketMatrix.RemoveTranslation();
		}
		else if ( SelectedWindActor.IsValid() )
		{
			return SelectedWindActor->GetTransform().ToMatrixNoScale().RemoveTranslation();
		}
	}

	return FMatrix::Identity;
}

ECoordSystem FAnimationViewportClient::GetWidgetCoordSystemSpace() const
{ 
	return GEditorModeTools().GetCoordSystem();
}

void FAnimationViewportClient::SetWidgetCoordSystemSpace(ECoordSystem NewCoordSystem)
{
	GEditorModeTools().SetCoordSystem(NewCoordSystem);
	Invalidate();
}

void FAnimationViewportClient::SetViewMode(EViewModeIndex InViewModeIndex)
{
	FEditorViewportClient::SetViewMode(InViewModeIndex);

	ConfigOption->SetViewModeIndex(InViewModeIndex);
}

void FAnimationViewportClient::SetViewportType(ELevelViewportType InViewportType)
{
	FEditorViewportClient::SetViewportType(InViewportType);
	FocusViewportOnPreviewMesh();
}

bool FAnimationViewportClient::InputKey( FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool bGamepad )
{
	const int32 HitX = Viewport->GetMouseX();
	const int32 HitY = Viewport->GetMouseY();

	bool bHandled = false;

	
	// Handle switching modes - only allowed when not already manipulating
	if ( (Event == IE_Pressed) && (Key == EKeys::SpaceBar) && !bManipulating )
	{
		int32 SelectedBone = FindSelectedBone();
		USkeletalMeshSocket* SelectedSocket = FindSelectedSocket();
		TWeakObjectPtr<AWindDirectionalSource> SelectedWind = FindSelectedWindActor();

		if (SelectedBone >= 0 || SelectedSocket || SelectedWind.IsValid())
		{
			if (WidgetMode == FWidget::WM_Rotate)
			{
				WidgetMode = FWidget::WM_Translate;
			}
			else
			{
				WidgetMode = FWidget::WM_Rotate;
			}

			//@TODO: ANIMATION: Add scaling support
		}
		bHandled = true;
		Invalidate();
	}

	if( (Event == IE_Pressed) && (Key == EKeys::F) )
	{
		bHandled = true;
		FocusViewportOnPreviewMesh();
	}

	// Pass keys to standard controls, if we didn't consume input
	return (bHandled)
		? true
		: FEditorViewportClient::InputKey(Viewport,  ControllerId,  Key,  Event,  AmountDepressed,  bGamepad);
}

void FAnimationViewportClient::SetWidgetMode(FWidget::EWidgetMode InMode)
{
	WidgetMode = InMode;

	// Force viewport to redraw
	Viewport->Invalidate();
}

bool FAnimationViewportClient::CanSetWidgetMode(FWidget::EWidgetMode NewMode) const
{
	//@TODO: ANIMATION: Add scaling support here
	return (NewMode != FWidget::WM_Scale);
}

void FAnimationViewportClient::SetLocalAxesMode(ELocalAxesMode::Type AxesMode)
{
	LocalAxesMode = AxesMode;
	ConfigOption->SetDefaultLocalAxesSelection( AxesMode );
}

bool FAnimationViewportClient::IsLocalAxesModeSet( ELocalAxesMode::Type AxesMode ) const
{
	return LocalAxesMode == AxesMode;
}

void FAnimationViewportClient::DrawBonesFromTransforms(TArray<FTransform>& Transforms, UDebugSkelMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI, FLinearColor BoneColour, FLinearColor RootBoneColour) const
{
	if ( Transforms.Num() > 0 )
	{
		TArray<FTransform> WorldTransforms;
		WorldTransforms.AddUninitialized(Transforms.Num());

		TArray<FLinearColor> BoneColours;
		BoneColours.AddUninitialized(Transforms.Num());

		// we could cache parent bones as we calculate, but right now I'm not worried about perf issue of this
		for ( int32 Index=0; Index < MeshComponent->RequiredBones.Num(); ++Index )
		{
			const int32 BoneIndex = MeshComponent->RequiredBones[Index];
			const int32 ParentIndex = MeshComponent->SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);

			WorldTransforms[BoneIndex] = Transforms[BoneIndex] * MeshComponent->ComponentToWorld;
			BoneColours[BoneIndex] = (ParentIndex >= 0) ? BoneColour : RootBoneColour;
		}

		DrawBones(MeshComponent, MeshComponent->RequiredBones, WorldTransforms, PDI, BoneColours);
	}
}

void FAnimationViewportClient::DrawMeshBonesCompressedAnimation(UDebugSkelMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI) const
{
	if ( MeshComponent && MeshComponent->SkeletalMesh )
	{
		DrawBonesFromTransforms(MeshComponent->CompressedSpaceBases, MeshComponent, PDI, FColor(255, 127, 39, 255), FColor(255, 127, 39, 255));
	}
}

void FAnimationViewportClient::DrawMeshBonesNonRetargetedAnimation(UDebugSkelMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI) const
{
	if ( MeshComponent && MeshComponent->SkeletalMesh )
	{
		DrawBonesFromTransforms(MeshComponent->NonRetargetedSpaceBases, MeshComponent, PDI, FColor(159, 159, 39, 255), FColor(159, 159, 39, 255));
	}
}

void FAnimationViewportClient::DrawMeshBonesAdditiveBasePose(UDebugSkelMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI) const
{
	if ( MeshComponent && MeshComponent->SkeletalMesh )
	{
		DrawBonesFromTransforms(MeshComponent->AdditiveBasePoses, MeshComponent, PDI, FColor(0, 159, 0, 255), FColor(0, 159, 0, 255));
	}
}

void FAnimationViewportClient::DrawMeshBones(USkeletalMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI) const
{
	if ( MeshComponent && MeshComponent->SkeletalMesh )
	{
		TArray<FTransform> WorldTransforms;
		WorldTransforms.AddUninitialized(MeshComponent->SpaceBases.Num());

		TArray<FLinearColor> BoneColours;
		BoneColours.AddUninitialized(MeshComponent->SpaceBases.Num());

		TArray<int32> SelectedBones;
		if(UDebugSkelMeshComponent* DebugMeshComponent = Cast<UDebugSkelMeshComponent>(MeshComponent))
		{
			SelectedBones = DebugMeshComponent->BonesOfInterest;
		}

		// we could cache parent bones as we calculate, but right now I'm not worried about perf issue of this
		for ( int32 Index=0; Index<MeshComponent->RequiredBones.Num(); ++Index )
		{
			const int32 BoneIndex = MeshComponent->RequiredBones[Index];
			const int32 ParentIndex = MeshComponent->SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);

			WorldTransforms[BoneIndex] = MeshComponent->SpaceBases[BoneIndex]*MeshComponent->ComponentToWorld;
			
			if(SelectedBones.Contains(BoneIndex))
			{
				BoneColours[BoneIndex] = FLinearColor(1.0f, 0.34f, 0.0f, 1.0f);
			}
			else
			{
				BoneColours[BoneIndex] = (ParentIndex >= 0) ? FLinearColor::White : FLinearColor::Red;
			}
		}

		DrawBones(MeshComponent, MeshComponent->RequiredBones, WorldTransforms, PDI, BoneColours);
	}
}

void FAnimationViewportClient::DrawBones(const USkeletalMeshComponent * MeshComponent, const TArray<FBoneIndexType> & RequiredBones, const TArray<FTransform> & WorldTransforms, FPrimitiveDrawInterface* PDI, const TArray<FLinearColor> BoneColours, float LineThickness/*=0.f*/) const
{
	check ( MeshComponent && MeshComponent->SkeletalMesh );

	TArray<int32> SelectedBones;
	if(const UDebugSkelMeshComponent* DebugMeshComponent = Cast<const UDebugSkelMeshComponent>(MeshComponent))
	{
		SelectedBones = DebugMeshComponent->BonesOfInterest;

		// we could cache parent bones as we calculate, but right now I'm not worried about perf issue of this
		for ( int32 Index=0; Index<RequiredBones.Num(); ++Index )
		{
			const int32 BoneIndex = RequiredBones[Index];
			const int32 ParentIndex = MeshComponent->SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
			FVector Start, End;
			FLinearColor LineColor = BoneColours[BoneIndex];

			if (ParentIndex >=0)
			{
				Start = WorldTransforms[ParentIndex].GetLocation();
				End = WorldTransforms[BoneIndex].GetLocation();
			}
			else
			{
				Start = FVector::ZeroVector;
				End = WorldTransforms[BoneIndex].GetLocation();
			}

			static const float SphereRadius = 1.0f;
			TArray<FVector> Verts;

			//Calc cone size 
			FVector EndToStart = (Start-End);
			float ConeLength = EndToStart.Size();
			float Angle = FMath::RadiansToDegrees(FMath::Atan(SphereRadius / ConeLength));

			//Render Sphere for bone end point and a cone between it and its parent.
			PDI->SetHitProxy( new HPersonaBoneProxy( MeshComponent->SkeletalMesh->RefSkeleton.GetBoneName(BoneIndex)) );
			DrawWireSphere(PDI, End, LineColor, SphereRadius, 10, SDPG_Foreground);
			DrawWireCone(PDI, FRotationMatrix::MakeFromX(EndToStart)*FTranslationMatrix(End), ConeLength, Angle, 4, LineColor, SDPG_Foreground, Verts);
			PDI->SetHitProxy( NULL );
		
			// draw gizmo
			if( (LocalAxesMode == ELocalAxesMode::All) || 
				((LocalAxesMode == ELocalAxesMode::Selected) && SelectedBones.Contains(BoneIndex)) 
			  )
			{
				RenderGizmo(WorldTransforms[BoneIndex], PDI);
			}
		}
	}
}

void FAnimationViewportClient::RenderGizmo(const FTransform & Transform, FPrimitiveDrawInterface* PDI) const
{
	// Display colored coordinate system axes for this joint.
	const float AxisLength = 3.75f;
	const float LineThickness = 0.3f;
	const FVector Origin = Transform.GetLocation();

	// Red = X
	FVector XAxis = Transform.TransformVector( FVector(1.0f,0.0f,0.0f) );
	XAxis.Normalize();
	PDI->DrawLine( Origin, Origin + XAxis * AxisLength, FColor( 255, 80, 80),SDPG_Foreground, LineThickness);			

	// Green = Y
	FVector YAxis = Transform.TransformVector( FVector(0.0f,1.0f,0.0f) );
	YAxis.Normalize();
	PDI->DrawLine( Origin, Origin + YAxis * AxisLength, FColor( 80, 255, 80),SDPG_Foreground, LineThickness); 

	// Blue = Z
	FVector ZAxis = Transform.TransformVector( FVector(0.0f,0.0f,1.0f) );
	ZAxis.Normalize();
	PDI->DrawLine( Origin, Origin + ZAxis * AxisLength, FColor( 80, 80, 255),SDPG_Foreground, LineThickness); 
}

void FAnimationViewportClient::DrawMeshSubsetBones(const USkeletalMeshComponent * MeshComponent, const TArray<int32>& BonesOfInterest, FPrimitiveDrawInterface* PDI) const
{
	// this BonesOfInterest has to be in MeshComponent base, not Skeleton 
	if ( MeshComponent && MeshComponent->SkeletalMesh && BonesOfInterest.Num() > 0 )
	{
		TArray<FTransform> WorldTransforms;
		WorldTransforms.AddUninitialized(MeshComponent->SpaceBases.Num());

		TArray<FLinearColor> BoneColours;
		BoneColours.AddUninitialized(MeshComponent->SpaceBases.Num());

		TArray<FBoneIndexType> RequiredBones;

		const FReferenceSkeleton & RefSkeleton = MeshComponent->SkeletalMesh->RefSkeleton;
		const FSlateColor SelectionColor = FEditorStyle::GetSlateColor("SelectionColor");
		const FLinearColor LinearSelectionColor( SelectionColor.IsColorSpecified() ? SelectionColor.GetSpecifiedColor() : FLinearColor::White );

		// we could cache parent bones as we calculate, but right now I'm not worried about perf issue of this
		for ( auto Iter = MeshComponent->RequiredBones.CreateConstIterator(); Iter; ++Iter)
		{
			const int32 BoneIndex = *Iter;
			bool bDrawBone = false;

			const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);

			// need to see if it's child of any of Bones of interest
			for (auto SubIter=BonesOfInterest.CreateConstIterator(); SubIter; ++SubIter )
			{
				const int32 SubBoneIndex = *SubIter;
				// if I'm child of the BonesOfInterest
				if(BoneIndex == SubBoneIndex)
				{
					//found a bone we are interested in
					if(ParentIndex >= 0)
					{
						WorldTransforms[ParentIndex] = MeshComponent->SpaceBases[ParentIndex]*MeshComponent->ComponentToWorld;
					}
					BoneColours[BoneIndex] = LinearSelectionColor;
					bDrawBone = true;
					break;
				}
				else if ( RefSkeleton.BoneIsChildOf(BoneIndex, SubBoneIndex) )
				{
					BoneColours[BoneIndex] = FLinearColor::White;
					bDrawBone = true;
					break;
				}
			}

			if (bDrawBone)
			{
				//add to the list
				RequiredBones.AddUnique(BoneIndex);
				WorldTransforms[BoneIndex] = MeshComponent->SpaceBases[BoneIndex]*MeshComponent->ComponentToWorld;
			}
		}

		DrawBones(MeshComponent, RequiredBones, WorldTransforms, PDI, BoneColours, 0.3f);
	}
}

void FAnimationViewportClient::DrawSockets( TArray<USkeletalMeshSocket*>& Sockets, FPrimitiveDrawInterface* PDI, bool bUseSkeletonSocketColor ) const
{
	if ( const UDebugSkelMeshComponent* DebugMeshComponent = PreviewSkelMeshComp.Get() )
	{
		TArray<FSelectedSocketInfo> SelectedSockets;
		SelectedSockets = DebugMeshComponent->SocketsOfInterest;

		for ( auto SocketIt = Sockets.CreateConstIterator(); SocketIt; ++SocketIt )
		{
			USkeletalMeshSocket* Socket = *(SocketIt);
			FReferenceSkeleton& RefSkeleton = DebugMeshComponent->SkeletalMesh->RefSkeleton;

			const int32 ParentIndex = RefSkeleton.FindBoneIndex(Socket->BoneName);

			FTransform WorldTransformSocket = Socket->GetSocketTransform(DebugMeshComponent);

			FLinearColor SocketColor;
			FVector Start, End;
			if (ParentIndex >=0)
			{
				FTransform WorldTransformParent = DebugMeshComponent->SpaceBases[ParentIndex]*DebugMeshComponent->ComponentToWorld;
				Start = WorldTransformParent.GetLocation();
				End = WorldTransformSocket.GetLocation();
			}
			else
			{
				Start = FVector::ZeroVector;
				End = WorldTransformSocket.GetLocation();
			}

			const bool bSelectedSocket = SelectedSockets.ContainsByPredicate([Socket](const FSelectedSocketInfo& Info){ return Info.Socket == Socket; });

			if( bSelectedSocket )
			{
				SocketColor = FLinearColor(1.0f, 0.34f, 0.0f, 1.0f);
			}
			else
			{
				SocketColor = (ParentIndex >= 0) ? FLinearColor::White : FLinearColor::Red;
			}

			static const float SphereRadius = 1.0f;
			TArray<FVector> Verts;

			//Calc cone size 
			FVector EndToStart = (Start-End);
			float ConeLength = EndToStart.Size();
			float Angle = FMath::RadiansToDegrees(FMath::Atan(SphereRadius / ConeLength));

			//Render Sphere for bone end point and a cone between it and its parent.
			PDI->SetHitProxy( new HPersonaBoneProxy( Socket->BoneName ) );
			PDI->DrawLine( Start, End, SocketColor, SDPG_Foreground );
			PDI->SetHitProxy( NULL );
			
			// draw gizmo
			if( (LocalAxesMode == ELocalAxesMode::All) || bSelectedSocket )
			{
				FMatrix SocketMatrix;
				Socket->GetSocketMatrix( SocketMatrix, PreviewSkelMeshComp.Get() );

				PDI->SetHitProxy( new HPersonaSocketProxy( FSelectedSocketInfo( Socket, bUseSkeletonSocketColor ) ) );
				DrawWireDiamond( PDI, SocketMatrix, 2.f, SocketColor, SDPG_Foreground );
				PDI->SetHitProxy( NULL );
				
				RenderGizmo(FTransform(SocketMatrix), PDI);
			}
		}
	}
}

FSphere FAnimationViewportClient::GetCameraTarget()
{
	bool bFoundTarget = false;
	FSphere Sphere(FVector(0,0,0), 100.0f); // default
	if( PreviewSkelMeshComp.IsValid() )
	{
		FTransform ComponentToWorld = PreviewSkelMeshComp->ComponentToWorld;
		PreviewSkelMeshComp.Get()->CalcBounds(ComponentToWorld);

		if( PreviewSkelMeshComp->BonesOfInterest.Num() > 0 )
		{
			const int32 FocusBoneIndex = PreviewSkelMeshComp->BonesOfInterest[0];
			if( FocusBoneIndex != INDEX_NONE )
			{
				const FName BoneName = PreviewSkelMeshComp->SkeletalMesh->RefSkeleton.GetBoneName(FocusBoneIndex);
				Sphere.Center = PreviewSkelMeshComp->GetBoneLocation(BoneName);
				Sphere.W = 30.0f;
				bFoundTarget = true;
			}
		}


		if( !bFoundTarget && (PreviewSkelMeshComp->SocketsOfInterest.Num() > 0) )
		{
			USkeletalMeshSocket * Socket = PreviewSkelMeshComp->SocketsOfInterest[0].Socket;
			if( Socket )
			{
				Sphere.Center = Socket->GetSocketLocation( PreviewSkelMeshComp.Get() );
				Sphere.W = 30.0f;
				bFoundTarget = true;
			}
		}

		if( !bFoundTarget )
		{
			FBoxSphereBounds Bounds = PreviewSkelMeshComp.Get()->CalcBounds(FTransform::Identity);
			Sphere = Bounds.GetSphere();
		}
	}
	return Sphere;
}

void FAnimationViewportClient::UpdateCameraSetup()
{
	static FRotator CustomOrbitRotation(-33.75, -135, 0);
	if (PreviewSkelMeshComp.IsValid() && PreviewSkelMeshComp->SkeletalMesh)
	{
		FSphere BoundSphere = GetCameraTarget();
		FVector CustomOrbitZoom(0, BoundSphere.W / (75.0f * (float)PI / 360.0f), 0);
		FVector CustomOrbitLookAt = BoundSphere.Center;

		SetCameraSetup(CustomOrbitLookAt, CustomOrbitRotation, CustomOrbitZoom, CustomOrbitLookAt, GetViewLocation(), GetViewRotation() );

		// Move the floor to the bottom of the bounding box of the mesh, rather than on the origin
		FVector Bottom = PreviewSkelMeshComp->Bounds.GetBoxExtrema(0);

		if ( EditorFloorComp )
		{
			EditorFloorComp->SetWorldTransform( FTransform( FQuat::Identity, FVector( 0.0f, 0.0f, Bottom.Z + GetFloorOffset() ), FVector( 3.0f, 3.0f, 1.0f ) ) );
		}
	}
}

void FAnimationViewportClient::FocusViewportOnSphere( FSphere& Sphere )
{
	FBox Box( Sphere.Center - FVector(Sphere.W), Sphere.Center + FVector(Sphere.W) );

	bool bInstant = false;
	FocusViewportOnBox( Box, bInstant );

	Invalidate();
}

void FAnimationViewportClient::FocusViewportOnPreviewMesh()
{
	FSphere Sphere = GetCameraTarget();
	FocusViewportOnSphere(Sphere);
}

float FAnimationViewportClient::GetFloorOffset() const
{
	if ( PersonaPtr.IsValid() )
	{
		USkeletalMesh* Mesh = PersonaPtr.Pin()->GetMesh();

		if ( Mesh )
		{
			return Mesh->FloorOffset;
		}
	}

	return 0.0f;
}

void FAnimationViewportClient::SetFloorOffset( float NewValue )
{
	if ( PersonaPtr.IsValid() )
	{
		USkeletalMesh* Mesh = PersonaPtr.Pin()->GetMesh();

		if ( Mesh )
		{
			// This value is saved in a UPROPERTY for the mesh, so changes are transactional
			FScopedTransaction Transaction( LOCTEXT( "SetFloorOffset", "Set Floor Offset" ) );
			Mesh->Modify();

			Mesh->FloorOffset = NewValue;
			UpdateCameraSetup(); // This does the actual moving of the floor mesh
			Invalidate();
		}
	}
}

TWeakObjectPtr<AWindDirectionalSource> FAnimationViewportClient::CreateWindActor(UWorld* World)
{
	TWeakObjectPtr<AWindDirectionalSource> Wind = World->SpawnActor<AWindDirectionalSource>(PrevWindLocation, PrevWindRotation);

	check(Wind.IsValid());
	//initial wind strength value 
	Wind->Component->Strength = PrevWindStrength;
	return Wind;
}

void FAnimationViewportClient::EnableWindActor(bool bEnableWind)
{
	UWorld* World = PersonaPtr.Pin()->PreviewComponent->GetWorld();
	check(World);

	if(bEnableWind)
	{
		if(!WindSourceActor.IsValid())
		{
			WindSourceActor = CreateWindActor(World);
		}
	}
	else
	{
		PrevWindLocation = WindSourceActor->GetActorLocation();
		PrevWindRotation = WindSourceActor->GetActorRotation();
		PrevWindStrength = WindSourceActor->Component->Strength;

		if(World->DestroyActor(WindSourceActor.Get()))
		{
			//clear the gizmo (translation or rotation)
			PersonaPtr.Pin()->DeselectAll();
		}
	}
}

void FAnimationViewportClient::SetWindStrength( float SliderPos )
{
	if(WindSourceActor.IsValid())
	{
		//Clamp grid size slider value between 0 - 1
		WindSourceActor->Component->Strength = FMath::Clamp<float>(SliderPos, 0.0f, 1.0f);
		//to apply this new wind strength
		WindSourceActor->UpdateComponentTransforms();
	}
}

float FAnimationViewportClient::GetWindStrengthSliderValue() const
{
	if(WindSourceActor.IsValid())
	{
		return WindSourceActor->Component->Strength;
	}

	return 0;
}

FString FAnimationViewportClient::GetWindStrengthLabel() const
{
	//Clamp slide value so that minimum value displayed is 0.00 and maximum is 1.0
	float SliderValue = FMath::Clamp<float>(GetWindStrengthSliderValue(), 0.0f, 1.0f);

	FString Value = FString::Printf(TEXT("%.2f%"),SliderValue);
	return Value;
}

void FAnimationViewportClient::SetGravityScale( float SliderPos )
{
	GravityScaleSliderValue = SliderPos;
	float RealGravityScale = SliderPos * 4;

	check(PersonaPtr.Pin()->PreviewComponent);

	UWorld* World = PersonaPtr.Pin()->PreviewComponent->GetWorld();
	AWorldSettings* Setting = World->GetWorldSettings();
	Setting->WorldGravityZ = Setting->DefaultGravityZ*RealGravityScale;
	Setting->bWorldGravitySet = true;
}

float FAnimationViewportClient::GetGravityScaleSliderValue() const
{
	return GravityScaleSliderValue;
}

FString FAnimationViewportClient::GetGravityScaleLabel() const
{
	//Clamp slide value so that minimum value displayed is 0.00 and maximum is 4.0
	float SliderValue = FMath::Clamp<float>(GetGravityScaleSliderValue()*4, 0.0f, 4.0f);

	FString Value = FString::Printf(TEXT("%.2f%"),SliderValue);
	return Value;
}

void FAnimationViewportClient::ToggleShowNormals()
{
	if(PreviewSkelMeshComp.IsValid())
	{
		PreviewSkelMeshComp->bDrawNormals = !PreviewSkelMeshComp->bDrawNormals;
		PreviewSkelMeshComp->MarkRenderStateDirty();
		Invalidate();
	}
}

bool FAnimationViewportClient::IsSetShowNormalsChecked() const
{
	if (PreviewSkelMeshComp.IsValid())
	{
		return PreviewSkelMeshComp->bDrawNormals;
	}
	return false;
}

void FAnimationViewportClient::ToggleShowTangents()
{
	if(PreviewSkelMeshComp.IsValid())
	{
		PreviewSkelMeshComp->bDrawTangents = !PreviewSkelMeshComp->bDrawTangents;
		PreviewSkelMeshComp->MarkRenderStateDirty();
		Invalidate();
	}
}

bool FAnimationViewportClient::IsSetShowTangentsChecked() const
{
	if (PreviewSkelMeshComp.IsValid())
	{
		return PreviewSkelMeshComp->bDrawTangents;
	}
	return false;
}

void FAnimationViewportClient::ToggleShowBinormals()
{
	if(PreviewSkelMeshComp.IsValid())
	{
		PreviewSkelMeshComp->bDrawBinormals = !PreviewSkelMeshComp->bDrawBinormals;
		PreviewSkelMeshComp->MarkRenderStateDirty();
		Invalidate();
	}
}

bool FAnimationViewportClient::IsSetShowBinormalsChecked() const
{
	if (PreviewSkelMeshComp.IsValid())
	{
		return PreviewSkelMeshComp->bDrawBinormals;
	}
	return false;
}

void FAnimationViewportClient::ToggleDrawUVOverlay()
{
	bDrawUVs = !bDrawUVs;
	Invalidate();
}

bool FAnimationViewportClient::IsSetDrawUVOverlayChecked() const
{
	return bDrawUVs;
}

void FAnimationViewportClient::OnToggleShowMeshStats()
{
	ConfigOption->SetShowMeshStats(!ConfigOption->bShowMeshStats);
}

bool FAnimationViewportClient::IsShowingMeshStats() const
{
	const bool bShouldBeEnabled = ConfigOption->bShowMeshStats;
	const bool bCanBeEnabled = !PersonaPtr.Pin()->IsModeCurrent(FPersonaModes::AnimBlueprintEditMode);

	return bShouldBeEnabled && bCanBeEnabled;
}

#undef LOCTEXT_NAMESPACE

