// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;
using System.Reflection;

//ProjectUtil.cs


public class BranchInfo
{

    public static TargetRules.TargetType[] MonolithicKinds = new TargetRules.TargetType[]
    {
        TargetRules.TargetType.Game,
        TargetRules.TargetType.Client,
        TargetRules.TargetType.Server,
    };
    public class BranchUProject
    {
        public string GameName;
        public string FilePath;
        public ProjectProperties Properties;

        public BranchUProject(UnrealBuildTool.UProjectInfo InfoEntry)
        {
            GameName = InfoEntry.GameName;

            //not sure what the heck this path is relative to
            FilePath = Path.GetFullPath(CommandUtils.CombinePaths(CommandUtils.CmdEnv.LocalRoot, "Engine", "Binaries", InfoEntry.FilePath));

            if (!CommandUtils.FileExists_NoExceptions(FilePath))
            {
                throw new AutomationException("Could not resolve relative path corrctly {0} -> {1} which doesn't exist.", InfoEntry.FilePath, FilePath);
            }

            Properties = ProjectUtils.GetProjectProperties(Path.GetFullPath(FilePath));



        }
        public BranchUProject()
        {
            GameName = "UE4";
            Properties = ProjectUtils.GetProjectProperties("");
            if (!Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                throw new AutomationException("Base UE4 project did not contain an editor target.");
            }
        }
        public TargetRules.GUBPProjectOptions Options(UnrealTargetPlatform HostPlatform)
        {
            var Options = new TargetRules.GUBPProjectOptions();
            if (Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                var Target = Properties.Targets[TargetRules.TargetType.Editor];
                Options = Target.Rules.GUBP_IncludeProjectInPromotedBuild_EditorTypeOnly(HostPlatform);
            }
            return Options;
        }
        public void Dump(List<UnrealTargetPlatform> InHostPlatforms)
        {
            CommandUtils.Log("    ShortName:    " + GameName);
            CommandUtils.Log("      FilePath          : " + FilePath);
            CommandUtils.Log("      bIsCodeBasedProject  : " + (Properties.bIsCodeBasedProject ? "YES" : "NO"));
            CommandUtils.Log("      bUsesSteam  : " + (Properties.bUsesSteam ? "YES" : "NO"));
            CommandUtils.Log("      bUsesSlate  : " + (Properties.bUsesSlate ? "YES" : "NO"));
            foreach (var HostPlatform in InHostPlatforms)
            {
                CommandUtils.Log("      For Host : " + HostPlatform.ToString());
                if (Properties.bIsCodeBasedProject)
                {
                    CommandUtils.Log("          Promoted  : " + (Options(HostPlatform).bIsPromotable ? "YES" : "NO"));
                    CommandUtils.Log("          SeparatePromotable  : " + (Options(HostPlatform).bSeparateGamePromotion ? "YES" : "NO"));
                    CommandUtils.Log("          Test With Shared  : " + (Options(HostPlatform).bTestWithShared ? "YES" : "NO"));
                }
                CommandUtils.Log("          Targets {0}:", Properties.Targets.Count);
                foreach (var ThisTarget in Properties.Targets)
                {
                    CommandUtils.Log("            TargetName          : " + ThisTarget.Value.TargetName);
                    CommandUtils.Log("              Type          : " + ThisTarget.Key.ToString());
                    CommandUtils.Log("              bUsesSteam  : " + (ThisTarget.Value.Rules.bUsesSteam ? "YES" : "NO"));
                    CommandUtils.Log("              bUsesSlate  : " + (ThisTarget.Value.Rules.bUsesSlate ? "YES" : "NO"));
                    if (Array.IndexOf(MonolithicKinds, ThisTarget.Key) >= 0)
                    {
                        var Plats = ThisTarget.Value.Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform);
                        foreach (var Plat in Plats)
                        {
                            var Configs = ThisTarget.Value.Rules.GUBP_GetConfigs_MonolithicOnly(HostPlatform, Plat);
                            foreach (var Config in Configs)
                            {
                                CommandUtils.Log("                Config  : " + Plat.ToString() + " " + Config.ToString());
                            }
                        }

                    }
                }
            }
            CommandUtils.Log("      Programs {0}:", Properties.Programs.Count);
            foreach (var ThisTarget in Properties.Programs)
            {
                CommandUtils.Log("        TargetName          : " + ThisTarget.TargetName);
                CommandUtils.Log("          Build With Editor  : " + (ThisTarget.Rules.GUBP_AlwaysBuildWithBaseEditor() ? "YES" : "NO"));
            }
        }
    };
    public BranchUProject BaseEngineProject = null;
    public List<BranchUProject> CodeProjects = new List<BranchUProject>();
    public List<BranchUProject> NonCodeProjects = new List<BranchUProject>();

    public BranchInfo(List<UnrealTargetPlatform> InHostPlatforms)
    {
        BaseEngineProject = new BranchUProject();

        var AllProjects = UnrealBuildTool.UProjectInfo.FilterGameProjects(false, null);

        foreach (var InfoEntry in AllProjects)
        {
            var UProject = new BranchUProject(InfoEntry);
            if (UProject.Properties.bIsCodeBasedProject)
            {
                CodeProjects.Add(UProject);
            }
            else
            {
                NonCodeProjects.Add(UProject);
            }
        }

        CommandUtils.Log("  Base Engine:");
        BaseEngineProject.Dump(InHostPlatforms);

        CommandUtils.Log("  {0} Code projects:", CodeProjects.Count);
        foreach (var Proj in CodeProjects)
        {
            Proj.Dump(InHostPlatforms);
        }
        CommandUtils.Log("  {0} Non-Code projects:", CodeProjects.Count);
        foreach (var Proj in NonCodeProjects)
        {
            Proj.Dump(InHostPlatforms);
        }
    }

    public BranchUProject FindGame(string GameName)
    {
        foreach (var Proj in CodeProjects)
        {
            if (Proj.GameName.Equals(GameName, StringComparison.InvariantCultureIgnoreCase))
            {
                return Proj;
            }
        }
        foreach (var Proj in NonCodeProjects)
        {
            if (Proj.GameName.Equals(GameName, StringComparison.InvariantCultureIgnoreCase))
            {
                return Proj;
            }
        }
        return null;
    }
};

class DumpBranch : BuildCommand
{

    public override void ExecuteBuild()
    {
        Log("************************* DumpBranch");

        var HostPlatforms = new List<UnrealTargetPlatform>();
        HostPlatforms.Add(UnrealTargetPlatform.Win64);
        HostPlatforms.Add(UnrealTargetPlatform.Mac);
        new BranchInfo(HostPlatforms);
 
    }


}



public class GUBP : BuildCommand
{
    public string StoreName = null;
    public int CL = 0;
    public bool bSignBuildProducts = false;
    public List<UnrealTargetPlatform> ActivePlatforms = null;
    public BranchInfo Branch = null;
    public bool bWide = true;
    public bool bOrthogonalizeEditorPlatforms = false;

    public abstract class GUBPNodeAdder
    {
        public virtual void AddNodes(GUBP bp, UnrealTargetPlatform InHostPlatform)
        {
        }
    }

    private static List<GUBPNodeAdder> Adders;
    private void AddCustomNodes(UnrealTargetPlatform InHostPlatform)
    {
        if (Adders == null)
        {
            Adders = new List<GUBPNodeAdder>();
            Assembly[] LoadedAssemblies = AppDomain.CurrentDomain.GetAssemblies();
            foreach (var Dll in LoadedAssemblies)
            {
                Type[] AllTypes = Dll.GetTypes();
                foreach (var PotentialConfigType in AllTypes)
                {
                    if (PotentialConfigType != typeof(GUBPNodeAdder) && typeof(GUBPNodeAdder).IsAssignableFrom(PotentialConfigType))
                    {
                        GUBPNodeAdder Config = Activator.CreateInstance(PotentialConfigType) as GUBPNodeAdder;
                        if (Config != null)
                        {
                            Adders.Add(Config);
                        }
                    }
                }
            }
        }
        foreach(var Adder in Adders)
        {
            Adder.AddNodes(this, InHostPlatform);
        }
    }

    public abstract class GUBPNode
    {
        public List<string> FullNamesOfDependencies = new List<string>();
        public List<string> FullNamesOfPseudosependencies = new List<string>(); //these are really only used for sorting. We want the editor to fail before the monolithics. Think of it as "can't possibly be useful without".
        public List<string> BuildProducts = null;
        public List<string> AllDependencyBuildProducts = null;

        public virtual string GetFullName()
        {
            throw new AutomationException("Unimplemented GetFullName.");
        }
        public virtual string GameNameIfAnyForTempStorage()
        {
            return "";
        }
        public virtual string RootIfAnyForTempStorage()
        {
            return "";
        }
        public virtual void DoBuild(GUBP bp)
        {
            BuildProducts = new List<string>();
            SaveRecordOfSuccessAndAddToBuildProducts();
        }
        public virtual void DoFakeBuild(GUBP bp) // this is used to more rapidly test a build system, it does nothing but save a record of success as a build product
        {
            BuildProducts = new List<string>();
            SaveRecordOfSuccessAndAddToBuildProducts();
        }
        public virtual bool IsSticky()
        {
            return false;
        }
        public virtual bool SendSuccessEmail()
        {
            return false;
        }
        public virtual bool RunInEC()
        {
            return true;
        }
        public virtual bool IsAggregate()
        {
            return false;
        }
        public virtual int AgentMemoryRequirement()
        {
            return 0;
        }
        public virtual int TimeoutInMinutes()
        {
            return 90;
        }
        public virtual float Priority()
        {
            return 100.0f;
        }
        public virtual bool TriggerNode()
        {
            return false;
        }
        public virtual void SetAsExplicitTrigger()
        {

        }
        public virtual string ECAgentString()
        {
            return "";
        }
        public virtual string ECProcedure()
        {
            return String.Format("GUBP_UAT_Node{0}", IsSticky() ? "" : "_Parallel");
        }
        public virtual string ECProcedureParams()
        {
            return String.Format("--actualParameter Sticky={0}", IsSticky() ? 1 : 0);
        }
        public virtual string FailureEMails(GUBP bp, string Branch)
        {
            return "";
        }
        public static string MergeSpaceStrings(params string[] EmailLists)
        {
            var Emails = new List<string>();
            foreach (var EmailList in EmailLists)
            {
                if (!String.IsNullOrEmpty(EmailList))
                {
                    List<string> Parts = new List<string>(EmailList.Split(' '));
                    foreach (var Email in Parts)
                    {
                        if (!string.IsNullOrEmpty(Email) && !Emails.Contains(Email))
                        {
                            Emails.Add(Email);
                        }
                    }
                }
            }
            string Result = "";
            foreach (var Email in Emails)
            {
                if (Result != "")
                {
                    Result += " ";
                }
                Result += Email;
            }
            return Result;
        }

        public void SaveRecordOfSuccessAndAddToBuildProducts(string Contents = "Just a record of success.")
        {
            string RecordOfSuccess = CombinePaths(CommandUtils.CmdEnv.LocalRoot, "Engine", "Saved", "Logs", GetFullName() + "_Success.log");
            CommandUtils.CreateDirectory(Path.GetDirectoryName(RecordOfSuccess));
            CommandUtils.WriteAllText(RecordOfSuccess, Contents);
            AddBuildProduct(RecordOfSuccess);
        }
        public void AddDependency(string Node)
        {
            if (!FullNamesOfDependencies.Contains(Node))
            {
                FullNamesOfDependencies.Add(Node);
            }
        }
        public void AddPseudodependency(string Node)
        {
            if (!FullNamesOfPseudosependencies.Contains(Node))
            {
                FullNamesOfPseudosependencies.Add(Node);
            }
        }
        public void RemovePseudodependency(string Node)
        {
            if (FullNamesOfPseudosependencies.Contains(Node))
            {
                FullNamesOfPseudosependencies.Remove(Node);
            }
        }
        public void AddBuildProduct(string Filename)
        {
            if (!CommandUtils.FileExists_NoExceptions(true, Filename))
            {
                throw new AutomationException("Cannot add build product {0} because it does not exist.", Filename);
            }
            FileInfo Info = new FileInfo(Filename);
            if (!BuildProducts.Contains(Info.FullName))
            {
                BuildProducts.Add(Info.FullName);
            }
        }
        public void AddDependentBuildProduct(string Filename)
        {
            if (!CommandUtils.FileExists_NoExceptions(true, Filename))
            {
                throw new AutomationException("Cannot add build product {0} because it does not exist.", Filename);
            }
            FileInfo Info = new FileInfo(Filename);
            if (!AllDependencyBuildProducts.Contains(Info.FullName))
            {
                AllDependencyBuildProducts.Add(Info.FullName);
            }
        }
        public void RemoveOveralppingBuildProducts()
        {
            foreach (var ToRemove in AllDependencyBuildProducts)
            {
                BuildProducts.Remove(ToRemove);
            }
        }

    }
    public class VersionFilesNode : GUBPNode
    {
        public static string StaticGetFullName()
        {
            return "VersionFiles";
        }
        public override string GetFullName()
        {
            return StaticGetFullName();
        }

        public override void DoBuild(GUBP bp)
        {
            var UE4Build = new UE4Build(bp);
            BuildProducts = UE4Build.UpdateVersionFiles();
        }
        public override bool IsSticky()
        {
            return true;
        }
    }

    public class HostPlatformNode : GUBPNode
    {
        protected UnrealTargetPlatform HostPlatform;
        public HostPlatformNode(UnrealTargetPlatform InHostPlatform)
        {
            HostPlatform = InHostPlatform;
        }
        public static string StaticGetHostPlatformSuffix(UnrealTargetPlatform InHostPlatform)
        {
            if (InHostPlatform == UnrealTargetPlatform.Mac)
            {
                return "_OnMac";
            }
            return "";
        }
        public override string ECAgentString()
        {
            return HostPlatform == UnrealTargetPlatform.Mac ? "Mac" : "";
        }
        public virtual string GetHostPlatformSuffix()
        {
            return StaticGetHostPlatformSuffix(HostPlatform);
        }
        public UnrealTargetPlatform GetAltHostPlatform()
        {
            return GUBP.GetAltHostPlatform(HostPlatform);
        }
    }

