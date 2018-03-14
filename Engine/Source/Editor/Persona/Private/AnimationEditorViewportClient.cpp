// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AnimationEditorViewportClient.h"
#include "Modules/ModuleManager.h"
#include "EngineGlobals.h"
#include "Animation/AnimSequence.h"
#include "EditorStyleSet.h"
#include "AnimationEditorPreviewScene.h"
#include "Materials/Material.h"
#include "CanvasItem.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "Animation/AnimBlueprint.h"
#include "Preferences/PersonaOptions.h"
#include "Engine/CollisionProfile.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "GameFramework/WorldSettings.h"
#include "PersonaModule.h"
#include "Rendering/SkeletalMeshRenderData.h"

#include "SEditorViewport.h"
#include "CanvasTypes.h"
#include "AnimPreviewInstance.h"
#include "ScopedTransaction.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Engine/SkeletalMeshSocket.h"
#include "ISkeletonTree.h"
#include "SAnimationEditorViewport.h"
#include "AssetViewerSettings.h"
#include "IPersonaEditorModeManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "SkeletalDebugRendering.h"
#include "SkeletalRenderPublic.h"
#include "AudioDevice.h"
#include "RawIndexBuffer.h"
#include "CameraController.h"
#include "Animation/MorphTarget.h"
#include "Rendering/SkeletalMeshModel.h"

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

namespace EAnimationPlaybackSpeeds
{
	// Speed scales for animation playback, must match EAnimationPlaybackSpeeds::Type
	float Values[EAnimationPlaybackSpeeds::NumPlaybackSpeeds] = { 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f };
}

IMPLEMENT_HIT_PROXY( HPersonaSocketProxy, HHitProxy );
IMPLEMENT_HIT_PROXY( HPersonaBoneProxy, HHitProxy );

#define LOCTEXT_NAMESPACE "FAnimationViewportClient"

/////////////////////////////////////////////////////////////////////////
// FAnimationViewportClient

FAnimationViewportClient::FAnimationViewportClient(const TSharedRef<ISkeletonTree>& InSkeletonTree, const TSharedRef<IPersonaPreviewScene>& InPreviewScene, const TSharedRef<SAnimationEditorViewport>& InAnimationEditorViewport, const TSharedRef<FAssetEditorToolkit>& InAssetEditorToolkit, int32 InViewportIndex, bool bInShowStats)
	: FEditorViewportClient(FModuleManager::LoadModuleChecked<FPersonaModule>("Persona").CreatePersonaEditorModeManager(), &InPreviewScene.Get(), StaticCastSharedRef<SEditorViewport>(InAnimationEditorViewport))
	, SkeletonTreePtr(InSkeletonTree)
	, PreviewScenePtr(InPreviewScene)
	, AssetEditorToolkitPtr(InAssetEditorToolkit)
	, AnimationPlaybackSpeedMode(EAnimationPlaybackSpeeds::Normal)
	, bFocusOnDraw(false)
	, bFocusUsingCustomCamera(false)
	, CachedScreenSize(0.0f)
	, bShowMeshStats(bInShowStats)
	, bInitiallyFocused(false)
	, OrbitRotation(FQuat::Identity)
	, ViewportIndex(InViewportIndex)
{
	// we actually own the mode tools here, we just override its type in the FEditorViewportClient constructor above
	bOwnsModeTools = true;

	CachedDefaultCameraController = CameraController;

	OnCameraControllerChanged();

	InPreviewScene->RegisterOnCameraOverrideChanged(FSimpleDelegate::CreateRaw(this, &FAnimationViewportClient::OnCameraControllerChanged));

	// Let the asset editor toolkit know about the mode manager so it can be used outside of thew viewport
	InAssetEditorToolkit->SetAssetEditorModeManager((FAssetEditorModeManager*)ModeTools);

	Widget->SetUsesEditorModeTools(ModeTools);
	((FAssetEditorModeManager*)ModeTools)->SetPreviewScene(&InPreviewScene.Get());
	((FAssetEditorModeManager*)ModeTools)->SetDefaultMode(FPersonaEditModes::SkeletonSelection);

	// load config
	ConfigOption = UPersonaOptions::StaticClass()->GetDefaultObject<UPersonaOptions>();
	check (ConfigOption);

	// DrawHelper set up
	DrawHelper.PerspectiveGridSize = HALF_WORLD_MAX1;
	DrawHelper.AxesLineThickness = ConfigOption->bHighlightOrigin ? 1.0f : 0.0f;
	DrawHelper.bDrawGrid = true;	// Toggling grid now relies on the show flag

	WidgetMode = FWidget::WM_Rotate;
	ModeTools->SetWidgetMode(WidgetMode);

	EngineShowFlags.Game = 0;
	EngineShowFlags.ScreenSpaceReflections = 1;
	EngineShowFlags.AmbientOcclusion = 1;
	EngineShowFlags.SetSnap(0);
	EngineShowFlags.Grid = ConfigOption->bShowGrid;

	SetRealtime(true);
	if(GEditor->PlayWorld)
	{
		SetRealtime(false,true); // We are PIE, don't start in realtime mode
	}

	ViewFOV = FMath::Clamp<float>(ConfigOption->GetAssetEditorOptions(InAssetEditorToolkit->GetEditorName()).ViewportConfigs[ViewportIndex].ViewFOV, FOVMin, FOVMax);

	EngineShowFlags.SetSeparateTranslucency(true);
	EngineShowFlags.SetCompositeEditorPrimitives(true);

	EngineShowFlags.SetSelectionOutline(true);

	bDrawUVs = false;
	UVChannelToDraw = 0;

	bAutoAlignFloor = ConfigOption->bAutoAlignFloorToMesh;

	// Set audio mute option
	UWorld* World = PreviewScene->GetWorld();
	if(World)
	{
		World->bAllowAudioPlayback = !ConfigOption->bMuteAudio;

		if(FAudioDevice* AudioDevice = World->GetAudioDevice())
		{
			AudioDevice->SetUseAttenuationForNonGameWorlds(ConfigOption->bUseAudioAttenuation);
		}
	}

	InPreviewScene->RegisterOnPreviewMeshChanged(FOnPreviewMeshChanged::CreateRaw(this, &FAnimationViewportClient::HandleSkeletalMeshChanged));
	HandleSkeletalMeshChanged(nullptr, InPreviewScene->GetPreviewMeshComponent()->SkeletalMesh);
	InPreviewScene->RegisterOnInvalidateViews(FSimpleDelegate::CreateRaw(this, &FAnimationViewportClient::HandleInvalidateViews));
	InPreviewScene->RegisterOnFocusViews(FSimpleDelegate::CreateRaw(this, &FAnimationViewportClient::HandleFocusViews));
	InPreviewScene->RegisterOnPreTick(FSimpleDelegate::CreateRaw(this, &FAnimationViewportClient::HandlePreviewScenePreTick));
	InPreviewScene->RegisterOnPostTick(FSimpleDelegate::CreateRaw(this, &FAnimationViewportClient::HandlePreviewScenePostTick));

	UDebugSkelMeshComponent* MeshComponent = InPreviewScene->GetPreviewMeshComponent();
	MeshComponent->SelectionOverrideDelegate = UPrimitiveComponent::FSelectionOverride::CreateRaw(this, &FAnimationViewportClient::PreviewComponentSelectionOverride);
	MeshComponent->PushSelectionToProxy();

	// Register delegate to update the show flags when the post processing is turned on or off
	UAssetViewerSettings::Get()->OnAssetViewerSettingsChanged().AddRaw(this, &FAnimationViewportClient::OnAssetViewerSettingsChanged);
	// Set correct flags according to current profile settings
	SetAdvancedShowFlagsForScene(UAssetViewerSettings::Get()->Profiles[GetMutableDefault<UEditorPerProjectUserSettings>()->AssetViewerProfileIndex].bPostProcessingEnabled);
}

FAnimationViewportClient::~FAnimationViewportClient()
{
	CameraController = CachedDefaultCameraController;

	if (PreviewScenePtr.IsValid())
	{
		TSharedPtr<IPersonaPreviewScene> ScenePtr = PreviewScenePtr.Pin();

		UDebugSkelMeshComponent* MeshComponent = ScenePtr->GetPreviewMeshComponent();
		if (MeshComponent)
		{
			MeshComponent->SelectionOverrideDelegate.Unbind();
		}

		ScenePtr->UnregisterOnPreviewMeshChanged(this);
		ScenePtr->UnregisterOnInvalidateViews(this);
		ScenePtr->UnregisterOnCameraOverrideChanged(this);
		ScenePtr->UnregisterOnPreTick(this);
		ScenePtr->UnregisterOnPostTick(this);
	}

	if (AssetEditorToolkitPtr.IsValid())
	{
		AssetEditorToolkitPtr.Pin()->SetAssetEditorModeManager(nullptr);
	}

	((FAssetEditorModeManager*)ModeTools)->SetPreviewScene(nullptr);

	UAssetViewerSettings::Get()->OnAssetViewerSettingsChanged().RemoveAll(this);
}

void FAnimationViewportClient::OnToggleAutoAlignFloor()
{
	bAutoAlignFloor = !bAutoAlignFloor;
	UpdateCameraSetup();

	ConfigOption->SetAutoAlignFloorToMesh(bAutoAlignFloor);
}

