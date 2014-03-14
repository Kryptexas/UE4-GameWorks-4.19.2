// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionLauncherModel.h: Declares the FSessionLauncherModel class.
=============================================================================*/

#pragma once


namespace ESessionLauncherTasks
{
	/**
	 * Enumerates available session launcher tasks.
	 */
	enum Type
	{
		NoTask = 0,
		Build,
		DeployRepository,
		QuickLaunch,
		Advanced,
		Preview,
		Progress,
		End,
	};
}


/**
 * Type definition for shared pointers to instances of FSessionLauncherModel.
 */
typedef TSharedPtr<class FSessionLauncherModel> FSessionLauncherModelPtr;

/**
 * Type definition for shared references to instances of FSessionLauncherModel.
 */
typedef TSharedRef<class FSessionLauncherModel> FSessionLauncherModelRef;


/**
 * Delegate type for launcher profile selection changes.
 *
 * The first parameter is the newly selected profile (or NULL if none was selected).
 * The second parameter is the old selected profile (or NULL if none was previous selected).
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSelectedLauncherProfileChanged, const ILauncherProfilePtr&, const ILauncherProfilePtr&)


/**
 * Implements the data model for the session launcher.
 */
class FSessionLauncherModel
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InDeviceProxyManager - The device proxy manager to use.
	 * @param InLauncher - The launcher to use.
	 * @param InProfileManager - The profile manager to use.
	 */
	FSessionLauncherModel( const ITargetDeviceProxyManagerRef& InDeviceProxyManager, const ILauncherRef& InLauncher, const ILauncherProfileManagerRef& InProfileManager )
		: DeviceProxyManager(InDeviceProxyManager)
		, Launcher(InLauncher)
		, ProfileManager(InProfileManager)
	{
		ProfileManager->OnProfileAdded().AddRaw(this, &FSessionLauncherModel::HandleProfileManagerProfileAdded);
		ProfileManager->OnProfileRemoved().AddRaw(this, &FSessionLauncherModel::HandleProfileManagerProfileRemoved);

		LoadConfig();
	}

	/**
	 * Destructor.
	 */
	~FSessionLauncherModel( )
	{
		ProfileManager->OnProfileAdded().RemoveAll(this);
		ProfileManager->OnProfileRemoved().RemoveAll(this);

		SaveConfig();
	}

public:

	/** 
	 * Deletes the active profile 
	 */
	void DeleteSelectedProfile( )
	{
		if (SelectedProfile.IsValid())
		{
			ProfileManager->RemoveProfile(SelectedProfile.ToSharedRef());
		}
	}

	/**
	 * Gets the model's device proxy manager.
	 *
	 * @return Device proxy manager.
	 */
	const ITargetDeviceProxyManagerRef& GetDeviceProxyManager( ) const
	{
		return DeviceProxyManager;
	}

	/**
	 * Gets the model's launcher.
	 *
	 * @return Launcher.
	 */
	const ILauncherRef& GetLauncher( ) const
	{
		return Launcher;
	}

	/**
	 * Get the model's profile manager.
	 *
	 * @return Profile manager.
	 */
	const ILauncherProfileManagerRef& GetProfileManager( ) const
	{
		return ProfileManager;
	}

	/**
	 * Gets the active profile.
	 *
	 * @return The active profile.
	 *
	 * @see GetProfiles
	 */
	const ILauncherProfilePtr& GetSelectedProfile( ) const
	{
		return SelectedProfile;
	}

	/**
	 * Sets the active profile.
	 *
	 * @param InProfile The profile to assign as active,, or NULL to deselect all.
	 */
	void SelectProfile( const ILauncherProfilePtr& Profile )
	{
		if (!Profile.IsValid() || ProfileManager->GetAllProfiles().Contains(Profile))
		{
			if (Profile != SelectedProfile)
			{
				ILauncherProfilePtr& PreviousProfile = SelectedProfile;
				SelectedProfile = Profile;

				ProfileSelectedDelegate.Broadcast(Profile, PreviousProfile);
			}
		}
	}

public:

	/**
	 * Returns a delegate to be invoked when the profile list has been modified.
	 *
	 * @return The delegate.
	 */
	FSimpleMulticastDelegate OnProfileListChanged( )
	{
		return ProfileListChangedDelegate;
	}

	/**
	 * Returns a delegate to be invoked when the selected profile changed.
	 *
	 * @return The delegate.
	 */
	FOnSelectedLauncherProfileChanged& OnProfileSelected( )
	{
		return ProfileSelectedDelegate;
	}

protected:

	/*
	 * Load all profiles from disk.
	 */
	void LoadConfig( )
	{
		ILauncherProfilePtr LoadedSelectedProfile;

		// restore the previous profile selection
		if (GConfig != NULL)
		{
			FString SelectedProfileString;

			if (GConfig->GetString(TEXT("FLauncherProfileManager"), TEXT("SelectedProfile"), SelectedProfileString, GEngineIni))
			{
				FGuid SelectedProfileId;

				if (FGuid::Parse(SelectedProfileString, SelectedProfileId) && SelectedProfileId.IsValid())
				{
					LoadedSelectedProfile = ProfileManager->GetProfile(SelectedProfileId);
				}
			}
		}

		// if no profile was selected, select the first profile
		if (!LoadedSelectedProfile.IsValid())
		{
			const TArray<ILauncherProfilePtr>& Profiles = ProfileManager->GetAllProfiles( );

			if (Profiles.Num() > 0)
			{
				LoadedSelectedProfile = Profiles[0];
			}
		}

		SelectProfile(LoadedSelectedProfile);
	}

	/*
	 * Saves all profiles to disk.
	 */
	void SaveConfig( )
	{
		FGuid SelectedProfileId;

		if (SelectedProfile.IsValid())
		{
			SelectedProfileId = SelectedProfile->GetId();
		}

		if (GConfig != NULL)
		{
			GConfig->SetString(TEXT("FSessionLauncherModel"), TEXT("SelectedProfile"), *SelectedProfileId.ToString(), GEngineIni);
		}
	}

private:

	// Callback for when a profile was added to the profile manager.
	void HandleProfileManagerProfileAdded( const ILauncherProfileRef& Profile )
	{
		ProfileListChangedDelegate.Broadcast();

		SelectProfile(Profile);
	}

	// Callback for when a profile was removed from the profile manager.
	void HandleProfileManagerProfileRemoved( const ILauncherProfileRef& Profile )
	{
		ProfileListChangedDelegate.Broadcast();

		if (Profile == SelectedProfile)
		{
			const TArray<ILauncherProfilePtr>& Profiles = ProfileManager->GetAllProfiles();

			if (Profiles.Num() > 0)
			{
				SelectProfile(Profiles[0]);
			}
			else
			{
				SelectProfile(NULL);
			}
		}
	}

private:

	// Holds a pointer to the device proxy manager.
	ITargetDeviceProxyManagerRef DeviceProxyManager;

	// Holds a pointer to the launcher.
	ILauncherRef Launcher;

	// Holds a pointer to the profile manager.
	ILauncherProfileManagerRef ProfileManager;

	// Holds a pointer to active profile.
	ILauncherProfilePtr SelectedProfile;

private:

	// Holds a delegate to be invoked when the profile list has been modified.
	FSimpleMulticastDelegate ProfileListChangedDelegate;

	// Holds a delegate to be invoked when the selected profile changed.
	FOnSelectedLauncherProfileChanged ProfileSelectedDelegate;
};
