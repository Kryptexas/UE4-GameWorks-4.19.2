// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWrapperTypes.h"
#include "SlateWrapperComponent.generated.h"

/** This is the base class for all Slate widget wrapper components */
UCLASS(Abstract, hideCategories=(Activation, "Components|Activation", ComponentReplication, LOD, Rendering))
class UMG_API USlateWrapperComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	UPROPERTY(Transient, EditInline, EditAnywhere, BlueprintReadWrite, Category=Layout)
	UPanelSlot* Slot;
#endif

	// UActorComponent interface
	virtual void OnRegister() OVERRIDE;
	virtual void OnUnregister() OVERRIDE;
	// End of UActorComponent interface

	TSharedRef<SWidget> GetWidget();

#if WITH_EDITOR
	virtual void ConnectEditorData() { }
#endif

	// Utility methods
	//@TODO: Should move elsewhere
	static EVisibility ConvertSerializedVisibilityToRuntime(TEnumAsByte<ESlateVisibility::Type> Input);
	static FSizeParam ConvertSerializedSizeParamToRuntime(const FSlateChildSize& Input);

protected:
	virtual TSharedRef<SWidget> RebuildWidget();

protected:
	TSharedPtr<SWidget> MyWidget;
};
