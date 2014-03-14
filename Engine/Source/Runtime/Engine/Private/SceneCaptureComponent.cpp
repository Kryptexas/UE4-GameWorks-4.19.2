// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineDecalClasses.h"
#include "../../Renderer/Private/ScenePrivate.h"

ASceneCapture::ASceneCapture(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	MeshComp = PCIP.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("CamMesh0"));
	MeshComp->BodyInstance.bEnableCollision_DEPRECATED = false;

	MeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	MeshComp->bHiddenInGame = true;
	MeshComp->CastShadow = false;
	MeshComp->PostPhysicsComponentTick.bCanEverTick = false;
	RootComponent = MeshComp;
	
	bWantsInitialize = false;
}
// -----------------------------------------------

ASceneCapture2D::ASceneCapture2D(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	DrawFrustum = PCIP.CreateDefaultSubobject<UDrawFrustumComponent>(this, TEXT("DrawFrust0"));
	DrawFrustum->AlwaysLoadOnClient = false;
	DrawFrustum->AlwaysLoadOnServer = false;
	DrawFrustum->AttachParent = MeshComp;

	CaptureComponent2D = PCIP.CreateDefaultSubobject<USceneCaptureComponent2D>(this, TEXT("NewSceneCaptureComponent2D"));
	CaptureComponent2D->AttachParent = MeshComp;
}

void ASceneCapture2D::OnInterpToggle(bool bEnable)
{
	CaptureComponent2D->SetVisibility(bEnable);
}

void ASceneCapture2D::UpdateDrawFrustum()
{
	if(DrawFrustum && CaptureComponent2D)
	{
		DrawFrustum->FrustumStartDist = GNearClippingPlane;
		
		// 1000 is the default frustum distance, ideally this would be infinite but that might cause rendering issues
		DrawFrustum->FrustumEndDist = (CaptureComponent2D->MaxViewDistanceOverride > DrawFrustum->FrustumStartDist)
			? CaptureComponent2D->MaxViewDistanceOverride : 1000.0f;

		DrawFrustum->FrustumAngle = CaptureComponent2D->FOVAngle;
		//DrawFrustum->FrustumAspectRatio = CaptureComponent2D->AspectRatio;
	}
}

void ASceneCapture2D::PostActorCreated()
{
	Super::PostActorCreated();

	// no need load the editor mesh when there is no editor
#if WITH_EDITOR
	if(MeshComp)
	{
		if (!IsRunningCommandlet())
		{
			if( !MeshComp->StaticMesh)
			{
				UStaticMesh* CamMesh = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorMeshes/MatineeCam_SM.MatineeCam_SM"), NULL, LOAD_None, NULL);
				MeshComp->SetStaticMesh(CamMesh);
			}
		}
	}
#endif

	// Sync component with CameraActor frustum settings.
	UpdateDrawFrustum();
}
// -----------------------------------------------

ASceneCaptureCube::ASceneCaptureCube(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	DrawFrustum = PCIP.CreateDefaultSubobject<UDrawFrustumComponent>(this, TEXT("DrawFrust0"));
	DrawFrustum->AlwaysLoadOnClient = false;
	DrawFrustum->AlwaysLoadOnServer = false;
	DrawFrustum->AttachParent = MeshComp;

	CaptureComponentCube = PCIP.CreateDefaultSubobject<USceneCaptureComponentCube>(this, TEXT("NewSceneCaptureComponentCube"));
	CaptureComponentCube->AttachParent = MeshComp;
}

void ASceneCaptureCube::OnInterpToggle(bool bEnable)
{
	CaptureComponentCube->SetVisibility(bEnable);
}

void ASceneCaptureCube::UpdateDrawFrustum()
{
	if(DrawFrustum && CaptureComponentCube)
	{
		DrawFrustum->FrustumStartDist = GNearClippingPlane;

		// 1000 is the default frustum distance, ideally this would be infinite but that might cause rendering issues
		DrawFrustum->FrustumEndDist = (CaptureComponentCube->MaxViewDistanceOverride > DrawFrustum->FrustumStartDist)
			? CaptureComponentCube->MaxViewDistanceOverride : 1000.0f;

		DrawFrustum->FrustumAngle = 90;
	}
}

