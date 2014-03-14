// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SocketsPrivatePCH.h"

#if PLATFORM_HAS_BSD_SOCKETS

#include "SocketSubsystemBSD.h"
#include "SocketsBSD.h"


class FSocketBSD* FSocketSubsystemBSD::InternalBSDSocketFactory(SOCKET Socket, ESocketType SocketType, const FString& SocketDescription)
{
	// return a new socket object
	return new FSocketBSD(Socket, SocketType, SocketDescription, this);
}

FSocket* FSocketSubsystemBSD::CreateSocket(const FName& SocketType, const FString& SocketDescription, bool bForceUDP)
{
	SOCKET Socket = INVALID_SOCKET;
	FSocket* NewSocket = NULL;
	switch (SocketType.GetIndex())
	{
	case NAME_DGram:
		// Creates a data gram (UDP) socket
		Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		NewSocket = (Socket != INVALID_SOCKET) ? InternalBSDSocketFactory(Socket, SOCKTYPE_Datagram, SocketDescription) : NULL;
		break;
	case NAME_Stream:
		// Creates a stream (TCP) socket
		Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		NewSocket = (Socket != INVALID_SOCKET) ? InternalBSDSocketFactory(Socket, SOCKTYPE_Streaming, SocketDescription) : NULL;
		break;
	default:
		break;
	}

	if (!NewSocket)
	{
		UE_LOG(LogSockets, Warning, TEXT("Failed to create socket %s [%s]"), *SocketType.ToString(), *SocketDescription);
	}

	return NewSocket;
}

/**
 * Cleans up a socket class
 *
 * @param Socket the socket object to destroy
 */
void FSocketSubsystemBSD::DestroySocket(FSocket* Socket)
{
	delete Socket;
}


ESocketErrors FSocketSubsystemBSD::GetHostByName(const ANSICHAR* HostName, FInternetAddr& OutAddr)
{
#if PLATFORM_HAS_BSD_SOCKET_FEATURE_GETHOSTNAME
	ESocketErrors ErrorCode = SE_NO_ERROR;
	// gethostbyname() touches a static object so lock for thread safety
	FScopeLock ScopeLock(&HostByNameSynch);
	hostent* HostEnt = gethostbyname(HostName);
	if (HostEnt != NULL)
	{
		// Make sure it's a valid type
		if (HostEnt->h_addrtype == PF_INET)
		{
			// Copy the data before letting go of the lock. This is safe only
			// for the copy locally. If another thread is reading this while
			// we are copying they will get munged data. This relies on the
			// consumer of this class to call the resolved() accessor before
			// attempting to read this data
			((FInternetAddrBSD&)OutAddr).SetIp(*(in_addr*)(*HostEnt->h_addr_list));
		}
		else
		{
			ErrorCode = SE_HOST_NOT_FOUND;
		}
	}
	else
	{
		ErrorCode = GetLastErrorCode();
	}
	return ErrorCode;
#else
	UE_LOG(LogSockets, Error, TEXT("Platform has no gethostbyname(), but did not override FSocketSubsystem::GetHostByName()"));
	return SE_NO_RECOVERY;
#endif
}

/**
 * Determines the name of the local machine
 *
 * @param HostName the string that receives the data
 *
 * @return true if successful, false otherwise
 */
bool FSocketSubsystemBSD::GetHostName(FString& HostName)
{
#if PLATFORM_HAS_BSD_SOCKET_FEATURE_GETHOSTNAME
	ANSICHAR Buffer[256];
	bool bRead = gethostname(Buffer,256) == 0;
	if (bRead == true)
	{
		HostName = Buffer;
	}
	return bRead;
#else
	UE_LOG(LogSockets, Error, TEXT("Platform has no gethostname(), but did not override FSocketSubsystem::GetHostName()"));
	return false;
#endif
}


/**
 *	Get the name of the socket subsystem
 * @return a string naming this subsystem
 */
const TCHAR* FSocketSubsystemBSD::GetSocketAPIName() const
{
	return TEXT("BSD");
}

/**
 *	Create a proper FInternetAddr representation
 * @param Address host address
 * @param Port host port
 */
TSharedRef<FInternetAddr> FSocketSubsystemBSD::CreateInternetAddr(uint32 Address, uint32 Port)
{
	TSharedRef<FInternetAddr> Result = MakeShareable(new FInternetAddrBSD);
	Result->SetIp(Address);
	Result->SetPort(Port);
	return Result;
}