bool FAnimationViewportClient::IsAutoAlignFloor() const
{
	return bAutoAlignFloor;
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

void FAnimationViewportClient::OnToggleUseAudioAttenuation()
{
	ConfigOption->SetUseAudioAttenuation(!ConfigOption->bUseAudioAttenuation);

	UWorld* World = PreviewScene->GetWorld();
	if(World)
	{
		if(FAudioDevice* AudioDevice = GetWorld()->GetAudioDevice())
		{
			AudioDevice->SetUseAttenuationForNonGameWorlds(ConfigOption->bUseAudioAttenuation);
		}
	}
}

bool FAnimationViewportClient::IsUsingAudioAttenuation() const
{
	return ConfigOption->bUseAudioAttenuation;
}

void FAnimationViewportClient::SetCameraFollowMode(EAnimationViewportCameraFollowMode InCameraFollowMode, FName InBoneName)
{
	bool bCanFollow = true;
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if(InCameraFollowMode == EAnimationViewportCameraFollowMode::Bone && PreviewMeshComponent)
	{
		bCanFollow = PreviewMeshComponent->GetBoneIndex(InBoneName) != INDEX_NONE;
	}

	if(bCanFollow && InCameraFollowMode != EAnimationViewportCameraFollowMode::None)
	{
		ConfigOption->SetViewCameraFollow(AssetEditorToolkitPtr.Pin()->GetEditorName(), InCameraFollowMode, InBoneName, ViewportIndex);

		CameraFollowMode = InCameraFollowMode;
		CameraFollowBoneName = InBoneName;

		bCameraLock = true;
		bUsingOrbitCamera = true;

		if (PreviewMeshComponent != nullptr)
		{
			switch(CameraFollowMode)
			{
			case EAnimationViewportCameraFollowMode::Bounds:
				{
					FBoxSphereBounds Bound = PreviewMeshComponent->CalcBounds(PreviewMeshComponent->GetComponentTransform());
					SetLookAtLocation(Bound.Origin, true);
					OrbitRotation = FQuat::Identity;
				}
				break;
			case EAnimationViewportCameraFollowMode::Bone:
				{
					FVector BoneLocation = PreviewMeshComponent->GetBoneLocation(InBoneName);
					SetLookAtLocation(BoneLocation, true);
					OrbitRotation = PreviewMeshComponent->GetBoneQuaternion(InBoneName) * FQuat(FVector(0.0f, 1.0f, 0.0f), PI * 0.5f);
				}
				break;
			}
		}

		SetViewLocation(GetViewTransform().ComputeOrbitMatrix().Inverse().GetOrigin());
	}
	else
	{
		ConfigOption->SetViewCameraFollow(AssetEditorToolkitPtr.Pin()->GetEditorName(), EAnimationViewportCameraFollowMode::None, NAME_None, ViewportIndex);

		CameraFollowMode = EAnimationViewportCameraFollowMode::None;
		CameraFollowBoneName = NAME_None;

		OrbitRotation = FQuat::Identity;
		EnableCameraLock(false);
		FocusViewportOnPreviewMesh(false);
		Invalidate();
	}
}

EAnimationViewportCameraFollowMode FAnimationViewportClient::GetCameraFollowMode() const
{
	return CameraFollowMode;
}

FName FAnimationViewportClient::GetCameraFollowBoneName() const
{
	return CameraFollowBoneName;
}

void FAnimationViewportClient::JumpToDefaultCamera()
{
	FocusViewportOnPreviewMesh(true);
}


void FAnimationViewportClient::SaveCameraAsDefault()
{
	USkeletalMesh* SkelMesh = GetAnimPreviewScene()->GetPreviewMeshComponent()->SkeletalMesh;
	if (SkelMesh)
	{
		FScopedTransaction Transaction(LOCTEXT("SaveCameraAsDefault", "Save Camera As Default"));

		FViewportCameraTransform& ViewTransform = GetViewTransform();
		SkelMesh->Modify();
		SkelMesh->DefaultEditorCameraLocation = ViewTransform.GetLocation();
		SkelMesh->DefaultEditorCameraRotation = ViewTransform.GetRotation();
		SkelMesh->DefaultEditorCameraLookAt = ViewTransform.GetLookAt();
		SkelMesh->DefaultEditorCameraOrthoZoom = ViewTransform.GetOrthoZoom();
		SkelMesh->bHasCustomDefaultEditorCamera = true;

		// Create and display a notification 
		const FText NotificationText = FText::Format(LOCTEXT("SavedDefaultCamera", "Saved default camera for {0}"), FText::AsCultureInvariant(SkelMesh->GetName()));
		FNotificationInfo Info(NotificationText);
		Info.ExpireDuration = 2.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

bool FAnimationViewportClient::CanSaveCameraAsDefault() const
{
	return CameraFollowMode == EAnimationViewportCameraFollowMode::None;
}

void FAnimationViewportClient::ClearDefaultCamera()
{
	USkeletalMesh* SkelMesh = GetAnimPreviewScene()->GetPreviewMeshComponent()->SkeletalMesh;
	if (SkelMesh)
	{
		FScopedTransaction Transaction(LOCTEXT("ClearDefaultCamera", "Clear Default Camera"));

		SkelMesh->Modify();
		SkelMesh->bHasCustomDefaultEditorCamera = false;

		// Create and display a notification 
		const FText NotificationText = FText::Format(LOCTEXT("ClearedDefaultCamera", "Cleared default camera for {0}"), FText::AsCultureInvariant(SkelMesh->GetName()));
		FNotificationInfo Info(NotificationText);
		Info.ExpireDuration = 2.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

bool FAnimationViewportClient::HasDefaultCameraSet() const
{
	USkeletalMesh* SkelMesh = GetAnimPreviewScene()->GetPreviewMeshComponent()->SkeletalMesh;
	return (SkelMesh && SkelMesh->bHasCustomDefaultEditorCamera);
}

void FAnimationViewportClient::HandleSkeletalMeshChanged(USkeletalMesh* OldSkeletalMesh, USkeletalMesh* NewSkeletalMesh)
{
	if (OldSkeletalMesh != NewSkeletalMesh || NewSkeletalMesh == nullptr)
	{
		GetSkeletonTree()->DeselectAll();

		if (!bInitiallyFocused)
		{
			FocusViewportOnPreviewMesh(true);
			bInitiallyFocused = true;
		}

		UpdateCameraSetup();
	}

	// Setup physics data from physics assets if available, clearing any physics setup on the component
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	UPhysicsAsset* PhysAsset = PreviewMeshComponent->GetPhysicsAsset();
	if(PhysAsset)
	{
		PhysAsset->InvalidateAllPhysicsMeshes();
		PreviewMeshComponent->TermArticulated();
		PreviewMeshComponent->InitArticulated(GetWorld()->GetPhysicsScene());

		// Set to PhysicsActor to enable tracing regardless of project overrides
		static FName CollisionProfileName(TEXT("PhysicsActor"));
		PreviewMeshComponent->SetCollisionProfileName(CollisionProfileName);
	}

	Invalidate();
}

void FAnimationViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);

	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (PreviewMeshComponent->SkeletalMesh)
	{
		// Can't have both bones of interest and sockets of interest set
		check( !(GetAnimPreviewScene()->GetSelectedBoneIndex() != INDEX_NONE && GetAnimPreviewScene()->GetSelectedSocket().IsValid() ) )

		// if we have BonesOfInterest, draw sub set of the bones only
		if (GetAnimPreviewScene()->GetSelectedBoneIndex() != INDEX_NONE)
		{
			DrawMeshSubsetBones(PreviewMeshComponent, PreviewMeshComponent->BonesOfInterest, PDI);
		}
		// otherwise, if we display bones, display
		if ( GetBoneDrawMode() != EBoneDrawMode::None )
		{
			DrawMeshBones(PreviewMeshComponent, PDI);
		}
		if (PreviewMeshComponent->bDisplayRawAnimation )
		{
			DrawMeshBonesUncompressedAnimation(PreviewMeshComponent, PDI);
		}
		if (PreviewMeshComponent->NonRetargetedSpaceBases.Num() > 0 )
		{
			DrawMeshBonesNonRetargetedAnimation(PreviewMeshComponent, PDI);
		}
		if(PreviewMeshComponent->bDisplayAdditiveBasePose )
		{
			DrawMeshBonesAdditiveBasePose(PreviewMeshComponent, PDI);
		}
		if(PreviewMeshComponent->bDisplayBakedAnimation)
		{
			DrawMeshBonesBakedAnimation(PreviewMeshComponent, PDI);
		}
		if(PreviewMeshComponent->bDisplaySourceAnimation)
		{
			DrawMeshBonesSourceRawAnimation(PreviewMeshComponent, PDI);
		}
		
		DrawWatchedPoses(PreviewMeshComponent, PDI);

		PreviewMeshComponent->DebugDrawClothing(PDI);

		
		// Display socket hit points
		if (PreviewMeshComponent->bDrawSockets )
		{
			if (PreviewMeshComponent->bSkeletonSocketsVisible && PreviewMeshComponent->SkeletalMesh->Skeleton )
			{
				DrawSockets(PreviewMeshComponent, PreviewMeshComponent->SkeletalMesh->Skeleton->Sockets, FSelectedSocketInfo(), PDI, true);
			}

			if ( PreviewMeshComponent->bMeshSocketsVisible )
			{
				DrawSockets(PreviewMeshComponent, PreviewMeshComponent->SkeletalMesh->GetMeshOnlySocketList(), FSelectedSocketInfo(), PDI, false);
			}
		}
	}

	if (bFocusOnDraw)
	{
		bFocusOnDraw = false;
		FocusViewportOnPreviewMesh(bFocusUsingCustomCamera);
	}

	// set camera mode if need be (we need to do this here as focus on draw can take us out of orbit mode)
	FAssetEditorOptions& Options = ConfigOption->GetAssetEditorOptions(AssetEditorToolkitPtr.Pin()->GetEditorName());
	if(Options.ViewportConfigs[ViewportIndex].CameraFollowMode != CameraFollowMode)
	{
		SetCameraFollowMode(Options.ViewportConfigs[ViewportIndex].CameraFollowMode, Options.ViewportConfigs[ViewportIndex].CameraFollowBoneName);
	}
}

void FAnimationViewportClient::DrawCanvas( FViewport& InViewport, FSceneView& View, FCanvas& Canvas )
{
	FEditorViewportClient::DrawCanvas(InViewport, View, Canvas);

	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (PreviewMeshComponent != nullptr)
	{
		// Display bone names
		if (PreviewMeshComponent->bShowBoneNames)
		{
			ShowBoneNames(&Canvas, &View);
		}

		if (bDrawUVs)
		{
			DrawUVsForMesh(Viewport, &Canvas, 1.0f);
		}
	}
}

void FAnimationViewportClient::DrawUVsForMesh(FViewport* InViewport, FCanvas* InCanvas, int32 InTextYPos)
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();

	//use the overridden LOD level
	const uint32 LODLevel = FMath::Clamp(PreviewMeshComponent->ForcedLodModel - 1, 0, PreviewMeshComponent->SkeletalMesh->LODInfo.Num() - 1);

	TArray<FVector2D> SelectedEdgeTexCoords; //No functionality in Persona for this (yet?)

	DrawUVs(InViewport, InCanvas, InTextYPos, LODLevel, UVChannelToDraw, SelectedEdgeTexCoords, NULL, &PreviewMeshComponent->GetSkeletalMeshRenderData()->LODRenderData[LODLevel] );
}

void FAnimationViewportClient::Tick(float DeltaSeconds) 
{
	FEditorViewportClient::Tick(DeltaSeconds);

	GetAnimPreviewScene()->FlagTickable();
}

void FAnimationViewportClient::HandlePreviewScenePreTick()
{
	RelativeViewLocation = FVector::ZeroVector;

	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (CameraFollowMode != EAnimationViewportCameraFollowMode::None && PreviewMeshComponent != nullptr)
	{
		switch(CameraFollowMode)
		{
		case EAnimationViewportCameraFollowMode::Bounds:
			{
				FBoxSphereBounds Bound = PreviewMeshComponent->CalcBounds(PreviewMeshComponent->GetComponentTransform());
				RelativeViewLocation = Bound.Origin - GetViewLocation();
			}
			break;
		case EAnimationViewportCameraFollowMode::Bone:
			{
				int32 BoneIndex = PreviewMeshComponent->GetBoneIndex(CameraFollowBoneName);
				if(BoneIndex != INDEX_NONE)
				{
					FTransform BoneTransform = PreviewMeshComponent->GetBoneTransform(BoneIndex);
					RelativeViewLocation = BoneTransform.InverseTransformVector(BoneTransform.GetLocation() - GetViewLocation());
				}
			}
			break;
		}
	}
}

void FAnimationViewportClient::HandlePreviewScenePostTick()
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (CameraFollowMode != EAnimationViewportCameraFollowMode::None && PreviewMeshComponent != nullptr)
	{
		switch(CameraFollowMode)
		{
		case EAnimationViewportCameraFollowMode::Bounds:
			{
				FBoxSphereBounds Bound = PreviewMeshComponent->CalcBounds(PreviewMeshComponent->GetComponentTransform());
				SetViewLocation(Bound.Origin + RelativeViewLocation);
				SetLookAtLocation(Bound.Origin);
			}
			break;
		case EAnimationViewportCameraFollowMode::Bone:
			{
				int32 BoneIndex = PreviewMeshComponent->GetBoneIndex(CameraFollowBoneName);
				if(BoneIndex != INDEX_NONE)
				{
					FTransform BoneTransform = PreviewMeshComponent->GetBoneTransform(BoneIndex);
					if(IsPerspective())
					{
						SetViewLocation(BoneTransform.GetLocation() - BoneTransform.TransformVector(RelativeViewLocation));
					}
					else
					{
						SetViewLocation(BoneTransform.GetLocation());
					}
					SetLookAtLocation(BoneTransform.GetLocation());
					OrbitRotation = BoneTransform.GetRotation() * FQuat(FVector(0.0f, 1.0f, 0.0f), PI * 0.5f);
				}
			}
			break;
		}
	}
}

void FAnimationViewportClient::SetCameraTargetLocation(const FSphere &BoundSphere, float DeltaSeconds)
{
	FVector OldViewLoc = GetViewLocation();
	FMatrix EpicMat = FTranslationMatrix(-GetViewLocation());
	EpicMat = EpicMat * FInverseRotationMatrix(GetViewRotation());
	FMatrix CamRotMat = EpicMat.InverseFast();
	FVector CamDir = FVector(CamRotMat.M[0][0],CamRotMat.M[0][1],CamRotMat.M[0][2]);
	FVector NewViewLocation = BoundSphere.Center - BoundSphere.W * 2 * CamDir;

	NewViewLocation.X = FMath::FInterpTo(OldViewLoc.X, NewViewLocation.X, DeltaSeconds, FollowCamera_InterpSpeed);
	NewViewLocation.Y = FMath::FInterpTo(OldViewLoc.Y, NewViewLocation.Y, DeltaSeconds, FollowCamera_InterpSpeed);
	NewViewLocation.Z = FMath::FInterpTo(OldViewLoc.Z, NewViewLocation.Z, DeltaSeconds, FollowCamera_InterpSpeed_Z);

	SetViewLocation( NewViewLocation );
}

void FAnimationViewportClient::ShowBoneNames( FCanvas* Canvas, FSceneView* View )
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (PreviewMeshComponent == nullptr || PreviewMeshComponent->MeshObject == nullptr)
	{
		return;
	}

	//Most of the code taken from FASVViewportClient::Draw() in AnimSetViewerMain.cpp
	FSkeletalMeshRenderData* SkelMeshRenderData = PreviewMeshComponent->GetSkeletalMeshRenderData();
	check(SkelMeshRenderData);
	const int32 LODIndex = FMath::Clamp(PreviewMeshComponent->PredictedLODLevel, 0, SkelMeshRenderData->LODRenderData.Num()-1);
	FSkeletalMeshLODRenderData& LODData = SkelMeshRenderData->LODRenderData[ LODIndex ];

	const int32 HalfX = Viewport->GetSizeXY().X/2;
	const int32 HalfY = Viewport->GetSizeXY().Y/2;

	for (int32 i=0; i< LODData.RequiredBones.Num(); i++)
	{
		const int32 BoneIndex = LODData.RequiredBones[i];

		// If previewing a specific section, only show the bone names that belong to it
		if ((PreviewMeshComponent->GetSectionPreview() >= 0) && !LODData.RenderSections[PreviewMeshComponent->GetSectionPreview()].BoneMap.Contains(BoneIndex))
		{
			continue;
		}
		if ((PreviewMeshComponent->GetMaterialPreview() >= 0))
		{
			TArray<int32> FoundSectionIndex;
			for (int32 SectionIndex = 0; SectionIndex < LODData.RenderSections.Num(); ++SectionIndex)
			{
				if (LODData.RenderSections[SectionIndex].MaterialIndex == PreviewMeshComponent->GetMaterialPreview())
				{
					FoundSectionIndex.Add(SectionIndex);
					break;
				}
			}
			if (FoundSectionIndex.Num() > 0)
			{
				bool PreviewSectionContainBoneIndex = false;
				for (int32 SectionIndex : FoundSectionIndex)
				{
					if (LODData.RenderSections[SectionIndex].BoneMap.Contains(BoneIndex))
					{
						PreviewSectionContainBoneIndex = true;
						break;
					}
				}
				if (!PreviewSectionContainBoneIndex)
				{
					continue;
				}
			}
		}

		const FColor BoneColor = FColor::White;
		if (BoneColor.A != 0)
		{
			const FVector BonePos = PreviewMeshComponent->GetComponentTransform().TransformPosition(PreviewMeshComponent->GetComponentSpaceTransforms()[BoneIndex].GetLocation());

			const FPlane proj = View->Project(BonePos);
			if (proj.W > 0.f)
			{
				const int32 XPos = HalfX + ( HalfX * proj.X );
				const int32 YPos = HalfY + ( HalfY * (proj.Y * -1) );

				const FName BoneName = PreviewMeshComponent->SkeletalMesh->RefSkeleton.GetBoneName(BoneIndex);
				const FString BoneString = FString::Printf( TEXT("%d: %s"), BoneIndex, *BoneName.ToString() );
				FCanvasTextItem TextItem( FVector2D( XPos, YPos), FText::FromString( BoneString ), GEngine->GetSmallFont(), BoneColor );
				TextItem.EnableShadow(FLinearColor::Black);
				Canvas->DrawItem( TextItem );
			}
		}
	}
}

