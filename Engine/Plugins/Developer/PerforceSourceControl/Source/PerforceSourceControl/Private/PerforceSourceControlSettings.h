// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FPerforceSourceControlSettings
{
public:
	/** Get the Perforce host */
	const FString& GetHost() const;

	/** Set the Perforce host */
	void SetHost(const FString& InString);

	/** Get the Perforce username */
	const FString& GetUserName() const;

	/** Set the Perforce username */
	void SetUserName(const FString& InString);

	/** Get the Perforce workspace */
	const FString& GetWorkspace() const;

	/** Set the Perforce workspace */
	void SetWorkspace(const FString& InString);

	/** Load settings from ini file */
	void LoadSettings();

	/** Save settings to ini file */
	void SaveSettings() const;

private:
	/** A critical section for settings access */
	mutable FCriticalSection CriticalSection;

	/** The host and port for your Perforce server. Usage hostname:1234 */
	FString Host;

	/** Perforce username */
	FString UserName;

	/** Perforce workspace */
	FString Workspace;
};