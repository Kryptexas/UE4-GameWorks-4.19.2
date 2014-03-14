// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ClaimedTargetDevice.h: Declares and implements the FClaimedTargetDevice class.
=============================================================================*/

#pragma once


/**
 * Implements remote services for a specific target device.
 */
class FTargetDeviceService
	: public ITargetDeviceService
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InDeviceId - The identifier of the target device to expose.
	 * @param InDeviceName - The name of the device to expose.
	 * @param InMessageBus - The message bus to listen on for clients.
	 */
	FTargetDeviceService( const FTargetDeviceId& InDeviceId, const FString& InDeviceName, const IMessageBusRef& InMessageBus )
		: CachedDeviceName(InDeviceName)
		, DeviceId(InDeviceId)
		, Running(false)
		, Shared(false)
	{
		// initialize messaging
		MessageEndpoint = FMessageEndpoint::Builder(FName(*FString::Printf(TEXT("FTargetDeviceService (%s)"), *DeviceId.ToString())), InMessageBus)
			.Handling<FTargetDeviceClaimDenied>(this, &FTargetDeviceService::HandleClaimDeniedMessage)
			.Handling<FTargetDeviceClaimed>(this, &FTargetDeviceService::HandleClaimedMessage)
			.Handling<FTargetDeviceUnclaimed>(this, &FTargetDeviceService::HandleUnclaimedMessage)
			.Handling<FTargetDeviceServiceDeployCommit>(this, &FTargetDeviceService::HandleDeployCommitMessage)
			.Handling<FTargetDeviceServiceDeployFile>(this, &FTargetDeviceService::HandleDeployFileMessage)
			.Handling<FTargetDeviceServiceLaunchApp>(this, &FTargetDeviceService::HandleLaunchAppMessage)
			.Handling<FTargetDeviceServicePing>(this, &FTargetDeviceService::HandlePingMessage)
			.Handling<FTargetDeviceServicePowerOff>(this, &FTargetDeviceService::HandlePowerOffMessage)
			.Handling<FTargetDeviceServicePowerOn>(this, &FTargetDeviceService::HandlePowerOnMessage)
			.Handling<FTargetDeviceServiceReboot>(this, &FTargetDeviceService::HandleRebootMessage)
			.Handling<FTargetDeviceServiceRunExecutable>(this, &FTargetDeviceService::HandleRunExecutableMessage);

		if (MessageEndpoint.IsValid())
		{
			MessageEndpoint->Subscribe<FTargetDeviceClaimed>();
			MessageEndpoint->Subscribe<FTargetDeviceUnclaimed>();
			MessageEndpoint->Subscribe<FTargetDeviceServicePing>();
		}
	}

	/**
	 * Destructor.
	 */
	~FTargetDeviceService( )
	{
		Stop();
		FMessageEndpoint::SafeRelease(MessageEndpoint);
	}

