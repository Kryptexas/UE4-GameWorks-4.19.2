// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FPackageDependencyData : public FLinkerTables
{
public:
	/** The name of the package that dependency data is gathered from */
	FString PackageName;

	/**
	 * Return the path name of the UObject represented by the specified import. 
	 * (can be used with StaticFindObject)
	 * 
	 * @param	ImportIndex	index into the ImportMap for the resource to get the name for
	 *
	 * @return	the path name of the UObject represented by the resource at ImportIndex
	 */
	FString GetImportPathName(int32 ImportIndex);
	/**
	 * Return the path name of the UObject represented by the specified import. 
	 * (can be used with StaticFindObject)
	 * 
	 * @param	PackageIndex	package index for the resource to get the name for
	 *
	 * @return	the path name of the UObject represented by the resource at PackageIndex, or the empty string if this isn't an import
	 */
	FString GetImportPathName(FPackageIndex PackageIndex)
	{
		if (PackageIndex.IsImport())
		{
			return GetImportPathName(PackageIndex.ToImport());
		}
		return FString();
	}

	/**
	 * Return the package name of the UObject represented by the specified import. 
	 * 
	 * @param	PackageIndex	package index for the resource to get the name for
	 *
	 * @return	the path name of the UObject represented by the resource at PackageIndex, or the empty string if this isn't an import
	 */
	FName GetImportPackageName(int32 ImportIndex);

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
	FString GetExportPathName(int32 ExportIndex, bool bResolveForcedExports = false);
	/**
	 * Return the path name of the UObject represented by the specified export.
	 * (can be used with StaticFindObject)
	 * 
	 * @param	PackageIndex			package index for the resource to get the name for
	 * @param	bResolveForcedExports	if true, the package name part of the return value will be the export's original package,
	 *									not the name of the package it's currently contained within.
	 *
	 * @return	the path name of the UObject represented by the resource at PackageIndex, or the empty string if this isn't an export
	 */
	FString GetExportPathName(FPackageIndex PackageIndex,bool bResolveForcedExports=false)
	{
		if (PackageIndex.IsExport())
		{
			return GetExportPathName(PackageIndex.ToExport(), bResolveForcedExports);
		}
		return FString();
	}

	/**
	 * Return the path name of the UObject represented by the specified import. 
	 * (can be used with StaticFindObject)
	 * 
	 * @param	PackageIndex	package index
	 *
	 * @return	the path name of the UObject represented by the resource at PackageIndex, or the empty string if this is null
	 */
	FString GetPathName(FPackageIndex PackageIndex)
	{
		if (PackageIndex.IsImport())
		{
			return GetImportPathName(PackageIndex);
		}
		else if (PackageIndex.IsExport())
		{
			return GetExportPathName(PackageIndex);
		}
		return FString();
	}


};