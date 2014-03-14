// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"
#include "PropertyHelper.h"

/*-----------------------------------------------------------------------------
	UDelegateProperty.
-----------------------------------------------------------------------------*/

void UDelegateProperty::InstanceSubobjects(void* Data, void const* DefaultData, UObject* Owner, FObjectInstancingGraph* InstanceGraph )
{
	for( int32 i=0; i<ArrayDim; i++ )
	{
		FScriptDelegate& DestDelegate = ((FScriptDelegate*)Data)[i];
		UObject* CurrentValue = DestDelegate.GetObject();
		if ( CurrentValue )
		{
			UObject *Template = NULL;
			if (DefaultData)
			{
				FScriptDelegate& DefaultDelegate = ((FScriptDelegate*)DefaultData)[i];
				Template = DefaultDelegate.GetObject();
			}
			UObject* NewValue = InstanceGraph->InstancePropertyValue(Template, CurrentValue, Owner, HasAnyPropertyFlags(CPF_Transient), false, true);
			DestDelegate.SetObject(NewValue);
		}
	}
}

bool UDelegateProperty::Identical( const void* A, const void* B, uint32 PortFlags ) const
{
	bool bResult = false;

	FScriptDelegate* DA = (FScriptDelegate*)A;
	FScriptDelegate* DB = (FScriptDelegate*)B;
	
	if( DB == NULL )
	{
		bResult = DA->GetFunctionName() == NAME_None;
	}
	else if ( DA->GetFunctionName() == DB->GetFunctionName() )
	{
		if ( DA->GetObject() == DB->GetObject() )
		{
			bResult = true;
		}
		else if	((DA->GetObject() == NULL || DB->GetObject() == NULL)
			&&	(PortFlags&PPF_DeltaComparison) != 0)
		{
			bResult = true;
		}
	}

	return bResult;
}


void UDelegateProperty::SerializeItem( FArchive& Ar, void* Value, int32 MaxReadBytes, void const* Defaults ) const
{
	Ar << *GetPropertyValuePtr(Value);
}


bool UDelegateProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData ) const
{
	// Do not allow replication of delegates, as there is no way to make this secure (it allows the execution of any function in any object, on the remote client/server)
	return 1;
}


FString UDelegateProperty::GetCPPType( FString* ExtendedTypeText/*=NULL*/, uint32 CPPExportFlags/*=0*/ ) const
{
	const FString UnmangledFunctionName = SignatureFunction->GetName().LeftChop( FString( HEADER_GENERATED_DELEGATE_SIGNATURE_SUFFIX ).Len() );
	const FString DelegateName( FString( TEXT( "F" ) ) + UnmangledFunctionName );
	return DelegateName;
}


void UDelegateProperty::ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const
{
	FScriptDelegate* ScriptDelegate = (FScriptDelegate*)PropertyValue;
	check(ScriptDelegate != NULL);
	bool bDelegateHasValue = ScriptDelegate->GetFunctionName() != NAME_None;
	ValueStr += FString::Printf( TEXT("%s.%s"),
		ScriptDelegate->GetObject() != NULL ? *ScriptDelegate->GetObject()->GetName() : TEXT("(null)"),
		*ScriptDelegate->GetFunctionName().ToString() );
}


const TCHAR* UDelegateProperty::ImportText_Internal( const TCHAR* Buffer, void* PropertyValue, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText ) const
{
	return DelegatePropertyTools::ImportDelegateFromText( *(FScriptDelegate*)PropertyValue, SignatureFunction, Buffer, Parent, ErrorText );
}

void UDelegateProperty::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	Ar << SignatureFunction;
}

IMPLEMENT_CORE_INTRINSIC_CLASS(UDelegateProperty, UProperty,
	{
		Class->EmitObjectReference( STRUCT_OFFSET( UDelegateProperty, SignatureFunction ) );
	}
);
