// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//#define TEST_INDEX_PAGE

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;
using System.Text.RegularExpressions;
using System.Reflection;
using System.Diagnostics;
using System.Web;
using System.Security.Cryptography;
using System.CodeDom.Compiler;
using System.Runtime.InteropServices;
using DoxygenLib;

namespace APIDocTool
{
    public class Program
	{
		[DllImport("kernel32.dll")]
		public static extern uint SetErrorMode(uint hHandle);

		public static IniFile Settings;
		public static HashSet<string> IgnoredFunctionMacros;
		public static HashSet<string> IncludedSourceFiles;

        public static String Availability = null;

        public static List<String> ExcludeDirectories = new List<String>();
        public static List<String> SourceDirectories = new List<String>();

		public const string TabSpaces = "&nbsp;&nbsp;&nbsp;&nbsp;";
        public const string APIFolder = "API";

		public static bool bOutputPublish = false;

		static string[] ExcludeSourceDirectories =
		{
			"Apple",
			"Windows",
			"Win32",
//			"Win64",	// Need to allow Win64 now that the intermediate headers are in a Win64 directory
			"Mac",
			"XboxOne",
			"PS4",
			"IOS",
			"Android",
			"WinRT",
			"WinRT_ARM",
			"HTML5",
			"Linux",
			"TextureXboxOneFormat",
		};

		static string[] ExcludeSourceFiles = 
		{
			"*/CoreUObject/Classes/Object.h",
		};

		static string[] DoxygenExpandedMacros = 
		{
		};

		static string[] DoxygenPredefinedMacros =
		{
			"UE_BUILD_DOCS=1",
			"DECLARE_FUNCTION(X)=void X(FFrame &Stack, void *Result)",

			// Compilers
			"DLLIMPORT=",
			"DLLEXPORT=",
			"PURE_VIRTUAL(Func,Extra)=;",
			"FORCEINLINE=",
			"MSVC_PRAGMA(X)=",
			"MS_ALIGN(X)= ",
			"GCC_ALIGN(X)= ",
			"VARARGS=",
			"VARARG_DECL(FuncRet,StaticFuncRet,Return,FuncName,Pure,FmtType,ExtraDecl,ExtraCall)=FuncRet FuncName(ExtraDecl FmtType Fmt, ...)",
			"VARARG_BODY(FuncRet,FuncName,FmtType,ExtraDecl)=FuncRet FuncName(ExtraDecl FmtType Fmt, ...)",
			"PRAGMA_DISABLE_OPTIMIZATION=",
			"PRAGMA_ENABLE_OPTIMIZATION=",
			"NO_API= ",
			"OVERRIDE= ",
			"CDECL= ",

			// Platform
			"PLATFORM_SUPPORTS_DRAW_MESH_EVENTS=1",
			"PLATFORM_SUPPORTS_VOICE_CAPTURE=1",

			// Features
			"DO_CHECK=1",
			"DO_GUARD_SLOW=1",
			"STATS=0",
			"ENABLE_VISUAL_LOG=1",
			"WITH_EDITOR=1",
			"WITH_NAVIGATION_GENERATOR=1",
			"WITH_APEX_CLOTHING=1",
			"WITH_CLOTH_COLLISION_DETECTION=1",
			"WITH_EDITORONLY_DATA=1",
			"WITH_PHYSX=1",
			"WITH_SUBSTEPPING=1",
			"NAVOCTREE_CONTAINS_COLLISION_DATA=1",
			"SOURCE_CONTROL_WITH_SLATE=1",
			"MATCHMAKING_HACK_FOR_EGP_IE_HOSTING=1",

			// Online subsystems
			"PACKAGE_SCOPE:=protected",

			// Hit proxies
			"DECLARE_HIT_PROXY()=public: static HHitProxyType * StaticGetType(); virtual HHitProxyType * GetType() const;",

			// Vertex factories
			"DECLARE_VERTEX_FACTORY_TYPE(FactoryClass)=public: static FVertexFactoryType StaticType; virtual FVertexFactoryType* GetType() const;",

			// Slate declarative syntax
			"SLATE_BEGIN_ARGS(WidgetType)=public: struct FArguments : public TSlateBaseNamedArgs<WidgetType> { typedef FArguments WidgetArgsType; FArguments()",
			"SLATE_ATTRIBUTE(AttrType, AttrName)=WidgetArgsType &AttrName( const TAttribute<AttrType>& InAttribute );",
			"SLATE_TEXT_ATTRIBUTE(AttrName)=WidgetArgsType &AttrName( const TAttribute<FText>& InAttribute ); WidgetArgsType &AttrName( const TAttribute<FString>& InAttribute );",
			"SLATE_ARGUMENT(ArgType, ArgName)=WidgetArgsType &ArgName(ArgType InArg);",
			"SLATE_TEXT_ARGUMENT(ArgName)=WidgetArgsType &ArgName(FString InArg); WidgetArgsType &ArgName(FText InArg);",
			"SLATE_STYLE_ARGUMENT(ArgType, ArgName)=WidgetArgsType &ArgName(const ArgType* InArg);",
			"SLATE_SUPPORTS_SLOT(SlotType)=WidgetArgsType &operator+(SlotType &SlotToAdd);",
			"SLATE_SUPPORTS_SLOT_WITH_ARGS(SlotType)=WidgetArgsType &operator+(SlotType::FArguments &ArgumentsForNewSlot);",
			"SLATE_NAMED_SLOT(DeclarationType, SlotName)=NamedSlotProperty<DeclarationType> SlotName();",
			"SLATE_EVENT(DelegateName,EventName)=WidgetArgsType& EventName( const DelegateName& InDelegate );",
			"SLATE_END_ARGS()=};",

			// Rendering macros
			"IMPLEMENT_SHADER_TYPE(TemplatePrefix,ShaderClass,SourceFilename,FunctionName,Frequency)= ",
			"IMPLEMENT_SHADER_TYPE2(ShaderClass,Frequency)= ",
			"IMPLEMENT_SHADER_TYPE3(ShaderClass,Frequency)= ",

			// Stats
			"DEFINE_STAT(Stat)=",
			"DECLARE_STATS_GROUP(GroupDesc,GroupId)=",
			"DECLARE_STATS_GROUP_VERBOSE(GroupDesc,GroupId)=",
			"DECLARE_STATS_GROUP_COMPILED_OUT(GroupDesc,GroupId)=",
			"DECLARE_CYCLE_STAT_EXTERN(CounterName,StatId,GroupId, API)= ",
			"DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(CounterName,StatId,GroupId, API)= ",
			"DECLARE_DWORD_COUNTER_STAT_EXTERN(CounterName,StatId,GroupId, API)= ",
			"DECLARE_MEMORY_STAT_EXTERN(CounterName,StatId,GroupId, API)= ",

			// Steam
			"STEAM_CALLBACK(C,N,P,X)=void N(P*)",
			"STEAM_GAMESERVER_CALLBACK(C,N,P,X)=void N(P*)",

			// Log categories
			"DECLARE_LOG_CATEGORY_EXTERN(CategoryName, DefaultVerbosity, CompileTimeVerbosity)= ",
		};

