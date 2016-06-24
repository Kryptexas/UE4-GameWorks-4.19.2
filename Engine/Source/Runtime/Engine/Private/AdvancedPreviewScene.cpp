// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#if WITH_EDITOR

#include "EnginePrivate.h"
#include "AdvancedPreviewScene.h"
#include "Components/PostProcessComponent.h"
#include "Engine/Scene.h"
#include "Engine/TextureCube.h"
#include "Materials/MaterialInstanceConstant.h"

#include "UnrealClient.h"
#include "Editor/EditorPerProjectUserSettings.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SphereReflectionCaptureComponent.h"
#include "Components/SkyLightComponent.h"

#include "AssetViewerSettings.h"

FAdvancedPreviewScene::FAdvancedPreviewScene(ConstructionValues CVS, float InFloorOffset)
	: FPreviewScene(CVS)
{
	DefaultSettings = UAssetViewerSettings::Get();
	CurrentProfileIndex = DefaultSettings->Profiles.IsValidIndex(CurrentProfileIndex) ? GetDefault<UEditorPerProjectUserSettings>()->AssetViewerProfileIndex : 0;
	ensureMsgf(DefaultSettings && DefaultSettings->Profiles.IsValidIndex(CurrentProfileIndex), TEXT("Invalid default settings pointer or current profile index"));
	FPreviewSceneProfile& Profile = DefaultSettings->Profiles[CurrentProfileIndex];

	const FTransform Transform(FRotator(0, 0, 0), FVector(0, 0, 0), FVector(1));
	
	// Add and set up sky light using the set cube map texture
	SkyLightComponent = NewObject<USkyLightComponent>();
	SkyLightComponent->Cubemap = Profile.EnvironmentCubeMap;
	SkyLightComponent->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
	SkyLightComponent->Mobility = EComponentMobility::Movable;
	SkyLightComponent->bLowerHemisphereIsBlack = false;
	SkyLightComponent->Intensity = Profile.SkyLightIntensity;
	AddComponent(SkyLightComponent, Transform);
	SkyLightComponent->UpdateSkyCaptureContents(PreviewWorld);
	
	// Large scale to prevent sphere from clipping
	const FTransform SphereTransform(FRotator(0, 0, 0), FVector(0, 0, 0), FVector(2000));
	SkyComponent = NewObject<UStaticMeshComponent>(GetTransientPackage());

	// Set up sky sphere showing hte same cube map as used by the sky light
	UStaticMesh* SkySphere = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/BasicShapes/Sphere.Sphere"), NULL, LOAD_None, NULL);
	check(SkySphere);
	SkyComponent->SetStaticMesh(SkySphere);

	UMaterial* SkyMaterial = LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorMaterials/AssetViewer/M_SkyBox.M_SkyBox"), NULL, LOAD_None, NULL);
	check(SkyMaterial);

	InstancedSkyMaterial = NewObject<UMaterialInstanceConstant>(GetTransientPackage());
	InstancedSkyMaterial->Parent = SkyMaterial;		

	UTextureCube* DefaultTexture = LoadObject<UTextureCube>(NULL, TEXT("/Engine/MapTemplates/Sky/SunsetAmbientCubemap.SunsetAmbientCubemap"));
	InstancedSkyMaterial->SetTextureParameterValueEditorOnly(FName("SkyBox"), ( Profile.EnvironmentCubeMap != nullptr ) ? Profile.EnvironmentCubeMap : DefaultTexture );
	InstancedSkyMaterial->SetScalarParameterValueEditorOnly(FName("CubemapRotation"), Profile.LightingRigRotation / 360.0f);
	InstancedSkyMaterial->SetScalarParameterValueEditorOnly(FName("Intensity"), Profile.SkyLightIntensity);
	InstancedSkyMaterial->PostLoad();
	SkyComponent->SetMaterial(0, InstancedSkyMaterial);
	AddComponent(SkyComponent, SphereTransform);
	
	PostProcessComponent = NewObject<UPostProcessComponent>();
	PostProcessComponent->Settings = Profile.PostProcessingSettings;
	PostProcessComponent->bUnbound = true;
	AddComponent(PostProcessComponent, Transform);

	UStaticMesh* FloorMesh = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorMeshes/PhAT_FloorBox.PhAT_FloorBox"), NULL, LOAD_None, NULL);
	check(FloorMesh);
	FloorMeshComponent = NewObject<UStaticMeshComponent>(GetTransientPackage());
	FloorMeshComponent->SetStaticMesh(FloorMesh);

	FTransform FloorTransform(FRotator(0, 0, 0), FVector(0, 0, -(InFloorOffset)), FVector(4.0f, 4.0f, 1.0f ));
	AddComponent(FloorMeshComponent, FloorTransform);	

	// Set up directional light's cascaded shadow maps for higher shadow quality
	DirectionalLight->DynamicShadowCascades = 4;
	DirectionalLight->DynamicShadowDistanceMovableLight = 5000.0f;
	DirectionalLight->ShadowBias = 0.7f;

	bRotateLighting = Profile.bRotateLightingRig;
	CurrentRotationSpeed = Profile.RotationSpeed;
}

