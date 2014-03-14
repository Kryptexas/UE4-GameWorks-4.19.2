// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITextureFormatModule.h: Declares the ITextureFormatModule interface.
=============================================================================*/

#pragma once


/**
 * IPhysXFormat, PhysX cooking abstraction
**/
class IPhysXFormat
{
public:

	/**
	 * Checks whether parallel PhysX cooking is allowed.
	 *
	 * Note: This method is not currently used yet.
	 *
	 * @return true if this PhysX format can cook in parallel, false otherwise.
	 */
	virtual bool AllowParallelBuild( ) const
	{
		return false;
	}

	/**
	 * Cooks the source convex data for the platform and stores the cooked data internally.
	 *
	 * @param Format - The desired format
	 * @param SrcBuffer - The source buffer
	 * @param OutBuffer - The resulting cooked data
	 *
	 * @return	true if succeeded, false otherwise.
	 */
	virtual bool CookConvex( FName Format, const TArray<FVector>& SrcBuffer, TArray<uint8>& OutBuffer ) const = 0;

	/**
	 * Cooks the source Tri-Mesh data for the platform and stores the cooked data internally.
	 *
	 * @param Format - The desired format.
	 * @param SrcBuffer - The source buffer.
	 * @param OutBuffer - The resulting cooked data.
	 *
	 * @return	true if succeeded, false otherwise.
	 */
	virtual bool CookTriMesh( FName Format, const TArray<FVector>& SrcVertices, const TArray<struct FTriIndices>& SrcIndices, const TArray<uint16>& SrcMaterialIndices, const bool FlipNormals, TArray<uint8>& OutBuffer ) const = 0;

	/**
	 * Gets the list of supported formats.
	 *
	 * @param OutFormats - Will hold the list of formats.
	 */
	virtual void GetSupportedFormats( TArray<FName>& OutFormats ) const = 0;

	/**
	 * Gets the current version of the specified PhysX format.
	 *
	 * @param Format - The format to get the version for.
	 *
	 * @return Version number.
	 */
	virtual uint16 GetVersion( FName Format ) const = 0;


public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IPhysXFormat( ) { }
};