		const string SitemapContentsFileName = "API.hhc";
		const string SitemapIndexFileName = "API.hhk";

		static Program()
		{
			List<string> DelegateMacros = new List<string>();

			string ArgumentList = "";
			string NamedArgumentList = "";
			string[] Suffixes = { "NoParams", "OneParam", "TwoParams", "ThreeParams", "FourParams", "FiveParams", "SixParams", "SevenParams", "EightParams" };

			for (int Idx = 0; Idx < Suffixes.Length; Idx++)
			{
				string Suffix = Suffixes[Idx];
				string MacroSuffix = (Idx == 0) ? "" : "_" + Suffix;

				DelegateMacros.Add(String.Format("DECLARE_DELEGATE{0}(Name{1})=typedef TBaseDelegate_{2}<void{1}> Name;", MacroSuffix, ArgumentList, Suffix));
				DelegateMacros.Add(String.Format("DECLARE_DELEGATE_RetVal{0}(RT, Name{1})=typedef TBaseDelegate_{2}<RT{1}> Name;", MacroSuffix, ArgumentList, Suffix));
				DelegateMacros.Add(String.Format("DECLARE_EVENT{0}(OT, Name{1})=class Name : public TBaseMulticastDelegate_{2}<void{1}>{{ }}", MacroSuffix, ArgumentList, Suffix));
				DelegateMacros.Add(String.Format("DECLARE_MULTICAST_DELEGATE{0}(Name{1})=typedef TMulticastDelegate_{2}<void{1}> Name;", MacroSuffix, ArgumentList, Suffix));
				DelegateMacros.Add(String.Format("DECLARE_DYNAMIC_DELEGATE{0}(Name{1})=class Name {{ }}", MacroSuffix, NamedArgumentList));
				DelegateMacros.Add(String.Format("DECLARE_DYNAMIC_DELEGATE_RetVal{0}(RT, Name{1})=class Name {{ }}", MacroSuffix, NamedArgumentList));
				DelegateMacros.Add(String.Format("DECLARE_DYNAMIC_MULTICAST_DELEGATE{0}(Name{1})=class Name {{ }};", MacroSuffix, NamedArgumentList));

				ArgumentList += String.Format(", T{0}", Idx + 1);
				NamedArgumentList += String.Format(", T{0}, N{0}", Idx + 1);
			}

			DoxygenPredefinedMacros = DoxygenPredefinedMacros.Union(DelegateMacros).ToArray();
		}

