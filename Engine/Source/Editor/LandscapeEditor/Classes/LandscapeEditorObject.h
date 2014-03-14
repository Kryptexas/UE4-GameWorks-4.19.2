// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LandscapeEditorObject.generated.h"

UENUM()
namespace ELandscapeToolFlattenMode
{
	enum Type
	{
		//Invalid = -1,

		// Flatten may both raise and lower values
		Both = 0,

		// Flatten may only raise values, values above the clicked point will be left unchanged
		Raise = 1,

		// Flatten may only lower values, values below the clicked point will be left unchanged
		Lower = 2,
	};
}

UENUM()
namespace ELandscapeToolErosionMode
{
	enum Type
	{
		//Invalid = -1,

		// Apply all erosion effects, both raising and lowering the heightmap
		Both = 0,

		// Only applies erosion effects that result in raising the heightmap
		Raise = 1,

		// Only applies erosion effects that result in lowering the heightmap
		Lower = 2,
	};
}

UENUM()
namespace ELandscapeToolHydroErosionMode
{
	enum Type
	{
		//Invalid = -1,

		// Rains in some places and not others, randomly
		Both = 0,

		// Rain is applied to the entire area
		Positive = 1,
	};
}

// Temp
#if !CPP
UENUM()
namespace ELandscapeToolNoiseMode
{
	enum Type
	{
		//Invalid = -1,

		// Noise will both raise and lower the heightmap
		Both = 0,

		// Noise will only raise the heightmap
		Raise = 1,

		// Noise will only lower the heightmap
		Lower = 2,
	};
}
#endif

UENUM()
namespace ELandscapeToolPasteMode
{
	enum Type
	{
		//Invalid = -1,

		// Paste may both raise and lower values
		Both = 0,

		// Paste may only raise values, places where the pasted data would be below the heightmap are left unchanged. Good for copy/pasting mountains
		Raise = 1,

		// Paste may only lower values, places where the pasted data would be above the heightmap are left unchanged. Good for copy/pasting valleys or pits
		Lower = 2,
	};
}

UENUM()
namespace ELandscapeConvertMode
{
	enum Type
	{
		//Invalid = -1,

		// Adds data to the edge of the landscape to get to a whole number of components
		Expand = 0,

		// Removes data from the edge of the landscape to get to a whole number of components
		Clip = 1,
	};
}

UENUM()
namespace EColorChannel
{
	enum Type
	{
		Red,
		Green,
		Blue,
		Alpha,
	};
}

USTRUCT()
struct FGizmoImportLayer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category="Import", EditAnywhere)
	FString LayerFilename;

	UPROPERTY(Category="Import", EditAnywhere)
	FString LayerName;

	UPROPERTY(Category="Import", EditAnywhere)
	bool bNoImport;

	FGizmoImportLayer()
		: LayerFilename("")
		, LayerName("")
		, bNoImport(false)
	{
	}
};

UENUM()
namespace ELandscapeImportHeightmapError
{
	enum Type
	{
		None,
		FileNotFound,
		InvalidSize,
		CorruptFile,
		ColorPng,
		LowBitDepth,
	};
}

UENUM()
namespace ELandscapeImportLayerError
{
	enum Type
	{
		None,
		MissingLayerInfo,
		FileNotFound,
		FileSizeMismatch,
		CorruptFile,
		ColorPng,
	};
}

USTRUCT()
struct FLandscapeImportLayer : public FLandscapeImportLayerInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category="Import", VisibleAnywhere)
	TEnumAsByte<ELandscapeImportLayerError::Type> ImportError;

	FLandscapeImportLayer()
		: FLandscapeImportLayerInfo()
		, ImportError(ELandscapeImportLayerError::None)
	{
	}
};

UCLASS()
class ULandscapeEditorObject : public UObject
{
	GENERATED_UCLASS_BODY()

	class FEdModeLandscape* ParentMode;


	// Common Tool Settings:

