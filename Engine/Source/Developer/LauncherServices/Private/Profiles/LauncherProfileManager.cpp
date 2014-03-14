// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherProfileManager.cpp: Implements the FLauncherProfileManager class.
=============================================================================*/

#include "LauncherServicesPrivatePCH.h"


/* ILauncherProfileManager structors
 *****************************************************************************/

FLauncherProfileManager::FLauncherProfileManager( )
{
	LoadDeviceGroups();
	LoadProfiles();
}


/* ILauncherProfileManager interface
 *****************************************************************************/

void FLauncherProfileManager::AddDeviceGroup( const ILauncherDeviceGroupRef& DeviceGroup )
{
	if (!DeviceGroups.Contains(DeviceGroup))
	{
		// replace the existing device group
		ILauncherDeviceGroupPtr ExistingGroup = GetDeviceGroup(DeviceGroup->GetId());

		if (ExistingGroup.IsValid())
		{
			RemoveDeviceGroup(ExistingGroup.ToSharedRef());
		}

		// add the new device group
		DeviceGroups.Add(DeviceGroup);
		SaveDeviceGroups();

		DeviceGroupAddedDelegate.Broadcast(DeviceGroup);
	}
}


ILauncherDeviceGroupRef FLauncherProfileManager::AddNewDeviceGroup( )
{
	ILauncherDeviceGroupRef  NewGroup = MakeShareable(new FLauncherDeviceGroup(FGuid::NewGuid(), FString::Printf(TEXT("New Group %d"), DeviceGroups.Num())));

	AddDeviceGroup(NewGroup);

	return NewGroup;
}


ILauncherProfileRef FLauncherProfileManager::AddNewProfile( )
{
	// find a unique name for the profile.
	int32 ProfileIndex = Profiles.Num();
	FString ProfileName = FString::Printf(TEXT("New Profile %d"), ProfileIndex);

	for (int32 Index = 0; Index < Profiles.Num(); ++Index)
	{
		if (Profiles[Index]->GetName() == ProfileName)
		{
			ProfileName = FString::Printf(TEXT("New Profile %d"), ++ProfileIndex);
			Index = -1;

			continue;
		}
	}

	// create and add the profile
	ILauncherProfileRef NewProfile = MakeShareable(new FLauncherProfile(ProfileName));

	AddProfile(NewProfile);

	return NewProfile;
}


void FLauncherProfileManager::AddProfile( const ILauncherProfileRef& Profile )
{
	if (!Profiles.Contains(Profile))
	{
		// replace the existing profile
		ILauncherProfilePtr ExistingProfile = GetProfile(Profile->GetId());

		if (ExistingProfile.IsValid())
		{
			RemoveProfile(ExistingProfile.ToSharedRef());
		}

		// add the new profile
		Profiles.Add(Profile);
		SaveProfiles();

		ProfileAddedDelegate.Broadcast(Profile);
	}
}


ILauncherProfilePtr FLauncherProfileManager::FindProfile( const FString& ProfileName )
{
	for (int32 ProfileIndex = 0; ProfileIndex < Profiles.Num(); ++ProfileIndex)
	{
		ILauncherProfilePtr Profile = Profiles[ProfileIndex];

		if (Profile->GetName() == ProfileName)
		{
			return Profile;
		}
	}

	return NULL;
}


const TArray<ILauncherDeviceGroupPtr>& FLauncherProfileManager::GetAllDeviceGroups( ) const
{
	return DeviceGroups;
}


const TArray<ILauncherProfilePtr>& FLauncherProfileManager::GetAllProfiles( ) const
{
	return Profiles;
}


ILauncherDeviceGroupPtr FLauncherProfileManager::GetDeviceGroup( const FGuid& GroupId ) const
{
	for (int32 GroupIndex = 0; GroupIndex < DeviceGroups.Num(); ++GroupIndex)
	{
		const ILauncherDeviceGroupPtr& Group = DeviceGroups[GroupIndex];

		if (Group->GetId() == GroupId)
		{
			return Group;
		}
	}

	return NULL;
}


ILauncherProfilePtr FLauncherProfileManager::GetProfile( const FGuid& ProfileId ) const
{
	for (int32 ProfileIndex = 0; ProfileIndex < Profiles.Num(); ++ProfileIndex)
	{
		ILauncherProfilePtr Profile = Profiles[ProfileIndex];

		if (Profile->GetId() == ProfileId)
		{
			return Profile;
		}
	}

	return NULL;
}


ILauncherProfilePtr FLauncherProfileManager::LoadProfile( FArchive& Archive )
{
	FLauncherProfile* Profile = new FLauncherProfile();

	if (Profile->Serialize(Archive))
	{
		Profile->SetDeployedDeviceGroup(GetDeviceGroup(Profile->GetDeployedDeviceGroupId()));

		return MakeShareable(Profile);
	}

	return NULL;
}


void FLauncherProfileManager::LoadSettings( )
{
	LoadDeviceGroups();
	LoadProfiles();
}


void FLauncherProfileManager::RemoveDeviceGroup( const ILauncherDeviceGroupRef& DeviceGroup )
{
	if (DeviceGroups.Remove(DeviceGroup) > 0)
	{
		SaveDeviceGroups();

		DeviceGroupRemovedDelegate.Broadcast(DeviceGroup);
	}
}


