// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FCanvasSlotCustomization : public IPropertyTypeCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<class IPropertyTypeCustomization> MakeInstance(UBlueprint* Blueprint)
	{
		return MakeShareable(new FCanvasSlotCustomization(Blueprint));
	}

	FCanvasSlotCustomization(UBlueprint* InBlueprint)
		: Blueprint(CastChecked<UWidgetBlueprint>(InBlueprint))
	{
	}
	
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) OVERRIDE;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) OVERRIDE;
	
private:

	void CustomizeAnchors(TSharedRef<IPropertyHandle> PropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils);
	void FillOutChildren(TSharedRef<IPropertyHandle> PropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils);
	
	FReply OnAnchorClicked(TSharedRef<IPropertyHandle> AnchorsHandle, FAnchors Anchors);

private:
	UWidgetBlueprint* Blueprint;
	
};
