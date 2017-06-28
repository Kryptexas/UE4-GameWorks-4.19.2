// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MixedRealityCaptureComponent.h"
#include "MotionControllerComponent.h"
#include "MixedRealityBillboard.h"
#include "Engine/TextureRenderTarget2D.h"
#include "MediaPlayer.h"
#include "IMediaPlayer.h"
#include "IMediaTracks.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Pawn.h" // for GetWorld()
#include "Engine/World.h" // for GetPlayerControllerIterator()
#include "GameFramework/PlayerController.h" // for GetPawn()

#if WITH_EDITORONLY_DATA
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#endif

//------------------------------------------------------------------------------
UMixedRealityCaptureComponent::UMixedRealityCaptureComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ChromaColor(0.122f, 0.765f, 0.261, 1.f)
	, bAutoTracking(false)
	, TrackingDevice(EControllerHand::Special_1)
{
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinder<UMediaPlayer> DefaultMediaSource;
		ConstructorHelpers::FObjectFinder<UMaterial>    DefaultVideoProcessingMaterial;
#if WITH_EDITORONLY_DATA
		ConstructorHelpers::FObjectFinder<UStaticMesh>  EditorCameraMesh;
#endif

		FConstructorStatics()
			: DefaultMediaSource(TEXT("/MixedRealityFramework/MRVideoSource"))
			, DefaultVideoProcessingMaterial(TEXT("/MixedRealityFramework/M_MRVidProcessing"))
#if WITH_EDITORONLY_DATA
			, EditorCameraMesh(TEXT("/Engine/EditorMeshes/MatineeCam_SM"))
#endif
		{}
	};
	static FConstructorStatics ConstructorStatics;

	MediaSource = ConstructorStatics.DefaultMediaSource.Object;
	VideoProcessingMaterial = ConstructorStatics.DefaultVideoProcessingMaterial.Object;

#if WITH_EDITORONLY_DATA
	if (!IsRunningCommandlet())
	{
		ProxyMesh = ConstructorStatics.EditorCameraMesh.Object;
	}
#endif

	// ensure InitializeComponent() gets called
	bWantsInitializeComponent = true;
}

//------------------------------------------------------------------------------
void UMixedRealityCaptureComponent::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
#if WITH_EDITORONLY_DATA
	UMixedRealityCaptureComponent* This = CastChecked<UMixedRealityCaptureComponent>(InThis);
	Collector.AddReferencedObject(This->ProxyMeshComponent);
#endif

	Super::AddReferencedObjects(InThis, Collector);
}
 
//------------------------------------------------------------------------------
void UMixedRealityCaptureComponent::OnRegister()
{
	USceneComponent* Parent = GetAttachParent();
	UMotionControllerComponent* TrackingComponent = Cast<UMotionControllerComponent>(Parent);

	AActor* MyOwner = GetOwner();
#if WITH_EDITOR
	UWorld* MyWOrld = MyOwner->GetWorld();
	const bool bIsEditorInst = MyWOrld && (MyWOrld->WorldType == EWorldType::Editor || MyWOrld->WorldType == EWorldType::EditorPreview);

	if (!bIsEditorInst)
#endif
	{
		if (bAutoTracking && (TrackingComponent == nullptr || TrackingComponent->Hand != TrackingDevice))
		{
			TrackingComponent = NewObject<UMotionControllerComponent>(this, TEXT("MixedRealityMotionController"), RF_Transient | RF_TextExportTransient);
			TrackingComponent->Hand = TrackingDevice;
			if (Parent)
			{
				TrackingComponent->SetupAttachment(Parent, GetAttachSocketName());
			}
			else
			{
				MyOwner->SetRootComponent(TrackingComponent);
			}

			FAttachmentTransformRules ReattachRules(EAttachmentRule::KeepRelative, /*bWeldSimulatedBodies =*/false);
			AttachToComponent(TrackingComponent, ReattachRules);

			TrackingComponent->RegisterComponent();
		}
	}

	Super::OnRegister();

	if (VideoProcessingMaterial)
	{
		//UMaterialInterface*
		//UMaterialInstanceDynamic* BillboardMaterial = UMaterialInstanceDynamic::Create(VideoProcessingMaterial);

		VideoProjectionComponent = NewObject<UMixedRealityBillboard>(this, TEXT("ProjectionPlane"), RF_Transient | RF_TextExportTransient);
		VideoProjectionComponent->AddElement(VideoProcessingMaterial, /*DistanceToOpacityCurve =*/nullptr, /*bSizeIsInScreenSpace =*/true, /*BaseSizeX =*/1.0f, /*BaseSizeY=*/GetDesiredAspectRatio(), /*DistanceToSizeCurve =*/nullptr);
		//VideoProjectionComponent->SetTranslucentSortPriority();
		VideoProjectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		VideoProjectionComponent->CastShadow = false;
		VideoProjectionComponent->SetupAttachment(this);
		// arbitrary distance (somewhere beyond the near plane so the scene cap sees it)
		VideoProjectionComponent->SetRelativeLocation(FVector(10.f, 0.f, 0.f));

#if WITH_EDITOR
		// UMixedRealityBillboard is already hidden from all editor views (see UMixedRealityBillboard::GetHiddenEditorViews)
		// in editor, we want it displaying in the preview viewport, so for that we don't hide it from anyone but his owner
		VideoProjectionComponent->bOnlyOwnerSee = !bIsEditorInst;
#else 
		VideoProjectionComponent->bOnlyOwnerSee = true;
#endif 
		VideoProjectionComponent->RegisterComponent();
	}
	

#if WITH_EDITORONLY_DATA
	if (MyOwner)
	{
		if (ProxyMeshComponent == nullptr)
		{
			ProxyMeshComponent = NewObject<UStaticMeshComponent>(MyOwner, NAME_None, RF_Transactional | RF_TextExportTransient);
			ProxyMeshComponent->SetupAttachment(this);
			ProxyMeshComponent->bIsEditorOnly = true;
			ProxyMeshComponent->SetStaticMesh(ProxyMesh);
			ProxyMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
			ProxyMeshComponent->bHiddenInGame = true;
			ProxyMeshComponent->CastShadow = false;
			ProxyMeshComponent->PostPhysicsComponentTick.bCanEverTick = false;
			ProxyMeshComponent->CreationMethod = CreationMethod;
			ProxyMeshComponent->RegisterComponent();
		}
	}

	if (ProxyMeshComponent)
	{
		ProxyMeshComponent->ResetRelativeTransform();
	}
#endif

	


	AttachMediaListeners();
}

