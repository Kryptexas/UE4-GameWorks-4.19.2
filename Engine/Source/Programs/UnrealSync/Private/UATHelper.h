// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

DECLARE_DELEGATE_RetVal_OneParam(bool, FOnUATMadeProgress, const FString&)

/**
 * Helper function to run UAT script with custom command line and with catching output.
 *
 * @param CommandLine Command line to run UAT with.
 * @param Output Output parameter to store std output of the process.
 *
 * @returns True if succeeded. False otherwise.
 */
bool RunUATLog(const FString& CommandLine, FString& Output);

/**
 * Helper function to run UAT script with custom command line and with catching output.
 *
 * @param CommandLine Command line to run UAT with.
 * @param OnUATMadeProgress Called when process make progress.
 *
 * @returns True if succeeded. False otherwise.
 */
bool RunUATProgress(const FString& CommandLine, FOnUATMadeProgress OnUATMadeProgress);