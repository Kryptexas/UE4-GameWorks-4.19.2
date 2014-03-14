// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "json.h"

DEFINE_LOG_CATEGORY(LogDataTable);

ENGINE_API const FString FDataTableRowHandle::Unknown(TEXT("UNKNOWN"));
ENGINE_API const FString FDataTableCategoryHandle::Unknown(TEXT("UNKNOWN"));

/** Util that removes invalid chars and then make an FName */
FName MakeValidName(const FString& InString)
{
	FString InvalidChars(INVALID_NAME_CHARACTERS);

	FString FixedString;
	TArray<TCHAR>& FixedCharArray = FixedString.GetCharArray();

	// Iterate over input string characters
	for(int32 CharIdx=0; CharIdx<InString.Len(); CharIdx++)
	{
		// See if this char occurs in the InvalidChars string
		FString Char = InString.Mid( CharIdx, 1 );
		if( !InvalidChars.Contains(Char) )
		{
			// Its ok, add to result
			FixedCharArray.Add(Char[0]);
		}
	}
	FixedCharArray.Add(0);

	return FName(*FixedString);
}

/** Util to see if this property is supported in a row struct. */
static bool IsSupportedTableProperty(const UProperty* InProp)
{
	return(	InProp->IsA(UIntProperty::StaticClass()) || 
			InProp->IsA(UFloatProperty::StaticClass()) ||
			InProp->IsA(UNameProperty::StaticClass()) ||
			InProp->IsA(UStrProperty::StaticClass()) ||
			InProp->IsA(UBoolProperty::StaticClass()) ||
			InProp->IsA(UObjectPropertyBase::StaticClass()) ||
			InProp->IsA(UStructProperty::StaticClass()) ||
			InProp->IsA(UByteProperty::StaticClass()) ||
			InProp->IsA(UTextProperty::StaticClass()));
}



/** Util to assign a value (given as a string) to a struct property. */
static FString AssignStringToProperty(const FString& InString, const UProperty* InProp, uint8* InData)
{
	FStringOutputDevice ImportError;
	if(InProp != NULL && IsSupportedTableProperty(InProp))
	{
		InProp->ImportText(*InString, InProp->ContainerPtrToValuePtr<uint8>(InData), PPF_None, NULL, &ImportError);
	}

	FString Error = ImportError;
	return Error;
}

/** Util to assign get a property as a string */
static FString GetPropertyValueAsString(const UProperty* InProp, uint8* InData)
{
	FString Result(TEXT(""));

	if(InProp != NULL && IsSupportedTableProperty(InProp))
	{
		InProp->ExportText_InContainer(0, Result, InData, InData, NULL, PPF_None);
	}

	return Result;
}

/** Util to get all property names from a struct */
TArray<FName> GetStructPropertyNames(UStruct* InStruct)
{
	TArray<FName> PropNames;
	for (TFieldIterator<UProperty> It(InStruct); It; ++It)
	{
		PropNames.Add(It->GetFName());
	}
	return PropNames;
}


//////////////////////////////////////////////////////////////////////////

UDataTable::UDataTable(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UDataTable::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar); // When loading, this should load our RowStruct!	

	if(Ar.IsLoading())
	{
		UScriptStruct* LoadUsingStruct = RowStruct;
		if(LoadUsingStruct == NULL)
		{
			UE_LOG(LogDataTable, Error, TEXT("Missing RowStruct while loading DataTable '%s'!"), *GetPathName());
			LoadUsingStruct = FTableRowBase::StaticStruct();
		}

		int32 NumRows;
		Ar << NumRows;

		for(int32 RowIdx=0; RowIdx<NumRows; RowIdx++)
		{
			// Load row name
			FName RowName;
			Ar << RowName;

			// Load row data
			uint8* RowData = (uint8*)FMemory::Malloc(LoadUsingStruct->PropertiesSize);
			LoadUsingStruct->InitializeScriptStruct(RowData);
			// And be sure to call DestroyScriptStruct later
			LoadUsingStruct->SerializeTaggedProperties(Ar, RowData, LoadUsingStruct, NULL);

			// Add to map
			RowMap.Add(RowName, RowData);
		}
	}
	else if(Ar.IsSaving())
	{
		// Don't even try to save rows if no RowStruct
		if(RowStruct != NULL)
		{
			int32 NumRows = RowMap.Num();
			Ar << NumRows;

			// Now iterate over rows in the map
			for ( auto RowIt = RowMap.CreateIterator(); RowIt; ++RowIt )
			{
				// Save out name
				FName RowName = RowIt.Key();
				Ar << RowName;

				// Save out data
				uint8* RowData = RowIt.Value();
				RowStruct->SerializeTaggedProperties(Ar, RowData, RowStruct, NULL);
			}
		}
	}
}

