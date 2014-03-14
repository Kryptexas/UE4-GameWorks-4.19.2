// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Threading;
using System.Reflection;
using AutomationTool;
using UnrealBuildTool;

[Help(@"Builds engine documentation.")]
[Help("-env", "Builds the compile environment")]
public class BuildDocumentation : BuildCommand
{
	const string EngineTarget = "DocumentationEditor";
	const string EnginePlatform = "Win64";
	const string EngineConfiguration = "Debug";

	const string EngineUProject = "Engine\\Documentation\\Extras\\API\\Build\\Documentation.uproject";

	public override void ExecuteBuild()
	{
		// Parse the command line
		bool bClean = ParseParam("clean");
		bool bCleanMeta = bClean || ParseParam("cleanmeta");
		bool bCleanEnv = bClean || ParseParam("cleanenv");
		bool bCleanXml = bClean || ParseParam("cleanxml");
		bool bCleanUdn = bClean || ParseParam("cleanudn");
		bool bCleanHtml = bClean || ParseParam("cleanhtml");
		bool bCleanChm = bClean || ParseParam("cleanchm");

		bool bBuild = ParseParam("build");
		bool bBuildMeta = bBuild || ParseParam("buildmeta");
		bool bBuildEnv = bBuild || ParseParam("buildenv");
		bool bBuildXml = bBuild || ParseParam("buildxml");
		bool bBuildUdn = bBuild || ParseParam("buildudn");
		bool bBuildHtml = bBuild || ParseParam("buildhtml");
		bool bBuildChm = bBuild || ParseParam("buildchm");

		bool bStats = ParseParam("stats");

		bool bMakeArchives = ParseParam("archive");

		string Filter = ParseParamValue("filter");

		// Create the intermediate folders
		string IntermediateDir = Path.Combine(CmdEnv.LocalRoot, "Engine\\Intermediate\\Documentation");
		CommandUtils.DeleteDirectory(IntermediateDir);
		CommandUtils.CreateDirectory(IntermediateDir);
		string BuildDir = Path.Combine(IntermediateDir, "build");
		CommandUtils.CreateDirectory(BuildDir);
		string DoxygenDir = Path.Combine(IntermediateDir, "doxygen");
		CommandUtils.CreateDirectory(DoxygenDir);
		string MetadataDir = Path.Combine(IntermediateDir, "metadata");
		CommandUtils.CreateDirectory(MetadataDir);
		string ArchiveDir = Path.Combine(CmdEnv.LocalRoot, "Engine\\Documentation\\Builds");
		CommandUtils.CreateDirectory(ArchiveDir);

		// Get all the file paths
		string TargetInfoPath = Path.Combine(BuildDir, "targetinfo.xml");
		string UnrealBuildToolPath = Path.Combine(CmdEnv.LocalRoot, "Engine/Binaries/DotNET/UnrealBuildTool.exe");
		string UnrealBuildToolProject = Path.Combine(CmdEnv.LocalRoot, "Engine/Source/Programs/UnrealBuildTool/UnrealBuildTool.csproj");
		string ApiDocToolSolution = Path.Combine(CmdEnv.LocalRoot, "Engine/Source/Programs/UnrealDocTool/APIDocTool/APIDocTool.sln");
		string ApiDocToolPath = Path.Combine(CmdEnv.LocalRoot, "Engine/Source/Programs/UnrealDocTool/APIDocTool/APIDocTool/bin/x64/Release/APIDocTool.exe");
		string UnrealDocToolSolution = Path.Combine(CmdEnv.LocalRoot, "Engine/Source/Programs/UnrealDocTool/UnrealDocTool/UnrealDocTool.sln");
		string StatsPath = "\\\\epicgames.net\\root\\UE3\\UE4\\Documentation\\API-Stats.txt";

		// Compile UnrealBuildTool
		MsBuild(CmdEnv, UnrealBuildToolProject, "/verbosity:normal /target:Rebuild /property:Configuration=Development /property:Platform=AnyCPU", "BuildUnrealBuildTool");

		// Compile APIDocTool
		BuildSolution(CmdEnv, ApiDocToolSolution, "Release|x64", "BuildApiDocTool");

		// Compile UnrealDocTool
		BuildSolution(CmdEnv, UnrealDocToolSolution, "Release", "BuildUnrealDocTool");

		// Prepare the shared doc tool command line, containing all the paths
		string ApiToolCommandLine = "";
		ApiToolCommandLine += " -enginedir=\"" + Path.Combine(CmdEnv.LocalRoot, "Engine") + "\"";
		ApiToolCommandLine += " -targetinfo=\"" + TargetInfoPath + "\"";
		ApiToolCommandLine += " -xmldir=\"" + DoxygenDir + "\"";
		ApiToolCommandLine += " -metadatadir=\"" + MetadataDir + "\"";
		ApiToolCommandLine += " -verbose";
		if (Filter != null) ApiToolCommandLine += " -filter=" + Filter;

		// Execute the clean
		if(bCleanEnv)
		{
			File.Delete(TargetInfoPath);
		}
		if (bCleanMeta)
		{
			RunAndLog(CmdEnv, ApiDocToolPath, "-cleanmeta" + ApiToolCommandLine, "ApiDocTool-CleanMeta");
		}
		if(bCleanXml)
		{
			RunAndLog(CmdEnv, ApiDocToolPath, "-cleanxml" + ApiToolCommandLine, "ApiDocTool-CleanXML");
		}
		if(bCleanUdn)
		{
			RunAndLog(CmdEnv, ApiDocToolPath, "-cleanudn" + ApiToolCommandLine, "ApiDocTool-CleanUDN");
		}
		if(bCleanHtml)
		{
			RunAndLog(CmdEnv, ApiDocToolPath, "-cleanhtml" + ApiToolCommandLine, "ApiDocTool-CleanHTML");
		}
		if (bCleanChm)
		{
			RunAndLog(CmdEnv, ApiDocToolPath, "-cleanchm" + ApiToolCommandLine, "ApiDocTool-CleanCHM");
		}

		// Execute the build
		if (bBuildEnv)
		{
			CommandUtils.RunUBT(CmdEnv, UnrealBuildToolPath, String.Format("{0} {1} {2} -project=\"{3}\" -writetargetinfo=\"{4}\"", EngineTarget, EnginePlatform, EngineConfiguration, Path.GetFullPath(Path.Combine(CmdEnv.LocalRoot, EngineUProject)), TargetInfoPath));
		}
		if (bBuildMeta)
		{
			RunAndLog(CmdEnv, ApiDocToolPath, "-buildmeta" + ApiToolCommandLine, "ApiDocTool-BuildMeta");
		}
		if (bBuildXml)
		{
			RunAndLog(CmdEnv, ApiDocToolPath, "-buildxml" + ApiToolCommandLine, "ApiDocTool-BuildXML");
			if (bMakeArchives)
			{
				CreateAndSubmitArchiveFromDir("Engine/Documentation/Builds/API-XML.tgz", "API documentation intermediates", IntermediateDir);
			}
		}
		if (bBuildUdn)
		{
			RunAndLog(CmdEnv, ApiDocToolPath, "-buildudn" + (bStats? (" -stats=\"" + StatsPath + "\"") : "") + ApiToolCommandLine, "ApiDocTool-BuildUDN");
			if (bMakeArchives)
			{
				CreateAndSubmitArchiveFromDir("Engine/Documentation/Builds/API-UDN.tgz", "API documentation UDN output", Path.Combine(CmdEnv.LocalRoot, "Engine\\Documentation\\Source\\API"));
				CreateAndSubmitArchiveFromFiles("Engine/Documentation/Builds/API-Sitemap.tgz", "API documentation sitemap output", Path.Combine(CmdEnv.LocalRoot, "Engine\\Documentation\\CHM"), new string[] { "API.hhc", "API.hhk" });
			}
		}
		if (bBuildHtml)
		{
			RunAndLog(CmdEnv, ApiDocToolPath, "-buildhtml" + ApiToolCommandLine, "ApiDocTool-BuildHTML");
			if (bMakeArchives)
			{
				string BaseDir = Path.Combine(CmdEnv.LocalRoot, "Engine\\Documentation\\HTML");

				List<string> InputFiles = new List<string>();
				FindFilesForTar(BaseDir, "Include", InputFiles, true);
				FindFilesForTar(BaseDir, "INT\\API", InputFiles, true);
				FindFilesForTar(BaseDir, "Images", InputFiles, false);

				CommandUtils.CopyFile(Path.Combine(CmdEnv.LocalRoot, "Engine\\Documentation\\Extras\\API\\index.html"), Path.Combine(BaseDir, "index.html"));
				InputFiles.Add("index.html");

				CreateAndSubmitArchiveFromFiles("Engine/Documentation/Builds/API-HTML.tgz", "API documentation HTML output", BaseDir, InputFiles);
			}
		}
		if (bBuildChm)
		{
			RunAndLog(CmdEnv, ApiDocToolPath, "-buildchm" + ApiToolCommandLine, "ApiDocTool-BuildCHM");
			SubmitFile("Engine/Documentation/CHM/API.chm", "API documentation CHM output");
		}
	}

