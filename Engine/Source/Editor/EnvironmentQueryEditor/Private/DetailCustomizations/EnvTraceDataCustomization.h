// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"

class FEnvTraceDataCustomization : public IStructCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IStructCustomization> MakeInstance();

	// Begin IStructCustomization interface
	virtual void CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	// End IStructCustomization interface

protected:

	void CacheTraceModes(TSharedRef<class IPropertyHandle> StructPropertyHandle);

	/** cached names of trace modes */
	struct FStringIntPair
	{
		FString Str;
		int32 Int;

		FStringIntPair() {}
		FStringIntPair(FString InStr, int32 InInt) : Str(InStr), Int(InInt) {}
	};
	TArray<FStringIntPair> TraceModes;
	bool bCanShowProjection;
	uint8 ActiveMode;

	TSharedPtr<IPropertyHandle> PropTraceMode;
	TSharedPtr<IPropertyHandle> PropTraceShape;

	void OnTraceModeChanged(int32 Index);
	TSharedRef<SWidget> OnGetTraceModeContent();
	FString GetCurrentTraceModeDesc() const;
	FString GetShortDescription() const;

	EVisibility GetGeometryVisibility() const;
	EVisibility GetNavigationVisibility() const;
	EVisibility GetProjectionVisibility() const;
	EVisibility GetExtentX() const;
	EVisibility GetExtentY() const;
	EVisibility GetExtentZ() const;
};