bool FAnimationViewportClient::ShouldDisplayAdditiveScaleErrorMessage() const
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(GetAnimPreviewScene()->GetPreviewAnimationAsset());
	if (AnimSequence)
	{
		if (AnimSequence->IsValidAdditive() && AnimSequence->RefPoseSeq)
		{
			FGuid AnimSeqGuid = AnimSequence->RefPoseSeq->GetRawDataGuid();
			if (RefPoseGuid != AnimSeqGuid)
			{
				RefPoseGuid = AnimSeqGuid;
				bDoesAdditiveRefPoseHaveZeroScale = AnimSequence->DoesSequenceContainZeroScale();
			}
			return bDoesAdditiveRefPoseHaveZeroScale;
		}
	}

	RefPoseGuid.Invalidate();
	return false;
}

static FText ConcatenateLine(const FText& InText, const FText& InNewLine)
{
	if(InText.IsEmpty())
	{
		return InNewLine;
	}

	return FText::Format(LOCTEXT("ViewportTextNewlineFormatter", "{0}\n{1}"), InText, InNewLine);
}

FText FAnimationViewportClient::GetDisplayInfo(bool bDisplayAllInfo) const
{
	FText TextValue;

	const UAssetViewerSettings* Settings = UAssetViewerSettings::Get();	
	const UEditorPerProjectUserSettings* PerProjectUserSettings = GetDefault<UEditorPerProjectUserSettings>();
	const int32 ProfileIndex = Settings->Profiles.IsValidIndex(PerProjectUserSettings->AssetViewerProfileIndex) ? PerProjectUserSettings->AssetViewerProfileIndex : 0;

	// if not valid skeletalmesh
	UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewScene()->GetPreviewMeshComponent();
	if (PreviewMeshComponent == nullptr || PreviewMeshComponent->SkeletalMesh == nullptr)
	{
		return FText();
	}

	if (ShouldDisplayAdditiveScaleErrorMessage())
	{
		TextValue = ConcatenateLine(TextValue, LOCTEXT("AdditiveRefPoseWarning", "<AnimViewport.WarningText>Additive ref pose contains scales of 0.0, this can cause additive animations to not give the desired results</>"));
	}

	if (PreviewMeshComponent->SkeletalMesh->MorphTargets.Num() > 0)
	{
		TArray<UMaterial*> ProcessedMaterials;
		TArray<UMaterial*> MaterialsThatNeedMorphFlagOn;
		TArray<UMaterial*> MaterialsThatNeedSaving;

		const TIndirectArray<FSkeletalMeshLODModel>& LODModels = PreviewMeshComponent->SkeletalMesh->GetImportedModel()->LODModels;
		int32 LodNumber = LODModels.Num();
		TArray<UMaterialInterface*> MaterialUsingMorphTarget;
		for (UMorphTarget *MorphTarget : PreviewMeshComponent->SkeletalMesh->MorphTargets)
		{
			if (MorphTarget == nullptr)
			{
				continue;
			}
			for (const FMorphTargetLODModel& MorphTargetLODModel : MorphTarget->MorphLODModels)
			{
				for (int32 SectionIndex : MorphTargetLODModel.SectionIndices)
				{
					for (int32 LodIdx = 0; LodIdx < LodNumber; LodIdx++)
					{
						const FSkeletalMeshLODModel& LODModel = LODModels[LodIdx];
						if (LODModel.Sections.IsValidIndex(SectionIndex))
						{
							MaterialUsingMorphTarget.AddUnique(PreviewMeshComponent->SkeletalMesh->Materials[LODModel.Sections[SectionIndex].MaterialIndex].MaterialInterface);
						}
					}
				}
			}
		}

		for (int i = 0; i < PreviewMeshComponent->GetNumMaterials(); ++i)
		{
			if (UMaterialInterface* MaterialInterface = PreviewMeshComponent->GetMaterial(i))
			{
				UMaterial* Material = MaterialInterface->GetMaterial();
				if ((Material != nullptr) && !ProcessedMaterials.Contains(Material))
				{
					ProcessedMaterials.Add(Material);
					if (MaterialUsingMorphTarget.Contains(MaterialInterface) && !Material->GetUsageByFlag(MATUSAGE_MorphTargets))
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
			TextValue = ConcatenateLine(TextValue, LOCTEXT("MorphSupportNeeded", "<AnimViewport.WarningText>The following materials need morph support ('Used with Morph Targets' in material editor):</>"));

			for(auto Iter = MaterialsThatNeedMorphFlagOn.CreateIterator(); Iter; ++Iter)
			{
				TextValue = ConcatenateLine(TextValue,FText::FromString((*Iter)->GetPathName()));
			}
		}

		if (MaterialsThatNeedSaving.Num() > 0)
		{
			TextValue = ConcatenateLine(TextValue, LOCTEXT("MaterialsNeedSaving", "<AnimViewport.WarningText>The following materials need saving to fully support morph targets:</>"));

			for(auto Iter = MaterialsThatNeedSaving.CreateIterator(); Iter; ++Iter)
			{
				TextValue = ConcatenateLine(TextValue, FText::FromString((*Iter)->GetPathName()));
			}
		}
	}

	UAnimPreviewInstance* PreviewInstance = PreviewMeshComponent->PreviewInstance;
	if( PreviewInstance )
	{
		// see if you have anim sequence that has transform curves
		UAnimSequence* Sequence = Cast<UAnimSequence>(PreviewInstance->GetCurrentAsset());
		if (Sequence)
		{
			if (Sequence->DoesNeedRebake())
			{
				TextValue = ConcatenateLine(TextValue, LOCTEXT("ApplyRawAnimationDataWarning", "<AnimViewport.WarningText>Animation is being edited. To apply to raw animation data, click \"Apply\"</>"));
			}

			if (Sequence->DoesNeedRecompress())
			{
				TextValue = ConcatenateLine(TextValue, LOCTEXT("ApplyToCompressedDataWarning", "<AnimViewport.WarningText>Animation is being edited. To apply to compressed data (and recalculate baked additives), click \"Apply\"</>"));
			}
		}
	}

	if (PreviewMeshComponent->IsUsingInGameBounds())
	{
		if (!PreviewMeshComponent->CheckIfBoundsAreCorrrect())
		{
			if( PreviewMeshComponent->GetPhysicsAsset() == NULL )
			{
				TextValue = ConcatenateLine(TextValue, LOCTEXT("NeedToSetupPhysicsAssetForAccurateBounds", "<AnimViewport.WarningText>You may need to setup Physics Asset to use more accurate bounds</>"));
			}
			else
			{
				TextValue = ConcatenateLine(TextValue, LOCTEXT("NeedToSetupBoundsInPhysicsAsset", "<AnimViewport.WarningText>You need to setup bounds in Physics Asset to include whole mesh</>"));
			}
		}
	}

	if (PreviewMeshComponent != NULL && PreviewMeshComponent->MeshObject != NULL)
	{
		if (bDisplayAllInfo)
		{
			FSkeletalMeshRenderData* SkelMeshResource = PreviewMeshComponent->GetSkeletalMeshRenderData();
			check(SkelMeshResource);

			// Draw stats about the mesh
			int32 NumBonesInUse;
			int32 NumBonesMappedToVerts;
			int32 NumSectionsInUse;

			const int32 LODIndex = FMath::Clamp(PreviewMeshComponent->PredictedLODLevel, 0, SkelMeshResource->LODRenderData.Num() - 1);
			FSkeletalMeshLODRenderData& LODData = SkelMeshResource->LODRenderData[LODIndex];

			NumBonesInUse = LODData.RequiredBones.Num();
			NumBonesMappedToVerts = LODData.ActiveBoneIndices.Num();
			NumSectionsInUse = LODData.RenderSections.Num();

			// Calculate polys based on non clothing sections so we don't duplicate the counts.
			uint32 NumTotalTriangles = 0;
			int32 NumSections = LODData.NumNonClothingSections();
			for(int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
			{
				NumTotalTriangles += LODData.RenderSections[SectionIndex].NumTriangles;
			}

			TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("MeshInfoFormat", "LOD: {0}, Bones: {1} (Mapped to Vertices: {2}), Polys: {3}"),
				FText::AsNumber(LODIndex),
				FText::AsNumber(NumBonesInUse),
				FText::AsNumber(NumBonesMappedToVerts),
				FText::AsNumber(NumTotalTriangles)));

			TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("ScreenSizeFOVFormat", "Current Screen Size: {0}, FOV: {1}"), FText::AsNumber(CachedScreenSize), FText::AsNumber(ViewFOV)));

			for (int32 SectionIndex = 0; SectionIndex < LODData.RenderSections.Num(); SectionIndex++)
			{
				int32 SectionVerts = LODData.RenderSections[SectionIndex].GetNumVertices();

				TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("SectionFormat", " [Section {0}] Verts: {1}, Bones: {2}"),
					FText::AsNumber(SectionIndex),
					FText::AsNumber(SectionVerts),
					FText::AsNumber(LODData.RenderSections[SectionIndex].BoneMap.Num())
					));
			}

			TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("TotalVerts", "TOTAL Verts: {0}"),
				FText::AsNumber(LODData.GetNumVertices())));

			TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("Sections", "Sections: {0}"),
				NumSectionsInUse
				));

			if (PreviewMeshComponent->BonesOfInterest.Num() > 0)
			{
				int32 BoneIndex = PreviewMeshComponent->BonesOfInterest[0];
				FTransform ReferenceTransform = PreviewMeshComponent->SkeletalMesh->RefSkeleton.GetRefBonePose()[BoneIndex];
				FTransform LocalTransform = PreviewMeshComponent->BoneSpaceTransforms[BoneIndex];
				FTransform ComponentTransform = PreviewMeshComponent->GetComponentSpaceTransforms()[BoneIndex];

				TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("LocalTransform", "Local: {0}"), FText::FromString(LocalTransform.ToHumanReadableString())));

				TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("ComponentTransform", "Component: {0}"), FText::FromString(ComponentTransform.ToHumanReadableString())));

				TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("ReferenceTransform", "Reference: {0}"), FText::FromString(ReferenceTransform.ToHumanReadableString())));
			}

			TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("ApproximateSize", "Approximate Size: {0}x{1}x{2}"),
				FText::AsNumber(FMath::RoundToInt(PreviewMeshComponent->Bounds.BoxExtent.X * 2.0f)),
				FText::AsNumber(FMath::RoundToInt(PreviewMeshComponent->Bounds.BoxExtent.Y * 2.0f)),
				FText::AsNumber(FMath::RoundToInt(PreviewMeshComponent->Bounds.BoxExtent.Z * 2.0f))));

			uint32 NumNotiesWithErrors = PreviewMeshComponent->AnimNotifyErrors.Num();
			for (uint32 i = 0; i < NumNotiesWithErrors; ++i)
			{
				uint32 NumErrors = PreviewMeshComponent->AnimNotifyErrors[i].Errors.Num();
				for (uint32 ErrorIdx = 0; ErrorIdx < NumErrors; ++ErrorIdx)
				{
					TextValue = ConcatenateLine(TextValue, FText::FromString(PreviewMeshComponent->AnimNotifyErrors[i].Errors[ErrorIdx]));
				}
			}
		}
		else // simplified default display info to be same as static mesh editor
		{
			FSkeletalMeshRenderData* SkelMeshResource = PreviewMeshComponent->GetSkeletalMeshRenderData();
			check(SkelMeshResource);

			const int32 LODIndex = FMath::Clamp(PreviewMeshComponent->PredictedLODLevel, 0, SkelMeshResource->LODRenderData.Num() - 1);
			FSkeletalMeshLODRenderData& LODData = SkelMeshResource->LODRenderData[LODIndex];

			// Current LOD 
			TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("LODFormat", "LOD: {0}"), FText::AsNumber(LODIndex)));

			// current screen size
			TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("ScreenSizeFormat", "Current Screen Size: {0}"), FText::AsNumber(CachedScreenSize)));

			// Triangles
			uint32 NumTotalTriangles = 0;
			int32 NumSections = LODData.NumNonClothingSections();
			for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
			{
				NumTotalTriangles += LODData.RenderSections[SectionIndex].NumTriangles;
			}
			TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("TrianglesFormat", "Triangles: {0}"), FText::AsNumber(NumTotalTriangles)));

			// Vertices
			TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("VerticesFormat", "Vertices: {0}"), FText::AsNumber(LODData.GetNumVertices())));

			// UV Channels
			TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("UVChannelsFormat", "UV Channels: {0}"), FText::AsNumber(LODData.GetNumTexCoords())));
			
			// Approx Size 
			TextValue = ConcatenateLine(TextValue, FText::Format(LOCTEXT("ApproxSize", "Approx Size: {0}x{1}x{2}"),
				FText::AsNumber(FMath::RoundToInt(PreviewMeshComponent->Bounds.BoxExtent.X * 2.0f)),
				FText::AsNumber(FMath::RoundToInt(PreviewMeshComponent->Bounds.BoxExtent.Y * 2.0f)),
				FText::AsNumber(FMath::RoundToInt(PreviewMeshComponent->Bounds.BoxExtent.Z * 2.0f))));
		}
	}

	if (PreviewMeshComponent->GetSectionPreview() != INDEX_NONE)
	{
		// Notify the user if they are isolating a mesh section.
		TextValue = ConcatenateLine(TextValue, LOCTEXT("MeshSectionsHiddenWarning", "Mesh Sections Hidden"));
	}
	if (PreviewMeshComponent->GetMaterialPreview() != INDEX_NONE)
	{
		// Notify the user if they are isolating a mesh section.
		TextValue = ConcatenateLine(TextValue, LOCTEXT("MeshMaterialHiddenWarning", "Mesh Materials Hidden"));
	}

	return TextValue;
}
void FAnimationViewportClient::DrawNodeDebugLines(TArray<FText>& Lines, FCanvas* Canvas, FSceneView* View)
{
	if(Lines.Num() > 0)
	{
		int32 CurrentXOffset = 5;
		int32 CurrentYOffset = 60;

		int32 CharWidth;
		int32 CharHeight;
		StringSize(GEngine->GetSmallFont(), CharWidth, CharHeight, TEXT("0"));

		const int32 LineHeight = CharHeight + 2;

		for(FText& Line : Lines)
		{
			FCanvasTextItem TextItem(FVector2D(CurrentXOffset, CurrentYOffset), Line, GEngine->GetSmallFont(), FLinearColor::White);
			TextItem.EnableShadow(FLinearColor::Black);

			Canvas->DrawItem(TextItem);

			CurrentYOffset += LineHeight;
		}
	}
}

