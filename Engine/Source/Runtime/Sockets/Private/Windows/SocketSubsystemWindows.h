// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SocketSubsystem.h"
#include "BSDSockets/SocketSubsystemBSD.h"
#include "SocketSubsystemPackage.h"

/**
 * Windows specific socket subsystem implementation
 */
class FSocketSubsystemWindows : public FSocketSubsystemBSD
{
public:

	/**
	 * Default constructor.
	 */
	FSocketSubsystemWindows( ) :
		bTriedToInit(false)
	{ }

	/**
	 * Virtual destructor.
	 */
	virtual ~FSocketSubsystemWindows( ) { }


public:

	// Begin ISocketSubsystem interface

	virtual bool HasNetworkDevice( ) OVERRIDE;

	virtual ESocketErrors GetLastErrorCode( ) OVERRIDE;

	virtual bool GetLocalAdapterAddresses( TArray<TSharedPtr<FInternetAddr> >& OutAdresses ) OVERRIDE;

	virtual const TCHAR* GetSocketAPIName( ) const OVERRIDE;

	virtual bool Init( FString& Error ) OVERRIDE;

	virtual void Shutdown( ) OVERRIDE;

	virtual ESocketErrors TranslateErrorCode( int32 Code ) OVERRIDE;

	// End ISocketSubsystem interface


PACKAGE_SCOPE:

	/** 
	 * Singleton interface for this subsystem 
	 * @return the only instance of this subsystem
	 */
	static FSocketSubsystemWindows* Create( );

	/**
	 * Performs Windows specific socket clean up
	 */
	static void Destroy( );


protected:

	// Holds a flag indicating whether Init() has been called before or not.
	bool bTriedToInit;

	// Holds the single instantiation of this subsystem.
	static FSocketSubsystemWindows* SocketSingleton;
};