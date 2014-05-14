// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using System.IO;
using System.Diagnostics;

namespace AutomationToolLauncher
{
	class Launcher
	{
        static void KillMe()
        {
            Process NewProcess = new Process();
            NewProcess.StartInfo.FileName = "TaskKill";
            NewProcess.StartInfo.Arguments = "/IM AutomationToolLauncher.exe /T /F";
            NewProcess.StartInfo.UseShellExecute = true;
            Console.WriteLine("Killing with: {0} {1}", NewProcess.StartInfo.FileName, NewProcess.StartInfo.Arguments);
            NewProcess.Start();
            System.Threading.Thread.Sleep(25000);
            Console.WriteLine("We should be dead.");
        }

		static int Main()
		{
			// Create application domain setup information.
			var Domaininfo = new AppDomainSetup();
			Domaininfo.ApplicationBase = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
			Domaininfo.ShadowCopyFiles = "true";

			// Create the application domain.			
			var Domain = AppDomain.CreateDomain("AutomationTool", AppDomain.CurrentDomain.Evidence, Domaininfo);
			// Execute assembly and pass through command line
			var CommandLine = AutomationTool.SharedUtils.ParseCommandLine();
			var UATExecutable = Path.Combine(Domaininfo.ApplicationBase, "AutomationTool.exe");
			// Default exit code in case UAT does not even start, otherwise we always return UAT's exit code.
			var ExitCode = 193;
            try
            {
                ExitCode = Domain.ExecuteAssembly(UATExecutable, CommandLine);
            }
            catch (Exception Ex)
            {
                Console.WriteLine("AutomationToolLauncher failed to execute the assembly");
                Console.WriteLine(Ex.Message);
								Console.WriteLine(Ex.StackTrace);
            }
            int Retry = 5;
            bool Unloaded = false;
            while (!Unloaded)
            {
                try
                {
                    // Unload the application domain.
                    AppDomain.Unload(Domain);
                    Unloaded = true;
                }
                catch (Exception Ex)
                {
                    Console.WriteLine(Ex.Message);
                    if (--Retry == 0)
                    {
                        KillMe();
                    }
                    Console.WriteLine("AutomationToolLauncher failed unload the app domain, retrying in 30s...");
                    System.Threading.Thread.Sleep(30000);
                }
            }
			Console.WriteLine("AutomationToolLauncher exiting with ExitCode={0}", ExitCode);
			return ExitCode;
		}
	}
}
