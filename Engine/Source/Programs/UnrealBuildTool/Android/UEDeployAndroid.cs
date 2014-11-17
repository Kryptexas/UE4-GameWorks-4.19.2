// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;

namespace UnrealBuildTool.Android
{
	class UEDeployAndroid : UEBuildDeploy
	{
		/**
		 *	Register the platform with the UEBuildDeploy class
		 */
		public override void RegisterBuildDeploy()
		{
			string NDKPath = Environment.GetEnvironmentVariable("ANDROID_HOME");

			// we don't have an NDKROOT specified
			if (String.IsNullOrEmpty(NDKPath))
			{
				Log.TraceVerbose("        Unable to register Android deployment class because the ANDROID_HOME environment variable isn't set or points to something that doesn't exist");
			}
			else
			{
				UEBuildDeploy.RegisterBuildDeploy(UnrealTargetPlatform.Android, this);
			}
		}

		/** Internal usage for GetApiLevel */
		private static List<string> PossibleApiLevels = null;

		/** Simple function to pipe output asynchronously */
		private static void ParseApiLevel(object Sender, DataReceivedEventArgs Event)
		{
			// DataReceivedEventHandler is fired with a null string when the output stream is closed.  We don't want to
			// print anything for that event.
			if (!String.IsNullOrEmpty(Event.Data))
			{
				string Line = Event.Data;
				if (Line.StartsWith("id:"))
				{
					// the line should look like: id: 1 or "android-19"
					string[] Tokens = Line.Split("\"".ToCharArray());
					if (Tokens.Length >= 2)
					{
						PossibleApiLevels.Add(Tokens[1]);
					}
				}
			}
		}

		static private bool bHasPrintedApiLevel = false;
		private static string GetSdkApiLevel()
		{
			// default to looking on disk for latest API level
			string Target = AndroidPlatform.AndroidSdkApiTarget;

			// if we want to use whatever version the ndk uses, then use that
			if (Target == "matchndk")
			{
				Target = AndroidToolChain.GetNdkApiLevel();
			}

			// run a command and capture output
			if (Target == "latest")
			{
				// we expect there to be one, so use the first one
				string AndroidCommandPath = Environment.ExpandEnvironmentVariables("%ANDROID_HOME%/tools/android.bat");

				var ExeInfo = new ProcessStartInfo(AndroidCommandPath, "list targets");
				ExeInfo.UseShellExecute = false;
				ExeInfo.RedirectStandardOutput = true;
				using (var GameProcess = Process.Start(ExeInfo))
				{
					PossibleApiLevels = new List<string>();
					GameProcess.BeginOutputReadLine();
					GameProcess.OutputDataReceived += ParseApiLevel;
					GameProcess.WaitForExit();
				}

				if (PossibleApiLevels != null && PossibleApiLevels.Count > 0)
				{
					Target = AndroidToolChain.GetLargestApiLevel(PossibleApiLevels.ToArray());
				}
				else
				{
					throw new BuildException("Can't make an APK an API installed (see \"android.bat list targets\")");
				}
			}

			if (!bHasPrintedApiLevel)
			{
				Console.WriteLine("Building with SDK API '{0}'", Target);
				bHasPrintedApiLevel = true;
			}
			return Target;
		}

		private static string GetAntPath()
		{
			// look up an ANT_HOME env var
			string AntHome = Environment.GetEnvironmentVariable("ANT_HOME");
			if (!string.IsNullOrEmpty(AntHome) && Directory.Exists(AntHome))
			{
				string AntPath = AntHome + "/bin/ant.bat";
				// use it if found
				if (File.Exists(AntPath))
				{
					return AntPath;
				}
			}

			// otherwise, look in the eclipse install for the ant plugin (matches the unzipped Android ADT from Google)
			string PluginDir = Environment.ExpandEnvironmentVariables("%ANDROID_HOME%/../eclipse/plugins");
			if (Directory.Exists(PluginDir))
			{
				string[] Plugins = Directory.GetDirectories(PluginDir, "org.apache.ant*");
				// use the first one with ant.bat
				if (Plugins.Length > 0)
				{
					foreach (string PluginName in Plugins)
					{
						string AntPath = PluginName + "/bin/ant.bat";
						// use it if found
						if (File.Exists(AntPath))
						{
							return AntPath;
						}
					}
				}
			}

			throw new BuildException("Unable to find ant.bat (via %ANT_HOME% or %ANDROID_HOME%/../eclipse/plugins/org.apache.ant*");
		}