//------------------------------------------------------------------------------
void UMixedRealityCaptureComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (MediaSource)
	{
		AttachMediaListeners();
		MediaSource->OpenUrl(TEXT("vidcap://"));
	}
}

//------------------------------------------------------------------------------
void UMixedRealityCaptureComponent::BeginPlay()
{
	// has to happen after the player pawn is spawned/registered (this is not reliable in InitializeComponent)
	APlayerController* TargetPlayer = FindTargetPlayer();
	if (TargetPlayer && VideoProjectionComponent)
	{
		VideoProjectionComponent->SetTargetPawn(TargetPlayer->GetPawn());
		TargetPlayer->HiddenPrimitiveComponents.AddUnique(VideoProjectionComponent);
	}

	Super::BeginPlay();
}

//------------------------------------------------------------------------------
void UMixedRealityCaptureComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
#if WITH_EDITORONLY_DATA
	if (ProxyMeshComponent)
	{
		const FTransform WorldXform = GetComponentToWorld();
		ProxyMeshComponent->SetWorldTransform(WorldXform);
	}
#endif

	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
}

//------------------------------------------------------------------------------
void UMixedRealityCaptureComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	DetachMediaListeners();

#if WITH_EDITORONLY_DATA
	if (ProxyMeshComponent)
	{
		ProxyMeshComponent->DestroyComponent();
	}
#endif 

	if (VideoProjectionComponent)
	{
		VideoProjectionComponent->DestroyComponent();
	}

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

//------------------------------------------------------------------------------
#if WITH_EDITOR
void UMixedRealityCaptureComponent::PreEditChange(UProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);

	const FName PropertyName = (PropertyThatWillChange != nullptr)
		? PropertyThatWillChange->GetFName()
		: NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UMixedRealityCaptureComponent, MediaSource))
	{
		DetachMediaListeners();
	}
}
#endif	// WITH_EDITOR

//------------------------------------------------------------------------------
#if WITH_EDITOR
void UMixedRealityCaptureComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UMixedRealityCaptureComponent, MediaSource))
	{
		AttachMediaListeners();
	}
}
#endif	// WITH_EDITOR

