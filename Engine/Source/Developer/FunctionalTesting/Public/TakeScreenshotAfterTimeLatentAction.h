// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

class FTakeScreenshotAfterTimeLatentAction : public FPendingLatentAction
{
public:
	FTakeScreenshotAfterTimeLatentAction(const FLatentActionInfo& LatentInfo, const FString& InScreenshotName, FIntPoint Resolution, float Seconds);
	virtual ~FTakeScreenshotAfterTimeLatentAction();

	virtual void UpdateOperation(FLatentResponse& Response) override;

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override;
#endif

private:
	void OnScreenshotTaken(int32 InSizeX, int32 InSizeY, const TArray<FColor>& InImageData);

private:
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	FDelegateHandle	DelegateHandle;
	FString ScreenshotName;
	float SecondsRemaining;
	bool IssuedScreenshotCapture;
	bool TakenScreenshot;
	FIntPoint DesiredResolution;
};
