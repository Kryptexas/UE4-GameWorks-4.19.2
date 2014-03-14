// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"

/*-----------------------------------------------------------------------------
	UPackage.
-----------------------------------------------------------------------------*/

/** Delegate to notify subscribers when a package has been saved. This is triggered when the package saving
 *  has completed and was successful. */
UPackage::FOnPackageSaved UPackage::PackageSavedEvent;
/** Delegate to notify subscribers about the dirty state of a package being set.
 *  Allows the editor to register the modified package as one that should be prompted for source control checkout. 
 *  Use Package->IsDirty() to get the updated dirty state of the package */
UPackage::FOnPackageDirtyStateUpdated UPackage::PackageDirtyStateUpdatedEvent;

void UPackage::PostInitProperties()
{
	Super::PostInitProperties();
	if ( !HasAnyFlags(RF_ClassDefaultObject) )
	{
		bDirty = 0;
	}

	MetaData = NULL;

#if WITH_EDITOR
	PIEInstanceID = INDEX_NONE;
#endif
}


/**
 * Marks/Unmarks the package's bDirty flag
 */
void UPackage::SetDirtyFlag( bool bIsDirty )
{
	if ( GetOutermost() != GetTransientPackage() )
	{
		if ( GUndo != NULL
		// PIE world objects should never end up in the transaction buffer as we cannot undo during gameplay.
		&& !(GetOutermost()->PackageFlags & (PKG_PlayInEditor|PKG_ContainsScript)) )
		{
			// make sure we're marked as transactional
			SetFlags(RF_Transactional);

			// don't call Modify() since it calls SetDirtyFlag()
			GUndo->SaveObject( this );
		}

		// Update dirty bit
		bDirty = bIsDirty;

		if( GIsEditor									// Only fire the callback in editor mode
			&& !(PackageFlags & PKG_ContainsScript)		// Skip script packages
			&& !(PackageFlags & PKG_PlayInEditor)		// Skip packages for PIE
			&& GetTransientPackage() != this )			// Skip the transient package
		{
			// Package is changing dirty state, let the editor know so we may prompt for source control checkout
			PackageDirtyStateUpdatedEvent.Broadcast(this);
		}
	}
}

/**
 * Serializer
 * Save the value of bDirty into the transaction buffer, so that undo/redo will also mark/unmark the package as dirty, accordingly
 */
void UPackage::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	if ( Ar.IsTransacting() )
	{
		Ar << bDirty;
	}
}

void UPackage::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{	
	UPackage* This = CastChecked<UPackage>(InThis);
#if WITH_EDITOR
	if( GIsEditor )
	{
		// Required by the unified GC when running in the editor
		Collector.AddReferencedObject(This->MetaData, This);
	}
#endif
	Super::AddReferencedObjects(This, Collector);
}

/**
 * Get (after possibly creating) a metadata object for this package
 *
 * @param	bAllowLoadObject				Can load an object to find it's UMetaData if not currently loaded.
 *
 * @return A valid UMetaData pointer for all objects in this package
 */
UMetaData* UPackage::GetMetaData(const bool bAllowLoadObject)
{
	if (MetaData == NULL)
	{
		// first try to load it
		if ( (PackageFlags&(PKG_Compiling|PKG_ContainsMap|PKG_CompiledIn)) == 0 && this != GetTransientPackage())
		{
			// Try to find before loading, meta data may already exists and we want to avoid calling LoadAllObjects again.
			MetaData = Cast<UMetaData>(StaticFindObjectFast(UMetaData::StaticClass(), this, FName(NAME_PackageMetaData)));
			if (MetaData == NULL && bAllowLoadObject)
			{
				MetaData = LoadObject<UMetaData>(this, *FName(NAME_PackageMetaData).ToString(), NULL, LOAD_NoWarn|LOAD_Quiet, NULL);
				if (MetaData == NULL)
				{
					// Check old name, and rename if applicable
					MetaData = LoadObject<UMetaData>(this, *UMetaData::StaticClass()->GetName(), NULL, LOAD_NoWarn|LOAD_Quiet, NULL);
					if (MetaData)
					{				
						MetaData->Rename(*FName(NAME_PackageMetaData).ToString(), NULL, REN_ForceNoResetLoaders);
					}
				}
			}
		}

		// if it wasn't found, then create it
		if (MetaData == NULL)
		{
			// make a metadata object, but only allow it to be loaded in the editor
			MetaData = ConstructObject<UMetaData>(UMetaData::StaticClass(), this, NAME_PackageMetaData, RF_Standalone);
		}
		check(MetaData);
	}

	if (MetaData && MetaData->HasAllFlags(RF_NeedLoad) && bAllowLoadObject)
	{
		if (auto Linker = MetaData->GetLinker())
		{
			Linker->Preload(MetaData);
		}
	}

	return MetaData;
}

/**
 * Fully loads this package. Safe to call multiple times and won't clobber already loaded assets.
 */
void UPackage::FullyLoad()
{
	// Make sure we're a topmost package.
	check(GetOuter()==NULL);

	// Only perform work if we're not already fully loaded.
	if( !IsFullyLoaded() )
	{
		// Mark package so exports are found in memory first instead of being clobbered.
		bool bSavedState = ShouldFindExportsInMemoryFirst();
		FindExportsInMemoryFirst( true );

		// Re-load this package.
		LoadPackage( NULL, *GetName(), LOAD_None );

		// Restore original state.
		FindExportsInMemoryFirst( bSavedState );
	}
}

/** Tags generated objects with flags */
void UPackage::TagSubobjects(EObjectFlags NewFlags)
{
	Super::TagSubobjects(NewFlags);

	if (MetaData)
	{
		MetaData->SetFlags(NewFlags);
	}

	if (GetLinker())
	{
		GetLinker()->SetFlags(NewFlags);
	}
}

/**
 * Returns whether the package is fully loaded.
 *
 * @return true if fully loaded or no file associated on disk, false otherwise
 */
bool UPackage::IsFullyLoaded()
{
	// Newly created packages aren't loaded and therefore haven't been marked as being fully loaded. They are treated as fully
	// loaded packages though in this case, which is why we are looking to see whether the package exists on disk and assume it
	// has been fully loaded if it doesn't.
	if( !bHasBeenFullyLoaded )
	{
		FString DummyFilename;
		// Try to find matching package in package file cache.
		if( !FPackageName::DoesPackageExist( *GetName(), NULL, &DummyFilename ) 
		||	(GIsEditor && IFileManager::Get().FileSize(*DummyFilename) < 0) )
		{
			// Package has NOT been found, so we assume it's a newly created one and therefore fully loaded.
			bHasBeenFullyLoaded = true;
		}
	}

	return bHasBeenFullyLoaded;
}


IMPLEMENT_CORE_INTRINSIC_CLASS(UPackage, UObject,
	{
		Class->ClassAddReferencedObjects = &UPackage::AddReferencedObjects;
		Class->EmitObjectReference(STRUCT_OFFSET(UPackage, MetaData));
	}
);
