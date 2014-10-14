// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FWidgetNavigationCustomization : public IPropertyTypeCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<class IPropertyTypeCustomization> MakeInstance(UBlueprint* Blueprint)
	{
		return MakeShareable(new FWidgetNavigationCustomization(Blueprint));
	}

	FWidgetNavigationCustomization(UBlueprint* InBlueprint)
		: Blueprint(Cast<UWidgetBlueprint>(InBlueprint))
	{
	}
	
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	
private:
	void FillOutChildren(TSharedRef<IPropertyHandle> PropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils);

	FReply OnCustomizeNavigation(TWeakPtr<IPropertyHandle> PropertyHandle);

private:
	UWidgetBlueprint* Blueprint;
};
