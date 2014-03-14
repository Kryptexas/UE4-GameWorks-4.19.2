// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PerforceSourceControlPrivatePCH.h"
#include "PerforceSourceControlSettings.h"
#include "SourceControlHelpers.h"

namespace PerforceSettingsConstants
{

/** The section of the ini file we load our settings from */
static const FString SettingsSection = TEXT("PerforceSourceControl.PerforceSourceControlSettings");

}


const FString& FPerforceSourceControlSettings::GetHost() const
{
	FScopeLock ScopeLock(&CriticalSection);
	return Host;
}

void FPerforceSourceControlSettings::SetHost(const FString& InString)
{
	FScopeLock ScopeLock(&CriticalSection);
	Host = InString;
}

const FString& FPerforceSourceControlSettings::GetUserName() const
{
	FScopeLock ScopeLock(&CriticalSection);
	return UserName;
}

void FPerforceSourceControlSettings::SetUserName(const FString& InString)
{
	FScopeLock ScopeLock(&CriticalSection);
	UserName = InString;
}

const FString& FPerforceSourceControlSettings::GetWorkspace() const
{
	FScopeLock ScopeLock(&CriticalSection);
	return Workspace;
}

void FPerforceSourceControlSettings::SetWorkspace(const FString& InString)
{
	FScopeLock ScopeLock(&CriticalSection);
	Workspace = InString;
}

void FPerforceSourceControlSettings::LoadSettings()
{
	FScopeLock ScopeLock(&CriticalSection);
	const FString& IniFile = SourceControlHelpers::GetSettingsIni();
	GConfig->GetString(*PerforceSettingsConstants::SettingsSection, TEXT("Host"), Host, IniFile);
	GConfig->GetString(*PerforceSettingsConstants::SettingsSection, TEXT("UserName"), UserName, IniFile);
	GConfig->GetString(*PerforceSettingsConstants::SettingsSection, TEXT("Workspace"), Workspace, IniFile);
}

void FPerforceSourceControlSettings::SaveSettings() const
{
	FScopeLock ScopeLock(&CriticalSection);
	const FString& IniFile = SourceControlHelpers::GetSettingsIni();
	GConfig->SetString(*PerforceSettingsConstants::SettingsSection, TEXT("Host"), *Host, IniFile);
	GConfig->SetString(*PerforceSettingsConstants::SettingsSection, TEXT("UserName"), *UserName, IniFile);
	GConfig->SetString(*PerforceSettingsConstants::SettingsSection, TEXT("Workspace"), *Workspace, IniFile);
}