// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "json.h"

DEFINE_LOG_CATEGORY(LogCurveTable);

ENGINE_API const FString FCurveTableRowHandle::Unknown(TEXT("UNKNOWN"));

DECLARE_CYCLE_STAT(TEXT("CurveTableRowHandle Eval"),STAT_CurveTableRowHandleEval,STATGROUP_Engine);


//////////////////////////////////////////////////////////////////////////
UCurveTable::UCurveTable(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

/** Util that removes invalid chars and then make an FName */
FName UCurveTable::MakeValidName(const FString& InString)
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

void UCurveTable::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar); // When loading, this should load our RowCurve!	

	if(Ar.IsLoading())
	{
		int32 NumRows;
		Ar << NumRows;

		for(int32 RowIdx = 0; RowIdx < NumRows; RowIdx++)
		{
			// Load row name
			FName RowName;
			Ar << RowName;

			// Load row data
			FRichCurve* NewCurve = new FRichCurve();
			FRichCurve::StaticStruct()->SerializeTaggedProperties(Ar, (uint8*)NewCurve, FRichCurve::StaticStruct(), NULL);

			// Add to map
			RowMap.Add(RowName, NewCurve);
		}
	}
	else if(Ar.IsSaving())
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
			FRichCurve* Curve = RowIt.Value();
			FRichCurve::StaticStruct()->SerializeTaggedProperties(Ar, (uint8*)Curve, FRichCurve::StaticStruct(), NULL);
		}
	}
}

void UCurveTable::FinishDestroy()
{
	Super::FinishDestroy();

	EmptyTable(); // Free memory when UObject goes away
}

FString UCurveTable::GetTableAsString()
{
	FString Result;

	if(RowMap.Num() > 0)
	{
		TArray<FName> Names;
		TArray<FRichCurve*> Curves;
		
		// get the row names and curves they represent
		RowMap.GenerateKeyArray(Names);
		RowMap.GenerateValueArray(Curves);

		// Determine the curve with the longest set of data, for headers
		int32 LongestCurveIndex = 0;
		for(int32 CurvesIdx = 1; CurvesIdx < Curves.Num(); CurvesIdx++)
		{
			if(Curves[CurvesIdx]->GetNumKeys() > Curves[LongestCurveIndex]->GetNumKeys())
			{
				LongestCurveIndex = CurvesIdx;
			}
		}

		// First row, column titles, taken from the longest curve
		Result += TEXT("---");
		for (auto It(Curves[LongestCurveIndex]->GetKeyIterator()); It; ++It)
		{
			Result += FString::Printf(TEXT(",%f"), It->Time);
		}
		Result += TEXT("\n");

		// display all the curves
		for(int32 CurvesIdx = 0; CurvesIdx < Curves.Num(); CurvesIdx++)
		{
			// show name of curve
			Result += Names[CurvesIdx].ToString();

			// show data of curve
			for (auto It(Curves[CurvesIdx]->GetKeyIterator()); It; ++It)
			{
				Result += FString::Printf(TEXT(",%f"), It->Value);
			}

			Result += TEXT("\n");
		}
	}
	else
	{
		Result += FString(TEXT("No data in row curve!\n"));
	}
	return Result;
}

FString UCurveTable::GetTableAsJSON() const
{
	FString Result;

	if(RowMap.Num() > 0)
	{
		TArray<FName> Names;
		TArray<FRichCurve*> Curves;

		// get the row names and curves they represent
		RowMap.GenerateKeyArray(Names);
		RowMap.GenerateValueArray(Curves);

		// Determine the curve with the longest set of data, for headers
		int32 LongestCurveIndex = 0;
		for(int32 CurvesIdx = 1; CurvesIdx < Curves.Num(); CurvesIdx++)
		{
			if(Curves[CurvesIdx]->GetNumKeys() > Curves[LongestCurveIndex]->GetNumKeys())
			{
				LongestCurveIndex = CurvesIdx;
			}
		}

		// use the pretty print policy since these values are usually getting dumpped for check-in to P4 (or for inspection)
		TSharedRef< TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR> > > JsonWriter = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR> >::Create(&Result);
		JsonWriter->WriteArrayStart();

		// display all the curves
		for(int32 CurvesIdx = 0; CurvesIdx < Curves.Num(); CurvesIdx++)
		{
			JsonWriter->WriteObjectStart();
			// show name of curve
			JsonWriter->WriteValue(TEXT("Name"),Names[CurvesIdx].ToString());

			// show data of curve
			auto LongIt(Curves[LongestCurveIndex]->GetKeyIterator());
			for (auto It(Curves[CurvesIdx]->GetKeyIterator()); It; ++It)
			{
				JsonWriter->WriteValue(FString::Printf(TEXT("%d"), (int32)LongIt->Time), It->Value);
				++LongIt;
			}
			JsonWriter->WriteObjectEnd();
		}

		JsonWriter->WriteArrayEnd();
		JsonWriter->Close();
	}
	else
	{
		Result += FString(TEXT("No data in row curve!\n"));
	}
	return Result;
}

