// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "../Classes/Kismet/DataTableFunctionLibrary.h"

UDataTableFunctionLibrary::UDataTableFunctionLibrary(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UDataTableFunctionLibrary::EvaluateCurveTableRow(UCurveTable* CurveTable, FName RowName, float InXY, TEnumAsByte<EEvaluateCurveTableResult::Type>& OutResult, float& OutXY)
{
    FCurveTableRowHandle Handle;
    Handle.CurveTable = CurveTable;
    Handle.RowName = RowName;
    
	bool found = Handle.Eval(InXY, &OutXY);
    
    if (found)
    {
        OutResult = EEvaluateCurveTableResult::RowFound;
    }
    else
    {
        OutResult = EEvaluateCurveTableResult::RowNotFound;
    }
}

bool UDataTableFunctionLibrary::GetDataTableRowFromName(UDataTable* Table, FName RowName, FTableRowBase& OutRow)
{
	// We should never hit this!  stubs to avoid NoExport on the class.
	check(0);
    return false;
}