// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UniformGridPanel.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UUniformGridPanel : public USlateNonLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

	/** The items placed on the canvas */
	UPROPERTY(EditAnywhere, EditInline, Category=Slots)
	TArray<UUniformGridSlot*> Slots;

	UUniformGridSlot* AddSlot(USlateWrapperComponent* Content);

	// USlateNonLeafWidgetComponent
	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual USlateWrapperComponent* GetChildAt(int32 Index) const OVERRIDE;
	virtual bool AddChild(USlateWrapperComponent* Child, FVector2D Position) OVERRIDE;
	virtual bool RemoveChild(USlateWrapperComponent* Child) OVERRIDE;
	virtual void ReplaceChildAt(int32 Index, USlateWrapperComponent* Child) OVERRIDE;
	// End USlateNonLeafWidgetComponent

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface

	// USlateWrapperComponent interface
	virtual void ConnectEditorData() OVERRIDE;
	// End USlateWrapperComponent interface
#endif

protected:

	TWeakPtr<class SUniformGridPanel> MyUniformGridPanel;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface
};