void FAnimationViewportClient::TrackingStarted( const struct FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge )
{
	ModeTools->StartTracking(this, Viewport);
}

void FAnimationViewportClient::TrackingStopped() 
{
	ModeTools->EndTracking(this, Viewport);

	Invalidate();
}

FVector FAnimationViewportClient::GetWidgetLocation() const
{
	return ModeTools->GetWidgetLocation();
}

FMatrix FAnimationViewportClient::GetWidgetCoordSystem() const
{
	const bool bIsLocal = GetWidgetCoordSystemSpace() == COORD_Local;

	if( bIsLocal )
	{
		return ModeTools->GetCustomInputCoordinateSystem();
	}

	return FMatrix::Identity;
}

ECoordSystem FAnimationViewportClient::GetWidgetCoordSystemSpace() const
{ 
	return ModeTools->GetCoordSystem();
}

void FAnimationViewportClient::SetWidgetCoordSystemSpace(ECoordSystem NewCoordSystem)
{
	ModeTools->SetCoordSystem(NewCoordSystem);
	Invalidate();
}

void FAnimationViewportClient::SetViewMode(EViewModeIndex InViewModeIndex)
{
	FEditorViewportClient::SetViewMode(InViewModeIndex);

	ConfigOption->SetViewModeIndex(AssetEditorToolkitPtr.Pin()->GetEditorName(), InViewModeIndex, ViewportIndex);
}