		static void Main(string[] Arguments)
        {
			bool bValidArgs = true;

			bool bCleanMetadata = false;
			bool bCleanXml = false;
			bool bCleanUdn = false;
			bool bCleanHtml = false;
			bool bCleanChm = false;

			bool bBuildMetadata = false;
			bool bBuildXml = false;
			bool bBuildUdn = false;
			bool bBuildHtml = false;
			bool bBuildChm = false;

			string TargetInfoPath = null;
			string EngineDir = null;
			string MetadataDir = null;
			string XmlDir = null;
			string StatsPath = null;

			bool bVerbose = false;
			List<string> Filters = new List<string>();

			// Parse the command line
			foreach (string Argument in Arguments)
			{
				int EqualsIdx = Argument.IndexOf('=');
				string OptionName = (EqualsIdx == -1)? Argument : Argument.Substring(0, EqualsIdx + 1);
				string OptionValue = (EqualsIdx == -1)? null : Argument.Substring(EqualsIdx + 1);

				if (OptionName == "-clean")
				{
					bCleanMetadata = bCleanXml = bCleanUdn = bCleanHtml = true;
				}
				else if (OptionName == "-build")
				{
					bBuildMetadata = bBuildXml = bBuildUdn = bBuildHtml = true;
				}
				else if (OptionName == "-rebuild")
				{
					bCleanMetadata = bCleanXml = bCleanUdn = bCleanHtml = bBuildMetadata = bBuildXml = bBuildUdn = bBuildHtml = true;
				}
				else if (OptionName == "-cleanmeta")
				{
					bCleanMetadata = true;
				}
				else if (OptionName == "-buildmeta")
				{
					bBuildMetadata = true;
				}
				else if (OptionName == "-rebuildmeta")
				{
					bCleanMetadata = bBuildMetadata = true;
				}
				else if (OptionName == "-cleanxml")
				{
					bCleanXml = true;
				}
				else if (OptionName == "-buildxml")
				{
					bBuildXml = true;
				}
				else if (OptionName == "-rebuildxml")
				{
					bCleanXml = bBuildXml = true;
				}
				else if (OptionName == "-cleanudn")
				{
					bCleanUdn = true;
				}
				else if (OptionName == "-buildudn")
				{
					bBuildUdn = true;
				}
				else if (OptionName == "-rebuildudn")
				{
					bCleanUdn = bBuildUdn = true;
				}
				else if (OptionName == "-cleanhtml")
				{
					bCleanHtml = true;
				}
				else if (OptionName == "-buildhtml")
				{
					bBuildHtml = true;
				}
				else if (OptionName == "-rebuildhtml")
				{
					bCleanHtml = bBuildHtml = true;
				}
				else if (OptionName == "-cleanchm")
				{
					bCleanChm = true;
				}
				else if (OptionName == "-buildchm")
				{
					bBuildChm = true;
				}
				else if (OptionName == "-rebuildchm")
				{
					bCleanChm = bBuildChm = true;
				}
				else if (OptionName == "-targetinfo=")
				{
					TargetInfoPath = Path.GetFullPath(OptionValue);
				}
				else if (OptionName == "-enginedir=")
				{
					EngineDir = Path.GetFullPath(OptionValue);
				}
				else if (OptionName == "-xmldir=")
				{
					XmlDir = Path.GetFullPath(OptionValue);
				}
				else if (OptionName == "-metadatadir=")
				{
					MetadataDir = Path.GetFullPath(OptionValue);
				}
				else if (OptionName == "-stats=")
				{
					StatsPath = Path.GetFullPath(OptionValue);
				}
				else if (OptionName == "-verbose")
				{
					bVerbose = true;
				}
				else if (OptionName == "-filter=")
				{
					Filters.AddRange(OptionValue.Split(',').Select(x => x.Replace('\\', '/').Trim()));
				}
				else
				{
					Console.WriteLine("Invalid argument: '{0}'", OptionName);
					bValidArgs = false;
				}
			}

			// Check we have all the required parameters
			if (bBuildXml && TargetInfoPath == null)
			{
				Console.WriteLine("Missing -buildenvironment parameter to be able to build XML");
			}
			else if (bValidArgs && EngineDir != null)
			{
				// If we don't intermediate paths, make them up
				if (XmlDir == null) XmlDir = Path.Combine(EngineDir, "Intermediate\\Documentation\\Default\\Xml");
				if (MetadataDir == null) MetadataDir = Path.Combine(EngineDir, "Intermediate\\Documentation\\Default\\Metadata");

				// Derive all the engine paths we need in one place
				string DoxygenPath = Path.Combine(EngineDir, "Extras\\NoRedist\\Doxygen\\bin\\doxygen.exe");
				string UdnDir = Path.Combine(EngineDir, "Documentation\\Source");
				string HtmlDir = Path.Combine(EngineDir, "Documentation\\HTML");
				string ChmDir = Path.Combine(EngineDir, "Documentation\\CHM");
				string DocToolPath = Path.Combine(EngineDir, "Binaries\\DotNET\\UnrealDocTool.exe");
				string ChmCompilerPath = Path.Combine(EngineDir, "Extras\\NoRedist\\HTML Help Workshop\\hhc.exe");
				string SettingsPath = Path.Combine(EngineDir, "Documentation\\Extras\\API\\API.ini");
				string MetadataPath = Path.Combine(MetadataDir, "metadata.xml");

				// Create all the output directories
				Utility.SafeCreateDirectory(UdnDir);
				Utility.SafeCreateDirectory(HtmlDir);
				Utility.SafeCreateDirectory(ChmDir);

				// Read the settings file
				Settings = IniFile.Read(SettingsPath);
				IgnoredFunctionMacros = new HashSet<string>(Settings.FindValueOrDefault("Input.IgnoredFunctionMacros", "").Split('\n'));
				IncludedSourceFiles = new HashSet<string>(Settings.FindValueOrDefault("Output.IncludedSourceFiles", "").Split('\n'));

				// Clean the output folders
				if (bCleanMetadata)
				{
					CleanMetadata(MetadataDir);
				}
				if (bCleanXml)
				{
					CleanXml(XmlDir);
				}
				if (bCleanUdn)
				{
					CleanUdn(UdnDir);
				}
				if (bCleanHtml)
				{
					CleanHtml(HtmlDir);
				}
				if (bCleanChm)
				{
					CleanChm(ChmDir);
				}

				// Build the data
				if (!bBuildMetadata || BuildMetadata(DoxygenPath, EngineDir, MetadataDir, MetadataPath))
				{
					if (!bBuildXml || BuildXml(EngineDir, TargetInfoPath, DoxygenPath, XmlDir, Filters, bVerbose))
					{
						if (!bBuildUdn || BuildUdn(XmlDir, UdnDir, ChmDir, MetadataPath, StatsPath, Filters))
						{
							if (!bBuildHtml || BuildHtml(EngineDir, DocToolPath, UdnDir))
							{
								if (!bBuildChm || BuildChm(ChmCompilerPath, HtmlDir, ChmDir))
								{
									Console.WriteLine("Complete.");
								}
							}
						}
					}
				}
			}
			else
			{
				// Write the command line options
				Console.WriteLine("APIDocTool.exe [options] -enginedir=<...>");                                    // <-- 80 character limit to start of comment
				Console.WriteLine();
				Console.WriteLine("Options:");
				Console.WriteLine("    -rebuild:                        Clean and build everything");
				Console.WriteLine("    -rebuild[meta|xml|udn|html|chm]: Clean and build specific files");
				Console.WriteLine("    -clean:                          Clean all files");
				Console.WriteLine("    -clean[meta|xml|udn|html|chm]:   Clean specific files");
				Console.WriteLine("    -build:						    Build everything");
				Console.WriteLine("    -build[meta|xml|udn|html|chm]:   Build specific output files");
				Console.WriteLine("    -targetinfo=<...>:			    Specifies the build info, created by");
				Console.WriteLine("                                     running UBT with -writetargetinfo=<...>");
				Console.WriteLine("    -xmldir=<...>:				    Output directory for xml files");
				Console.WriteLine("    -metadatadir=<...>:			    Output directory for metadata files");
				Console.WriteLine("    -filter=<...>,<...>:             Filter conversion, eg.");
				Console.WriteLine("                                       Folders:  -filter=Core/Containers/...");
				Console.WriteLine("                                       Entities: -filter=Core/TArray");
			}
		}