		private static void CopyFileDirectory(string SourceDir, string DestDir, Dictionary<string,string> Replacements)
		{
			if (!Directory.Exists(SourceDir))
			{
				return;
			}

			string[] Files = Directory.GetFiles(SourceDir, "*.*", SearchOption.AllDirectories);
			foreach (string Filename in Files)
			{
				// skip template files
				if (Path.GetExtension(Filename) == ".template")
				{
					continue;
				}

				// make the dst filename with the same structure as it was in SourceDir
				string DestFilename = Path.Combine(DestDir, Utils.MakePathRelativeTo(Filename, SourceDir));
				if (File.Exists(DestFilename))
				{
					File.Delete(DestFilename);
				}

				// make the subdirectory if needed
				string DestSubdir = Path.GetDirectoryName(DestFilename);
				if (!Directory.Exists(DestSubdir))
				{
					Directory.CreateDirectory(DestSubdir);
				}

				// some files are handled specially
				string Ext = Path.GetExtension(Filename);
				if (Ext == ".xml")
				{
					string Contents = File.ReadAllText(Filename);

					// replace some varaibles
					foreach (var Pair in Replacements)
					{
						Contents = Contents.Replace(Pair.Key, Pair.Value);
					}

					// write out file
					File.WriteAllText(DestFilename, Contents);
				}
				else
				{
					File.Copy(Filename, DestFilename);

					// remove any read only flags
					FileInfo DestFileInfo = new FileInfo(DestFilename);
					DestFileInfo.Attributes = DestFileInfo.Attributes & ~FileAttributes.ReadOnly;
				}
			}
		}

		private void DeleteDirectory(string Path)
		{
			Console.WriteLine("\nDeleting: " + Path);
			if (Directory.Exists(Path))
			{
				try
				{
					// first make sure everything it writable
					string[] Files = Directory.GetFiles(Path, "*.*", SearchOption.AllDirectories);
					foreach (string Filename in Files)
					{
						// remove any read only flags
						FileInfo FileInfo = new FileInfo(Filename);
						FileInfo.Attributes = FileInfo.Attributes & ~FileAttributes.ReadOnly;
					}

					// now deleting will work better
					Directory.Delete(Path, true);
				}
				catch (Exception)
				{
					Log.TraceInformation("Failed to delete intermediate cdirectory {0}. Continuing on...", Path);
				}
			}
		}

        public string GetUE4BuildFilePath(String EngineDirectory)
        {
            return Path.GetFullPath(Path.Combine(EngineDirectory, "Build/Android/Java"));
        }

        public string GetUE4JavaFilePath(String EngineDirectory)
        {
            return Path.GetFullPath(Path.Combine(GetUE4BuildFilePath(EngineDirectory), "src/com/epicgames/ue4"));
        }

        public string GetUE4JavaBuildSettingsFileName(String EngineDirectory)
        {
            return Path.Combine(GetUE4JavaFilePath(EngineDirectory), "JavaBuildSettings.java");
        }