	// Strength of the tool
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(ShowForTools="ToolSet_Paint,ToolSet_Sculpt,ToolSet_Smooth,ToolSet_Flatten,ToolSet_Erosion,ToolSet_HydraErosion,ToolSet_Noise,ToolSet_Mask,ToolSet_CopyPaste", ClampMin="0", ClampMax="10", UIMin="0", UIMax="1"))
	float ToolStrength;

	// Enable to make tools blend towards a target value
	UPROPERTY()
	bool bUseWeightTargetValue;

	// Enable to make tools blend towards a target value
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Use Target Value", editcondition="bUseWeightTargetValue", ShowForTools="ToolSet_Paint,ToolSet_Sculpt,ToolSet_Noise", ClampMin="0", ClampMax="10", UIMin="0", UIMax="1"))
	float WeightTargetValue;

	// I have no idea what this is for but it's used by the noise and erosion tools, and isn't exposed to the UI
	UPROPERTY()
	float MaximumValueRadius;

	// Flatten Tool:

	// Whether to flatten by lowering, raising, or both
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(ShowForTools="ToolSet_Flatten"))
	TEnumAsByte<ELandscapeToolFlattenMode::Type> FlattenMode;

	// Flattens to the angle of the clicked point, instead of horizontal
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(ShowForTools="ToolSet_Flatten"))
	bool bUseSlopeFlatten;

	// Constantly picks new values to flatten towards when dragging around, instead of only using the first clicked point
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(ShowForTools="ToolSet_Flatten"))
	bool bPickValuePerApply;

	// Enable to flatten towards a target height
	UPROPERTY()
	bool bUseFlattenTarget;

	// Target height to flatten towards (in Unreal Units)
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(ShowForTools="ToolSet_Flatten", editcondition="bUseFlattenTarget", UIMin="-32768", UIMax="32768"))
	float FlattenTarget;

	// Ramp Tool:

	// Width of ramp
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(ShowForTools="ToolSet_Ramp", ClampMin="1", UIMin="1", UIMax="8192", SliderExponent=3))
	float RampWidth;

	// Falloff on side of ramp
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Side Falloff", ShowForTools="ToolSet_Ramp", ClampMin="0", ClampMax="1", UIMin="0", UIMax="1"))
	float RampSideFalloff;

	// Smooth Tool:

	// Scale multiplier for the smoothing filter kernel
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Filter Kernel Scale", ShowForTools="ToolSet_Smooth", ClampMin="0", ClampMax="1", UIMin="0", UIMax="1"))
	float SmoothFilterKernelScale;

	// If checked, performs a detail preserving smooth using the specified detail smoothing value
	UPROPERTY()
	bool bDetailSmooth;

	// Larger detail smoothing values remove more details, while smaller values preserve more details
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Detail Smooth", editcondition="bDetailSmooth", ShowForTools="ToolSet_Smooth", ClampMin="0", ClampMax="0.99"))
	float DetailScale;

	// Erosion Tool:

	// The minimum height difference necessary for the erosion effects to be applied. Smaller values will result in more erosion being applied
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Threshold", ShowForTools="ToolSet_Erosion", ClampMin="0", ClampMax="256", UIMin="0", UIMax="128"))
	int32 ErodeThresh;

	// The thickness of the surface for the layer weight erosion effect
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Surface Thickness", ShowForTools="ToolSet_Erosion", ClampMin="128", ClampMax="1024", UIMin="128", UIMax="512"))
	int32 ErodeSurfaceThickness;

	// Number of erosion iterations, more means more erosion but is slower
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Iterations", ShowForTools="ToolSet_Erosion", ClampMin="1", ClampMax="300", UIMin="1", UIMax="150"))
	int32 ErodeIterationNum;

	// Whether to erode by lowering, raising, or both
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Noise Mode", ShowForTools="ToolSet_Erosion"))
	TEnumAsByte<ELandscapeToolErosionMode::Type> ErosionNoiseMode;

	// The size of the perlin noise filter used
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Noise Scale", ShowForTools="ToolSet_Erosion", ClampMin="1", ClampMax="512", UIMin="1.1", UIMax="256"))
	float ErosionNoiseScale;

	// Hydraulic Erosion Tool:

	// The amount of rain to apply to the surface. Larger values will result in more erosion
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(ShowForTools="ToolSet_HydraErosion", ClampMin="1", ClampMax="512", UIMin="1", UIMax="256"))
	int32 RainAmount;

	// The amount of sediment that the water can carry. Larger values will result in more erosion
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Sediment Cap.", ShowForTools="ToolSet_HydraErosion", ClampMin="0.1", ClampMax="1.0"))
	float SedimentCapacity;

	// Number of erosion iterations, more means more erosion but is slower
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Iterations", ShowForTools="ToolSet_HydraErosion", ClampMin="1", ClampMax="300", UIMin="1", UIMax="150"))
	int32 HErodeIterationNum;

	// Initial Rain Distribution
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Initial Rain Distribution", ShowForTools="ToolSet_HydraErosion"))
	TEnumAsByte<ELandscapeToolHydroErosionMode::Type> RainDistMode;

	// The size of the noise filter for applying initial rain to the surface
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(ShowForTools="ToolSet_HydraErosion", ClampMin="1", ClampMax="512", UIMin="1.1", UIMax="256"))
	float RainDistScale;

	// If checked, performs a detail-preserving smooth to the erosion effect using the specified detail smoothing value
	UPROPERTY()
	bool bHErosionDetailSmooth;

	// Larger detail smoothing values remove more details, while smaller values preserve more details
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Detail Smooth", editcondition="bHErosionDetailSmooth", ShowForTools="ToolSet_HydraErosion", ClampMin="0", ClampMax="0.99"))
	float HErosionDetailScale;

	// Noise Tool:

	// Whether to apply noise that raises, lowers, or both
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Noise Mode", ShowForTools="ToolSet_Noise"))
	TEnumAsByte<ELandscapeToolNoiseMode::Type> NoiseMode;

	// The size of the perlin noise filter used
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Noise Scale", ShowForTools="ToolSet_Noise", ClampMin="1", ClampMax="512", UIMin="1.1", UIMax="256"))
	float NoiseScale;

	// Mask Tool:

	// Uses selected region as a mask for other tools
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Use Region as Mask", ShowForTools="ToolSet_Mask", ShowForMask))
	bool bUseSelectedRegion;

	// If enabled, protects the selected region from changes
	// If disabled, only allows changes in the selected region
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Negative Mask", ShowForTools="ToolSet_Mask", ShowForMask))
	bool bUseNegativeMask;

	// Copy/Paste Tool:

	// Whether to paste will only raise, only lower, or both
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(ShowForTools="ToolSet_CopyPaste"))
	TEnumAsByte<ELandscapeToolPasteMode::Type> PasteMode;

	// If set, copies/pastes all layers, otherwise only copy/pastes the layer selected in the targets panel
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Gizmo copy/paste all layers", ShowForTools="ToolSet_CopyPaste"))
	bool bApplyToAllTargets;

	// Makes sure the gizmo is snapped perfectly to the landscape so that the sample points line up, which makes copy/paste less blurry. Irrelevant if gizmo is scaled
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Snap Gizmo to Landscape grid", ShowForTools="ToolSet_CopyPaste"))
	bool bSnapGizmo;

	// Smooths the edges of the gizmo data into the landscape. Without this, the edges of the pasted data will be sharp
	UPROPERTY(Category="Tool Settings", EditAnywhere, meta=(DisplayName="Use Smooth Gizmo Brush", ShowForTools="ToolSet_CopyPaste"))
	bool bSmoothGizmoBrush;

	UPROPERTY(Category="Tool Settings", EditAnywhere, AdvancedDisplay, meta=(DisplayName="Heightmap", ShowForTools="ToolSet_CopyPaste"))
	FString GizmoHeightmapFilenameString;

	UPROPERTY(Category="Tool Settings", EditAnywhere, AdvancedDisplay, meta=(DisplayName="Heightmap Size", ShowForTools="ToolSet_CopyPaste"))
	FIntPoint GizmoImportSize;

	UPROPERTY(Category="Tool Settings", EditAnywhere, AdvancedDisplay, meta=(DisplayName="Layers", ShowForTools="ToolSet_CopyPaste"))
	TArray<FGizmoImportLayer> GizmoImportLayers;

	TArray<FGizmoHistory> GizmoHistories;


	// Resize Landscape Tool

	// Number of quads per landscape component section
	UPROPERTY(Category="Change Component Size", EditAnywhere, meta=(DisplayName="Section Size", ShowForTools="ToolSet_ResizeLandscape"))
	int32 ResizeLandscape_QuadsPerSection;

	// Number of sections per landscape component
	UPROPERTY(Category="Change Component Size", EditAnywhere, meta=(DisplayName="Sections Per Component", ShowForTools="ToolSet_ResizeLandscape"))
	int32 ResizeLandscape_SectionsPerComponent;

	// Number of components in resulting landscape
	UPROPERTY(Category="Change Component Size", EditAnywhere, meta=(DisplayName="Number of Components", ShowForTools="ToolSet_ResizeLandscape"))
	FIntPoint ResizeLandscape_ComponentCount;

	// 
	UPROPERTY(Category="Change Component Size", EditAnywhere, meta=(DisplayName="Resize Mode", ShowForTools="ToolSet_ResizeLandscape"))
	TEnumAsByte<ELandscapeConvertMode::Type> ResizeLandscape_ConvertMode;

	FIntPoint ResizeLandscape_OriginalResolution; // In Quads

	// New Landscape "Tool"

	// Material initially applied to the landscape. Setting a material here exposes properties for setting up layer info based on the landscape blend nodes in the material
	UPROPERTY(Category="New Landscape", EditAnywhere, meta=(DisplayName="Material", ShowForTools="ToolSet_NewLandscape"))
	TWeakObjectPtr<UMaterialInterface> NewLandscape_Material;

	UPROPERTY(Category="New Landscape", EditAnywhere, meta=(DisplayName="Section Size", ShowForTools="ToolSet_NewLandscape"))
	int32 NewLandscape_QuadsPerSection;
	UPROPERTY(Category="New Landscape", EditAnywhere, meta=(DisplayName="Sections Per Component", ShowForTools="ToolSet_NewLandscape"))
	int32 NewLandscape_SectionsPerComponent;
	UPROPERTY(Category="New Landscape", EditAnywhere, meta=(DisplayName="Number of Components", ShowForTools="ToolSet_NewLandscape"))
	FIntPoint NewLandscape_ComponentCount;

	UPROPERTY(Category="New Landscape", EditAnywhere, meta=(DisplayName="Location", ShowForTools="ToolSet_NewLandscape"))
	FVector NewLandscape_Location;
	UPROPERTY(Category="New Landscape", EditAnywhere, meta=(DisplayName="Rotation", ShowForTools="ToolSet_NewLandscape"))
	FRotator NewLandscape_Rotation;
	UPROPERTY(Category="New Landscape", EditAnywhere, meta=(DisplayName="Scale", ShowForTools="ToolSet_NewLandscape"))
	FVector NewLandscape_Scale;

	// Import

	UPROPERTY(Category="New Landscape", VisibleAnywhere, meta=(ShowForTools="ToolSet_NewLandscape"))
	TEnumAsByte<ELandscapeImportHeightmapError::Type> ImportLandscape_HeightmapError;
	UPROPERTY(Category="New Landscape", EditAnywhere, meta=(DisplayName="Heightmap File", ShowForTools="ToolSet_NewLandscape"))
	FString ImportLandscape_HeightmapFilename;
	UPROPERTY()
	int32 ImportLandscape_Width;
	UPROPERTY()
	int32 ImportLandscape_Height;
	UPROPERTY()
	TArray<uint16> ImportLandscape_Data;

	UPROPERTY(Category="New Landscape", EditAnywhere, EditFixedSize, meta=(DisplayName="Layers", ShowForTools="ToolSet_NewLandscape"))
	TArray<FLandscapeImportLayer> ImportLandscape_Layers;

	// Common Brush Settings:

	// The radius of the brush, in unreal units
	UPROPERTY(Category="Brush Settings", EditAnywhere, meta=(DisplayName="Brush Size", ShowForBrushes="BrushSet_Circle,BrushSet_Alpha,BrushSet_Pattern", ClampMin="1", ClampMax="65536", UIMin="1", UIMax="8192", SliderExponent="3"))
	float BrushRadius;

	// The falloff at the edge of the brush, as a fraction of the brush's size. 0 = no falloff, 1 = all falloff
	UPROPERTY(Category="Brush Settings", EditAnywhere, meta=(ShowForBrushes="BrushSet_Circle,BrushSet_Gizmo,BrushSet_Pattern", ClampMin="0", ClampMax="1"))
	float BrushFalloff;

	// Selects the Clay Brush painting mode
	UPROPERTY(Category="Brush Settings", EditAnywhere, meta=(ShowForTools="ToolSet_Sculpt", ShowForBrushes="BrushSet_Circle,BrushSet_Alpha,BrushSet_Pattern"))
	bool bUseClayBrush;

	// Alpha/Pattern Brush:

	// Scale of the brush texture. A scale of 1.000 maps the brush texture to the landscape at a 1 pixel = 1 vertex size
	UPROPERTY(Category="Brush Settings", EditAnywhere, meta=(DisplayName="Texture Scale", ShowForBrushes="BrushSet_Pattern", ClampMin="0.005", ClampMax="5", SliderExponent="3"))
	float AlphaBrushScale;

	// Rotates the brush mask texture
	UPROPERTY(Category="Brush Settings", EditAnywhere, meta=(DisplayName="Texture Rotation", ShowForBrushes="BrushSet_Pattern", ClampMin="-360", ClampMax="360", UIMin="-180", UIMax="180"))
	float AlphaBrushRotation;

	// Horizontally offsets the brush mask texture
	UPROPERTY(Category="Brush Settings", EditAnywhere, meta=(DisplayName="Texture Pan U", ShowForBrushes="BrushSet_Pattern", ClampMin="0", ClampMax="1"))
	float AlphaBrushPanU;

	// Vertically offsets the brush mask texture
	UPROPERTY(Category="Brush Settings", EditAnywhere, meta=(DisplayName="Texture Pan V", ShowForBrushes="BrushSet_Pattern", ClampMin="0", ClampMax="1"))
	float AlphaBrushPanV;

	// Mask texture to use
	UPROPERTY(Category="Brush Settings", EditAnywhere, meta=(DisplayName="Texture", ShowForBrushes="BrushSet_Alpha,BrushSet_Pattern"))
	UTexture2D* AlphaTexture;

	// Channel of Mask Texture to use
	UPROPERTY(Category="Brush Settings", EditAnywhere, meta=(DisplayName="Texture Channel", ShowForBrushes="BrushSet_Alpha,BrushSet_Pattern"))
	TEnumAsByte<EColorChannel::Type> AlphaTextureChannel;

	UPROPERTY()
	int32 AlphaTextureSizeX;
	UPROPERTY()
	int32 AlphaTextureSizeY;
	UPROPERTY()
	TArray<uint8> AlphaTextureData;

	// Component Brush:

	// Number of components X/Y to affect at once. 1 means 1x1, 2 means 2x2, etc
	UPROPERTY(Category="Brush Settings", EditAnywhere, meta=(DisplayName="Brush Size", ShowForBrushes="BrushSet_Component", ClampMin="1", ClampMax="128", UIMin="1", UIMax="64", SliderExponent="3"))
	int32 BrushComponentSize;


	// Target Layer Settings:

	// Limits painting to only the components that already have the selected layer
	UPROPERTY(Category="Target Layers", EditAnywhere)
	TEnumAsByte<ELandscapeLayerPaintingRestriction::Type> PaintingRestriction;

