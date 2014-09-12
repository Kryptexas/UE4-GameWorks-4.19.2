// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Implements remote services for a specific target device.
 */
class FTargetDeviceService
	: public ITargetDeviceService
{
	struct FVariantSortCallback
	{
		FORCEINLINE bool operator()(const ITargetDeviceWeakPtr& A, const ITargetDeviceWeakPtr& B) const
		{
			ITargetDevicePtr APtr = A.Pin();
			ITargetDevicePtr BPtr = B.Pin();
			return APtr->GetTargetPlatform().GetVariantPriority() > BPtr->GetTargetPlatform().GetVariantPriority();
		}
	};

public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InDeviceName The name of the device to expose.
	 * @param InMessageBus The message bus to listen on for clients.
	 */
	FTargetDeviceService(const FString& InDeviceName, const IMessageBusRef& InMessageBus)
		: DeviceName(InDeviceName)
		, Running(false)
		, Shared(false)
	{
		// initialize messaging
		MessageEndpoint = FMessageEndpoint::Builder(FName(*FString::Printf(TEXT("FTargetDeviceService (%s)"), *DeviceName)), InMessageBus)
			.Handling<FTargetDeviceClaimDenied>(this, &FTargetDeviceService::HandleClaimDeniedMessage)
			.Handling<FTargetDeviceClaimed>(this, &FTargetDeviceService::HandleClaimedMessage)
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

	// ITargetDeviceService interface

	virtual void AddTargetDevice(ITargetDevicePtr InDevice) override;

	virtual void RemoveTargetDevice(ITargetDevicePtr InDevice) override;

	virtual int32 NumTargetDevices() override
	{
		return TargetDevicePtrs.Num();
	}

	virtual bool CanStart(FName InFlavor = NAME_None) const override
	{
		ITargetDevicePtr TargetDevice = GetDevice(InFlavor);

		if (TargetDevice.IsValid())
		{
			return TargetDevice->IsConnected();
		}

		return false;
	}

	virtual const FString& GetClaimHost() override
	{
		return ClaimHost;
	}

	virtual const FString& GetClaimUser() override
	{
		return ClaimUser;
	}

	virtual ITargetDevicePtr GetDevice(FName InVariant = NAME_None) const override
	{
		ITargetDevicePtr TargetDevice;
		if (InVariant == NAME_None)
		{
			TargetDevice = DefaultDevicePtr.Pin();
		}
		else
		{
			const ITargetDeviceWeakPtr * WeakTargetDevicePtr = TargetDevicePtrs.Find(InVariant);
			if (WeakTargetDevicePtr != NULL)
			{
				TargetDevice = WeakTargetDevicePtr->Pin();
			}
		}
		return TargetDevice;
	}

	virtual FString GetDeviceName() const override
	{
		return DeviceName;
	}

	virtual FName GetDevicePlatformName() const override
	{
		return DevicePlatformName;
	}

	virtual FString GetDevicePlatformDisplayName() const override
	{
		return DevicePlatformDisplayName;
	}

	virtual bool IsRunning() const override
	{
		return Running;
	}

	virtual bool IsShared( ) const override
	{
		return (Running && Shared);
	}

	virtual void SetShared(bool InShared) override
	{
		Shared = InShared;
	}

	virtual bool Start() override
	{
		if (!Running && MessageEndpoint.IsValid())
		{
			// notify other services
			ClaimAddress = MessageEndpoint->GetAddress();
			ClaimHost = FPlatformProcess::ComputerName();
			ClaimUser = FPlatformProcess::UserName(true);

			MessageEndpoint->Publish(new FTargetDeviceClaimed(DeviceName, ClaimHost, ClaimUser));

			Running = true;
		}

		return true;
	}

	virtual void Stop() override
	{
		if (Running)
		{
			// notify other services
			MessageEndpoint->Publish(new FTargetDeviceUnclaimed(DeviceName, FPlatformProcess::ComputerName(), FPlatformProcess::UserName(true)));

			Running = false;
		}
	}

protected:

	/**
	* Stores the specified file to deploy.
	*
	* @param FileReader The archive reader providing access to the file data.
	* @param TargetFileName The desired name of the file on the target device.
	*/
	bool StoreDeployedFile(FArchive* FileReader, const FString& TargetFileName) const
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
		if (Running && (Message.DeviceName == DeviceName))
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
		if (Message.DeviceName == DeviceName)
		{
			if (Running)
			{
				if (Context->GetSender() != MessageEndpoint->GetAddress())
				{
					MessageEndpoint->Send(new FTargetDeviceClaimDenied(DeviceName, FPlatformProcess::ComputerName(), FPlatformProcess::UserName(true)), Context->GetSender());
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
		if (Message.DeviceName == DeviceName)
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
	void HandleDeployFileMessage(const FTargetDeviceServiceDeployFile& Message, const IMessageContextRef& Context)
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
	void HandleDeployCommitMessage(const FTargetDeviceServiceDeployCommit& Message, const IMessageContextRef& Context)
	{
		if (!Running)
		{
			return;
		}

		ITargetDevicePtr TargetDevice = GetDevice(Message.Variant);
		if (TargetDevice.IsValid())
		{
			FString SourceFolder = FPaths::EngineIntermediateDir() / TEXT("Deploy") / Message.TransactionId.ToString();
			FString OutAppId;

			bool Succeeded = TargetDevice->Deploy(SourceFolder, OutAppId);

			IFileManager::Get().DeleteDirectory(*SourceFolder, false, true);
			MessageEndpoint->Send(new FTargetDeviceServiceDeployFinished(Message.Variant, OutAppId, Succeeded, Message.TransactionId), Context->GetSender());
		}
	}

	// Callback for FTargetDeviceServiceLaunchApp messages.
	void HandleLaunchAppMessage(const FTargetDeviceServiceLaunchApp& Message, const IMessageContextRef& Context)
	{
		if (!Running)
		{
			return;
		}

		ITargetDevicePtr TargetDevice = GetDevice(Message.Variant);
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
	void HandlePingMessage(const FTargetDeviceServicePing& InMessage, const IMessageContextRef& Context);

	// Callback for FTargetDeviceServicePowerOff messages.
	void HandlePowerOffMessage( const FTargetDeviceServicePowerOff& Message, const IMessageContextRef& Context )
	{
		if (!Running)
		{
			return;
		}

		ITargetDevicePtr TargetDevice = GetDevice(); // Any Device is fine here
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

		ITargetDevicePtr TargetDevice = GetDevice(); // Any Device is fine here
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

		ITargetDevicePtr TargetDevice = GetDevice(); // Any Device is fine here
		if (TargetDevice.IsValid())
		{
			TargetDevice->Reboot();
		}
	}

	// Callback for FTargetDeviceServiceRunExecutable messages.
	void HandleRunExecutableMessage(const FTargetDeviceServiceRunExecutable& Message, const IMessageContextRef& Context)
	{
		if (!Running)
		{
			return;
		}

		ITargetDevicePtr TargetDevice = GetDevice(Message.Variant);
		if (TargetDevice.IsValid())
		{
			uint32 OutProcessId;
			bool Succeeded = TargetDevice->Run(Message.ExecutablePath, Message.Params, &OutProcessId);

			MessageEndpoint->Send(new FTargetDeviceServiceRunFinished(Message.Variant, Message.ExecutablePath, OutProcessId, Succeeded), Context->GetSender());
		}
	}

private:

	// Caches the name of the device name that this services exposes.
	FString DeviceName;

	// Caches the platform name of the device name that this services exposes.
	FName DevicePlatformName;

	// Caches the platform name of the device name that this services exposes.
	FString DevicePlatformDisplayName;

	// Holds the name of the host that has a claim on the device.
	FString ClaimHost;

	// Holds the message address of the target device service that has a claim on the device.
	FMessageAddress ClaimAddress;

	// Holds the name of the user that has a claim on the device.
	FString ClaimUser;

	// Holds the message endpoint.
	FMessageEndpointPtr MessageEndpoint;

	// Holds a flag indicating whether this service is currently running.
	bool Running;

	// Holds a flag indicating whether the device is shared with other users.
	bool Shared;

	// Default target device used when no flavor is specified
	ITargetDeviceWeakPtr DefaultDevicePtr;
	
	// Map of all the Flavors for this Service
	TMap<FName, ITargetDeviceWeakPtr> TargetDevicePtrs;
};