public:

	// Begin ITargetDeviceService interface

	virtual bool CanStart( ) const OVERRIDE
	{
		ITargetDevicePtr TargetDevice = TargetDevicePtr.Pin();

		if (TargetDevice.IsValid())
		{
			return TargetDevice->IsConnected();
		}

		return false;
	}

	virtual const FString& GetClaimHost( ) OVERRIDE
	{
		return ClaimHost;
	}

	virtual const FString& GetClaimUser( ) OVERRIDE
	{
		return ClaimUser;
	}

	virtual ITargetDevicePtr GetDevice( ) OVERRIDE
	{
		return AcquireDevice();
	}

	virtual const FTargetDeviceId& GetDeviceId( ) const OVERRIDE
	{
		return DeviceId;
	}

	virtual FString GetCachedDeviceName( ) const OVERRIDE
	{
		ITargetDevicePtr TargetDevice = TargetDevicePtr.Pin();

		if (TargetDevice.IsValid())
		{
			return TargetDevice->GetName();
		}

		if (CachedDeviceName.IsEmpty())
		{
			return DeviceId.ToString();
		}

		return CachedDeviceName;
	}

	virtual bool IsRunning( ) const OVERRIDE
	{
		return (Running && TargetDevicePtr.IsValid());
	}

	virtual bool IsShared( ) const OVERRIDE
	{
		return (Running && Shared);
	}

	virtual void SetShared( bool InShared ) OVERRIDE
	{
		Shared = InShared;
	}

	virtual bool Start( ) OVERRIDE
	{
		if (!Running && MessageEndpoint.IsValid())
		{
			// initializes target device
			ITargetDevicePtr TargetDevice = AcquireDevice();

			if (!TargetDevice.IsValid())
			{
				return false;
			}

			CachedDeviceName = TargetDevice->GetName();

			// notify other services
			ClaimAddress = MessageEndpoint->GetAddress();
			ClaimHost = FPlatformProcess::ComputerName();
			ClaimUser = FPlatformProcess::UserName(false);

			MessageEndpoint->Publish(new FTargetDeviceClaimed(DeviceId.ToString(), ClaimHost, ClaimUser));

			Running = true;
		}

		return true;
	}

	virtual void Stop( ) OVERRIDE
	{
		if (Running)
		{
			// cache device details
			ITargetDevicePtr TargetDevice = TargetDevicePtr.Pin();

			if (TargetDevice.IsValid())
			{
				CachedDeviceName = TargetDevice->GetName();
			}

			// notify other services
			MessageEndpoint->Publish(new FTargetDeviceUnclaimed(DeviceId.ToString(), FPlatformProcess::ComputerName(), FPlatformProcess::UserName(false)));

			Running = false;
		}
	}

	// End ITargetDeviceService interface

protected:

	/**
	 * Acquires the target device, if needed.
	 *
	 * @return The target device.
	 */
	ITargetDevicePtr AcquireDevice( )
	{
		ITargetDevicePtr TargetDevice = TargetDevicePtr.Pin();

		if (!TargetDevice.IsValid())
		{
			TargetDevice = GetTargetPlatformManager()->FindTargetDevice(DeviceId);
			TargetDevicePtr = TargetDevice;
		}

		return TargetDevice;
	}

	/**
	 * Stores the specified file to deploy.
	 *
	 */
	bool StoreDeployedFile( FArchive* FileReader, const FString& TargetFileName  ) const
	{
		if (FileReader == NULL)
		{
			return false;
		}

		// create target file
		FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*TargetFileName);

		if (FileWriter == NULL)
		{
			return false;
		}

		FileReader->Seek(0);

		// copy file contents
		int64 BytesRemaining = FileReader->TotalSize();
		int32 BufferSize = 128 * 1024;

		if (BytesRemaining < BufferSize)
		{
			BufferSize = BytesRemaining;
		}

		void* Buffer = FMemory::Malloc(BufferSize);
	
		while (BytesRemaining > 0)
		{
			FileReader->Serialize(Buffer, BufferSize);
			FileWriter->Serialize(Buffer, BufferSize);

			BytesRemaining -= BufferSize;

			if (BytesRemaining < BufferSize)
			{
				BufferSize = BytesRemaining;
			}
		}

		// clean up
		FMemory::Free(Buffer);

		delete FileWriter;

		return true;
	}