void UDataTable::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{	
	UDataTable* This = CastChecked<UDataTable>(InThis);

	// Need to emit references for referenced rows
	if(This->RowStruct != NULL)
	{
		// Now iterate over rows in the map
		for ( auto RowIt = This->RowMap.CreateIterator(); RowIt; ++RowIt )
		{
			uint8* RowData = RowIt.Value();

			if (RowData)
			{
				// Serialize all of the properties to make sure they get in the collector
				FSimpleObjectReferenceCollectorArchive ObjectReferenceCollector( This, Collector );
				This->RowStruct->SerializeBin(ObjectReferenceCollector, RowData, 0);
			}
		}
	}

	Super::AddReferencedObjects( This, Collector );
}

void UDataTable::FinishDestroy()
{
	Super::FinishDestroy();
	if(!IsTemplate())
	{
		EmptyTable(); // Free memory when UObject goes away
	}
}

FString UDataTable::GetTableAsString()
{
	FString Result;

	if(RowStruct != NULL)
	{
		Result += FString::Printf(TEXT("Using RowStruct: %s\n\n"), *RowStruct->GetPathName());

		// First build array of properties
		TArray<UProperty*> StructProps;
		for (TFieldIterator<UProperty> It(RowStruct); It; ++It)
		{
			UProperty* Prop = *It;
			check(Prop != NULL);
			StructProps.Add(Prop);
		}

		// First row, column titles, taken from properties
		Result += TEXT("---");
		for(int32 PropIdx=0; PropIdx<StructProps.Num(); PropIdx++)
		{
			Result += TEXT(",");
			Result += StructProps[PropIdx]->GetName();
		}
		Result += TEXT("\n");

		// Now iterate over rows
		for ( auto RowIt = RowMap.CreateIterator(); RowIt; ++RowIt )
		{
			FName RowName = RowIt.Key();
			Result += RowName.ToString();

			uint8* RowData = RowIt.Value();
			for(int32 PropIdx=0; PropIdx<StructProps.Num(); PropIdx++)
			{
				Result += TEXT(",");
				Result += GetPropertyValueAsString(StructProps[PropIdx], RowData);
			}
			Result += TEXT("\n");			
		}
	}
	else
	{
		Result += FString(TEXT("Missing RowStruct!\n"));
	}
	return Result;
}

FString UDataTable::GetTableAsJSON() const
{
	FString Result;

	if(RowStruct != NULL)
	{
		// use the pretty print policy since these values are usually getting dumpped for check-in to P4 (or for inspection)
		TSharedRef< TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR> > > JsonWriter = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR> >::Create(&Result);

		JsonWriter->WriteArrayStart();

		// First build array of properties
		TArray<UProperty*> StructProps;
		for (TFieldIterator<UProperty> It(RowStruct); It; ++It)
		{
			UProperty* Prop = *It;
			check(Prop != NULL);
			StructProps.Add(Prop);
		}

		// Iterate over rows
		for ( auto RowIt = RowMap.CreateConstIterator(); RowIt; ++RowIt )
		{
			JsonWriter->WriteObjectStart();

			//RowName
			FName RowName = RowIt.Key();
			JsonWriter->WriteValue(TEXT("Name"),RowName.ToString());

			//Now the values
			uint8* RowData = RowIt.Value();
			for(int32 PropIdx=0; PropIdx<StructProps.Num(); PropIdx++)
			{
				UProperty* BaseProp = StructProps[PropIdx];
				const void* Data = BaseProp->ContainerPtrToValuePtr<void>(RowData, 0);
				if (UNumericProperty *NumProp = Cast<UNumericProperty>(StructProps[PropIdx]))
				{
					if (NumProp->IsInteger())
					{
						JsonWriter->WriteValue(BaseProp->GetName(), NumProp->GetSignedIntPropertyValue(Data));
					}
					else
					{
						JsonWriter->WriteValue(BaseProp->GetName(), NumProp->GetFloatingPointPropertyValue(Data));
					}
				}
				else if (UBoolProperty* BoolProp = Cast<UBoolProperty>(StructProps[PropIdx]))
				{
					JsonWriter->WriteValue(BaseProp->GetName(), BoolProp->GetPropertyValue(Data));
				}
				else
				{
					FString PropertyValue = GetPropertyValueAsString(BaseProp, RowData);
					JsonWriter->WriteValue(BaseProp->GetName(), PropertyValue);
				}
			}
			JsonWriter->WriteObjectEnd();
		}
		JsonWriter->WriteArrayEnd();
		JsonWriter->Close();
	}
	else
	{
		Result += FString(TEXT("Missing RowStruct!\n"));
	}
	return Result;
}