		static void CleanMetadata(string MetadataDir)
		{
			Console.WriteLine("Cleaning '{0}'", MetadataDir);
			Utility.SafeDeleteDirectoryContents(MetadataDir, true);
		}

		static bool BuildMetadata(string DoxygenPath, string EngineDir, string MetadataDir, string MetadataPath)
		{
			string MetadataInputPath = Path.Combine(EngineDir, "Source\\Runtime\\CoreUObject\\Public\\UObject\\ObjectBase.h");
			Console.WriteLine("Building metadata descriptions from '{0}'...", MetadataInputPath);

			DoxygenConfig Config = new DoxygenConfig("Metadata", MetadataInputPath, MetadataDir);
			if (Doxygen.Run(DoxygenPath, Path.Combine(EngineDir, "Source"), Config, true))
			{
				MetadataLookup.Reset();

				// Parse the xml output
				ParseMetadataTags(Path.Combine(MetadataDir, "xml\\namespace_u_c.xml"), MetadataLookup.ClassTags);
				ParseMetadataTags(Path.Combine(MetadataDir, "xml\\namespace_u_i.xml"), MetadataLookup.InterfaceTags);
				ParseMetadataTags(Path.Combine(MetadataDir, "xml\\namespace_u_f.xml"), MetadataLookup.FunctionTags);
				ParseMetadataTags(Path.Combine(MetadataDir, "xml\\namespace_u_p.xml"), MetadataLookup.PropertyTags);
				ParseMetadataTags(Path.Combine(MetadataDir, "xml\\namespace_u_s.xml"), MetadataLookup.StructTags);
				ParseMetadataTags(Path.Combine(MetadataDir, "xml\\namespace_u_m.xml"), MetadataLookup.MetaTags);

				// Rebuild all the reference names now that we've parsed a bunch of new tags
				MetadataLookup.BuildReferenceNameList();

				// Write it to a summary file
				MetadataLookup.Save(MetadataPath);
				return true;
			}
			return false;
		}

		static bool ParseMetadataTags(string InputFile, Dictionary<string, string> Tags)
		{
			XmlDocument Document = Utility.TryReadXmlDocument(InputFile);
			if (Document != null)
			{
				XmlNode EnumNode = Document.SelectSingleNode("doxygen/compounddef/sectiondef/memberdef");
				if (EnumNode != null && EnumNode.Attributes["kind"].Value == "enum")
				{
					using (XmlNodeList ValueNodeList = EnumNode.SelectNodes("enumvalue"))
					{
						foreach (XmlNode ValueNode in ValueNodeList)
						{
							string Name = ValueNode.SelectSingleNode("name").InnerText;

							string Description = ValueNode.SelectSingleNode("briefdescription").InnerText;
							if (Description == null) Description = ValueNode.SelectSingleNode("detaileddescription").InnerText;

							Description = Description.Trim();
							if (Description.StartsWith("[") && Description.Contains(']')) Description = Description.Substring(Description.IndexOf(']') + 1);

							Tags.Add(MetadataLookup.GetReferenceName(Name), Description);
						}
					}
					return true;
				}
			}
			return false;
		}

		static void CleanXml(string XmlDir)
		{
			Console.WriteLine("Cleaning '{0}'", XmlDir);
			Utility.SafeDeleteDirectoryContents(XmlDir, true);
		}

