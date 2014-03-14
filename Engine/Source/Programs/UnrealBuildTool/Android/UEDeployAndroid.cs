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
			string Target = Utils.GetStringEnvironmentVariable("ue.AndroidSdkApiTarget", "latest");

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

		private void MakeAPK(string ProjectName, string ProjectDirectory, string OutputPath, string EngineDirectory, bool bForDistribution)
		{
			// cache some build product paths
			string SourceSOName = OutputPath;
			string DestApkName = ProjectDirectory + "/Binaries/Android/" + Path.GetFileNameWithoutExtension(SourceSOName) + ".apk";

			// if the source binary was UE4Game, replace it with the new project name, when re-packaging a binary only build
			DestApkName = DestApkName.Replace("UE4Game-", ProjectName + "-");

			// cache some tools paths
			string AndroidCommandPath = Environment.ExpandEnvironmentVariables("%ANDROID_HOME%/tools/android.bat");
			string NDKBuildPath = Environment.ExpandEnvironmentVariables("%NDKROOT%/ndk-build.cmd");
			string AntBuildPath = GetAntPath();

			// set up some directory info
			string UE4BuildPath = ProjectDirectory + "/Intermediate/Android/APK";
			string UE4BuildFilesPath = Path.GetFullPath(Path.Combine(EngineDirectory, "Build/Android/Java"));
			string GameBuildFilesPath = ProjectDirectory + "/Build/Android";

			//Wipe the Intermediate/Build/APK directory first
			Console.WriteLine("\nDeleting: " + UE4BuildPath);
			if (Directory.Exists(UE4BuildPath))
			{
				try
				{
					// first make sure everything it writable
					string[] Files = Directory.GetFiles(UE4BuildPath, "*.*", SearchOption.AllDirectories);
					foreach (string Filename in Files)
					{
						// remove any read only flags
						FileInfo FileInfo = new FileInfo(Filename);
						FileInfo.Attributes = FileInfo.Attributes & ~FileAttributes.ReadOnly;
					}

					// now deleting will work better
					Directory.Delete(UE4BuildPath, true);
				}
				catch (Exception)
				{
					Log.TraceInformation("Failed to delete intermediate cdirectory {0}. Continuing on...", UE4BuildPath);
				}
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
			string SourceSOName = InTarget.OutputPath;
			string DestApkName = InTarget.ProjectDirectory + "/Binaries/Android/" + Path.GetFileNameWithoutExtension(SourceSOName) + ".apk";

			// don't run the whole MakeApk path if the .so and .java files aren't newer than the .apk, 
			// so repeated uses of F5 in the debugger aren't painfully slow
			List<String> InputFiles = new List<string>();

			InputFiles.Add(SourceSOName);
			string UE4BuildFilesPath = BuildConfiguration.RelativeEnginePath + "Build/Android/Java";
			InputFiles.AddRange(Directory.EnumerateFiles(UE4BuildFilesPath + "/src/com/epicgames/ue4", "*.java", SearchOption.AllDirectories));

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
				return true;
			}

			return PrepForUATPackageOrDeploy(InTarget.AppName, InTarget.ProjectDirectory, InTarget.OutputPath, BuildConfiguration.RelativeEnginePath, false);
		}

		public override bool PrepForUATPackageOrDeploy(string ProjectName, string ProjectDirectory, string ExecutablePath, string EngineDirectory, bool bForDistribution)
		{
			MakeAPK(ProjectName, ProjectDirectory, ExecutablePath, EngineDirectory, bForDistribution);
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