        public void WriteJavaBuildSettingsFile(string FileName, bool OBBinAPK)
        {
             // (!UEBuildConfiguration.bOBBinAPK ? "PackageType.AMAZON" : /*bPackageForGoogle ? "PackageType.GOOGLE" :*/ "PackageType.DEVELOPMENT") + ";\n");
            string Setting = OBBinAPK ? "AMAZON" : "DEVELOPMENT";
            if (!File.Exists(FileName) || ShouldWriteJavaBuildSettingsFile(FileName, Setting))
            {
                StringBuilder BuildSettings = new StringBuilder("package com.epicgames.ue4;\npublic class JavaBuildSettings\n{\n");
                BuildSettings.Append("\tpublic enum PackageType {AMAZON, GOOGLE, DEVELOPMENT};\n");
                BuildSettings.Append("\tpublic static final PackageType PACKAGING = PackageType." + Setting + ";\n");
                BuildSettings.Append("}\n");
                File.WriteAllText(FileName, BuildSettings.ToString());
            }
            else
            {
                Console.WriteLine("::Didn't write config file; contains same data as before.");
            }
        }

        public bool ShouldWriteJavaBuildSettingsFile(string FileName, string setting)
        {
            var fileContent = File.ReadAllLines(FileName);
            var packageLine = fileContent[4]; // We know this to be true... because we write it below...
            int location = packageLine.IndexOf("PACKAGING") + 12 + 12; // + (PACKAGING = ) + ("PackageType.")
            return String.Compare(setting, packageLine.Substring(location)) != 0;
        }


