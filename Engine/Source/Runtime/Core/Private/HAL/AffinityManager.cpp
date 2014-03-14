// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"

#define MAXAFFINITYMANAGERTHREADS (32)

#define		MAKEAFFINITYMASK1(x) (1<<x)
#define		MAKEAFFINITYMASK2(x,y) ((1<<x)+(1<<y))
#define		MAKEAFFINITYMASK3(x,y,z) ((1<<x)+(1<<y)+(1<<z))
#define		MAKEAFFINITYMASK4(w,x,y,z) ((1<<w)+(1<<x)+(1<<y)+(1<<z))
#define		MAKEAFFINITYMASK5(v,w,x,y,z) ((1<<v)+(1<<w)+(1<<x)+(1<<y)+(1<<z))

struct AffinityData
{
	const TCHAR*	name;
	uint64			mask;
};

AffinityData gAffinityData[MAXAFFINITYMANAGERTHREADS] = {
	{ TEXT("MainGame"),						MAKEAFFINITYMASK1(0)	},

	{ TEXT("RenderingThread 0"),			MAKEAFFINITYMASK1(1)	},
	{ TEXT("RTHeartBeat 0"),				MAKEAFFINITYMASK1(4)	},
	{ TEXT("RenderingThread 1"),			MAKEAFFINITYMASK1(1)	},
	{ TEXT("RTHeartBeat 1"),				MAKEAFFINITYMASK1(4)	},
	{ TEXT("RenderingThread 2"),			MAKEAFFINITYMASK1(1)	},
	{ TEXT("RTHeartBeat 2"),				MAKEAFFINITYMASK1(4)	},

	{ TEXT("Wwise"),						MAKEAFFINITYMASK1(5)	},

	{ TEXT("PoolThread"),					MAKEAFFINITYMASK1(4)	},			// jobs?
	{ TEXT("TaskGraphThread"),				MAKEAFFINITYMASK3(0,2,3)	},			// jobs?
	{ TEXT("FUdpMessageProcessor"),			MAKEAFFINITYMASK1(4)	},
	{ TEXT("FUdpMessageBeacon"),			MAKEAFFINITYMASK1(4)	},
	{ TEXT("FUdpMessageProcessor"),			MAKEAFFINITYMASK1(4)	},
	{ TEXT("FUdpMessageProcessor.Sender"),	MAKEAFFINITYMASK1(4)	},
	{ TEXT("FMessageBus.Router"),			MAKEAFFINITYMASK1(4)	},
	{ TEXT("UdpMessageMulticastReceiver"),	MAKEAFFINITYMASK1(4)	},
	{ TEXT("UdpMessageUnicastReceiver"),	MAKEAFFINITYMASK1(4)	},
	{ TEXT("UdpMessageProcessor"),			MAKEAFFINITYMASK1(4)	},
	{ TEXT("ShaderCompilingThread"),		MAKEAFFINITYMASK1(4)	},
	{ TEXT("AsyncIOSystem"),				MAKEAFFINITYMASK1(4)	},
	{ TEXT("FDDCCleanup"),					MAKEAFFINITYMASK1(4)	},
	{ TEXT("FTargetDeviceDiscovery"),		MAKEAFFINITYMASK1(4)	},
	{ TEXT("ScreenSaverInhibitor"),			MAKEAFFINITYMASK1(4)	},
	{ TEXT("StatsThread"),					MAKEAFFINITYMASK1(5)	},

	// Should always go last
	{ nullptr,				0	},
};


uint64 AffinityManagerGetAffinity( const TCHAR* ThreadName )
{
	if( !GIsEditor )
	{
#if PLATFORM_XBOXONE
		for( int i = 0; i < MAXAFFINITYMANAGERTHREADS; i++ )
		{
			if( gAffinityData[i].name )
			{
				if( !TCString<TCHAR>::Stricmp( ThreadName, gAffinityData[i].name ) )
				{
					return gAffinityData[i].mask;
				}
			}
			else
			{
				break;
			}
		}
		UE_LOG(LogInit, Log, TEXT("Runnable thread %s hasn't been setup to have it's affinity managed, add it to your gAffinityData"), ThreadName );
#endif
	}
	return 0xffffffffffffffff;
}

