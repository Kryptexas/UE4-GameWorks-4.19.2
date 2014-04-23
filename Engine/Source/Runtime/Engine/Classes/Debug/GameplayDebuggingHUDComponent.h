// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayDebuggingHUDComponent.generated.h"

UCLASS(notplaceable)
class ENGINE_API AGameplayDebuggingHUDComponent : public AHUD
{
	GENERATED_UCLASS_BODY()

	struct FPrintContext
	{
	public:
		class UCanvas* Canvas; 
		class UFont* Font;
		float CursorX, CursorY;
		float DefaultX, DefaultY;
		FFontRenderInfo FontRenderInfo;
	public:
		FPrintContext() {}
		FPrintContext(class UFont* InFont, class UCanvas* InCanvas, float InX, float InY) : Canvas(InCanvas), Font(InFont), CursorX(InX), CursorY(InY), DefaultX(InX), DefaultY(InY) {}
	};

public:
	virtual void PostRender() OVERRIDE;
	FString GenerateAllData();

protected:

	//virtual void DrawOnCanvas(class UCanvas* Canvas, APlayerController* PC);
	virtual void DrawPath(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent);
	virtual void DrawOverHeadInformation(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent);
	virtual void DrawBasicData(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent);
	virtual void DrawBehaviorTreeData(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent);
	virtual void DrawEQSData(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent);
	virtual void DrawPerception(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent);
	virtual void DrawGameSpecificView(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent) {}
	virtual void DrawNavMeshSnapshot(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent);

	void PrintAllData();
	static void CalulateStringSize(const AGameplayDebuggingHUDComponent::FPrintContext& DefaultContext, UFont* Font, const FString& InString, float& OutX, float& OutY);
	static void CalulateTextSize(const AGameplayDebuggingHUDComponent::FPrintContext& DefaultContext, UFont* Font, const FText& InText, float& OutX, float& OutY);
	static FVector ProjectLocation(const AGameplayDebuggingHUDComponent::FPrintContext& Context, const FVector& Location);
	static void DrawItem(const AGameplayDebuggingHUDComponent::FPrintContext& DefaultContext, class FCanvasItem& Item, float X, float Y );
	static void DrawIcon(const AGameplayDebuggingHUDComponent::FPrintContext& DefaultContext, const FColor& InColor, const FCanvasIcon& Icon, float X, float Y, float Scale = 0.f);

private:
	// local player related draw from PostRender
	void DrawDebugComponentData(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent);

protected:
	bool bDrawToScreen;
	FString HugeOutputString;

	FPrintContext OverHeadContext;
	FPrintContext DefaultContext;

	void PrintString(FPrintContext& Context, const FString& InString );
	void PrintString(FPrintContext& Context, const FColor& InColor, const FString& InString );
	void PrintString(FPrintContext& Context, const FColor& InColor, const FString& InString, float X, float Y );
};