		static bool BuildXml(string EngineDir, string TargetInfoPath, string DoxygenPath, string XmlDir, List<string> Filters = null, bool bVerbose = false)
		{
			// Read the target that we're building
			BuildTarget Target = BuildTarget.Read(TargetInfoPath);

			// Create an invariant list of exclude directories
			string[] InvariantExcludeDirectories = ExcludeSourceDirectories.Select(x => x.ToLowerInvariant()).ToArray();

			// Flatten the target into a list of modules
			List<BuildModule> InputModules = new List<BuildModule>();
			foreach (BuildBinary Binary in Target.Binaries.Where(x => x.Type == "cpp"))
			{
				foreach(BuildModule Module in Binary.Modules)
				{
					if(Module.Type == "cpp" && !Module.Path.ToLowerInvariant().Contains("\\engine\\plugins\\"))
					{
						if(!Module.Path.ToLowerInvariant().Split('\\', '/').Any(x => InvariantExcludeDirectories.Contains(x)))
						{
							InputModules.Add(Module);
						}
					}
				}
			}

#if TEST_INDEX_PAGE
			// Just use all the input modules
			List<BuildModule> ParsedModules = new List<BuildModule>(InputModules);
#else
			// Filter the input module list
			if (Filters != null && Filters.Count > 0)
			{
				InputModules = new List<BuildModule>(InputModules.Where(x => Filters.Exists(y => y.StartsWith(x.Name + "/", StringComparison.InvariantCultureIgnoreCase))));
			}

			// Sort all the modules into the required build order
			Console.WriteLine("Sorting module parse order...");
			List<BuildModule> SortedModules = SortModules(InputModules);

			// List all the modules which have circular dependencies
			foreach (BuildModule Module in SortedModules)
			{
				if (SortedModules.Count(x => x == Module) > 1)
				{
					Console.WriteLine("  Circular dependency on {0}; module will be parsed twice", Module.Name);
				}
			}

			// Set our error mode so as to not bring up the WER dialog if Doxygen crashes (which it often does)
			SetErrorMode(0x0007);

			// Create the output directory
			Utility.SafeCreateDirectory(XmlDir);

			// Build all the modules
			List<BuildModule> ParsedModules = new List<BuildModule>();
			Dictionary<string, string> ModuleToTagfile = new Dictionary<string, string>();
			for (int Idx = 0; Idx < SortedModules.Count; Idx++)
			{
				BuildModule Module = SortedModules[Idx];
				Console.WriteLine("Parsing source for {0}... ({1}/{2})", Module.Name, Idx + 1, SortedModules.Count);
				string ModuleDir = Path.Combine(XmlDir, Module.Name);

				// Build the configuration for this module
				DoxygenConfig Config = new DoxygenConfig(Module.Name, Module.Path, ModuleDir);
				Config.Definitions.AddRange(Module.Definitions.Select(x => x.Replace("DLLIMPORT", "").Replace("DLLEXPORT", "")));
				Config.Definitions.AddRange(DoxygenPredefinedMacros);
				Config.ExpandAsDefined.AddRange(DoxygenExpandedMacros);
				Config.IncludePaths.AddRange(Module.IncludePaths.Select(x => ExpandIncludePath(EngineDir, x).TrimEnd('\\')));
				Config.ExcludePatterns.AddRange(ExcludeSourceDirectories.Select(x => "*/" + x + "/*"));
				Config.ExcludePatterns.AddRange(ExcludeSourceFiles);
				Config.OutputTagfile = Path.Combine(ModuleDir, "tags.xml");

				// Add all the valid tagfiles
				foreach (string Dependency in Module.Dependencies)
				{
					string Tagfile;
					if (ModuleToTagfile.TryGetValue(Dependency, out Tagfile))
					{
						Config.InputTagfiles.Add(Tagfile);
					}
				}

				// Run doxygen
				if (Doxygen.Run(DoxygenPath, Path.Combine(EngineDir, "Source"), Config, bVerbose))
				{
					if (!ModuleToTagfile.ContainsKey(Module.Name))
					{
						ModuleToTagfile.Add(Module.Name, Config.OutputTagfile);
						ParsedModules.Add(Module);
					}
				}
				else
				{
					Console.WriteLine("  Doxygen crashed. Skipping.");
				}
			}
#endif

			// Write the modules file
			WriteModulesXml(XmlDir, ParsedModules);
			return true;
		}

		static string ExpandEnvironmentVariables(string Text)
		{
			int StartIdx = -1;
			for (int Idx = 0; Idx < Text.Length; Idx++)
			{
				if (Text[Idx] == '$' && (Idx + 1 < Text.Length && Text[Idx + 1] == '('))
				{
					// Save the start of a variable name
					StartIdx = Idx;
				}
				else if(Text[Idx] == ')' && StartIdx != -1)
				{
					// Replace the variable
					string Name = Text.Substring(StartIdx + 2, Idx - (StartIdx + 2));
					string Value = Environment.GetEnvironmentVariable(Name);
					if (Value != null)
					{
						Text = Text.Substring(0, StartIdx) + Value + Text.Substring(Idx + 1);
						Idx = StartIdx + Value.Length - 1;
					}
					StartIdx = -1;
				}
			}
			return Text;
		}

		static string ExpandIncludePath(string EngineDir, string IncludePath)
		{
			// Expand any environment variables in it
			string NewIncludePath = ExpandEnvironmentVariables(IncludePath);

			// Convert it to a full path
			return Path.GetFullPath(Path.Combine(EngineDir, "Source", NewIncludePath));
		}

		static void WriteModulesXml(string XmlDir, List<BuildModule> ParsedModules)
		{
			// Create XML settings for output
			XmlWriterSettings Settings = new XmlWriterSettings();
			Settings.Indent = true;

			// Save a list of all the converted modules
			string ModuleXmlPath = Path.Combine(XmlDir, "modules.xml");
			using(XmlWriter Writer = XmlWriter.Create(ModuleXmlPath, Settings))
			{
				Writer.WriteStartElement("modules");
				foreach(BuildModule Module in ParsedModules)
				{
					Writer.WriteStartElement("module");
					Writer.WriteAttributeString("name", Module.Name);
					Writer.WriteAttributeString("source", Module.Path);
					Writer.WriteEndElement();
				}
				Writer.WriteEndElement();
			}
		}

		static void CleanUdn(string UdnDir)
		{
			string CleanDir = Path.Combine(UdnDir, "API");
			Console.WriteLine("Cleaning '{0}'", CleanDir);

			// Delete all the files
			foreach(string FileName in Directory.EnumerateFiles(CleanDir))
			{
				Utility.SafeDeleteFile(Path.Combine(CleanDir, FileName));
			}

			// Delete all the subdirectories
			foreach (string SubDir in Directory.EnumerateDirectories(CleanDir))
			{
				Utility.SafeDeleteDirectory(SubDir);
			}
		}