	static void CreateAndSubmitArchiveFromDir(string DepotArchivePath, string Description, string BaseDir)
	{
		// Find all the input files
		List<string> InputFiles = new List<string>();
		FindFilesForTar(new DirectoryInfo(BaseDir), "", InputFiles, true);
		CreateAndSubmitArchiveFromFiles(DepotArchivePath, Description, BaseDir, InputFiles.ToArray());
	}

	static void CreateAndSubmitArchiveFromFiles(string DepotArchivePath, string Description, string BaseDir, IEnumerable<string> InputFiles)
	{
		string ArchivePath = Path.Combine(CmdEnv.LocalRoot, DepotArchivePath);

		// Create the archive
		CreateTgzFromFiles(ArchivePath, BaseDir, InputFiles);

		// Submit the archive to P4
		SubmitFile(DepotArchivePath, Description);
	}

	static void SubmitFile(string DepotPath, string Description)
	{
		if (P4Enabled)
		{
			int Changelist = CreateChange(P4Env.Client, String.Format("{0} from CL#{1}", Description, P4Env.Changelist));
			Reconcile(Changelist, CombinePaths(PathSeparator.Slash, P4Env.ClientRoot, DepotPath));

			if (!TryDeleteEmptyChange(Changelist))
			{
				if (!GlobalCommandLine.NoSubmit)
				{
					int SubmittedChangelist;
					Submit(Changelist, out SubmittedChangelist, true, true);
				}
			}
		}
	}

