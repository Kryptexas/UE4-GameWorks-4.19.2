// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherProfile.h: Declares the FLauncherProfile class.
=============================================================================*/

#pragma once


/**
 * Implements a profile which controls the desired output of the Launcher
 */
class FLauncherProfile
	: public ILauncherProfile
{
public:

	/**
	 * Default constructor.
	 */
	FLauncherProfile( )
		: DefaultLaunchRole(MakeShareable(new FLauncherProfileLaunchRole()))
	{ }

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InProfileName - The name of the profile.
	 */
	FLauncherProfile( const FString& InProfileName )
		: DefaultLaunchRole(MakeShareable(new FLauncherProfileLaunchRole()))
		, Id(FGuid::NewGuid())
		, Name(InProfileName)
	{
		SetDefaults();
	}

	/**
	 * Destructor.
	 */
	~FLauncherProfile( ) { }


public:

	/**
	 * Gets the identifier of the device group to deploy to.
	 *
	 * This method is used internally by the profile manager to read the device group identifier after
	 * loading this profile from a file. The profile manager will use this identifier to locate the
	 * actual device group to deploy to.
	 *
	 * @return The device group identifier, or an invalid GUID if no group was set or deployment is disabled.
	 */
	const FGuid& GetDeployedDeviceGroupId( ) const
	{
		return DeployedDeviceGroupId;
	}


public:

	// Begin ILauncherProfile interface

	virtual void AddCookedCulture( const FString& CultureName ) OVERRIDE
	{
		CookedCultures.AddUnique(CultureName);

		Validate();
	}

	virtual void AddCookedMap( const FString& MapName ) OVERRIDE
	{
		CookedMaps.AddUnique(MapName);

		Validate();
	}

	virtual void AddCookedPlatform( const FString& PlatformName ) OVERRIDE
	{
		CookedPlatforms.AddUnique(PlatformName);

		Validate();
	}

	virtual void ClearCookedCultures( ) OVERRIDE
	{
		if (CookedCultures.Num() > 0)
		{
			CookedCultures.Reset();

			Validate();
		}
	}

	virtual void ClearCookedMaps( ) OVERRIDE
	{
		if (CookedMaps.Num() > 0)
		{
			CookedMaps.Reset();

			Validate();
		}
	}

	virtual void ClearCookedPlatforms( ) OVERRIDE
	{
		if (CookedPlatforms.Num() > 0)
		{
			CookedPlatforms.Reset();

			Validate();
		}
	}

	virtual ILauncherProfileLaunchRolePtr CreateLaunchRole( ) OVERRIDE
	{
		ILauncherProfileLaunchRolePtr Role = MakeShareable(new FLauncherProfileLaunchRole());
			
		LaunchRoles.Add(Role);

		Validate();

		return Role;
	}

	virtual EBuildConfigurations::Type GetBuildConfiguration( ) const OVERRIDE
	{
		return BuildConfiguration;
	}

	virtual EBuildConfigurations::Type GetCookConfiguration( ) const OVERRIDE
	{
		return CookConfiguration;
	}

	virtual ELauncherProfileCookModes::Type GetCookMode( ) const OVERRIDE
	{
		return CookMode;
	}

	virtual const FString& GetCookOptions( ) const OVERRIDE
	{
		return CookOptions;
	}

	virtual const TArray<FString>& GetCookedCultures( ) const OVERRIDE
	{
		return CookedCultures;
	}

	virtual const TArray<FString>& GetCookedMaps( ) const OVERRIDE
	{
		return CookedMaps;
	}

	virtual const TArray<FString>& GetCookedPlatforms( ) const OVERRIDE
	{
		return CookedPlatforms;
	}

	virtual const ILauncherProfileLaunchRoleRef& GetDefaultLaunchRole( ) const OVERRIDE
	{
		return DefaultLaunchRole;
	}

	virtual ILauncherDeviceGroupPtr GetDeployedDeviceGroup( ) const OVERRIDE
	{
		return DeployedDeviceGroup;
	}

	virtual ELauncherProfileDeploymentModes::Type GetDeploymentMode( ) const OVERRIDE
	{
		return DeploymentMode;
	}

    virtual bool GetForceClose() const OVERRIDE
    {
        return ForceClose;
    }
    
	virtual FGuid GetId( ) const OVERRIDE
	{
		return Id;
	}

	virtual ELauncherProfileLaunchModes::Type GetLaunchMode( ) const OVERRIDE
	{
		return LaunchMode;
	}

	virtual const TArray<ILauncherProfileLaunchRolePtr>& GetLaunchRoles( ) const OVERRIDE
	{
		return LaunchRoles;
	}

	virtual const int32 GetLaunchRolesFor( const FString& DeviceId, TArray<ILauncherProfileLaunchRolePtr>& OutRoles ) OVERRIDE
	{
		OutRoles.Empty();

		if (LaunchMode == ELauncherProfileLaunchModes::CustomRoles)
		{
			for (TArray<ILauncherProfileLaunchRolePtr>::TConstIterator It(LaunchRoles); It; ++It)
			{
				ILauncherProfileLaunchRolePtr Role = *It;

				if (Role->GetAssignedDevice() == DeviceId)
				{
					OutRoles.Add(Role);
				}
			}
		}
		else if (LaunchMode == ELauncherProfileLaunchModes::DefaultRole)
		{
			OutRoles.Add(DefaultLaunchRole);
		}

		return OutRoles.Num();
	}

	virtual FString GetName( ) const OVERRIDE
	{
		return Name;
	}

	virtual ELauncherProfilePackagingModes::Type GetPackagingMode( ) const OVERRIDE
	{
		return PackagingMode;
	}

	virtual FString GetPackageDirectory( ) const OVERRIDE
	{
		return PackageDir;
	}

	virtual FString GetProjectName( ) const OVERRIDE
	{
		if (!ProjectPath.IsEmpty())
		{
			return FPaths::GetBaseFilename(ProjectPath);
		}

		return FString();
	}

	virtual FString GetProjectBasePath() const OVERRIDE
	{
		if (!ProjectPath.IsEmpty())
		{
			FString OutPath = ProjectPath;
			FPaths::MakePathRelativeTo(OutPath, *FPaths::RootDir());
			return FPaths::GetPath(OutPath);
		}

		return FString();
	}

	virtual const FString& GetProjectPath( ) const OVERRIDE
	{
		return ProjectPath;
	}

    virtual uint32 GetTimeout() const OVERRIDE
    {
        return Timeout;
    }
    
	virtual bool HasValidationError( ELauncherProfileValidationErrors::Type Error ) const OVERRIDE
	{
		return ValidationErrors.Contains(Error);
	}

	virtual bool IsBuilding() const OVERRIDE
	{
		return BuildGame;
	}

	virtual bool IsCookingIncrementally( ) const OVERRIDE
	{
		return CookIncremental;
	}

	virtual bool IsCookingUnversioned( ) const OVERRIDE
	{
		return CookUnversioned;
	}

	virtual bool IsDeployablePlatform( const FString& PlatformName ) OVERRIDE
	{
		if (CookMode == ELauncherProfileCookModes::ByTheBook)
		{
			return CookedPlatforms.Contains(PlatformName);
		}

		return true;
	}

	virtual bool IsFileServerHidden( ) const OVERRIDE
	{
		return HideFileServerWindow;
	}

	virtual bool IsFileServerStreaming( ) const OVERRIDE
	{
		return DeployStreamingServer;
	}

	virtual bool IsPackingWithUnrealPak( ) const  OVERRIDE
	{
		return DeployWithUnrealPak;
	}

	virtual bool IsValidForLaunch( ) OVERRIDE
	{
		return (ValidationErrors.Num() == 0);
	}

	virtual void RemoveCookedCulture( const FString& CultureName ) OVERRIDE
	{
		CookedCultures.Remove(CultureName);

		Validate();
	}

	virtual void RemoveCookedMap( const FString& MapName ) OVERRIDE
	{
		CookedMaps.Remove(MapName);

		Validate();
	}

	virtual void RemoveCookedPlatform( const FString& PlatformName ) OVERRIDE
	{
		CookedPlatforms.Remove(PlatformName);

		Validate();
	}

	virtual void RemoveLaunchRole( const ILauncherProfileLaunchRoleRef& Role ) OVERRIDE
	{
		LaunchRoles.Remove(Role);

		Validate();
	}

	virtual bool Serialize( FArchive& Archive ) OVERRIDE
	{
		int32 Version = LAUNCHERSERVICES_PROFILEVERSION;

		Archive	<< Version;

		if (Version != LAUNCHERSERVICES_PROFILEVERSION)
		{
			return false;
		}

		if (Archive.IsSaving())
		{
			if (DeployedDeviceGroup.IsValid())
			{
				DeployedDeviceGroupId = DeployedDeviceGroup->GetId();
			}
			else
			{
				DeployedDeviceGroupId = FGuid();
			}
		}

		// IMPORTANT: make sure to bump LAUNCHERSERVICES_PROFILEVERSION when modifying this!
		Archive << Id
				<< Name
				<< BuildConfiguration
				<< ProjectPath
				<< CookConfiguration
				<< CookIncremental
				<< CookOptions
				<< CookMode
				<< CookUnversioned
				<< CookedCultures
				<< CookedMaps
				<< CookedPlatforms
				<< DeployStreamingServer
				<< DeployWithUnrealPak
				<< DeployedDeviceGroupId
				<< DeploymentMode
				<< HideFileServerWindow
				<< LaunchMode
				<< PackagingMode
				<< PackageDir
				<< BuildGame
                << ForceClose
                << Timeout;

		DefaultLaunchRole->Serialize(Archive);

		// serialize launch roles
		if (Archive.IsLoading())
		{
			DeployedDeviceGroup.Reset();
			LaunchRoles.Reset();
		}

		int32 NumLaunchRoles = LaunchRoles.Num();

		Archive << NumLaunchRoles;

		for (int32 RoleIndex = 0; RoleIndex < NumLaunchRoles; ++RoleIndex)
		{
			if (Archive.IsLoading())
			{
				LaunchRoles.Add(MakeShareable(new FLauncherProfileLaunchRole(Archive)));				
			}
			else
			{
				LaunchRoles[RoleIndex]->Serialize(Archive);
			}
		}

		Validate();

		return true;
	}

	virtual void SetDefaults( ) OVERRIDE
	{
		// default project settings
		if (FPaths::IsProjectFilePathSet())
		{
			ProjectPath = FPaths::GetProjectFilePath();
		}
		else if (FGameProjectHelper::IsGameAvailable(GGameName))
		{
			ProjectPath = FPaths::RootDir() / GGameName / GGameName + TEXT(".uproject");
		}
		else
		{
			ProjectPath = FString();
		}
		
		BuildConfiguration = FApp::GetBuildConfiguration();

		// default build settings
		BuildGame = false;

		// default cook settings
		CookConfiguration = FApp::GetBuildConfiguration();
		CookMode = ELauncherProfileCookModes::DoNotCook;
		CookOptions = FString();
		CookIncremental = true;
		CookUnversioned = false;
		CookedCultures.Reset();
		CookedCultures.Add(FInternationalization::GetCurrentCulture()->GetName());
		CookedMaps.Reset();
		CookedPlatforms.Reset();
        ForceClose = false;
        Timeout = 180;

/*		if (GetTargetPlatformManager()->GetRunningTargetPlatform() != NULL)
		{
			CookedPlatforms.Add(GetTargetPlatformManager()->GetRunningTargetPlatform()->PlatformName());
		}*/	

		// default deploy settings
		DeployedDeviceGroup.Reset();
		DeploymentMode = ELauncherProfileDeploymentModes::DoNotDeploy;
		DeployStreamingServer = false;
		DeployWithUnrealPak = false;
		DeployedDeviceGroupId = FGuid();
		HideFileServerWindow = false;

		// default launch settings
		LaunchMode = ELauncherProfileLaunchModes::DoNotLaunch;
		DefaultLaunchRole->SetCommandLine(FString());
		DefaultLaunchRole->SetInitialCulture(FInternationalization::GetCurrentCulture()->GetName());
		DefaultLaunchRole->SetInitialMap(FString());
		DefaultLaunchRole->SetName(TEXT("Default Role"));
		DefaultLaunchRole->SetInstanceType(ELauncherProfileRoleInstanceTypes::StandaloneClient);
		DefaultLaunchRole->SetVsyncEnabled(false);
		LaunchRoles.Reset();

		// default packaging settings
		PackagingMode = ELauncherProfilePackagingModes::DoNotPackage;

		Validate();
	}

	virtual void SetBuildGame(bool Build) OVERRIDE
	{
		if (BuildGame != Build)
		{
			BuildGame = Build;

			Validate();
		}
	}

	virtual void SetBuildConfiguration( EBuildConfigurations::Type Configuration ) OVERRIDE
	{
		if (BuildConfiguration != Configuration)
		{
			BuildConfiguration = Configuration;

			Validate();
		}
	}

	virtual void SetCookConfiguration( EBuildConfigurations::Type Configuration ) OVERRIDE
	{
		if (CookConfiguration != Configuration)
		{
			CookConfiguration = Configuration;

			Validate();
		}
	}

	virtual void SetCookMode( ELauncherProfileCookModes::Type Mode ) OVERRIDE
	{
		if (CookMode != Mode)
		{
			CookMode = Mode;

			Validate();
		}
	}

	virtual void SetDeployWithUnrealPak( bool UseUnrealPak ) OVERRIDE
	{
		if (DeployWithUnrealPak != UseUnrealPak)
		{
			DeployWithUnrealPak = UseUnrealPak;

			Validate();
		}
	}

	virtual void SetDeployedDeviceGroup( const ILauncherDeviceGroupPtr& DeviceGroup ) OVERRIDE
	{
		DeployedDeviceGroup = DeviceGroup;
		if (DeployedDeviceGroup.IsValid())
		{
			DeployedDeviceGroupId = DeployedDeviceGroup->GetId();
		}
		else
		{
			DeployedDeviceGroupId = FGuid();
		}

		Validate();
	}

	virtual void SetDeploymentMode( ELauncherProfileDeploymentModes::Type Mode ) OVERRIDE
	{
		if (DeploymentMode != Mode)
		{
			DeploymentMode = Mode;

			Validate();
		}
	}

    virtual void SetForceClose( bool Close ) OVERRIDE
    {
        if (ForceClose != Close)
        {
            ForceClose = Close;
            Validate();
        }
    }
    
	virtual void SetHideFileServerWindow( bool Hide ) OVERRIDE
	{
		HideFileServerWindow = Hide;
	}

	virtual void SetIncrementalCooking( bool Incremental ) OVERRIDE
	{
		if (CookIncremental != Incremental)
		{
			CookIncremental = Incremental;

			Validate();
		}
	}

	virtual void SetLaunchMode( ELauncherProfileLaunchModes::Type Mode ) OVERRIDE
	{
		if (LaunchMode != Mode)
		{
			LaunchMode = Mode;

			Validate();
		}
	}

	virtual void SetName( const FString& NewName ) OVERRIDE
	{
		if (Name != NewName)
		{
			Name = NewName;

			Validate();
		}
	}

	virtual void SetPackagingMode( ELauncherProfilePackagingModes::Type Mode ) OVERRIDE
	{
		if (PackagingMode != Mode)
		{
			PackagingMode = Mode;

			Validate();
		}
	}

	virtual void SetPackageDirectory( const FString& Dir ) OVERRIDE
	{
		if (PackageDir != Dir)
		{
			PackageDir = Dir;

			Validate();
		}
	}

	virtual void SetProjectPath( const FString& Path ) OVERRIDE
	{
		if (ProjectPath != Path)
		{
			ProjectPath = FPaths::ConvertRelativePathToFull(Path);
			CookedMaps.Reset();

			Validate();

			ProjectChangedDelegate.Broadcast();
		}
	}

	virtual void SetStreamingFileServer( bool Streaming ) OVERRIDE
	{
		if (DeployStreamingServer != Streaming)
		{
			DeployStreamingServer = Streaming;

			Validate();
		}
	}

    virtual void SetTimeout( uint32 InTime ) OVERRIDE
    {
        if (Timeout != InTime)
        {
            Timeout = InTime;
            
            Validate();
        }
    }
    
	virtual void SetUnversionedCooking( bool Unversioned ) OVERRIDE
	{
		if (CookUnversioned != Unversioned)
		{
			CookUnversioned = Unversioned;

			Validate();
		}
	}

	virtual bool SupportsEngineMaps( ) const OVERRIDE
	{
		return false;
	}

	virtual FOnProfileProjectChanged& OnProjectChanged() OVERRIDE
	{
		return ProjectChangedDelegate;
	}

	// End ILauncherProfile interface


protected:

	/**
	 * Validates the profile's current settings.
	 *
	 * Possible validation errors and warnings can be retrieved using the HasValidationError() method.
	 *
	 * @return true if the profile passed validation, false otherwise.
	 */
	void Validate( )
	{
		ValidationErrors.Reset();

		// Build: a build configuration must be selected
		if (BuildConfiguration == EBuildConfigurations::Unknown)
		{
			ValidationErrors.Add(ELauncherProfileValidationErrors::NoBuildConfigurationSelected);
		}

		// Build: a project must be selected
		if (ProjectPath.IsEmpty())
		{
			ValidationErrors.Add(ELauncherProfileValidationErrors::NoProjectSelected);
		}

		// Cook: at least one platform must be selected when cooking by the book
		if ((CookMode == ELauncherProfileCookModes::ByTheBook) && (CookedPlatforms.Num() == 0))
		{
			ValidationErrors.Add(ELauncherProfileValidationErrors::NoPlatformSelected);
		}

		// Cook: at least one culture must be selected when cooking by the book
		if ((CookMode == ELauncherProfileCookModes::ByTheBook) && (CookedCultures.Num() == 0))
		{
			ValidationErrors.Add(ELauncherProfileValidationErrors::NoCookedCulturesSelected);
		}

		// Deploy: a device group must be selected when deploying builds
		if ((DeploymentMode == ELauncherProfileDeploymentModes::CopyToDevice) && !DeployedDeviceGroupId.IsValid())
		{
			ValidationErrors.Add(ELauncherProfileValidationErrors::DeployedDeviceGroupRequired);
		}

		// Deploy: deployment by copying to devices requires cooking by the book
		if ((DeploymentMode == ELauncherProfileDeploymentModes::CopyToDevice) && (CookMode != ELauncherProfileCookModes::ByTheBook))
		{
			ValidationErrors.Add(ELauncherProfileValidationErrors::CopyToDeviceRequiresCookByTheBook);
		}

		// Deploy: deployment by copying a packaged build to devices requires a package dir
		if ((DeploymentMode == ELauncherProfileDeploymentModes::CopyRepository) && (PackageDir == TEXT("")))
		{
			ValidationErrors.Add(ELauncherProfileValidationErrors::NoPackageDirectorySpecified);
		}

		// Launch: custom launch roles are not supported yet
		if (LaunchMode == ELauncherProfileLaunchModes::CustomRoles)
		{
			ValidationErrors.Add(ELauncherProfileValidationErrors::CustomRolesNotSupportedYet);
		}

		// Launch: when using custom launch roles, all roles must have a device assigned
		if (LaunchMode == ELauncherProfileLaunchModes::CustomRoles)
		{
			for (int32 RoleIndex = 0; RoleIndex < LaunchRoles.Num(); ++RoleIndex)
			{
				if (LaunchRoles[RoleIndex]->GetAssignedDevice().IsEmpty())
				{
					ValidationErrors.Add(ELauncherProfileValidationErrors::NoLaunchRoleDeviceAssigned);
				
					break;
				}
			}
		}

		// Launch: when launching, all devices that the build is launched on must have content cooked for their platform
		if ((LaunchMode != ELauncherProfileLaunchModes::DoNotLaunch) && (CookMode != ELauncherProfileCookModes::OnTheFly))
		{
			// @todo ensure that launched devices have cooked content
		}
	}


private:

	// Holds the desired build configuration (only used if creating new builds).
	TEnumAsByte<EBuildConfigurations::Type> BuildConfiguration;

	// Holds the build mode.
	// Holds the build configuration name of the cooker.
	TEnumAsByte<EBuildConfigurations::Type> CookConfiguration;

	// Holds additional cooker command line options.
	FString CookOptions;

	// Holds the cooking mode.
	TEnumAsByte<ELauncherProfileCookModes::Type> CookMode;

	// Holds a flag indicating whether the game should be built
	bool BuildGame;

	// Holds a flag indicating whether only modified content should be cooked.
	bool CookIncremental;

	// Holds a flag indicating whether packages should be saved without a version.
	bool CookUnversioned;

	// This setting is used only if cooking by the book (only used if cooking by the book).
	TArray<FString> CookedCultures;

	// Holds the collection of maps to cook (only used if cooking by the book).
	TArray<FString> CookedMaps;

	// Holds the platforms to include in the build (only used if creating new builds).
	TArray<FString> CookedPlatforms;

	// Holds the default role (only used if launch mode is DefaultRole).
	ILauncherProfileLaunchRoleRef DefaultLaunchRole;

	// Holds a flag indicating whether a streaming file server should be used.
	bool DeployStreamingServer;

	// Holds a flag indicating whether content should be packaged with UnrealPak.
	bool DeployWithUnrealPak;

	// Holds the device group to deploy to.
	ILauncherDeviceGroupPtr DeployedDeviceGroup;

	// Holds the identifier of the deployed device group.
	FGuid DeployedDeviceGroupId;

	// Holds the deployment mode.
	TEnumAsByte<ELauncherProfileDeploymentModes::Type> DeploymentMode;

	// Holds a flag indicating whether the file server's console window should be hidden.
	bool HideFileServerWindow;

	// Holds the profile's unique identifier.
	FGuid Id;

	// Holds the launch mode.
	TEnumAsByte<ELauncherProfileLaunchModes::Type> LaunchMode;

	// Holds the launch roles (only used if launch mode is UsingRoles).
	TArray<ILauncherProfileLaunchRolePtr> LaunchRoles;

	// Holds the profile's name.
	FString Name;

	// Holds the packaging mode.
	TEnumAsByte<ELauncherProfilePackagingModes::Type> PackagingMode;

	// Holda the package directory
	FString PackageDir;

	// Holds the path to the Unreal project used by this profile.
	FString ProjectPath;

	// Holds the collection of validation errors.
	TArray<ELauncherProfileValidationErrors::Type> ValidationErrors;

    // Holds the time out time for the cook on the fly server
    uint32 Timeout;

    // Holds the close value for the cook on the fly server
    bool ForceClose;
        
private:

	// Holds a delegate to be invoked when changing the device group to deploy to.
	FOnLauncherProfileDeployedDeviceGroupChanged DeployedDeviceGroupChangedDelegate;

	// Holds a delegate to be invoked when the project has changed
	FOnProfileProjectChanged ProjectChangedDelegate;
};
