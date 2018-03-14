// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ARTypes.h"
#include "ARTrackable.generated.h"

class FARSystemBase;

UCLASS(BlueprintType)
class AUGMENTEDREALITY_API UARTrackedGeometry : public UObject
{
	GENERATED_BODY()
	
public:
	UARTrackedGeometry();
	
	void InitializeNativeResource(IARRef* InNativeResource);

	virtual void DebugDraw( UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds = 0.0f) const;
	
	void UpdateTrackedGeometry(const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, uint32 FrameNumber, double Timestamp, const FTransform& InLocalToTrackingTransform, const FTransform& InAlignmentTransform );
	
	void UpdateTrackingState( EARTrackingState NewTrackingState );
	
	void UpdateAlignmentTransform( const FTransform& NewAlignmentTransform );
	
	void SetDebugName( FName InDebugName );

	IARRef* GetNativeResource();
	
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Tracked Geometry")
	FTransform GetLocalToWorldTransform() const;
	
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Tracked Geometry")
	FTransform GetLocalToTrackingTransform() const;
	
	FTransform GetLocalToTrackingTransform_NoAlignment() const;
	
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Tracked Geometry")
	EARTrackingState GetTrackingState() const;
	
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Tracked Geometry")
	FName GetDebugName() const;
	
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Tracked Geometry")
	int32 GetLastUpdateFrameNumber() const;
	
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Tracked Geometry")
	float GetLastUpdateTimestamp() const;
	
protected:
	TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> GetARSystem() const;
	
	UPROPERTY()
	FTransform LocalToTrackingTransform;
	
	UPROPERTY()
	FTransform LocalToAlignedTrackingTransform;
	
	UPROPERTY()
	EARTrackingState TrackingState;

	/** A pointer to the native resource in the native AR system */
	TUniquePtr<IARRef> NativeResource;
	
private:
	TWeakPtr<FARSystemBase, ESPMode::ThreadSafe> ARSystem;
	
	/** The frame number this tracked geometry was last updated on */
	uint32 LastUpdateFrameNumber;
	
	/** The time reported by the AR system that this object was last updated */
	double LastUpdateTimestamp;
	
	/** A unique name that can be used to identify the anchor for debug purposes */
	FName DebugName;
};

UCLASS(BlueprintType)
class AUGMENTEDREALITY_API UARPlaneGeometry : public UARTrackedGeometry
{
	GENERATED_BODY()
	
public:
	void UpdateTrackedGeometry(const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, uint32 FrameNumber, double Timestamp, const FTransform& InLocalToTrackingTransform, const FTransform& InAlignmentTransform, const FVector InCenter, const FVector InExtent );
	
	void UpdateTrackedGeometry(const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, uint32 FrameNumber, double Timestamp, const FTransform& InLocalToTrackingTransform, const FTransform& InAlignmentTransform, const FVector InCenter, const FVector InExtent, const TArray<FVector>& InBoundaryPolygon, UARPlaneGeometry* InSubsumedBy);
	
	virtual void DebugDraw( UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds = 0.0f) const override;
	
public:
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Plane Geometry")
	FVector GetCenter() const { return Center; }
	
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Plane Geometry")
	FVector GetExtent() const { return Extent; }
	
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Plane Geometry")
	TArray<FVector> GetBoundaryPolygonInLocalSpace() const { return BoundaryPolygon; }

	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Plane Geometry")
	UARPlaneGeometry* GetSubsumedBy() const { return SubsumedBy; };

private:
	UPROPERTY()
	FVector Center;
	
	UPROPERTY()
	FVector Extent;
	
	// Used by ARCore Only
	UPROPERTY()
	TArray<FVector> BoundaryPolygon;

	// Used by ARCore Only
	UPROPERTY()
	UARPlaneGeometry* SubsumedBy = nullptr;
};

UCLASS(BlueprintType)
class AUGMENTEDREALITY_API UARTrackedPoint : public UARTrackedGeometry
{
	GENERATED_BODY()

public:
	virtual void DebugDraw(UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds = 0.0f) const override;

