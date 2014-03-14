// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "LandscapeGizmoActiveActor.generated.h"

UENUM()
enum ELandscapeGizmoType
{
	LGT_None,
	LGT_Height,
	LGT_Weight,
	LGT_MAX,
};

USTRUCT()
struct FGizmoSelectData
{
	GENERATED_USTRUCT_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	float Ratio;

	UPROPERTY()
	float HeightData;

#endif // WITH_EDITORONLY_DATA


		TMap<class ULandscapeLayerInfoObject*, float>	WeightDataMap;

		FGizmoSelectData()
		#if WITH_EDITORONLY_DATA
		: Ratio(0.f), HeightData(0.f), WeightDataMap()
		#endif
		{
		}
	
};

UCLASS(notplaceable, HeaderGroup=Terrain, MinimalAPI)
class ALandscapeGizmoActiveActor : public ALandscapeGizmoActor
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TEnumAsByte<enum ELandscapeGizmoType> DataType;

	UPROPERTY(Transient)
	class UTexture2D* GizmoTexture;

	UPROPERTY()
	FVector2D TextureScale;

	UPROPERTY()
	TArray<FVector> SampledHeight;

	UPROPERTY()
	TArray<FVector> SampledNormal;

	UPROPERTY()
	int32 SampleSizeX;

	UPROPERTY()
	int32 SampleSizeY;

	UPROPERTY()
	float CachedWidth;

	UPROPERTY()
	float CachedHeight;

	UPROPERTY()
	float CachedScaleXY;

	UPROPERTY(Transient)
	FVector FrustumVerts[8];

	UPROPERTY()
	class UMaterial* GizmoMaterial;

	UPROPERTY()
	class UMaterialInstance* GizmoDataMaterial;

	UPROPERTY()
	class UMaterial* GizmoMeshMaterial;

	UPROPERTY(Category=LandscapeGizmoActiveActor, VisibleAnywhere)
	TArray<class ULandscapeLayerInfoObject*> LayerInfos;

	UPROPERTY(transient)
	bool bSnapToLandscapeGrid;

	UPROPERTY(transient)
	FRotator UnsnappedRotation;
#endif // WITH_EDITORONLY_DATA

public:
	TMap<FIntPoint, FGizmoSelectData> SelectedData;

#if WITH_EDITOR
	// Begin UObject interface.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void PostEditMove(bool bFinished) OVERRIDE;
	// End UObject interface.

	virtual FVector SnapToLandscapeGrid(const FVector& GizmoLocation) const;
	virtual FRotator SnapToLandscapeGrid(const FRotator& GizmoRotation) const;

	virtual void EditorApplyTranslation(const FVector& DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown) OVERRIDE;
	virtual void EditorApplyRotation(const FRotator& DeltaRotation, bool bAltDown, bool bShiftDown, bool bCtrlDown) OVERRIDE;

	/**
	 * Whenever the decal actor has moved:
	 *  - Copy the actor rot/pos info over to the decal component
	 *  - Trigger updates on the decal component to recompute its matrices and generate new geometry.
	 */
	ENGINE_API ALandscapeGizmoActor* SpawnGizmoActor();

	// @todo document
	ENGINE_API void ClearGizmoData();

	// @todo document
	ENGINE_API void FitToSelection();

	// @todo document
	ENGINE_API void FitMinMaxHeight();

	// @todo document
	ENGINE_API void SetTargetLandscape(ULandscapeInfo* LandscapeInfo);


	// @todo document
	void CalcNormal();

	// @todo document
	ENGINE_API void SampleData(int32 SizeX, int32 SizeY);


	// @todo document
	ENGINE_API void ExportToClipboard();

	// @todo document
	ENGINE_API void ImportFromClipboard();

	// @todo document
	ENGINE_API void Import(int32 VertsX, int32 VertsY, uint16* HeightData, TArray<ULandscapeLayerInfoObject*> ImportLayerInfos, uint8* LayerDataPointers[] );

	// @todo document
	ENGINE_API void Export(int32 Index, TArray<FString>& Filenames);


	// @todo document
	ENGINE_API float GetNormalizedHeight(uint16 LandscapeHeight) const;

	// @todo document
	ENGINE_API float GetLandscapeHeight(float NormalizedHeight) const;


	// @todo document
	float GetWidth() const { return Width   * GetRootComponent()->RelativeScale3D.X; }

	// @todo document
	float GetHeight() const { return Height  * GetRootComponent()->RelativeScale3D.Y; }

	// @todo document
	float GetLength() const { return LengthZ * GetRootComponent()->RelativeScale3D.Z; }


	// @todo document
	void SetLength(float WorldLength) { LengthZ = WorldLength / GetRootComponent()->RelativeScale3D.Z; }

	ENGINE_API static const int32 DataTexSize = 128;
private:

	// @todo document
	FORCEINLINE float GetWorldHeight(float NormalizedHeight) const;
#endif
};
