// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;
using System.Reflection;

namespace UnrealBuildTool
{
	/** Information about a module that needs to be passed to UnrealHeaderTool for code generation */
	public struct UHTModuleInfo
	{
		/** Module name */
		public string ModuleName;

		/** Module base directory */
		public string ModuleDirectory;

		/** Public UObject headers found in the Classes directory (legacy) */
		public List<FileItem> PublicUObjectClassesHeaders;

		/** Public headers with UObjects */
		public List<FileItem> PublicUObjectHeaders;

		/** Private headers with UObjects */
		public List<FileItem> PrivateUObjectHeaders;

		public override string ToString()
		{
			return ModuleName;
		}
	}

	public struct UHTManifest
	{
		public struct Module
		{
			public string       Name;
			public string       BaseDirectory;
			public string       OutputDirectory;
			public List<string> ClassesHeaders;
			public List<string> PublicHeaders;
			public List<string> PrivateHeaders;
			public bool         SaveExportedHeaders;

			public override string ToString()
			{
				return Name;
			}
		}

		public UHTManifest(bool InUseRelativePaths, UEBuildTarget Target, string InRootLocalPath, string InRootBuildPath, IEnumerable<UHTModuleInfo> ModuleInfo)
		{
			UseRelativePaths = InUseRelativePaths;
			RootLocalPath    = InRootLocalPath;
			RootBuildPath    = InRootBuildPath;

			Modules = ModuleInfo.Select(Info => new Module{
				Name                = Info.ModuleName,
				BaseDirectory       = Info.ModuleDirectory,
				OutputDirectory     = UEBuildModuleCPP.GetGeneratedCodeDirectoryForModule(Target, Info.ModuleDirectory, Info.ModuleName),
				ClassesHeaders      = Info.PublicUObjectClassesHeaders.Select((Header) => Header.AbsolutePath).ToList(),
				PublicHeaders       = Info.PublicUObjectHeaders       .Select((Header) => Header.AbsolutePath).ToList(),
				PrivateHeaders      = Info.PrivateUObjectHeaders      .Select((Header) => Header.AbsolutePath).ToList(),
				//@todo.Rocket: This assumes Engine/Source is a 'safe' folder name to check for
				SaveExportedHeaders = !UnrealBuildTool.RunningRocket() || !Info.ModuleDirectory.Contains("Engine\\Source\\")

			}).ToList();
		}

		public bool         UseRelativePaths; // Generate relative paths or absolute paths
		public string       RootLocalPath;    // The engine path on the local machine
		public string       RootBuildPath;    // The engine path on the build machine, if different (e.g. Mac/iOS builds)
		public List<Module> Modules;
	}


	/**
	 * This handles all running of the UnrealHeaderTool
	 */
	public class ExternalExecution
	{
		static ExternalExecution()
		{
		}

		static string GetHeaderToolPath()
		{
			UnrealTargetPlatform Platform = GetRuntimePlatform();
			string ExeExtension = UEBuildPlatform.GetBuildPlatform(Platform).GetBinaryExtension(UEBuildBinaryType.Executable);
			string HeaderToolExeName = "UnrealHeaderTool";
			string HeaderToolPath = Path.Combine("..", "Binaries", Platform.ToString(), HeaderToolExeName + ExeExtension);
			return HeaderToolPath;
		}
		
		/** Returns the name of platform UBT is running on */
		public static UnrealTargetPlatform GetRuntimePlatform()
		{
			PlatformID Platform = Environment.OSVersion.Platform;
			switch (Platform)
			{
			case PlatformID.Win32NT:
				return UnrealTargetPlatform.Win64;
			case PlatformID.Unix: // @todo Mac: Mono returns Unix instead of MacOSX, so we need some additional check if we start support for Linux or some other Unix
				return UnrealTargetPlatform.Mac;
			default:
				throw new BuildException("Unhandled runtime platform " + Platform);
			}
		}

