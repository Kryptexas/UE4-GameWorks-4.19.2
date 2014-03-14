// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FDialogueContextStructCustomization : public IStructCustomization
{
public:
	/** @return A new instance of this class */
	static TSharedRef<IStructCustomization> MakeInstance();

	/** IStructCustomization interface begin */
	virtual void CustomizeStructHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeStructChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	/** IStructCustomization interface end */
};

class FDialogueWaveParameterStructCustomization : public IStructCustomization
{
public:
	/** @return A new instance of this class */
	static TSharedRef<IStructCustomization> MakeInstance();

	/** IStructCustomization interface begin */
	virtual void CustomizeStructHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeStructChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	/** IStructCustomization interface end */
};