void UDataTable::EmptyTable()
{
	UScriptStruct* LoadUsingStruct = RowStruct;
	if(LoadUsingStruct == NULL)
	{
		UE_LOG(LogDataTable, Error, TEXT("Missing RowStruct while emptying DataTable '%s'!"), *GetPathName());
		LoadUsingStruct = FTableRowBase::StaticStruct();
	}

	// Iterate over all rows in table and free mem
	for ( auto RowIt = RowMap.CreateIterator(); RowIt; ++RowIt )
	{
		uint8* RowData = RowIt.Value();
		LoadUsingStruct->DestroyScriptStruct(RowData);
		FMemory::Free(RowData);
	}

	// Finally empty the map
	RowMap.Empty();
}

/** Returns the column property where PropertyName matches the name of the column property. Returns NULL if no match is found or the match is not a supported table property */
UProperty* UDataTable::FindTableProperty(const FName& PropertyName) const
{
	UProperty* Property = NULL;
	for (TFieldIterator<UProperty> It(RowStruct); It; ++It)
	{
		Property = *It;
		check(Property != NULL);
		if (PropertyName == Property->GetFName())
		{
			break;
		}
	}
	if (!IsSupportedTableProperty(Property))
	{
		Property = NULL;
	}

	return Property;
}

/** Get array of UProperties that corresponds to columns in the table */
TArray<UProperty*> GetTablePropertyArray(const FString& FirstRowString, UStruct* RowStruct, TArray<FString>& OutProblems)
{
	TArray<UProperty*> ColumnProps;

	// Get list of all expected properties from the struct
	TArray<FName> ExpectedPropNames = GetStructPropertyNames(RowStruct);	

	// Find the column names from first row
	TArray<FString> ColumnNameStrings;
	FirstRowString.ParseIntoArray(&ColumnNameStrings, TEXT(","), false);

	// Need at least 2 columns, first column is skipped, will contain row names
	if(ColumnNameStrings.Num() > 0)
	{
		ColumnProps.AddZeroed( ColumnNameStrings.Num() );

		// first element always NULL - as first column is row names

		for(int32 ColIdx=1; ColIdx<ColumnNameStrings.Num(); ColIdx++)
		{
			FName PropName = MakeValidName(ColumnNameStrings[ColIdx]);
			if(PropName == NAME_None)
			{
				OutProblems.Add(FString(TEXT("Missing name for column %d."), ColIdx));
			}
			else
			{
				UProperty* ColumnProp = FindField<UProperty>(RowStruct, PropName);
				// Didn't find a property with this name, problem..
				if(ColumnProp == NULL)
				{
					OutProblems.Add(FString::Printf(TEXT("Cannot find Property for column '%s' in struct '%s'."), *PropName.ToString(), *RowStruct->GetName()));
				}
				// Found one!
				else
				{
					// Check we don't have this property already
					if(ColumnProps.Contains(ColumnProp))
					{
						OutProblems.Add(FString::Printf(TEXT("Duplicate column '%s'."), *ColumnProp->GetName()));
					}
					// Check we support this property type
					else if( !IsSupportedTableProperty(ColumnProp) )
					{
						OutProblems.Add(FString::Printf(TEXT("Unsupported Property type for struct member '%s'."), *ColumnProp->GetName()));
					}
					// Looks good, add to array
					else
					{
						ColumnProps[ColIdx] = ColumnProp;
					}

					// Track that we found this one
					ExpectedPropNames.Remove(PropName);
				}
			}
		}
	}

	// Generate warning for any properties in struct we are not filling in
	for(int32 PropIdx=0; PropIdx < ExpectedPropNames.Num(); PropIdx++)
	{
		OutProblems.Add(FString::Printf(TEXT("Expected column '%s' not found in input."), *ExpectedPropNames[PropIdx].ToString()));
	}

	return ColumnProps;
}

