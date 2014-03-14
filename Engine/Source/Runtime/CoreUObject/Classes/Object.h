// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.



//=============================================================================
// Unreal base structures.

// Temporary mirrors of C++ structs

#pragma once
#if !CPP      //noexport class

// Generic axis enum (mirrored for native use in Axis.h).
UENUM()
namespace EAxis
{
	enum Type
	{
		None,
		X,
		Y,
		Z
	};
}

// Interpolation data types.
UENUM()
enum EInterpCurveMode
{
	CIM_Linear,
	CIM_CurveAuto,
	CIM_Constant,
	CIM_CurveUser,
	CIM_CurveBreak,
	CIM_CurveAutoClamped,
	CIM_MAX,
};

// @warning:	When you update this, you must add an entry to GPixelFormats(see RenderUtils.cpp)
// @warning:	When you update this, you must add an entries to PixelFormat.h, usually just copy the generated section on the header into EPixelFormat
// @warning:	The *Tools DLLs will also need to be recompiled if the ordering is changed, but should not need code changes.
UENUM()
enum EPixelFormat
{
	PF_Unknown,
	PF_A32B32G32R32F,
	PF_B8G8R8A8,
	// UNORM red (0..1)
	PF_G8,
	PF_G16,
	PF_DXT1,
	PF_DXT3,
	PF_DXT5,
	PF_UYVY,
	// A RGB FP format with platform-specific implementation, for use with render targets
	PF_FloatRGB,
	// A RGBA FP format with platform-specific implementation, for use with render targets
	PF_FloatRGBA,
	// A depth+stencil format with platform-specific implementation, for use with render targets
	PF_DepthStencil,
	// A depth format with platform-specific implementation, for use with render targets
	PF_ShadowDepth,
	PF_R32_FLOAT,
	PF_G16R16,
	PF_G16R16F,
	PF_G16R16F_FILTER,
	PF_G32R32F,
	PF_A2B10G10R10,
	PF_A16B16G16R16,
	PF_D24,
	PF_R16F,
	PF_R16F_FILTER,
	PF_BC5,
	// SNORM red, green (-1..1)
	PF_V8U8,
	PF_A1,
	// A low precision floating point format.
	PF_FloatR11G11B10,
	PF_A8,
	PF_R32_UINT,
	PF_R32_SINT,
	PF_PVRTC2,
	PF_PVRTC4,
	PF_R16_UINT,
	PF_R16_SINT,
	PF_R16G16B16A16_UINT,
	PF_R16G16B16A16_SINT,
	PF_R5G6B5_UNORM,
	PF_R8G8B8A8,
	// Only used for legacy loading; do NOT use!
	PF_A8R8G8B8,
	// High precision single channel block compressed, equivalent to a single channel BC5, 8 bytes per 4x4 block.
	PF_BC4,
	// UNORM red, green (0..1)
	PF_R8G8,
	// ATITC formats
	PF_ATC_RGB,
	PF_ATC_RGBA_E,
	PF_ATC_RGBA_I,
	// Used for creating SRVs to alias a DepthStencil buffer to read Stencil.  Don't use for creating textures.
	PF_X24_G8,
	PF_ETC1,
	PF_ETC2_RGB,
	PF_ETC2_RGBA,
	PF_MAX,
};

UENUM()
namespace EMouseCursor
{
	enum Type
	{
		/** Causes no mouse cursor to be visible */
		None,

		/** Default cursor (arrow) */
		Default,

		/** Text edit beam */
		TextEditBeam,

		/** Resize horizontal */
		ResizeLeftRight,

		/** Resize vertical */
		ResizeUpDown,

		/** Resize diagonal */
		ResizeSouthEast,

		/** Resize other diagonal */
		ResizeSouthWest,

		/** MoveItem */
		CardinalCross,

		/** Target Cross */
		Crosshairs,

		/** Hand cursor */
		Hand,

		/** Grab Hand cursor */
		GrabHand,

		/** Grab Hand cursor closed */
		GrabHandClosed,

		/** a circle with a diagonal line through it */
		SlashedCircle,

		/** Eye-dropper cursor for picking colors */
		EyeDropper,
	};
}

// A globally unique identifier.
USTRUCT(immutable, noexport)
struct FGuid
{
	UPROPERTY(EditAnywhere, SaveGame, Category=Guid)
	int32 A;

	UPROPERTY(EditAnywhere, SaveGame, Category=Guid)
	int32 B;

