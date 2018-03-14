// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "LiveLinkRefSkeleton.h"
#include "LiveLinkTypes.generated.h"

USTRUCT()
struct FLiveLinkSubjectName
{
public:
	GENERATED_USTRUCT_BODY()

	// Name of the subject
	UPROPERTY(EditAnywhere, Category="Live Link")
	FName Name;

	// FName operators
	operator FName&() { return Name; }
	operator const FName&() const { return Name; }
};

USTRUCT()
struct FLiveLinkCurveElement
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName CurveName;
	
	UPROPERTY()
	float CurveValue;
};

USTRUCT()
struct FLiveLinkWorldTime
{
public:
	GENERATED_USTRUCT_BODY()

	// Time for this frame. Used during interpolation. If this goes backwards we will dump already stored frames. 
	UPROPERTY()
	double Time;

	// Value calculated on create to represent the different between the source time and client time
	UPROPERTY()
	double Offset;

	FLiveLinkWorldTime()
		: Offset(0.0)
	{
		Time = FPlatformTime::Seconds();
	};

	FLiveLinkWorldTime(const double InTime)
		: Time(InTime)
	{
		Offset = FPlatformTime::Seconds() - InTime;
	};
};

USTRUCT()
struct FLiveLinkFrameRate
{
public:
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY()
	uint32 Numerator;

	UPROPERTY()
	uint32 Denominator;

	// Defaults to 60fps
	FLiveLinkFrameRate()
		: Numerator(60), Denominator(1)
	{};

	FLiveLinkFrameRate(uint32 InNumerator, uint32 InDenominator) :
		Numerator(InNumerator), Denominator(InDenominator)
	{};

	FLiveLinkFrameRate(const FLiveLinkFrameRate& OtherFrameRate)
	{
		Numerator = OtherFrameRate.Numerator;
		Denominator = OtherFrameRate.Denominator;
	};

	bool IsValid() const
	{
		return Denominator > 0;
	};

	static LIVELINKINTERFACE_API const FLiveLinkFrameRate FPS_15;
	static LIVELINKINTERFACE_API const FLiveLinkFrameRate FPS_24;
	static LIVELINKINTERFACE_API const FLiveLinkFrameRate FPS_25;
	static LIVELINKINTERFACE_API const FLiveLinkFrameRate FPS_30;
	static LIVELINKINTERFACE_API const FLiveLinkFrameRate FPS_48;
	static LIVELINKINTERFACE_API const FLiveLinkFrameRate FPS_50;
	static LIVELINKINTERFACE_API const FLiveLinkFrameRate FPS_60;
	static LIVELINKINTERFACE_API const FLiveLinkFrameRate FPS_100;
	static LIVELINKINTERFACE_API const FLiveLinkFrameRate FPS_120;
	static LIVELINKINTERFACE_API const FLiveLinkFrameRate FPS_240;

	static LIVELINKINTERFACE_API const FLiveLinkFrameRate NTSC_24;
	static LIVELINKINTERFACE_API const FLiveLinkFrameRate NTSC_30;
	static LIVELINKINTERFACE_API const FLiveLinkFrameRate NTSC_60;
};

// A Qualified TimeCode associated with 
USTRUCT()
struct FLiveLinkTimeCode
{
public:
	GENERATED_USTRUCT_BODY()

	// Integer Seconds since Epoch 
	UPROPERTY()
	int32 Seconds;

	// Integer Frames since last second
	UPROPERTY()
	int32 Frames;

	// Value calculated on create to represent the different between the source time and client time
	UPROPERTY()
	FLiveLinkFrameRate FrameRate;

	FLiveLinkTimeCode()
		: Seconds(0), Frames(0), FrameRate()
	{};

	FLiveLinkTimeCode(const int32 InSeconds, const int32 InFrames, const FLiveLinkFrameRate& InFrameRate)
		: Seconds(InSeconds), Frames(InFrames), FrameRate(InFrameRate)
	{ };
};

USTRUCT()
struct FLiveLinkMetaData
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TMap<FName, FString> StringMetaData;

	UPROPERTY()
	FLiveLinkTimeCode SceneTime;
};

USTRUCT()
struct FLiveLinkFrameData
{
public:
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY()
	TArray<FTransform> Transforms;
	
	UPROPERTY()
	TArray<FLiveLinkCurveElement> CurveElements;
	
	UPROPERTY()
	FLiveLinkWorldTime WorldTime;

	UPROPERTY()
	FLiveLinkMetaData MetaData;
};

struct FOptionalCurveElement
{
	/** Curve Value */
	float					Value;
	/** Whether this value is set or not */
	bool					bValid;

	FOptionalCurveElement(float InValue)
		: Value(InValue)
		, bValid(true)
	{}

	FOptionalCurveElement()
		: Value(0.f)
		, bValid(false)
	{}

	bool IsValid() const
	{
		return bValid;
	}

	void SetValue(float InValue)
	{
		Value = InValue;
		bValid = true;
	}
};

//Helper struct for updating curve data across multiple frames of live link data
struct FLiveLinkCurveIntegrationData
{
public:

	// Number of new curves that need to be added to existing frames
	int32 NumNewCurves;

	// Built curve buffer for current frame in existing curve key format
	TArray<FOptionalCurveElement> CurveValues;
};

struct FLiveLinkCurveKey
{
	TArray<FName> CurveNames;

	FLiveLinkCurveIntegrationData UpdateCurveKey(const TArray<FLiveLinkCurveElement>& CurveElements);
};

struct FLiveLinkSubjectFrame
{
	// Ref Skeleton for transforms
	FLiveLinkRefSkeleton RefSkeleton;

	// Fuid for ref skeleton so we can track modifications
	FGuid RefSkeletonGuid;

	// Key for storing curve data (Names)
	FLiveLinkCurveKey	 CurveKeyData;

	// Transforms for this frame
	TArray<FTransform>		Transforms;
	
	// Curve data for this frame
	TArray<FOptionalCurveElement>	Curves;

	// Metadata for this frame
	FLiveLinkMetaData MetaData;
};