		private void MakeAPK(string ProjectName, string ProjectDirectory, string OutputPath, string EngineDirectory, bool bForDistribution, string CookFlavor)
		{
			// cache some build product paths
			string SourceSOName = OutputPath;
			string DestApkName = Path.Combine(ProjectDirectory, "Binaries/Android/") + Path.GetFileNameWithoutExtension(SourceSOName) + ".apk";

			// if the source binary was UE4Game, replace it with the new project name, when re-packaging a binary only build
			DestApkName = DestApkName.Replace("UE4Game-", ProjectName + "-");

			// cache some tools paths
			string AndroidCommandPath = Environment.ExpandEnvironmentVariables("%ANDROID_HOME%/tools/android.bat");
			string NDKBuildPath = Environment.ExpandEnvironmentVariables("%NDKROOT%/ndk-build.cmd");
			string AntBuildPath = GetAntPath();

			// set up some directory info
			string IntermediateAndroidPath = Path.Combine(ProjectDirectory, "Intermediate/Android/");
			string UE4BuildPath = IntermediateAndroidPath + "APK";
            string UE4BuildFilesPath = GetUE4BuildFilePath(EngineDirectory);
			string GameBuildFilesPath = Path.Combine(ProjectDirectory, "Build/Android");

            // See if we need to create a 'default' Java Build settings file if one doesn't exist (if it does exist we have to assume it has been setup correctly)
            string UE4JavaBuildSettingsFileName = GetUE4JavaBuildSettingsFileName(EngineDirectory);
            WriteJavaBuildSettingsFile(UE4JavaBuildSettingsFileName, UEBuildConfiguration.bOBBinAPK);

			// check to see if it's out of date before trying the slow make apk process (look at .so and all Engine and Project build files to be safe)
			List<String> InputFiles = new List<string>();
			InputFiles.Add(SourceSOName);
			InputFiles.AddRange(Directory.EnumerateFiles(UE4BuildFilesPath, "*.*", SearchOption.AllDirectories));
			if (Directory.Exists(GameBuildFilesPath))
			{
				InputFiles.AddRange(Directory.EnumerateFiles(GameBuildFilesPath, "*.*", SearchOption.AllDirectories));
			}

			// look for any newer input file
			DateTime ApkTime = File.GetLastWriteTimeUtc(DestApkName);
			bool bAllInputsCurrent = true;
			foreach (var InputFileName in InputFiles)
			{
				DateTime InputFileTime = File.GetLastWriteTimeUtc(InputFileName);
				if (InputFileTime.CompareTo(ApkTime) > 0)
				{
					// could break here
					bAllInputsCurrent = false;
					break;
				}
			}

			if (bAllInputsCurrent)
			{
				Log.TraceInformation("{0} is up to date (compared to the .so and .java input files)", DestApkName);
				return;
			}

			
			//Wipe the Intermediate/Build/APK directory first
			DeleteDirectory(UE4BuildPath);

            // If we are packaging for Amazon then we need to copy the PAK files to the correct location
            // Currently we'll just support 1 of 'em
            if (UEBuildConfiguration.bOBBinAPK)
            {
                string PAKFileLocation = ProjectDirectory + "/Saved/StagedBuilds/Android" + CookFlavor + "/" + ProjectName + "/Content/Paks";
                Console.WriteLine("Pak location {0}", PAKFileLocation);
                string PAKFileDestination = UE4BuildPath + "/assets";
                Console.WriteLine("Pak destination location {0}", PAKFileDestination);
                if (Directory.Exists(PAKFileLocation))
                {
                    Directory.CreateDirectory(UE4BuildPath);
                    Directory.CreateDirectory(PAKFileDestination);
                    Console.WriteLine("PAK file exists...");
                    var pakFiles = Directory.EnumerateFiles(PAKFileLocation, "*.pak", SearchOption.TopDirectoryOnly);
                    foreach (var s in pakFiles)
                    {
                        Console.WriteLine("Found file {0}", s);
                    }

                    if (pakFiles.Count() > 0)
                    {
                        var destFileName =  Path.Combine(PAKFileDestination, Path.GetFileName(pakFiles.ElementAt(0)) + ".png"); // Need a rename to turn off compression
                        var srcFileName = pakFiles.ElementAt(0);
                        if(!File.Exists(destFileName) || File.GetLastWriteTimeUtc(destFileName) < File.GetLastWriteTimeUtc(srcFileName))
                        {
                            Console.WriteLine("Copying {0} to {1}", srcFileName, destFileName);
                            File.Copy(srcFileName,destFileName);
                        }
                    }
                }
                // Do we want to kill the OBB here or not???
            }

			Dictionary<string, string> Replacements = new Dictionary<string, string>();
			Replacements.Add("${EXECUTABLE_NAME}", ProjectName);

			// distribution apps can't be debuggable, so if it was set to true, set it to false:
			if (bForDistribution)
			{
				Replacements.Add("android:debuggable=\"true\"", "android:debuggable=\"false\"");
			}

			//Copy build files to the intermediate folder in this order (later overrides earlier):
			//	- Shared Engine
			//  - Shared Engine NoRedist (for Epic secret files)
			//  - Game
			//  - Game NoRedist (for Epic secret files)
			CopyFileDirectory(UE4BuildFilesPath, UE4BuildPath, Replacements);
			CopyFileDirectory(UE4BuildFilesPath + "/NoRedist", UE4BuildPath, Replacements);
			CopyFileDirectory(GameBuildFilesPath, UE4BuildPath, Replacements);
			CopyFileDirectory(GameBuildFilesPath + "/NoRedist", UE4BuildPath, Replacements);

			// Copy the generated .so file from the binaries directory to the jni folder
			if (!File.Exists(SourceSOName))
			{
				throw new BuildException("Can't make an APK without the compiled .so [{0}]", SourceSOName);
			}
			if (!Directory.Exists(UE4BuildPath + "/jni"))
			{
				throw new BuildException("Can't make an APK without the jni directory [{0}/jni]", UE4BuildFilesPath);
			}

			//Android.bat for game-specific
			ProcessStartInfo AndroidBatStartInfoGame = new ProcessStartInfo();
			AndroidBatStartInfoGame.WorkingDirectory = UE4BuildPath;
			AndroidBatStartInfoGame.FileName = AndroidCommandPath;
			AndroidBatStartInfoGame.Arguments = "update project --name " + ProjectName + " --path . --target " + GetSdkApiLevel();
			AndroidBatStartInfoGame.UseShellExecute = false;
			Console.WriteLine("\nRunning: " + AndroidBatStartInfoGame.FileName + " " + AndroidBatStartInfoGame.Arguments);
			Process AndroidBatGame = new Process();
			AndroidBatGame.StartInfo = AndroidBatStartInfoGame;
			AndroidBatGame.Start();
			AndroidBatGame.WaitForExit();

			// android bat failure
			if (AndroidBatGame.ExitCode != 0)
			{
				throw new BuildException("android.bat failed [{0}]", AndroidBatStartInfoGame.Arguments);
			}

            // Update the Google Play services lib with the target platform version currently in use.
            // This appears to be required for the build to work without errors when Play services are referenced by the game.
            // This will try to modify existing files, like project.properties, so we copy the entire library into
            // an intermediate directory and work from there.
            string GooglePlayServicesSourcePath = Path.GetFullPath(Path.Combine(EngineDirectory, "Build/Android/Java/google-play-services_lib/"));
			string GooglePlayServicesIntermediatePath = Path.GetFullPath(Path.Combine(IntermediateAndroidPath, "google-play-services_lib/"));

			DeleteDirectory(GooglePlayServicesIntermediatePath);
			CopyFileDirectory(GooglePlayServicesSourcePath, GooglePlayServicesIntermediatePath, new Dictionary<string, string>());
		    
			ProcessStartInfo AndroidBatStartInfoPlayServicesLib = new ProcessStartInfo();
			AndroidBatStartInfoPlayServicesLib.WorkingDirectory = GooglePlayServicesIntermediatePath;
			AndroidBatStartInfoPlayServicesLib.FileName = AndroidCommandPath;
			AndroidBatStartInfoPlayServicesLib.Arguments = "update project " + " --path . --target " + GetSdkApiLevel();
			AndroidBatStartInfoPlayServicesLib.UseShellExecute = false;
			Console.WriteLine("\nRunning: " + AndroidBatStartInfoPlayServicesLib.FileName + " " + AndroidBatStartInfoPlayServicesLib.Arguments);
			Process AndroidBatPlayServicesLib = new Process();
			AndroidBatPlayServicesLib.StartInfo = AndroidBatStartInfoPlayServicesLib;
			AndroidBatPlayServicesLib.Start();
			AndroidBatPlayServicesLib.WaitForExit();

            // android bat failure
            if (AndroidBatPlayServicesLib.ExitCode != 0)
            {
                throw new BuildException("android.bat failed [{0}]", AndroidBatStartInfoPlayServicesLib.Arguments);
            }

            //need to create separate run for each lib. Will be added to project.properties in order in which they are added 
            //the order is important.
            //as android.library.reference.X=libpath where X = 1 - N
            //for e.g this one will be added as android.library.reference.1=<EngineDirectory>/Source/ThirdParty/Android/google_play_services_lib
            
            // Ant seems to need a relative path to work
			Uri ServicesBuildUri = new Uri(GooglePlayServicesIntermediatePath);
            Uri ProjectUri = new Uri(UE4BuildPath + "/");
			string RelativeServicesUri = ProjectUri.MakeRelativeUri(ServicesBuildUri).ToString();

            AndroidBatStartInfoGame.Arguments = " update project --name " + ProjectName + " --path .  --target " + GetSdkApiLevel() + " --library " + RelativeServicesUri;
            Console.WriteLine("\nRunning: " + AndroidBatStartInfoGame.FileName + " " + AndroidBatStartInfoGame.Arguments);
            AndroidBatGame.Start();
            AndroidBatGame.WaitForExit();

            if (AndroidBatGame.ExitCode != 0)
            {
                throw new BuildException("android.bat failed [{0}]", AndroidBatStartInfoGame.Arguments);
            }

			// Use ndk-build to do stuff and move the .so file to the lib folder (only if NDK is installed)

			string FinalSOName = "";
			if (File.Exists(NDKBuildPath))
			{
				// copy the binary to the standard .so location
				FinalSOName = UE4BuildPath + "/jni/libUE4.so";
				File.Copy(SourceSOName, FinalSOName, true);

				ProcessStartInfo NDKBuildInfo = new ProcessStartInfo();
				NDKBuildInfo.WorkingDirectory = UE4BuildPath;
				NDKBuildInfo.FileName = NDKBuildPath;
				if (!bForDistribution)
				{
					NDKBuildInfo.Arguments = "NDK_DEBUG=1";
				}
				NDKBuildInfo.UseShellExecute = true;
				NDKBuildInfo.WindowStyle = ProcessWindowStyle.Minimized;
				Console.WriteLine("\nRunning: " + NDKBuildInfo.FileName + " " + NDKBuildInfo.Arguments);
				Process NDKBuild = new Process();
				NDKBuild.StartInfo = NDKBuildInfo;
				NDKBuild.Start();
				NDKBuild.WaitForExit();

				// ndk build failure
				if (NDKBuild.ExitCode != 0)
				{
					throw new BuildException("ndk-build failed [{0}]", NDKBuildInfo.Arguments);
				}
			}
			else
			{
				// if no NDK, we don't need any of the debugger stuff, so we just copy the .so to where it will end up
				FinalSOName = UE4BuildPath + "/libs/armeabi-v7a/libUE4.so";
				Directory.CreateDirectory(Path.GetDirectoryName(FinalSOName));
				File.Copy(SourceSOName, FinalSOName);

			}
			
			// remove any read only flags
			FileInfo DestFileInfo = new FileInfo(FinalSOName);
			DestFileInfo.Attributes = DestFileInfo.Attributes & ~FileAttributes.ReadOnly;

			// Use ant debug to build the .apk file
			ProcessStartInfo CallAntStartInfo = new ProcessStartInfo();
			CallAntStartInfo.WorkingDirectory = UE4BuildPath;
			CallAntStartInfo.FileName = "cmd.exe";
			CallAntStartInfo.Arguments = "/c \"" + AntBuildPath + "\"  " + (bForDistribution ? "release" : "debug");
			CallAntStartInfo.UseShellExecute = false;
			Console.WriteLine("\nRunning: " + CallAntStartInfo.Arguments);
			Process CallAnt = new Process();
			CallAnt.StartInfo = CallAntStartInfo;
			CallAnt.Start();
			CallAnt.WaitForExit();

			// ant failure
			if (CallAnt.ExitCode != 0)
			{
				throw new BuildException("ant.bat failed [{0}]", CallAntStartInfo.Arguments);
			}

			// make sure destination exists
			Directory.CreateDirectory(Path.GetDirectoryName(DestApkName));

			// do we need to sign for distro?
			if (bForDistribution)
			{
				// use diffeent source and dest apk's for signed mode
				string SourceApkName = UE4BuildPath + "/bin/" + ProjectName + "-release-unsigned.apk";
				SignApk(UE4BuildPath + "/SigningConfig.xml", SourceApkName, DestApkName);
			}
			else
			{
				// now copy to the final location
				File.Copy(UE4BuildPath + "/bin/" + ProjectName + "-debug" + ".apk", DestApkName, true);
			}
		}

