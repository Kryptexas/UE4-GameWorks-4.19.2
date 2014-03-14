// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AutomationDeviceClusterManager.cpp: Implements the FAutomationDeviceClusterManager class.
=============================================================================*/

#include "AutomationControllerPrivatePCH.h"

void FAutomationDeviceClusterManager::Reset()
{
	Clusters.Empty();
}


void FAutomationDeviceClusterManager::Add(const FMessageAddress& MessageAddress, const FString& DeviceTypeName, const FString& GameInstanceName)
{
	int32 TestClusterIndex;
	int32 TestDeviceIndex;
	//if we don't already know about this device
	if (!FindDevice(MessageAddress, TestClusterIndex, TestDeviceIndex))
	{
		//ensure the proper cluster exists
		int32 ClusterIndex = 0;
		for (; ClusterIndex < Clusters.Num(); ++ClusterIndex)
		{
			if (Clusters[ClusterIndex].DeviceTypeName == DeviceTypeName)
			{
				//found the cluster, now just append the device
				Clusters[ClusterIndex].Devices.Add(FDeviceState(MessageAddress, GameInstanceName));
				break;
			}
		}
		// if we didn't find the device type yet, add a new cluster and add this device
		if (ClusterIndex == Clusters.Num())
		{
			FDeviceCluster NewCluster;
			NewCluster.DeviceTypeName = DeviceTypeName;
			NewCluster.Devices.Add(FDeviceState(MessageAddress, GameInstanceName));
			Clusters.Add(NewCluster);
		}
	}
}


void FAutomationDeviceClusterManager::Remove(const FMessageAddress& MessageAddress)
{
	for (int32 ClusterIndex = 0; ClusterIndex < Clusters.Num(); ++ClusterIndex)
	{
		for (int32 DeviceIndex = Clusters[ClusterIndex].Devices.Num()-1; DeviceIndex >= 0; --DeviceIndex)
		{
			if (MessageAddress == Clusters[ClusterIndex].Devices[DeviceIndex].DeviceMessageAddress)
			{
				Clusters[ClusterIndex].Devices.RemoveAt(DeviceIndex);
			}
		}
	}
}


int32 FAutomationDeviceClusterManager::GetNumClusters() const
{
	return Clusters.Num();
}


int32 FAutomationDeviceClusterManager::GetTotalNumDevices() const
{
	int Total = 0;
	for (int32 ClusterIndex = 0; ClusterIndex < Clusters.Num(); ++ClusterIndex)
	{
		Total += Clusters[ClusterIndex].Devices.Num();
	}
	return Total;
}


int32 FAutomationDeviceClusterManager::GetNumDevicesInCluster(const int32 ClusterIndex) const
{
	check((ClusterIndex >= 0) && (ClusterIndex < Clusters.Num()));
	return Clusters[ClusterIndex].Devices.Num();
}

int32 FAutomationDeviceClusterManager::GetNumActiveDevicesInCluster(const int32 ClusterIndex) const
{
	check((ClusterIndex >= 0) && (ClusterIndex < Clusters.Num()));
	int32 ActiveDevices = 0;
	for ( int32 Index = 0; Index < Clusters[ ClusterIndex ].Devices.Num(); Index++ )
	{
		if ( Clusters[ ClusterIndex ].Devices[ Index ].IsDeviceAvailable )
		{
			ActiveDevices++;
		}
	}
	return ActiveDevices;
}

FString FAutomationDeviceClusterManager::GetClusterDeviceType(const int32 ClusterIndex) const
{
	check((ClusterIndex >= 0) && (ClusterIndex < Clusters.Num()));
	return Clusters[ClusterIndex].DeviceTypeName;
}

FString FAutomationDeviceClusterManager::GetClusterDeviceName(const int32 ClusterIndex, const int32 DeviceIndex) const
{
	check((ClusterIndex >= 0) && (ClusterIndex < Clusters.Num()));
	check((DeviceIndex >= 0) && (DeviceIndex < Clusters[ClusterIndex].Devices.Num()));
	return Clusters[ClusterIndex].Devices[DeviceIndex].GameInstanceName;
}

bool FAutomationDeviceClusterManager::FindDevice(const FMessageAddress& MessageAddress, int32& OutClusterIndex, int32& OutDeviceIndex)
{
	OutClusterIndex = INDEX_NONE;
	OutDeviceIndex = INDEX_NONE;
	for (int32 ClusterIndex = 0; ClusterIndex < Clusters.Num(); ++ClusterIndex)
	{
		for (int32 DeviceIndex = 0; DeviceIndex < Clusters[ClusterIndex].Devices.Num(); ++DeviceIndex)
		{
			//if network addresses match
			if (MessageAddress == Clusters[ClusterIndex].Devices[DeviceIndex].DeviceMessageAddress)
			{
				OutClusterIndex = ClusterIndex;
				OutDeviceIndex = DeviceIndex;
				return true;
			}
		}
	}
	return false;
}