		static bool BuildUdn(string XmlDir, string UdnDir, string SitemapDir, string MetadataPath, string StatsPath, List<string> Filters = null)
		{
			// Read the metadata
			MetadataLookup.Load(MetadataPath);

			// Read the input module index
			XmlDocument Document = Utility.ReadXmlDocument(Path.Combine(XmlDir, "modules.xml"));

			// Read the list of modules
			List<KeyValuePair<string, string>> InputModules = new List<KeyValuePair<string,string>>();
			using (XmlNodeList NodeList = Document.SelectNodes("modules/module"))
			{
				foreach (XmlNode Node in NodeList)
				{
					string Name = Node.Attributes["name"].Value;
					string Source = Node.Attributes["source"].Value;
					InputModules.Add(new KeyValuePair<string, string>(Name, Source));
				}
			}

#if TEST_INDEX_PAGE
			// Just create empty modules
			List<DoxygenModule> Modules = new List<DoxygenModule>();
			for (int Idx = 0; Idx < InputModules.Count; Idx++)
			{
				Modules.Add(new DoxygenModule(InputModules[Idx].Key, InputModules[Idx].Value));
			}
#else
			// Filter the input module list
			if (Filters != null && Filters.Count > 0)
			{
				InputModules = new List<KeyValuePair<string, string>>(InputModules.Where(x => Filters.Exists(y => y.StartsWith(x.Key + "/", StringComparison.InvariantCultureIgnoreCase))));
			}

			// Read all the doxygen modules
			List<DoxygenModule> Modules = new List<DoxygenModule>();
			for(int Idx = 0; Idx < InputModules.Count; Idx++)
			{
				Console.WriteLine("Reading module {0}... ({1}/{2})", InputModules[Idx].Key, Idx + 1, InputModules.Count);
				Modules.Add(DoxygenModule.Read(InputModules[Idx].Key, InputModules[Idx].Value, Path.Combine(XmlDir, InputModules[Idx].Key, "xml")));
			}

			// Now filter all the entities in each module
			if(Filters != null && Filters.Count > 0)
			{
				FilterEntities(Modules, Filters);
			}
#endif

			// Create the index page, and all the pages below it
			APIIndex Index = new APIIndex(Modules);

			// Build a list of pages to output
			List<APIPage> OutputPages = new List<APIPage>(Index.GatherPages().OrderBy(x => x.LinkPath));

			// Dump the output stats
			if (StatsPath != null)
			{
				Console.WriteLine("Writing stats to " + StatsPath + "...");
				Stats NewStats = new Stats(OutputPages.OfType<APIMember>());
				NewStats.Write(StatsPath);
			}

			// Setup the output directory 
			Utility.SafeCreateDirectory(UdnDir);

			// Build the manifest
			Console.WriteLine("Writing manifest...");
			UdnManifest Manifest = new UdnManifest(Index);
			Manifest.PrintConflicts();
			Manifest.Write(Path.Combine(UdnDir, APIFolder + "\\API.manifest"));

			// Write all the pages
			using (Tracker UdnTracker = new Tracker("Writing UDN pages...", OutputPages.Count))
			{
				foreach(int Idx in UdnTracker.Indices)
				{
					APIPage Page = OutputPages[Idx];

					// Create the output directory
					string MemberDirectory = Path.Combine(UdnDir, Page.LinkPath);
					if (!Directory.Exists(MemberDirectory))
					{
						Directory.CreateDirectory(MemberDirectory);
					}

					// Write the page
					Page.WritePage(Manifest, Path.Combine(MemberDirectory, "index.INT.udn"));
				}
			}

			// Write the sitemap contents
			Console.WriteLine("Writing sitemap contents...");
			Index.WriteSitemapContents(Path.Combine(SitemapDir, SitemapContentsFileName));

			// Write the sitemap index
			Console.WriteLine("Writing sitemap index...");
			Index.WriteSitemapIndex(Path.Combine(SitemapDir, SitemapIndexFileName));

			return true;
		}

		public static IEnumerable<APIPage> GatherPages(params APIPage[] RootSet)
		{
			// Visit all the pages and collect all their references
			HashSet<APIPage> PendingSet = new HashSet<APIPage>(RootSet);
			HashSet<APIPage> VisitedSet = new HashSet<APIPage>();
			while (PendingSet.Count > 0)
			{
				APIPage Page = PendingSet.First();

				List<APIPage> ReferencedPages = new List<APIPage>();
				Page.GatherReferencedPages(ReferencedPages);

				foreach (APIPage ReferencedPage in ReferencedPages)
				{
					if (!VisitedSet.Contains(ReferencedPage))
					{
						PendingSet.Add(ReferencedPage);
					}
				}

				PendingSet.Remove(Page);
				VisitedSet.Add(Page);
			}
			return VisitedSet;
		}

		public static void FilterEntities(List<DoxygenModule> Modules, List<string> Filters)
		{
			foreach (DoxygenModule Module in Modules)
			{
				HashSet<DoxygenEntity> FilteredEntities = new HashSet<DoxygenEntity>();
				foreach (DoxygenEntity Entity in Module.Entities)
				{
					if(Filters.Exists(x => FilterEntity(Entity, x)))
					{
						for(DoxygenEntity AddEntity = Entity; AddEntity != null; AddEntity = AddEntity.Parent)
						{
							FilteredEntities.Add(AddEntity);
						}
					}
				}
				Module.Entities = new List<DoxygenEntity>(FilteredEntities);
			}
		}

