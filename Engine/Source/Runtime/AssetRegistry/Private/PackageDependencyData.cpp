// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetRegistryPCH.h"

/**
 * Return the path name of the UObject represented by the specified import. 
 * (can be used with StaticFindObject)
 * 
 * @param	ImportIndex	index into the ImportMap for the resource to get the name for
 *
 * @return	the path name of the UObject represented by the resource at ImportIndex
 */
FString FPackageDependencyData::GetImportPathName(int32 ImportIndex)
{
	FString Result;
	for (FPackageIndex LinkerIndex = FPackageIndex::FromImport(ImportIndex); !LinkerIndex.IsNull();)
	{
		FObjectResource* Resource = &ImpExp(LinkerIndex);
		bool bSubobjectDelimiter=false;

		// if this import is not a UPackage but this import's Outer is a UPackage, we need to use subobject notation
		if (Result.Len() > 0 && GetClassName(LinkerIndex) != NAME_Package)
		{
			if (Resource->OuterIndex.IsNull() || GetClassName(Resource->OuterIndex) == NAME_Package)
			{
				bSubobjectDelimiter = true;
			}
		}

		// don't append a dot in the first iteration
		if ( Result.Len() > 0 )
		{
			if ( bSubobjectDelimiter )
			{
				Result = FString(SUBOBJECT_DELIMITER) + Result;
			}
			else
			{
				Result = FString(TEXT(".")) + Result;
			}
		}

		Result = Resource->ObjectName.ToString() + Result;
		LinkerIndex = Resource->OuterIndex;
	}
	return Result;
}

FName FPackageDependencyData::GetImportPackageName(int32 ImportIndex)
{
	FName Result;
	for (FPackageIndex LinkerIndex = FPackageIndex::FromImport(ImportIndex); !LinkerIndex.IsNull();)
	{
		FObjectResource* Resource = &ImpExp(LinkerIndex);
		LinkerIndex = Resource->OuterIndex;
		if ( LinkerIndex.IsNull() )
		{
			Result = Resource->ObjectName;
		}
	}

	return Result;
}

/**
 * Return the path name of the UObject represented by the specified export.
 * (can be used with StaticFindObject)
 * 
 * @param	ExportIndex				index into the ExportMap for the resource to get the name for
 * @param	bResolveForcedExports	if true, the package name part of the return value will be the export's original package,
 *									not the name of the package it's currently contained within.
 *
 * @return	the path name of the UObject represented by the resource at ExportIndex
 */
FString FPackageDependencyData::GetExportPathName(int32 ExportIndex, bool bResolveForcedExports/*=false*/)
{
	FString Result;

	bool bForcedExport = false;
	for ( FPackageIndex LinkerIndex = FPackageIndex::FromExport(ExportIndex); !LinkerIndex.IsNull(); LinkerIndex = Exp(LinkerIndex).OuterIndex )
	{ 
		const FObjectExport Export = Exp(LinkerIndex);

		// don't append a dot in the first iteration
		if ( Result.Len() > 0 )
		{
			// if this export is not a UPackage but this export's Outer is a UPackage, we need to use subobject notation
			if ((Export.OuterIndex.IsNull() || GetClassName(Export.OuterIndex) == NAME_Package)
				&&	GetClassName(LinkerIndex) != NAME_Package)
			{
				Result = FString(SUBOBJECT_DELIMITER) + Result;
			}
			else
			{
				Result = FString(TEXT(".")) + Result;
			}
		}
		Result = Export.ObjectName.ToString() + Result;
		bForcedExport = bForcedExport || Export.bForcedExport;
	}

	if ( bForcedExport && bResolveForcedExports )
	{
		// Result already contains the correct path name for this export
		return Result;
	}

	return PackageName + TEXT(".") + Result;
}