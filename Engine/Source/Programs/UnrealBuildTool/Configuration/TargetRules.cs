using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnrealBuildTool
{
	/// <summary>
	/// TargetRules is a data structure that contains the rules for defining a target (application/executable)
	/// </summary>
	public abstract class TargetRules
	{
		/// <summary>
		/// The type of target
		/// </summary>
		[Serializable]
		public enum TargetType
		{
			/// <summary>
			/// Cooked monolithic game executable (GameName.exe).  Also used for a game-agnostic engine executable (UE4Game.exe or RocketGame.exe)
			/// </summary>
			Game,

			/// <summary>
			/// Uncooked modular editor executable and DLLs (UE4Editor.exe, UE4Editor*.dll, GameName*.dll)
			/// </summary>
			Editor,

			/// <summary>
			/// Cooked monolithic game client executable (GameNameClient.exe, but no server code)
			/// </summary>
			Client,

			/// <summary>
			/// Cooked monolithic game server executable (GameNameServer.exe, but no client code)
			/// </summary>
			Server,

			/// <summary>
			/// Program (standalone program, e.g. ShaderCompileWorker.exe, can be modular or monolithic depending on the program)
			/// </summary>
			Program,
		}

		/// <summary>
		/// Specifies how to link all the modules in this target
		/// </summary>
		[Serializable]
		public enum TargetLinkType
		{
			/// <summary>
			/// Use the default link type based on the current target type
			/// </summary>
			Default,

			/// <summary>
			/// Link all modules into a single binary
			/// </summary>
			Monolithic,

			/// <summary>
			/// Link modules into individual dynamic libraries
			/// </summary>
			Modular,
		}

		/// <summary>
		/// Type of target
		/// </summary>
		public TargetType Type = TargetType.Game;

		/// <summary>
		/// The name of the game, this is set up by the rules compiler after it compiles and constructs this
		/// </summary>
		public string TargetName = null;

		/// <summary>
		/// Whether the target uses Steam (todo: substitute with more generic functionality)
		/// </summary>
		public bool bUsesSteam;

		/// <summary>
		/// Whether the target uses CEF3
		/// </summary>
		public bool bUsesCEF3;

		/// <summary>
		/// Whether the project uses visual Slate UI (as opposed to the low level windowing/messaging which is always used)
		/// </summary>
		public bool bUsesSlate = true;

		/// <summary>
		/// Forces linking against the static CRT. This is not supported across the engine due to the need for allocator implementations to be shared (for example), and TPS 
		/// libraries to be consistent with each other, but can be used for utility programs.
		/// </summary>
		public bool bUseStaticCRT = false;

		/// <summary>
		/// By default we use the Release C++ Runtime (CRT), even when compiling Debug builds.  This is because the Debug C++
		/// Runtime isn't very useful when debugging Unreal Engine projects, and linking against the Debug CRT libraries forces
		/// our third party library dependencies to also be compiled using the Debug CRT (and often perform more slowly.)  Often
		/// it can be inconvenient to require a separate copy of the debug versions of third party static libraries simply
		/// so that you can debug your program's code.
		/// </summary>
		public bool bDebugBuildsActuallyUseDebugCRT = false;

		/// <summary>
		/// Whether the output from this target can be publicly distributed, even if it has
		/// dependencies on modules that are not (i.e. CarefullyRedist, NotForLicensees, NoRedist).
		/// This should be used when you plan to release binaries but not source.
		/// </summary>
		public bool bOutputPubliclyDistributable = false;

		/// <summary>
		/// Specifies the configuration whose binaries do not require a "-Platform-Configuration" suffix.
		/// </summary>
		public UnrealTargetConfiguration UndecoratedConfiguration = UnrealTargetConfiguration.Development;

		/// <summary>
		/// Build all the plugins that we can find, even if they're not enabled. This is particularly useful for content-only projects, 
		/// where you're building the UE4Editor target but running it with a game that enables a plugin.
		/// </summary>
		public bool bBuildAllPlugins = false;

		/// <summary>
		/// A list of additional plugins which need to be included in this target. This allows referencing non-optional plugin modules
		/// which cannot be disabled, and allows building against specific modules in program targets which do not fit the categories
		/// in ModuleHostType.
		/// </summary>
		public List<string> AdditionalPlugins = new List<string>();

		/// <summary>
		/// Path to the set of pak signing keys to embed in the executable
		/// </summary>
		public string PakSigningKeysFile = "";

		/// <summary>
		/// Allows a Program Target to specify it's own solution folder path
		/// </summary>
		public string SolutionDirectory = String.Empty;

		/// <summary>
		/// If true, the built target goes into the Engine/Binaries/<PLATFORM> folder
		/// </summary>
		public bool bOutputToEngineBinaries = false;

		/// <summary>
		/// Sub folder where the built target goes: Engine/Binaries/<PLATFORM>/<SUBDIR>
		/// </summary>
		public string ExeBinariesSubFolder = String.Empty;

		/// <summary>
		/// Allow target module to override UHT code generation version.
		/// </summary>
		public EGeneratedCodeVersion GeneratedCodeVersion = EGeneratedCodeVersion.None;

		/// <summary>
		/// Specifies how to link modules in this target (monolithic or modular). This is currently protected for backwards compatibility. Call the GetLinkType() accessor
		/// until support for the deprecated ShouldCompileMonolithic() override has been removed.
		/// </summary>
		public TargetLinkType LinkType
		{
			get
			{
				return (LinkTypePrivate != TargetLinkType.Default) ? LinkTypePrivate : ((Type == TargetType.Editor) ? TargetLinkType.Modular : TargetLinkType.Monolithic);
			}
			set
			{
				LinkTypePrivate = value;
			}
		}

		/// <summary>
		/// Backing storage for the LinkType property.
		/// </summary>
		public TargetLinkType LinkTypePrivate = TargetLinkType.Default;

		/// <summary>
		/// Accessor to return the link type for this module, including support for setting LinkType, or for overriding ShouldCompileMonolithic().
		/// </summary>
		/// <param name="Platform">The platform being compiled.</param>
		/// <param name="Configuration">The configuration being compiled.</param>
		/// <returns>Link type for the given combination</returns>
		internal TargetRules.TargetLinkType GetLegacyLinkType(UnrealTargetPlatform Platform, UnrealTargetConfiguration Configuration)
		{
#pragma warning disable 0612
			if (GetType().GetMethod("ShouldCompileMonolithic").DeclaringType != typeof(TargetRules))
			{
				return ShouldCompileMonolithic(Platform, Configuration) ? TargetRules.TargetLinkType.Monolithic : TargetRules.TargetLinkType.Modular;
			}
			else
			{
				return LinkType;
			}
#pragma warning restore 0612
		}

		/// <summary>
		/// Whether this target should be compiled in monolithic mode
		/// </summary>
		/// <param name="InPlatform">The platform being built</param>
		/// <param name="InConfiguration">The configuration being built</param>
		/// <returns>true if it should, false if not</returns>
		[ObsoleteOverride("ShouldCompileMonolithic() is deprecated in 4.15. Please set LinkType = TargetLinkType.Monolithic or LinkType = TargetLinkType.Modular in your target's constructor instead.")]
		public virtual bool ShouldCompileMonolithic(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			return Type != TargetType.Editor;
		}

		/// <summary>
		/// Give the target an opportunity to override toolchain settings
		/// </summary>
		/// <param name="Target">The target currently being setup</param>
		/// <returns>true if successful, false if not</returns>
		public virtual bool ConfigureToolchain(TargetInfo Target)
		{
			return true;
		}

		/// <summary>
		/// Get the supported platforms for this target. 
		/// This function is deprecated in 4.15, and a warning will be given by RulesCompiler if it is implemented by a derived class.
		/// </summary>
		/// <param name="OutPlatforms">The list of platforms supported</param>
		/// <returns>true if successful, false if not</returns>
		[ObsoleteOverride("GetSupportedPlatforms() is deprecated in the 4.15 release. Add a [SupportedPlatforms(...)] attribute to your TargetRules class instead.")]
		public virtual bool GetSupportedPlatforms(ref List<UnrealTargetPlatform> OutPlatforms)
		{
			if (Type == TargetType.Program)
			{
				OutPlatforms = new List<UnrealTargetPlatform>(Utils.GetPlatformsInClass(UnrealPlatformClass.Desktop));
				return true;
			}
			else if (Type == TargetType.Editor)
			{
				OutPlatforms = new List<UnrealTargetPlatform>(Utils.GetPlatformsInClass(UnrealPlatformClass.Editor));
				return true;
			}
			else
			{
				OutPlatforms = new List<UnrealTargetPlatform>(Utils.GetPlatformsInClass(UnrealPlatformClass.All));
				return true;
			}
		}

		/// <summary>
		/// Get the supported configurations for this target
		/// </summary>
		/// <param name="OutConfigurations">The list of configurations supported</param>
		/// <returns>true if successful, false if not</returns>
		[ObsoleteOverride("GetSupportedConfigurations() is deprecated in the 4.15 release. Add a [SupportedConfigurations(...)] attribute to your TargetRules class instead.")]
		public virtual bool GetSupportedConfigurations(ref List<UnrealTargetConfiguration> OutConfigurations, bool bIncludeTestAndShippingConfigs)
		{
			if (Type == TargetType.Program)
			{
				// By default, programs are Debug and Development only.
				OutConfigurations.Add(UnrealTargetConfiguration.Debug);
				OutConfigurations.Add(UnrealTargetConfiguration.Development);
			}
			else
			{
				// By default all games support all configurations
				foreach (UnrealTargetConfiguration Config in Enum.GetValues(typeof(UnrealTargetConfiguration)))
				{
					if (Config != UnrealTargetConfiguration.Unknown)
					{
						// Some configurations just don't make sense for the editor
						if (Type == TargetType.Editor &&
							(Config == UnrealTargetConfiguration.Shipping || Config == UnrealTargetConfiguration.Test))
						{
							// We don't currently support a "shipping" editor config
						}
						else if (!bIncludeTestAndShippingConfigs &&
							(Config == UnrealTargetConfiguration.Shipping || Config == UnrealTargetConfiguration.Test))
						{
							// User doesn't want 'Test' or 'Shipping' configs in their project files
						}
						else
						{
							OutConfigurations.Add(Config);
						}
					}
				}
			}

			return (OutConfigurations.Count > 0) ? true : false;
		}

		/// <summary>
		/// Setup the binaries associated with this target.
		/// </summary>
		/// <param name="Target">The target information - such as platform and configuration</param>
		/// <param name="OutBuildBinaryConfigurations">Output list of binaries to generated</param>
		/// <param name="OutExtraModuleNames">Output list of extra modules that this target could utilize</param>
		public virtual void SetupBinaries(
			TargetInfo Target,
			ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
			ref List<string> OutExtraModuleNames
			)
		{
		}

		/// <summary>
		/// Setup the global environment for building this target
		/// IMPORTANT: Game targets will *not* have this function called if they use the shared build environment.
		/// See ShouldUseSharedBuildEnvironment().
		/// </summary>
		/// <param name="Target">The target information - such as platform and configuration</param>
		/// <param name="OutLinkEnvironmentConfiguration">Output link environment settings</param>
		/// <param name="OutCPPEnvironmentConfiguration">Output compile environment settings</param>
		public virtual void SetupGlobalEnvironment(
			TargetInfo Target,
			ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
			ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
			)
		{
		}

		/// <summary>
		/// Allows a target to choose whether to use the shared build environment for a given configuration. Using
		/// the shared build environment allows binaries to be reused between targets, but prevents customizing the
		/// compile environment through SetupGlobalEnvironment().
		/// </summary>
		/// <param name="Target">Information about the target</param>
		/// <returns>True if the target should use the shared build environment</returns>
		public virtual bool ShouldUseSharedBuildEnvironment(TargetInfo Target)
		{
			return UnrealBuildTool.IsEngineInstalled() || (Target.Type != TargetType.Program && !Target.IsMonolithic);
		}

		/// <summary>
		/// Allows the target to specify modules which can be precompiled with the -Precompile/-UsePrecompiled arguments to UBT. 
		/// All dependencies of the specified modules will be included.
		/// </summary>
		/// <param name="Target">The target information, such as platform and configuration</param>
		/// <param name="ModuleNames">List which receives module names to precompile</param>
		[ObsoleteOverride("GetModulesToPrecompile() is deprecated in the 4.11 release. The -precompile option to UBT now automatically compiles all engine modules compatible with the current target.")]
		public virtual void GetModulesToPrecompile(TargetInfo Target, List<string> ModuleNames)
		{
		}

		/// <summary>
		/// Allow target module to override UHT code generation version.
		/// </summary>
		[ObsoleteOverride("GetGeneratedCodeVersion() is deprecated in the 4.15 release. Set the GeneratedCodeVersion field from the TargetRules constructor instead.")]
		public virtual EGeneratedCodeVersion GetGeneratedCodeVersion()
		{
			return GeneratedCodeVersion;
		}
	}

	/// <summary>
	/// Read-only wrapper around an existing TargetRules instance. This exposes target settings to modules without letting them to modify the global environment.
	/// </summary>
	public partial class ReadOnlyTargetRules
	{
		/// <summary>
		/// The writeable TargetRules instance
		/// </summary>
		TargetRules Inner;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="Inner">The TargetRules instance to wrap around</param>
		public ReadOnlyTargetRules(TargetRules Inner)
		{
			this.Inner = Inner;
		}

		/// <summary>
		/// Accessors for fields on the inner TargetRules instance
		/// </summary>
		#region Read-only accessor properties 

		public TargetRules.TargetType Type
		{
			get { return Inner.Type; }
		}

		public bool bUsesSteam
		{
			get { return Inner.bUsesSteam; }
		}

		public bool bUsesCEF3
		{
			get { return Inner.bUsesCEF3; }
		}

		public bool bUsesSlate
		{
			get { return Inner.bUsesSlate; }
		}

		public bool bUseStaticCRT
		{
			get { return Inner.bUseStaticCRT; }
		}

		public bool bDebugBuildsActuallyUseDebugCRT
		{
			get { return Inner.bDebugBuildsActuallyUseDebugCRT; }
		}

		public bool bOutputPubliclyDistributable
		{
			get { return Inner.bOutputPubliclyDistributable; }
		}

		public UnrealTargetConfiguration UndecoratedConfiguration
		{
			get { return Inner.UndecoratedConfiguration; }
		}

		public bool bBuildAllPlugins
		{
			get { return Inner.bBuildAllPlugins; }
		}

		public IEnumerable<string> AdditionalPlugins
		{
			get { return Inner.AdditionalPlugins; }
		}

		public string PakSigningKeysFile
		{
			get { return Inner.PakSigningKeysFile; }
		}

		public string SolutionDirectory
		{
			get { return Inner.SolutionDirectory; }
		}

		public bool bOutputToEngineBinaries
		{
			get { return Inner.bOutputToEngineBinaries; }
		}

		public string ExeBinariesSubFolder
		{
			get { return Inner.ExeBinariesSubFolder; }
		}

		public EGeneratedCodeVersion GeneratedCodeVersion
		{
			get { return Inner.GeneratedCodeVersion; }
		}

		public TargetRules.TargetLinkType LinkType
		{
			get { return Inner.LinkType; }
		}

		#endregion
	}
}
