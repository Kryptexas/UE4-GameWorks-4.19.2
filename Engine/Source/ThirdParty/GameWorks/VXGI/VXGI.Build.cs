// NVCHANGE_BEGIN: Add VXGI
using UnrealBuildTool;

public class VXGI : ModuleRules
{
	public VXGI(ReadOnlyTargetRules Target) : base(Target)
    {
		Type = ModuleType.External;
		
		PublicDefinitions.Add("WITH_GFSDK_VXGI=1");

		string VXGIDir = Target.UEThirdPartySourceDirectory + "GameWorks/VXGI";
		PublicIncludePaths.Add(VXGIDir + "/include");
		PublicLibraryPaths.Add(VXGIDir + "/lib");

		string ArchName = "<unknown>";

		if (Target.Platform == UnrealTargetPlatform.Win64)
			ArchName = "x64";
		else if (Target.Platform == UnrealTargetPlatform.Win32)
			ArchName = "x86";

		PublicAdditionalLibraries.Add("GFSDK_VXGI_" + ArchName + ".lib");
		PublicDelayLoadDLLs.Add("GFSDK_VXGI_" + ArchName + ".dll");
	}
}
// NVCHANGE_END: Add VXGI