void FAnimationViewportClient::SetViewportType(ELevelViewportType InViewportType)
{
	FEditorViewportClient::SetViewportType(InViewportType);
	FocusViewportOnPreviewMesh(true);

	if(CameraFollowMode != EAnimationViewportCameraFollowMode::None)
	{
		bUsingOrbitCamera = true;
	}
}

void FAnimationViewportClient::RotateViewportType()
{
	FEditorViewportClient::RotateViewportType();
	FocusViewportOnPreviewMesh(true);

	if(CameraFollowMode != EAnimationViewportCameraFollowMode::None)
	{
		bUsingOrbitCamera = true;
	}
}

bool FAnimationViewportClient::InputKey( FViewport* InViewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool bGamepad )
{
	bool bHandled = false;

	FAdvancedPreviewScene* AdvancedScene = static_cast<FAdvancedPreviewScene*>(PreviewScene);
	bHandled |= AdvancedScene->HandleInputKey(InViewport, ControllerId, Key, Event, AmountDepressed, bGamepad);

	// Pass keys to standard controls, if we didn't consume input
	return (bHandled)
		? true
		: FEditorViewportClient::InputKey(InViewport, ControllerId, Key, Event, AmountDepressed, bGamepad);
}

bool FAnimationViewportClient::InputAxis(FViewport* InViewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples /*= 1*/, bool bGamepad /*= false*/)
{
	bool bResult = true;

	if (!bDisableInput)
	{
		FAdvancedPreviewScene* AdvancedScene = (FAdvancedPreviewScene*)PreviewScene;
		bResult = AdvancedScene->HandleViewportInput(InViewport, ControllerId, Key, Delta, DeltaTime, NumSamples, bGamepad);
		if (bResult)
		{
			Invalidate();
		}
		else
		{
			bResult = FEditorViewportClient::InputAxis(InViewport, ControllerId, Key, Delta, DeltaTime, NumSamples, bGamepad);
		}
	}

	return bResult;
}

void FAnimationViewportClient::SetLocalAxesMode(ELocalAxesMode::Type AxesMode)
{
	ConfigOption->SetDefaultLocalAxesSelection( AxesMode );
}

bool FAnimationViewportClient::IsLocalAxesModeSet( ELocalAxesMode::Type AxesMode ) const
{
	return (ELocalAxesMode::Type)ConfigOption->DefaultLocalAxesSelection == AxesMode;
}

ELocalAxesMode::Type FAnimationViewportClient::GetLocalAxesMode() const
{ 
	return (ELocalAxesMode::Type)ConfigOption->DefaultLocalAxesSelection;
}

void FAnimationViewportClient::SetBoneDrawMode(EBoneDrawMode::Type AxesMode)
{
	ConfigOption->SetDefaultBoneDrawSelection(AxesMode);
}

bool FAnimationViewportClient::IsBoneDrawModeSet(EBoneDrawMode::Type AxesMode) const
{
	return (EBoneDrawMode::Type)ConfigOption->DefaultBoneDrawSelection == AxesMode;
}

EBoneDrawMode::Type FAnimationViewportClient::GetBoneDrawMode() const 
{ 
	return (EBoneDrawMode::Type)ConfigOption->DefaultBoneDrawSelection;
}