	void UpdateTrackedGeometry(const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, uint32 FrameNumber, double Timestamp, const FTransform& InLocalToTrackingTransform, const FTransform& InAlignmentTransform);
};

UENUM(BlueprintType, Category="AR AugmentedReality", meta=(Experimental))
enum class EARFaceTrackingDirection : uint8
{
	/** Blend shapes are tracked as if looking out of the face, e.g. right eye is the mesh's right eye and left side of screen if facing you */
	FaceRelative,
	/** Blend shapes are tracked as if looking at the face, e.g. right eye is the mesh's left eye and right side of screen if facing you (like a mirror) */
	FaceMirrored
};

UENUM(BlueprintType, Category="AR AugmentedReality", meta=(Experimental))
enum class EARFaceBlendShape : uint8
{
	// Left eye blend shapes
	EyeBlinkLeft,
	EyeLookDownLeft,
	EyeLookInLeft,
	EyeLookOutLeft,
	EyeLookUpLeft,
	EyeSquintLeft,
	EyeWideLeft,
	// Right eye blend shapes
	EyeBlinkRight,
	EyeLookDownRight,
	EyeLookInRight,
	EyeLookOutRight,
	EyeLookUpRight,
	EyeSquintRight,
	EyeWideRight,
	// Jaw blend shapes
	JawForward,
	JawLeft,
	JawRight,
	JawOpen,
	// Mouth blend shapes
	MouthClose,
	MouthFunnel,
	MouthPucker,
	MouthLeft,
	MouthRight,
	MouthSmileLeft,
	MouthSmileRight,
	MouthFrownLeft,
	MouthFrownRight,
	MouthDimpleLeft,
	MouthDimpleRight,
	MouthStretchLeft,
	MouthStretchRight,
	MouthRollLower,
	MouthRollUpper,
	MouthShrugLower,
	MouthShrugUpper,
	MouthPressLeft,
	MouthPressRight,
	MouthLowerDownLeft,
	MouthLowerDownRight,
	MouthUpperUpLeft,
	MouthUpperUpRight,
	// Brow blend shapes
	BrowDownLeft,
	BrowDownRight,
	BrowInnerUp,
	BrowOuterUpLeft,
	BrowOuterUpRight,
	// Cheek blend shapes
	CheekPuff,
	CheekSquintLeft,
	CheekSquintRight,
	// Nose blend shapes
	NoseSneerLeft,
	NoseSneerRight,
	MAX
};

typedef TMap<EARFaceBlendShape, float> FARBlendShapeMap;

UCLASS(BlueprintType)
class AUGMENTEDREALITY_API UARFaceGeometry : public UARTrackedGeometry
{
	GENERATED_BODY()
	
public:
	void UpdateTrackedGeometry(const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, uint32 FrameNumber, double Timestamp, const FTransform& InTransform, const FTransform& InAlignmentTransform, FARBlendShapeMap& InBlendShapes, TArray<FVector>& InVertices, const TArray<int32>& Indices);
	
	virtual void DebugDraw( UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds = 0.0f) const override;
	
public:
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Face Geometry")
	float GetBlendShapeValue(EARFaceBlendShape BlendShape) const;
	
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Face Geometry")
	const TMap<EARFaceBlendShape, float> GetBlendShapes() const;

	const FARBlendShapeMap& GetBlendShapesRef() const { return BlendShapes; }
	
	const TArray<FVector>& GetVertexBuffer() const { return VertexBuffer; }
	const TArray<int32>& GetIndexBuffer() const { return IndexBuffer; }
	const TArray<FVector2D>& GetUVs() const { return UVs; }
	
private:
	UPROPERTY()
	TMap<EARFaceBlendShape, float> BlendShapes;
	
	// Holds the face data for one or more face components that want access
	TArray<FVector> VertexBuffer;
	TArray<int32> IndexBuffer;
	// @todo JoeG - route the uvs in
	TArray<FVector2D> UVs;
};
