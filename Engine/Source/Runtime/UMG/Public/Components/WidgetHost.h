// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetHost.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UWidgetHost : public USlateNonLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, EditInline, Category=Appearance)
	class UUserWidget* Widget;

	// USlateNonLeafWidgetComponent
	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual USlateWrapperComponent* GetChildAt(int32 Index) const OVERRIDE;
	//virtual bool AddChild(USlateWrapperComponent* Child, FVector2D Position) OVERRIDE;
	//virtual bool RemoveChild(USlateWrapperComponent* Child) OVERRIDE;
	//virtual void ReplaceChildAt(int32 Index, USlateWrapperComponent* Child) OVERRIDE;
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
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface
};