void FAnimationViewportClient::DrawBonesFromTransforms(TArray<FTransform>& Transforms, UDebugSkelMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI, FLinearColor BoneColour, FLinearColor RootBoneColour) const
{
	if ( Transforms.Num() > 0 && MeshComponent->SkeletalMesh )
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

			WorldTransforms[BoneIndex] = Transforms[BoneIndex] * MeshComponent->GetComponentTransform();
			BoneColours[BoneIndex] = (ParentIndex >= 0) ? BoneColour : RootBoneColour;
		}

		DrawBones(MeshComponent, MeshComponent->RequiredBones, WorldTransforms, PDI, BoneColours);
	}
}

void FAnimationViewportClient::DrawBonesFromCompactPose(const FCompactHeapPose& Pose, UDebugSkelMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI, const FLinearColor& DrawColour) const
{
	if (Pose.GetNumBones() > 0)
	{
		TArray<FTransform> WorldTransforms;
		WorldTransforms.AddUninitialized(Pose.GetBoneContainer().GetNumBones());

		TArray<FLinearColor> BoneColours;
		BoneColours.AddUninitialized(Pose.GetBoneContainer().GetNumBones());

		// we could cache parent bones as we calculate, but right now I'm not worried about perf issue of this
		for (FCompactPoseBoneIndex BoneIndex : Pose.ForEachBoneIndex())
		{
			FMeshPoseBoneIndex MeshBoneIndex = Pose.GetBoneContainer().MakeMeshPoseIndex(BoneIndex);

			int32 ParentIndex = Pose.GetBoneContainer().GetParentBoneIndex(MeshBoneIndex.GetInt());

			if (ParentIndex == INDEX_NONE)
			{
				WorldTransforms[MeshBoneIndex.GetInt()] = Pose[BoneIndex] * MeshComponent->GetComponentTransform();
			}
			else
			{
				WorldTransforms[MeshBoneIndex.GetInt()] = Pose[BoneIndex] * WorldTransforms[ParentIndex];
			}
			BoneColours[MeshBoneIndex.GetInt()] = DrawColour;
		}

		DrawBones(MeshComponent, MeshComponent->RequiredBones, WorldTransforms, PDI, BoneColours, 1.0f, true);
	}
}