    public class CompileNode : HostPlatformNode
    {
        public CompileNode(UnrealTargetPlatform InHostPlatform)
            : base(InHostPlatform)
        {
            AddDependency(VersionFilesNode.StaticGetFullName());
        }
        public virtual UE4Build.BuildAgenda GetAgenda(GUBP bp)
        {
            return null;
        }
        public virtual void PostBuild(GUBP bp, UE4Build UE4Build)
        {
        }
        public virtual bool DeleteBuildProducts()
        {
            return false;
        }
        public override void DoBuild(GUBP bp)
        {
            BuildProducts = new List<string>();
            var UE4Build = new UE4Build(bp);
            UE4Build.BuildAgenda Agenda = GetAgenda(bp);
            if (Agenda != null)
            {
                Agenda.DoRetries = false; // these would delete build products
                UE4Build.Build(Agenda, InDeleteBuildProducts: DeleteBuildProducts(), InUpdateVersionFiles: false, InForceUnity: true);
                PostBuild(bp, UE4Build);

                UE4Build.CheckBuildProducts(UE4Build.BuildProductFiles);
                foreach (var Product in UE4Build.BuildProductFiles)
                {
                    AddBuildProduct(Product);
                }

                RemoveOveralppingBuildProducts();
                if (bp.bSignBuildProducts)
                {
                    // Sign everything we built
                    CodeSign.SignMultipleIfEXEOrDLL(bp, BuildProducts);
                }
            }
            else
            {
                SaveRecordOfSuccessAndAddToBuildProducts("Nothing to actually compile");
            }
        }
    }
    public class RootEditorNode : CompileNode
    {
        public RootEditorNode(UnrealTargetPlatform InHostPlatform)
            : base(InHostPlatform)
        {
        }
        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform)
        {
            return "RootEditor" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform);
        }
        public override bool IsSticky()
        {
            return true;
        }
        public override bool DeleteBuildProducts()
        {
            return true;
        }
        public override UE4Build.BuildAgenda GetAgenda(GUBP bp)
        {
            var Agenda = new UE4Build.BuildAgenda();
            //if (HostPlatform == UnrealTargetPlatform.Mac)
            {
                Agenda.DotNetProjects.AddRange(
                    new string[] 
			        {
				        @"Engine\Source\Programs\DotNETCommon\DotNETUtilities\DotNETUtilities.csproj",
                        @"Engine\Source\Programs\RPCUtility\RPCUtility.csproj",
			        }
                    );
            }
            string AddArgs = "";
            if (bp.bOrthogonalizeEditorPlatforms)
            {
                AddArgs = "-skipnonhostplatforms";
            }
            Agenda.AddTargets(new string[] { "UnrealHeaderTool" }, HostPlatform, UnrealTargetConfiguration.Development);
            Agenda.AddTargets(
                new string[] { bp.Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor].TargetName },
                HostPlatform, UnrealTargetConfiguration.Development, InAddArgs: AddArgs);
            foreach (var ProgramTarget in bp.Branch.BaseEngineProject.Properties.Programs)
            {
                if (ProgramTarget.Rules.GUBP_AlwaysBuildWithBaseEditor() && ProgramTarget.Rules.SupportsPlatform(HostPlatform))
                {
                    Agenda.AddTargets(new string[] { ProgramTarget.TargetName }, HostPlatform, UnrealTargetConfiguration.Development, InAddArgs: AddArgs);
                }
            }

            return Agenda;
        }
        public override void PostBuild(GUBP bp, UE4Build UE4Build)
        {
            UE4Build.AddUATFilesToBuildProducts();
            UE4Build.AddUBTFilesToBuildProducts();
        }
    }

    public class ToolsNode : CompileNode
    {
        public ToolsNode(UnrealTargetPlatform InHostPlatform)
            : base(InHostPlatform)
        {
            AddPseudodependency(RootEditorNode.StaticGetFullName(HostPlatform));
        }
        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform)
        {
            return "Tools" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform);
        }
        public override bool IsSticky()
        {
            return false;
        }
        public override float Priority()
        {
            return base.Priority() - 1;
        }
        public override bool DeleteBuildProducts()
        {
            return true;
        }
        public override UE4Build.BuildAgenda GetAgenda(GUBP bp)
        {
            var Agenda = new UE4Build.BuildAgenda();

            Agenda.DotNetProjects.AddRange(
                new string[] 
			    {
				    @"Engine\Source\Programs\Distill\Distill.csproj",		       
                    @"Engine\Source\Programs\UnrealControls\UnrealControls.csproj",
			    }
                );
            if (HostPlatform == UnrealTargetPlatform.Mac)
            {
                Agenda.DotNetProjects.AddRange(
                    new string[] 
					{
						// UnrealSwarm
						@"Engine/Source/Programs/UnrealSwarm/SwarmCoordinatorInterface/SwarmCoordinatorInterface_Mono.csproj",
						@"Engine/Source/Programs/UnrealSwarm/Agent/Agent_Mono.csproj",
						@"Engine/Source/Programs/UnrealControls/UnrealControls_Mono.csproj",
						@"Engine/Source/Programs/UnrealSwarm/SwarmCoordinator/SwarmCoordinator_Mono.csproj",
						@"Engine/Source/Programs/UnrealSwarm/AgentInterface/AgentInterface_Mono.csproj",
						// AutomationTool
						@"Engine/Source/Programs/AutomationTool/AutomationTool_Mono.csproj",
					}
                    );
            }
            else
            {
                Agenda.DotNetSolutions.AddRange(
                    new string[] 
			        {
				        @"Engine\Source\Programs\UnrealDocTool\UnrealDocTool\UnrealDocTool.sln",
				        @"Engine\Source\Programs\NetworkProfiler\NetworkProfiler.sln",   
			        }
                    );

                Agenda.SwarmProject = @"Engine\Source\Programs\UnrealSwarm\UnrealSwarm.sln";

                Agenda.ExtraDotNetFiles.AddRange(
                    new string[] 
			        {
				        "Interop.IWshRuntimeLibrary",
				        "UnrealMarkdown",
				        "CommonUnrealMarkdown",
			        }
                    );
                Agenda.DotNetProjects.AddRange(new string[] 
				{
    				@"Engine\Source\Programs\CrashReporter\RegisterPII\RegisterPII.csproj",
					@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportCommon\CrashReportCommon.csproj",
					@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportReceiver\CrashReportReceiver.csproj",
					@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportProcess\CrashReportProcess.csproj",
				});
            }



            foreach (var ProgramTarget in bp.Branch.BaseEngineProject.Properties.Programs)
            {
                if (ProgramTarget.Rules.GUBP_AlwaysBuildWithTools() && ProgramTarget.Rules.SupportsPlatform(HostPlatform))
                {
                    Agenda.AddTargets(new string[] { ProgramTarget.TargetName }, HostPlatform, UnrealTargetConfiguration.Development);
                }
            }

            return Agenda;
        }
    }

    public class EditorPlatformNode : CompileNode
    {
        UnrealTargetPlatform EditorPlatform;

        public EditorPlatformNode(UnrealTargetPlatform InHostPlatform, UnrealTargetPlatform Plat)
            : base(InHostPlatform)
        {
            EditorPlatform = Plat;
            AddDependency(RootEditorNode.StaticGetFullName(InHostPlatform));
        }

        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform, UnrealTargetPlatform Plat)
        {
            return Plat.ToString() + "_EditorPlatform" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform, EditorPlatform);
        }

        public override float Priority()
        {
            return 101.0f;
        }
        public override bool IsSticky()
        {
            return true;
        }
        public override UE4Build.BuildAgenda GetAgenda(GUBP bp)
        {
            if (EditorPlatform != UnrealTargetPlatform.IOS && !bp.bOrthogonalizeEditorPlatforms)
            {
                return null;
            }
            var Agenda = new UE4Build.BuildAgenda();

            if (EditorPlatform == UnrealTargetPlatform.IOS)
            {
                //@todo, this should be generic platform stuff
                Agenda.IOSDotNetProjects.AddRange(
                        new string[]
                    {
                        @"Engine\Source\Programs\IOS\iPhonePackager\iPhonePackager.csproj",
                        @"Engine\Source\Programs\IOS\MobileDeviceInterface\MobileDeviceInterface.csproj",
                        @"Engine\Source\Programs\IOS\DeploymentInterface\DeploymentInterface.csproj",
                    }
                        );
            }
            if (bp.bOrthogonalizeEditorPlatforms)
            {
                Agenda.AddTargets(
                    new string[] { bp.Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor].TargetName },
                    HostPlatform, UnrealTargetConfiguration.Development, InAddArgs: "-nobuilduht -skipactionhistory -onlyplatformspecificfor=" + EditorPlatform.ToString());
                foreach (var ProgramTarget in bp.Branch.BaseEngineProject.Properties.Programs)
                {
                    if (ProgramTarget.Rules.GUBP_AlwaysBuildWithBaseEditor() && ProgramTarget.Rules.SupportsPlatform(HostPlatform) && ProgramTarget.Rules.GUBP_NeedsPlatformSpecificDLLs())
                    {
                        Agenda.AddTargets(new string[] { ProgramTarget.TargetName }, HostPlatform, UnrealTargetConfiguration.Development, InAddArgs: "-nobuilduht -skipactionhistory -onlyplatformspecificfor=" + EditorPlatform.ToString());
                    }
                }
            }
            return Agenda;
        }

        public override string FailureEMails(GUBP bp, string Branch)
        {
            return MergeSpaceStrings(base.FailureEMails(bp, Branch), 
                Platform.Platforms[EditorPlatform].GUBP_GetPlatformFailureEMails(Branch));
		}
    }

    public class EditorGameNode : CompileNode
    {
        BranchInfo.BranchUProject GameProj;

        public EditorGameNode(GUBP bp, UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj)
            : base(InHostPlatform)
        {
            GameProj = InGameProj;
            AddDependency(RootEditorNode.StaticGetFullName(InHostPlatform));
        }

        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj)
        {
            return InGameProj.GameName + "_EditorGame" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform, GameProj);
        }
        public override string GameNameIfAnyForTempStorage()
        {
            return GameProj.GameName;
        }

        public override bool IsSticky()
        {
            return true;
        }
        public override UE4Build.BuildAgenda GetAgenda(GUBP bp)
        {
            var Agenda = new UE4Build.BuildAgenda();

            Agenda.AddTargets(
                new string[] { GameProj.Properties.Targets[TargetRules.TargetType.Editor].TargetName },
                HostPlatform, UnrealTargetConfiguration.Development, GameProj.FilePath, InAddArgs: "-nobuilduht -skipactionhistory -skipnonhostplatforms");

            return Agenda;
        }
        public override string FailureEMails(GUBP bp, string Branch)
        {
            return MergeSpaceStrings(base.FailureEMails(bp, Branch),
               GameProj.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetGameFailureEMails_EditorTypeOnly(Branch));
        }
    }

    public class GamePlatformMonolithicsNode : CompileNode
    {
        BranchInfo.BranchUProject GameProj;
        UnrealTargetPlatform TargetPlatform;

        public GamePlatformMonolithicsNode(GUBP bp, UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, UnrealTargetPlatform InTargetPlatform)
            : base(InHostPlatform)
        {
            GameProj = InGameProj;
            TargetPlatform = InTargetPlatform;
            if (InGameProj.GameName != bp.Branch.BaseEngineProject.GameName && GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                AddPseudodependency(EditorGameNode.StaticGetFullName(HostPlatform, GameProj));
                AddPseudodependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, TargetPlatform));
            }
            else
            {
                AddPseudodependency(RootEditorNode.StaticGetFullName(HostPlatform));
                if (TargetPlatform != HostPlatform)
                {
                    AddPseudodependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, HostPlatform));
                }
            }
            if (InGameProj.Options(HostPlatform).bTestWithShared)  /// compiling templates is only for testing purposes
            {
                AddPseudodependency(WaitForTestShared.StaticGetFullName());
            }
        }

        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, UnrealTargetPlatform InTargetPlatform)
        {
            return InGameProj.GameName + "_" + InTargetPlatform + "_Monolithics" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform, GameProj, TargetPlatform);
        }
        public override string GameNameIfAnyForTempStorage()
        {
            return GameProj.GameName;
        }
        public override bool DeleteBuildProducts()
        {
            return true;
        }

        public override UE4Build.BuildAgenda GetAgenda(GUBP bp)
        {
            if (!bp.ActivePlatforms.Contains(TargetPlatform))
            {
                throw new AutomationException("{0} is not a supported platform for {1}", TargetPlatform.ToString(), GetFullName());
            }
            var Agenda = new UE4Build.BuildAgenda();

            foreach (var Kind in BranchInfo.MonolithicKinds)
            {
                if (GameProj.Properties.Targets.ContainsKey(Kind))
                {
                    var Target = GameProj.Properties.Targets[Kind];
                    var Platforms = Target.Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform);
                    if (Platforms.Contains(TargetPlatform) && Target.Rules.SupportsPlatform(TargetPlatform))
                    {
                        var Configs = Target.Rules.GUBP_GetConfigs_MonolithicOnly(HostPlatform, TargetPlatform);
                        foreach (var Config in Configs)
                        {
                            Agenda.AddTargets(new string[] { Target.TargetName }, TargetPlatform, Config, GameProj.FilePath);
                        }

                    }
                }
            }

            return Agenda;
        }
        public override string FailureEMails(GUBP bp, string Branch)
        {
            return MergeSpaceStrings(base.FailureEMails(bp, Branch),
               GameProj.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetGameFailureEMails_EditorTypeOnly(Branch),
               Platform.Platforms[TargetPlatform].GUBP_GetPlatformFailureEMails(Branch));
        }
    }

    public class AggregateNode : GUBPNode
    {
        public AggregateNode()
        {
        }
        public override bool RunInEC()
        {
            return false;
        }
        public override bool IsAggregate()
        {
            return true;
        }
        public override void DoBuild(GUBP bp)
        {
            BuildProducts = new List<string>();
        }
        public override void DoFakeBuild(GUBP bp) // this is used to more rapidly test a build system, it does nothing but save a record of success as a build product
        {
            BuildProducts = new List<string>();
        }
    }

    public class HostPlatformAggregateNode : AggregateNode
    {
        protected UnrealTargetPlatform HostPlatform;
        public HostPlatformAggregateNode(UnrealTargetPlatform InHostPlatform)
        {
            HostPlatform = InHostPlatform;
        }
        public static string StaticGetHostPlatformSuffix(UnrealTargetPlatform InHostPlatform)
        {
            return HostPlatformNode.StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public virtual string GetHostPlatformSuffix()
        {
            return StaticGetHostPlatformSuffix(HostPlatform);
        }
        public UnrealTargetPlatform GetAltHostPlatform()
        {
            return GUBP.GetAltHostPlatform(HostPlatform);
        }
    }

    public class EditorAndToolsNode : HostPlatformAggregateNode
    {
        public EditorAndToolsNode(UnrealTargetPlatform InHostPlatform)
            : base(InHostPlatform)
        {
            AddDependency(RootEditorNode.StaticGetFullName(HostPlatform));
            AddDependency(ToolsNode.StaticGetFullName(HostPlatform));
        }
        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform)
        {
            return "BaseEditorAndTools" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform);
        }
    }


    public class AggregatePromotableNode : AggregateNode
    {
        protected List<UnrealTargetPlatform> HostPlatforms;
        string PromotionLabelPrefix;

        public AggregatePromotableNode(List<UnrealTargetPlatform> InHostPlatforms, string InPromotionLabelPrefix)
        {
            HostPlatforms = InHostPlatforms;
            foreach (var HostPlatform in HostPlatforms)
            {
                AddDependency(EditorAndToolsNode.StaticGetFullName(HostPlatform));
            }
            PromotionLabelPrefix = InPromotionLabelPrefix; 
        }
        public static string StaticGetFullName(string InPromotionLabelPrefix)
        {
            return InPromotionLabelPrefix + "_Promotable_Aggregate";
        }
        public override string GetFullName()
        {
            return StaticGetFullName(PromotionLabelPrefix);
        }
    }

    public class GameAggregatePromotableNode : AggregatePromotableNode
    {
        BranchInfo.BranchUProject GameProj;

        public GameAggregatePromotableNode(GUBP bp, List<UnrealTargetPlatform> InHostPlatforms, BranchInfo.BranchUProject InGameProj)
            : base(InHostPlatforms, InGameProj.GameName)
        {
            GameProj = InGameProj;

            foreach (var HostPlatform in HostPlatforms)
            {
                if (InGameProj.GameName != bp.Branch.BaseEngineProject.GameName && GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
                {
                    AddDependency(EditorGameNode.StaticGetFullName(HostPlatform, GameProj));
                }

                // add all of the platforms I use
                {
                    var Platforms = bp.GetMonolithicPlatformsForUProject(HostPlatform, InGameProj, false);
                    foreach (var Plat in Platforms)
                    {
                        AddDependency(EditorPlatformNode.StaticGetFullName(HostPlatform, Plat));
                    }
                }
                {
                    var Platforms = bp.GetMonolithicPlatformsForUProject(HostPlatform, InGameProj, true);
                    foreach (var Plat in Platforms)
                    {
                        AddDependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, GameProj, Plat));
                    }
                }
            }
        }

        public static string StaticGetFullName(BranchInfo.BranchUProject InGameProj)
        {
            return AggregatePromotableNode.StaticGetFullName(InGameProj.GameName);
        }

        public override string GameNameIfAnyForTempStorage()
        {
            return GameProj.GameName;
        }
    }

    public class SharedAggregatePromotableNode : AggregatePromotableNode
    {

        public SharedAggregatePromotableNode(GUBP bp, List<UnrealTargetPlatform> InHostPlatforms)
            : base(InHostPlatforms, "Shared")
        {
            foreach (var HostPlatform in HostPlatforms)
            {
                {
                    var Options = bp.Branch.BaseEngineProject.Options(HostPlatform);
                    if (Options.bIsPromotable && !Options.bSeparateGamePromotion)
                    {
                        AddDependency(GameAggregatePromotableNode.StaticGetFullName(bp.Branch.BaseEngineProject));
                    }
                }
                foreach (var CodeProj in bp.Branch.CodeProjects)
                {
                    var Options = CodeProj.Options(HostPlatform);
                    if (!Options.bSeparateGamePromotion)
                    {
                        if (Options.bIsPromotable)
                        {
                            AddDependency(GameAggregatePromotableNode.StaticGetFullName(CodeProj));
                        }
                        else if (Options.bTestWithShared)
                        {
                            AddDependency(EditorGameNode.StaticGetFullName(HostPlatform, CodeProj)); // if we are just testing, we will still include the editor stuff
                        }
                    }
                }
            }
        }

        public static string StaticGetFullName()
        {
            return AggregatePromotableNode.StaticGetFullName("Shared");
        }
    }

    public class WaitForUserInput : GUBPNode
    {
        protected bool bSticky;
        public WaitForUserInput()
        {
            bSticky = false;
        }

        public override void DoBuild(GUBP bp)
        {
            BuildProducts = new List<string>();
            SaveRecordOfSuccessAndAddToBuildProducts();
        }
        public override bool TriggerNode()
        {
            return true;
        }
        public override void SetAsExplicitTrigger()
        {
            bSticky = true;
        }
        public override bool IsSticky()
        {
            return bSticky;
        }
        public override string ECProcedure()
        {
            if (bSticky)
            {
                return base.ECProcedure(); // after this user hits the trigger, we want to run this as an ordinary node
            }
            return String.Format("GUBP_UAT_Trigger"); //here we set up to ask the user
        }

    }

    public class WaitForPromotionUserInput : WaitForUserInput
    {
        string PromotionLabelPrefix;
        protected bool bLabelPromoted; // true if this is the promoted version

        public WaitForPromotionUserInput(string InPromotionLabelPrefix, bool bInLabelPromoted)
        {
            PromotionLabelPrefix = InPromotionLabelPrefix;
            bLabelPromoted = bInLabelPromoted;
            if (bLabelPromoted)
            {
                AddDependency(LabelPromotableNode.StaticGetFullName(PromotionLabelPrefix, false));
            }
            else
            {
                AddDependency(AggregatePromotableNode.StaticGetFullName(PromotionLabelPrefix));
            }
        }
        public static string StaticGetFullName(string InPromotionLabelPrefix, bool bInLabelPromoted)
        {
            return InPromotionLabelPrefix + (bInLabelPromoted ? "_WaitForPromotion" : "_WaitForPromotable");
        }
        public override string GetFullName()
        {
            return StaticGetFullName(PromotionLabelPrefix, bLabelPromoted);
        }
    }

    public class WaitForGamePromotionUserInput : WaitForPromotionUserInput
    {
        BranchInfo.BranchUProject GameProj;

        public WaitForGamePromotionUserInput(GUBP bp, BranchInfo.BranchUProject InGameProj, bool bInLabelPromoted)
            : base(InGameProj.GameName, bInLabelPromoted)
        {
            GameProj = InGameProj;
        }
        public static string StaticGetFullName(BranchInfo.BranchUProject InGameProj, bool bInLabelPromoted)
        {
            return WaitForPromotionUserInput.StaticGetFullName(InGameProj.GameName, bInLabelPromoted);
        }
        public override string GameNameIfAnyForTempStorage()
        {
            return GameProj.GameName;
        }
        public override string FailureEMails(GUBP bp, string Branch)
        {
            return MergeSpaceStrings(base.FailureEMails(bp, Branch),
               GameProj.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetPromotionEMails_EditorTypeOnly(Branch));
        }
    }

    public class WaitForSharedPromotionUserInput : WaitForPromotionUserInput
    {

        public WaitForSharedPromotionUserInput(GUBP bp, bool bInLabelPromoted)
            : base("Shared", bInLabelPromoted)
        {
        }

        public static string StaticGetFullName(bool bInLabelPromoted)
        {
            return WaitForPromotionUserInput.StaticGetFullName("Shared", bInLabelPromoted);
        }
    }

    public class LabelPromotableNode : GUBPNode
    {
        string PromotionLabelPrefix;
        protected bool bLabelPromoted; // true if this is the promoted version

        public LabelPromotableNode(string InPromotionLabelPrefix, bool bInLabelPromoted)
        {
             PromotionLabelPrefix = InPromotionLabelPrefix;
             bLabelPromoted = bInLabelPromoted;
             AddDependency(WaitForPromotionUserInput.StaticGetFullName(PromotionLabelPrefix, bLabelPromoted));
        }
        string LabelName(bool bLocalLabelPromoted)
        {
            string LabelPrefix = PromotionLabelPrefix;
            string CompleteLabelPrefix = (bLocalLabelPromoted ? "Promoted-" : "Promotable-") + LabelPrefix;
            if (LabelPrefix == "Shared" && bLocalLabelPromoted)
            {
                // shared promotion has a shorter name
                CompleteLabelPrefix = "Promoted";
            }
            if (LabelPrefix == "Shared" && !bLocalLabelPromoted)
            {
                //shared promotable has a shorter name
                CompleteLabelPrefix = "Promotable";
            }
            // for now we give these labels funky names to prevent QA from looking at them unless working in the Releases branch
            if(!P4Env.BuildRootP4.Contains("Releases"))
            {
                CompleteLabelPrefix = "TEST-GUBP-" + CompleteLabelPrefix;
            }            
            return CompleteLabelPrefix;
        }

        public override bool IsSticky()
        {
            return true;
        }
        public override bool SendSuccessEmail()
        {
            return true;
        }


        public override void DoBuild(GUBP bp)
        {
            BuildProducts = new List<string>();

            if (P4Enabled)
            {
                if (AllDependencyBuildProducts.Count == 0)
                {
                    throw new AutomationException("{0} has no build products", GetFullName());

                }

                if (bLabelPromoted)
                {
                    MakeDownstreamLabelFromLabel(LabelName(true), LabelName(false));
                }
                else
                {
                    int WorkingCL = CreateChange(P4Env.Client, String.Format("GUBP Node {0} built from changelist {1}", GetFullName(), bp.CL));
                    Log("Build from {0}    Working in {1}", bp.CL, WorkingCL);

                    var ProductsToSubmit = new List<String>();

                    foreach (var Product in AllDependencyBuildProducts)
                    {
                        // hacks to keep certain things out of P4
                        if (
                            !Product.EndsWith("version.h", StringComparison.InvariantCultureIgnoreCase) && 
                            !Product.EndsWith("version.cpp", StringComparison.InvariantCultureIgnoreCase) &&
							!Product.Replace('\\', '/').EndsWith("DotNetCommon/MetaData.cs", StringComparison.InvariantCultureIgnoreCase) &&
                            !Product.EndsWith("_Success.log", StringComparison.InvariantCultureIgnoreCase)
                            )
                        {
                            ProductsToSubmit.Add(Product);
                        }
                    }

                    // Open files for add or edit
                    UE4Build.AddBuildProductsToChangelist(WorkingCL, ProductsToSubmit);

                    // Check everything in!
                    int SubmittedCL;
                    Submit(WorkingCL, out SubmittedCL, true, true);

                    // Label it       
                    MakeDownstreamLabel(LabelName(false), null);
                }
            }
            SaveRecordOfSuccessAndAddToBuildProducts();
        }
        public static string StaticGetFullName(string InPromotionLabelPrefix, bool bInLabelPromoted)
        {
            return InPromotionLabelPrefix + (bInLabelPromoted ? "_LabelPromoted" : "_LabelPromotable");
        }
        public override string GetFullName()
        {
            return StaticGetFullName(PromotionLabelPrefix, bLabelPromoted);
        }
    }

    public class GameLabelPromotableNode : LabelPromotableNode
    {
        BranchInfo.BranchUProject GameProj;

        public GameLabelPromotableNode(GUBP bp, BranchInfo.BranchUProject InGameProj, bool bInLabelPromoted)
            : base(InGameProj.GameName, bInLabelPromoted)
        {
            GameProj = InGameProj;
        }

        public static string StaticGetFullName(BranchInfo.BranchUProject InGameProj, bool bInLabelPromoted)
        {
            return LabelPromotableNode.StaticGetFullName(InGameProj.GameName, bInLabelPromoted);
        }
        public override string GameNameIfAnyForTempStorage()
        {
            return GameProj.GameName;
        }
        public override string FailureEMails(GUBP bp, string Branch)
        {
            return MergeSpaceStrings(base.FailureEMails(bp, Branch),
               GameProj.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetPromotionEMails_EditorTypeOnly(Branch));

        }
    }

    public class SharedLabelPromotableNode : LabelPromotableNode
    {
        public SharedLabelPromotableNode(GUBP bp, bool bInLabelPromoted)
            : base("Shared", bInLabelPromoted)
        {
        }

        public static string StaticGetFullName(bool bInLabelPromoted)
        {
            return LabelPromotableNode.StaticGetFullName("Shared", bInLabelPromoted);
        }
        public override string FailureEMails(GUBP bp, string Branch)
        {
            return MergeSpaceStrings(base.FailureEMails(bp, Branch),
               bp.Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetPromotionEMails_EditorTypeOnly(Branch));

        }
    }

    public class WaitForTestShared : GUBP.WaitForUserInput
    {
        public WaitForTestShared(GUBP bp)
        {
            // delay this until the root editor succeeds
            AddPseudodependency(GUBP.SharedLabelPromotableNode.StaticGetFullName(false));
        }
        public static string StaticGetFullName()
        {
            return "Shared_WaitForTesting";
        }
        public override string GetFullName()
        {
            return StaticGetFullName();
        }
        public override string FailureEMails(GUBP bp, string Branch)
        {
            return MergeSpaceStrings(base.FailureEMails(bp, Branch),
               bp.Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetPromotionEMails_EditorTypeOnly(Branch));

        }
    }

    public class CookNode : HostPlatformNode
    {
        BranchInfo.BranchUProject GameProj;
        UnrealTargetPlatform TargetPlatform;
        string CookPlatform;
        bool bIsMassive;

        public CookNode(GUBP bp, UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, UnrealTargetPlatform InTargetPlatform, string InCookPlatform)
            : base(InHostPlatform)
        {
            GameProj = InGameProj;
            TargetPlatform = InTargetPlatform;
            CookPlatform = InCookPlatform;
            bIsMassive = false;
            AddDependency(EditorAndToolsNode.StaticGetFullName(HostPlatform));
            if (TargetPlatform != HostPlatform && TargetPlatform != GUBP.GetAltHostPlatform(HostPlatform))
            {
                AddDependency(EditorPlatformNode.StaticGetFullName(HostPlatform, TargetPlatform));
            }

            // is this the "base game" or a non code project?
            if (InGameProj.GameName != bp.Branch.BaseEngineProject.GameName && GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                var Options = InGameProj.Options(HostPlatform);
                bIsMassive = Options.bIsMassive;

                AddDependency(EditorGameNode.StaticGetFullName(HostPlatform, GameProj));
                if (!bp.bWide || bIsMassive)
                {
                    // add an arc to prevent cooks from running until promotable is labeled
                    if (Options.bIsPromotable)
                    {
                        if (Options.bSeparateGamePromotion)
                        {
                            AddPseudodependency(GameLabelPromotableNode.StaticGetFullName(GameProj, false));
                        }
                        else
                        {
                            AddPseudodependency(WaitForTestShared.StaticGetFullName());
                        }
                    }
                }
                AddPseudodependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, GameProj, TargetPlatform));
            }
            else
            {
                if (!bp.bWide)
                {
                    // add an arc to prevent cooks from running until promotable is labeled
                    AddPseudodependency(WaitForTestShared.StaticGetFullName());
                }
                AddPseudodependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, TargetPlatform));
            }
        }
        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, string InCookPlatform)
        {
            return InGameProj.GameName + "_" + InCookPlatform + "_Cook" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform, GameProj, CookPlatform);
        }
        public override string GameNameIfAnyForTempStorage()
        {
            return GameProj.GameName;
        }
        public override float Priority()
        {
            return 10.0f;
        }
        public override int AgentMemoryRequirement()
        {
            return bIsMassive ? 32 : 0;
        }
        public override int TimeoutInMinutes()
        {
            return bIsMassive ? 240 : 90;
        }

        public override string RootIfAnyForTempStorage()
        {
            return CombinePaths(Path.GetDirectoryName(GameProj.FilePath), "Saved", "Sandboxes", "Cooked-" + CookPlatform);
        }
        public override void DoBuild(GUBP bp)
        {
            if (HostPlatform == UnrealTargetPlatform.Mac)
            {
                // not sure if we need something here or if the cook commandlet will automatically convert the exe name
            }
            CommandUtils.CookCommandlet(GameProj.FilePath, "UE4Editor-Cmd.exe", null, null, CookPlatform);
            var CookedPath = RootIfAnyForTempStorage();
            var CookedFiles = CommandUtils.FindFiles("*", true, CookedPath);
            BuildProducts = new List<string>();
            foreach (var CookedFile in CookedFiles)
            {
                AddBuildProduct(CookedFile);
            }
        }
        public override string FailureEMails(GUBP bp, string Branch)
        {
            if (GameProj.GameName != bp.Branch.BaseEngineProject.GameName && GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                return MergeSpaceStrings(base.FailureEMails(bp, Branch),
                   GameProj.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetGameFailureEMails_EditorTypeOnly(Branch),
                   Platform.Platforms[TargetPlatform].GUBP_GetPlatformFailureEMails(Branch));
            }
            else
            {
                return MergeSpaceStrings(base.FailureEMails(bp, Branch),
                   bp.Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetGameFailureEMails_EditorTypeOnly(Branch),
                   Platform.Platforms[TargetPlatform].GUBP_GetPlatformFailureEMails(Branch));
            }
        }
    }

    //@todo copy-paste from temp storage, unify
    public static string ResolveSharedBuildDirectory(string GameFolder)
    {
        string Root = RootSharedTempStorageDirectory();
        string Result = CombinePaths(Root, GameFolder);
        if (!DirectoryExists_NoExceptions(Result))
        {
            string GameStr = "Game";
            bool HadGame = false;
            if (GameFolder.EndsWith(GameStr, StringComparison.InvariantCultureIgnoreCase))
            {
                string ShortFolder = GameFolder.Substring(0, GameFolder.Length - GameStr.Length);
                Result = CombinePaths(Root, ShortFolder);
                HadGame = true;
            }
            if (!HadGame || !DirectoryExists_NoExceptions(Result))
            {
                Result = CombinePaths(Root, "UE4");
                if (!DirectoryExists_NoExceptions(Result))
                {
                    throw new AutomationException("Could not find an appropriate shared temp folder {0}", Result);
                }
            }
        }
        return Result;
    }


    public class GamePlatformCookedAndCompiledNode : HostPlatformAggregateNode
    {
        BranchInfo.BranchUProject GameProj;
        UnrealTargetPlatform TargetPlatform;

        public GamePlatformCookedAndCompiledNode(GUBP bp, UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, UnrealTargetPlatform InTargetPlatform, bool bCodeProject)
            : base(InHostPlatform)
        {
            GameProj = InGameProj;
            TargetPlatform = InTargetPlatform;

            foreach (var Kind in BranchInfo.MonolithicKinds)
            {
                if (bCodeProject)
                {
                    if (GameProj.Properties.Targets.ContainsKey(Kind))
                    {
                        var Target = GameProj.Properties.Targets[Kind];
                        var Platforms = Target.Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform);
                        if (Platforms.Contains(TargetPlatform) && Target.Rules.SupportsPlatform(TargetPlatform))
                        {
                            //@todo how do we get the client target platform?
                            string CookedPlatform = Platform.Platforms[TargetPlatform].GetCookPlatform(Kind == TargetRules.TargetType.Server, Kind == TargetRules.TargetType.Client, "");
                            AddDependency(CookNode.StaticGetFullName(HostPlatform, GameProj, CookedPlatform));
                            AddDependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, GameProj, TargetPlatform));
                        }
                    }
                }
                else
                {
                    if (Kind == TargetRules.TargetType.Game) //for now, non-code projects don't do client or server.
                    {
                        if (bp.Branch.BaseEngineProject.Properties.Targets.ContainsKey(Kind))
                        {
                            var Target = bp.Branch.BaseEngineProject.Properties.Targets[Kind];
                            var Platforms = Target.Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform);
                            if (Platforms.Contains(TargetPlatform) && Target.Rules.SupportsPlatform(TargetPlatform))
                            {
                                //@todo how do we get the client target platform?
                                string CookedPlatform = Platform.Platforms[TargetPlatform].GetCookPlatform(Kind == TargetRules.TargetType.Server, Kind == TargetRules.TargetType.Client, "");
                                AddDependency(CookNode.StaticGetFullName(HostPlatform, GameProj, CookedPlatform));
                                AddDependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, TargetPlatform));
                            }
                        }
                    }
                }
            }
        }

        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, UnrealTargetPlatform InTargetPlatform)
        {
            return InGameProj.GameName + "_" + InTargetPlatform + "_CookedAndCompiled" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform, GameProj, TargetPlatform);
        }
        public override string GameNameIfAnyForTempStorage()
        {
            return GameProj.GameName;
        }
    };

    public class FormalBuildNode : HostPlatformNode
    {
        BranchInfo.BranchUProject GameProj;
        List<UnrealTargetPlatform> ClientTargetPlatforms;
        List<UnrealTargetPlatform> ServerTargetPlatforms;
        List<UnrealTargetConfiguration> ClientConfigs;
        List<UnrealTargetConfiguration> ServerConfigs;

        public FormalBuildNode(GUBP bp,
            BranchInfo.BranchUProject InGameProj,
            UnrealTargetPlatform InHostPlatform,
            List<UnrealTargetPlatform> InClientTargetPlatforms,
            List<UnrealTargetConfiguration> InClientConfigs = null,
            List<UnrealTargetPlatform> InServerTargetPlatforms = null,
            List<UnrealTargetConfiguration> InServerConfigs = null

            )
            : base(InHostPlatform)
        {
            GameProj = InGameProj;
            ClientTargetPlatforms = InClientTargetPlatforms;
            ServerTargetPlatforms = InServerTargetPlatforms;
            ClientConfigs = InClientConfigs;
            ServerConfigs = InServerConfigs;

            if (ClientConfigs == null)
            {
                ClientConfigs = new List<UnrealTargetConfiguration>() { UnrealTargetConfiguration.Development };
            }

            var AllTargetPlatforms = new List<UnrealTargetPlatform>(ClientTargetPlatforms);
            if (ServerTargetPlatforms != null)
            {
                foreach (var Plat in ServerTargetPlatforms)
                {
                    if (!AllTargetPlatforms.Contains(Plat))
                    {
                        AllTargetPlatforms.Add(Plat);
                    }
                }
                if (ServerConfigs == null)
                {
                    ServerConfigs = new List<UnrealTargetConfiguration>() { UnrealTargetConfiguration.Development };
                }
            }

            // add dependencies for cooked and compiled
            foreach (var Plat in AllTargetPlatforms)
            {
                AddDependency(GamePlatformCookedAndCompiledNode.StaticGetFullName(HostPlatform, GameProj, Plat));
            }

            // verify we actually built these
            var WorkingGameProject = InGameProj;
            if (!WorkingGameProject.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                // this is a codeless project, use the base project
                WorkingGameProject = bp.Branch.BaseEngineProject;
            }

            if (!WorkingGameProject.Properties.Targets.ContainsKey(TargetRules.TargetType.Game))
            {
                throw new AutomationException("Can't make a game build for {0} because it doesn't have a game target.", WorkingGameProject.GameName);
            }
            //@todo this doesn't support client only 
            foreach (var Plat in ClientTargetPlatforms)
            {
                if (!WorkingGameProject.Properties.Targets[TargetRules.TargetType.Game].Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform).Contains(Plat))
                {
                    throw new AutomationException("Can't make a game build for {0} because we didn't build platform {1}.", WorkingGameProject.GameName, Plat.ToString());
                }
                foreach (var Config in ClientConfigs)
                {
                    if (!WorkingGameProject.Properties.Targets[TargetRules.TargetType.Game].Rules.GUBP_GetConfigs_MonolithicOnly(HostPlatform, Plat).Contains(Config))
                    {
                        throw new AutomationException("Can't make a game build for {0} because we didn't build platform {1} config {2}.", WorkingGameProject.GameName, Plat.ToString(), Config.ToString());
                    }
                }
            }
            if (ServerTargetPlatforms != null)
            {
                foreach (var Plat in ServerTargetPlatforms)
                {
                    if (!WorkingGameProject.Properties.Targets[TargetRules.TargetType.Server].Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform).Contains(Plat))
                    {
                        throw new AutomationException("Can't make a server build for {0} because we didn't build platform {1}.", WorkingGameProject.GameName, Plat.ToString());
                    }
                    foreach (var Config in ServerConfigs)
                    {
                        if (!WorkingGameProject.Properties.Targets[TargetRules.TargetType.Server].Rules.GUBP_GetConfigs_MonolithicOnly(HostPlatform, Plat).Contains(Config))
                        {
                            throw new AutomationException("Can't make a server build for {0} because we didn't build platform {1} config {2}.", WorkingGameProject.GameName, Plat.ToString(), Config.ToString());
                        }
                    }
                }
            }
        }

        public static string StaticGetFullName(BranchInfo.BranchUProject InGameProj, UnrealTargetPlatform InHostPlatform, List<UnrealTargetPlatform> InClientTargetPlatforms)
        {
            string Infix = "";
            if (InClientTargetPlatforms.Count == 1)
            {
                Infix = "_" + InClientTargetPlatforms[0].ToString();
            }
            return InGameProj.GameName + Infix + "_MakeBuild" + HostPlatformNode.StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(GameProj, HostPlatform, ClientTargetPlatforms);
        }
        public override string GameNameIfAnyForTempStorage()
        {
            return GameProj.GameName;
        }
        public override bool SendSuccessEmail()
        {
            return true;
        }
        public override float Priority()
        {
            return base.Priority() + 20.0f;
        }

        public override void DoBuild(GUBP bp)
        {
            BuildProducts = new List<string>();
            string Args = String.Format("BuildCookRun -project={0} -SkipBuild -SkipCook -Stage -Pak -Package -NoSubmit", GameProj.FilePath);

            bool bFirstClient = true;
            foreach (var Plat in ClientTargetPlatforms)
            {
                if (bFirstClient)
                {
                    bFirstClient = false;
                    Args += String.Format(" -platform={0}", Plat.ToString());
                }
                else
                {
                    Args += String.Format("+{0}", Plat.ToString());
                }
            }
            bool bFirstClientConfig = true;
            foreach (var Config in ClientConfigs)
            {
                if (bFirstClientConfig)
                {
                    bFirstClientConfig = false;
                    Args += String.Format(" -clientconfig={0}", Config.ToString());
                }
                else
                {
                    Args += String.Format("+{0}", Config.ToString());
                }
            }
            if (ServerTargetPlatforms != null)
            {
                Args += " -server";
                bool bFirstServer = true;
                foreach (var Plat in ServerTargetPlatforms)
                {
                    if (bFirstServer)
                    {
                        bFirstServer = false;
                        Args += String.Format(" -serverplatform={0}", Plat.ToString());
                    }
                    else
                    {
                        Args += String.Format("+{0}", Plat.ToString());
                    }
                }
                bool bFirstServerConfig = true;
                foreach (var Config in ServerConfigs)
                {
                    if (bFirstServerConfig)
                    {
                        bFirstServerConfig = false;
                        Args += String.Format(" -serverconfig={0}", Config.ToString());
                    }
                    else
                    {
                        Args += String.Format("+{0}", Config.ToString());
                    }
                }
            }

            if (P4Enabled)
            {
                string BaseDir = ResolveSharedBuildDirectory(GameProj.GameName);
                string ArchiveDirectory = CombinePaths(BaseDir, GetFullName(), P4Env.BuildRootEscaped + "-CL-" + P4Env.ChangelistString);
                if (DirectoryExists_NoExceptions(ArchiveDirectory))
                {
                    if (IsBuildMachine)
                    {
                        throw new AutomationException("Archive directory already exists {0}", ArchiveDirectory);
                    }
                    DeleteDirectory_NoExceptions(ArchiveDirectory);
                }
                Args += String.Format(" -Archive -archivedirectory={0}", ArchiveDirectory);
            }


            string LogFile = CommandUtils.RunUAT(CommandUtils.CmdEnv, Args);
            SaveRecordOfSuccessAndAddToBuildProducts(CommandUtils.ReadAllText(LogFile));
        }
        public override string FailureEMails(GUBP bp, string Branch)
        {
            var WorkingGameProject = GameProj;
            if (!WorkingGameProject.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                // this is a codeless project, use the base project
                WorkingGameProject = bp.Branch.BaseEngineProject;
            }

            return MergeSpaceStrings(base.FailureEMails(bp, Branch),
               WorkingGameProject.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetPromotionEMails_EditorTypeOnly(Branch));
        }
    }


    public class TestNode : HostPlatformNode
    {
        public TestNode(UnrealTargetPlatform InHostPlatform)
            : base(InHostPlatform)
        {
        }
        public override float Priority()
        {
            return 0.0f;
        }
        public virtual void DoTest(GUBP bp)
        {
        }
        public override void DoBuild(GUBP bp)
        {
            //aggregates don't do anything, they just merge dependencies
            BuildProducts = new List<string>();
            DoTest(bp);
        }
    }

    public class NonUnityTestNode : TestNode
    {
        public NonUnityTestNode(UnrealTargetPlatform InHostPlatform)
            : base(InHostPlatform)
        {
            AddPseudodependency(SharedLabelPromotableNode.StaticGetFullName(false));
        }
        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform)
        {
            return "NonUnityTestCompile" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform);
        }

        public override void DoTest(GUBP bp)
        {
            var Build = new UE4Build(bp);
            var Agenda = new UE4Build.BuildAgenda();
            
            Agenda.AddTargets(new string[] { "UnrealHeaderTool" }, HostPlatform, UnrealTargetConfiguration.Development);
            Agenda.AddTargets(
                new string[] { bp.Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor].TargetName },
                HostPlatform, UnrealTargetConfiguration.Development, InAddArgs: "-skipnonhostplatforms");

            foreach (var Kind in BranchInfo.MonolithicKinds)
            {
                if (bp.Branch.BaseEngineProject.Properties.Targets.ContainsKey(Kind))
                {
                    var Target = bp.Branch.BaseEngineProject.Properties.Targets[Kind];
                    Agenda.AddTargets(new string[] { Target.TargetName }, HostPlatform, UnrealTargetConfiguration.Development, bp.Branch.BaseEngineProject.FilePath);
                }
            }

            Build.Build(Agenda, InDeleteBuildProducts: true, InUpdateVersionFiles: false, InForceNonUnity: true);

            UE4Build.CheckBuildProducts(Build.BuildProductFiles);
            SaveRecordOfSuccessAndAddToBuildProducts();
        }
        public override string FailureEMails(GUBP bp, string Branch)
        {
            string Emails;
            Emails = MergeSpaceStrings(base.FailureEMails(bp, Branch),
                bp.Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetGameFailureEMails_EditorTypeOnly(Branch));
            return Emails;
        }


    }

    public class UATTestNode : TestNode
    {
        string TestName;
        BranchInfo.BranchUProject GameProj;
        string UATCommandLine;
        bool DependsOnEditor;
        List<UnrealTargetPlatform> DependsOnCooked;
        bool bExposeToEC;
        float ECPriority;

        public UATTestNode(GUBP bp, UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, string InTestName, string InUATCommandLine, bool InDependsOnEditor = true, List<UnrealTargetPlatform> InDependsOnCooked = null, bool bInExposeToEC = false, float InECPriority = 0.0f)
            : base(InHostPlatform)
        {
            bExposeToEC = bInExposeToEC;
            ECPriority = InECPriority;
            GameProj = InGameProj;
            TestName = InTestName;
            UATCommandLine = InUATCommandLine;
            bool bWillCook = InUATCommandLine.IndexOf("-cook") >= 0;
            DependsOnEditor = InDependsOnEditor || bWillCook;
            if (InDependsOnCooked != null)
            {
                DependsOnCooked = InDependsOnCooked;
            }
            else
            {
                DependsOnCooked = new List<UnrealTargetPlatform>();
            }
            if (DependsOnEditor)
            {
                AddDependency(EditorAndToolsNode.StaticGetFullName(HostPlatform));
                if (GameProj.GameName != bp.Branch.BaseEngineProject.GameName)
                {
                    if (GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
                    {
                        AddDependency(EditorGameNode.StaticGetFullName(HostPlatform, GameProj));
                    }
                }
            }
            foreach (var Plat in DependsOnCooked)
            {
                AddDependency(GamePlatformCookedAndCompiledNode.StaticGetFullName(HostPlatform, GameProj, Plat));
                if (bWillCook && Plat != HostPlatform && Plat != GUBP.GetAltHostPlatform(HostPlatform))
                {
                    AddDependency(EditorPlatformNode.StaticGetFullName(HostPlatform, Plat));
                }
            }

            if (!bp.bWide)
            {
                var Options = InGameProj.Options(HostPlatform);
                if (Options.bSeparateGamePromotion)
                {
                    AddPseudodependency(GameLabelPromotableNode.StaticGetFullName(GameProj, false));
                }
                else
                {
                    AddPseudodependency(WaitForTestShared.StaticGetFullName());
                }
            }
            if (InGameProj.GameName == bp.Branch.BaseEngineProject.GameName)
            {
                ECPriority = ECPriority + 1.0f;
            }
            if (UATCommandLine.IndexOf("-RunAutomationTests", StringComparison.InvariantCultureIgnoreCase) >= 0)
            {
                ECPriority = ECPriority - 4.0f;
                if (UATCommandLine.IndexOf("-EditorTest", StringComparison.InvariantCultureIgnoreCase) >= 0)
                {
                    ECPriority = ECPriority - 4.0f;
                }
            }
            else if (UATCommandLine.IndexOf("-EditorTest", StringComparison.InvariantCultureIgnoreCase) >= 0)
            {
                ECPriority = ECPriority + 2.0f;
            }

        }
        public override float Priority()
        {
            return ECPriority;
        }
        public override bool RunInEC()
        {
            return bExposeToEC;
        }
        public override string ECAgentString()
        {
            string Result = base.ECAgentString();
            foreach (UnrealTargetPlatform platform in Enum.GetValues(typeof(UnrealTargetPlatform)))
            {
                if (platform != HostPlatform && platform != GetAltHostPlatform())
                {
                    if (UATCommandLine.IndexOf("-platform=" + platform.ToString(), StringComparison.InvariantCultureIgnoreCase) >= 0)
                    {
                        Result = MergeSpaceStrings(Result, platform.ToString());
                    }
                }
            }
            return Result;
        }

        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, string InTestName)
        {
            return InGameProj.GameName + "_" + InTestName + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform, GameProj, TestName);
        }
        public override string GameNameIfAnyForTempStorage()
        {
            return GameProj.GameName;
        }
        public override void DoTest(GUBP bp)
        {
            string ProjectArg = "";
            if (!String.IsNullOrEmpty(GameProj.FilePath) && UATCommandLine.IndexOf("-project=", StringComparison.InvariantCultureIgnoreCase) < 0)
            {
                ProjectArg = " -project=\"" + GameProj.FilePath + "\"";
            }
            string WorkingCommandline = UATCommandLine + ProjectArg + " -NoSubmit";
            if (WorkingCommandline.Contains("-project=\"\""))
            {
                throw new AutomationException("Command line {0} contains -project=\"\" which is doomed to fail", WorkingCommandline);
            }
            string LogFile = RunUAT(CommandUtils.CmdEnv, WorkingCommandline);
            SaveRecordOfSuccessAndAddToBuildProducts(CommandUtils.ReadAllText(LogFile));
        }
        public override string FailureEMails(GUBP bp, string Branch)
        {
            string Emails;
            if (GameProj.GameName != bp.Branch.BaseEngineProject.GameName && GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                Emails = MergeSpaceStrings(base.FailureEMails(bp, Branch),
                   GameProj.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetGameFailureEMails_EditorTypeOnly(Branch));
            }
            else
            {
                Emails = MergeSpaceStrings(base.FailureEMails(bp, Branch),
                   bp.Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetGameFailureEMails_EditorTypeOnly(Branch));
            }
            foreach (var Plat in DependsOnCooked)
            {
                Emails = MergeSpaceStrings(Emails, Platform.Platforms[Plat].GUBP_GetPlatformFailureEMails(Branch));
            }
            return Emails;
        }
    }

    public class GameAggregateNode : HostPlatformAggregateNode
    {
        BranchInfo.BranchUProject GameProj;
        string AggregateName;
        bool bExposeToEC;
        float ECPriority;

        public GameAggregateNode(GUBP bp, UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, string InAggregateName, List<string> Dependencies, bool bInExposeToEC, float InECPriority = 100.0f)
            : base(InHostPlatform)
        {
            GameProj = InGameProj;
            AggregateName = InAggregateName;
            bExposeToEC = bInExposeToEC;
            ECPriority = InECPriority;

            foreach (var Dep in Dependencies)
            {
                AddDependency(Dep);
            }
        }
        public override float Priority()
        {
            return ECPriority;
        }
        public override bool RunInEC()
        {
            return bExposeToEC;
        }

        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, string InAggregateName)
        {
            return InGameProj.GameName + "_" + InAggregateName + StaticGetHostPlatformSuffix(InHostPlatform); ;
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform, GameProj, AggregateName);
        }
        public override string GameNameIfAnyForTempStorage()
        {
            return GameProj.GameName;
        }
        public override string FailureEMails(GUBP bp, string Branch)
        {
            if (GameProj.GameName != bp.Branch.BaseEngineProject.GameName && GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                return MergeSpaceStrings(base.FailureEMails(bp, Branch),
                   GameProj.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetGameFailureEMails_EditorTypeOnly(Branch));
            }
            return MergeSpaceStrings(base.FailureEMails(bp, Branch),
               bp.Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetGameFailureEMails_EditorTypeOnly(Branch));
        }
    };

    Dictionary<string, GUBPNode> GUBPNodes;
    Dictionary<string, bool> GUBPNodesCompleted;
    Dictionary<string, string> GUBPNodesControllingTrigger;
    Dictionary<string, string> GUBPNodesControllingTriggerDotName;

    class NodeHistory
    {
        public int LastSucceeded = 0;
        public List<int> InProgress = new List<int>();
        public string InProgressString = "";
        public List<int> Failed = new List<int>();
        public string FailedString = "";
        public List<int> AllStarted = new List<int>(); 
        public List<int> AllSucceeded = new List<int>(); 
        public List<int> AllFailed = new List<int>();
    };

    Dictionary<string, NodeHistory> GUBPNodesHistory;



    public string AddNode(GUBPNode Node)
    {
        string Name = Node.GetFullName();
        if (GUBPNodes.ContainsKey(Name))
        {
            throw new AutomationException("Attempt to add a duplicate node {0}", Node.GetFullName());
        }
        GUBPNodes.Add(Name, Node);
        return Name;
    }

    public void RemovePseudodependencyFromNode(string Node, string Dep)
    {
        if (!GUBPNodes.ContainsKey(Node))
        {
            throw new AutomationException("Node {0} not found", Node);
        }
        GUBPNodes[Node].RemovePseudodependency(Dep);
    }

    List<string> GetDependencies(string NodeToDo, bool bFlat = false, bool ECOnly = false)
    {
        var Result = new List<string>();
        foreach (var Node in GUBPNodes[NodeToDo].FullNamesOfDependencies)
        {
            bool Usable = GUBPNodes[Node].RunInEC() || !ECOnly;
            if (Usable)
            {
                if (!Result.Contains(Node))
                {
                    Result.Add(Node);
                }
            }
            if (bFlat || !Usable)
            {
                foreach (var RNode in GetDependencies(Node, bFlat, ECOnly))
                {
                    if (!Result.Contains(RNode))
                    {
                        Result.Add(RNode);
                    }
                }
            }
        }
        foreach (var Node in GUBPNodes[NodeToDo].FullNamesOfPseudosependencies)
        {
            bool Usable = GUBPNodes[Node].RunInEC() || !ECOnly;
            if (Usable)
            {
                if (!Result.Contains(Node))
                {
                    Result.Add(Node);
                }
            }
            if (bFlat || !Usable)
            {
                foreach (var RNode in GetDependencies(Node, bFlat, ECOnly))
                {
                    if (!Result.Contains(RNode))
                    {
                        Result.Add(RNode);
                    }
                }
            }
        }
        return Result;
    }

    List<string> GetECDependencies(string NodeToDo, bool bFlat = false)
    {
        return GetDependencies(NodeToDo, bFlat, true);
    }
    bool NodeDependsOn(string Rootward, string Leafward)
    {
        var Deps = GetDependencies(Leafward, true);
        return Deps.Contains(Rootward);
    }

    string GetControllingTrigger(string NodeToDo)
    {
        if (GUBPNodesControllingTrigger.ContainsKey(NodeToDo))
        {
            return GUBPNodesControllingTrigger[NodeToDo];
        }
        var Result = "";
        foreach (var Node in GUBPNodes[NodeToDo].FullNamesOfDependencies)
        {
            if (!GUBPNodes.ContainsKey(Node))
            {
                throw new AutomationException("Dependency {0} in {1} not found.", Node, NodeToDo);
            }
            bool IsTrigger = GUBPNodes[Node].TriggerNode();
            if (IsTrigger)
            {
                if (Node != Result && !string.IsNullOrEmpty(Result))
                {
                    throw new AutomationException("Node {0} has two controlling triggers {1} and {2}.", NodeToDo, Node, Result);
                }
                Result = Node;
            }
            else
            {
                string NewResult = GetControllingTrigger(Node);
                if (!String.IsNullOrEmpty(NewResult))
                {
                    if (NewResult != Result && !string.IsNullOrEmpty(Result))
                    {
                        throw new AutomationException("Node {0} has two controlling triggers {1} and {2}.", NodeToDo, NewResult, Result);
                    }
                    Result = NewResult;
                }
            }
        }
        foreach (var Node in GUBPNodes[NodeToDo].FullNamesOfPseudosependencies)
        {
            if (!GUBPNodes.ContainsKey(Node))
            {
                throw new AutomationException("Pseudodependency {0} in {1} not found.", Node, NodeToDo);
            }
            bool IsTrigger = GUBPNodes[Node].TriggerNode();
            if (IsTrigger)
            {
                if (Node != Result && !string.IsNullOrEmpty(Result))
                {
                    throw new AutomationException("Node {0} has two controlling triggers {1} and {2}.", NodeToDo, Node, Result);
                }
                Result = Node;
            }
            else
            {
                string NewResult = GetControllingTrigger(Node);
                if (!String.IsNullOrEmpty(NewResult))
                {
                    if (NewResult != Result && !string.IsNullOrEmpty(Result))
                    {
                        throw new AutomationException("Node {0} has two controlling triggers {1} and {2}.", NodeToDo, NewResult, Result);
                    }
                    Result = NewResult;
                }
            }
        }
        GUBPNodesControllingTrigger.Add(NodeToDo, Result);
        return Result;
    }
    string GetControllingTriggerDotName(string NodeToDo)
    {
        if (GUBPNodesControllingTriggerDotName.ContainsKey(NodeToDo))
        {
            return GUBPNodesControllingTriggerDotName[NodeToDo];
        }
        string Result = "";
        string WorkingNode = NodeToDo;
        while (true)
        {
            string ThisResult = GetControllingTrigger(WorkingNode);
            if (ThisResult == "")
            {
                break;
            }
            if (Result != "")
            {
                Result = "." + Result;
            }
            Result = ThisResult + Result;
            WorkingNode = ThisResult;
        }
        GUBPNodesControllingTriggerDotName.Add(NodeToDo, Result);
        return Result;
    }

    bool NodeIsAlreadyComplete(string NodeToDo, bool LocalOnly)
    {
        if (GUBPNodesCompleted.ContainsKey(NodeToDo))
        {
            return GUBPNodesCompleted[NodeToDo];
        }
        string NodeStoreName = StoreName + "-" + GUBPNodes[NodeToDo].GetFullName();
        string GameNameIfAny = GUBPNodes[NodeToDo].GameNameIfAnyForTempStorage();
        bool Result;
        if (LocalOnly)
        {
            Result = LocalTempStorageExists(CmdEnv, NodeStoreName);
        }
        else
        {
            Result = TempStorageExists(CmdEnv, NodeStoreName, GameNameIfAny);
        }
        if (Result)
        {
            Log("***** GUBP Trigger Node was already triggered {0} -> {1} : {2}", GUBPNodes[NodeToDo].GetFullName(), GameNameIfAny, NodeStoreName);
        }
        else
        {
            Log("***** GUBP Trigger Node was NOT yet triggered {0} -> {1} : {2}", GUBPNodes[NodeToDo].GetFullName(), GameNameIfAny, NodeStoreName);
        }
        GUBPNodesCompleted.Add(NodeToDo, Result);
        return Result;
    }
    void RunECTool(string Args)
    {
        if (ParseParam("FakeEC"))
        {
            Log("***** Would have ran ectool {0}", Args);
        }
        else
        {
            RunAndLog("ectool", Args);
        }
    }
    string GetEMailListForNode(GUBP bp, string NodeToDo)
    {        
        var BranchForEmail = "";
        if (P4Enabled)
        {
            BranchForEmail = P4Env.BuildRootP4;
        }
        var Emails = GUBPNodes[NodeToDo].FailureEMails(this, BranchForEmail);
        if (Emails == "")
        {
            Emails = "Engine-QA[epic]";
        }
        return Emails;
    }


    int PrintChanges(int LastOutputForChanges, int TopCL, int LastGreen)
    {
        if (TopCL > LastGreen)
        {
            if (LastOutputForChanges > 1990000)
            {
                string Cmd = String.Format("{0}@{1},{2} {3}@{4},{5}",
                    CombinePaths(PathSeparator.Slash, P4Env.BuildRootP4, "*", "Source", "..."), LastOutputForChanges + 1, TopCL,
                    CombinePaths(PathSeparator.Slash, P4Env.BuildRootP4, "*", "Build", "..."), LastOutputForChanges + 1, TopCL
                    );
                List<ChangeRecord> ChangeRecords;
                if (Changes(out ChangeRecords, Cmd, false, true))
                {
                    foreach (var Record in ChangeRecords)
                    {
                        if (!Record.User.Equals("buildmachine", StringComparison.InvariantCultureIgnoreCase))
                        {
                            Log("         {0} {1} {2}", Record.CL, Record.UserEmail, Record.Summary);
                        }
                    }
                }
                else
                {
                    throw new AutomationException("Could not get changes; cmdline: p4 changes {0}", Cmd);
                }
            }
        }
        return TopCL;
    }
    void PrintDetailedChanges(NodeHistory History, bool bShowAllChanges = false)
    {
        string Me = String.Format("{0}   <<<< local sync", P4Env.Changelist);
        int LastOutputForChanges = 0;
        int LastGreen = History.LastSucceeded;
        if (bShowAllChanges)
        {
            LastGreen = 0;
        }
        foreach (var cl in History.AllStarted)
        {
            if (cl < LastGreen)
            {
                continue;
            }
            if (P4Env.Changelist < cl && Me != "")
            {
                LastOutputForChanges = PrintChanges(LastOutputForChanges, P4Env.Changelist, LastGreen);
                Log("         {0}", Me);
                Me = "";
            }
            string Status = "In Process";
            if (History.AllSucceeded.Contains(cl))
            {
                Status = "ok";
            }
            if (History.AllFailed.Contains(cl))
            {
                Status = "FAIL";
            }
            LastOutputForChanges = PrintChanges(LastOutputForChanges, cl, LastGreen);
            Log("         {0}   {1}", cl, Status);
        }
        if (Me != "")
        {
            LastOutputForChanges = PrintChanges(LastOutputForChanges, P4Env.Changelist, LastGreen);
            Log("         {0}", Me);
        }
    }
    void PrintNodes(GUBP bp, List<string> Nodes, bool LocalOnly, List<string> UnfinishedTriggers = null)
    {
        bool bShowAllChanges = bp.ParseParam("AllChanges") && GUBPNodesHistory != null;
        bool bShowChanges = (bp.ParseParam("Changes") && GUBPNodesHistory != null) || bShowAllChanges;
        bool bShowDetailedHistory = (bp.ParseParam("History") && GUBPNodesHistory != null) || bShowChanges;
        bool bShowDependencies = !bp.ParseParam("NoShowDependencies") && !bShowDetailedHistory;
        bool bShowECDependencies = bp.ParseParam("ShowECDependencies");
        bool bShowHistory = !bp.ParseParam("NoHistory") && GUBPNodesHistory != null;
        bool AddEmailProps = bp.ParseParam("ShowEmails");
        bool bShowTriggers = true;
        string LastControllingTrigger = "$$NoMatch";
        foreach (var NodeToDo in Nodes)
        {
            string EMails = "";
            if (AddEmailProps)
            {
                EMails = GetEMailListForNode(bp, NodeToDo);
            }
            if (bShowTriggers)
            {
                string MyControllingTrigger = GetControllingTriggerDotName(NodeToDo);
                if (MyControllingTrigger != LastControllingTrigger)
                {
                    LastControllingTrigger = MyControllingTrigger;
                    if (MyControllingTrigger != "")
                    {
                        string Finished = "";
                        if (UnfinishedTriggers != null)
                        {
                            string MyShortControllingTrigger = GetControllingTrigger(NodeToDo);
                            if (UnfinishedTriggers.Contains(MyShortControllingTrigger))
                            {
                                Finished = "(not yet triggered)";
                            }
                            else
                            {
                                Finished = "(already triggered)";
                            }
                        }
                        Log("  Controlling Trigger: {0}    {1}", MyControllingTrigger, Finished);
                    }
                }
            }
            string Agent = GUBPNodes[NodeToDo].ECAgentString();
            if (Agent != "")
            {
                Agent = "[" + Agent + "]";
            }
            Log("    {0}{1}{2}{3}{4} {5}", NodeToDo,
                NodeIsAlreadyComplete(NodeToDo, LocalOnly) ? " - (Completed)" : "",
                GUBPNodes[NodeToDo].TriggerNode() ? " - (TriggerNode)" : "",
                GUBPNodes[NodeToDo].IsSticky() ? " - (Sticky)" : "",
                Agent,
                EMails
                );
            if (bShowHistory && GUBPNodesHistory.ContainsKey(NodeToDo))
            {
                var History = GUBPNodesHistory[NodeToDo];

                if (bShowDetailedHistory)
                {
                    PrintDetailedChanges(History, bShowAllChanges);
                }
                else
                {
                    Log("         Last Success: {0}", History.LastSucceeded);
                    Log("          Fails Since: {0}", History.FailedString);
                    Log("     InProgress Since: {0}", History.InProgressString);
                }
            }
            if (bShowDependencies)
            {
                foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfDependencies)
                {
                    Log("         {0}", Dep);
                }
                foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfPseudosependencies)
                {
                    Log("         {0} (pseudo)", Dep);
                }
            }
            if (bShowECDependencies)
            {
                foreach (var Dep in GetECDependencies(NodeToDo))
                {
                    Log("         {0}", Dep);
                }
            }
        }
    }
    public void SaveGraphVisualization(List<string> Nodes)
    {
        var TimerStartTime = DateTime.UtcNow;

        var GraphNodes = new List<GraphNode>();

        var NodeToGraphNodeMap = new Dictionary<string, GraphNode>();

        for (var NodeIndex = 0; NodeIndex < Nodes.Count; ++NodeIndex)
        {
            var Node = Nodes[NodeIndex];

            var GraphNode = new GraphNode()
            {
                Id = GraphNodes.Count,
                Label = Node
            };
            GraphNodes.Add(GraphNode);
            NodeToGraphNodeMap.Add(Node, GraphNode);
        }

        // Connect everything together
        var GraphEdges = new List<GraphEdge>();

        for (var NodeIndex = 0; NodeIndex < Nodes.Count; ++NodeIndex)
        {
            var Node = Nodes[NodeIndex];
            GraphNode NodeGraphNode = NodeToGraphNodeMap[Node];

            foreach (var Dep in GUBPNodes[Node].FullNamesOfDependencies)
            {
                GraphNode PrerequisiteFileGraphNode;
                if (NodeToGraphNodeMap.TryGetValue(Dep, out PrerequisiteFileGraphNode))
                {
                    // Connect a file our action is dependent on, to our action itself
                    var NewGraphEdge = new GraphEdge()
                    {
                        Id = GraphEdges.Count,
                        Source = PrerequisiteFileGraphNode,
                        Target = NodeGraphNode,
                        Color = new GraphColor() { R = 0.0f, G = 0.0f, B = 0.0f, A = 0.75f }
                    };

                    GraphEdges.Add(NewGraphEdge);
                }

            }
            foreach (var Dep in GUBPNodes[Node].FullNamesOfPseudosependencies)
            {
                GraphNode PrerequisiteFileGraphNode;
                if (NodeToGraphNodeMap.TryGetValue(Dep, out PrerequisiteFileGraphNode))
                {
                    // Connect a file our action is dependent on, to our action itself
                    var NewGraphEdge = new GraphEdge()
                    {
                        Id = GraphEdges.Count,
                        Source = PrerequisiteFileGraphNode,
                        Target = NodeGraphNode,
                        Color = new GraphColor() { R = 0.0f, G = 0.0f, B = 0.0f, A = 0.25f }
                    };

                    GraphEdges.Add(NewGraphEdge);
                }

            }
        }

        string Filename = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LogFolder, "GubpGraph.gexf");
        Log("Writing graph to {0}", Filename);
        GraphVisualization.WriteGraphFile(Filename, "GUBP Nodes", GraphNodes, GraphEdges);
        Log("Wrote graph to {0}", Filename);
     }


    // when the host is win64, this is win32 because those are also "host platforms"
    static public UnrealTargetPlatform GetAltHostPlatform(UnrealTargetPlatform HostPlatform)
    {
        UnrealTargetPlatform AltHostPlatform = UnrealTargetPlatform.Unknown; // when the host is win64, this is win32 because those are also "host platforms"
        if (HostPlatform == UnrealTargetPlatform.Win64)
        {
            AltHostPlatform = UnrealTargetPlatform.Win32;
        }
        return AltHostPlatform;
    }

    public List<UnrealTargetPlatform> GetMonolithicPlatformsForUProject(UnrealTargetPlatform HostPlatform, BranchInfo.BranchUProject GameProj, bool bIncludeHostPlatform)
    {
        UnrealTargetPlatform AltHostPlatform = GetAltHostPlatform(HostPlatform);
        var Result = new List<UnrealTargetPlatform>();
        foreach (var Kind in BranchInfo.MonolithicKinds)
        {
            if (GameProj.Properties.Targets.ContainsKey(Kind))
            {
                var Target = GameProj.Properties.Targets[Kind];
                var Platforms = Target.Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform);
                foreach (var Plat in Platforms)
                {
                    if (ActivePlatforms.Contains(Plat) && Target.Rules.SupportsPlatform(Plat) &&
                        ((Plat != HostPlatform && Plat != AltHostPlatform) || bIncludeHostPlatform))
                    {
                        Result.Add(Plat);
                    }
                }
            }
        }
        return Result;
    }

    List<int> ConvertCLToIntList(List<string> Strings)
    {
        var Result = new List<int>();
        foreach (var ThisString in Strings)
        {
            int ThisInt = int.Parse(ThisString);
            if (ThisInt < 1960000 || ThisInt > 3000000)
            {
                Log("CL {0} appears to be out of range", ThisInt);
            }
            Result.Add(ThisInt);
        }
        Result.Sort();
        return Result;
    }
    void SaveStatus(string NodeToDo, string Suffix, string NodeStoreName, bool bSaveSharedTempStorage, string GameNameIfAny)
    {
        string Contents = "Just a status record: " + Suffix;
        string RecordOfSuccess = CombinePaths(CommandUtils.CmdEnv.LocalRoot, "Engine", "Saved", "Logs", NodeToDo + Suffix +".log");
        CreateDirectory(Path.GetDirectoryName(RecordOfSuccess));
        WriteAllText(RecordOfSuccess, Contents);
        StoreToTempStorage(CmdEnv, NodeStoreName + Suffix, new List<string> { RecordOfSuccess }, !bSaveSharedTempStorage, GameNameIfAny);
    }



    [Help("Runs one, several or all of the GUBP nodes")]
    [Help(typeof(UE4Build))]
    [Help("WithMac", "Toggle to set the host platform to Mac+Win64, default is Win64")]
    [Help("OnlyMac", "Toggle to set the host platform to Mac, default is Win64")]
    [Help("CleanLocal", "delete the local temp storage before we start")]
    [Help("Store=", "Sets the name of the temp storage block, normally, this is built for you.")]
    [Help("StoreSuffix=", "Tacked onto a store name constructed from CL, branch, etc")]
    [Help("Node=", "Nodes to process, -node=Node1+Node2+Node3, if no nodes or games are specified, defaults to all nodes.")]
    [Help("OnlyNode=", "Nodes to process NO dependencies, -OnlyNode=Node1+Node2+Node3, if no nodes or games are specified, defaults to all nodes.")]
    [Help("TriggerNode=", "Trigger Nodes to process, -triggernode=Node.")]
    [Help("Game=", "Games to process, -game=Game1+Game2+Game3, if no games or nodes are specified, defaults to all nodes.")]
    [Help("ListOnly", "List Nodes in this branch")]
    [Help("SaveGraph", "Save graph as an xml file")]
    [Help("CommanderJobSetupOnly", "Set up the EC branch info via ectool and quit")]
    [Help("FakeEC", "don't run ectool, rather just do it locally, emulating what EC would have done.")]
    [Help("Fake", "Don't actually build anything, just store a record of success as the build product for each node.")]
    [Help("AllPlatforms", "Regardless of what is installed on this machine, set up the graph for all platforms; true by default on build machines.")]
    [Help("SkipTriggers", "ignore all triggers")]
    [Help("CL", "force the CL to something, disregarding the P4 value.")]
    [Help("History", "Like ListOnly, except gives you a full history. Must have -P4 for this to work.")]
    [Help("Changes", "Like history, but also shows the P4 changes. Must have -P4 for this to work.")]
    [Help("AllChanges", "Like changes except includes changes before the last green. Must have -P4 for this to work.")]
    [Help("Wide", "Remove pseudodependencies that prevent cooks and tests before promotable.")]
    public override void ExecuteBuild()
    {
        Log("************************* GUBP");

        var HostPlatforms = new List<UnrealTargetPlatform>();
        if (!ParseParam("OnlyMac"))
        {
            HostPlatforms.Add(UnrealTargetPlatform.Win64);
        }

        if (ParseParam("WithMac") || HostPlatforms.Count == 0)
        {
            HostPlatforms.Add(UnrealTargetPlatform.Mac);
        }

        StoreName = ParseParamValue("Store");
        string StoreSuffix = ParseParamValue("StoreSuffix", "");
        CL = ParseParamInt("CL", 0);
        bool bCleanLocalTempStorage = ParseParam("CleanLocal");
        bool bChanges = ParseParam("Changes") || ParseParam("AllChanges");
        bool bHistory = ParseParam("History") || bChanges;
        bool bListOnly = ParseParam("ListOnly") || bHistory;
        bool bSkipTriggers = ParseParam("SkipTriggers");
        bool bFake = ParseParam("fake");
        bool bFakeEC = ParseParam("FakeEC");
        bWide = ParseParam("Wide");
        bool bSaveSharedTempStorage = false;

        if (bHistory && !P4Enabled)
        {
            throw new AutomationException("-Changes and -History require -P4.");
        }
        bool LocalOnly = true;
        string CLString = "";
        if (String.IsNullOrEmpty(StoreName))
        {
            if (P4Enabled)
            {
                if (CL == 0)
                {
                    CL = P4Env.Changelist;
                }
                CLString = String.Format("{0}", CL);
                StoreName = "GUBP-" + P4Env.BuildRootEscaped + "-CL-" + CLString + "-" + StoreSuffix;
                bSaveSharedTempStorage = CommandUtils.IsBuildMachine;
                LocalOnly = false;
            }
            else
            {
                StoreName = "TempLocal" + "-" + StoreSuffix;
                bSaveSharedTempStorage = false;
            }
        }
        if (bFakeEC)
        {
            LocalOnly = true;
        }
        if (bSaveSharedTempStorage)
        {
            if (!HaveSharedTempStorage())
            {
                throw new AutomationException("Request to save to temp storage, but {0} is unavailable.", UE4TempStorageDirectory());
            }
            bSignBuildProducts = true;
        }
        else if (!LocalOnly && !HaveSharedTempStorage())
        {
            Log("Looks like we want to use shared temp storage, but since we don't have it, we won't use it.");
            LocalOnly = false;
        }


        Log("************************* CL:			                    {0}", CL); 
        Log("************************* P4Enabled:			            {0}", P4Enabled);
        Log("************************* HostPlatform:		            {0}", HostPlatforms.ToString()); 
        Log("************************* StoreName:		                {0}", StoreName.ToString());
        Log("************************* bCleanLocalTempStorage:		    {0}", bCleanLocalTempStorage);
        Log("************************* bSkipTriggers:		            {0}", bSkipTriggers);
        Log("************************* bSaveSharedTempStorage:		    {0}", bSaveSharedTempStorage);
        Log("************************* bSignBuildProducts:		        {0}", bSignBuildProducts);
        Log("************************* bFake:           		        {0}", bFake);
        Log("************************* bFakeEC:           		        {0}", bFakeEC);
        Log("************************* bHistory:           		        {0}", bHistory);

        GUBPNodes = new Dictionary<string, GUBPNode>();

        Branch = new BranchInfo(HostPlatforms);


        if (IsBuildMachine || ParseParam("AllPlatforms"))
        {
            ActivePlatforms = new List<UnrealTargetPlatform>();
            foreach (var GameProj in Branch.CodeProjects)
            {
                foreach (var Kind in BranchInfo.MonolithicKinds)
                {
                    if (GameProj.Properties.Targets.ContainsKey(Kind))
                    {
                        var Target = GameProj.Properties.Targets[Kind];
                        foreach (var HostPlatform in HostPlatforms)
                        {
                            var Platforms = Target.Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform);
                            foreach (var Plat in Platforms)
                            {
                                if (Target.Rules.SupportsPlatform(Plat) && !ActivePlatforms.Contains(Plat))
                                {
                                    ActivePlatforms.Add(Plat);
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            ActivePlatforms = new List<UnrealTargetPlatform>(CommandUtils.KnownTargetPlatforms);
        }
        foreach (var Plat in ActivePlatforms)
        {
            Log("Active Platform: {0}", Plat.ToString());
        }


        AddNode(new VersionFilesNode());
        foreach (var HostPlatform in HostPlatforms)
        {

            AddNode(new RootEditorNode(HostPlatform));
            AddNode(new ToolsNode(HostPlatform));
            AddNode(new EditorAndToolsNode(HostPlatform));
            AddNode(new NonUnityTestNode(HostPlatform));

            foreach (var Plat in ActivePlatforms)
            {
                if (Plat != HostPlatform && Plat != GetAltHostPlatform(HostPlatform))
                {
                    AddNode(new EditorPlatformNode(HostPlatform, Plat));
                }
            }
            int NumShared = 0;
            foreach (var CodeProj in Branch.CodeProjects)
            {
                var Options = CodeProj.Options(HostPlatform);

                if (Options.bIsPromotable && !Options.bSeparateGamePromotion)
                {
                    NumShared++;
                }

                if (!Options.bIsPromotable && !Options.bTestWithShared)
                {
                    continue; // we skip things that aren't promotable and aren't tested
                }

                AddNode(new EditorGameNode(this, HostPlatform, CodeProj));
                {
                    var EditorTests = CodeProj.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetEditorTests_EditorTypeOnly();
                    var EditorTestNodes = new List<string>();
                    foreach (var Test in EditorTests)
                    {
                        EditorTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, CodeProj, Test.Key, Test.Value)));
                    }
                    if (EditorTestNodes.Count > 0)
                    {
                        AddNode(new GameAggregateNode(this, HostPlatform, CodeProj, "AllEditorTests", EditorTestNodes, true, 0.0f));
                    }
                }

                var ServerPlatforms = new List<UnrealTargetPlatform>();
                var GamePlatforms = new List<UnrealTargetPlatform>();
                var GameTestNodes = new List<string>();
                foreach (var Kind in BranchInfo.MonolithicKinds)
                {
                    if (CodeProj.Properties.Targets.ContainsKey(Kind))
                    {
                        var Target = CodeProj.Properties.Targets[Kind];
                        var Platforms = Target.Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform);
                        foreach (var Plat in Platforms)
                        {
                            if (ActivePlatforms.Contains(Plat))
                            {
                                if (Kind == TargetRules.TargetType.Server && !ServerPlatforms.Contains(Plat))
                                {
                                    ServerPlatforms.Add(Plat);
                                }
                                if (Kind == TargetRules.TargetType.Game && !GamePlatforms.Contains(Plat))
                                {
                                    GamePlatforms.Add(Plat);
                                }
                                if (!GUBPNodes.ContainsKey(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, CodeProj, Plat)))
                                {
                                    AddNode(new GamePlatformMonolithicsNode(this, HostPlatform, CodeProj, Plat));
                                }
                                string CookedPlatform = Platform.Platforms[Plat].GetCookPlatform(Kind == TargetRules.TargetType.Server, Kind == TargetRules.TargetType.Client, "");
                                if (!GUBPNodes.ContainsKey(CookNode.StaticGetFullName(HostPlatform, CodeProj, CookedPlatform)))
                                {
                                    AddNode(new CookNode(this, HostPlatform, CodeProj, Plat, CookedPlatform));
                                }
                                if (!GUBPNodes.ContainsKey(GamePlatformCookedAndCompiledNode.StaticGetFullName(HostPlatform, CodeProj, Plat)))
                                {
                                    AddNode(new GamePlatformCookedAndCompiledNode(this, HostPlatform, CodeProj, Plat, true));
                                }
                                var GameTests = Target.Rules.GUBP_GetGameTests_MonolithicOnly(HostPlatform, GetAltHostPlatform(HostPlatform), Plat);
                                var RequiredPlatforms = new List<UnrealTargetPlatform> { Plat };
                                var ThisMonoGameTestNodes = new List<string>();
                                foreach (var Test in GameTests)
                                {
                                    ThisMonoGameTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, CodeProj, Test.Key + "_" + Plat.ToString(), Test.Value, false, RequiredPlatforms)));
                                }
                                if (ThisMonoGameTestNodes.Count > 0)
                                {
                                    GameTestNodes.Add(AddNode(new GameAggregateNode(this, HostPlatform, CodeProj, "CookedTests_" + Plat.ToString() + "_" + Kind.ToString(), ThisMonoGameTestNodes, true, 0.0f)));
                                }
                            }
                        }
                    }
                }
                foreach (var ServerPlatform in ServerPlatforms)
                {
                    foreach (var GamePlatform in GamePlatforms)
                    {
                        var Target = CodeProj.Properties.Targets[TargetRules.TargetType.Game];
                        var ClientServerTests = Target.Rules.GUBP_GetClientServerTests_MonolithicOnly(HostPlatform, GetAltHostPlatform(HostPlatform), ServerPlatform, GamePlatform);
                        var RequiredPlatforms = new List<UnrealTargetPlatform> { ServerPlatform };
                        if (ServerPlatform != GamePlatform)
                        {
                            RequiredPlatforms.Add(GamePlatform);
                        }
                        foreach (var Test in ClientServerTests)
                        {
                            GameTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, CodeProj, Test.Key + "_" + GamePlatform.ToString() + "_" + ServerPlatform.ToString(), Test.Value, false, RequiredPlatforms, true)));
                        }
                    }
                }
                if (GameTestNodes.Count > 0)
                {
                    AddNode(new GameAggregateNode(this, HostPlatform, CodeProj, "AllCookedTests", GameTestNodes, false));
                }
            }

            bool DoASharedPromotable = false;

            {
                var Options = Branch.BaseEngineProject.Options(HostPlatform);

                if (!Options.bIsPromotable || Options.bSeparateGamePromotion)
                {
                    if (NumShared > 0)
                    {
                        throw new AutomationException("We assume that if we have shared promotable, the base engine is in it. Some games are marked as shared and promotable, but the base engine is not.");
                    }
                }
                else
                {
                    DoASharedPromotable = true;
                }
            }

            if (DoASharedPromotable)
            {
                Dictionary<string, List<UnrealTargetPlatform>> NonCodeProjectNames;
                {
                    var Target = Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor];
                    NonCodeProjectNames = Target.Rules.GUBP_NonCodeProjects_BaseEditorTypeOnly(HostPlatform);

                    var Options = Branch.BaseEngineProject.Options(HostPlatform);

                    if (!Options.bIsPromotable || Options.bSeparateGamePromotion)
                    {
                        throw new AutomationException("We assume that if we have shared promotable, the base engine is in it.");
                    }
                }
                {
                    var EditorTests = Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetEditorTests_EditorTypeOnly();
                    var EditorTestNodes = new List<string>();
                    foreach (var Test in EditorTests)
                    {
                        EditorTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, Branch.BaseEngineProject, Test.Key, Test.Value)));
                        foreach (var NonCodeProject in Branch.NonCodeProjects)
                        {
                            if (!NonCodeProjectNames.ContainsKey(NonCodeProject.GameName))
                            {
                                continue;
                            }
                            EditorTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, NonCodeProject, Test.Key, Test.Value)));
                        }
                    }
                    if (EditorTestNodes.Count > 0)
                    {
                        AddNode(new GameAggregateNode(this, HostPlatform, Branch.BaseEngineProject, "AllEditorTests", EditorTestNodes, true, 0.0f));
                    }
                }

                var ServerPlatforms = new List<UnrealTargetPlatform>();
                var GamePlatforms = new List<UnrealTargetPlatform>();

                foreach (var Kind in BranchInfo.MonolithicKinds) 
                {
                    if (Branch.BaseEngineProject.Properties.Targets.ContainsKey(Kind))
                    {
                        var Target = Branch.BaseEngineProject.Properties.Targets[Kind];
                        var Platforms = Target.Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform);
                        foreach (var Plat in Platforms)
                        {
                            if (ActivePlatforms.Contains(Plat))
                            {
                                if (Kind == TargetRules.TargetType.Server && !ServerPlatforms.Contains(Plat))
                                {
                                    ServerPlatforms.Add(Plat);
                                }
                                if (Kind == TargetRules.TargetType.Game && !GamePlatforms.Contains(Plat))
                                {
                                    GamePlatforms.Add(Plat);
                                }
                                if (!GUBPNodes.ContainsKey(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, Branch.BaseEngineProject, Plat)))
                                {
                                    AddNode(new GamePlatformMonolithicsNode(this, HostPlatform, Branch.BaseEngineProject, Plat));
                                }
                            }
                        }
                    }
                }


                var GameTestNodes = new List<string>();
                var GameCookNodes = new List<string>();


                //foreach (var Kind in BranchInfo.MonolithicKinds)//for now, non-code projects don't do client or server.
                {
                    var Kind = TargetRules.TargetType.Game;
                    if (Branch.BaseEngineProject.Properties.Targets.ContainsKey(Kind))
                    {
                        var Target = Branch.BaseEngineProject.Properties.Targets[Kind];
                        var Platforms = Target.Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform);
                        foreach (var Plat in Platforms)
                        {
                            if (ActivePlatforms.Contains(Plat))
                            {
                                foreach (var NonCodeProject in Branch.NonCodeProjects)
                                {
                                    if (!NonCodeProjectNames.ContainsKey(NonCodeProject.GameName) || !NonCodeProjectNames[NonCodeProject.GameName].Contains(Plat))
                                    {
                                        continue;
                                    }

                                    string CookedPlatform = Platform.Platforms[Plat].GetCookPlatform(Kind == TargetRules.TargetType.Server, Kind == TargetRules.TargetType.Client, "");
                                    if (!GUBPNodes.ContainsKey(CookNode.StaticGetFullName(HostPlatform, NonCodeProject, CookedPlatform)))
                                    {
                                        GameCookNodes.Add(AddNode(new CookNode(this, HostPlatform, NonCodeProject, Plat, CookedPlatform)));
                                    }
                                    if (!GUBPNodes.ContainsKey(GamePlatformCookedAndCompiledNode.StaticGetFullName(HostPlatform, NonCodeProject, Plat)))
                                    {
                                        AddNode(new GamePlatformCookedAndCompiledNode(this, HostPlatform, NonCodeProject, Plat, false));
                                    }
                                    var GameTests = Target.Rules.GUBP_GetGameTests_MonolithicOnly(HostPlatform, GetAltHostPlatform(HostPlatform), Plat);
                                    var RequiredPlatforms = new List<UnrealTargetPlatform> { Plat };
                                    var ThisMonoGameTestNodes = new List<string>();
                                    foreach (var Test in GameTests)
                                    {
                                        ThisMonoGameTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, NonCodeProject, Test.Key + "_" + Plat.ToString(), Test.Value, false, RequiredPlatforms)));
                                    }
                                    if (ThisMonoGameTestNodes.Count > 0)
                                    {
                                        GameTestNodes.Add(AddNode(new GameAggregateNode(this, HostPlatform, NonCodeProject, "CookedTests_" + Plat.ToString() + "_" + Kind.ToString(), ThisMonoGameTestNodes, true, 0.0f)));
                                    }
                                }
                            }
                        }
                    }
                }