void FLauncherProfileManager::RemoveProfile( const ILauncherProfileRef& Profile )
{
	if (Profiles.Remove(Profile) > 0)
	{
		// delete the persisted profile on disk
		FString ProfileFileName = GetProfileFolder() / Profile->GetId().ToString() + TEXT(".ulp");

		// delete the profile
		IFileManager::Get().Delete(*ProfileFileName);
		SaveProfiles();

		ProfileRemovedDelegate.Broadcast(Profile);
	}
}


void FLauncherProfileManager::SaveProfile( const ILauncherProfileRef& Profile, FArchive& Archive )
{
	Profile->Serialize(Archive);
}


void FLauncherProfileManager::SaveSettings( )
{
	SaveDeviceGroups();
	SaveProfiles();
}


/* FLauncherProfileManager implementation
 *****************************************************************************/

void FLauncherProfileManager::LoadDeviceGroups( )
{
	if (GConfig != NULL)
	{
		FConfigSection* LoadedDeviceGroups = GConfig->GetSectionPrivate(TEXT("Launcher.DeviceGroups"), false, true, GEngineIni);

		if (LoadedDeviceGroups != NULL)
		{
			// parse the configuration file entries into device groups
			for (FConfigSection::TIterator It(*LoadedDeviceGroups); It; ++It)
			{
				if (It.Key() == TEXT("DeviceGroup"))
				{
					DeviceGroups.Add(ParseDeviceGroup(*It.Value()));
				}
			}
		}
	}
}


void FLauncherProfileManager::LoadProfiles( )
{
	TArray<FString> ProfileFileNames;

	IFileManager::Get().FindFiles(ProfileFileNames, *(GetProfileFolder() / TEXT("*.ulp")), true, false);
	
	for (TArray<FString>::TConstIterator It(ProfileFileNames); It; ++It)
	{
		FString ProfileFilePath = GetProfileFolder() / *It;
		FArchive* ProfileFileReader = IFileManager::Get().CreateFileReader(*ProfileFilePath);

		if (ProfileFileReader != NULL)
		{
			ILauncherProfilePtr LoadedProfile = LoadProfile(*ProfileFileReader);

			if (LoadedProfile.IsValid())
			{
				AddProfile(LoadedProfile.ToSharedRef());
			}
			else
			{
				IFileManager::Get().Delete(*ProfileFilePath);
			}

			delete ProfileFileReader;
		}
	}
}


ILauncherDeviceGroupPtr FLauncherProfileManager::ParseDeviceGroup( const FString& GroupString )
{
	TSharedPtr<FLauncherDeviceGroup> Result;

	FString GroupIdString;

	if (FParse::Value(*GroupString, TEXT("Id="), GroupIdString))
	{
		FGuid GroupId;

		if (!FGuid::Parse(GroupIdString, GroupId))
		{
			GroupId = FGuid::NewGuid();
		}

		FString GroupName;
		FParse::Value(*GroupString, TEXT("Name="), GroupName);

		FString DevicesString;
		FParse::Value(*GroupString, TEXT("Devices="), DevicesString);

		Result = MakeShareable(new FLauncherDeviceGroup(GroupId, GroupName));

		TArray<FString> DeviceList;
		DevicesString.ParseIntoArray(&DeviceList, TEXT(", "), false);

		for (int32 Index = 0; Index < DeviceList.Num(); ++Index)
		{
			Result->AddDevice(DeviceList[Index]);
		}
	}

	return Result;
}


void FLauncherProfileManager::SaveDeviceGroups( )
{
	if (GConfig != NULL)
	{
		GConfig->EmptySection(TEXT("Launcher.DeviceGroups"), GEngineIni);

		TArray<FString> DeviceGroupStrings;

		// create a string representation of all groups and their devices
		for (int32 GroupIndex = 0; GroupIndex < DeviceGroups.Num(); ++GroupIndex)
		{
			const ILauncherDeviceGroupPtr& Group = DeviceGroups[GroupIndex];
			const TArray<FString>& Devices = Group->GetDevices();

			FString DeviceListString;

			for (int32 DeviceIndex = 0; DeviceIndex < Devices.Num(); ++DeviceIndex)
			{
				if (DeviceIndex > 0)
				{
					DeviceListString += ", ";
				}

				DeviceListString += Devices[DeviceIndex];
			}

			FString DeviceGroupString = FString::Printf(TEXT("(Id=\"%s\", Name=\"%s\", Devices=\"%s\")" ), *Group->GetId().ToString(), *Group->GetName(), *DeviceListString);

			DeviceGroupStrings.Add(DeviceGroupString);
		}

		// save configuration
		GConfig->SetArray(TEXT("Launcher.DeviceGroups"), TEXT("DeviceGroup"), DeviceGroupStrings, GEngineIni);
		GConfig->Flush(false, GEngineIni);
	}
}


void FLauncherProfileManager::SaveProfiles( )
{
	for (TArray<ILauncherProfilePtr>::TIterator It(Profiles); It; ++It)
	{
		FString ProfileFileName = GetProfileFolder() / (*It)->GetId().ToString() + TEXT(".ulp");
		FArchive* ProfileFileWriter = IFileManager::Get().CreateFileWriter(*ProfileFileName);

		if (ProfileFileWriter != NULL)
		{
			SaveProfile((*It).ToSharedRef(), *ProfileFileWriter);

			delete ProfileFileWriter;
		}
	}
}