		public static bool FilterEntity(DoxygenEntity Entity, string Filter)
		{
			// Check the module matches
			if(!Filter.StartsWith(Entity.Module.Name + "/", StringComparison.InvariantCultureIgnoreCase))
			{
				return false;
			}

			// Remove the module from the start of the filter
			string PathFilter = Filter.Substring(Entity.Module.Name.Length + 1);
			
			// Now check what sort of filter it is
			if (PathFilter == "...")
			{
				// Let anything through matching the module name, regardless of which subdirectory it's in (maybe it doesn't match a normal subdirectory at all)
				return true;
			}
			else if(PathFilter.EndsWith("/..."))
			{
				// Remove the ellipsis
				PathFilter = PathFilter.Substring(0, PathFilter.Length - 3);

				// Check the entity starts with the base directory
				if(!Entity.File.StartsWith(Entity.Module.BaseSrcDir, StringComparison.InvariantCultureIgnoreCase))
				{
					return false;
				}

				// Get the entity path, ignoring the 
				int EntityMinIdx = Entity.File.IndexOf('\\', Entity.Module.BaseSrcDir.Length);
				string EntityPath = Entity.File.Substring(EntityMinIdx + 1).Replace('\\', '/');

				// Get the entity path. Ignore the first directory under the module directory, as it'll be public/private/classes etc...
				return EntityPath.StartsWith(PathFilter, StringComparison.InvariantCultureIgnoreCase);
			}
			else
			{
				// Get the full entity name
				string EntityFullName = Entity.Name;
				for (DoxygenEntity ParentEntity = Entity.Parent; ParentEntity != null; ParentEntity = ParentEntity.Parent)
				{
					EntityFullName = ParentEntity.Name + "::" + EntityFullName;
				}

				// Compare it to the filter
				return Filter == (Entity.Module.Name + "/" + EntityFullName);
			}
		}

		public static List<BuildModule> SortModules(IEnumerable<BuildModule> Modules)
		{
			List<BuildModule> OutputModules = new List<BuildModule>();

			// Build a set of unbuilt module names
			HashSet<string> UnbuiltModules = new HashSet<string>();
			foreach (BuildModule Module in Modules)
			{
				UnbuiltModules.Add(Module.Name);
			}

			// Build the output list by iteratively removing modules whose dependencies are met
			LinkedList<BuildModule> RemainingModules = new LinkedList<BuildModule>(Modules);
			while(RemainingModules.Count > 0)
			{
				int InitialOutputModuleCount = OutputModules.Count;

				// Move all the modules whose dependencies are met to the output list
				for(LinkedListNode<BuildModule> ModuleNode = RemainingModules.First; ModuleNode != null; )
				{
					LinkedListNode<BuildModule> NextModuleNode = ModuleNode.Next;
					if(HasAllDependencies(ModuleNode.Value, UnbuiltModules))
					{
						OutputModules.Add(ModuleNode.Value);
						UnbuiltModules.Remove(ModuleNode.Value.Name);
						RemainingModules.Remove(ModuleNode);
					}
					ModuleNode = NextModuleNode;
				}

				// If we didn't manage to build anything on this iteration, we've got a recursive dependency. Try to break it by builting the first module that hasn't been seen 
				// yet (but still build it a second time later)
				if(OutputModules.Count == InitialOutputModuleCount)
				{
					for (LinkedListNode<BuildModule> ModuleNode = RemainingModules.First; ModuleNode != null; ModuleNode = ModuleNode.Next)
					{
						BuildModule Module = ModuleNode.Value;
						if(UnbuiltModules.Contains(Module.Name))
						{
							OutputModules.Add(Module);
							UnbuiltModules.Remove(Module.Name);
							break;
						}
					}
				}
			}
			return OutputModules;
		}

		public static bool HasAllDependencies(BuildModule Module, HashSet<string> UnbuiltModules)
		{
			foreach (string Dependency in Module.Dependencies)
			{
				if (UnbuiltModules.Contains(Dependency))
				{
					return false;
				}
			}
			return true;
		}

		public static void CleanHtml(string HtmlPath)
		{
			string CleanDir = Path.Combine(HtmlPath, "INT\\API");
			Console.WriteLine("Cleaning '{0}'", CleanDir);
			Utility.SafeDeleteDirectoryContents(CleanDir, true);
		}

		public static bool BuildHtml(string EngineDir, string DocToolPath, string UdnPath)
		{
			using (Process DocToolProcess = new Process())
			{
				DocToolProcess.StartInfo.WorkingDirectory = EngineDir;
				DocToolProcess.StartInfo.FileName = DocToolPath;
				DocToolProcess.StartInfo.Arguments = "API\\* -lang=INT -t=DefaultAPI.html";
				DocToolProcess.StartInfo.UseShellExecute = false;
				DocToolProcess.StartInfo.RedirectStandardOutput = true;
				DocToolProcess.StartInfo.RedirectStandardError = true;

				DocToolProcess.OutputDataReceived += new DataReceivedEventHandler(ProcessOutputReceived);
				DocToolProcess.ErrorDataReceived += new DataReceivedEventHandler(ProcessOutputReceived);

				try
				{
					DocToolProcess.Start();
					DocToolProcess.BeginOutputReadLine();
					DocToolProcess.BeginErrorReadLine();
					DocToolProcess.WaitForExit();
					return DocToolProcess.ExitCode == 0;
				}
				catch (Exception Ex)
				{
					Console.WriteLine(Ex.ToString() + "\n" + Ex.StackTrace);
					return false;
				}
			}
		}

		public static void CleanChm(string ChmPath)
		{
			Console.WriteLine("Cleaning '{0}'", ChmPath);
			Utility.SafeDeleteDirectoryContents(ChmPath, true);
		}