#if WITH_EDITOR
	// Begin UObject Interface
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End UObject Interface
#endif // WITH_EDITOR

	void Load();
	void Save();

	// Region
	void SetbUseSelectedRegion(bool InbUseSelectedRegion);
	void SetbUseNegativeMask(bool InbUseNegativeMask);

	// Copy/Paste
	void SetPasteMode(ELandscapeToolPasteMode::Type InPasteMode);
	void GuessGizmoImportSize();

	// Alpha/Pattern Brush
	bool SetAlphaTexture(UTexture2D* InTexture, EColorChannel::Type InTextureChannel);

	// New Landscape
	FString LastImportPath;

	const TArray<uint16>& GetImportLandscapeData();
	void ClearImportLandscapeData() { ImportLandscape_Data.Empty(); }

	void RefreshImportLayersList();

	void NewLandscape_ClampSize()
	{
		// Max size is either whole components below 8192 verts, or 32 components
		NewLandscape_ComponentCount.X = FMath::Clamp(NewLandscape_ComponentCount.X, 1, FMath::Min(32, FMath::Floor(8191 / (NewLandscape_SectionsPerComponent * NewLandscape_QuadsPerSection))));
		NewLandscape_ComponentCount.Y = FMath::Clamp(NewLandscape_ComponentCount.Y, 1, FMath::Min(32, FMath::Floor(8191 / (NewLandscape_SectionsPerComponent * NewLandscape_QuadsPerSection))));
	}

	void UpdateComponentCount()
	{
		const int32 ComponentSizeQuads = ResizeLandscape_QuadsPerSection * ResizeLandscape_SectionsPerComponent;
		if (ResizeLandscape_ConvertMode == ELandscapeConvertMode::Expand)
		{
			ResizeLandscape_ComponentCount.X = FMath::DivideAndRoundUp(ResizeLandscape_OriginalResolution.X, ComponentSizeQuads);
			ResizeLandscape_ComponentCount.Y = FMath::DivideAndRoundUp(ResizeLandscape_OriginalResolution.Y, ComponentSizeQuads);
		}
		else // Clip
		{
			ResizeLandscape_ComponentCount.X = FMath::Max(1, ResizeLandscape_OriginalResolution.X / ComponentSizeQuads);
			ResizeLandscape_ComponentCount.Y = FMath::Max(1, ResizeLandscape_OriginalResolution.Y / ComponentSizeQuads);
		}
	}

	void SetbSnapGizmo(bool InbSnapGizmo);

	void SetParent( class FEdModeLandscape* LandscapeParent )
	{
		ParentMode = LandscapeParent;
	}
};