		/// <summary>
		/// Gets the timestamp of CoreUObject.generated.inl file.
		/// </summary>
		/// <returns>Last write time of CoreUObject.generated.inl or DateTime.MaxValue if it doesn't exist.</returns>
		private static DateTime GetCoreGeneratedTimestamp(UEBuildTarget Target)
		{			
			DateTime Timestamp;
			if( UnrealBuildTool.RunningRocket() )
			{
				// In Rocket, we don't check the timestamps on engine headers.  Default to a very old date.
				Timestamp = DateTime.MinValue;
			}
			else
			{
				var CoreUObjectModule = (UEBuildModuleCPP)Target.GetModuleByName( "CoreUObject" );
				string CoreGeneratedFilename = Path.Combine(UEBuildModuleCPP.GetGeneratedCodeDirectoryForModule(Target, CoreUObjectModule.ModuleDirectory, CoreUObjectModule.Name), CoreUObjectModule.Name + ".generated.inl");
				if (File.Exists(CoreGeneratedFilename))
				{
					Timestamp = new FileInfo(CoreGeneratedFilename).LastWriteTime;
				}
				else
				{
					// Doesn't exist, so use a 'newer that everything' date to force rebuild headers.
					Timestamp = DateTime.MaxValue; 
				}
			}

			return Timestamp;
		}

		/**
		 * Checks the class header files and determines if generated UObject code files are out of date in comparison.
		 * @param UObjectModules	Modules that we generate headers for
		 * 
		 * @return					True if the code files are out of date
		 * */
		private static bool AreGeneratedCodeFilesOutOfDate(UEBuildTarget Target, List<UHTModuleInfo> UObjectModules)
		{
			bool bIsOutOfDate = false;

			// Get UnrealHeaderTool timestamp. If it's newer than generated headers, they need to be rebuilt too.
			string HeaderToolPath = GetHeaderToolPath();
			var HeaderToolTimestamp = DateTime.MaxValue;				// If UHT doesn't exist, force regenerate.
			if( File.Exists( HeaderToolPath ) )
			{
				HeaderToolTimestamp = new FileInfo(HeaderToolPath).LastWriteTime;
			}

			// Get CoreUObject.generated.inl timestamp.  If the source files are older than the CoreUObject generated code, we'll
			// need to regenerate code for the module
			var CoreGeneratedTimestamp = GetCoreGeneratedTimestamp(Target);

			foreach( var Module in UObjectModules )
			{
				// In Rocket, we skip checking timestamps for modules that don't exist within the project's directory
				if (UnrealBuildTool.RunningRocket())
				{
					// @todo Rocket: This could be done in a better way I'm sure
					if (!Utils.IsFileUnderDirectory( Module.ModuleDirectory, UnrealBuildTool.GetUProjectPath() ))
					{
						// Engine or engine plugin module - Rocket does not regenerate them so don't compare their timestamps
						continue;
					}
				}

				// Make sure we have an existing folder for generated code.  If not, then we definitely need to generate code!
				var GeneratedCodeDirectory = UEBuildModuleCPP.GetGeneratedCodeDirectoryForModule(Target, Module.ModuleDirectory, Module.ModuleName);
				var TestDirectory = (FileSystemInfo)new DirectoryInfo(GeneratedCodeDirectory);
				if( TestDirectory.Exists )
				{
					// Grab our special "Timestamp" file that we saved after the last set of headers were generated
					string TimestampFile = Path.Combine( GeneratedCodeDirectory, @"Timestamp" );
					var SavedTimestampFileInfo = (FileSystemInfo)new FileInfo(TimestampFile);
					if (SavedTimestampFileInfo.Exists)
					{
						// Make sure the last UHT run completed after UnrealHeaderTool.exe was compiled last, and after the CoreUObject headers were touched last.
						var SavedTimestamp = SavedTimestampFileInfo.LastWriteTime;
						if( SavedTimestamp.CompareTo(HeaderToolTimestamp) > 0 &&
							SavedTimestamp.CompareTo(CoreGeneratedTimestamp) > 0 )
						{
							// Iterate over our UObjects headers and figure out if any of them have changed
							var AllUObjectHeaders = new List<FileItem>();
							AllUObjectHeaders.AddRange( Module.PublicUObjectClassesHeaders );
							AllUObjectHeaders.AddRange( Module.PublicUObjectHeaders );
							AllUObjectHeaders.AddRange( Module.PrivateUObjectHeaders );
							DateTime NewestSourceFileTimestamp = DateTime.MinValue;
							foreach( var HeaderFile in AllUObjectHeaders )
							{
								var HeaderFileTimestamp = HeaderFile.Info.LastWriteTime;

								// Has the source header changed since we last generated headers successfully?
								if( SavedTimestamp.CompareTo( HeaderFileTimestamp ) < 0 )
								{
									bIsOutOfDate = true;
									break;
								}

								// Also check the timestamp on the directory the source file is in.  If the directory timestamp has
								// changed, new source files may have been added or deleted.  We don't know whether the new/deleted
								// files were actually UObject headers, but because we don't know all of the files we processed
								// in the previous run, we need to assume our generated code is out of date if the directory timestamp
								// is newer.
								var HeaderDirectoryTimestamp = new DirectoryInfo( Path.GetDirectoryName( HeaderFile.AbsolutePath ) ).LastWriteTime;
								if( SavedTimestamp.CompareTo( HeaderDirectoryTimestamp) < 0 )
								{
									bIsOutOfDate = true;
									break;
								}
							}
						}
						else
						{
							// Generated code is older UnrealHeaderTool.exe or CoreUObject headers.  Out of date!
							bIsOutOfDate = true;
						}
					}
					else
					{
						// Timestamp file was missing (possibly deleted/cleaned), so headers are out of date
						bIsOutOfDate = true;
					}
				}
				else
				{
					// Generated code directory is missing entirely!
					bIsOutOfDate = true;
				}

				// If even one module is out of date, we're done!  UHT does them all in one fell swoop.;
				if( bIsOutOfDate )
				{
					break;
				}
			}

			return bIsOutOfDate;
		}

