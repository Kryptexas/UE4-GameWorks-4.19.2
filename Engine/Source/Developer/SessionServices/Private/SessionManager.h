// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionManager.h: Declares the FSessionManager interface.
=============================================================================*/

#pragma once


/**
 * Implement the session manager
 */
class FSessionManager
	: public TSharedFromThis<FSessionManager>
	, public ISessionManager
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InMessageBus - The message bus to use.
	 */
	FSessionManager( const IMessageBusRef& InMessageBus );
	
	/**
	 * Default destructor
	 */
	~FSessionManager( );

public:

	// Begin ISessionManager Interface

	virtual void AddOwner( const FString& InOwner ) OVERRIDE;

	virtual void GetSelectedInstances( TArray<ISessionInstanceInfoPtr>& OutInstances) const OVERRIDE;

	virtual const ISessionInfoPtr& GetSelectedSession( ) const OVERRIDE
	{
		return SelectedSession;
	}

	virtual void GetSessions( TArray<ISessionInfoPtr>& OutSessions ) const OVERRIDE;

	virtual bool IsInstanceSelected( const ISessionInstanceInfoRef& Instance ) const OVERRIDE
	{
		return ((Instance->GetOwnerSession() == SelectedSession) && !DeselectedInstances.Contains(Instance));
	}

	virtual FOnCanSelectSession& OnCanSelectSession( ) OVERRIDE
	{
		return CanSelectSessionDelegate;
	}

	virtual FOnSessionInstanceSelectionChanged& OnInstanceSelectionChanged( ) OVERRIDE
	{
		return InstanceSelectionChangedDelegate;
	}

	virtual FOnSessionLogReceived& OnLogReceived( ) OVERRIDE
	{
		return LogReceivedDelegate;
	}

	virtual FOnSelectedSessionChanged& OnSelectedSessionChanged( ) OVERRIDE
	{
		return SelectedSessionChangedDelegate;
	}

	virtual FSimpleMulticastDelegate& OnSessionsUpdated( ) OVERRIDE
	{
		return SessionsUpdatedDelegate;
	}

	virtual void RemoveOwner( const FString& InOwner ) OVERRIDE;

	virtual bool SelectSession( const ISessionInfoPtr& Session ) OVERRIDE;

	virtual bool SetInstanceSelected( const ISessionInstanceInfoPtr& Instance, bool Selected ) OVERRIDE;

	// End ISessionManager Interface

protected:

	/**
	 * Finds and removes sessions that haven't been updated in a while.
	 *
	 * @param Now - The current time.
	 */
	void FindExpiredSessions( const FDateTime& Now );

	/**
	 * Checks whether the specified owner is valid.
	 *
	 * @return true if the owner is valid, false otherwise.
	 */
	bool IsValidOwner( const FString& Owner );

	/**
	 * Refresh the sessions based on the owner filter list.
	 */
	void RefreshSessions();

	/**
	 * Pings all sessions on the network.
	 */
	void SendPing( );

private:

	// Handles FSessionServicePong messages.
	void HandleEnginePongMessage( const FEngineServicePong& Message, const IMessageContextRef& Context );

	// Callback for newly discovered instances.
	void HandleInstanceDiscovered( const ISessionInfoRef& Session, const ISessionInstanceInfoRef& Instance );

	// Callback received log entries.
	void HandleLogReceived( const ISessionInfoRef& Session, const ISessionInstanceInfoRef& Instance, const FSessionLogMessageRef& Message );

	// Handles FSessionServicePong messages.
	void HandleSessionPongMessage( const FSessionServicePong& Message, const IMessageContextRef& Context );

	// Callback for ticks from the ticker.
	bool HandleTicker( float DeltaTime );

private:

	// The address of the automation controller to where we can forward any automation workers found.
	FGuid AutomationControllerAddress;

	// Holds the list of currently selected instances.
	TArray<ISessionInstanceInfoPtr> DeselectedInstances;

	// Holds the time at which the last ping was sent.
	FDateTime LastPingTime;

	// Holds a pointer to the message bus.
	IMessageBusWeakPtr MessageBusPtr;

	// Holds the messaging endpoint.
	FMessageEndpointPtr MessageEndpoint;

	// Holds a reference to the currently selected session.
	ISessionInfoPtr SelectedSession;

	// Holds the collection of discovered sessions.
	TMap<FGuid, TSharedPtr<FSessionInfo> > Sessions;

private:

	// Holds a delegate to be invoked before a session is selected.
	FOnCanSelectSession CanSelectSessionDelegate;

	// Holds a delegate to be invoked when an instance changes its selection state.
	FOnSessionInstanceSelectionChanged InstanceSelectionChangedDelegate;

	// Holds a delegate to be invoked when the selected session received a log message.
	FOnSessionLogReceived LogReceivedDelegate;

	// Holds a delegate to be invoked when the selected session changed.
	FOnSelectedSessionChanged SelectedSessionChangedDelegate;

	// Holds a delegate to be invoked when the session list was updated.
	FSimpleMulticastDelegate SessionsUpdatedDelegate;

	// Holds a delegate to be invoked when the widget ticks.
	FTickerDelegate TickDelegate;

	// Owner filter list
	TArray<FString> FilteredOwners;
};
