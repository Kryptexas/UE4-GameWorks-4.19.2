// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/SaveGame.h"
#include "Math/Color.h" // for FLinearColor
#include "MixedRealityCaptureComponent.h" // for FChromaKeyParams
#include "MixedRealityCaptureDevice.h" // for FMRCaptureDeviceIndex
#include "LensDistortionAPI.h" // for FLensDistortionCameraModel
#include "MixedRealityConfigurationSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FMRLensCalibrationData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = Data)
	float FOV = 90.f;

	UPROPERTY(BlueprintReadWrite, Category = Data)
	FLensDistortionCameraModel DistortionParameters;
};

USTRUCT(BlueprintType)
struct FMRAlignmentSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = Data)
	FVector CameraOrigin = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = Data)
	FVector LookAtDir = FVector::ForwardVector;	

	UPROPERTY(BlueprintReadWrite, Category = Data)
	FName TrackingAttachmentId;
};

USTRUCT(BlueprintType)
struct FGarbageMatteSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = Data)
	FTransform Transform;
};

USTRUCT(BlueprintType)
struct FMRCompositingSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = Data)
	FChromaKeyParams ChromaKeySettings;

	UPROPERTY(BlueprintReadWrite, Category = Data)
	FMRCaptureDeviceIndex CaptureDeviceURL;

	UPROPERTY(BlueprintReadWrite, Category = Data)
	float DepthOffset = 0.0f;
};

UCLASS(BlueprintType, Blueprintable, config = Engine)
class MIXEDREALITYFRAMEWORK_API UMixedRealityCalibrationData : public USaveGame
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = SaveData)
	FMRLensCalibrationData LensData;

	UPROPERTY(BlueprintReadWrite, Category = SaveData)
	FMRAlignmentSaveData AlignmentData;

	UPROPERTY(BlueprintReadWrite, Category = SaveData)
	TArray<FGarbageMatteSaveData> GarbageMatteSaveDatas;

	UPROPERTY(BlueprintReadWrite, Category = SaveData)
	FMRCompositingSaveData CompositingData;
};

/**
 * 
 */
UCLASS(BlueprintType, config = Engine)
class MIXEDREALITYFRAMEWORK_API UMixedRealityConfigurationSaveGame : public UMixedRealityCalibrationData
{
	GENERATED_UCLASS_BODY()

public:
	// Metadata about the save file

	UPROPERTY(BlueprintReadWrite, config, Category = SaveMetadata)
	FString SaveSlotName;

	UPROPERTY(BlueprintReadWrite, config, Category = SaveMetadata)
	int32 UserIndex;
	
	UPROPERTY(BlueprintReadOnly, Category = SaveMetadata)
	int32 ConfigurationSaveVersion;
};