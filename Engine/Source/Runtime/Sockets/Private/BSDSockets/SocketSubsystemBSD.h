// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SocketSubsystem.h"
#include "SocketSubsystemPackage.h"

#if PLATFORM_HAS_BSD_SOCKETS

#include "SocketSubsystemBSDPrivate.h"
#include "IPAddressBSD.h"


/**
 * Standard BSD specific socket subsystem implementation
 */
class FSocketSubsystemBSD
	: public ISocketSubsystem
{
public:

	// Begin ISocketSubsystem interface

	virtual TSharedRef<FInternetAddr> CreateInternetAddr( uint32 Address = 0, uint32 Port = 0 ) OVERRIDE;

	virtual class FSocket* CreateSocket( const FName& SocketType, const FString& SocketDescription, bool bForceUDP = false ) OVERRIDE;

	virtual void DestroySocket( class FSocket* Socket ) OVERRIDE;

	virtual ESocketErrors GetHostByName( const ANSICHAR* HostName, FInternetAddr& OutAddr ) OVERRIDE;

	virtual bool GetHostName( FString& HostName ) OVERRIDE;

	virtual ESocketErrors GetLastErrorCode( ) OVERRIDE;

	virtual bool GetLocalAdapterAddresses( TArray<TSharedPtr<FInternetAddr> >& OutAdresses ) OVERRIDE
	{
		bool bCanBindAll;

		OutAdresses.Add(GetLocalHostAddr(*GLog, bCanBindAll));

		return true;
	}

	virtual const TCHAR* GetSocketAPIName( ) const OVERRIDE;

	virtual bool RequiresChatDataBeSeparate( ) OVERRIDE
	{
		return false;
	}

	virtual bool RequiresEncryptedPackets( ) OVERRIDE
	{
		return false;
	}

	virtual ESocketErrors TranslateErrorCode( int32 Code ) OVERRIDE;

	// End ISocketSubsystem interface


protected:

	/**
	 * Allows a subsystem subclass to create a FSocketBSD sub class.
	 */
	virtual class FSocketBSD* InternalBSDSocketFactory( SOCKET Socket, ESocketType SocketType, const FString& SocketDescription );

	// allow BSD sockets to use this when creating new sockets from accept() etc
	friend FSocketBSD;

private:

	// Used to prevent multiple threads accessing the shared data.
	FCriticalSection HostByNameSynch;
};
#endif