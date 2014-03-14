// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Factory for sprites
 */

#include "PaperSpriteFactory.generated.h"

UCLASS()
class UPaperSpriteFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// Initial texture to create the sprite from (Can be NULL)
	class UTexture2D* InitialTexture;

	// UFactory interface
	virtual bool ConfigureProperties() OVERRIDE;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) OVERRIDE;
	// End of UFactory interface

protected:
	/** A pointer to the window that is asking the user to select a parent class */
//	TSharedPtr<SWindow> PickerWindow;
};
