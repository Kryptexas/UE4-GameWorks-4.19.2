// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "MixedRealityCalibrationUtilLibrary.generated.h"

class UTexture2D;
class UMediaTexture;
class AActor;
class UActorComponent;

USTRUCT(BlueprintType)
struct FMRAlignmentSample
{
	GENERATED_USTRUCT_BODY()

public:
	FMRAlignmentSample();

	UPROPERTY(BlueprintReadWrite, Category = SampleData)
	FVector  SampledWorldPosition;
	UPROPERTY(BlueprintReadWrite, Category = SampleData)
	FRotator SampledWorldOrientation;
	UPROPERTY(BlueprintReadWrite, Category = TargetInfo)
	FVector  RelativeTargetPosition;
	UPROPERTY(BlueprintReadWrite, Category = TargetInfo)
	FRotator RelativeTargetRotation;
	UPROPERTY(BlueprintReadWrite, Category = FixupData)
	FVector  SampleAdjustmentOffset;
	UPROPERTY(BlueprintReadWrite, Category = FixupData)
	FRotator SampleAdjustmentRotation;

	FVector GetAdjustedSamplePoint() const;
	FVector GetTargetPositionInWorldSpace(const FVector& ViewOrigin, const FRotator& ViewOrientation) const;

	UPROPERTY(BlueprintReadWrite, Category = PlaneData)
	int32 PlanarId;
};

UCLASS()
class UMRCalibrationUtilLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="MixedReality|CalibrationUtils")
	static void FindOutliers(const TArray<float>& DataSet, TArray<int32>& OutlierIndices);

	UFUNCTION(BlueprintCallable, Category = "MixedReality|CalibrationUtils")
	static int32 FindBestAnchorPoint(const TArray<FMRAlignmentSample>& AlignmentPoints, const FVector& ViewOrigin, const FRotator& ViewOrientation);

	UFUNCTION(BlueprintCallable, Category = "MixedReality|CalibrationUtils")
	static void FindBalancingAlignmentOffset(const TArray<FMRAlignmentSample>& AlignmentPoints, const FVector& ViewOrigin, const FRotator& ViewOrientation, const bool bOmitOutliers, FVector& BalancingOffset);

	UFUNCTION(BlueprintCallable, Category = "MixedReality|CalibrationUtils")
	static void CalculateAlignmentNormals(const TArray<FMRAlignmentSample>& AlignmentPoints, const FVector& ViewOrigin, TArray<FVector>& PlanarNormals, const bool bOmitOutliers = true);

	UFUNCTION(BlueprintCallable, Category = "MixedReality|CalibrationUtils")
	static bool FindAverageLookAtDirection(const TArray<FMRAlignmentSample>& AlignmentPoints, const FVector& ViewOrigin, FVector& LookAtDir, const bool bOmitOutliers = true);

	UFUNCTION(BlueprintCallable, Category = "MixedReality|CalibrationUtils", meta = (WorldContext="WorldContextObj"))
	static void GetCommandKeyStates(UObject* WorldContextObj, bool& bIsCtlDown, bool& bIsAltDown, bool& bIsShiftDown);

	UFUNCTION(BlueprintCallable, Category = "MixedReality|CalibrationUtils")
	static UActorComponent* AddComponentFromClass(AActor* Owner, TSubclassOf<UActorComponent> ComponentClass, FName ComponentName = TEXT("MRCC"), bool bManualAttachment = false);

	UFUNCTION(BlueprintPure, Category = "MixedReality|CalibrationUtils")
	static bool ClassImplementsInterface(UClass* ObjectClass, TSubclassOf<UInterface> InterfaceClass);
};
