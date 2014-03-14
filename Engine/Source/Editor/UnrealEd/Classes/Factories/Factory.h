// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Base class for all factories
 * An object responsible for creating and importing new objects.
 * 
 */

#pragma once
#include "Factory.generated.h"

UCLASS(abstract,MinimalAPI)
class UFactory : public UObject
{
	GENERATED_UCLASS_BODY()

	/** The class manufactured by this factory. */
	UPROPERTY()
	TSubclassOf<class UObject>  SupportedClass;

	/** Class of the context object used to help create the object. */
	UPROPERTY()
	TSubclassOf<class UObject>  ContextClass;

	/** List of formats supported by the factory. Each entry is of the form "ext;Description" where ext is the file extension. */
	UPROPERTY()
	TArray<FString> Formats;

	/** true if the factory creates a new object from scratch. */
	UPROPERTY()
	uint32 bCreateNew:1;

	/** true if the associated editor should be opened after creating a new object. */
	UPROPERTY()
	uint32 bEditAfterNew:1;

	/** true if the factory imports objects from files. */
	UPROPERTY()
	uint32 bEditorImport:1;

	/** true if the factory imports objects from text. */
	UPROPERTY()
	uint32 bText:1;

	/** Determines the order in which factories are tried when importing an object. */
	UPROPERTY()
	int32 AutoPriority;

	/** List of game names that this factory can be used for (if empty, all games valid) */
	UPROPERTY()
	TArray<FString> ValidGameNames;

	UNREALED_API static FString GetCurrentFilename() { return CurrentFilename; }
	static FString	CurrentFilename;

	/** For interactive object imports, this value indicates whether the user wants objects to be automatically
		overwritten (See EAppReturnType), or -1 if the user should be prompted. */
	static int32 OverwriteYesOrNoToAllState;

	/** If this value is true, warning messages will be shown once for all objects being imported at the same time.  
		This value will be reset to false each time a new import operation is started. */
	static bool bAllowOneTimeWarningMessages;

	// Begin UObject interface.
	UNREALED_API static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject interface.

	/** Returns true if this factory should be shown in the New Asset menu. */
	UNREALED_API virtual bool ShouldShowInNewMenu() const;

	/** Returns an optional override brush name for the new asset menu. If this is not specified, the thumbnail for the supported class will be used. */
	UNREALED_API virtual FName GetNewAssetThumbnailOverride() const { return NAME_None; }

	/** Returns the name of the factory for menus */
	UNREALED_API virtual FText GetDisplayName() const;

	/** When shown in menus, this is the category containing this factory. Return type is a BitFlag mask using EAssetTypeCategories. */
	UNREALED_API virtual uint32 GetMenuCategories() const;

	/** Returns the tooltip text description of this factory */
	UNREALED_API virtual FText GetToolTip() const;

	/**
	 * @return		The object class supported by this factory.
	 */
	UNREALED_API UClass* GetSupportedClass() const;

	/**
	 * @return true if it supports this class 
	 */
	UNREALED_API virtual bool DoesSupportClass(UClass * Class);

	/**
	 * Resolves SupportedClass for factories which support multiple classes.
	 * Such factories will have a NULL SupportedClass member.
	 */
	UNREALED_API virtual UClass* ResolveSupportedClass();

	/**
	 * Resets the saved state of this factory.  The states are used to suppress messages during multiple object import. 
	 * It needs to be reset each time a new import is started
	 */
	UNREALED_API static void ResetState();

	/** Opens a dialog to configure the factory properties. Return false if user opted out of configuring properties */
	virtual bool ConfigureProperties() { return true; }

	// @todo document
	virtual UObject* FactoryCreateText( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn ) {return NULL;}

	// @todo document
	// @param Type must not be 0, e.g. TEXT("TGA")
	virtual UObject* FactoryCreateBinary( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) {return NULL;}
	// @param Type must not be 0, e.g. TEXT("TGA")
	virtual UObject* FactoryCreateBinary( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn, bool& bOutOperationCanceled) { return FactoryCreateBinary(InClass, InParent, InName, Flags, Context, Type, Buffer, BufferEnd, Warn); }

	// @todo document
	virtual UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext )
	{
		return FactoryCreateNew(InClass, InParent, InName, Flags, Context, Warn);
	}
	virtual UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn ) {return NULL;}

	/**
	 * @return	true if this factory can deal with the file sent in.
	 */
	UNREALED_API virtual bool FactoryCanImport( const FString& Filename );


	// @todo document
	UNREALED_API virtual bool ImportUntypedBulkDataFromText(const TCHAR*& Buffer, FUntypedBulkData& BulkData);


	// @todo document
	UNREALED_API static UObject* StaticImportObject( UClass* Class, UObject* InOuter, FName Name, EObjectFlags Flags, const TCHAR* Filename=TEXT(""), UObject* Context=NULL, UFactory* Factory=NULL, const TCHAR* Parms=NULL, FFeedbackContext* Warn=GWarn, int32 MaxImportFileSize = 0xC100000 );
	UNREALED_API static UObject* StaticImportObject( UClass* Class, UObject* InOuter, FName Name, EObjectFlags Flags, bool& bOutOperationCanceled, const TCHAR* Filename=TEXT(""), UObject* Context=NULL, UFactory* Factory=NULL, const TCHAR* Parms=NULL, FFeedbackContext* Warn=GWarn, int32 MaxImportFileSize = 0xC100000 );

	/**
	 * UFactory::ValidForCurrentGame
	 *
	 * Search through list of valid game names
	 * If list is empty or current game name is in the list - return true
	 * Otherwise, name wasn't found in the list - return false
	 *
	 * author: superville
	 */
	UNREALED_API bool	ValidForCurrentGame();

	/** Creates a list of file extensions supported by this factory */
	UNREALED_API void GetSupportedFileExtensions( TArray<FString>& OutExtensions ) const;

	/** 
	 * Do clean up after importing is done. Will be called once for multi batch import
	 */
	virtual void CleanUp() {};

	/**
	 * Creates an asset if it doesn't exist. If it does exist then it overwrites it if possible. If it can not overwrite then it will delete and replace. If it can not delete, it will return NULL.
	 * 
	 * @param	InClass		the class of the asset to create
	 * @param	InPackage	the package to create this object within.
	 * @param	Name		the name to give the new asset. If no value (NAME_None) is specified, the asset will be given a unique name in the form of ClassName_#.
	 * @param	InFlags		the ObjectFlags to assign to the new asset.
	 * @param	Template	if specified, the property values from this object will be copied to the new object, and the new object's ObjectArchetype value will be set to this object.
	 *						If NULL, the class default object is used instead.
	 * @return	A pointer to a new asset of the specified type or null if the creation failed.
	 */
	UNREALED_API UObject* CreateOrOverwriteAsset(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InTemplate = NULL) const;
};
