// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperJsonImporterFactory.generated.h"

// Imports a paper animated sprite (and associated paper sprites and textures) from an Adobe Flash CS6 exported JSON file
UCLASS()
class UPaperJsonImporterFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// UFactory interface
	virtual FText GetToolTip() const OVERRIDE;
	virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn) OVERRIDE;
	// End of UFactory interface

protected:
	TSharedPtr<class FJsonObject> ParseJSON(const FString& FileContents, const FString& NameForErrors);

	// returns the value of Key in Item, or DefaultValue if the Key is missing or the wrong type
	FString ReadString(TSharedPtr<class FJsonObject> Item, const FString& Key, const FString& DefaultValue);

	// Returns the object named Key or NULL if it is missing or the wrong type
	TSharedPtr<class FJsonObject> ReadObject(TSharedPtr<class FJsonObject> Item, const FString& Key);

	// Returns the bool named Key or bDefaultIfMissing if it is missing or the wrong type (note: no way to determine errors!)
	bool ReadBoolean(const TSharedPtr<class FJsonObject> Item, const FString& Key, bool bDefaultIfMissing);

	// Returns true if the field named Key is a Number, populating Out_Value, or false if it is missing or the wrong type
	bool ReadFloatNoDefault(const TSharedPtr<class FJsonObject> Item, const FString& Key, float& Out_Value);

	// Returns true if the object named Key is a struct containing four floats (x,y,w,h), populating XY and WH with the values)
	bool ReadRectangle(const TSharedPtr<class FJsonObject> Item, const FString& Key, FVector2D& Out_XY, FVector2D& Out_WH);

	// Returns true if the object named Key is a struct containing two floats (w,h), populating WH with the values)
	bool ReadSize(const TSharedPtr<class FJsonObject> Item, const FString& Key, FVector2D& Out_WH);
};

