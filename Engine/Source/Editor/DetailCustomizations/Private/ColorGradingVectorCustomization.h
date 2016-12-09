// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MathStructCustomizations.h"
#include "IDetailCustomNodeBuilder.h"

class FColorGradingVectorCustomization : public TSharedFromThis<FColorGradingVectorCustomization>
{
public:
	FColorGradingVectorCustomization();
	void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils, TArray<TWeakPtr<IPropertyHandle>> WeakChildArray);
	void OnGetData(TArray<FVector4*> &OutVector4Data, TWeakPtr<IPropertyHandle> StructPropertyHandle);
	void OnVector4DataChanged(FVector4 ColorGradingData, TWeakPtr<IPropertyHandle> StructPropertyHandle);
	TArray<FVector4*> CurrentData;
	TSharedPtr<class FColorGradingCustomBuilder> CustomColorGradingBuilder;
};

// Delegate called when we need to get the current Color Data
DECLARE_DELEGATE_TwoParams(FOnGetColorGradingData, TArray<FVector4*>&, TWeakPtr<IPropertyHandle>);
// Delegate called when the widget Color Data changed
DECLARE_DELEGATE_TwoParams(FOnColorGradingChanged, FVector4, TWeakPtr<IPropertyHandle>);

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
	FColorGradingCustomBuilder(FColorGradingCustomBuilderDelegates& ColorGradingCustomBuilderDelegates, TWeakPtr<IPropertyHandle> InColorGradingPropertyHandle, TArray< TWeakPtr<IPropertyHandle> >& InSortedChildArray);
	~FColorGradingCustomBuilder()
	{
	}

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
	void OnColorGradingPickerChanged(FVector4 &NewValue);
	void GetCurrentColorGradingValue(FVector4 &OutCurrentValue);

	void OnSliderValueChanged(float NewValue, TWeakPtr<IPropertyHandle> WeakHandlePtr);
	TOptional<float> OnSliderGetValue(TWeakPtr<IPropertyHandle> WeakHandlePtr) const;

	/** Delegates for the Section list */
	FColorGradingCustomBuilderDelegates ColorGradingDelegates;
	/** Called to rebuild the children of the detail tree */
	FSimpleDelegate OnRebuildChildren;
	/* The property we want to change */
	TWeakPtr<IPropertyHandle> ColorGradingPropertyHandle;
	// The sorted child array
	TArray< TWeakPtr<IPropertyHandle> > SortedChildArray;
};