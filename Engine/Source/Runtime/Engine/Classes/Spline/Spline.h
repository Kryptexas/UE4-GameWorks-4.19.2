// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Spline.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSpline, Log, All);

USTRUCT()
struct ENGINE_API FSplineBase 
{
	GENERATED_USTRUCT_BODY()

	FSplineBase() 
	: StartTime(0.0f)
	, Duration(1.0f)
	{
	};

	/** Takes in an array of points, and a duration for how long you want the spline to play. MUST HAVE AT LEAST 4 POINTS */
	virtual void SetupSpline(TArray<FVector> ControlPoints, float InDuration = 1.0f);

	/** Returns if this is a valid curve or not. Needs at least 4 control points. */
	virtual bool IsValid() const;

	/** Gets the duration of this spline */
	float GetDuration() const;

	void SetDuration(float InDuration);

	/** Resets everything in the spline */
	virtual void ClearSpline();

	/** Returns the starting position of the spline. */
	virtual FVector GetStartPoint() const;

	/** Gets the final position of the spline */
	virtual FVector GetEndPoint() const;

	/** Gets the point along the spine using the time value (0 - Duration) */
	virtual FVector GetPositionFromTime(float Time) const;

	virtual FVector GetTangentFromTime(float Time) const;

	virtual FVector GetNormalFromTime(float Time) const;

	virtual FVector GetBinormalFromTime(float Time) const;

	virtual float GetSplineLengthAtTime(float Time, float StepSize = 0.1f) const;

	virtual float GetSplineLength(float StepSize = 0.1f) const;

	/** Interface for adding control points. This spline requires 4 points to work correctly */
	virtual void AddControlPoint(const FVector& Point, int32 Index = -1);

	/** Returns whether or not we are at the start of the curve */
	virtual bool IsAtStart(UWorld* World) const;

	/** Returns whether or not we are at the End of the curve */
	virtual bool IsAtEnd(UWorld* World) const;

	/** Function for drawing the curve */
	virtual void DrawDebugSpline(UWorld* World = NULL, int32 Steps = 20, float LifeTime = 0.0f, float Thickness = 0.0f, bool bPersistent = false, bool bJustDrawSpline = false, float BasisLength = 30.0f) const;

	/** Starts the curve playing. Now, GetPosition will return correct results */
	virtual void Play(UWorld* World);

	/** returns if this spline is playing or not */
	virtual bool IsPlaying(UWorld* World) const;

	/** Call after Play() to get positions along the spline */
	virtual FVector GetPosition(UWorld* World) const;

	/** Gets the tangent at a time duirng the curve */
	virtual FVector GetTanget(UWorld* World) const;

	/** Gets how far along we are on the curve from 0-1 */
	virtual float GetT(UWorld* World) const;

	/** Gets how far along we are on the curve from 0-Duration */
	virtual float GetTime(UWorld* World) const;

	/** Returns how many control points we have */
	virtual int32 GetNumOfControlPoints() const;

	/** Makes the spline somewhat usable  */
	virtual void MakeSplineValid(int32 NumOfPoints = 4);

protected: 

	UPROPERTY()
	float StartTime;

	UPROPERTY()
	float Duration;
};

/** Just a plane class to make it lighter weight CatMull-Rom spline*/
USTRUCT()
struct ENGINE_API FCatmullRomSpline : public FSplineBase
{
	GENERATED_USTRUCT_BODY()

	FCatmullRomSpline();

	virtual ~FCatmullRomSpline() {}

	/** Call after Play() to get positions along the spline */
	virtual FVector GetPosition(UWorld* World) const OVERRIDE;

	/** Gets the tangent at a time duirng the curve */
	virtual FVector GetTanget(UWorld* World) const OVERRIDE;

	/** Interface for adding control points. This spline requires 4 points to work correctly */
	virtual void AddControlPoint(const FVector& Point, int32 Index = -1) OVERRIDE;

	/** Returns if this is a valid curve or not. Needs at least 4 control points. */
	virtual bool IsValid() const OVERRIDE;

	/** Returns how many control points we have */
	virtual int32 GetNumOfControlPoints() const OVERRIDE;

	/** Resets everything in the spline */
	virtual void ClearSpline() OVERRIDE;

	/** Makes the spline somewhat usable  */
	virtual void MakeSplineValid(int32 NumOfPoints = 4) OVERRIDE;

	/** Gets the point along the spine using the time value (0 - Duration) */
	virtual FVector GetPositionFromTime(float Time) const OVERRIDE;

	/** Gets a tangent at a specific time on the curve */
	virtual FVector GetTangentFromTime(float Time) const OVERRIDE;

