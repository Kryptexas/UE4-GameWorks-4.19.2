// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#if PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS
	#include "AllowWindowsPlatformTypes.h"

	#include <winsock2.h>
	#include <ws2tcpip.h>

	typedef int32 SOCKLEN;

	#include "HideWindowsPlatformTypes.h"
#else
	#include <sys/socket.h>
#if PLATFORM_HAS_BSD_SOCKET_FEATURE_IOCTL
	#include <sys/ioctl.h>
#endif
	#include <netinet/in.h>
	#include <arpa/inet.h>
#if PLATFORM_HAS_BSD_SOCKET_FEATURE_GETHOSTNAME
	#include <netdb.h>
#endif
	#include <unistd.h>

	#define ioctlsocket ioctl
	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1

	typedef socklen_t SOCKLEN;
	typedef int32 SOCKET;
	typedef sockaddr_in SOCKADDR_IN;
	typedef struct timeval TIMEVAL;

	inline int32 closesocket(SOCKET Socket)
	{
		shutdown(Socket, SHUT_RDWR); // gracefully shutdown if connected
		return close(Socket);
	}

#endif

#include <errno.h>
