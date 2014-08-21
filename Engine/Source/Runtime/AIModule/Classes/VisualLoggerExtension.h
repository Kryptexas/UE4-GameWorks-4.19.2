// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#if ENABLE_VISUAL_LOG
#	include "VisualLog.h"
#endif
#include "VisualLoggerExtension.generated.h"

namespace EVisLogTags
{
	const FString TAG_EQS = TEXT("EQS");
}

#if ENABLE_VISUAL_LOG
class FVisualLoggerExtension : public FVisualLogExtensionInterface
{
public:
	FVisualLoggerExtension();
	virtual void OnTimestampChange(float Timestamp, class UWorld* InWorld, class AActor* HelperActor);
	virtual void DrawData(class UWorld* InWorld, class UCanvas* Canvas, class AActor* HelperActor, const FName& TagName, const struct FVisLogEntry::FDataBlock& DataBlock, float Timestamp);
	virtual void DisableDrawingForData(class UWorld* InWorld, class UCanvas* Canvas, class AActor* HelperActor, const FName& TagName, const FVisLogEntry::FDataBlock& DataBlock, float Timestamp);

private:
	void DisableEQSRendering(class AActor* HelperActor);

protected:
	uint32 CachedEQSId;
	float CurrentTimestamp;
};
#endif //ENABLE_VISUAL_LOG

UCLASS(Abstract, CustomConstructor)
class AIMODULE_API UVisualLoggerExtension : public UObject
{
	GENERATED_UCLASS_BODY()

	UVisualLoggerExtension(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP) {}
};