	UPROPERTY(EditAnywhere, SaveGame, Category=Guid)
	int32 C;

	UPROPERTY(EditAnywhere, SaveGame, Category=Guid)
	int32 D;
};

// A unique identifier for networking objects
USTRUCT(immutable, noexport)
struct FNetworkGUID
{
	UPROPERTY(SaveGame)
	uint32 Value;
};

// A point or direction FVector in 3d space.
USTRUCT(immutable, noexport, BlueprintType, meta=(HasNativeMakeBreak="true"))
struct FVector
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector, SaveGame)
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector, SaveGame)
	float Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector, SaveGame)
	float Z;

};

USTRUCT(immutable, noexport)
struct FVector4
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector4, SaveGame)
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector4, SaveGame)
	float Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector4, SaveGame)
	float Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector4, SaveGame)
	float W;

};


USTRUCT(immutable, noexport, BlueprintType, meta=(HasNativeMakeBreak="true"))
struct FVector2D
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector2D, SaveGame)
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector2D, SaveGame)
	float Y;

};



USTRUCT(immutable, noexport)
struct FTwoVectors
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TwoVectors, SaveGame)
	FVector v1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TwoVectors, SaveGame)
	FVector v2;

};


// A plane definition in 3D space.

USTRUCT(immutable, noexport)
struct FPlane : public FVector
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Plane, SaveGame)
	float W;

};


// An orthogonal rotation in 3d space.

USTRUCT(immutable, noexport, BlueprintType, meta=(HasNativeMakeBreak="true"))
struct FRotator
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Rotator, SaveGame)
	float Pitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Rotator, SaveGame)
	float Yaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Rotator, SaveGame)
	float Roll;

};


// Quaternion.

USTRUCT(immutable, noexport)
struct FQuat
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Quat, SaveGame)
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Quat, SaveGame)
	float Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Quat, SaveGame)
	float Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Quat, SaveGame)
	float W;

};


// A packed normal.

USTRUCT(immutable, noexport)
struct FPackedNormal
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PackedNormal, SaveGame)
	uint8 X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PackedNormal, SaveGame)
	uint8 Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PackedNormal, SaveGame)
	uint8 Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PackedNormal, SaveGame)
	uint8 W;

};


/**
 * Screen coordinates.
 */

USTRUCT(immutable, noexport)
struct FIntPoint
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=IntPoint, SaveGame)
	int32 X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=IntPoint, SaveGame)
	int32 Y;

};


// A Color.

USTRUCT(immutable, noexport)
struct FColor
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Color, SaveGame, meta=(ClampMin="0", ClampMax="255"))
	uint8 B;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Color, SaveGame, meta=(ClampMin="0", ClampMax="255"))
	uint8 G;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Color, SaveGame, meta=(ClampMin="0", ClampMax="255"))
	uint8 R;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Color, SaveGame, meta=(ClampMin="0", ClampMax="255"))
	uint8 A;

};


// A linear color.

USTRUCT(immutable, noexport, BlueprintType)
struct FLinearColor
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LinearColor, SaveGame)
	float R;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LinearColor, SaveGame)
	float G;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LinearColor, SaveGame)
	float B;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LinearColor, SaveGame)
	float A;

};


// A bounding box.

USTRUCT(immutable, noexport)
struct FBox
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Box, SaveGame)
	FVector Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Box, SaveGame)
	FVector Max;

	UPROPERTY()
	uint8 IsValid;

};


// A bounding box and bounding sphere with the same origin.

USTRUCT(noexport)
struct FBoxSphereBounds
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BoxSphereBounds, SaveGame)
	FVector Origin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BoxSphereBounds, SaveGame)
	FVector BoxExtent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BoxSphereBounds, SaveGame)
	float SphereRadius;

};

/**
 * Structure for arbitrarily oriented boxes (i.e. not necessarily axis-aligned).
 */
USTRUCT(immutable, noexport)
struct FOrientedBox
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=OrientedBox, SaveGame)
	FVector Center;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=OrientedBox, SaveGame)
	FVector AxisX;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=OrientedBox, SaveGame)
	FVector AxisY;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=OrientedBox, SaveGame)
	FVector AxisZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=OrientedBox, SaveGame)
	float ExtentX;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=OrientedBox, SaveGame)
	float ExtentY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=OrientedBox, SaveGame)
	float ExtentZ;
};

// a 4x4 matrix.