void FAnimationViewportClient::DrawMeshBonesUncompressedAnimation(UDebugSkelMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI) const
{
	if ( MeshComponent && MeshComponent->SkeletalMesh )
	{
		DrawBonesFromTransforms(MeshComponent->UncompressedSpaceBases, MeshComponent, PDI, FColor(255, 127, 39, 255), FColor(255, 127, 39, 255));
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

void FAnimationViewportClient::DrawMeshBonesSourceRawAnimation(UDebugSkelMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI) const
{
	if(MeshComponent && MeshComponent->SkeletalMesh)
	{
		DrawBonesFromTransforms(MeshComponent->SourceAnimationPoses, MeshComponent, PDI, FColor(195, 195, 195, 255), FColor(195, 159, 195, 255));
	}
}

void FAnimationViewportClient::DrawWatchedPoses(UDebugSkelMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI)
{
	if (UAnimBlueprintGeneratedClass* AnimBlueprintGeneratedClass = Cast<UAnimBlueprintGeneratedClass>(MeshComponent->AnimClass))
	{
		if (UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(AnimBlueprintGeneratedClass->ClassGeneratedBy))
		{
			if (Blueprint->GetObjectBeingDebugged())
			{
				for (const FAnimNodePoseWatch& AnimNodePoseWatch : AnimBlueprintGeneratedClass->GetAnimBlueprintDebugData().AnimNodePoseWatch)
				{
					DrawBonesFromCompactPose(*AnimNodePoseWatch.PoseInfo.Get(), MeshComponent, PDI, AnimNodePoseWatch.PoseDrawColour);
				}
			}
		}
	}
}

void FAnimationViewportClient::DrawMeshBonesBakedAnimation(UDebugSkelMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI) const
{
	if(MeshComponent && MeshComponent->SkeletalMesh)
	{
		DrawBonesFromTransforms(MeshComponent->BakedAnimationPoses, MeshComponent, PDI, FColor(0, 128, 192, 255), FColor(0, 128, 192, 255));
	}
}

void FAnimationViewportClient::DrawMeshBones(USkeletalMeshComponent * MeshComponent, FPrimitiveDrawInterface* PDI) const
{
	if ( MeshComponent && MeshComponent->SkeletalMesh )
	{
		TArray<FTransform> WorldTransforms;
		WorldTransforms.AddUninitialized(MeshComponent->GetNumComponentSpaceTransforms());

		TArray<FLinearColor> BoneColours;
		BoneColours.AddUninitialized(MeshComponent->GetNumComponentSpaceTransforms());

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

			WorldTransforms[BoneIndex] = MeshComponent->GetComponentSpaceTransforms()[BoneIndex] * MeshComponent->GetComponentTransform();
			
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

void FAnimationViewportClient::DrawBones(const USkeletalMeshComponent* MeshComponent, const TArray<FBoneIndexType> & RequiredBones, const TArray<FTransform> & WorldTransforms, FPrimitiveDrawInterface* PDI, const TArray<FLinearColor>& BoneColours, float LineThickness/*=0.f*/, bool bForceDraw/*=false*/) const
{
	check ( MeshComponent && MeshComponent->SkeletalMesh );

	TArray<int32> SelectedBones;
	if(const UDebugSkelMeshComponent* DebugMeshComponent = Cast<const UDebugSkelMeshComponent>(MeshComponent))
	{
		SelectedBones = DebugMeshComponent->BonesOfInterest;

		if(GetBoneDrawMode() == EBoneDrawMode::SelectedAndParents)
		{
			int32 BoneIndex = GetAnimPreviewScene()->GetSelectedBoneIndex();
			while (BoneIndex != INDEX_NONE)
			{
				int32 ParentIndex = DebugMeshComponent->SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
				if (ParentIndex != INDEX_NONE)
				{
					SelectedBones.AddUnique(ParentIndex);
				}
				BoneIndex = ParentIndex;
			}
		}

		// we could cache parent bones as we calculate, but right now I'm not worried about perf issue of this
		for ( int32 Index=0; Index<RequiredBones.Num(); ++Index )
		{
			const int32 BoneIndex = RequiredBones[Index];

			if (bForceDraw ||
				(GetBoneDrawMode() == EBoneDrawMode::All) ||
				((GetBoneDrawMode() == EBoneDrawMode::Selected || GetBoneDrawMode() == EBoneDrawMode::SelectedAndParents) && SelectedBones.Contains(BoneIndex) )
				)
			{
				const int32 ParentIndex = MeshComponent->SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
				FVector Start, End;
				FLinearColor LineColor = BoneColours[BoneIndex];

				if (ParentIndex >= 0)
				{
					Start = WorldTransforms[ParentIndex].GetLocation();
					End = WorldTransforms[BoneIndex].GetLocation();
				}
				else
				{
					Start = FVector::ZeroVector;
					End = WorldTransforms[BoneIndex].GetLocation();
				}

				//Render Sphere for bone end point and a cone between it and its parent.
				PDI->SetHitProxy(new HPersonaBoneProxy(MeshComponent->SkeletalMesh->RefSkeleton.GetBoneName(BoneIndex)));
				SkeletalDebugRendering::DrawWireBone(PDI, Start, End, LineColor, SDPG_Foreground);
				PDI->SetHitProxy(NULL);

				// draw gizmo
				if ((GetLocalAxesMode() == ELocalAxesMode::All) ||
					((GetLocalAxesMode() == ELocalAxesMode::Selected) && SelectedBones.Contains(BoneIndex))
					)
				{
					SkeletalDebugRendering::DrawAxes(PDI, WorldTransforms[BoneIndex], SDPG_Foreground);
				}
			}
		}
	}
}

void FAnimationViewportClient::DrawMeshSubsetBones(const USkeletalMeshComponent* MeshComponent, const TArray<int32>& BonesOfInterest, FPrimitiveDrawInterface* PDI) const
{
	// this BonesOfInterest has to be in MeshComponent base, not Skeleton 
	if ( MeshComponent && MeshComponent->SkeletalMesh && BonesOfInterest.Num() > 0 )
	{
		TArray<FTransform> WorldTransforms;
		WorldTransforms.AddUninitialized(MeshComponent->GetNumComponentSpaceTransforms());

		TArray<FLinearColor> BoneColours;
		BoneColours.AddUninitialized(MeshComponent->GetNumComponentSpaceTransforms());

		TArray<FBoneIndexType> RequiredBones;

		const FReferenceSkeleton& RefSkeleton = MeshComponent->SkeletalMesh->RefSkeleton;

		static const FName SelectionColorName("SelectionColor");

		const FSlateColor SelectionColor = FEditorStyle::GetSlateColor(SelectionColorName);
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
						WorldTransforms[ParentIndex] = MeshComponent->GetComponentSpaceTransforms()[ParentIndex]*MeshComponent->GetComponentTransform();
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
				WorldTransforms[BoneIndex] = MeshComponent->GetComponentSpaceTransforms()[BoneIndex] * MeshComponent->GetComponentTransform();
			}
		}

		DrawBones(MeshComponent, RequiredBones, WorldTransforms, PDI, BoneColours, 0.3f);
	}
}

void FAnimationViewportClient::DrawSockets(const UDebugSkelMeshComponent* InPreviewMeshComponent, TArray<USkeletalMeshSocket*>& InSockets, FSelectedSocketInfo InSelectedSocket, FPrimitiveDrawInterface* PDI, bool bUseSkeletonSocketColor)
{
	if (InPreviewMeshComponent && InPreviewMeshComponent->SkeletalMesh)
	{
		ELocalAxesMode::Type LocalAxesMode = (ELocalAxesMode::Type)GetDefault<UPersonaOptions>()->DefaultLocalAxesSelection;

		for ( auto SocketIt = InSockets.CreateConstIterator(); SocketIt; ++SocketIt )
		{
			USkeletalMeshSocket* Socket = *(SocketIt);
			FReferenceSkeleton& RefSkeleton = InPreviewMeshComponent->SkeletalMesh->RefSkeleton;

			const int32 ParentIndex = RefSkeleton.FindBoneIndex(Socket->BoneName);

			FTransform WorldTransformSocket = Socket->GetSocketTransform(InPreviewMeshComponent);

			FLinearColor SocketColor;
			FVector Start, End;
			if (ParentIndex >=0)
			{
				FTransform WorldTransformParent = InPreviewMeshComponent->GetComponentSpaceTransforms()[ParentIndex] * InPreviewMeshComponent->GetComponentTransform();
				Start = WorldTransformParent.GetLocation();
				End = WorldTransformSocket.GetLocation();
			}
			else
			{
				Start = FVector::ZeroVector;
				End = WorldTransformSocket.GetLocation();
			}

			const bool bSelectedSocket = (InSelectedSocket.Socket == Socket);

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
				Socket->GetSocketMatrix( SocketMatrix, InPreviewMeshComponent);

				PDI->SetHitProxy( new HPersonaSocketProxy( FSelectedSocketInfo( Socket, bUseSkeletonSocketColor ) ) );
				DrawWireDiamond( PDI, SocketMatrix, 2.f, SocketColor, SDPG_Foreground );
				PDI->SetHitProxy( NULL );
				
				SkeletalDebugRendering::DrawAxes(PDI, FTransform(SocketMatrix), SDPG_Foreground);
			}
		}
	}
}

FSphere FAnimationViewportClient::GetCameraTarget()
{
	const FSphere DefaultSphere(FVector(0,0,0), 100.0f);

	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if( !PreviewMeshComponent )
	{
		return DefaultSphere;
	}

	PreviewMeshComponent->CalcBounds(PreviewMeshComponent->GetComponentTransform());

	// give the editor mode a chance to give us a camera target
	if (GetPersonaModeManager())
	{
		FSphere Target;
		if (GetPersonaModeManager()->GetCameraTarget(Target))
		{
			return Target;
		}
	}

	FBoxSphereBounds Bounds = PreviewMeshComponent->CalcBounds(FTransform::Identity);
	return Bounds.GetSphere();
}

void FAnimationViewportClient::UpdateCameraSetup()
{
	static FRotator CustomOrbitRotation(-33.75, -135, 0);
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (PreviewMeshComponent != nullptr && PreviewMeshComponent->SkeletalMesh)
	{
		FSphere BoundSphere = GetCameraTarget();
		FVector CustomOrbitZoom(0, BoundSphere.W / (75.0f * (float)PI / 360.0f), 0);
		FVector CustomOrbitLookAt = BoundSphere.Center;

		SetCameraSetup(CustomOrbitLookAt, CustomOrbitRotation, CustomOrbitZoom, CustomOrbitLookAt, GetViewLocation(), GetViewRotation() );

		// Move the floor to the bottom of the bounding box of the mesh, rather than on the origin
		FVector Bottom = PreviewMeshComponent->Bounds.GetBoxExtrema(0);

		FVector FloorPos(0.f, 0.f, GetFloorOffset());
		if (bAutoAlignFloor)
		{
			FloorPos.Z += Bottom.Z;
		}
		GetAnimPreviewScene()->SetFloorLocation(FloorPos);
	}
}

void FAnimationViewportClient::FocusViewportOnSphere( FSphere& Sphere, bool bInstant /*= true*/ )
{
	FBox Box( Sphere.Center - FVector(Sphere.W, 0.0f, 0.0f), Sphere.Center + FVector(Sphere.W, 0.0f, 0.0f) );

	FocusViewportOnBox( Box, bInstant );

	Invalidate();
}

void FAnimationViewportClient::TransformVertexPositionsToWorld(TArray<FFinalSkinVertex>& LocalVertices) const
{
	const UDebugSkelMeshComponent* const PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if ( !PreviewMeshComponent )
	{
		return;
	}

	const FTransform& LocalToWorldTransform = PreviewMeshComponent->GetComponentTransform();

	for ( int32 VertexIndex = 0; VertexIndex < LocalVertices.Num(); ++VertexIndex )
	{
		FVector& VertexPosition = LocalVertices[VertexIndex].Position;
		VertexPosition = LocalToWorldTransform.TransformPosition(VertexPosition);
	}
}

void FAnimationViewportClient::GetAllVertexIndicesUsedInSection(const FRawStaticIndexBuffer16or32Interface& IndexBuffer,
																const FSkelMeshRenderSection& SkelMeshSection,
																TArray<int32>& OutIndices) const
{

	const uint32 BaseIndex = SkelMeshSection.BaseIndex;
	const int32 NumWedges = SkelMeshSection.NumTriangles * 3;

	for (int32 WedgeIndex = 0; WedgeIndex < NumWedges; ++WedgeIndex)
	{
		const int32 VertexIndexForWedge = IndexBuffer.Get(SkelMeshSection.BaseIndex + WedgeIndex);
		OutIndices.Add(VertexIndexForWedge);
	}
}

bool FAnimationViewportClient::PreviewComponentSelectionOverride(const UPrimitiveComponent* InComponent) const
{
	if (InComponent == GetPreviewScene()->GetPreviewMeshComponent())
	{
		const USkeletalMeshComponent* Component = CastChecked<USkeletalMeshComponent>(InComponent);
		return (Component->GetSelectedEditorSection() != INDEX_NONE || Component->GetSelectedEditorMaterial() != INDEX_NONE);
	}

	return false;
}

FBox FAnimationViewportClient::ComputeBoundingBoxForSelectedEditorSection() const
{
	UDebugSkelMeshComponent* const PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if ( !PreviewMeshComponent )
	{
		return FBox(ForceInitToZero);
	}

	USkeletalMesh* const SkeletalMesh = PreviewMeshComponent->SkeletalMesh;
	FSkeletalMeshObject* const MeshObject = PreviewMeshComponent->MeshObject;
	if ( !SkeletalMesh || !MeshObject )
	{
		return FBox(ForceInitToZero);
	}

	const int32 LODLevel = PreviewMeshComponent->PredictedLODLevel;
	const int32 SelectedEditorSection = PreviewMeshComponent->GetSelectedEditorSection();
	const FSkeletalMeshRenderData& SkelMeshRenderData = MeshObject->GetSkeletalMeshRenderData();

	const FSkeletalMeshLODRenderData& LODData = SkelMeshRenderData.LODRenderData[LODLevel];
	const FSkelMeshRenderSection& SelectedSectionSkelMesh = LODData.RenderSections[SelectedEditorSection];

	// Get us vertices from the entire LOD model.
	TArray<FFinalSkinVertex> SkinnedVertices;
	PreviewMeshComponent->GetCPUSkinnedVertices(SkinnedVertices, LODLevel);
	TransformVertexPositionsToWorld(SkinnedVertices);

	// Find out which of these the selected section actually uses.
	TArray<int32> VertexIndices;
	GetAllVertexIndicesUsedInSection(*LODData.MultiSizeIndexContainer.GetIndexBuffer(), SelectedSectionSkelMesh, VertexIndices);

	// Get their bounds.
	FBox BoundingBox(ForceInitToZero);
	for ( int32 Index = 0; Index < VertexIndices.Num(); ++Index )
	{
		const int32 VertexIndex = VertexIndices[Index];
		BoundingBox += SkinnedVertices[VertexIndex].Position;
	}

	return BoundingBox;
}

void FAnimationViewportClient::FocusViewportOnPreviewMesh(bool bUseCustomCamera)
{
	FIntPoint ViewportSize(FIntPoint::ZeroValue);
	if (Viewport != nullptr)
	{
		ViewportSize = Viewport->GetSizeXY();
	}

	if(!(ViewportSize.SizeSquared() > 0))
	{
		// We cannot focus fully right now as the viewport does not know its size
		// and we must have the aspect to correctly focus on the component,
		bFocusOnDraw = true;
		bFocusUsingCustomCamera = bUseCustomCamera;
		return;
	}

	UDebugSkelMeshComponent* const PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if ( !PreviewMeshComponent )
	{
		return;
	}

	USkeletalMesh* const SkelMesh = PreviewMeshComponent->SkeletalMesh;
	if (!SkelMesh)
	{
		return;
	}

	if (bUseCustomCamera && SkelMesh->bHasCustomDefaultEditorCamera)
	{
		FViewportCameraTransform& ViewTransform = GetViewTransform();

		ViewTransform.SetLocation(SkelMesh->DefaultEditorCameraLocation);
		ViewTransform.SetRotation(SkelMesh->DefaultEditorCameraRotation);
		ViewTransform.SetLookAt(SkelMesh->DefaultEditorCameraLookAt);
		ViewTransform.SetOrthoZoom(SkelMesh->DefaultEditorCameraOrthoZoom);

		Invalidate();
		return;
	}

	if (PreviewMeshComponent->GetSelectedEditorSection() != INDEX_NONE )
	{
		const FBox SelectedSectionBounds = ComputeBoundingBoxForSelectedEditorSection();
		
		if ( SelectedSectionBounds.IsValid )
		{
			FocusViewportOnBox(SelectedSectionBounds);
		}

		return;
	}

	FSphere Sphere = GetCameraTarget();
	FocusViewportOnSphere(Sphere);
}

float FAnimationViewportClient::GetFloorOffset() const
{
	USkeletalMesh* Mesh = GetPreviewScene()->GetPreviewMeshComponent()->SkeletalMesh;
	if ( Mesh )
	{
		return Mesh->FloorOffset;
	}

	return 0.0f;
}

void FAnimationViewportClient::SetFloorOffset( float NewValue )
{
	USkeletalMesh* Mesh = GetPreviewScene()->GetPreviewMeshComponent()->SkeletalMesh;

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

void FAnimationViewportClient::ToggleCPUSkinning()
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (PreviewMeshComponent != nullptr)
	{
		PreviewMeshComponent->bCPUSkinning = !PreviewMeshComponent->bCPUSkinning;
		PreviewMeshComponent->MarkRenderStateDirty();
		Invalidate();
	}
}

bool FAnimationViewportClient::IsSetCPUSkinningChecked() const
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (PreviewMeshComponent != nullptr)
	{
		return PreviewMeshComponent->bCPUSkinning;
	}
	return false;
}

