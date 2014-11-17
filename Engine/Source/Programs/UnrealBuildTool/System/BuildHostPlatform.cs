using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Runtime.InteropServices;

namespace UnrealBuildTool
{
	/// <summary>
	/// Host platform abstraction
	/// </summary>
	public abstract class BuildHostPlatform
	{
		private static BuildHostPlatform CurrentPlatform;
		private static bool bIsMac = File.Exists("/System/Library/CoreServices/SystemVersion.plist");

		/// <summary>
		/// Returns the name of platform UBT is running on. Internal use only. If you need access this this enum, use BuildHostPlatform.Current.Platform */
		/// </summary>
		private static UnrealTargetPlatform GetRuntimePlatform()
		{
			PlatformID Platform = Environment.OSVersion.Platform;
			switch (Platform)
			{
				case PlatformID.Win32NT:
					return UnrealTargetPlatform.Win64;
				case PlatformID.Unix:
					return bIsMac ? UnrealTargetPlatform.Mac : UnrealTargetPlatform.Linux;
				case PlatformID.MacOSX:
					return UnrealTargetPlatform.Mac;
				default:
					throw new BuildException("Unhandled runtime platform " + Platform);
			}
		}

		/// <summary>
		/// Host platform singleton.
		/// </summary>
		static public BuildHostPlatform Current
		{
			get 
			{
				if (CurrentPlatform == null)
				{
					switch (GetRuntimePlatform())
					{
						case UnrealTargetPlatform.Win64:
							CurrentPlatform = new WindowsBuildHostPlatform();
							break;
						case UnrealTargetPlatform.Mac:
							CurrentPlatform = new MacBuildHostPlatform();
							break;
						case UnrealTargetPlatform.Linux:
							CurrentPlatform = new LinuxBuildHostPlatform();
							break;
					}
				}
				return CurrentPlatform;
			}
		}

		/// <summary>
		/// Gets the current host platform type.
		/// </summary>
		abstract public UnrealTargetPlatform Platform { get; }

		/// <summary>
		/// Checks the API version of a dynamic library
		/// </summary>
		/// <param name="Filename">Filename of the library</param>
		/// <returns>API version of -1 if not found.</returns>
		abstract public int GetDllApiVersion(string Filename);
	}

	class WindowsBuildHostPlatform : BuildHostPlatform
	{
		public override UnrealTargetPlatform Platform
		{
			get { return UnrealTargetPlatform.Win64; }
		}

		[DllImport("kernel32.dll", SetLastError = true)]
		static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hFile, uint dwFlags);
		[DllImport("kernel32.dll", SetLastError = true)]
		static extern IntPtr FindResource(IntPtr hModule, string lpName, string lpType);
		[DllImport("kernel32.dll")]
		static extern IntPtr FindResource(IntPtr hModule, IntPtr lpID, IntPtr lpType);
		[DllImport("kernel32.dll", SetLastError = true)]
		static extern IntPtr LoadResource(IntPtr hModule, IntPtr hResInfo);
		[DllImport("kernel32.dll", SetLastError = true)]
		static extern uint SizeofResource(IntPtr hModule, IntPtr hResInfo);
		[DllImport("kernel32.dll", SetLastError = true)]
		static extern IntPtr LockResource(IntPtr hResData);
		[DllImport("kernel32.dll", SetLastError = true)]
		static extern void UnlockResource(IntPtr hResInfo);
		[DllImport("kernel32.dll", SetLastError = true)]
		static extern void FreeLibrary(IntPtr hModule);

		[Flags]
		enum LoadLibraryFlags : uint
		{
			DONT_RESOLVE_DLL_REFERENCES = 0x00000001,
			LOAD_LIBRARY_AS_DATAFILE = 0x00000002,
			LOAD_WITH_ALTERED_SEARCH_PATH = 0x00000008,
			LOAD_IGNORE_CODE_AUTHZ_LEVEL = 0x00000010,
			LOAD_LIBRARY_AS_IMAGE_RESOURCE = 0x00000020,
			LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE = 0x00000040,
			LOAD_LIBRARY_REQUIRE_SIGNED_TARGET = 0x00000080
		}

		enum ResourceType
		{
			RT_CURSOR = 1,
			RT_BITMAP = 2,
			RT_ICON = 3,
			RT_MENU = 4,
			RT_DIALOG = 5,
			RT_STRING = 6,
			RT_FONTDIR = 7,
			RT_FONT = 8,
			RT_ACCELERATOR = 9,
			RT_RCDATA = 10,
			RT_MESSAGETABLE = 11
		}

		public override int GetDllApiVersion(string Filename)
		{
			int Result = -1;

			try
			{
				const int ID_MODULE_API_VERSION_RESOURCE = 191;

				// Retrieves the embedded API version from a DLL
				IntPtr hModule = LoadLibraryEx(Filename, IntPtr.Zero, (uint)LoadLibraryFlags.LOAD_LIBRARY_AS_DATAFILE);
				if (hModule != IntPtr.Zero)
				{
					IntPtr hResInfo = FindResource(hModule, new IntPtr(ID_MODULE_API_VERSION_RESOURCE), new IntPtr((int)ResourceType.RT_RCDATA));
					if (hResInfo != IntPtr.Zero)
					{
						IntPtr hResGlobal = LoadResource(hModule, hResInfo);
						if (hResGlobal != IntPtr.Zero)
						{
							IntPtr pResData = LockResource(hResGlobal);
							if (pResData != IntPtr.Zero)
							{
								uint Length = SizeofResource(hModule, hResInfo);
								if (Length > 0)
								{
									var Str = Marshal.PtrToStringAnsi(pResData);
									Result = Int32.Parse(Str);
								}
							}
						}
					}
					FreeLibrary(hModule);
				}
			}
			catch (Exception Ex)
			{
				Log.TraceWarning("Failed to get DLL API version for {0}. Exception: {1}", Filename, Ex.Message);
			}

			return Result;
		}
	}

	class MacBuildHostPlatform : BuildHostPlatform
	{
		public override UnrealTargetPlatform Platform
		{
			get { return UnrealTargetPlatform.Mac; }
		}

		public override int GetDllApiVersion(string Filename)
		{
			// @TODO: Implement GetDllApiVersion for Mac
			return -1;
		}
	}

	class LinuxBuildHostPlatform : BuildHostPlatform
	{
		public override UnrealTargetPlatform Platform
		{
			get { return UnrealTargetPlatform.Linux; }
		}

		public override int GetDllApiVersion(string Filename)
		{
			// @TODO: Implement GetDllApiVersion for Linux
			return -1;
		}
	}
}