void FAdvancedPreviewScene::UpdateScene(FPreviewSceneProfile& Profile, bool bUpdateSkyLight /*= true*/, bool bUpdateEnvironment  /*= true*/, bool bUpdatePostProcessing /*= true*/, bool bUpdateDirectionalLight /*= true*/)
{
	if (bUpdateSkyLight)
	{
		SkyLightComponent->Cubemap = Profile.EnvironmentCubeMap;
		SkyLightComponent->SourceCubemapAngle = Profile.LightingRigRotation;
		SkyLightComponent->SetIntensity(Profile.SkyLightIntensity);
		SkyLightComponent->SetCaptureIsDirty();
		SkyLightComponent->MarkRenderStateDirty();
		SkyLightComponent->UpdateSkyCaptureContents(PreviewWorld);	

		InstancedSkyMaterial->SetScalarParameterValueEditorOnly(FName("Intensity"), Profile.SkyLightIntensity);
		InstancedSkyMaterial->PostEditChange();
	}

	if (bUpdateEnvironment)
	{
		InstancedSkyMaterial->SetTextureParameterValueEditorOnly(FName("SkyBox"), Profile.EnvironmentCubeMap);		
		InstancedSkyMaterial->SetScalarParameterValueEditorOnly(FName("CubemapRotation"), Profile.LightingRigRotation / 360.0f);
		InstancedSkyMaterial->PostEditChange();		
	}

	if (bUpdatePostProcessing)
	{
		PostProcessComponent->Settings = Profile.PostProcessingSettings;
	}

	if (bUpdateDirectionalLight)
	{
		DirectionalLight->SetIntensity(Profile.DirectionalLightIntensity);
		DirectionalLight->SetLightColor(Profile.DirectionalLightColor);
	}

	SkyComponent->SetVisibility(Profile.bShowEnvironment, true);
	SkyComponent->MarkRenderStateDirty();
	FloorMeshComponent->SetVisibility(Profile.bShowFloor, true);

	bRotateLighting = Profile.bRotateLightingRig;
	CurrentRotationSpeed = Profile.RotationSpeed;
}

void FAdvancedPreviewScene::SetFloorOffset(const float InFloorOffset)
{
	FTransform FloorTransform(FRotator(0, 0, 0), FVector(0, 0, -(InFloorOffset)), FVector(4.0f, 4.0f, 1.0f));

	FloorMeshComponent->SetRelativeTransform(FloorTransform);
}

void FAdvancedPreviewScene::SetProfileIndex(const int32 InProfileIndex)
{
	CurrentProfileIndex = InProfileIndex;
	UpdateScene(DefaultSettings->Profiles[CurrentProfileIndex]);
}

