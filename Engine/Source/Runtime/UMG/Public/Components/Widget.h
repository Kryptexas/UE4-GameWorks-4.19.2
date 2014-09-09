// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWrapperTypes.h"
#include "WidgetTransform.h"

#include "Widget.generated.h"

/**
 * Helper macro for binding to a delegate or using the constant value when constructing the underlying SWidget
 */
#define OPTIONAL_BINDING(ReturnType, MemberName) ( MemberName ## Delegate.IsBound() && !IsDesignTime() ) ? TAttribute< ReturnType >::Create(MemberName ## Delegate.GetUObject(), MemberName ## Delegate.GetFunctionName()) : TAttribute< ReturnType >(MemberName)

/**
 * Helper macro for binding to a delegate or using the constant value when constructing the underlying SWidget,
 * also allows a conversion function to be provided to convert between the SWidget value and the value exposed to UMG.
 */
#define OPTIONAL_BINDING_CONVERT(ReturnType, MemberName, ConvertedType, ConversionFunction) ( MemberName ## Delegate.IsBound() && !IsDesignTime() ) ? TAttribute< ConvertedType >::Create(TAttribute< ConvertedType >::FGetter::CreateUObject(this, &ThisClass::ConversionFunction, TAttribute< ReturnType >::Create(MemberName ## Delegate.GetUObject(), MemberName ## Delegate.GetFunctionName()))) : ConversionFunction(TAttribute< ReturnType >(MemberName))

/**
 * This is the base class for all wrapped Slate controls that are exposed to UMG.
 */
UCLASS(Abstract)
class UMG_API UWidget : public UVisual
{
	GENERATED_UCLASS_BODY()

public:

	// Common Bindings
	DECLARE_DYNAMIC_DELEGATE_RetVal(bool, FGetBool);
	DECLARE_DYNAMIC_DELEGATE_RetVal(float, FGetFloat);
	DECLARE_DYNAMIC_DELEGATE_RetVal(int32, FGetInt32);
	DECLARE_DYNAMIC_DELEGATE_RetVal(FText, FGetText);
	DECLARE_DYNAMIC_DELEGATE_RetVal(FVector2D, FGetVector2D);
	DECLARE_DYNAMIC_DELEGATE_RetVal(FVector, FGetVector);
	//DECLARE_DYNAMIC_DELEGATE_RetVal(FVector4, FGetVector4);
	DECLARE_DYNAMIC_DELEGATE_RetVal(FMargin, FGetMargin);
	DECLARE_DYNAMIC_DELEGATE_RetVal(FLinearColor, FGetSlateColor);
	DECLARE_DYNAMIC_DELEGATE_RetVal(FLinearColor, FGetLinearColor);
	DECLARE_DYNAMIC_DELEGATE_RetVal(ESlateVisibility::Type, FGetSlateVisibility);
	DECLARE_DYNAMIC_DELEGATE_RetVal(EMouseCursor::Type, FGetMouseCursor);
	DECLARE_DYNAMIC_DELEGATE_RetVal(USlateBrushAsset*, FGetSlateBrushAsset);
	DECLARE_DYNAMIC_DELEGATE_RetVal(FSlateBrush, FGetSlateBrush);
	DECLARE_DYNAMIC_DELEGATE_RetVal(ESlateCheckBoxState::Type, FGetCheckBoxState);
	DECLARE_DYNAMIC_DELEGATE_RetVal(UWidget*, FGetContent);

	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(UWidget*, FGenerateWidgetForString, FString, Item);
	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(UWidget*, FGenerateWidgetForObject, UObject*, Item);