		/** Updates the intermediate include directory timestamps of all the passed in UObject modules */
		private static void UpdateDirectoryTimestamps(UEBuildTarget Target, List<UHTModuleInfo> UObjectModules)
		{
			foreach( var Module in UObjectModules )
			{
				string GeneratedCodeDirectory = UEBuildModuleCPP.GetGeneratedCodeDirectoryForModule(Target, Module.ModuleDirectory, Module.ModuleName);
				var GeneratedCodeDirectoryInfo = new DirectoryInfo( GeneratedCodeDirectory );

				try
				{
					if (GeneratedCodeDirectoryInfo.Exists)
					{
						if (UnrealBuildTool.RunningRocket())
						{
							// If it is an Engine folder and we are building a rocket project do NOT update the timestamp!
							// @todo Rocket: This contains check is hacky/fragile
							string FullGeneratedCodeDirectory = GeneratedCodeDirectoryInfo.FullName;
							FullGeneratedCodeDirectory = FullGeneratedCodeDirectory.Replace("\\", "/");
							if (FullGeneratedCodeDirectory.Contains("Engine/Intermediate/Build"))
							{
								continue;
							}

							// Skip checking timestamps for engine plugin intermediate headers in Rocket
							PluginInfo Info = Plugins.GetPluginInfoForModule( Module.ModuleName );
							if( Info != null )
							{
								if( Info.LoadedFrom == PluginInfo.LoadedFromType.Engine )
								{
									continue;
								}
							}
						}

						// Touch the include directory since we have technically 'generated' the headers
						// However, the headers might not be touched at all since that would cause the compiler to recompile everything
						// We can't alter the directory timestamp directly, because this may throw exceptions when the directory is
						// open in visual studio or windows explorer, so instead we create a blank file that will change the timestamp for us
						string TimestampFile = GeneratedCodeDirectoryInfo.FullName + Path.DirectorySeparatorChar + @"Timestamp";

						if( !GeneratedCodeDirectoryInfo.Exists )
						{
							GeneratedCodeDirectoryInfo.Create();
						}
						File.Delete(TimestampFile);
						File.Create(TimestampFile);
					}
				}
				catch (Exception Exception)
				{
					throw new BuildException(Exception, "Couldn't touch header directories: " + Exception.Message);
				}
			}
		}