FGuid FAutomationDeviceClusterManager::GetDeviceMessageAddress(const int32 ClusterIndex, const int32 DeviceIndex) const
{
	//verify cluster/device index
	check((ClusterIndex >= 0) && (ClusterIndex < Clusters.Num()));
	check((DeviceIndex >= 0) && (DeviceIndex < Clusters[ClusterIndex].Devices.Num()));
	return Clusters[ClusterIndex].Devices[DeviceIndex].DeviceMessageAddress;
}


TArray<FMessageAddress> FAutomationDeviceClusterManager::GetDevicesReservedForTest(const int32 ClusterIndex, TSharedPtr <IAutomationReport> Report)
{
	TArray<FMessageAddress> DeviceAddresses;

	check((ClusterIndex >= 0) && (ClusterIndex < Clusters.Num()));
	for (int32 DeviceIndex = 0; DeviceIndex < Clusters[ClusterIndex].Devices.Num(); ++DeviceIndex)
	{
		if (Clusters[ClusterIndex].Devices[DeviceIndex].Report == Report)
		{
			DeviceAddresses.Add(Clusters[ClusterIndex].Devices[DeviceIndex].DeviceMessageAddress);
		}
	}
	return DeviceAddresses;
}


TSharedPtr <IAutomationReport> FAutomationDeviceClusterManager::GetTest(const int32 ClusterIndex, const int32 DeviceIndex) const
{
	//verify cluster/device index
	check((ClusterIndex >= 0) && (ClusterIndex < Clusters.Num()));
	check((DeviceIndex >= 0) && (DeviceIndex < Clusters[ClusterIndex].Devices.Num()));
	return Clusters[ClusterIndex].Devices[DeviceIndex].Report;
}


void FAutomationDeviceClusterManager::SetTest(const int32 ClusterIndex, const int32 DeviceIndex, TSharedPtr <IAutomationReport> NewReport)
{
	//verify cluster/device index
	check((ClusterIndex >= 0) && (ClusterIndex < Clusters.Num()));
	check((DeviceIndex >= 0) && (DeviceIndex < Clusters[ClusterIndex].Devices.Num()));
	Clusters[ClusterIndex].Devices[DeviceIndex].Report = NewReport;
}


void FAutomationDeviceClusterManager::ResetAllDevicesRunningTest( const int32 ClusterIndex, IAutomationReportPtr InTest )
{	
	TArray<FMessageAddress> DeviceAddresses;

	check((ClusterIndex >= 0) && (ClusterIndex < Clusters.Num()));
	for (int32 DeviceIndex = 0; DeviceIndex < Clusters[ClusterIndex].Devices.Num(); ++DeviceIndex)
	{
		if( Clusters[ClusterIndex].Devices[DeviceIndex].Report == InTest )
		{
			Clusters[ClusterIndex].Devices[DeviceIndex].Report = NULL;
		}
	}
}


void FAutomationDeviceClusterManager::DisableDevice( const int32 ClusterIndex, const int32 DeviceIndex )
{
	//verify cluster/device index
	check((ClusterIndex >= 0) && (ClusterIndex < Clusters.Num()));
	check((DeviceIndex >= 0) && (DeviceIndex < Clusters[ClusterIndex].Devices.Num()));
	Clusters[ClusterIndex].Devices[DeviceIndex].IsDeviceAvailable = false;
}


bool FAutomationDeviceClusterManager::DeviceEnabled( const int32 ClusterIndex, const int32 DeviceIndex )
{
	//verify cluster/device index
	check((ClusterIndex >= 0) && (ClusterIndex < Clusters.Num()));
	check((DeviceIndex >= 0) && (DeviceIndex < Clusters[ClusterIndex].Devices.Num()));
	return Clusters[ClusterIndex].Devices[DeviceIndex].IsDeviceAvailable;
}


bool FAutomationDeviceClusterManager::HasActiveDevice()
{
	bool IsDeviceAvailable = false;
	for (int32 ClusterIndex = 0; ClusterIndex < Clusters.Num(); ++ClusterIndex)
	{
		for (int32 DeviceIndex = 0; DeviceIndex < Clusters[ClusterIndex].Devices.Num(); ++DeviceIndex)
		{
			//if network addresses match
			if ( Clusters[ClusterIndex].Devices[DeviceIndex].IsDeviceAvailable )
			{
				IsDeviceAvailable = true;
				break;
			}
		}
	}
	return IsDeviceAvailable;
}