TArray<FString> UDataTable::CreateTableFromCSVString(const FString& InString)
{
	// Array used to store problems about table creation
	TArray<FString> OutProblems;

	// Check we have a RowStruct specified
	if(RowStruct == NULL)
	{
		OutProblems.Add(FString(TEXT("No RowStruct specified.")));
		return OutProblems;
	}

	// Split one giant string into one string per row
	TArray<FString> RowStrings;
	InString.ParseIntoArray(&RowStrings, TEXT("\r\n"), true);

	// Must have at least 2 rows (column names + data)
	if(RowStrings.Num() <= 1)
	{
		OutProblems.Add(FString(TEXT("Too few rows.")));
		return OutProblems;
	}

	// Find property for each column
	TArray<UProperty*> ColumnProps = GetTablePropertyArray(RowStrings[0], RowStruct, OutProblems);

	// Empty existing data
	EmptyTable();

	// Iterate over rows
	for(int32 RowIdx=1; RowIdx<RowStrings.Num(); RowIdx++)
	{
		TArray<FString> CellStrings;
		RowStrings[RowIdx].ParseIntoArray(&CellStrings, TEXT(","), false);

		// Need at least 1 cells (row name)
		if(CellStrings.Num() < 1)
		{
			OutProblems.Add(FString::Printf(TEXT("Row '%d' has too few cells."), RowIdx));
			continue;
		}

		// Need enough columns in the properties!
		if( ColumnProps.Num() < CellStrings.Num() )
		{
			OutProblems.Add(FString::Printf(TEXT("Row '%d' has more cells than properties, is there a malformed string?"), RowIdx));
			continue;
		}

		// Get row name
		FName RowName = MakeValidName(CellStrings[0]);

		// Check its not 'none'
		if(RowName == NAME_None)
		{
			OutProblems.Add(FString::Printf(TEXT("Row '%d' missing a name."), RowIdx));
			continue;
		}

		// Check its not a duplicate
		if(RowMap.Find(RowName) != NULL)
		{
			OutProblems.Add(FString::Printf(TEXT("Duplicate row name '%s'."), *RowName.ToString()));
			continue;
		}

		// Allocate data to store information, using UScriptStruct to know its size
		uint8* RowData = (uint8*)FMemory::Malloc(RowStruct->PropertiesSize);
		RowStruct->InitializeScriptStruct(RowData);
		// And be sure to call DestroyScriptStruct later

		// Add to row map
		RowMap.Add(RowName, RowData);

		// Now iterate over cells (skipping first cell, that was row name)
		for(int32 CellIdx=1; CellIdx<CellStrings.Num(); CellIdx++)
		{
			// Try and assign string to data using the column property
			UProperty* ColumnProp = ColumnProps[CellIdx];
			FString Error = AssignStringToProperty(CellStrings[CellIdx], ColumnProp, RowData);

			// If we failed, output a problem string
			if(Error.Len() > 0)
			{
				FString ColumnName = (ColumnProp != NULL) ? ColumnProp->GetName() : FString(TEXT("NONE"));
				OutProblems.Add(FString::Printf(TEXT("Problem assigning string '%s' to property '%s' on row '%s' : %s"), *CellStrings[CellIdx], *ColumnName, *RowName.ToString(), *Error));
			}
		}

		// Problem if we didn't have enough cells on this row
		if(CellStrings.Num() < ColumnProps.Num())
		{
			OutProblems.Add(FString::Printf(TEXT("Too few cells on row '%s'."), *RowName.ToString()));			
		}
	}

	Modify(true);
	return OutProblems;
}


//NOT READY YET, PLACEHOLDER
TArray<FString> UDataTable::CreateTableFromJSONString(const FString& InString)
{
	// Array used to store problems about table creation
	TArray<FString> OutProblems;

	return OutProblems;
}


TArray<FString> UDataTable::GetColumnTitles() const
{
	TArray<FString> Result;
	Result.Add(TEXT("Name"));
	for (TFieldIterator<UProperty> It(RowStruct); It; ++It)
	{
		UProperty* Prop = *It;
		check(Prop != NULL);
		Result.Add(Prop->GetName());
	}
	return Result;
}

TArray< TArray<FString> > UDataTable::GetTableData() const
{
	 TArray< TArray<FString> > Result;

	 Result.Add(GetColumnTitles());

	 // First build array of properties
	 TArray<UProperty*> StructProps;
	 for (TFieldIterator<UProperty> It(RowStruct); It; ++It)
	 {
		 UProperty* Prop = *It;
		 check(Prop != NULL);
		 StructProps.Add(Prop);
	 }

	 // Now iterate over rows
	 for ( auto RowIt = RowMap.CreateConstIterator(); RowIt; ++RowIt )
	 {
		 TArray<FString> RowResult;
		 FName RowName = RowIt.Key();
		 RowResult.Add(RowName.ToString());

		 uint8* RowData = RowIt.Value();
		 for(int32 PropIdx=0; PropIdx<StructProps.Num(); PropIdx++)
		 {
			 RowResult.Add(GetPropertyValueAsString(StructProps[PropIdx], RowData));
		 }
		 Result.Add(RowResult);
	 }
	 return Result;

}

