// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"
#include "PropertyHelper.h"

void UAttributeProperty::LinkInternal(FArchive& Ar)
{
	ULinkerLoad* MyLinker = GetLinker();
	if( MyLinker )
	{
		MyLinker->Preload(this);
	}
	Ar.Preload(Inner);
	Inner->Link(Ar);
}

bool UAttributeProperty::Identical( const void* A, const void* B, uint32 PortFlags ) const
{
	checkSlow(Inner);

	return Inner->Identical( A, B, PortFlags );
}

void UAttributeProperty::SerializeItem( FArchive& Ar, void* Value, int32 MaxReadBytes, void const* Defaults ) const
{
	checkSlow(Inner);

	// Ensure that the Inner itself has been loaded before calling SerializeItem() on it
	Ar.Preload(Inner);

	Inner->SerializeItem( Ar, Value, MaxReadBytes, Defaults);
}

bool UAttributeProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData ) const
{
	UE_LOG( LogProperty, Fatal, TEXT( "Deprecated code path" ) );
	return 1;
}

void UAttributeProperty::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	Ar << Inner;
	checkSlow(Inner||HasAnyFlags(RF_ClassDefaultObject));
}

void UAttributeProperty::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UAttributeProperty* This = CastChecked<UAttributeProperty>(InThis);
	Collector.AddReferencedObject( This->Inner, This );
	Super::AddReferencedObjects( This, Collector );
}

FString UAttributeProperty::GetCPPType( FString* ExtendedTypeText/*=NULL*/, uint32 CPPExportFlags/*=0*/ ) const
{
	checkSlow(Inner);

	if ( ExtendedTypeText != NULL )
	{
		FString InnerExtendedTypeText;
		FString InnerTypeText = Inner->GetCPPType(&InnerExtendedTypeText, CPPExportFlags & ~CPPF_ArgumentOrReturnValue); // we won't consider array inners to be "arguments or return values"
		if ( InnerExtendedTypeText.Len() && InnerExtendedTypeText.Right(1) == TEXT(">") )
		{
			// if our internal property type is a template class, add a space between the closing brackets b/c VS.NET cannot parse this correctly
			InnerExtendedTypeText += TEXT(" ");
		}
		else if ( !InnerExtendedTypeText.Len() && InnerTypeText.Len() && InnerTypeText.Right(1) == TEXT(">") )
		{
			// if our internal property type is a template class, add a space between the closing brackets b/c VS.NET cannot parse this correctly
			InnerExtendedTypeText += TEXT(" ");
		}
		*ExtendedTypeText = FString::Printf(TEXT("<%s%s>"), *InnerTypeText, *InnerExtendedTypeText);
	}
	return TEXT("TAttribute");
}

FString UAttributeProperty::GetCPPMacroType( FString& ExtendedTypeText ) const
{
	checkSlow(Inner);
	ExtendedTypeText = Inner->GetCPPType();
	return TEXT("TATTRIBUTE");
}

void UAttributeProperty::ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const
{
	checkSlow(Inner);
	Inner->ExportTextItem( ValueStr, PropertyValue, DefaultValue, Parent, PortFlags, ExportRootScope );
}

const TCHAR* UAttributeProperty::ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText ) const
{
	checkSlow(Inner);
	Inner->ImportText( Buffer, Data, PortFlags, Parent, ErrorText );

	return Buffer;
}

void UAttributeProperty::CopyValuesInternal(void* Dest, void const* Src, int32 Count) const
{
	checkSlow(Inner);
	Inner->CopyCompleteValue(Dest, Src);
}

void UAttributeProperty::ClearValueInternal(void* Data) const 
{
	checkSlow(Inner);
	Inner->ClearValue(Data);
}

void UAttributeProperty::DestroyValueInternal(void* Dest) const
{
	checkSlow(Inner);
	Inner->DestroyValue(Dest);
}

void UAttributeProperty::AddCppProperty( UProperty* Property )
{
	check(!Inner);
	check(Property);

	Inner = Property;
}

bool UAttributeProperty::IsLocalized() const
{
	return Inner->IsLocalized() ? true : Super::IsLocalized();
}

bool UAttributeProperty::PassCPPArgsByRef() const
{
	return true;
}

void UAttributeProperty::InstanceSubobjects( void* Data, void const* DefaultData, UObject* Owner, FObjectInstancingGraph* InstanceGraph )
{
	check(Inner)
	Inner->InstanceSubobjects( Data, DefaultData, Owner, InstanceGraph );
}

bool UAttributeProperty::ContainsObjectReference() const
{
	check(Inner);
	return Inner->ContainsObjectReference();
}

bool UAttributeProperty::ContainsWeakObjectReference() const
{
	check(Inner);
	return Inner->ContainsWeakObjectReference();
}

void UAttributeProperty::EmitReferenceInfo(FGCReferenceTokenStream* ReferenceTokenStream, int32 BaseOffset)
{
	check(Inner);
	Inner->EmitReferenceInfo(ReferenceTokenStream, BaseOffset);
}

bool UAttributeProperty::SameType(const UProperty* Other) const
{
	return Super::SameType(Other) && Inner && Inner->SameType(((UAttributeProperty*)Other)->Inner);
}

IMPLEMENT_CORE_INTRINSIC_CLASS(UAttributeProperty, UProperty,
	{
		Class->EmitObjectReference( STRUCT_OFFSET( UAttributeProperty, Inner ) );
	}
);