void FAdvancedPreviewScene::Tick(float DeltaTime)
{
	checkf(DefaultSettings && DefaultSettings->Profiles.IsValidIndex(CurrentProfileIndex), TEXT("Invalid default settings pointer or current profile index"));

	FPreviewSceneProfile& Profile = DefaultSettings->Profiles[CurrentProfileIndex];
	if (Profile.bRotateLightingRig)
	{
		CurrentRotationSpeed = Profile.RotationSpeed;
		Profile.LightingRigRotation = FMath::Fmod(FMath::Max(FMath::Min(Profile.LightingRigRotation + (CurrentRotationSpeed * DeltaTime), 360.0f), 0.0f), 360.0f);

		FRotator LightDir = DirectionalLight->ComponentToWorld.GetUnitAxis(EAxis::X).Rotation();
		LightDir.Yaw += DeltaTime * -CurrentRotationSpeed;
		SetLightDirection(LightDir);
	}

	if ( PreviousRotation != Profile.LightingRigRotation )
	{		
		SkyLightComponent->SourceCubemapAngle = Profile.LightingRigRotation;
		SkyLightComponent->SetCaptureIsDirty();
		SkyLightComponent->MarkRenderStateDirty();
		SkyLightComponent->UpdateSkyCaptureContents(PreviewWorld);

		InstancedSkyMaterial->SetScalarParameterValueEditorOnly(FName("CubemapRotation"), Profile.LightingRigRotation / 360.0f);
		InstancedSkyMaterial->PostEditChange();

		PreviewWorld->UpdateAllReflectionCaptures();
		PreviewWorld->UpdateAllSkyCaptures();
	}

	PreviousRotation = Profile.LightingRigRotation;
}

bool FAdvancedPreviewScene::IsTickable() const
{
	return true;
}

TStatId FAdvancedPreviewScene::GetStatId() const
{
	return TStatId();
}

const bool FAdvancedPreviewScene::HandleViewportInput(FViewport* InViewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	bool bResult = false;
	const bool bMouseButtonDown = InViewport->KeyState(EKeys::LeftMouseButton) || InViewport->KeyState(EKeys::MiddleMouseButton) || InViewport->KeyState(EKeys::RightMouseButton);
	const bool bSkyMove = InViewport->KeyState(EKeys::K);

	// Look at which axis is being dragged and by how much
	const float DragX = (Key == EKeys::MouseX) ? Delta : 0.f;

	// Move the sky around if K is down and the mouse has moved on the X-axis
	if (bSkyMove && bMouseButtonDown)
	{
		static const float SkyRotationSpeed = 0.22f;
		float SkyRotation = GetSkyRotation();
		SkyRotation += -DragX * SkyRotationSpeed;
		SetSkyRotation(SkyRotation);		
		bResult = true;
	}

	return bResult;
}

const float FAdvancedPreviewScene::GetSkyRotation() const
{
	checkf(DefaultSettings && DefaultSettings->Profiles.IsValidIndex(CurrentProfileIndex), TEXT("Invalid default settings pointer or current profile index"));
	return DefaultSettings->Profiles[CurrentProfileIndex].LightingRigRotation;
}

void FAdvancedPreviewScene::SetSkyRotation(const float SkyRotation)
{
	checkf(DefaultSettings && DefaultSettings->Profiles.IsValidIndex(CurrentProfileIndex), TEXT("Invalid default settings pointer or current profile index"));

	float ClampedSkyRotation = FMath::Fmod(SkyRotation, 360.0f);
	// Clamp and wrap around sky rotation
	if (ClampedSkyRotation < 0.0f)
	{
		ClampedSkyRotation += 360.0f;
	}
	DefaultSettings->Profiles[CurrentProfileIndex].LightingRigRotation = ClampedSkyRotation;
}

const UStaticMeshComponent* FAdvancedPreviewScene::GetFloorMeshComponent() const
{
	checkf(FloorMeshComponent != nullptr, TEXT("Invalid floor mesh component pointer"));
	return FloorMeshComponent;
}

#endif // WITH_EDITOR