USTRUCT(immutable, noexport)
struct FMatrix
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Matrix, SaveGame)
	FPlane XPlane;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Matrix, SaveGame)
	FPlane YPlane;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Matrix, SaveGame)
	FPlane ZPlane;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Matrix, SaveGame)
	FPlane WPlane;

};



USTRUCT(noexport)
struct FInterpCurvePointFloat
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointFloat)
	float InVal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointFloat)
	float OutVal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointFloat)
	float ArriveTangent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointFloat)
	float LeaveTangent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointFloat)
	TEnumAsByte<enum EInterpCurveMode> InterpMode;

};



USTRUCT(noexport)
struct FInterpCurveFloat
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveFloat)
	TArray<FInterpCurvePointFloat> Points;

};



USTRUCT(noexport)
struct FInterpCurvePointVector2D
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector2D)
	float InVal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector2D)
	FVector2D OutVal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector2D)
	FVector2D ArriveTangent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector2D)
	FVector2D LeaveTangent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector2D)
	TEnumAsByte<enum EInterpCurveMode> InterpMode;

};



USTRUCT(noexport)
struct FInterpCurveVector2D
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveVector2D)
	TArray<FInterpCurvePointVector2D> Points;

};



USTRUCT(noexport)
struct FInterpCurvePointVector
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector)
	float InVal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector)
	FVector OutVal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector)
	FVector ArriveTangent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector)
	FVector LeaveTangent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector)
	TEnumAsByte<enum EInterpCurveMode> InterpMode;

};



USTRUCT(noexport)
struct FInterpCurveVector
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveVector)
	TArray<FInterpCurvePointVector> Points;

};



USTRUCT(noexport)
struct FInterpCurvePointTwoVectors
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointTwoVectors)
	float InVal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointTwoVectors)
	FTwoVectors OutVal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointTwoVectors)
	FTwoVectors ArriveTangent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointTwoVectors)
	FTwoVectors LeaveTangent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointTwoVectors)
	TEnumAsByte<enum EInterpCurveMode> InterpMode;

};



USTRUCT(noexport)
struct FInterpCurveTwoVectors
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveTwoVectors)
	TArray<FInterpCurvePointTwoVectors> Points;

};



USTRUCT(noexport)
struct FInterpCurvePointLinearColor
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointLinearColor)
	float InVal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointLinearColor)
	FLinearColor OutVal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointLinearColor)
	FLinearColor ArriveTangent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointLinearColor)
	FLinearColor LeaveTangent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointLinearColor)
	TEnumAsByte<enum EInterpCurveMode> InterpMode;

};



USTRUCT(noexport)
struct FInterpCurveLinearColor
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveLinearColor)
	TArray<FInterpCurvePointLinearColor> Points;

};


/** Transform definition. */

USTRUCT(noexport, BlueprintType, meta=(HasNativeMakeBreak="true"))
struct FTransform
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Transform, SaveGame)
	FQuat Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Transform, SaveGame)
	FVector Translation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Transform, SaveGame, meta=(MakeStructureDefaultValue = "1,1,1"))
	FVector Scale3D;

};


/** Thread-safe RNG. */

USTRUCT(noexport, BlueprintType)
struct FRandomStream
{
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RandomStream, SaveGame)
	int32 InitialSeed;

	int32 Seed;
};


// A date/time value.

USTRUCT(immutable, noexport)
struct FDateTime
{
	UPROPERTY()
	int64 Ticks;
};


// A time span value.

USTRUCT(immutable, noexport)
struct FTimespan
{
	UPROPERTY()
	int64 Ticks;
};


// A string asset reference

USTRUCT(noexport)
struct FStringAssetReference
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=StringAssetReference)
	FString AssetLongPathname;
};

// A string class reference

USTRUCT(noexport)
struct FStringClassReference
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=StringClassReference)
	FString ClassName;
};

// A struct used as stub for deleted ones.

USTRUCT(noexport)
struct FFallbackStruct  
{
};


//=============================================================================
// Object: The base class all objects.
// This is a built-in Unreal class and it shouldn't be modified by mod authors 
//=============================================================================

UCLASS(abstract, noexport)
class UObject
{
	GENERATED_UCLASS_BODY()

	//=============================================================================
	// K2 support functions.
	
	/**
	 * Executes some portion of the ubergraph.
	 *
	 * @param	EntryPoint	The entry point to start code execution at.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta=(BlueprintInternalUseOnly = "true"))
	virtual void ExecuteUbergraph(int32 EntryPoint);
};

#endif
