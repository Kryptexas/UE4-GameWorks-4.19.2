// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "../Classes/Kismet/DataTableFunctionLibrary.h"

UDataTableFunctionLibrary::UDataTableFunctionLibrary(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool UDataTableFunctionLibrary::EvaluateCurveTableRow(UCurveTable* Table, FName RowName, float InXY, float& OutXY)
{
    FCurveTableRowHandle Handle;
    Handle.CurveTable = Table;
    Handle.RowName = RowName;
    
	return Handle.Eval(InXY, &OutXY);
}

bool UDataTableFunctionLibrary::GetDataTableRowFromName(UDataTable* Table, FName RowName, FTableRowBase& OutRow)
{
	// We should never hit this!  stubs to avoid NoExport on the class.
	check(0);
    return false;
}