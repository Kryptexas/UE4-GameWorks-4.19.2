// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FCustomChildrenBuilder : public IDetailChildrenBuilder
{
public:
	FCustomChildrenBuilder( TSharedRef<FDetailCategoryImpl> InParentCategory )
		: ParentCategory( InParentCategory )
	{}
	
	virtual IDetailChildrenBuilder& AddChildCustomBuilder( TSharedRef<class IDetailCustomNodeBuilder> InCustomBuilder ) OVERRIDE;
	virtual IDetailGroup& AddChildGroup( FName GroupName, const FString& LocalizedDisplayName ) OVERRIDE;
	virtual FDetailWidgetRow& AddChildContent( const FString& SearchString ) OVERRIDE;
	virtual IDetailPropertyRow& AddChildProperty( TSharedRef<IPropertyHandle> PropertyHandle ) OVERRIDE;
	virtual TSharedRef<SWidget> GenerateStructValueWidget( TSharedRef<IPropertyHandle> StructPropertyHandle );

	const TArray< FDetailLayoutCustomization >& GetChildCustomizations() const { return ChildCustomizations; }
private:
	TArray< FDetailLayoutCustomization > ChildCustomizations;
	TWeakPtr<FDetailCategoryImpl> ParentCategory;
};