	/** Gets the normal at a time */
	virtual FVector GetNormalFromTime(float Time) const OVERRIDE;

	/** Gets the normal at a time */
	virtual FVector GetBinormalFromTime(float Time) const OVERRIDE;

	/** Returns the starting position of the spline. */
	virtual FVector GetStartPoint() const OVERRIDE;

	/** Gets the final position of the spline */
	virtual FVector GetEndPoint() const OVERRIDE;

	/** Get the splines length */
	virtual float GetSplineLengthAtTime(float Time, float StepSize = 0.1f) const OVERRIDE;

	/** Get the splines length */
	virtual float GetSplineLength(float StepSize = 0.1f) const OVERRIDE;

	/** Extra drawing for Catmull Splines */
	virtual void DrawDebugSpline(UWorld* World = NULL, int32 Steps = 20, float LifeTime = 0.0f, float Thickness = 0.0f, bool bPersistent = false, bool bJustDrawSpline = false, float BasisLength = 30.0f) const OVERRIDE;

	/** bracket operator for indexing into the control point array */
	FVector& operator[]( int32 Index );

protected:
	/** Returns Num() - 3 */
	virtual float GetLength() const;

	void GetCorrectPoints(float TValue,
		FVector& P0, 
		FVector& P1, 
		FVector& P2, 
		FVector& P3) const;

	/** Private helper for calculating a point along a CatMull-Rom spline */
	FVector GetCatMullRomPoint(const FVector& P0, 
		const FVector& P1, 
		const FVector& P2, 
		const FVector& P3, 
		float Time) const;

	FVector InternalGetTangetAtPoint(const FVector& P0, 
		const FVector& P1, 
		const FVector& P2, 
		const FVector& P3, 
		float Time) const;

	UPROPERTY()
	TArray<FVector> ControlPoints;
};

/** Uses a CatMull-Rom spline to create a linear approximation of a spline*/
USTRUCT()
struct ENGINE_API FLinearSpline
{
	GENERATED_USTRUCT_BODY()

	FLinearSpline();
};


/** Used for Blueprints */
UCLASS(BlueprintType, MinimalAPI, HideCategories=(Object))
class USpline : public UObject
{
	GENERATED_UCLASS_BODY()

	/** This function sets how long it will take for the curve to complete */
	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API void SetDuration(float InDuration);

	/** Takes in an array of points, and a duration for how long you want the spline to play. MUST HAVE AT LEAST 4 POINTS */
	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API void SetupSpline(TArray<FVector> ControlPoints, float InDuration = 1.0f);

	/** Interface for adding control points. This spline requires 4 points to work correctly */
	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API void AddControlPoint(const FVector& Point, int32 Index = -1);

	/** Gets the point along the spine using the time value (0 - Duration) */
	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API FVector GetPositionFromTime(float Time) const;

	/** Returns if this is a valid curve or not. Needs at least 4 control points. */
	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API bool IsValid() const;

	/** Gets the duration of this spline */
	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API float GetDuration() const;

	/** Resets everything in the spline */
	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API void ClearSpline();

	/** Returns the starting position of the spline. */
	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API FVector GetStartPoint() const;

	/** Gets the final position of the spline */
	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API FVector GetEndPoint() const;

	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API FVector GetTangentFromTime(float Time) const;

	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API FVector GetNormalFromTime(float Time) const;

	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API FVector GetBinormalFromTime(float Time) const;

	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API float GetSplineLengthAtTime(float Time, float StepSize = 0.1f) const;

	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API float GetSplineLength(float StepSize = 0.1f) const;

	UFUNCTION(BlueprintCallable, Category="Splines", meta=(HidePin="WorldContextObject", DefaultToSelf="WorldContextObject"))
	ENGINE_API void DrawDebugSpline(UObject* WorldContextObject, int32 Steps = 20, float LifeTime = 0.0f, float Thickness = 0.0f, bool bPersistent = false, bool bJustDrawSpline = false, float BasisLength = 30.0f);

	/** Updates the time if we are a constant vel spline */
	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API float GetVelocityCorrectedTime(float Time) const;

	/** Turns of or off constant velocity */
	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API void EnableConstantVelocity(bool ConstVelEnabled);

	UFUNCTION(BlueprintCallable, Category="Splines")
	ENGINE_API int32 GetNumberOfControlPoints() const;

protected:
	/** Do we want to move at a constant velocity? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Splines)
	bool bConstantVelocity;

private:
	/** Builds the velocity spline */
	void BuildVelocitySpline();

	/** Internal spline */
	UPROPERTY()
	FCatmullRomSpline Spline;

	/** Internal spline */
	UPROPERTY()
	FInterpCurveFloat SplineLookupTable;
};




