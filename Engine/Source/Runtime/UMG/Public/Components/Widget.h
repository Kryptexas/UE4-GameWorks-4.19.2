// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWrapperTypes.h"

#include "Widget.generated.h"

/**
 * Helper macro for binding to a delegate or using the constant value when constructing the underlying SWidget
 */
#define OPTIONAL_BINDING(ReturnType, MemberName) MemberName ## Delegate.IsBound() ? TAttribute< ReturnType >::Create(MemberName ## Delegate.GetUObject(), MemberName ## Delegate.GetFunctionName()) : TAttribute< ReturnType >(MemberName)

/**
 * Helper macro for binding to a delegate or using the constant value when constructing the underlying SWidget,
 * also allows a conversion function to be provided to convert between the SWidget value and the value exposed to UMG.
 */
#define OPTIONAL_BINDING_CONVERT(ReturnType, MemberName, ConvertedType, ConversionFunction) MemberName ## Delegate.IsBound() ? TAttribute< ConvertedType >::Create(TAttribute< ConvertedType >::FGetter::CreateUObject(this, &ThisClass::ConversionFunction, TAttribute< ReturnType >::Create(MemberName ## Delegate.GetUObject(), MemberName ## Delegate.GetFunctionName()))) : ConversionFunction(TAttribute< ReturnType >(MemberName))

/**
 * This is the base class for all wrapped Slate controls that are exposed to UMG.
 */
UCLASS(Abstract)
class UMG_API UWidget : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	// Common Bindings
	DECLARE_DYNAMIC_DELEGATE_RetVal(bool, FGetBool);
	DECLARE_DYNAMIC_DELEGATE_RetVal(FText, FGetText);
	DECLARE_DYNAMIC_DELEGATE_RetVal(FLinearColor, FGetSlateColor);
	DECLARE_DYNAMIC_DELEGATE_RetVal(FLinearColor, FGetLinearColor);
	DECLARE_DYNAMIC_DELEGATE_RetVal(ESlateVisibility::Type, FGetSlateVisibility);
	DECLARE_DYNAMIC_DELEGATE_RetVal(EMouseCursor::Type, FGetMouseCursor);

	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(UWidget*, FGenerateWidgetUObject, UObject*, Item);

	// Events
	DECLARE_DYNAMIC_DELEGATE_RetVal(FSReply, FOnReply);

	/**
	 * Allows controls to be exposed as variables in a blueprint.  Not all controls need to be exposed
	 * as variables, so this allows only the most useful ones to end up being exposed.
	 */
	UPROPERTY(EditDefaultsOnly, Category=Variable)
	bool bIsVariable;

	/** Flag if the Widget was created from a blueprint */
	UPROPERTY(Transient)
	bool bCreatedByConstructionScript;

#if WITH_EDITOR
	/**
	 * The parent slot of the UWidget.  Allows us to easily inline edit the layout controlling this widget.
	 */
	UPROPERTY(Transient, EditInline, EditDefaultsOnly, BlueprintReadWrite, Category=Layout, meta=( ShowOnlyInnerProperties ))
	UPanelSlot* Slot;
#endif

	/** Sets whether this widget can be modified interactively by the user */
	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	bool bIsEnabled;

	/** A bindable delegate for bIsEnabled */
	UPROPERTY()
	FGetBool bIsEnabledDelegate;

	//TODO UMG ToolTipWidget

	/** Tooltip text to show when the user hovers over the widget with the mouse */
	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	FText ToolTipText;

	/** A bindable delegate for ToolTipText */
	UPROPERTY()
	FGetText ToolTipTextDelegate;

	/** The visibility of the widget */
	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	TEnumAsByte<ESlateVisibility::Type> Visiblity;

	/** A bindable delegate for Visibility */
	UPROPERTY()
	FGetSlateVisibility VisiblityDelegate;

	//TODO UMG Cursor doesn't work yet, the underlying slate version needs it to be TOptional.

	/** The cursor to show when the mouse is over the widget */
	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	TEnumAsByte<EMouseCursor::Type> Cursor;

	/** A bindable delegate for Cursor */
	UPROPERTY()
	FGetMouseCursor CursorDelegate;

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

	TSharedRef<SWidget> GetWidget();

#if WITH_EDITOR
	virtual void ConnectEditorData() { }

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

	EVisibility ConvertVisibility(TAttribute<ESlateVisibility::Type> SerializedType) const
	{
		return ConvertSerializedVisibilityToRuntime(SerializedType.Get());
	}

protected:
	TSharedPtr<SWidget> MyWidget;
};
