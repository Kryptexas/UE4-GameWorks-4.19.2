// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"

/*-----------------------------------------------------------------------------
	Helper functions.
-----------------------------------------------------------------------------*/
namespace
{
	FORCEINLINE bool IsCorePackage(const FName& PackageName)
	{
		return PackageName == NAME_Core || PackageName == GLongCorePackageName;
	}
}

/*-----------------------------------------------------------------------------
	FObjectResource
-----------------------------------------------------------------------------*/

FObjectResource::FObjectResource()
{}

FObjectResource::FObjectResource( UObject* InObject )
:	ObjectName		( InObject ? InObject->GetFName() : FName(NAME_None)		)
{
}

/*-----------------------------------------------------------------------------
	FObjectExport.
-----------------------------------------------------------------------------*/

FObjectExport::FObjectExport()
:	FObjectResource	()
,	ObjectFlags		( RF_NoFlags												)
,	bExportLoadFailed	( false													)
,	SerialSize		( 0															)
,	SerialOffset	( 0															)
,	ScriptSerializationStartOffset	( 0											)
,	ScriptSerializationEndOffset	( 0											)
,	Object			( NULL														)
,	HashNext		( INDEX_NONE												)
,	bForcedExport	( false														)
,	bNotForClient	( false														)
,	bNotForServer	( false														)
,	PackageGuid		( FGuid(0,0,0,0)											)
{}

FObjectExport::FObjectExport( UObject* InObject )
:	FObjectResource	( InObject													)
,	ObjectFlags		( InObject ? InObject->GetMaskedFlags() : RF_NoFlags		)
,	bExportLoadFailed	( false													)
,	SerialSize		( 0															)
,	SerialOffset	( 0															)
,	ScriptSerializationStartOffset	( 0											)
,	ScriptSerializationEndOffset	( 0											)
,	Object			( InObject													)
,	HashNext		( INDEX_NONE												)
,	bForcedExport	( false														)
,	bNotForClient	( false														)
,	bNotForServer	( false														)
,	PackageGuid		( FGuid(0,0,0,0)											)
,	PackageFlags	( 0															)
{
	if(Object)		
	{
		bNotForClient = Object->HasAnyMarks(OBJECTMARK_NotForClient);
		bNotForServer = Object->HasAnyMarks(OBJECTMARK_NotForServer);
	}
}

FArchive& operator<<( FArchive& Ar, FObjectExport& E )
{
	Ar << E.ClassIndex;
	Ar << E.SuperIndex;
	Ar << E.OuterIndex;
	Ar << E.ObjectName;

	if (Ar.UE4Ver() < VER_UE4_REMOVE_ARCHETYPE_INDEX_FROM_LINKER_TABLES)
	{
		FPackageIndex OldArchetypeIndex;
		Ar << OldArchetypeIndex;	
	}

	uint32 Save = E.ObjectFlags & RF_Load;
	Ar << Save;
	if (Ar.IsLoading())
	{
		E.ObjectFlags = EObjectFlags(Save & RF_Load);
	}

	Ar << E.SerialSize;
	Ar << E.SerialOffset;

	Ar << E.bForcedExport;
	Ar << E.bNotForClient;
	Ar << E.bNotForServer;

	if (Ar.UE4Ver() < VER_UE4_REMOVE_NET_INDEX)
	{
		TArray<int32>		OldGenerationNetObjectCount;
		Ar << OldGenerationNetObjectCount;
	}
	Ar << E.PackageGuid;
	Ar << E.PackageFlags;

	return Ar;
}

/*-----------------------------------------------------------------------------
	FObjectImport.
-----------------------------------------------------------------------------*/

FObjectImport::FObjectImport()
:	FObjectResource	()
{
}

FObjectImport::FObjectImport( UObject* InObject )
:	FObjectResource	( InObject																)
,	ClassPackage	( InObject ? InObject->GetClass()->GetOuter()->GetFName()	: NAME_None	)
,	ClassName		( InObject ? InObject->GetClass()->GetFName()				: NAME_None	)
,	XObject			( InObject																)
,	SourceLinker	( NULL																	)
,	SourceIndex		( INDEX_NONE															)
{
}

FArchive& operator<<( FArchive& Ar, FObjectImport& I )
{
	Ar << I.ClassPackage << I.ClassName;
	Ar << I.OuterIndex;
	Ar << I.ObjectName;
	if( Ar.IsLoading() )
	{
		I.SourceLinker	= NULL;
		I.SourceIndex	= INDEX_NONE;
		I.XObject		= NULL;
		if (Ar.UE4Ver() < VER_UE4_CORE_SPLIT)
		{
			if (IsCorePackage(I.ClassPackage))
			{
				I.ClassPackage = GLongCoreUObjectPackageName;
			}
			if (IsCorePackage(I.ObjectName))
			{
				I.ObjectName = GLongCoreUObjectPackageName;
			}
		}
		// Short -> Long package name associations removal - convert all 
		// short script package names to long package names.
		if (Ar.UE4Ver() < VER_UE4_REMOVE_SHORT_PACKAGE_NAME_ASSOCIATIONS)
		{
			FName* LongScriptPackageName = FPackageName::FindScriptPackageName(I.ClassPackage);
			if (LongScriptPackageName)
			{
				I.ClassPackage = *LongScriptPackageName;
			}
			LongScriptPackageName = FPackageName::FindScriptPackageName(I.ObjectName);
			if (LongScriptPackageName)
			{
				I.ObjectName = *LongScriptPackageName;
			}
		}
	}
	return Ar;
}