		private void SignApk(string ConfigFilePath, string SourceApk, string DestApk)
		{
			string JarCommandPath = Environment.ExpandEnvironmentVariables("%JAVA_HOME%/bin/jarsigner.exe");
			string ZipalignCommandPath = Environment.ExpandEnvironmentVariables("%ANDROID_HOME%/tools/zipalign.exe");

			if (!File.Exists(ConfigFilePath))
			{
				throw new BuildException("Unable to sign for Shipping without signing config file: '{0}", ConfigFilePath);
			}

			// open an Xml parser for the config file
			string ConfigFile = File.ReadAllText(ConfigFilePath);
			XmlReader Xml = XmlReader.Create(new StringReader(ConfigFile));

			string Alias = "UESigningKey";
			string KeystorePassword = "";
			string KeyPassword = "_sameaskeystore_";
			string Keystore = "UE.keystore";

			Xml.ReadToFollowing("ue-signing-config");
			bool bFinishedSection = false;
			while (Xml.Read() && !bFinishedSection)
			{
				switch (Xml.NodeType)
				{
					case XmlNodeType.Element:
						if (Xml.Name == "keyalias")
						{
							Alias = Xml.ReadElementContentAsString();
						}
						else if (Xml.Name == "keystorepassword")
						{
							KeystorePassword = Xml.ReadElementContentAsString();
						}
						else if (Xml.Name == "keypassword")
						{
							KeyPassword = Xml.ReadElementContentAsString();
						}
						else if (Xml.Name == "keystore")
						{
							Keystore = Xml.ReadElementContentAsString();
						}
						break;
					case XmlNodeType.EndElement:
						if (Xml.Name == "ue-signing-config")
						{
							bFinishedSection = true;
						}
						break;
				}
			}

			string CommandLine = "-sigalg SHA1withRSA -digestalg SHA1";
			CommandLine += " -storepass " + KeystorePassword;

			if (KeyPassword != "_sameaskeystore_")
			{
				CommandLine += " -keypass " + KeyPassword;
			}

			// put on the keystore
			CommandLine += " -keystore \"" + Keystore + "\"";

			// finish off the commandline
			CommandLine += " \"" + SourceApk + "\" " + Alias;

			// sign in-place
			ProcessStartInfo CallJarStartInfo = new ProcessStartInfo();
			CallJarStartInfo.WorkingDirectory = Path.GetDirectoryName(ConfigFilePath);
			CallJarStartInfo.FileName = JarCommandPath;
			CallJarStartInfo.Arguments = CommandLine;// string.Format("galg SHA1withRSA -digestalg SHA1 -keystore {1} {2} {3}",	 Password, Keystore, SourceApk, Alias);
			CallJarStartInfo.UseShellExecute = false;
			Console.WriteLine("\nRunning: {0} {1}", CallJarStartInfo.FileName, CallJarStartInfo.Arguments);
			Process CallAnt = new Process();
			CallAnt.StartInfo = CallJarStartInfo;
			CallAnt.Start();
			CallAnt.WaitForExit();

			if (File.Exists(DestApk))
			{
				File.Delete(DestApk);
			}

			// now we need to zipalign the apk to the final destination (to 4 bytes, must be 4)
			ProcessStartInfo CallZipStartInfo = new ProcessStartInfo();
			CallZipStartInfo.WorkingDirectory = Path.GetDirectoryName(ConfigFilePath);
			CallZipStartInfo.FileName = ZipalignCommandPath;
			CallZipStartInfo.Arguments = string.Format("4 \"{0}\" \"{1}\"", SourceApk, DestApk);
			CallZipStartInfo.UseShellExecute = false;
			Console.WriteLine("\nRunning: {0} {1}", CallZipStartInfo.FileName, CallZipStartInfo.Arguments);
			Process CallZip = new Process();
			CallZip.StartInfo = CallZipStartInfo;
			CallZip.Start();
			CallZip.WaitForExit();
		}

		public override bool PrepTargetForDeployment(UEBuildTarget InTarget)
		{
			return PrepForUATPackageOrDeploy(InTarget.AppName, InTarget.ProjectDirectory, InTarget.OutputPath, BuildConfiguration.RelativeEnginePath, false, "");
		}

		public override bool PrepForUATPackageOrDeploy(string ProjectName, string ProjectDirectory, string ExecutablePath, string EngineDirectory, bool bForDistribution, string CookFlavor)
		{
			MakeAPK(ProjectName, ProjectDirectory, ExecutablePath, EngineDirectory, bForDistribution, CookFlavor);
			return true;
		}

		public static void OutputReceivedDataEventHandler(Object Sender, DataReceivedEventArgs Line)
		{
			if ((Line != null) && (Line.Data != null))
			{
				Log.TraceInformation(Line.Data);
			}
		}
	}
}