void FAnimationViewportClient::ToggleShowNormals()
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if(PreviewMeshComponent != nullptr)
	{
		PreviewMeshComponent->bDrawNormals = !PreviewMeshComponent->bDrawNormals;
		PreviewMeshComponent->MarkRenderStateDirty();
		Invalidate();
	}
}

bool FAnimationViewportClient::IsSetShowNormalsChecked() const
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (PreviewMeshComponent != nullptr)
	{
		return PreviewMeshComponent->bDrawNormals;
	}
	return false;
}

void FAnimationViewportClient::ToggleShowTangents()
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if(PreviewMeshComponent != nullptr)
	{
		PreviewMeshComponent->bDrawTangents = !PreviewMeshComponent->bDrawTangents;
		PreviewMeshComponent->MarkRenderStateDirty();
		Invalidate();
	}
}

bool FAnimationViewportClient::IsSetShowTangentsChecked() const
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (PreviewMeshComponent != nullptr)
	{
		return PreviewMeshComponent->bDrawTangents;
	}
	return false;
}

void FAnimationViewportClient::ToggleShowBinormals()
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if(PreviewMeshComponent != nullptr)
	{
		PreviewMeshComponent->bDrawBinormals = !PreviewMeshComponent->bDrawBinormals;
		PreviewMeshComponent->MarkRenderStateDirty();
		Invalidate();
	}
}

bool FAnimationViewportClient::IsSetShowBinormalsChecked() const
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (PreviewMeshComponent != nullptr)
	{
		return PreviewMeshComponent->bDrawBinormals;
	}
	return false;
}

void FAnimationViewportClient::SetDrawUVOverlay(bool bInDrawUVs)
{
	bDrawUVs = bInDrawUVs;
	Invalidate();
}

bool FAnimationViewportClient::IsSetDrawUVOverlayChecked() const
{
	return bDrawUVs;
}

void FAnimationViewportClient::OnSetShowMeshStats(int32 ShowMode)
{
	ConfigOption->SetShowMeshStats(ShowMode);
}

bool FAnimationViewportClient::IsShowingMeshStats() const
{
	const bool bShouldBeEnabled = ConfigOption->ShowMeshStats != EDisplayInfoMode::None;

	return bShouldBeEnabled && bShowMeshStats;
}

bool FAnimationViewportClient::IsShowingSelectedNodeStats() const
{
	return ConfigOption->ShowMeshStats == EDisplayInfoMode::SkeletalControls;
}

bool FAnimationViewportClient::IsDetailedMeshStats() const
{
	return ConfigOption->ShowMeshStats == EDisplayInfoMode::Detailed;
}

int32 FAnimationViewportClient::GetShowMeshStats() const
{
	return ConfigOption->ShowMeshStats;
}

void FAnimationViewportClient::OnAssetViewerSettingsChanged(const FName& InPropertyName)
{
	if (InPropertyName == GET_MEMBER_NAME_CHECKED(FPreviewSceneProfile, bPostProcessingEnabled) || InPropertyName == NAME_None)
	{
		UAssetViewerSettings* Settings = UAssetViewerSettings::Get();
		const int32 ProfileIndex = GetPreviewScene()->GetCurrentProfileIndex();
		if (Settings->Profiles.IsValidIndex(ProfileIndex))
		{			
			SetAdvancedShowFlagsForScene(Settings->Profiles[ProfileIndex].bPostProcessingEnabled);
		}
	}
}

void FAnimationViewportClient::SetAdvancedShowFlagsForScene(const bool bAdvancedShowFlags)
{	
	if (bAdvancedShowFlags)
	{
		EngineShowFlags.EnableAdvancedFeatures();
	}
	else
	{
		EngineShowFlags.DisableAdvancedFeatures();
	}
}

void FAnimationViewportClient::SetPlaybackSpeedMode(EAnimationPlaybackSpeeds::Type InMode)
{
	AnimationPlaybackSpeedMode = InMode;

	if (GetWorld())
	{
		GetWorld()->GetWorldSettings()->TimeDilation = EAnimationPlaybackSpeeds::Values[AnimationPlaybackSpeedMode];
	}
}

EAnimationPlaybackSpeeds::Type FAnimationViewportClient::GetPlaybackSpeedMode() const
{
	return AnimationPlaybackSpeedMode;
}

TSharedRef<FAnimationEditorPreviewScene> FAnimationViewportClient::GetAnimPreviewScene() const
{
	return StaticCastSharedRef<FAnimationEditorPreviewScene>(GetPreviewScene());
}

IPersonaEditorModeManager* FAnimationViewportClient::GetPersonaModeManager() const
{
	return static_cast<IPersonaEditorModeManager*>(ModeTools);
}

void FAnimationViewportClient::HandleInvalidateViews()
{
	Invalidate();
}

void FAnimationViewportClient::HandleFocusViews()
{
	SetCameraFollowMode(EAnimationViewportCameraFollowMode::None);
	FocusViewportOnPreviewMesh(false);
}

bool FAnimationViewportClient::CanCycleWidgetMode() const
{
	return ModeTools ? ModeTools->CanCycleWidgetMode() : false;
}

void FAnimationViewportClient::UpdateAudioListener(const FSceneView& View)
{
	UWorld* ViewportWorld = GetWorld();

	if (ViewportWorld)
	{
		if (FAudioDevice* AudioDevice = ViewportWorld->GetAudioDevice())
		{
			const FVector& ViewLocation = GetViewLocation();
			const FRotator& ViewRotation = GetViewRotation();

			FTransform ListenerTransform(ViewRotation);
			ListenerTransform.SetLocation(ViewLocation);

			AudioDevice->SetListener(ViewportWorld, 0, ListenerTransform, 0.0f);
		}
	}
}

void FAnimationViewportClient::SetupViewForRendering( FSceneViewFamily& ViewFamily, FSceneView& View )
{
	FEditorViewportClient::SetupViewForRendering( ViewFamily, View );

	if(bHasAudioFocus)
	{
		UpdateAudioListener(View);
	}

	// Cache screen size
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene()->GetPreviewMeshComponent();
	if (PreviewMeshComponent != NULL && PreviewMeshComponent->MeshObject != NULL)
	{
		const FBoxSphereBounds& SkelBounds = PreviewMeshComponent->Bounds;
		CachedScreenSize = ComputeBoundsScreenSize(SkelBounds.Origin, SkelBounds.SphereRadius, View);
	}
}

void FAnimationViewportClient::HandleToggleShowFlag(FEngineShowFlags::EShowFlag EngineShowFlagIndex)
{
	FEditorViewportClient::HandleToggleShowFlag(EngineShowFlagIndex);

	ConfigOption->SetShowGrid(EngineShowFlags.Grid);
}

void FAnimationViewportClient::OnCameraControllerChanged()
{
	TSharedPtr<FEditorCameraController> Override = GetAnimPreviewScene()->GetCurrentCameraOverride();
	CameraController = Override.IsValid() ? Override.Get() : CachedDefaultCameraController;
}

FMatrix FAnimationViewportClient::CalcViewRotationMatrix(const FRotator& InViewRotation) const
{
	auto ComputeOrbitMatrix = [this](const FViewportCameraTransform& InViewTransform)
	{
		FTransform Transform =
		FTransform( -InViewTransform.GetLookAt() ) * 
		FTransform( OrbitRotation.Inverse() ) *
		FTransform( FRotator(0, InViewTransform.GetRotation().Yaw,0) ) * 
		FTransform( FRotator(0, 0, InViewTransform.GetRotation().Pitch) ) *
		FTransform( FVector(0, (InViewTransform.GetLocation() - InViewTransform.GetLookAt()).Size(), 0) );

		return Transform.ToMatrixNoScale() * FInverseRotationMatrix( FRotator(0,90.f,0) );
	};

	const FViewportCameraTransform& ViewTransform = GetViewTransform();

	if (bUsingOrbitCamera)
	{
		// @todo vreditor: Not stereo friendly yet
		return FTranslationMatrix(ViewTransform.GetLocation()) * ComputeOrbitMatrix(ViewTransform);
	}
	else
	{
		// Create the view matrix
		return FInverseRotationMatrix(InViewRotation);
	}
}

#undef LOCTEXT_NAMESPACE