ESocketErrors FSocketSubsystemBSD::GetLastErrorCode()
{
	return TranslateErrorCode(errno);
}


ESocketErrors FSocketSubsystemBSD::TranslateErrorCode(int32 Code)
{
	// @todo sockets: Windows for some reason doesn't seem to have all of the standard error messages,
	// but it overrides this function anyway - however, this 
#if !PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS

	// handle the generic -1 error
	if (Code == SOCKET_ERROR)
	{
		return GetLastErrorCode();
	}

	switch (Code)
	{
	case 0: return SE_NO_ERROR;
	case EINTR: return SE_EINTR;
	case EBADF: return SE_EBADF;
	case EACCES: return SE_EACCES;
	case EFAULT: return SE_EFAULT;
	case EINVAL: return SE_EINVAL;
	case EMFILE: return SE_EMFILE;
	case EWOULDBLOCK: return SE_EWOULDBLOCK;
	case EINPROGRESS: return SE_EINPROGRESS;
	case EALREADY: return SE_EALREADY;
	case ENOTSOCK: return SE_ENOTSOCK;
	case EDESTADDRREQ: return SE_EDESTADDRREQ;
	case EMSGSIZE: return SE_EMSGSIZE;
	case EPROTOTYPE: return SE_EPROTOTYPE;
	case ENOPROTOOPT: return SE_ENOPROTOOPT;
	case EPROTONOSUPPORT: return SE_EPROTONOSUPPORT;
	case ESOCKTNOSUPPORT: return SE_ESOCKTNOSUPPORT;
	case EOPNOTSUPP: return SE_EOPNOTSUPP;
	case EPFNOSUPPORT: return SE_EPFNOSUPPORT;
	case EAFNOSUPPORT: return SE_EAFNOSUPPORT;
	case EADDRINUSE: return SE_EADDRINUSE;
	case EADDRNOTAVAIL: return SE_EADDRNOTAVAIL;
	case ENETDOWN: return SE_ENETDOWN;
	case ENETUNREACH: return SE_ENETUNREACH;
	case ENETRESET: return SE_ENETRESET;
	case ECONNABORTED: return SE_ECONNABORTED;
	case ECONNRESET: return SE_ECONNRESET;
	case ENOBUFS: return SE_ENOBUFS;
	case EISCONN: return SE_EISCONN;
	case ENOTCONN: return SE_ENOTCONN;
	case ESHUTDOWN: return SE_ESHUTDOWN;
	case ETOOMANYREFS: return SE_ETOOMANYREFS;
	case ETIMEDOUT: return SE_ETIMEDOUT;
	case ECONNREFUSED: return SE_ECONNREFUSED;
	case ELOOP: return SE_ELOOP;
	case ENAMETOOLONG: return SE_ENAMETOOLONG;
	case EHOSTDOWN: return SE_EHOSTDOWN;
	case EHOSTUNREACH: return SE_EHOSTUNREACH;
	case ENOTEMPTY: return SE_ENOTEMPTY;
	case EUSERS: return SE_EUSERS;
	case EDQUOT: return SE_EDQUOT;
	case ESTALE: return SE_ESTALE;
	case EREMOTE: return SE_EREMOTE;
#if !PLATFORM_HAS_NO_EPROCLIM
	case EPROCLIM: return SE_EPROCLIM;
#endif
// 	case EDISCON: return SE_EDISCON;
// 	case SYSNOTREADY: return SE_SYSNOTREADY;
// 	case VERNOTSUPPORTED: return SE_VERNOTSUPPORTED;
// 	case NOTINITIALISED: return SE_NOTINITIALISED;
#if PLATFORM_HAS_BSD_SOCKET_FEATURE_GETHOSTNAME
	case HOST_NOT_FOUND: return SE_HOST_NOT_FOUND;
	case TRY_AGAIN: return SE_TRY_AGAIN;
	case NO_RECOVERY: return SE_NO_RECOVERY;
#endif
//	case NO_DATA: return SE_NO_DATA;
		// case : return SE_UDP_ERR_PORT_UNREACH; //@TODO Find it's replacement
	}
#endif

	UE_LOG(LogSockets, Warning, TEXT("Unhandled socket error! Error Code: %d"), Code);
	check(0);
	return SE_NO_ERROR;
}

#endif	//PLATFORM_HAS_BSD_SOCKETS
