// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** 
 *	User defined enumerations:
 *	- have bIsNamespace set always to true (to comfortable handle names collisions)
 *	- always have the last '_MAX' enumerator, that cannot be changed by user
 *	- Full enumerator name has form: '<enumeration path>::<short, user defined enumerator name>'
 */

#include "UserDefinedEnum.generated.h"

/** 
 *	An Enumeration is a list of named values.
 */
UCLASS()
class ENGINE_API UUserDefinedEnum : public UEnum
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	uint32 UniqueNameIndex;
#endif //WITH_EDITORONLY_DATA

	/*
	 * Names stored in "DisplayName" meta data. They are duplicated here, 
	 * so functions like UKismetNodeHelperLibrary::GetEnumeratorUserFriendlyName can use them
	 * outside the editor. (When meta data are not loaded).
	 * To sync DisplayNames with meta-data use FEnumEditorUtils::EnsureAllDisplayNamesExist.
	 */
	UPROPERTY()
	TArray<FString> DisplayNames;

public:
	/**
	 * Generates full enum name give enum name.
	 * For UUserDefinedEnum full enumerator name has form: '<enumeration path>::<short, user defined enumerator name>'
	 *
	 * @param InEnumName Enum name.
	 * @return Full enum name.
	 */
	virtual FString GenerateFullEnumName(const TCHAR* InEnumName) const OVERRIDE;

	/*
	 *	Try to update value in enum variable after an enum's change.
	 *
	 *	@param EnumeratorIndex	old index
	 *	@return	new index
	 */
	virtual int32 ResolveEnumerator(FArchive& Ar, int32 EnumeratorIndex) const OVERRIDE;

	/**
	 * @return	The enum string at the specified index.
	 */
	virtual FString GetEnumString(int32 InIndex) const OVERRIDE;

#if WITH_EDITOR
	// Begin UObject interface
	virtual bool Rename(const TCHAR* NewName = NULL, UObject* NewOuter = NULL, ERenameFlags Flags = REN_None) OVERRIDE;
	virtual void PostDuplicate(bool bDuplicateForPIE) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	virtual void PostEditUndo() OVERRIDE;
	// End of UObject interface

	FString GenerateNewEnumeratorName();
#endif	// WITH_EDITOR
};
