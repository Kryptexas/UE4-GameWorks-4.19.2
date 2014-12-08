/**
 * Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Ipc;
using System.Diagnostics;

namespace DeploymentServer
{
    class Program
    {
        static void Main(string[] args)
        {
            if ((args.Length == 2) && (args[0].Equals("-iphonepackager")))
            {
				try
				{
					// We were run as a 'child' process, quit when our 'parent' process exits
					// There is no parent-child relationship WRT windows, it's self-imposed.
					int ParentPID = int.Parse(args[1]);

					IpcServerChannel Channel = new IpcServerChannel("iPhonePackager");
					ChannelServices.RegisterChannel(Channel, false);
					RemotingConfiguration.RegisterWellKnownServiceType(typeof(DeploymentImplementation), "DeploymentServer_PID" + ParentPID.ToString(), WellKnownObjectMode.Singleton);

					Process ParentProcess = Process.GetProcessById(ParentPID);
					while (!ParentProcess.HasExited)
					{
						System.Threading.Thread.Sleep(1000);
					}
				}
				catch (System.Exception)
				{
					
				}
            }
            else
            {
                // Run directly by some intrepid explorer
                Console.WriteLine("Note: This program should only be started by iPhonePackager");
                Console.WriteLine("  This program cannot be used on it's own.");


                DeploymentImplementation Deployer = new DeploymentImplementation();
                var DeviceList = Deployer.EnumerateConnectedDevices();
                foreach (var Device in DeviceList)
                {
                    Console.WriteLine("  - Found device named {0} of type {1} with UDID {2}", Device.DeviceName, Device.DeviceType, Device.UDID);
                }

                Console.WriteLine("Exiting.");
            }
        }
    }
}