private:

	// Callback for FTargetDeviceClaimDenied messages.
	void HandleClaimDeniedMessage( const FTargetDeviceClaimDenied& Message, const IMessageContextRef& Context )
	{
		if (Running && (Message.DeviceID == DeviceId.ToString()))
		{
			Stop();

			ClaimAddress = Context->GetSender();
			ClaimHost = Message.HostName;
			ClaimUser = Message.HostUser;
		}
	}

	// Callback for FTargetDeviceClaimDenied messages.
	void HandleClaimedMessage( const FTargetDeviceClaimed& Message, const IMessageContextRef& Context )
	{
		if (Message.DeviceID == DeviceId.ToString())
		{
			if (Running)
			{
				if (Context->GetSender() != MessageEndpoint->GetAddress())
				{
					MessageEndpoint->Send(new FTargetDeviceClaimDenied(DeviceId.ToString(), FPlatformProcess::ComputerName(), FPlatformProcess::UserName(false)), Context->GetSender());
				}
			}
			else
			{
				ClaimAddress = Context->GetSender();
				ClaimHost = Message.HostName;
				ClaimUser = Message.HostUser;
			}
		}
	}

	// Callback for FTargetDeviceClaimDropped messages.
	void HandleUnclaimedMessage( const FTargetDeviceUnclaimed& Message, const IMessageContextRef& Context )
	{
		if (Message.DeviceID == DeviceId.ToString())
		{
			if (Context->GetSender() == ClaimAddress)
			{
				ClaimAddress = FGuid();
				ClaimHost.Empty();
				ClaimUser.Empty();
			}
		}
	}

	// Callback for FTargetDeviceServiceDeployFile messages.
	void HandleDeployFileMessage( const FTargetDeviceServiceDeployFile& Message, const IMessageContextRef& Context )
	{
		if (!Running)
		{
			return;
		}

		IMessageAttachmentPtr Attachment = Context->GetAttachment();

		if (Attachment.IsValid())
		{
			FArchive* FileReader = Attachment->CreateReader();

			if (FileReader != NULL)
			{
				FString DeploymentFolder = FPaths::EngineIntermediateDir() / TEXT("Deploy") / Message.TransactionId.ToString();
				FString TargetPath = DeploymentFolder / Message.TargetFileName;

				StoreDeployedFile(FileReader, TargetPath);

				delete FileReader;
			}
		}
	}

	// Callback for FTargetDeviceServiceDeployCommit messages.
	void HandleDeployCommitMessage( const FTargetDeviceServiceDeployCommit& Message, const IMessageContextRef& Context )
	{
		if (!Running)
		{
			return;
		}

		ITargetDevicePtr TargetDevice = TargetDevicePtr.Pin();

		if (TargetDevice.IsValid())
		{
			FString SourceFolder = FPaths::EngineIntermediateDir() / TEXT("Deploy") / Message.TransactionId.ToString();
			FString OutAppId;

			bool Succeeded = TargetDevice->Deploy(SourceFolder, OutAppId);

			IFileManager::Get().DeleteDirectory(*SourceFolder, false, true);
			MessageEndpoint->Send(new FTargetDeviceServiceDeployFinished(OutAppId, Succeeded, Message.TransactionId), Context->GetSender());
		}
	}

	// Callback for FTargetDeviceServiceLaunchApp messages.
	void HandleLaunchAppMessage( const FTargetDeviceServiceLaunchApp& Message, const IMessageContextRef& Context )
	{
		if (!Running)
		{
			return;
		}

		ITargetDevicePtr TargetDevice = AcquireDevice();

		if (TargetDevice.IsValid())
		{
			uint32 ProcessId;
			bool Succeeded = TargetDevice->Launch(Message.AppID, (EBuildConfigurations::Type)Message.BuildConfiguration, EBuildTargets::Game, Message.Params, &ProcessId);

			if (MessageEndpoint.IsValid())
			{
				MessageEndpoint->Send(new FTargetDeviceServiceLaunchFinished(Message.AppID, ProcessId, Succeeded), Context->GetSender());
			}
		}
	}

	// Callback for FTargetDeviceServicePing messages.
	void HandlePingMessage( const FTargetDeviceServicePing& Message, const IMessageContextRef& Context )
	{
		if (!Running)
		{
			return;
		}

		if (Shared || (Message.HostUser == FPlatformProcess::UserName(false)))
		{
			ITargetDevicePtr TargetDevice = AcquireDevice();

			if (TargetDevice.IsValid())
			{
				FTargetDeviceServicePong* Message = new FTargetDeviceServicePong();

				Message->DeviceID = TargetDevice->GetId().ToString();
				Message->Name = TargetDevice->GetName();
				Message->Type = ETargetDeviceTypes::ToString(TargetDevice->GetDeviceType());
				Message->HostName = FPlatformProcess::ComputerName();
				Message->HostUser = FPlatformProcess::UserName(false);
				Message->Connected = TargetDevice->IsConnected();
				Message->Make = TEXT("@todo");
				Message->Model = TEXT("@todo");
				TargetDevice->GetUserCredentials(Message->DeviceUser, Message->DeviceUserPassword);
				Message->PlatformName = TargetDevice->GetTargetPlatform().PlatformName();
				Message->Shared = Shared;
				Message->SupportsMultiLaunch = TargetDevice->SupportsFeature(ETargetDeviceFeatures::MultiLaunch);
				Message->SupportsPowerOff = TargetDevice->SupportsFeature(ETargetDeviceFeatures::PowerOff);
				Message->SupportsPowerOn = TargetDevice->SupportsFeature(ETargetDeviceFeatures::PowerOn);
				Message->SupportsReboot = TargetDevice->SupportsFeature(ETargetDeviceFeatures::Reboot);

				MessageEndpoint->Send(Message, Context->GetSender());
			}
		}
	}

	// Callback for FTargetDeviceServicePowerOff messages.
	void HandlePowerOffMessage( const FTargetDeviceServicePowerOff& Message, const IMessageContextRef& Context )
	{
		if (!Running)
		{
			return;
		}

		ITargetDevicePtr TargetDevice = AcquireDevice();

		if (TargetDevice.IsValid())
		{
			TargetDevice->PowerOff(Message.Force);
		}
	}

	// Callback for FTargetDeviceServicePowerOn messages.
	void HandlePowerOnMessage( const FTargetDeviceServicePowerOn& Message, const IMessageContextRef& Context )
	{
		if (!Running)
		{
			return;
		}

		ITargetDevicePtr TargetDevice = AcquireDevice();

		if (TargetDevice.IsValid())
		{
			TargetDevice->PowerOn();
		}
	}

	// Callback for FTargetDeviceServiceReboot messages.
	void HandleRebootMessage( const FTargetDeviceServiceReboot& Message, const IMessageContextRef& Context )
	{
		if (!Running)
		{
			return;
		}

		ITargetDevicePtr TargetDevice = AcquireDevice();

		if (TargetDevice.IsValid())
		{
			TargetDevice->Reboot();
		}
	}

	// Callback for FTargetDeviceServiceRunExecutable messages.
	void HandleRunExecutableMessage( const FTargetDeviceServiceRunExecutable& Message, const IMessageContextRef& Context )
	{
		if (!Running)
		{
			return;
		}

		ITargetDevicePtr TargetDevice = AcquireDevice();

		if (TargetDevice.IsValid())
		{
			uint32 OutProcessId;
			bool Succeeded = TargetDevice->Run(Message.ExecutablePath, Message.Params, &OutProcessId);

			MessageEndpoint->Send(new FTargetDeviceServiceRunFinished(Message.ExecutablePath, OutProcessId, Succeeded), Context->GetSender());
		}
	}

private:

	// Caches the name of the device name that this services exposes.
	FString CachedDeviceName;

	// Holds the name of the host that has a claim on the device.
	FString ClaimHost;

	// Holds the message address of the target device service that has a claim on the device.
	FMessageAddress ClaimAddress;

	// Holds the name of the user that has a claim on the device.
	FString ClaimUser;

	// Holds the identifier of the device that this service exposes.
	const FTargetDeviceId DeviceId;

	// Holds the message endpoint.
	FMessageEndpointPtr MessageEndpoint;

	// Holds a flag indicating whether this service is currently running.
	bool Running;

	// Holds a flag indicating whether the device is shared with other users.
	bool Shared;

	// Holds a reference to the target device.
	ITargetDeviceWeakPtr TargetDevicePtr;
};