//------------------------------------------------------------------------------
#if WITH_EDITOR
bool UMixedRealityCaptureComponent::GetEditorPreviewInfo(float /*DeltaTime*/, FMinimalViewInfo& ViewOut)
{
	ViewOut.Location = GetComponentLocation();
	ViewOut.Rotation = GetComponentRotation();

	ViewOut.FOV = FOVAngle;

	ViewOut.AspectRatio = GetDesiredAspectRatio();
	ViewOut.bConstrainAspectRatio = true;

	// see default in FSceneViewInitOptions
	ViewOut.bUseFieldOfViewForLOD = true;
	
 	ViewOut.ProjectionMode = ProjectionType;
	ViewOut.OrthoWidth = OrthoWidth;

	// see BuildProjectionMatrix() in SceneCaptureRendering.cpp
	ViewOut.OrthoNearClipPlane = 0.0f;
	ViewOut.OrthoFarClipPlane  = WORLD_MAX / 8.0f;;

	ViewOut.PostProcessBlendWeight = PostProcessBlendWeight;
	if (PostProcessBlendWeight > 0.0f)
	{
		ViewOut.PostProcessSettings = PostProcessSettings;
	}

	return true;
}
#endif	// WITH_EDITOR

//------------------------------------------------------------------------------
float UMixedRealityCaptureComponent::GetDesiredAspectRatio() const
{
	float DesiredAspectRatio = 0.0f;
	if (MediaSource)
	{
		TSharedPtr<IMediaPlayer> MediaPlayer = MediaSource->GetBasePlayer().GetNativePlayer();
		if (MediaPlayer.IsValid())
		{
			IMediaTracks& Tracks = MediaPlayer->GetTracks();
			DesiredAspectRatio = Tracks.GetVideoTrackAspectRatio(0);
		}
	}

	if (DesiredAspectRatio == 0.0f)
	{
		if (TextureTarget)
		{
			DesiredAspectRatio = TextureTarget->GetSurfaceWidth() / TextureTarget->GetSurfaceHeight();
		}
		else
		{
			DesiredAspectRatio = 16.f / 9.f;
		}
	}
	return DesiredAspectRatio;
}

//------------------------------------------------------------------------------
void UMixedRealityCaptureComponent::AttachMediaListeners() const
{
	if (MediaSource)
	{
		MediaSource->OnMediaOpened.AddUniqueDynamic(this, &UMixedRealityCaptureComponent::OnVideoFeedOpened);
	}
}

//------------------------------------------------------------------------------
void UMixedRealityCaptureComponent::DetachMediaListeners() const
{
	if (MediaSource)
	{
		MediaSource->OnMediaOpened.RemoveDynamic(this, &UMixedRealityCaptureComponent::OnVideoFeedOpened);
	}
}

//------------------------------------------------------------------------------
void UMixedRealityCaptureComponent::OnVideoFeedOpened(FString MediaUrl)
{
	RefreshProjectionDimensions();
}

//------------------------------------------------------------------------------
void UMixedRealityCaptureComponent::RefreshProjectionDimensions()
{
	if (VideoProjectionComponent)
	{
		const float NewAspectRatio = GetDesiredAspectRatio();

		bool bDirtiedRenderState = false;
		for (FMaterialSpriteElement& VidMat : VideoProjectionComponent->Elements)
		{
			if (ensure(VidMat.bSizeIsInScreenSpace && VidMat.BaseSizeX == 1.0f) && NewAspectRatio != VidMat.BaseSizeY)
			{
				bDirtiedRenderState = true;
				VidMat.BaseSizeY = NewAspectRatio;
			}
		}	

		if (bDirtiedRenderState)
		{
			VideoProjectionComponent->MarkRenderStateDirty();
		}
	}
}

//------------------------------------------------------------------------------
APlayerController* UMixedRealityCaptureComponent::FindTargetPlayer() const
{
	APlayerController* TargetPlayer = nullptr;
	if (AActor* MyOwner = GetOwner())
	{
		UWorld* MyWorld = MyOwner->GetWorld();

		APlayerController* AutoTargetPlayer = nullptr;
		const int32 AutoTargetIndex = (AutoAttach == EAutoReceiveInput::Disabled) ? INDEX_NONE : int32(AutoAttach.GetValue()) - 1;

		int32 PlayerIndex = 0;
		for (auto PlayerControllerIt = MyWorld->GetPlayerControllerIterator(); PlayerControllerIt; ++PlayerControllerIt, ++PlayerIndex)
		{
			++PlayerIndex;
			if (!PlayerControllerIt->IsValid())
			{
				continue;
			}
			APlayerController* PlayerController = PlayerControllerIt->Get();
			
			const APawn* PlayerPawn = PlayerController->GetPawn();
			if (PlayerPawn == MyOwner)
			{
				TargetPlayer = PlayerController;
				break;
			}
			else if (PlayerIndex == AutoTargetIndex)
			{
				AutoTargetPlayer = PlayerController;
			}
		}

		if (!TargetPlayer)
		{
			TargetPlayer = AutoTargetPlayer;
		}
	}
	return TargetPlayer;
}
