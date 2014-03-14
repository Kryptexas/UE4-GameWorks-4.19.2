// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ObjectBase.h"

// need to break this out a different type so that the DECLARE_CASTED_CLASS_INTRINSIC macro can digest the comma
typedef TProperty<FText, UProperty> UTextProperty_Super;

class COREUOBJECT_API UTextProperty : public UTextProperty_Super
{
	DECLARE_CASTED_CLASS_INTRINSIC(UTextProperty, UTextProperty_Super, 0, CoreUObject, CASTCLASS_UTextProperty)

public:

	typedef UTextProperty_Super::TTypeFundamentals TTypeFundamentals;
	typedef TTypeFundamentals::TCppType TCppType;

	UTextProperty( const class FPostConstructInitializeProperties& PCIP, ECppProperty, int32 InOffset, uint64 InFlags )
		:	TProperty( PCIP, EC_CppProperty, InOffset, InFlags)
	{
	}

	// UProperty interface
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const OVERRIDE;
	virtual void SerializeItem( FArchive& Ar, void* Value, int32 MaxReadBytes, void const* Defaults ) const OVERRIDE;
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const OVERRIDE;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const OVERRIDE;
	// End of UProperty interface
};