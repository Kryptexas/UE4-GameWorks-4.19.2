// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapImage.generated.h"

//API Reference: https://developer.leapmotion.com/documentation/cpp/api/Leap.Image.html

UCLASS(BlueprintType)
class ULeapImage : public UObject
{
	friend class FPrivateLeapImage;

	GENERATED_UCLASS_BODY()
public:
	~ULeapImage();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Texture", CompactNodeTitle = "", Keywords = "get texture"), Category = "Leap Image")
	class UTexture2D* Texture();
	
	//Faster raw distortion (R=U, G=V), requires channel conversion, 32bit float per channel texture will look odd if rendered raw.
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Distortion", CompactNodeTitle = "", Keywords = "distortion"), Category = "Leap Image")
	class UTexture2D* Distortion();

	//Visually correct distortion in UE format (R=U, G=1-V) at the cost of additional CPU time (roughly 1ms) in 8bit per channel format
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Distortion UE", CompactNodeTitle = "", Keywords = "distortion ue"), Category = "Leap Image")
	class UTexture2D* DistortionUE();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Image")
	int32 DistortionHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Image")
	int32 DistortionWidth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Image")
	int32 Height;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Image")
	int32 Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Image")
	bool IsValid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Image")
	float RayOffsetX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Image")
	float RayOffsetY;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Image")
	float RayScaleX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Image")
	float RayScaleY;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Image")
	bool UseGammaCorrection;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "rectify", CompactNodeTitle = "", Keywords = "rectify"), Category = "Leap Image")
	FVector Rectify(FVector uv) const;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "warp", CompactNodeTitle = "", Keywords = "warp"), Category = "Leap Image")
	FVector Warp(FVector xy) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leap Image")
	int32 Width;

	void SetLeapImage(const class Leap::Image &LeapImage);

private:
	class FPrivateLeapImage* Private;

	UPROPERTY()
	UTexture2D* PImagePointer = nullptr;
	UPROPERTY()
	UTexture2D* PDistortionPointer = nullptr;
};