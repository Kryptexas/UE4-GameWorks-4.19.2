// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MathStructCustomizations.h"
#include "IDetailCustomNodeBuilder.h"

class FVector4StructCustomization;
class SColorGradingPicker;

class FColorGradingVectorCustomization : public TSharedFromThis<FColorGradingVectorCustomization>
{
public:
	FColorGradingVectorCustomization();
	void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils, const TArray<TWeakPtr<IPropertyHandle>>& WeakChildArray, TSharedRef<FVector4StructCustomization> InVector4Customization);
	void OnGetData(TArray<FVector4*> &OutVector4Data, TWeakPtr<IPropertyHandle> StructPropertyHandle);
	void OnVector4DataChanged(FVector4 ColorGradingData, TWeakPtr<IPropertyHandle> StructPropertyHandle, bool ShouldCommitValueChanges);
	TArray<FVector4*> CurrentData;
	TSharedPtr<class FColorGradingCustomBuilder> CustomColorGradingBuilder;
};

// Delegate called when we need to get the current Color Data
DECLARE_DELEGATE_TwoParams(FOnGetColorGradingData, TArray<FVector4*>&, TWeakPtr<IPropertyHandle>);
// Delegate called when the widget Color Data changed
DECLARE_DELEGATE_ThreeParams(FOnColorGradingChanged, FVector4, TWeakPtr<IPropertyHandle>, bool);

struct FColorGradingCustomBuilderDelegates
{
	FColorGradingCustomBuilderDelegates()
		: OnGetColorGradingData()
		, OnColorGradingChanged()
	{}
	// Delegate called when we need to get the current Color Data
	FOnGetColorGradingData OnGetColorGradingData;
	// Delegate called when the widget Color Data changed
	FOnColorGradingChanged OnColorGradingChanged;
};

class FColorGradingCustomBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FColorGradingCustomBuilder>
{
public:
	/** Notification when the max/min spinner values are changed (only apply if SupportDynamicSliderMaxValue or SupportDynamicSliderMinValue are true) */
	DECLARE_MULTICAST_DELEGATE_FourParams(FOnNumericEntryBoxDynamicSliderMaxValueChanged, float, TWeakPtr<SWidget>, bool, bool);
	DECLARE_MULTICAST_DELEGATE_FourParams(FOnNumericEntryBoxDynamicSliderMinValueChanged, float, TWeakPtr<SWidget>, bool, bool);

	FColorGradingCustomBuilder(FColorGradingCustomBuilderDelegates& ColorGradingCustomBuilderDelegates, TWeakPtr<IPropertyHandle> InColorGradingPropertyHandle, const TArray<TWeakPtr<IPropertyHandle>>& InSortedChildArray, TSharedRef<FVector4StructCustomization> InVector4Customization);
	virtual ~FColorGradingCustomBuilder();

private:

	/** IDetailCustomNodeBuilder interface */
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRebuildChildren) override { OnRebuildChildren = InOnRebuildChildren; }
	virtual bool RequiresTick() const override { return false; }
	virtual void Tick(float DeltaTime) override;
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual FName GetName() const override { return NAME_None; }
	virtual bool InitiallyCollapsed() const override { return false; }

	/* Local UI Handlers*/
	void OnColorGradingPickerChanged(FVector4 &NewValue, bool ShouldCommitValueChanges);
	void GetCurrentColorGradingValue(FVector4 &OutCurrentValue);

	void OnSliderValueChanged(float NewValue, TWeakPtr<IPropertyHandle> WeakHandlePtr, bool ShouldCommitValueChanges);
	TOptional<float> OnSliderGetValue(TWeakPtr<IPropertyHandle> WeakHandlePtr) const;

	/** Callback when the max/min spinner value are changed (only apply if SupportDynamicSliderMaxValue or SupportDynamicSliderMinValue are true) */
	void OnDynamicSliderMaxValueChanged(float NewMaxSliderValue, TWeakPtr<SWidget> InValueChangedSourceWidget, bool IsOriginator, bool UpdateOnlyIfHigher);
	void OnDynamicSliderMinValueChanged(float NewMinSliderValue, TWeakPtr<SWidget> InValueChangedSourceWidget, bool IsOriginator, bool UpdateOnlyIfLower);

	/** Delegates for the Section list */
	FColorGradingCustomBuilderDelegates ColorGradingDelegates;
	/** Called to rebuild the children of the detail tree */
	FSimpleDelegate OnRebuildChildren;
	/* The property we want to change */
	TWeakPtr<IPropertyHandle> ColorGradingPropertyHandle;
	// The sorted child array
	TArray< TWeakPtr<IPropertyHandle> > SortedChildArray;

	/** Parent of this custom builder (required to communicate with MathStruct builder) */
	TSharedRef<FVector4StructCustomization> Vector4Customization;
	
	/** All created numeric entry box widget for this customization */
	TArray<TWeakPtr<SWidget>> NumericEntryBoxWidgetList;
	TWeakPtr<SColorGradingPicker> ColorGradingPickerWidget;

	FOnNumericEntryBoxDynamicSliderMaxValueChanged OnNumericEntryBoxDynamicSliderMaxValueChanged;
	FOnNumericEntryBoxDynamicSliderMinValueChanged OnNumericEntryBoxDynamicSliderMinValueChanged;
};