	static void CreateTgzFromFiles(string TarPath, string BaseDir, IEnumerable<string> InputFiles)
	{
		Console.WriteLine("Creating archive '{0}'...", TarPath);
		if (File.Exists(TarPath))
		{
			CommandUtils.DeleteFile(TarPath);
		}
		using(FileStream TarFileStream = new FileStream(TarPath, FileMode.Create))
		{
			using (GZipStream ZipStream = new GZipStream(TarFileStream, CompressionMode.Compress))
			{
				using (BinaryWriter TarWriter = new BinaryWriter(ZipStream))
				{
					foreach (string InputFile in InputFiles)
					{
						byte[] FileData = File.ReadAllBytes(Path.Combine(BaseDir, InputFile));
						WriteFileToTar(TarWriter, InputFile, FileData);
					}
					TarWriter.Write(new byte[1024]);
				}
			}
		}
	}

	static void FindFilesForTar(string BaseDir, string BaseTarPath, List<string> Files, bool bRecursive)
	{
		FindFilesForTar(new DirectoryInfo(Path.Combine(BaseDir, BaseTarPath)), BaseTarPath, Files, bRecursive);
	}

	static void FindFilesForTar(DirectoryInfo Dir, string BaseTarPath, List<string> Files, bool bRecursive)
	{
		if (bRecursive)
		{
			foreach (DirectoryInfo ChildDir in Dir.GetDirectories())
			{
				FindFilesForTar(ChildDir, Path.Combine(BaseTarPath, ChildDir.Name), Files, bRecursive);
			}
		}
		foreach (FileInfo FileInfo in Dir.GetFiles())
		{
			Files.Add(Path.Combine(BaseTarPath, FileInfo.Name));
		}
	}

	const int TarFileNameFieldLength = 99;

	static void WriteFileToTar(BinaryWriter Writer, string FilePath, byte[] FileData)
	{
		string NormalizedFilePath = FilePath.Replace('\\', '/');

		// Generate a long header record if necessary
		if (NormalizedFilePath.Length > TarFileNameFieldLength)
		{
			WriteTarHeader(Writer, "././@LongLink", NormalizedFilePath.Length + 1, 'L');

			byte[] FilePathBytes = new byte[NormalizedFilePath.Length + 1];
			EncodeTarString(FilePathBytes, 0, FilePathBytes.Length, NormalizedFilePath);

			WriteTarBlocks(Writer, FilePathBytes);
		}

		// Now write the normal file
		WriteTarHeader(Writer, NormalizedFilePath, FileData.Length, '0');
		WriteTarBlocks(Writer, FileData);
	}

	static void WriteTarHeader(BinaryWriter Writer, string FileName, int Length, char Type)
	{
		// Write the name
		byte[] Header = new byte[512];

		// Name
		EncodeTarString(Header, 0, TarFileNameFieldLength, FileName);

		// File mode
		EncodeTarValue(Header, 100, 7, 0x81ff);

		// UID
		EncodeTarValue(Header, 108, 7, 0);

		// GID
		EncodeTarValue(Header, 116, 7, 0);

		// File size
		EncodeTarValue(Header, 124, 12, Length);

		// File timestamp
		EncodeTarValue(Header, 136, 12, 0);

		// File type ('0' = Regular file)
		Header[156] = (byte)Type;

		// Generate the the checksum (seed the checksum as if the field was initialized with 8 spaces)
		int Checksum = (byte)' ' * 8;
		for (int Idx = 0; Idx < Header.Length; Idx++) Checksum += Header[Idx];
		EncodeTarValue(Header, 148, 7, Checksum);

		// Write the header
		Writer.Write(Header);
	}

	static void WriteTarBlocks(BinaryWriter Writer, byte[] Data)
	{
		Writer.Write(Data);
		Writer.Write(new byte[(512 - (Data.Length % 512)) % 512]);
	}

	static void EncodeTarString(byte[] Data, int Offset, int Length, string Text)
	{
		// Convert the Text to ASCII
		for (int Idx = 0; Idx < Math.Min(Length, Text.Length); Idx++)
		{
			if (Text[Idx] == 0 || Text[Idx] > 0x7f)
			{
				throw new AutomationException("Invalid character in TAR string");
			}
			else
			{
				Data[Offset + Idx] = (byte)Text[Idx];
			}
		}
	}

	static void EncodeTarValue(byte[] Data, int Offset, int Length, int Value)
	{
		// Write the value as an octal string followed by a space
		string ValueString = Convert.ToString(Value, 8) + " ";
		while (ValueString.Length < Length) ValueString = " " + ValueString;
		EncodeTarString(Data, Offset, Length, ValueString);
	}
}
