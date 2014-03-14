// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"

class SOURCECONTROLWINDOWS_API FSourceControlWindows
{
public:
	/**
	 * Display check in dialog for the specified packages
	 *
	 * @param	InPackageNames	Names of packages to check in
	 */
	static bool PromptForCheckin(const TArray<FString>& InPackageNames);

	/**
	 * Display file revision history for the provided packages
	 *
	 * @param	InPackageNames	Names of packages to display file revision history for
	 */
	static void DisplayRevisionHistory( const TArray<FString>& InPackagesNames );

	/**
	 * Prompt the user with a revert files dialog, allowing them to specify which packages, if any, should be reverted.
	 *
	 * @param	InPackageNames	Names of the packages to consider for reverting
	 *
	 * @return	true if the files were reverted; false if the user canceled out of the dialog
	 */
	static bool PromptForRevert(const TArray<FString>& InPackageNames );
};