void ASceneCaptureCube::PostActorCreated()
{
	Super::PostActorCreated();

	// no need load the editor mesh when there is no editor
#if WITH_EDITOR
	if(MeshComp)
	{
		if (!IsRunningCommandlet())
		{
			if( !MeshComp->StaticMesh)
			{
				UStaticMesh* CamMesh = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorMeshes/MatineeCam_SM.MatineeCam_SM"), NULL, LOAD_None, NULL);
				MeshComp->SetStaticMesh(CamMesh);
			}
		}
	}
#endif

	// Sync component with CameraActor frustum settings.
	UpdateDrawFrustum();
}
#if WITH_EDITOR

void ASceneCaptureCube::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if(bFinished)
	{
		CaptureComponentCube->UpdateContent();
	}
}
#endif
// -----------------------------------------------
USceneCaptureComponent::USceneCaptureComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCaptureEveryFrame = true;
	MaxViewDistanceOverride = -1;
}


// -----------------------------------------------


USceneCaptureComponent2D::USceneCaptureComponent2D(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	FOVAngle = 90.0f;
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	// previous behavior was to capture from raw scene color 
	CaptureSource = SCS_SceneColorHDR;
	// default to full blend weight..
	PostProcessBlendWeight = 1.0f;
}

void USceneCaptureComponent2D::SendRenderTransform_Concurrent()
{	
	UpdateContent();

	Super::SendRenderTransform_Concurrent();
}

void USceneCaptureComponent2D::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bCaptureEveryFrame)
	{
		UpdateComponentToWorld(false);
	}
}

static TArray<USceneCaptureComponent2D*> SceneCapturesToUpdate;

void USceneCaptureComponent2D::UpdateContent()
{
	if (World && World->Scene && IsVisible())
	{
		// Defer until after updates finish
		SceneCapturesToUpdate.AddUnique( this );
	}	
}

void USceneCaptureComponent2D::UpdateDeferredCaptures( FSceneInterface* Scene )
{
	for( int32 CaptureIndex = 0; CaptureIndex < SceneCapturesToUpdate.Num(); CaptureIndex++ )
	{
		Scene->UpdateSceneCaptureContents( SceneCapturesToUpdate[ CaptureIndex ] );
	}
	SceneCapturesToUpdate.Reset();
}

#if WITH_EDITOR
void USceneCaptureComponent2D::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// AActor::PostEditChange will ForceUpdateComponents()
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdateContent();
}
#endif // WITH_EDITOR


// -----------------------------------------------


USceneCaptureComponentCube::USceneCaptureComponentCube(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
}

void USceneCaptureComponentCube::SendRenderTransform_Concurrent()
{	
	UpdateContent();

	Super::SendRenderTransform_Concurrent();
}

void USceneCaptureComponentCube::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bCaptureEveryFrame)
	{
		UpdateComponentToWorld(false);
	}
}

static TArray<USceneCaptureComponentCube*> CubedSceneCapturesToUpdate;

void USceneCaptureComponentCube::UpdateContent()
{
	if (World && World->Scene && IsVisible())
	{
		// Defer until after updates finish
		CubedSceneCapturesToUpdate.AddUnique( this );
	}	
}

void USceneCaptureComponentCube::UpdateDeferredCaptures( FSceneInterface* Scene )
{
	for( int32 CaptureIndex = 0; CaptureIndex < CubedSceneCapturesToUpdate.Num(); CaptureIndex++ )
	{
		Scene->UpdateSceneCaptureContents( CubedSceneCapturesToUpdate[ CaptureIndex ] );
	}
	CubedSceneCapturesToUpdate.Reset();
}

#if WITH_EDITOR
void USceneCaptureComponentCube::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// AActor::PostEditChange will ForceUpdateComponents()
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdateContent();
}
#endif // WITH_EDITOR