		/** Run an external exe (and capture the output), given the exe path and the commandline. */
		public static bool RunExternalExecutable(string ExePath, string Commandline)
		{
			bool bSuccess = false;

			var ExeInfo = new ProcessStartInfo(ExePath, Commandline);
			ExeInfo.UseShellExecute = false;
			ExeInfo.RedirectStandardOutput = true;
			using (var GameProcess = Process.Start(ExeInfo))
			{
				GameProcess.BeginOutputReadLine();
				GameProcess.OutputDataReceived += PrintProcessOutputAsync;
				GameProcess.WaitForExit();

				bSuccess = (GameProcess.ExitCode == 0);
			}
			return bSuccess;
		}

		/** Simple function to pipe output asynchronously */
		private static void PrintProcessOutputAsync(object Sender, DataReceivedEventArgs Event)
		{
			// DataReceivedEventHandler is fired with a null string when the output stream is closed.  We don't want to
			// print anything for that event.
			if( !String.IsNullOrEmpty( Event.Data ) )
			{
				Log.TraceInformation( Event.Data );
			}
		}



		/**
		 * Builds and runs the header tool and touches the header directories.
		 * Performs any early outs if headers need no changes, given the UObject modules, tool path, game name, and configuration
		 */
		public static bool ExecuteHeaderToolIfNecessary( UEBuildTarget Target, List<UHTModuleInfo> UObjectModules, string ModuleInfoFileName )
		{
			bool bSuccess = true;

			// We never want to try to execute the header tool when we're already trying to build it!
			var bIsBuildingUHT = Target.GetTargetName().Equals( "UnrealHeaderTool", StringComparison.InvariantCultureIgnoreCase );
			var BuildPlatform = UEBuildPlatform.GetBuildPlatform(Target.Platform);
			var CppPlatform = BuildPlatform.GetCPPTargetPlatform(Target.Platform);
			var ToolChain = UEToolChain.GetPlatformToolChain(CppPlatform);
			var RootLocalPath = Path.GetFullPath(ProjectFileGenerator.RootRelativePath);
			var Manifest = new UHTManifest(UnrealBuildTool.BuildingRocket() || UnrealBuildTool.RunningRocket(), Target, RootLocalPath, ToolChain.ConvertPath(RootLocalPath + '\\'), UObjectModules);

			// ensure the headers are up to date
			if (!bIsBuildingUHT && 
				(UEBuildConfiguration.bForceHeaderGeneration == true || AreGeneratedCodeFilesOutOfDate(Target, UObjectModules)))
			{
 				// Always build UnrealHeaderTool if header regeneration is required, unless we're running within a Rocket ecosystem
				if (UnrealBuildTool.RunningRocket() == false && UEBuildConfiguration.bDoNotBuildUHT == false)
				{
					// If it is out of date or not there it will be built.
					// If it is there and up to date, it will add 0.8 seconds to the build time.
					Log.TraceInformation("Building UnrealHeaderTool...");

					var UBTArguments = new StringBuilder();

					UBTArguments.Append( "UnrealHeaderTool" );

					// Which desktop platform do we need to compile UHT for?
					var UHTPlatform = UnrealTargetPlatform.Win64;
					if( Utils.IsRunningOnMono )
					{
						UHTPlatform = UnrealTargetPlatform.Mac;
					}
					UBTArguments.Append( " " + UHTPlatform.ToString() );

					// NOTE: We force Development configuration for UHT so that it runs quickly, even when compiling debug
					UBTArguments.Append( " " + UnrealTargetConfiguration.Development.ToString() );
				
					// NOTE: We disable mutex when launching UBT from within UBT to compile UHT
					UBTArguments.Append( " -NoMutex" );

					if (UnrealBuildTool.CommandLineContains("-noxge"))
					{
						UBTArguments.Append(" -noxge");
					}

					bSuccess = RunExternalExecutable( UnrealBuildTool.GetUBTPath(), UBTArguments.ToString() );
				}

				if( bSuccess )
				{
					var ActualTargetName = String.IsNullOrEmpty( Target.GetTargetName() ) ? "UE4" : Target.GetTargetName();
					Log.TraceInformation( "Parsing headers for {0}", ActualTargetName );

					string HeaderToolPath = GetHeaderToolPath();
					if (!File.Exists(HeaderToolPath))
					{
						throw new BuildException( "Unable to generate headers because UnrealHeaderTool binary was not found ({0}).", Path.GetFullPath( HeaderToolPath ) );
					}

					// Disable extensions when serializing to remove the $type fields
					Directory.CreateDirectory(Path.GetDirectoryName(ModuleInfoFileName));
					System.IO.File.WriteAllText(ModuleInfoFileName, fastJSON.JSON.Instance.ToJSON(Manifest, new fastJSON.JSONParameters{ UseExtensions = false }));

					string CmdLine = (UnrealBuildTool.HasUProjectFile()) ? "\"" + UnrealBuildTool.GetUProjectFile() + "\"" : Target.GetTargetName();
					CmdLine += " \"" + ModuleInfoFileName + "\" -LogCmds=\"loginit warning, logexit warning, logdatabase error\"";
					if (UnrealBuildTool.RunningRocket())
					{
						CmdLine += " -rocket -installed";
					}

					if (UEBuildConfiguration.bFailIfGeneratedCodeChanges)
					{
						CmdLine += " -FailIfGeneratedCodeChanges";
					}

					Stopwatch s = new Stopwatch();
					s.Start();
					bSuccess = RunExternalExecutable(ExternalExecution.GetHeaderToolPath(), CmdLine);
					s.Stop();
					
					if (bSuccess)
					{
						//if( BuildConfiguration.bPrintDebugInfo )
						{
							Log.TraceInformation( "Code generation finished for {0} and took {1}", ActualTargetName, (double)s.ElapsedMilliseconds/1000.0 );
						}

						// Now that UHT has successfully finished generating code, we need to update all cached FileItems in case their last write time has changed.
						// Otherwise UBT might not detect changes UHT made.
						DateTime StartTime = DateTime.UtcNow;
						FileItem.ResetInfos();
						double ResetDuration = (DateTime.UtcNow - StartTime).TotalSeconds;
						Log.TraceVerbose("FileItem.ResetInfos() duration: {0}s", ResetDuration);
					}
					else
					{
						Log.TraceInformation( "Error: Failed to generate code for {0}", ActualTargetName );
					}
				}
				else
				{
					bSuccess = false;
				}
			}
			else
			{
				Log.TraceVerbose( "Generated code is up to date." );
			}

			if (bSuccess)
			{
				// Allow generated code to be sync'd to remote machines if needed. This needs to be done even if UHT did not run because
				// generated headers include other generated headers using absolute paths which in case of building remotely are already
				// the remote machine absolute paths. Because of that parsing headers will not result in finding all includes properly.
				ToolChain.PostCodeGeneration(Target, Manifest);
				// touch the directories
				UpdateDirectoryTimestamps(Target, UObjectModules);
			}

			return bSuccess;
		}

	}
}
