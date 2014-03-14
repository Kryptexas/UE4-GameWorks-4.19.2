// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UdpMessagingModule.cpp: Implements the FUdpMessagingModule class.
=============================================================================*/

#include "UdpMessagingPrivatePCH.h"


#define LOCTEXT_NAMESPACE "FUdpMessagingModule"


/**
 * Implements the UdpMessagingModule module.
 */
class FUdpMessagingModule
	: public IModuleInterface
{
public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE
	{
		if (FApp::IsGame())
		{
			// don't use UDP transports in shipping and testing configurations
			if ((FApp::GetBuildConfiguration() == EBuildConfigurations::Shipping) || (FApp::GetBuildConfiguration() == EBuildConfigurations::Test))
			{
				return;
			}

			// don't use UDP transports in debug configurations unless supported and explicitly desired
			if (!FParse::Param(FCommandLine::Get(), TEXT("Messaging")) || !FPlatformMisc::SupportsMessaging() || !FPlatformProcess::SupportsMultithreading())
			{
				return;
			}
		}

		// load dependencies
		if (!FModuleManager::Get().LoadModule(TEXT("Networking")).IsValid())
		{
			GLog->Log(TEXT("Error: The required module 'Networking' failed to load. Plug-in 'UDP Messaging' cannot be used."));

			return;
		}

		// register settings
		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			FSettingsSectionDelegates SettingsDelegates;
			SettingsDelegates.ModifiedDelegate = FOnSettingsSectionModified::CreateRaw(this, &FUdpMessagingModule::HandleSettingsSaved);

			SettingsModule->RegisterSettings("Project", "Plugins", "UdpMessaging",
				LOCTEXT("UdpMessagingSettingsName", "UDP Messaging"),
				LOCTEXT("UdpMessagingSettingsDescription", "Configure the UDP Messaging plug-in."),
				TWeakObjectPtr<UObject>(UUdpMessagingSettings::StaticClass()->GetDefaultObject()),
				SettingsDelegates
			);
		}

		// start up services if desired
		const UUdpMessagingSettings& Settings = *GetDefault<UUdpMessagingSettings>();

		if (Settings.EnableTransport)
		{
			InitializeBridge();
		}

		if (Settings.EnableTunnel)
		{
			InitializeTunnel();
		}
	}

	virtual void ShutdownModule( ) OVERRIDE
	{
		// unregister settings
		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->UnregisterSettings("Project", "Plugins", "UdpMessaging");
		}

		// shut down services
		ShutdownBridge();
		ShutdownTunnel();
	}

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return false;
	}

	// End IModuleInterface interface

