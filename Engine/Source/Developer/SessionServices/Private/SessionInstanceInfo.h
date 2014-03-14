// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GameInstanceInfo.h: Declares the FSessionInstanceInfo class.
=============================================================================*/

#pragma once


/**
 * Implements a class to maintain all info related to a game instance in a session
 */
class FSessionInstanceInfo
	: public TSharedFromThis<FSessionInstanceInfo>
	, public ISessionInstanceInfo
{
public:

	/**
	 * Default constructor.
	 */
	FSessionInstanceInfo( ) { }

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InInstanceId - The instance's identifier.
	 * @param InOwner - The session that owns this instance.
	 * @param InMessageBus - The message bus to use.
	 */
	FSessionInstanceInfo( const FGuid& InInstanceId, const ISessionInfoRef& InOwner, const IMessageBusRef& InMessageBus )
		: EngineVersion(0)
		, InstanceId(InInstanceId)
		, Owner(InOwner)
	{
		MessageEndpoint = FMessageEndpoint::Builder("FSessionInstanceInfo", InMessageBus)
			.Handling<FSessionServiceLog>(this, &FSessionInstanceInfo::HandleSessionLogMessage);
	}

public:

	/**
	 * Updates this instance info with the data in the specified message.
	 *
	 * @param Message - The message containing engine information.
	 * @param Context - The message context.
	 */
	void UpdateFromMessage( const FEngineServicePong& Message, const IMessageContextRef& Context )
	{
		if (Message.InstanceId != InstanceId)
		{
			return;
		}

		CurrentLevel = Message.CurrentLevel;
		EngineAddress = Context->GetSender();
		EngineVersion = Message.EngineVersion;
		HasBegunPlay = Message.HasBegunPlay;
		WorldTimeSeconds = Message.WorldTimeSeconds;
		InstanceType = Message.InstanceType;
	}

	/**
	 * Updates this instance info with the data in the specified message.
	 *
	 * @param Message - The message containing instance information.
	 * @param Context - The message context.
	 */
	void UpdateFromMessage( const FSessionServicePong& Message, const IMessageContextRef& Context )
	{
		if (Message.InstanceId != InstanceId)
		{
			return;
		}

		if (MessageEndpoint.IsValid() && (ApplicationAddress != Context->GetSender()))
		{
			MessageEndpoint->Send(new FSessionServiceLogSubscribe(), Context->GetSender());
		}

		ApplicationAddress = Context->GetSender();
		BuildDate = Message.BuildDate;
		DeviceName = Message.DeviceName;
		InstanceName = Message.InstanceName;
		IsConsoleBuild = Message.IsConsoleBuild;
		PlatformName = Message.PlatformName;

		LastUpdateTime = FDateTime::UtcNow();
	}

public:	

	// Begin IGameInstanceInfo interface

	virtual void ExecuteCommand( const FString& CommandString )
	{
		if (MessageEndpoint.IsValid() && EngineAddress.IsValid())
		{
			MessageEndpoint->Send(new FEngineServiceExecuteCommand(CommandString, FPlatformProcess::UserName(false)), EngineAddress);
		}
	}

	virtual const FString GetBuildDate() const OVERRIDE
	{
		return BuildDate;
	}

	virtual const FString GetCurrentLevel() const OVERRIDE
	{
		return CurrentLevel;
	}

	virtual const FString GetDeviceName() const OVERRIDE
	{
		return DeviceName;
	}

	virtual int32 GetEngineVersion( ) const OVERRIDE
	{
		return EngineVersion;
	}

	virtual const FGuid GetInstanceId( ) const OVERRIDE
	{
		return InstanceId;
	}

	virtual const FString GetInstanceName() const OVERRIDE
	{
		return InstanceName;
	}

	virtual const FString GetInstanceType() const OVERRIDE
	{
		return InstanceType;
	}

	virtual FDateTime GetLastUpdateTime( ) OVERRIDE
	{
		return LastUpdateTime;
	}

	virtual const TArray<FSessionLogMessagePtr>& GetLog( ) OVERRIDE
	{
		return LogMessages;
	}

	virtual ISessionInfoPtr GetOwnerSession( ) OVERRIDE
	{
		return Owner.Pin();
	}

	virtual const FString& GetPlatformName( ) const OVERRIDE
	{
		return PlatformName;
	}

	virtual float GetWorldTimeSeconds( ) const OVERRIDE
	{
		return WorldTimeSeconds;
	}

	virtual const bool IsConsole() const OVERRIDE
	{
		return IsConsoleBuild;
	}

	virtual FOnSessionInstanceLogReceived& OnLogReceived( ) OVERRIDE
	{
		return LogReceivedDelegate;
	}

	virtual bool PlayHasBegun( ) const OVERRIDE
	{
		return HasBegunPlay;
	}

	virtual void Terminate( ) OVERRIDE
	{
		if (MessageEndpoint.IsValid() && EngineAddress.IsValid())
		{
			MessageEndpoint->Send(new FEngineServiceTerminate(FPlatformProcess::UserName(false)), EngineAddress);
		}
	}

	// End IGameInstanceInfo interface

private:

	// Handles FSessionServiceLog messages.
	void HandleSessionLogMessage( const FSessionServiceLog& Message, const IMessageContextRef& Context )
	{
		FSessionLogMessageRef LogMessage = MakeShareable(
			new FSessionLogMessage(
				InstanceId,
				InstanceName,
				Message.TimeSeconds,
				Message.Data,
				(ELogVerbosity::Type)Message.Verbosity,
				Message.Category
			)
		);

		LogMessages.Add(LogMessage);
		LogReceivedDelegate.Broadcast(AsShared(), LogMessage);
	}

private:

	// Holds the message bus address of the application instance.
	FMessageAddress ApplicationAddress;

	// Holds the instance's build date.
	FString BuildDate;

	// Holds the instance's current level.
	FString	CurrentLevel;

	// Holds the device name.
	FString DeviceName;

	// Holds the message bus address of the engine instance.
	FMessageAddress EngineAddress;

	// Holds the instance's engine version.
	int32 EngineVersion;

	// Holds a flag indicating whether the game has begun.
	bool HasBegunPlay;

	// Holds the instance identifier.
	FGuid InstanceId;

	// Holds the instance name.
	FString InstanceName;

	// Holds the instance type (i.e. game, editor etc.)
	FString InstanceType;

	// Holds a flag indicating whether this is a console build.
	bool IsConsoleBuild;

	// Holds the time at which the last pong was received.
	FDateTime LastUpdateTime;

	// Holds the collection of received log messages.
	TArray<FSessionLogMessagePtr> LogMessages;

	// Holds the message endpoint.
	FMessageEndpointPtr MessageEndpoint;

	// Holds a reference to the session that owns this instance.
	TWeakPtr<ISessionInfo> Owner;

	// Holds the name of the platform that the instance is running on.
	FString PlatformName;

	// Holds the instance's current game world time.
	float WorldTimeSeconds;

private:

	// Holds a delegate to be invoked when a log message was received from the remote session.
	FOnSessionInstanceLogReceived LogReceivedDelegate;
};