void UCurveTable::EmptyTable()
{
	// Iterate over all rows in table and free mem
	for ( auto RowIt = RowMap.CreateIterator(); RowIt; ++RowIt )
	{
		FRichCurve* RowData = RowIt.Value();
		delete RowData;
	}

	// Finally empty the map
	RowMap.Empty();
}


/** */
void GetCurveValues(const FString& RowString, TArray<float>* Values)
{
	// Find the x values from he first row
	TArray<FString> ValuesAsStrings;
	RowString.ParseIntoArray(&ValuesAsStrings, TEXT(","), false);

	// Need at least 2 columns, first column is skipped, will contain row names
	if(ValuesAsStrings.Num() >= 2)
	{
		// first element always NULL - as first column is row names
		for(int32 ColIdx = 1; ColIdx < ValuesAsStrings.Num(); ColIdx++)
		{
			Values->Add(FCString::Atof(*ValuesAsStrings[ColIdx]));
		}
	}
}

TArray<FString> UCurveTable::CreateTableFromCSVString(const FString& InString, ERichCurveInterpMode InterpMode)
{
	// Array used to store problems about table creation
	TArray<FString> OutProblems;

	// Split one giant string into one string per row
	TArray<FString> RowStrings;
	InString.ParseIntoArray(&RowStrings, TEXT("\r\n"), true);

	// Must have at least 2 rows (x values + y values for at least one row)
	if(RowStrings.Num() <= 1)
	{
		OutProblems.Add(FString(TEXT("Too few rows.")));
		return OutProblems;
	}

	// Empty existing data
	EmptyTable();

	TArray<float> XValues;
	GetCurveValues(RowStrings[0], &XValues);

	// Iterate over rows
	for(int32 RowIdx = 1; RowIdx < RowStrings.Num(); RowIdx++)
	{
		TArray<FString> CellStrings;
		RowStrings[RowIdx].ParseIntoArray(&CellStrings, TEXT(","), false);

		// Need at least 1 cells (row name)
		if(CellStrings.Num() < 1)
		{
			OutProblems.Add(FString::Printf(TEXT("Row '%d' has too few cells."), RowIdx));
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

		TArray<float> YValues;
		GetCurveValues(RowStrings[RowIdx], &YValues);

		if(XValues.Num() != YValues.Num())
		{
			OutProblems.Add(FString::Printf(TEXT("Row '%s' does not have the right number of columns."), *RowName.ToString()));
			continue;
		}

		FRichCurve* NewCurve = new FRichCurve();
		// Now iterate over cells (skipping first cell, that was row name)
		for(int32 ColumnIdx = 0; ColumnIdx < XValues.Num(); ColumnIdx++)
		{
			FKeyHandle KeyHandle = NewCurve->AddKey(XValues[ColumnIdx], YValues[ColumnIdx]);
			NewCurve->SetKeyInterpMode(KeyHandle, InterpMode);
		}

		RowMap.Add(RowName, NewCurve);
	}

	Modify(true);
	return OutProblems;
}

//////////////////////////////////////////////////////////////////////////


FRichCurve* FCurveTableRowHandle::GetCurve(const FString& ContextString) const
{
	if(CurveTable == NULL)
	{
		if (RowName != NAME_None)
		{
			UE_LOG(LogCurveTable, Warning, TEXT("FCurveTableRowHandle::FindRow : No CurveTable for row %s (%s)."), *RowName.ToString(), *ContextString);
		}
		return NULL;
	}

	return CurveTable->FindCurve(RowName, ContextString);
}

float FCurveTableRowHandle::Eval(float XValue) const
{
	SCOPE_CYCLE_COUNTER(STAT_CurveTableRowHandleEval); 

	FRichCurve* Curve = GetCurve();
	if(Curve != NULL)
	{
		return Curve->Eval(XValue);
	}

	return 0;
}

bool FCurveTableRowHandle::Eval(float XValue, float* YValue) const
{
	SCOPE_CYCLE_COUNTER(STAT_CurveTableRowHandleEval); 

	FRichCurve* Curve = GetCurve();
	if(Curve != NULL && YValue != NULL)
	{
		*YValue = Curve->Eval(XValue);
		return true;
	}

	return false;
}