		public static bool BuildChm(string ChmCompilerPath, string BaseHtmlDir, string ChmDir)
		{
			const string ProjectFileName = "API.hhp";
			Console.WriteLine("Searching for CHM input files...");

			// Build a list of all the files we want to copy
			List<string> FilePaths = new List<string>();
			List<string> DirectoryPaths = new List<string>();
			Utility.FindRelativeContents(BaseHtmlDir, "Images\\api*", false, FilePaths, DirectoryPaths);
			Utility.FindRelativeContents(BaseHtmlDir, "Include\\*", true, FilePaths, DirectoryPaths);

			// Find all the HTML files
			List<string> HtmlFilePaths = new List<string>();
			Utility.FindRelativeContents(BaseHtmlDir, "INT\\API\\*.html", true, HtmlFilePaths, DirectoryPaths);

			// Create all the target directories
			foreach (string DirectoryPath in DirectoryPaths)
			{
				Utility.SafeCreateDirectory(Path.Combine(ChmDir, DirectoryPath));
			}

			// Copy all the files across
			using (Tracker CopyTracker = new Tracker("Copying support files...", FilePaths.Count))
			{
				foreach(int Idx in CopyTracker.Indices)
				{
					Utility.SafeCopyFile(Path.Combine(BaseHtmlDir, FilePaths[Idx]), Path.Combine(ChmDir, FilePaths[Idx]));
				}
			}

			// Copy the HTML files across, fixing up the HTML for display in the CHM window
			using (Tracker HtmlTracker = new Tracker("Copying html files...", HtmlFilePaths.Count))
			{
				foreach(int Idx in HtmlTracker.Indices)
				{
					string HtmlFilePath = HtmlFilePaths[Idx];
					string HtmlText = File.ReadAllText(Path.Combine(BaseHtmlDir, HtmlFilePath));

					HtmlText = HtmlText.Replace("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n", "");
					HtmlText = HtmlText.Replace("<div id=\"crumbs_bg\"></div>", "");

					const string HeaderEndText = "<!-- end head -->";
					int HeaderMinIdx = HtmlText.IndexOf("<div id=\"head\">");
					int HeaderMaxIdx = HtmlText.IndexOf(HeaderEndText);
					HtmlText = HtmlText.Remove(HeaderMinIdx, HeaderMaxIdx + HeaderEndText.Length - HeaderMinIdx);

					int CrumbsMinIdx = HtmlText.IndexOf("<div class=\"crumbs\">");
					int HomeMinIdx = HtmlText.IndexOf("<strong>", CrumbsMinIdx);
					int HomeMaxIdx = HtmlText.IndexOf("&gt;", HomeMinIdx) + 4;
					HtmlText = HtmlText.Remove(HomeMinIdx, HomeMaxIdx - HomeMinIdx);

					File.WriteAllText(Path.Combine(ChmDir, HtmlFilePath), HtmlText);
				}
			}

			// Write the project file
			using (StreamWriter Writer = new StreamWriter(Path.Combine(ChmDir, ProjectFileName)))
			{
				Writer.WriteLine("[OPTIONS]");
				Writer.WriteLine("Title=UE4 API Documentation");
				Writer.WriteLine("Binary TOC=Yes");
				Writer.WriteLine("Compatibility=1.1 or later");
				Writer.WriteLine("Compiled file=API.chm");
				Writer.WriteLine("Contents file=" + SitemapContentsFileName);
				Writer.WriteLine("Index file=" + SitemapIndexFileName);
				Writer.WriteLine("Default topic=INT\\API\\index.html");
				Writer.WriteLine("Full-text search=Yes");
				Writer.WriteLine("Display compile progress=Yes");
				Writer.WriteLine("Language=0x409 English (United States)");
				Writer.WriteLine();
				Writer.WriteLine("[FILES]");
				foreach (string FilePath in FilePaths)
				{
					Writer.WriteLine(FilePath);
				}
				foreach (string HtmlFilePath in HtmlFilePaths)
				{
					Writer.WriteLine(HtmlFilePath);
				}
			}

			// Compile the project
			Console.WriteLine("Compiling CHM file...");
			using (Process CompilerProcess = new Process())
			{
				CompilerProcess.StartInfo.WorkingDirectory = ChmDir;
				CompilerProcess.StartInfo.FileName = ChmCompilerPath;
				CompilerProcess.StartInfo.Arguments = ProjectFileName;
				CompilerProcess.StartInfo.UseShellExecute = false;
				CompilerProcess.StartInfo.RedirectStandardOutput = true;
				CompilerProcess.StartInfo.RedirectStandardError = true;
				CompilerProcess.OutputDataReceived += ProcessOutputReceived;
				CompilerProcess.ErrorDataReceived += ProcessOutputReceived;
				CompilerProcess.Start();
				CompilerProcess.BeginOutputReadLine();
				CompilerProcess.BeginErrorReadLine();
				CompilerProcess.WaitForExit();
			}
			return true;
		}

		static private void ProcessOutputReceived(Object Sender, DataReceivedEventArgs Line)
		{
			if (Line.Data != null && Line.Data.Length > 0)
			{
				Console.WriteLine(Line.Data);
			}
		}

		public static void FindAllMembers(APIMember Member, List<APIMember> Members)
		{
			Members.Add(Member);

			foreach (APIMember Child in Member.Children)
			{
				FindAllMembers(Child, Members);
			}
		}

		public static void FindAllMatchingParents(APIMember Member, List<APIMember> Members, string[] FilterPaths)
		{
			if (Utility.MatchPath(Member.LinkPath + "\\", FilterPaths))
			{
				Members.Add(Member);
			}
			else
			{
				foreach (APIMember Child in Member.Children)
				{
					FindAllMatchingParents(Child, Members, FilterPaths);
				}
			}
		}
		/*
		public static string ParseXML(XmlNode ParentNode, XmlNode ChildNode, string Indent)
		{
//			string Indent = "";
//			for(int Idx = 0; Idx < TabDepth; Idx++) Indent += "\t";
			return Markdown.ParseXml(ChildNode, Indent, ResolveLink);
		}
		*/
		public static string ResolveLink(string Id)
		{
			APIMember RefMember = APIMember.ResolveRefLink(Id);
			return (RefMember != null) ? RefMember.LinkPath : null;
		}
    }
}