#if false
                //for now, non-code projects don't do client or server.
                foreach (var ServerPlatform in ServerPlatforms)
                {
                    var ServerTarget = Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Server];
                    foreach (var GamePlatform in GamePlatforms)
                    {
                        var Target = Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Game];

                        foreach (var NonCodeProject in Branch.NonCodeProjects)
                        {
                            if (!NonCodeProjectNames.ContainsKey(NonCodeProject.GameName) || !NonCodeProjectNames.ContainsKey(NonCodeProject.GameName) ||
                                 !NonCodeProjectNames[NonCodeProject.GameName].Contains(ServerPlatform)  || !NonCodeProjectNames[NonCodeProject.GameName].Contains(GamePlatform) )
                            {
                                continue;
                            }

                            var ClientServerTests = Target.Rules.GUBP_GetClientServerTests_MonolithicOnly(HostPlatform, GetAltHostPlatform(HostPlatform), ServerPlatform, GamePlatform);
                            var RequiredPlatforms = new List<UnrealTargetPlatform> { ServerPlatform };
                            if (ServerPlatform != GamePlatform)
                            {
                                RequiredPlatforms.Add(GamePlatform);
                            }
                            foreach (var Test in ClientServerTests)
                            {
                                GameTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, NonCodeProject, Test.Key + "_" + GamePlatform.ToString() + "_" + ServerPlatform.ToString(), Test.Value, false, RequiredPlatforms, true)));
                            }
                        }
                    }
                }