protected:

	/**
	 * Initializes the message bridge with the current settings.
	 */
	void InitializeBridge( )
	{
		ShutdownBridge();

		UUdpMessagingSettings* Settings = GetMutableDefault<UUdpMessagingSettings>();
		bool ResaveSettings = false;

		FIPv4Endpoint UnicastEndpoint;
		FIPv4Endpoint MulticastEndpoint;

		if (!FIPv4Endpoint::Parse(Settings->UnicastEndpoint, UnicastEndpoint))
		{
			GLog->Logf(TEXT("Warning: Invalid UDP Messaging UnicastEndpoint '%s' - binding to all local network adapters instead"), *Settings->UnicastEndpoint);
			UnicastEndpoint = FIPv4Endpoint::Any;
			Settings->UnicastEndpoint = UnicastEndpoint.ToText().ToString();
			ResaveSettings = true;
		}

		if (!FIPv4Endpoint::Parse(Settings->MulticastEndpoint, MulticastEndpoint))
		{
			GLog->Logf(TEXT("Warning: Invalid UDP Messaging MulticastEndpoint '%s' - using default endpoint '%s' instead"), *Settings->MulticastEndpoint, *UDP_MESSAGING_DEFAULT_MULTICAST_ENDPOINT.ToText().ToString());
			MulticastEndpoint = UDP_MESSAGING_DEFAULT_MULTICAST_ENDPOINT;
			Settings->MulticastEndpoint = MulticastEndpoint.ToText().ToString();
			ResaveSettings = true;
		}

		if (Settings->MulticastTimeToLive == 0)
		{
			Settings->MulticastTimeToLive = 1;
			ResaveSettings = true;		
		}

		if (ResaveSettings)
		{
			Settings->SaveConfig();
		}

		GLog->Logf(TEXT("UdpMessaging: Initializing bridge on interface %s to multicast group %s."), *UnicastEndpoint.ToText().ToString(), *MulticastEndpoint.ToText().ToString());

		MessageBridge = FMessageBridgeBuilder()
			.UsingJsonSerializer()
			.UsingTransport(MakeShareable(new FUdpMessageTransport(UnicastEndpoint, MulticastEndpoint, Settings->MulticastTimeToLive)));
	}

	/**
	 * Initializes the message tunnel with the current settings.
	 */
	void InitializeTunnel( )
	{
		ShutdownTunnel();

		UUdpMessagingSettings* Settings = GetMutableDefault<UUdpMessagingSettings>();
		bool ResaveSettings = false;

		FIPv4Endpoint UnicastEndpoint;
		FIPv4Endpoint MulticastEndpoint;

		if (!FIPv4Endpoint::Parse(Settings->TunnelUnicastEndpoint, UnicastEndpoint))
		{
			GLog->Logf(TEXT("Warning: Invalid UDP Tunneling UnicastEndpoint '%s' - binding to all local network adapters instead"), *Settings->UnicastEndpoint);
			UnicastEndpoint = FIPv4Endpoint::Any;
			Settings->UnicastEndpoint = UnicastEndpoint.ToText().ToString();
			ResaveSettings = true;
		}

		if (!FIPv4Endpoint::Parse(Settings->TunnelMulticastEndpoint, MulticastEndpoint))
		{
			GLog->Logf(TEXT("Warning: Invalid UDP Tunneling MulticastEndpoint '%s' - using default endpoint '%s' instead"), *Settings->MulticastEndpoint, *UDP_MESSAGING_DEFAULT_MULTICAST_ENDPOINT.ToText().ToString());
			MulticastEndpoint = UDP_MESSAGING_DEFAULT_MULTICAST_ENDPOINT;
			Settings->MulticastEndpoint = MulticastEndpoint.ToText().ToString();
			ResaveSettings = true;
		}

		if (ResaveSettings)
		{
			Settings->SaveConfig();
		}

		GLog->Logf(TEXT("UdpMessaging: Initializing tunnel on interface %s to multicast group %s."), *UnicastEndpoint.ToText().ToString(), *MulticastEndpoint.ToText().ToString());

		MessageTunnel = MakeShareable(new FUdpMessageTunnel(UnicastEndpoint, MulticastEndpoint));

		// initiate connections
		for (int32 EndpointIndex = 0; EndpointIndex < Settings->RemoteTunnelEndpoints.Num(); ++EndpointIndex)
		{
			FIPv4Endpoint RemoteEndpoint;

			if (FIPv4Endpoint::Parse(Settings->RemoteTunnelEndpoints[EndpointIndex], RemoteEndpoint))
			{
				MessageTunnel->Connect(RemoteEndpoint);
			}
			else
			{
				GLog->Logf(TEXT("Warning: Invalid UDP RemoteTunnelEndpoint '%s' - skipping"), *Settings->RemoteTunnelEndpoints[EndpointIndex]);
			}
		}
	}

	/**
	 * Shuts down the message bridge.
	 */
	void ShutdownBridge( )
	{
		if (MessageBridge.IsValid())
		{
			MessageBridge->Disable();
			FPlatformProcess::Sleep(0.1f);
			MessageBridge.Reset();
		}
	}

	/**
	 * Shuts down the message tunnel.
	 */
	void ShutdownTunnel( )
	{
		if (MessageTunnel.IsValid())
		{
			MessageTunnel->StopServer();
			MessageTunnel.Reset();
		}		
	}

private:

	// Handles saved settings.
	bool HandleSettingsSaved( )
	{
		const UUdpMessagingSettings& Settings = *GetDefault<UUdpMessagingSettings>();

		if (Settings.EnableTransport)
		{
			if (!MessageBridge.IsValid())
			{
				InitializeBridge();
			}
		}
		else
		{
			ShutdownBridge();
		}

		if (Settings.EnableTunnel)
		{
			if (!MessageTunnel.IsValid())
			{
				InitializeTunnel();
			}
		}
		else
		{
			ShutdownTunnel();
		}

		return true;
	}

private:

	// Holds the message bridge if present
	IMessageBridgePtr MessageBridge;

	// Holds the message tunnel if present
	IMessageTunnelPtr MessageTunnel;
};


IMPLEMENT_MODULE(FUdpMessagingModule, UdpMessaging);


#undef LOCTEXT_NAMESPACE
