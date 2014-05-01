// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWrapperTypes.h"
#include "SlateWrapperComponent.generated.h"

/** This is the base class for all Slate widget wrapper components */
UCLASS(Abstract, hideCategories=(Activation, "Components|Activation", ComponentReplication, LOD, Rendering))
class UMG_API USlateWrapperComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Variable)
	FString Name;

	UPROPERTY(EditAnywhere, Category=Variable)
	bool bIsVariable;

#if WITH_EDITOR
	UPROPERTY(Transient, EditInline, EditAnywhere, BlueprintReadWrite, Category=Layout, meta=( ShowOnlyInnerProperties ))
	UPanelSlot* Slot;
#endif

	/** Sets whether this widget can be modified interactively by the user */
	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	bool bIsEnabled;

	//TODO UMG ToolTipWidget

	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	FText ToolTipText;

	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	TEnumAsByte<ESlateVisibility::Type> Visiblity;

	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	TEnumAsByte<EMouseCursor::Type> Cursor;

	UFUNCTION(BlueprintCallable, Category="Widget")
	bool GetIsEnabled() const;

	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetIsEnabled(bool bInIsEnabled);

	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetToolTipText(const FText& InToolTipText);

	UFUNCTION(BlueprintCallable, Category="Widget")
	TEnumAsByte<ESlateVisibility::Type> GetVisibility();

	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetVisibility(TEnumAsByte<ESlateVisibility::Type> InVisibility);

	//TOptional< TEnumAsByte<EMouseCursor::Type> > GetCursor() const;

	//void SetCursor(const TAttribute< TOptional< TEnumAsByte<EMouseCursor::Type> > >& InCursor);

	// UActorComponent interface
	virtual void OnRegister() OVERRIDE;
	virtual void OnUnregister() OVERRIDE;
	// End of UActorComponent interface

	TSharedRef<SWidget> GetWidget();

#if WITH_EDITOR
	virtual void ConnectEditorData() { }
#endif

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface
#endif

	// Utility methods
	//@TODO UMG: Should move elsewhere
	static EVisibility ConvertSerializedVisibilityToRuntime(TEnumAsByte<ESlateVisibility::Type> Input);
	static TEnumAsByte<ESlateVisibility::Type> ConvertRuntimeToSerializedVisiblity(const EVisibility& Input);

	static FSizeParam ConvertSerializedSizeParamToRuntime(const FSlateChildSize& Input);

protected:
	virtual TSharedRef<SWidget> RebuildWidget();

protected:
	TSharedPtr<SWidget> MyWidget;
};
