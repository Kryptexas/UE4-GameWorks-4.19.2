// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// TrueTypeFontFactory
//=============================================================================

#pragma once
#include "TrueTypeFontFactory.generated.h"

UCLASS(hidecategories=Object, collapsecategories)
class UTrueTypeFontFactory : public UFontFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()

	/** Import options for this font */
	UPROPERTY(EditAnywhere, editinline, Category=TrueTypeFontFactory, meta=(ToolTip="Import options for the font"))
	class UFontImportOptions* ImportOptions;

	/** True when the font dialog was shown for this factory during the non-legacy creation process */
	UPROPERTY()
	bool bPropertiesConfigured;

	/** True if a font was selected during the non-legacy creation process */
	UPROPERTY()
	bool bFontSelected;

	// Begin UObject Interface
	virtual void PostInitProperties() OVERRIDE;
	// End UObject Interface

	// Begin UFactory Interface
	virtual bool ConfigureProperties() OVERRIDE;
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) OVERRIDE;
	// Begin UFactory Interface	

	// Begin FReimportHandler interface
	virtual bool CanReimport( UObject* Obj, TArray<FString>& OutFilenames ) OVERRIDE;
	virtual void SetReimportPaths( UObject* Obj, const TArray<FString>& NewReimportPaths ) OVERRIDE;
	virtual EReimportResult::Type Reimport( UObject* Obj ) OVERRIDE;
	// End FReimportHandler interface

	/** Creates the import options structure for this font */
	void SetupFontImportOptions();

#if PLATFORM_WINDOWS
	/**
	 * Win32 Platform Only: Creates a single font texture using the Windows GDI
	 *
	 * @param Font (In/Out) The font we're operating with
	 * @param dc The device context configured to paint this font
	 * @param RowHeight Height of a font row in pixels
	 * @param TextureNum The current texture index
	 *
	 * @return Returns the newly created texture, if successful, otherwise NULL
	 */
	UTexture2D* CreateTextureFromDC( UFont* Font, HDC dc, int32 RowHeight, int32 TextureNum );
#endif

#if PLATFORM_WINDOWS || PLATFORM_MAC
	void* LoadFontFace( void* FTLibrary, int32 Height, FFeedbackContext* Warn, void** OutFontData );
	UTexture2D* CreateTextureFromBitmap( UFont* Font, uint8* BitmapData, int32 Height, int32 TextureNum );
	bool CreateFontTexture( UFont* Font, FFeedbackContext* Warn, const int32 NumResolutions, const int32 CharsPerPage,
		const TMap<TCHAR,TCHAR>& InverseMap, const TArray< float >& ResHeights );

#if PLATFORM_WINDOWS
	FString FindBitmapFontFile();
#endif

	/**
	 * Windows/Mac Platform Only: Imports a TrueType font
	 *
	 * @param Font (In/Out) The font object that we're importing into
	 * @param Warn Feedback context for displaying warnings and errors
	 * @param NumResolutions Number of resolution pages we should generate 
	 * @param ResHeights Font height for each resolution (one array entry per resolution)
	 *
	 * @return true if successful
	 */
	bool ImportTrueTypeFont( UFont* Font, FFeedbackContext* Warn, const int32 NumResolutions, const TArray< float >& ResHeights );
#endif
};