#endif
                if (GameTestNodes.Count > 0)
                {
                    AddNode(new GameAggregateNode(this, HostPlatform, Branch.BaseEngineProject, "AllCookedTests", GameTestNodes, false));
                }
            }
            AddCustomNodes(HostPlatform);
        }

        int NumSharedAllHosts = 0;
        foreach (var CodeProj in Branch.CodeProjects)
        {
            if (CodeProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                var Target = CodeProj.Properties.Targets[TargetRules.TargetType.Editor];
                bool AnySeparate = false;
                var PromotedHosts = new List<UnrealTargetPlatform>();

                foreach (var HostPlatform in HostPlatforms)
                {
                    var Options = CodeProj.Options(HostPlatform);
                    AnySeparate = AnySeparate || Options.bSeparateGamePromotion;
                    if (Options.bIsPromotable)
                    {
                        if (!Options.bSeparateGamePromotion)
                        {
                            NumSharedAllHosts++;
                        }
                        PromotedHosts.Add(HostPlatform);
                    }
                }
                if (PromotedHosts.Count > 0)
                {
                    AddNode(new GameAggregatePromotableNode(this, PromotedHosts, CodeProj));
                    if (AnySeparate)
                    {
                        AddNode(new WaitForGamePromotionUserInput(this, CodeProj, false));
                        AddNode(new GameLabelPromotableNode(this, CodeProj, false));
                        AddNode(new WaitForGamePromotionUserInput(this, CodeProj, true));
                        AddNode(new GameLabelPromotableNode(this, CodeProj, true));
                    }
                }
            }
        }
        if (NumSharedAllHosts > 0)
        {
            AddNode(new GameAggregatePromotableNode(this, HostPlatforms, Branch.BaseEngineProject));

            AddNode(new SharedAggregatePromotableNode(this, HostPlatforms));
            AddNode(new WaitForSharedPromotionUserInput(this, false));
            AddNode(new SharedLabelPromotableNode(this, false));
            AddNode(new WaitForTestShared(this));
            AddNode(new WaitForSharedPromotionUserInput(this, true));
            AddNode(new SharedLabelPromotableNode(this, true));
        }

        var NodesToDo = new HashSet<string>();

        if (bCleanLocalTempStorage)  // shared temp storage can never be wiped
        {
            DeleteLocalTempStorageManifests(CmdEnv);
        }
        Log("******* Caching completion");
        GUBPNodesCompleted = new Dictionary<string, bool>();
        GUBPNodesControllingTrigger = new Dictionary<string, string>();
        GUBPNodesControllingTriggerDotName = new Dictionary<string, string>();
        GUBPNodesHistory = new Dictionary<string, NodeHistory>();

        foreach (var Node in GUBPNodes)
        {
            Log("** {0}", Node.Key);
            NodeIsAlreadyComplete(Node.Key, LocalOnly); // cache these now to avoid spam later
            GetControllingTriggerDotName(Node.Key);
            if (CLString != "" && StoreName.Contains(CLString))
            {
                if (GUBPNodes[Node.Key].RunInEC() && !GUBPNodes[Node.Key].TriggerNode())
                {
                    string GameNameIfAny = GUBPNodes[Node.Key].GameNameIfAnyForTempStorage();
                    string NodeStoreWildCard = StoreName.Replace(CLString, "*") + "-" + GUBPNodes[Node.Key].GetFullName();
                    var History = new NodeHistory();
                    
                    History.AllStarted = ConvertCLToIntList(FindTempStorageManifests(CmdEnv, NodeStoreWildCard + StartedTempStorageSuffix, false, true, GameNameIfAny));
                    History.AllSucceeded = ConvertCLToIntList(FindTempStorageManifests(CmdEnv, NodeStoreWildCard + SucceededTempStorageSuffix, false, true, GameNameIfAny));
                    History.AllFailed = ConvertCLToIntList(FindTempStorageManifests(CmdEnv, NodeStoreWildCard + FailedTempStorageSuffix, false, true, GameNameIfAny));

                    if (History.AllSucceeded.Count > 0)
                    {
                        History.LastSucceeded = History.AllSucceeded[History.AllSucceeded.Count - 1];
                        foreach (var Failed in History.AllFailed)
                        {
                            if (Failed > History.LastSucceeded)
                            {
                                History.Failed.Add(Failed);
                                History.FailedString = GUBPNode.MergeSpaceStrings(History.FailedString, String.Format("{0}", Failed));
                            }
                        }
                        foreach (var Started in History.AllStarted)
                        {
                            if (Started > History.LastSucceeded && !History.Failed.Contains(Started))
                            {
                                History.InProgress.Add(Started);
                                History.InProgressString = GUBPNode.MergeSpaceStrings(History.InProgressString, String.Format("{0}", Started));
                            }
                        }
                        GUBPNodesHistory.Add(Node.Key, History);
                    }
                }
            }
        }
        Log("******* {0} GUBP Nodes", GUBPNodes.Count);
        foreach (var Node in GUBPNodes)
        {
            Log("  {0}", Node.Key); 
        }
        bool bOnlyNode = false;
        string PreconditionSuffix = "_PreconditionOnly";
        {
            string NodeSpec = ParseParamValue("Node");
            if (String.IsNullOrEmpty(NodeSpec))
            {
                NodeSpec = ParseParamValue("OnlyNode");
                bOnlyNode = true;
            }
            if (!String.IsNullOrEmpty(NodeSpec))
            {
                if (NodeSpec.Equals("Noop", StringComparison.InvariantCultureIgnoreCase) || NodeSpec.EndsWith(PreconditionSuffix, StringComparison.InvariantCultureIgnoreCase))
                {
                    Log("Request for Noop node, done.");
                    PrintRunTime();
                    return;
                }
                List<string> Nodes = new List<string>(NodeSpec.Split('+'));
                foreach (var NodeArg in Nodes)
                {
                    var NodeName = NodeArg.Trim();
                    if (!String.IsNullOrEmpty(NodeName))
                    {
                        foreach (var Node in GUBPNodes)
                        {
                            if (Node.Value.GetFullName().Equals(NodeArg, StringComparison.InvariantCultureIgnoreCase))
                            {
                                NodesToDo.Add(Node.Key);
                                NodeName = null;
                                break;
                            }
                        }
                        if (NodeName != null)
                        {
                            throw new AutomationException("Could not find node named {0}", NodeName);
                        }
                    }
                }
            }
        }
        string GameSpec = ParseParamValue("Game");
        if (!String.IsNullOrEmpty(GameSpec))
        {
            List<string> Games = new List<string>(GameSpec.Split('+'));
            foreach (var GameArg in Games)
            {
                var GameName = GameArg.Trim();
                if (!String.IsNullOrEmpty(GameName))
                {
                    foreach (var GameProj in Branch.CodeProjects)
                    {
                        if (GameProj.GameName.Equals(GameName, StringComparison.InvariantCultureIgnoreCase))
                        {

                            NodesToDo.Add(GameAggregatePromotableNode.StaticGetFullName(GameProj));
                            foreach (var Node in GUBPNodes)
                            {
                                if (Node.Value.GameNameIfAnyForTempStorage() == GameProj.GameName)
                                {
                                    NodesToDo.Add(Node.Key);
                                }
                            }
 
                            GameName = null;
                        }
                    }
                    if (GameName != null)
                    {
                        throw new AutomationException("Could not find game named {0}", GameName);
                    }
                }
            }
        }
        if (NodesToDo.Count == 0)
        {
            Log("No nodes specified, adding all nodes");
            foreach (var Node in GUBPNodes)
            {
                NodesToDo.Add(Node.Key);
            }
        }
        Log("Desired Nodes");
        foreach (var NodeToDo in NodesToDo)
        {
            Log("  {0}", NodeToDo);
        }

        if (!bOnlyNode)
        {
            bool bDoneWithDependencies = false;

            while (!bDoneWithDependencies)
            {
                bDoneWithDependencies = true;
                var Fringe = new HashSet<string>(); 
                foreach (var NodeToDo in NodesToDo)
                {
                    foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfDependencies)
                    {
                        if (!GUBPNodes.ContainsKey(Dep))
                        {
                            throw new AutomationException("Node {0} is not in the graph. It is a dependency of {1}.", Dep, NodeToDo);
                        }
                        if (!NodesToDo.Contains(Dep))
                        {
                            Fringe.Add(Dep);
                            bDoneWithDependencies = false;
                        }
                    }
                    foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfPseudosependencies)
                    {
                        if (!GUBPNodes.ContainsKey(Dep))
                        {
                            throw new AutomationException("Node {0} is not in the graph. It is a pseudodependency of {1}.", Dep, NodeToDo);
                        }
                    }
                }
                NodesToDo.UnionWith(Fringe);
            }
        }

        string ExplicitTrigger = "";
        bool CommanderSetup = ParseParam("CommanderJobSetupOnly");
        string FakeFail = ParseParamValue("FakeFail");
        if (CommanderSetup)
        {
            ExplicitTrigger = ParseParamValue("TriggerNode");
            if (!String.IsNullOrEmpty(ExplicitTrigger))
            {
                bool bFoundIt = false;
                foreach (var Node in GUBPNodes)
                {
                    if (Node.Value.GetFullName().Equals(ExplicitTrigger, StringComparison.InvariantCultureIgnoreCase))
                    {
                        if (Node.Value.TriggerNode() && Node.Value.RunInEC())
                        {
                            Node.Value.SetAsExplicitTrigger();
                            bFoundIt = true;
                            break;
                        }
                    }
                }
                if (!bFoundIt)
                {
                    throw new AutomationException("Could not find trigger node named {0}", ExplicitTrigger);
                }
            }
            else
            {
                ExplicitTrigger = "";
                if (bSkipTriggers)
                {
                    foreach (var Node in GUBPNodes)
                    {
                        if (Node.Value.TriggerNode() && Node.Value.RunInEC())
                        {
                            Node.Value.SetAsExplicitTrigger();
                        }
                    }
                }
            }
        } 
        var OrdereredToDo = new List<string>();

        // here we do a topological sort of the nodes, subject to a lexographical and priority sort
        while (NodesToDo.Count > 0)
        {
            bool bProgressMade = false;
            float BestPriority = -1E20f;
            string BestNode = "";
            bool BestPseudoReady = false;
            foreach (var NodeToDo in NodesToDo)
            {
                bool bReady = true;
                foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfDependencies)
                {
                    if (!GUBPNodes.ContainsKey(Dep))
                    {
                        throw new AutomationException("Dependency {0} node found.", Dep);
                    }
                    if (NodesToDo.Contains(Dep))
                    {
                        bReady = false;
                        break;
                    }
                }
                bool bPseudoReady = true;
                foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfPseudosependencies)
                {
                    if (!GUBPNodes.ContainsKey(Dep))
                    {
                        throw new AutomationException("Pseudodependency {0} node found.", Dep);
                    }
                    if (NodesToDo.Contains(Dep))
                    {
                        bPseudoReady = false;
                        break;
                    }
                }
                var Priority = GUBPNodes[NodeToDo].Priority();

                if (bReady && BestNode != "")
                {
                    if (String.Compare(GetControllingTriggerDotName(BestNode), GetControllingTriggerDotName(NodeToDo)) < 0) //sorted by controlling trigger
                    {
                        bReady = false;
                    }
                    else if (String.Compare(GetControllingTriggerDotName(BestNode), GetControllingTriggerDotName(NodeToDo)) == 0) //sorted by controlling trigger
                    {
                        if (GUBPNodes[BestNode].IsSticky() && !GUBPNodes[NodeToDo].IsSticky()) //sticky nodes first
                        {
                            bReady = false;
                        }
                        else if (GUBPNodes[BestNode].IsSticky() == GUBPNodes[NodeToDo].IsSticky())
                        {
                            if (BestPseudoReady && !bPseudoReady)
                            {
                                bReady = false;
                            }
                            else if (BestPseudoReady == bPseudoReady)
                            {
                                bool IamLateTrigger = GUBPNodes[NodeToDo].TriggerNode() && NodeToDo != ExplicitTrigger && !NodeIsAlreadyComplete(NodeToDo, LocalOnly);
                                bool BestIsLateTrigger = GUBPNodes[BestNode].TriggerNode() && BestNode != ExplicitTrigger && !NodeIsAlreadyComplete(BestNode, LocalOnly);
                                if (BestIsLateTrigger && !IamLateTrigger)
                                {
                                    bReady = false;
                                }
                                else if (BestIsLateTrigger == IamLateTrigger)
                                {

                                    if (Priority < BestPriority)
                                    {
                                        bReady = false;
                                    }
                                    else if (Priority == BestPriority)
                                    {
                                        if (BestNode.CompareTo(NodeToDo) < 0)
                                        {
                                            bReady = false;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if (bReady)
                {
                    BestPriority = Priority;
                    BestNode = NodeToDo;
                    BestPseudoReady = bPseudoReady;
                    bProgressMade = true;
                }
            }
            if (bProgressMade)
            {
                OrdereredToDo.Add(BestNode);
                NodesToDo.Remove(BestNode);
            }

            if (!bProgressMade && NodesToDo.Count > 0)
            {
                Log("Cycle in GUBP, could not resolve:");
                foreach (var NodeToDo in NodesToDo)
                {
                    Log("  {0}", NodeToDo);
                }
                throw new AutomationException("Cycle in GUBP");
            }
        }

        // find all unfinished triggers, excepting the one we are triggering right now
        var UnfinishedTriggers = new List<string>();
        if (!bSkipTriggers)
        {
            foreach (var NodeToDo in OrdereredToDo)
            {
                if (GUBPNodes[NodeToDo].TriggerNode() && !NodeIsAlreadyComplete(NodeToDo, LocalOnly))
                {
                    if (String.IsNullOrEmpty(ExplicitTrigger) || ExplicitTrigger != NodeToDo)
                    {
                        UnfinishedTriggers.Add(NodeToDo);
                    }
                }
            }
        }

        Log("*********** Desired And Dependent Nodes, in order.");
        PrintNodes(this, OrdereredToDo, LocalOnly, UnfinishedTriggers);


        if (CommanderSetup)
        {
            if (OrdereredToDo.Count == 0)
            {
                throw new AutomationException("No nodes to do!");
            }
            // we don't really need this yet, but it is left here for future use
            var ECProps = new List<string>();
            var ECJobProps = new List<string>();
            if (ExplicitTrigger != "")
            {
                ECJobProps.Add("IsRoot=0");
            }
            else
            {
                ECJobProps.Add("IsRoot=1");
            }

            var FilteredOrdereredToDo = new List<string>();
            // remove nodes that have unfinished triggers
            foreach (var NodeToDo in OrdereredToDo)
            {
                string ControllingTrigger = GetControllingTrigger(NodeToDo);
                bool bNoUnfinishedTriggers = !UnfinishedTriggers.Contains(ControllingTrigger);

                if (bNoUnfinishedTriggers)
                {
                    // if we are triggering, then remove nodes that are not controlled by the trigger or are dependencies of this trigger
                    if (!String.IsNullOrEmpty(ExplicitTrigger))
                    {
                        if (ExplicitTrigger != NodeToDo && !NodeDependsOn(NodeToDo, ExplicitTrigger) && !NodeDependsOn(ExplicitTrigger, NodeToDo))
                        {
                            continue; // this wasn't on the chain related to the trigger we are triggering, so it is not relevant
                        }
                    }
                    FilteredOrdereredToDo.Add(NodeToDo);
                }
            }
            OrdereredToDo = FilteredOrdereredToDo;
            Log("*********** EC Nodes, in order.");
            PrintNodes(this, OrdereredToDo, LocalOnly, UnfinishedTriggers);

            // here we are just making sure everything before the explicit trigger is completed.
            if (!String.IsNullOrEmpty(ExplicitTrigger))
            {
                foreach (var NodeToDo in FilteredOrdereredToDo)
                {
                    if (GUBPNodes[NodeToDo].RunInEC() && !NodeIsAlreadyComplete(NodeToDo, LocalOnly) && NodeToDo != ExplicitTrigger && !NodeDependsOn(ExplicitTrigger, NodeToDo)) // if something is already finished, we don't put it into EC
                    {
                        throw new AutomationException("We are being asked to process node {0}, however, this is an explicit trigger {1}, so everything before it should already be handled. It seems likely that you waited too long to run the trigger. You will have to do a new build from scratch.", NodeToDo, ExplicitTrigger);
                    }
                }
            }

            string LastSticky = "";
            bool HitNonSticky = false;
            bool bHaveECNodes = false;
            // sticky nodes are ones that we run on the main agent. We run then first and they must not be intermixed with parallel jobs
            foreach (var NodeToDo in OrdereredToDo)
            {
                if (GUBPNodes[NodeToDo].RunInEC() && !NodeIsAlreadyComplete(NodeToDo, LocalOnly)) // if something is already finished, we don't put it into EC
                {
                    bHaveECNodes = true;
                    if (GUBPNodes[NodeToDo].IsSticky())
                    {
                        LastSticky = NodeToDo;
                        if (HitNonSticky && !bSkipTriggers)
                        {
                            throw new AutomationException("Sticky and non-sticky jobs did not sort right.");
                        }
                    }
                    else
                    {
                        HitNonSticky = true;
                    }
                }
            }
            string ParentPath = ParseParamValue("ParentPath");
            string BaseArgs = String.Format("createJobStep --parentPath {0}", ParentPath);

            if (LastSticky == "" && bHaveECNodes)
            {
                // if we don't have any sticky nodes and we have other nodes, we run a fake noop just to release the resource 
                string Args = String.Format("{0} --subprocedure GUBP_UAT_Node --parallel 0 --jobStepName Noop --actualParameter NodeName=Noop --actualParameter Sticky=1 --releaseMode release", BaseArgs);
                RunECTool(Args);
            }

            var FakeECArgs = new List<string>();
            bool AddEmailProps = !ParseParam("NoAddEmailProps");
            var BranchForEmail = "";
            if (P4Enabled)
            {
                BranchForEmail = P4Env.BuildRootP4;
            }

            foreach (var NodeToDo in OrdereredToDo)
            {
                if (GUBPNodes[NodeToDo].RunInEC() && !NodeIsAlreadyComplete(NodeToDo, LocalOnly)) // if something is already finished, we don't put it into EC  
                {
                    string EMails = "";
                    if (AddEmailProps)
                    {
                        if (bFake)
                        {
                            EMails = "kellan.carr[epic] gil.gribb[epic]";
                        }
                        else
                        {
                            EMails = GetEMailListForNode(this, NodeToDo);
                        }
                        EMails = EMails.Trim().Replace("[epic]", "@epicgames.com");
                        ECProps.Add("FailEmails/" + NodeToDo + "=" + EMails);
                    }
                    if (GUBPNodes[NodeToDo].SendSuccessEmail())
                    {
                        ECProps.Add("SendSuccessEmail/" + NodeToDo + "=1");
                    }
                    else
                    {
                        ECProps.Add("SendSuccessEmail/" + NodeToDo + "=0");
                    }
                    ECProps.Add(string.Format("AgentRequirementString/{0}={1}", NodeToDo, GUBPNodes[NodeToDo].ECAgentString()));
                    ECProps.Add(string.Format("RequiredMemory/{0}={1}", NodeToDo, GUBPNodes[NodeToDo].AgentMemoryRequirement()));
                    ECProps.Add(string.Format("Timeouts/{0}={1}", NodeToDo, GUBPNodes[NodeToDo].TimeoutInMinutes()));
                    if (GUBPNodesHistory.ContainsKey(NodeToDo))
                    {
                        ECProps.Add(string.Format("LastGreen/{0}={1}", NodeToDo, GUBPNodesHistory[NodeToDo].LastSucceeded));
                        ECProps.Add(string.Format("RedsSince/{0}={1}", NodeToDo, GUBPNodesHistory[NodeToDo].FailedString));
                        ECProps.Add(string.Format("InProgress/{0}={1}", NodeToDo, GUBPNodesHistory[NodeToDo].InProgressString));
                    }
                    else
                    {
                        ECProps.Add(string.Format("LastGreen/{0}=0", NodeToDo));
                        ECProps.Add(string.Format("RedsSince/{0}=", NodeToDo));
                        ECProps.Add(string.Format("InProgress/{0}=", NodeToDo));
                    }
                    bool Sticky = GUBPNodes[NodeToDo].IsSticky();
                    bool DoParallel = !Sticky;
                    if (Sticky && GUBPNodes[NodeToDo].ECAgentString() != "")
                    {
                        throw new AutomationException("Node {1} is sticky but has agent requirements.", NodeToDo);
                    }
                    string Procedure = GUBPNodes[NodeToDo].ECProcedure();

                    string Args = String.Format("{0} --subprocedure {1} --parallel {2} --jobStepName {3} --actualParameter NodeName={4}",
                        BaseArgs, Procedure, DoParallel ? 1 : 0, NodeToDo, NodeToDo);
                    string ProcedureParams = GUBPNodes[NodeToDo].ECProcedureParams();
                    if (!String.IsNullOrEmpty(ProcedureParams))
                    {
                        Args = Args + " " + ProcedureParams;
                    }

                    string PreCondition = "";
                    string RunCondition = "";

                    var EcDeps = GetECDependencies(NodeToDo);
                    var UncompletedEcDeps = new List<string>();
                    foreach (var Dep in EcDeps)
                    {
                        if (GUBPNodes[Dep].RunInEC() && !NodeIsAlreadyComplete(Dep, LocalOnly) && OrdereredToDo.Contains(Dep)) // if something is already finished, we don't put it into EC
                        {
                            UncompletedEcDeps.Add(Dep);
                        }
                    }
                    if (UncompletedEcDeps.Count > 0)
                    {
                        //$[/javascript if(getProperty("$[/myWorkflow/ParentWorkflow]/GUBP_Game_$[/myWorkflow/GameName]_MonoPlatform1_Exists") != "1" || getProperty("$[/myWorkflow/ParentWorkflow]/$[$[/myWorkflow/ParentWorkflow]/GUBP_Game_$[/myWorkflow/GameName]_MonoPlatform1_Name]_EditorPlatform_Success") != "1") true; else false;]
                        {
                            PreCondition = "\"$[/javascript if(";
                            int Index = 0;
                            foreach (var Dep in UncompletedEcDeps)
                            {
                                PreCondition = PreCondition + "getProperty('" + ParentPath + "/jobSteps[" + Dep + "]/status') == 'completed'";
                                Index++;
                                if (Index != UncompletedEcDeps.Count)
                                {
                                    PreCondition = PreCondition + " && ";
                                }
                            }
                            PreCondition = PreCondition + ") true;]\"";
                        }
                        {
                            RunCondition = "\"$[/javascript if(";
                            int Index = 0;
                            foreach (var Dep in UncompletedEcDeps)
                            {
                                RunCondition = RunCondition + "('$[" + ParentPath + "/jobSteps[" + Dep + "]/outcome]' == 'success' || ";
                                RunCondition = RunCondition + "'$[" + ParentPath + "/jobSteps[" + Dep + "]/outcome]' == 'warning')";

                                Index++;
                                if (Index != UncompletedEcDeps.Count)
                                {
                                    RunCondition = RunCondition + " && ";
                                }
                            }
                            RunCondition = RunCondition + ") true; else false;]\"";
                        }
                    }

                    if (!String.IsNullOrEmpty(PreCondition))
                    {
                        Args = Args + " --precondition " + PreCondition;
                    }
                    if (!String.IsNullOrEmpty(RunCondition))
                    {
                        Args = Args + " --condition " + RunCondition;
                    }
                    if (Sticky && NodeToDo == LastSticky)
                    {
                        Args = Args + " --releaseMode release";
                    }
                    RunECTool(Args);
                    if (bFakeEC && 
                        !UnfinishedTriggers.Contains(NodeToDo) &&
                        GUBPNodes[NodeToDo].ECProcedure().StartsWith("GUBP_UAT_Node") // other things we really can't test
                        ) // unfinished triggers are never run directly by EC, rather it does another job setup
                    {
                        string Arg = String.Format("gubp -Node={0} -FakeEC {1} {2} {3} {4}", 
                            NodeToDo,
                            bFake ? "-Fake" : "" , 
                            ParseParam("AllPlatforms") ? "-AllPlatforms" : "",
                            ParseParam("UnfinishedTriggersFirst") ? "-UnfinishedTriggersFirst" : "",
                            ParseParam("UnfinishedTriggersParallel") ? "-UnfinishedTriggersParallel" : ""
                            );

                        string Node = ParseParamValue("-Node");
                        if (!String.IsNullOrEmpty(Node))
                        {
                            Arg = Arg + " -Node=" + Node;
                        }
                        if (!String.IsNullOrEmpty(FakeFail))
                        {
                            Arg = Arg + " -FakeFail=" + FakeFail;
                        } 
                        FakeECArgs.Add(Arg);
                    }
                }

            }
            {
                ECProps.Add("GUBP_LoadedProps=1");
                string BranchDefFile = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LogFolder, "BranchDef.properties");
                CommandUtils.WriteAllLines(BranchDefFile, ECProps.ToArray());
                RunECTool(String.Format("setProperty \"/myWorkflow/BranchDefFile\" \"{0}\"", BranchDefFile.Replace("\\", "\\\\")));
            }
            {
                ECProps.Add("GUBP_LoadedJobProps=1");
                string BranchJobDefFile = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LogFolder, "BranchJobDef.properties");
                CommandUtils.WriteAllLines(BranchJobDefFile, ECProps.ToArray());
                RunECTool(String.Format("setProperty \"/myJob/BranchJobDefFile\" \"{0}\"", BranchJobDefFile.Replace("\\", "\\\\")));
            }
            if (bFakeEC)
            {
                foreach (var Args in FakeECArgs)
                {
                    RunUAT(CmdEnv, Args);
                }
            }
            Log("Commander setup only, done.");
            PrintRunTime();
            return;

        }
        if (ParseParam("SaveGraph"))
        {
            SaveGraphVisualization(OrdereredToDo);
        }
        if (bListOnly)
        {
            Log("List only, done.");
            return;
        }

        var BuildProductToNodeMap = new Dictionary<string, string>();
        foreach (var NodeToDo in OrdereredToDo)
        {
            if (GUBPNodes[NodeToDo].BuildProducts != null || GUBPNodes[NodeToDo].AllDependencyBuildProducts != null)
            {
                throw new AutomationException("topological sort error");
            }

            GUBPNodes[NodeToDo].AllDependencyBuildProducts = new List<string>();
            if (GUBPNodes[NodeToDo].FullNamesOfDependencies == null)
            {
                throw new AutomationException("Node {0} was not processed yet?", NodeToDo);
            }
            foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfDependencies)
            {
                if (GUBPNodes[Dep].BuildProducts == null)
                {
                    throw new AutomationException("Node {0} was not processed yet2? Processing {1}", Dep, NodeToDo);
                }
                foreach (var Prod in GUBPNodes[Dep].BuildProducts)
                {
                    GUBPNodes[NodeToDo].AddDependentBuildProduct(Prod);
                }
                if (GUBPNodes[Dep].AllDependencyBuildProducts == null)
                {
                    throw new AutomationException("Node {0} was not processed yet3?  Processing {1}", Dep, NodeToDo);
                }
                foreach (var Prod in GUBPNodes[Dep].AllDependencyBuildProducts)
                {
                    GUBPNodes[NodeToDo].AddDependentBuildProduct(Prod);
                } 
            }
            string NodeStoreName = StoreName + "-" + GUBPNodes[NodeToDo].GetFullName();
            
            string GameNameIfAny = GUBPNodes[NodeToDo].GameNameIfAnyForTempStorage();
            string StorageRootIfAny = GUBPNodes[NodeToDo].RootIfAnyForTempStorage();
            if (bFake)
            {
                StorageRootIfAny = ""; // we don't rebase fake runs since those are entirely "records of success", which are always in the logs folder
            }

            // this is kinda complicated
            bool SaveSuccessRecords = (IsBuildMachine || bFakeEC) && // no real reason to make these locally except for fakeEC tests
                (!GUBPNodes[NodeToDo].TriggerNode() || GUBPNodes[NodeToDo].IsSticky()) // trigger nodes are run twice, one to start the new workflow and once when it is actually triggered, we will save reconds for the latter
                && (GUBPNodes[NodeToDo].RunInEC() || !GUBPNodes[NodeToDo].IsAggregate()); //aggregates not in EC can be "run" multiple times, so we can't track those

            Log("***** Running GUBP Node {0} -> {1} : {2}", GUBPNodes[NodeToDo].GetFullName(), GameNameIfAny, NodeStoreName);
            if (NodeIsAlreadyComplete(NodeToDo, LocalOnly))
            {
                Log("***** Retrieving GUBP Node {0} from {1}", GUBPNodes[NodeToDo].GetFullName(), NodeStoreName);
                GUBPNodes[NodeToDo].BuildProducts = RetrieveFromTempStorage(CmdEnv, NodeStoreName, GameNameIfAny, StorageRootIfAny);
            }
            else
            {
                if (SaveSuccessRecords) 
                {
                    SaveStatus(NodeToDo, StartedTempStorageSuffix, NodeStoreName, bSaveSharedTempStorage, GameNameIfAny);
                }
                try
                {
                    if (!String.IsNullOrEmpty(FakeFail) && FakeFail.Equals(NodeToDo, StringComparison.InvariantCultureIgnoreCase))
                    {
                        throw new AutomationException("Failing node {0} by request.", NodeToDo);
                    }
                    if (bFake)
                    {
                        Log("***** FAKE!! Building GUBP Node {0} for {1}", NodeToDo, NodeStoreName);
                        GUBPNodes[NodeToDo].DoFakeBuild(this);
                    }
                    else
                    {
                        Log("***** Building GUBP Node {0} for {1}", NodeToDo, NodeStoreName);
                        GUBPNodes[NodeToDo].DoBuild(this);
                    }
                    if (!GUBPNodes[NodeToDo].IsAggregate())
                    {
                        StoreToTempStorage(CmdEnv, NodeStoreName, GUBPNodes[NodeToDo].BuildProducts, !bSaveSharedTempStorage, GameNameIfAny, StorageRootIfAny);
                    }
                }
                catch (Exception Ex)
                {
                    if (SaveSuccessRecords)
                    {
                        SaveStatus(NodeToDo, FailedTempStorageSuffix, NodeStoreName, bSaveSharedTempStorage, GameNameIfAny);
                    }

                    Log("{0}", ExceptionToString(Ex));


                    if (GUBPNodesHistory.ContainsKey(NodeToDo))
                    {
                        var History = GUBPNodesHistory[NodeToDo];
                        Log("Changes since last green *********************************");
                        Log("");
                        Log("");
                        Log("");
                        PrintDetailedChanges(History);
                    }

                    string FailInfo = "";
                    FailInfo += "********************************* Main log file";
                    FailInfo += Environment.NewLine + Environment.NewLine;
                    FailInfo += LogUtils.GetLogTail();
                    FailInfo += Environment.NewLine + Environment.NewLine + Environment.NewLine;



                    string OtherLog = "See logfile for details: '";
                    if (FailInfo.Contains(OtherLog))
                    {
                        string LogFile = FailInfo.Substring(FailInfo.IndexOf(OtherLog) + OtherLog.Length);
                        if (LogFile.Contains("'"))
                        {
                            LogFile = CombinePaths(CmdEnv.LogFolder, LogFile.Substring(0, LogFile.IndexOf("'")));
                            if (FileExists_NoExceptions(LogFile))
                            {
                                FailInfo += "********************************* Sub log file " + LogFile;
                                FailInfo += Environment.NewLine + Environment.NewLine;

                                FailInfo += LogUtils.GetLogTail(LogFile);
                                FailInfo += Environment.NewLine + Environment.NewLine + Environment.NewLine;
                            }
                        }
                    }

                    string Filename = CombinePaths(CmdEnv.LogFolder, "LogTailsAndChanges.log");
                    WriteAllText(Filename, FailInfo);

                    throw(Ex);
                }
                if (SaveSuccessRecords) 
                {
                    SaveStatus(NodeToDo, SucceededTempStorageSuffix, NodeStoreName, bSaveSharedTempStorage, GameNameIfAny);
                }
            }
            foreach (var Product in GUBPNodes[NodeToDo].BuildProducts)
            {
                if (BuildProductToNodeMap.ContainsKey(Product))
                {
                    throw new AutomationException("Overlapping build product: {0} and {1} both produce {2}", BuildProductToNodeMap[Product], NodeToDo, Product);
                }
                BuildProductToNodeMap.Add(Product, NodeToDo);
            }
        }

        PrintRunTime();
    }
    string StartedTempStorageSuffix = "_Started";
    string FailedTempStorageSuffix = "_Failed";
    string SucceededTempStorageSuffix = "_Succeeded";
}