	// Events
	DECLARE_DYNAMIC_DELEGATE_RetVal(FEventReply, FOnReply);
	DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FEventReply, FOnPointerEvent, FGeometry, MyGeometry, const FPointerEvent&, MouseEvent);

	/**
	 * Allows controls to be exposed as variables in a blueprint.  Not all controls need to be exposed
	 * as variables, so this allows only the most useful ones to end up being exposed.
	 */
	UPROPERTY()
	bool bIsVariable;

	/** Flag if the Widget was created from a blueprint */
	UPROPERTY(Transient)
	bool bCreatedByConstructionScript;

	/**
	 * The parent slot of the UWidget.  Allows us to easily inline edit the layout controlling this widget.
	 */
	UPROPERTY(Instanced, EditAnywhere, BlueprintReadOnly, Category = Layout, meta = (ShowOnlyInnerProperties))
	UPanelSlot* Slot;

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
	UPROPERTY(EditDefaultsOnly, Category=Behavior, AdvancedDisplay)
	TEnumAsByte<EMouseCursor::Type> Cursor;

	/** A bindable delegate for Cursor */
	UPROPERTY()
	FGetMouseCursor CursorDelegate;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category=RenderTransform, meta=(DisplayName="Transform"))
	FWidgetTransform RenderTransform;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category=RenderTransform, meta=(DisplayName="Pivot"))
	FVector2D RenderTransformPivot;

	/** */
	UFUNCTION(BlueprintCallable, Category="Widget|Transform")
	void SetRenderTransform(FWidgetTransform InTransform);

	/** */
	UFUNCTION(BlueprintCallable, Category="Widget|Transform")
	void SetRenderScale(FVector2D Scale);

	/** */
	UFUNCTION(BlueprintCallable, Category="Widget|Transform")
	void SetRenderShear(FVector2D Shear);

	/** */
	UFUNCTION(BlueprintCallable, Category="Widget|Transform")
	void SetRenderAngle(float Angle);
	
	/** */
	UFUNCTION(BlueprintCallable, Category="Widget|Transform")
	void SetRenderTranslation(FVector2D Translation);

	/** */
	UFUNCTION(BlueprintCallable, Category="Widget|Transform")
	void SetRenderTransformPivot(FVector2D Pivot);

	/** Gets the current enabled status of the widget */
	UFUNCTION(BlueprintCallable, Category="Widget")
	bool GetIsEnabled() const;

	/** Sets the current enabled status of the widget */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetIsEnabled(bool bInIsEnabled);

	/** Sets the tooltip text for the widget. */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetToolTipText(const FText& InToolTipText);

	/** Gets the current visibility of the widget. */
	UFUNCTION(BlueprintCallable, Category="Widget")
	TEnumAsByte<ESlateVisibility::Type> GetVisibility();

	/** Sets the visibility of the widget. */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetVisibility(TEnumAsByte<ESlateVisibility::Type> InVisibility);

	/** Gets if the button is currently being hovered by the mouse */
	UFUNCTION(BlueprintCallable, Category="Widget")
	bool IsHovered() const;

	/**
	 * Checks to see if this widget currently has the keyboard focus
	 *
	 * @return  True if this widget has keyboard focus
	 */
	UFUNCTION(BlueprintCallable, Category="Widget")
	bool HasKeyboardFocus() const;

	/**
	 * @return Whether this widget has any descendants with keyboard focus
	 */
	UFUNCTION(BlueprintCallable, Category="Widget")
	bool HasFocusedDescendants() const;

	/**
	 * Checks to see if this widget is the current mouse captor
	 *
	 * @return  True if this widget has captured the mouse
	 */
	UFUNCTION(BlueprintCallable, Category="Widget")
	bool HasMouseCapture() const;

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetKeyboardFocus() const;

	/** Forces the underlying slate system to perform a pre-pass on the layout of the widget.  This is for advanced users. */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void ForceLayoutPrepass();

	/** Gets the parent widget */
	UFUNCTION(BlueprintCallable, Category="Widget")
	class UPanelWidget* GetParent() const;

	/** Removes the widget from it's parent widget */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void RemoveFromParent();

	/**
	 * Gets the underlying slate widget or constructs it if it doesn't exist.  This function is
	 * virtual however, you should not inherit this function unless you're very aware of what you're
	 * doing.  Normal derived versions should only ever override RebuildWidget.
	 */
	TSharedRef<SWidget> TakeWidget();

	/** Gets the last created widget does not recreate the gc container for the widget if one is needed. */
	TSharedPtr<SWidget> GetCachedWidget() const;

	/**
	 * Applies all properties to the native widget if possible.  This is called after a widget is constructed.
	 * It can also be called by the editor to update modified state, so ensure all initialization to a widgets
	 * properties are performed here, or the property and visual state may become unsynced.
	 */
	virtual void SynchronizeProperties();

	/** Returns if the widget is currently being displayed in the designer, it may want to display different data. */
	bool IsDesignTime() const;
	
	/** Sets that this widget is being designed */
	void IsDesignTime(bool bInDesignTime);

	/** Mark this object as modified, also mark the slot as modified. */
	virtual bool Modify(bool bAlwaysMarkDirty = true);

	/** @return true if this widget is a child of the PossibleParent */
	bool IsChildOf(UWidget* PossibleParent);
	
#if WITH_EDITOR
	/** Is the label generated or provided by the user? */
	bool IsGeneratedName() const;

	/** Get Label Metadata, which may be as simple as a bit of string data to help identify an anonymous text block. */
	virtual FString GetLabelMetadata() const;

	/** Gets the label to display to the user for this widget. */
	FString GetLabel() const;

	/** Gets the toolbox category of the widget */
	virtual const FText GetToolboxCategory();

	/** Gets the editor icon */
	virtual const FSlateBrush* GetEditorIcon();
	
	/** Allows general fixups and connections only used at editor time. */
	virtual void ConnectEditorData() { }

	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface

	// Begin Designer contextual events
	void Select();
	void Deselect();

	virtual void OnSelected() { }
	virtual void OnDeselected() { }

	virtual void OnDescendantSelected(UWidget* DescendantWidget) { }
	virtual void OnDescendantDeselected(UWidget* DescendantWidget) { }

	virtual void OnBeginEdit() { }
	virtual void OnEndEdit() { }
	// End Designer contextual events
#endif

	// Utility methods
	//@TODO UMG: Should move elsewhere
	static EVisibility ConvertSerializedVisibilityToRuntime(TEnumAsByte<ESlateVisibility::Type> Input);
	static TEnumAsByte<ESlateVisibility::Type> ConvertRuntimeToSerializedVisiblity(const EVisibility& Input);

	static FSizeParam ConvertSerializedSizeParamToRuntime(const FSlateChildSize& Input);

	static void GatherChildren(UWidget* Root, TSet<UWidget*>& Children);
	static void GatherAllChildren(UWidget* Root, TSet<UWidget*>& Children);
	static UWidget* FindChildContainingDescendant(UWidget* Root, UWidget* Descendant);

protected:
	/** Function implemented by all subclasses of UWidget is called when the underlying SWidget needs to be constructed. */
	virtual TSharedRef<SWidget> RebuildWidget();
	
	TSharedRef<SWidget> BuildDesignTimeWidget(TSharedRef<SWidget> WrapWidget);

	void UpdateRenderTransform();

protected:
	//TODO UMG Consider moving conversion functions into another class.
	// Conversion functions
	EVisibility ConvertVisibility(TAttribute<ESlateVisibility::Type> SerializedType) const
	{
		return ConvertSerializedVisibilityToRuntime(SerializedType.Get());
	}

	TOptional<float> ConvertFloatToOptionalFloat(TAttribute<float> InFloat) const
	{
		return InFloat.Get();
	}

	FSlateColor ConvertLinearColorToSlateColor(TAttribute<FLinearColor> InLinearColor) const
	{
		return FSlateColor(InLinearColor.Get());
	}

protected:
	/** The underlying SWidget. */
	TWeakPtr<SWidget> MyWidget;

	/** The underlying SWidget contained in a SObjectWidget */
	TWeakPtr<SWidget> MyGCWidget;
	
	/** Is this widget being displayed on a designer surface */
	bool bDesignTime;
};
