// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;
using System.Reflection;

//ProjectUtil.cs

public class GUBP : BuildCommand
{
    public string StoreName = null;
    public int CL = 0;
    public bool bSignBuildProducts = false;
    public List<UnrealTargetPlatform> ActivePlatforms = null;
    public BranchInfo Branch = null;
    public bool bOrthogonalizeEditorPlatforms = false;
    public static bool bHackRunIOSCompilesOnMac = false;
    public List<UnrealTargetPlatform> HostPlatforms;
    public bool bFake = false;
    public static bool bNoIOSOnPC = false;
    public static bool bBuildRocket = false;


    public string RocketUBTArgs(bool bUseRocketInsteadOfBuildRocket = false)
    {
        if (bBuildRocket)
        {
            return " -NoSimplygon -NoSpeedTree " + (bUseRocketInsteadOfBuildRocket ? "-Rocket" : "-BuildRocket");
        }
        return "";
    }

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
        public string AgentSharingGroup = "";
        public int ComputedDependentCISFrequencyQuantumShift = -1;

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

        /// <summary>
        /// When triggered by CIS (instead of a person) this dictates how often this node runs.
        /// The builder has a increasing index, specified with -TimeIndex=N
        /// If N mod (1 lshift CISFrequencyQuantumShift()) is not 0, the node is skipped
        /// </summary>
        public virtual int CISFrequencyQuantumShift(GUBP bp)
        {
            return 0;
        }
        /// <summary>
        /// As above the maximum of all ancestors and pseudoancestors
        /// </summary>
        public int DependentCISFrequencyQuantumShift()
        {
            if (ComputedDependentCISFrequencyQuantumShift < 0)
            {
                throw new AutomationException("Asked for frequency shift before it was computed.");
            }
            return ComputedDependentCISFrequencyQuantumShift;
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
        public virtual string ECProcedureInfix()
        {
            return "";
        }
        public virtual string ECProcedure()
        {
            if (IsSticky() && AgentSharingGroup != "")
            {
                throw new AutomationException("Node {0} is both agent sharing and sitcky.", GetFullName());
            }
            return String.Format("GUBP{0}_UAT_Node{1}{2}", ECProcedureInfix(), IsSticky() ? "" : "_Parallel", AgentSharingGroup != "" ? "_AgentShare" : "");
        }
        public virtual string ECProcedureParams()
        {
            var Result = String.Format("--actualParameter Sticky={0}", IsSticky() ? 1 : 0);
            if (AgentSharingGroup != "")
            {
                Result += String.Format(" --actualParameter AgentSharingGroup={0}", AgentSharingGroup);
            }
            return Result;
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
            if (IsBuildMachine)
            {
                // only update version files on build machines
                var UE4Build = new UE4Build(bp);
                BuildProducts = UE4Build.UpdateVersionFiles();
            }
            else
            {
                base.DoBuild(bp);
            }
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
        public override string ECProcedureInfix()
        {
            if (HostPlatform == UnrealTargetPlatform.Mac)
            {
                if (IsSticky())
                {
                    throw new AutomationException("Node {0} is sticky, but Mac hosted. Sticky nodes must be PC hosted.", GetFullName());
                }
                return "_Mac";
            }
            return "";
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
        public CompileNode(UnrealTargetPlatform InHostPlatform, bool DependentOnCompileTools = true)
            : base(InHostPlatform)
        {
            if (DependentOnCompileTools)
            {
                AddDependency(ToolsForCompileNode.StaticGetFullName(HostPlatform));
            }
            else
            {
                AddDependency(VersionFilesNode.StaticGetFullName());
            }
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

    public class ToolsForCompileNode : CompileNode
    {
        public ToolsForCompileNode(UnrealTargetPlatform InHostPlatform)
            : base(InHostPlatform, false)
        {
            AgentSharingGroup = "Editor" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform)
        {
            return "ToolsForCompile" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform);
        }
        public override bool DeleteBuildProducts()
        {
            return true;
        }
        public override int AgentMemoryRequirement()
        {
            return 32;
        }
        public override UE4Build.BuildAgenda GetAgenda(GUBP bp)
        {
            var Agenda = new UE4Build.BuildAgenda();
            if (HostPlatform != UnrealTargetPlatform.Mac)
            {
                Agenda.DotNetProjects.AddRange(
                    new string[] 
			    {
				    @"Engine\Source\Programs\DotNETCommon\DotNETUtilities\DotNETUtilities.csproj",
                    @"Engine\Source\Programs\RPCUtility\RPCUtility.csproj",
			    }
                    );
            }
            Agenda.AddTargets(new string[] { "UnrealHeaderTool" }, HostPlatform, UnrealTargetConfiguration.Development);
            return Agenda;
        }
        public override void PostBuild(GUBP bp, UE4Build UE4Build)
        {
            if (HostPlatform != UnrealTargetPlatform.Mac)
            {
                UE4Build.AddUATFilesToBuildProducts();
                UE4Build.AddUBTFilesToBuildProducts();
            }
        }
    }

    public class RootEditorNode : CompileNode
    {
        public RootEditorNode(UnrealTargetPlatform InHostPlatform)
            : base(InHostPlatform)
        {
            AgentSharingGroup = "Editor" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform)
        {
            return "RootEditor" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform);
        }
        public override bool DeleteBuildProducts()
        {
            return true;
        }
        public override UE4Build.BuildAgenda GetAgenda(GUBP bp)
        {
            var Agenda = new UE4Build.BuildAgenda();

            string AddArgs = "-nobuiltuht" + bp.RocketUBTArgs();
            if (bp.bOrthogonalizeEditorPlatforms)
            {
                AddArgs += " -skipnonhostplatforms";
            }
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
    }

    public class ToolsNode : CompileNode
    {
        public ToolsNode(UnrealTargetPlatform InHostPlatform)
            : base(InHostPlatform)
        {
            AddPseudodependency(RootEditorNode.StaticGetFullName(HostPlatform));
            AgentSharingGroup = "Tools" + StaticGetHostPlatformSuffix(HostPlatform);
        }
        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform)
        {
            return "Tools" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform);
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

            if (HostPlatform != UnrealTargetPlatform.Mac)
            {
                Agenda.DotNetProjects.AddRange(
                    new string[] 
			    {
                    CombinePaths(@"Engine\Source\Programs\UnrealControls\UnrealControls.csproj"),
			    }
                    );
                Agenda.DotNetSolutions.AddRange(
                    new string[] 
			        {
				        CombinePaths(@"Engine\Source\Programs\NetworkProfiler\NetworkProfiler.sln"),   
			        }
                    );
                Agenda.SwarmProject = CombinePaths(@"Engine\Source\Programs\UnrealSwarm\UnrealSwarm.sln");
            }

            string AddArgs = "-nobuilduht -skipactionhistory" + bp.RocketUBTArgs(); ;

            foreach (var ProgramTarget in bp.Branch.BaseEngineProject.Properties.Programs)
            {
                bool bInternalOnly;
                if (ProgramTarget.Rules.GUBP_AlwaysBuildWithTools(HostPlatform, out bInternalOnly) && ProgramTarget.Rules.SupportsPlatform(HostPlatform) && !bInternalOnly)
                {
                    foreach (var Plat in ProgramTarget.Rules.GUBP_ToolPlatforms(HostPlatform))
                    {
                        foreach (var Config in ProgramTarget.Rules.GUBP_ToolConfigs(HostPlatform))
                        {
                            Agenda.AddTargets(new string[] { ProgramTarget.TargetName }, Plat, Config, InAddArgs: AddArgs);
                        }
                    }
                }
            }

            return Agenda;
        }
    }
    public class InternalToolsNode : CompileNode
    {
        public InternalToolsNode(UnrealTargetPlatform InHostPlatform)
            : base(InHostPlatform)
        {
            AddPseudodependency(RootEditorNode.StaticGetFullName(HostPlatform));
            AgentSharingGroup = "Tools" + StaticGetHostPlatformSuffix(HostPlatform);
        }
        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform)
        {
            return "InternalTools" + StaticGetHostPlatformSuffix(InHostPlatform);
        }
        public override string GetFullName()
        {
            return StaticGetFullName(HostPlatform);
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
            bool bAnyAdded = false;
            var Agenda = new UE4Build.BuildAgenda();

            if (HostPlatform != UnrealTargetPlatform.Mac)
            {
                bAnyAdded = true;
                Agenda.DotNetProjects.AddRange(
                    new string[] 
			    {
				    CombinePaths(@"Engine\Source\Programs\Distill\Distill.csproj"),		  
                    CombinePaths(@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportCommon\CrashReportCommon.csproj"),
					CombinePaths(@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportReceiver\CrashReportReceiver.csproj"),
					CombinePaths(@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportProcess\CrashReportProcess.csproj"),
                    CombinePaths(@"Engine\Source\Programs\CrashReporter\RegisterPII\RegisterPII.csproj"),
			    });
                Agenda.DotNetSolutions.AddRange(
                    new string[] 
			        {
				        CombinePaths(@"Engine\Source\Programs\UnrealDocTool\UnrealDocTool\UnrealDocTool.sln"),
			        }
                    );
                Agenda.ExtraDotNetFiles.AddRange(
                    new string[] 
			        {
				        "Interop.IWshRuntimeLibrary",
				        "UnrealMarkdown",
				        "CommonUnrealMarkdown",
			        }
                    );
                Agenda.IOSDotNetProjects.AddRange(
                        new string[]
                    {
                        CombinePaths(@"Engine\Source\Programs\IOS\iPhonePackager\iPhonePackager.csproj"),
                        CombinePaths(@"Engine\Source\Programs\IOS\MobileDeviceInterface\MobileDeviceInterface.csproj"),
                        CombinePaths(@"Engine\Source\Programs\IOS\DeploymentInterface\DeploymentInterface.csproj"),
                    }
                        );
            }
            string AddArgs = "-nobuilduht -skipactionhistory" + bp.RocketUBTArgs(); ;

            foreach (var ProgramTarget in bp.Branch.BaseEngineProject.Properties.Programs)
            {
                bool bInternalOnly;
                if (ProgramTarget.Rules.GUBP_AlwaysBuildWithTools(HostPlatform, out bInternalOnly) && ProgramTarget.Rules.SupportsPlatform(HostPlatform) && bInternalOnly)
                {
                    foreach (var Plat in ProgramTarget.Rules.GUBP_ToolPlatforms(HostPlatform))
                    {
                        foreach (var Config in ProgramTarget.Rules.GUBP_ToolConfigs(HostPlatform))
                        {
                            Agenda.AddTargets(new string[] { ProgramTarget.TargetName }, Plat, Config, InAddArgs: AddArgs);
                            bAnyAdded = true;
                        }
                    }
                }
            }
            if (bAnyAdded)
            {
                return Agenda;
            }
            return null;
        }
    }

    public class EditorPlatformNode : CompileNode
    {
        UnrealTargetPlatform EditorPlatform;

        public EditorPlatformNode(UnrealTargetPlatform InHostPlatform, UnrealTargetPlatform Plat)
            : base(InHostPlatform)
        {
            AgentSharingGroup = "Editor" + StaticGetHostPlatformSuffix(InHostPlatform);
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
            return base.Priority() + 1;
        }

        public override UE4Build.BuildAgenda GetAgenda(GUBP bp)
        {
            if (!bp.bOrthogonalizeEditorPlatforms)
            {
                throw new AutomationException("EditorPlatformNode node should not be used unless we are orthogonalizing editor platforms.");
            }
            var Agenda = new UE4Build.BuildAgenda();

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
            AgentSharingGroup = "Editor" + StaticGetHostPlatformSuffix(InHostPlatform);
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
        public override float Priority()
        {
            float Result = base.Priority();
            if (GameProj.Options(HostPlatform).bTestWithShared)
            {
                Result -= 1;
            }
            return Result;
        }
        public override int CISFrequencyQuantumShift(GUBP bp)
        {
            int Result = base.CISFrequencyQuantumShift(bp);
            if (GameProj.Options(HostPlatform).bTestWithShared)
            {
                Result += 2; // only every 80m
            }
            return Result;
        }
        public override UE4Build.BuildAgenda GetAgenda(GUBP bp)
        {
            var Agenda = new UE4Build.BuildAgenda();

            Agenda.AddTargets(
                new string[] { GameProj.Properties.Targets[TargetRules.TargetType.Editor].TargetName },
                HostPlatform, UnrealTargetConfiguration.Development, GameProj.FilePath, InAddArgs: "-nobuilduht -skipactionhistory -skipnonhostplatforms" + bp.RocketUBTArgs(true));

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
            : base((GUBP.bHackRunIOSCompilesOnMac && InTargetPlatform == UnrealTargetPlatform.IOS) ? UnrealTargetPlatform.Mac : InHostPlatform)
        {
            GameProj = InGameProj;
            TargetPlatform = InTargetPlatform;
            if (InGameProj.GameName != bp.Branch.BaseEngineProject.GameName && GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                AddPseudodependency(EditorGameNode.StaticGetFullName(InHostPlatform, GameProj));
                AddPseudodependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, TargetPlatform));
            }
            else
            {
                if (TargetPlatform != InHostPlatform)
                {
                    AddPseudodependency(GamePlatformMonolithicsNode.StaticGetFullName(InHostPlatform, bp.Branch.BaseEngineProject, InHostPlatform));
                }
            }
            if (InGameProj.Options(InHostPlatform).bTestWithShared)  /// compiling templates is only for testing purposes, and we will group them to avoid saturating the farm
            {
                AddPseudodependency(WaitForTestShared.StaticGetFullName());
                AgentSharingGroup = "TemplateMonolithics" + StaticGetHostPlatformSuffix((GUBP.bHackRunIOSCompilesOnMac && InTargetPlatform == UnrealTargetPlatform.IOS) ? UnrealTargetPlatform.Mac : InHostPlatform);
            }
        }

        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, UnrealTargetPlatform InTargetPlatform)
        {
            return InGameProj.GameName + "_" + InTargetPlatform + "_Monolithics" + StaticGetHostPlatformSuffix((GUBP.bHackRunIOSCompilesOnMac && InTargetPlatform == UnrealTargetPlatform.IOS) ? UnrealTargetPlatform.Mac : InHostPlatform);
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
        public override int CISFrequencyQuantumShift(GUBP bp)
        {
            int Result = base.CISFrequencyQuantumShift(bp);
            if (GameProj.GameName != bp.Branch.BaseEngineProject.GameName)
            {
                Result += 2; // only every 80m
            }
            else if (TargetPlatform != HostPlatform)
            {
                Result += 1; // only every 40m
            }
            return Result;
        }

        public override UE4Build.BuildAgenda GetAgenda(GUBP bp)
        {
            if (!bp.ActivePlatforms.Contains(TargetPlatform))
            {
                throw new AutomationException("{0} is not a supported platform for {1}", TargetPlatform.ToString(), GetFullName());
            }
            var Agenda = new UE4Build.BuildAgenda();

            string Args = "-nobuilduht -skipactionhistory" + bp.RocketUBTArgs();

            foreach (var Kind in BranchInfo.MonolithicKinds)
            {
                if (GUBP.bBuildRocket && Kind != TargetRules.TargetType.Game)
                {
                    continue;
                }
                if (GameProj.Properties.Targets.ContainsKey(Kind))
                {
                    var Target = GameProj.Properties.Targets[Kind];
                    var Platforms = Target.Rules.GUBP_GetPlatforms_MonolithicOnly(HostPlatform);
                    if (Platforms.Contains(TargetPlatform) && Target.Rules.SupportsPlatform(TargetPlatform))
                    {
                        var Configs = Target.Rules.GUBP_GetConfigs_MonolithicOnly(HostPlatform, TargetPlatform);
                        foreach (var Config in Configs)
                        {
                            if (GUBP.bBuildRocket)
                            {
                                if (HostPlatform == UnrealTargetPlatform.Win64 && TargetPlatform == UnrealTargetPlatform.Win32 && Config != UnrealTargetConfiguration.Shipping)
                                {
                                    continue;
                                }
                                if (HostPlatform == UnrealTargetPlatform.Win64 && TargetPlatform == UnrealTargetPlatform.Win64 && Config != UnrealTargetConfiguration.Development)
                                {
                                    continue;
                                }
                            }

                            if (GameProj.GameName == bp.Branch.BaseEngineProject.GameName)
                            {
                                Agenda.AddTargets(new string[] { Target.TargetName }, TargetPlatform, Config, InAddArgs: Args);
                            }
                            else
                            {
                                Agenda.AddTargets(new string[] { Target.TargetName }, TargetPlatform, Config, GameProj.FilePath, InAddArgs: Args);
                            }
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
            AddDependency(InternalToolsNode.StaticGetFullName(HostPlatform));
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
                AddDependency(RootEditorNode.StaticGetFullName(HostPlatform));
                if (InGameProj.GameName != bp.Branch.BaseEngineProject.GameName && GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
                {
                    AddDependency(EditorGameNode.StaticGetFullName(HostPlatform, GameProj));
                }

                // add all of the platforms I use
                {
                    var Platforms = bp.GetMonolithicPlatformsForUProject(HostPlatform, InGameProj, false);
                    if (bp.bOrthogonalizeEditorPlatforms)
                    {
                        foreach (var Plat in Platforms)
                        {
                            AddDependency(EditorPlatformNode.StaticGetFullName(HostPlatform, Plat));
                        }
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
                if(LabelPrefix == "OrionGame")
                {
                    CompleteLabelPrefix = "TEST-GUBP-" + CompleteLabelPrefix;
                }
                
            }       
            if (GUBP.bBuildRocket)
            {
                CompleteLabelPrefix = "Rocket-" + CompleteLabelPrefix;
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
            if (bp.bOrthogonalizeEditorPlatforms)
            {
                if (TargetPlatform != HostPlatform && TargetPlatform != GUBP.GetAltHostPlatform(HostPlatform))
                {
                    AddDependency(EditorPlatformNode.StaticGetFullName(HostPlatform, TargetPlatform));
                }
            }

            bool bIsShared = false;
            // is this the "base game" or a non code project?
            if (InGameProj.GameName != bp.Branch.BaseEngineProject.GameName && GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                var Options = InGameProj.Options(HostPlatform);
                bIsMassive = Options.bIsMassive;

                AddDependency(EditorGameNode.StaticGetFullName(HostPlatform, GameProj));
                // add an arc to prevent cooks from running until promotable is labeled
                if (Options.bIsPromotable)
                {
                    if (Options.bSeparateGamePromotion)
                    {
                        AddPseudodependency(GameLabelPromotableNode.StaticGetFullName(GameProj, false));
                    }
                    else
                    {
                        bIsShared = true;
                    }
                }
                else if (Options.bTestWithShared)
                {
                    bIsShared = true;
                }
                AddPseudodependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, GameProj, TargetPlatform));
            }
            else
            {
                bIsShared = true;
                AddPseudodependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, TargetPlatform));
            }
            if (bIsShared)
            {
                // add an arc to prevent cooks from running until promotable is labeled
                AddPseudodependency(WaitForTestShared.StaticGetFullName());
                AgentSharingGroup = "SharedCooks" + StaticGetHostPlatformSuffix(HostPlatform);

                // If the cook fails for the base engine, don't bother trying
                if (InGameProj.GameName != bp.Branch.BaseEngineProject.GameName && bp.HasNode(CookNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, CookPlatform)))
                {
                    AddPseudodependency(CookNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, CookPlatform));
                }

                // If the base cook platform fails, don't bother trying other ones
                string BaseCookedPlatform = Platform.Platforms[HostPlatform].GetCookPlatform(false, false, "");
                if (InGameProj.GameName == bp.Branch.BaseEngineProject.GameName && CookPlatform != BaseCookedPlatform &&
                    bp.HasNode(CookNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, BaseCookedPlatform)))
                {
                    AddPseudodependency(CookNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, BaseCookedPlatform));
                }
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
            if (CookedFiles.GetLength(0) < 1)
            {
                throw new AutomationException("CookedPath {1} did not produce any files.", CookedPath);
            }

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


    public class DDCNode : HostPlatformNode
    {
        BranchInfo.BranchUProject GameProj;
        UnrealTargetPlatform TargetPlatform;
        string CookPlatform;
        bool bIsMassive;

        public DDCNode(GUBP bp, UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, UnrealTargetPlatform InTargetPlatform, string InCookPlatform)
            : base(InHostPlatform)
        {
            GameProj = InGameProj;
            TargetPlatform = InTargetPlatform;
            CookPlatform = InCookPlatform;
            bIsMassive = false;
            AddDependency(RootEditorNode.StaticGetFullName(HostPlatform));
            if (bp.bOrthogonalizeEditorPlatforms)
            {
                if (TargetPlatform != HostPlatform && TargetPlatform != GUBP.GetAltHostPlatform(HostPlatform))
                {
                    AddDependency(EditorPlatformNode.StaticGetFullName(HostPlatform, TargetPlatform));
                }
            }

            bool bIsShared = false;
            // is this the "base game" or a non code project?
            if (InGameProj.GameName != bp.Branch.BaseEngineProject.GameName && GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                var Options = InGameProj.Options(HostPlatform);
                bIsMassive = Options.bIsMassive;

                AddDependency(EditorGameNode.StaticGetFullName(HostPlatform, GameProj));
                // add an arc to prevent DDCNode from running until promotable is labeled
                if (Options.bIsPromotable)
                {
                    if (Options.bSeparateGamePromotion)
                    {
                       // AddPseudodependency(GameLabelPromotableNode.StaticGetFullName(GameProj, false));
                    }
                    else
                    {
                        bIsShared = true;
                    }
                }
                else if (Options.bTestWithShared)
                {
                    bIsShared = true;
                }
                //AddPseudodependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, GameProj, TargetPlatform));
            }
            else
            {
                bIsShared = true;
                //AddPseudodependency(GamePlatformMonolithicsNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, TargetPlatform));
            }
            if (bIsShared)
            {
                // add an arc to prevent cooks from running until promotable is labeled
                //AddPseudodependency(WaitForTestShared.StaticGetFullName());
                //AgentSharingGroup = "SharedCooks" + StaticGetHostPlatformSuffix(HostPlatform);

                // If the cook fails for the base engine, don't bother trying
                if (InGameProj.GameName != bp.Branch.BaseEngineProject.GameName && bp.HasNode(DDCNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, CookPlatform)))
                {
                    //AddPseudodependency(DDCNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, CookPlatform));
                }

                // If the base cook platform fails, don't bother trying other ones
                string BaseCookedPlatform = Platform.Platforms[HostPlatform].GetCookPlatform(false, false, "");
                if (InGameProj.GameName == bp.Branch.BaseEngineProject.GameName && CookPlatform != BaseCookedPlatform &&
                    bp.HasNode(DDCNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, BaseCookedPlatform)))
                {
                    //AddPseudodependency(DDCNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, BaseCookedPlatform));
                }
            }
        }
        public static string StaticGetFullName(UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, string InCookPlatform)
        {
            return InGameProj.GameName + "_" + InCookPlatform + "_DDC" + StaticGetHostPlatformSuffix(InHostPlatform);
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
            return base.Priority() + 10.0f;
        }
        public override int AgentMemoryRequirement()
        {
            return bIsMassive ? 32 : 0;
        }
        public override int TimeoutInMinutes()
        {
            return bIsMassive ? 240 : 90;
        }

        public override void DoBuild(GUBP bp)
        {
            if (HostPlatform == UnrealTargetPlatform.Mac)
            {
                // not sure if we need something here or if the cook commandlet will automatically convert the exe name
            }
            CommandUtils.DDCCommandlet(GameProj.FilePath, "UE4Editor-Cmd.exe", null, CookPlatform, "-fill");

            BuildProducts = new List<string>();
            SaveRecordOfSuccessAndAddToBuildProducts();
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

            // put these in the right agent group, even though they aren't exposed to EC to sort right.
            if (InGameProj.GameName != bp.Branch.BaseEngineProject.GameName && GameProj.Properties.Targets.ContainsKey(TargetRules.TargetType.Editor))
            {
                var Options = InGameProj.Options(HostPlatform);
                if ((Options.bIsPromotable || Options.bTestWithShared) && !Options.bSeparateGamePromotion)
                {
                    AgentSharingGroup = "SharedCooks" + StaticGetHostPlatformSuffix(HostPlatform);
                }
            }
            else
            {
                AgentSharingGroup = "SharedCooks" + StaticGetHostPlatformSuffix(HostPlatform);
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
            string ProjectArg = "";
            if (!String.IsNullOrEmpty(GameProj.FilePath))
            {
                ProjectArg = " -project=\"" + GameProj.FilePath + "\"";
            }
            string Args = String.Format("BuildCookRun{0} -SkipBuild -SkipCook -Stage -Pak -Package -NoSubmit", ProjectArg);

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
                    Agenda.AddTargets(new string[] { Target.TargetName }, HostPlatform, UnrealTargetConfiguration.Development);
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
        float ECPriority;

        public UATTestNode(GUBP bp, UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, string InTestName, string InUATCommandLine, string InAgentSharingGroup, bool InDependsOnEditor = true, List<UnrealTargetPlatform> InDependsOnCooked = null, float InECPriority = 0.0f)
            : base(InHostPlatform)
        {
            AgentSharingGroup = InAgentSharingGroup;
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
            }

            var Options = InGameProj.Options(HostPlatform);

            // defer tests until later
            if (Options.bSeparateGamePromotion)
            {
                AddPseudodependency(GameLabelPromotableNode.StaticGetFullName(GameProj, false));
            }
            else
            {
                AddPseudodependency(WaitForTestShared.StaticGetFullName());
                // If the same test fails for the base engine, don't bother trying
                if (InGameProj.GameName != bp.Branch.BaseEngineProject.GameName)
                {
                    if (bp.HasNode(UATTestNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, TestName)))
                    {
                        AddPseudodependency(UATTestNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, InTestName));
                    }
                    else
                    {
                        bool bFoundACook = false;
                        foreach (var Plat in DependsOnCooked)
                        {
                            var PlatTestName = "CookedGameTest_"  + Plat.ToString();
                            if (bp.HasNode(UATTestNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, PlatTestName)))
                            {
                                AddPseudodependency(UATTestNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, PlatTestName));
                                bFoundACook = true;
                            }
                        }

                        if (!bFoundACook && bp.HasNode(UATTestNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, "EditorTest")))
                        {
                            AddPseudodependency(UATTestNode.StaticGetFullName(HostPlatform, bp.Branch.BaseEngineProject, "EditorTest"));
                        }

                    }
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
        float ECPriority;

        public GameAggregateNode(GUBP bp, UnrealTargetPlatform InHostPlatform, BranchInfo.BranchUProject InGameProj, string InAggregateName, List<string> Dependencies, float InECPriority = 100.0f)
            : base(InHostPlatform)
        {
            GameProj = InGameProj;
            AggregateName = InAggregateName;
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

    public bool HasNode(string Node)
    {
        return GUBPNodes.ContainsKey(Node);
    }

    public GUBPNode FindNode(string Node)
    {
        return GUBPNodes[Node];
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


    public int ComputeDependentCISFrequencyQuantumShift(string NodeToDo)
    {
        int Result = GUBPNodes[NodeToDo].ComputedDependentCISFrequencyQuantumShift;
        if (Result < 0)
        {
            Result = GUBPNodes[NodeToDo].CISFrequencyQuantumShift(this);
            foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfDependencies)
            {
                Result = Math.Max(ComputeDependentCISFrequencyQuantumShift(Dep), Result);
            }
            foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfPseudosependencies)
            {
                Result = Math.Max(ComputeDependentCISFrequencyQuantumShift(Dep), Result);
            }
            if (Result < 0)
            {
                throw new AutomationException("Failed to compute shift.");
            }
            GUBPNodes[NodeToDo].ComputedDependentCISFrequencyQuantumShift = Result;
        }
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

    List<ChangeRecord> GetChanges(int LastOutputForChanges, int TopCL, int LastGreen)
    {
        var Result = new List<ChangeRecord>();
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
                            Result.Add(Record);
                        }
                    }
                }
                else
                {
                    throw new AutomationException("Could not get changes; cmdline: p4 changes {0}", Cmd);
                }
            }
            else
            {
                throw new AutomationException("That CL looks pretty far off {0}", LastOutputForChanges);
            }
        }
        return Result;
    }

    int PrintChanges(int LastOutputForChanges, int TopCL, int LastGreen)
    {
        var ChangeRecords = GetChanges(LastOutputForChanges, TopCL, LastGreen);
        foreach (var Record in ChangeRecords)
        {
            Log("         {0} {1} {2}", Record.CL, Record.UserEmail, Record.Summary);
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
        bool ECProc = bp.ParseParam("ShowECProc");
        bool ECOnly = bp.ParseParam("ShowECOnly");
        bool bShowTriggers = true;
        string LastControllingTrigger = "";
        string LastAgentGroup = "";
        foreach (var NodeToDo in Nodes)
        {
            if (ECOnly && !GUBPNodes[NodeToDo].RunInEC())
            {
                continue;
            }
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
            if (GUBPNodes[NodeToDo].AgentSharingGroup != LastAgentGroup && GUBPNodes[NodeToDo].AgentSharingGroup != "")
            {
                Log("    Agent Group: {0}", GUBPNodes[NodeToDo].AgentSharingGroup);
            }
            LastAgentGroup = GUBPNodes[NodeToDo].AgentSharingGroup;

            string Agent = GUBPNodes[NodeToDo].ECAgentString();
            if (Agent != "")
            {
                Agent = "[" + Agent + "]";
            }
            string FrequencyString = "";
            int Quantum = GUBPNodes[NodeToDo].DependentCISFrequencyQuantumShift();
            if (Quantum > 0)
            {
                int Minutes = 20 * (1 << Quantum);
                if (Minutes < 60)
                {
                    FrequencyString = string.Format(" ({0}m)", Minutes);
                }
                else
                {
                    FrequencyString = string.Format(" ({0}h{1}m)", Minutes / 60, Minutes % 60);
                }
            }
            Log("      {0}{1}{2}{3}{4}{5}{6} {7}  {8}",
                (LastAgentGroup != "" ? "  " : ""),
                NodeToDo,
                FrequencyString,
                NodeIsAlreadyComplete(NodeToDo, LocalOnly) ? " - (Completed)" : "",
                GUBPNodes[NodeToDo].TriggerNode() ? " - (TriggerNode)" : "",
                GUBPNodes[NodeToDo].IsSticky() ? " - (Sticky)" : "",
                Agent,
                EMails,
                ECProc ? GUBPNodes[NodeToDo].ECProcedure() : ""
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
                    Log("           {0}", Dep);
                }
                foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfPseudosependencies)
                {
                    Log("           {0} (pseudo)", Dep);
                }
            }
            if (bShowECDependencies)
            {
                foreach (var Dep in GetECDependencies(NodeToDo))
                {
                    Log("           {0}", Dep);
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
                    if (GUBP.bNoIOSOnPC && Plat == UnrealTargetPlatform.IOS && HostPlatform == UnrealTargetPlatform.Win64)
                    {
                        continue;
                    }

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
    [Help("TimeIndex=", "An integer used to determine subsets to run based on DependentCISFrequencyQuantumShift")]
    [Help("Node=", "Nodes to process, -node=Node1+Node2+Node3, if no nodes or games are specified, defaults to all nodes.")]
    [Help("SetupNode=", "Like -Node, but only applies with CommanderJobSetupOnly")]
    [Help("RelatedToNode=", "Nodes to process, -RelatedToNode=Node1+Node2+Node3, use all nodes that either depend on these nodes or these nodes depend on them.")]
    [Help("SetupRelatedToNode=", "Like -RelatedToNode, but only applies with CommanderJobSetupOnly")]
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
    [Help("EmailOnly", "Only emails the folks given in the argument.")]
    [Help("AddEmails", "Add these space delimited emails too all email lists.")]
    [Help("NoShowDependencies", "Don't show node dependencies.")]
    [Help("ShowECDependencies", "Show EC node dependencies instead.")]
    [Help("ShowECProc", "Show EC proc names.")]
    [Help("BuildRocket", "Build in rocket mode.")]
    [Help("ShowECOnly", "Only show EC nodes.")]

    public override void ExecuteBuild()
    {
        Log("************************* GUBP");

        HostPlatforms = new List<UnrealTargetPlatform>();
        if (!ParseParam("OnlyMac"))
        {
            HostPlatforms.Add(UnrealTargetPlatform.Win64);
        }

        if (ParseParam("WithMac") || HostPlatforms.Count == 0)
        {
            HostPlatforms.Add(UnrealTargetPlatform.Mac);
        }
        bBuildRocket = ParseParam("BuildRocket");

        StoreName = ParseParamValue("Store");
        string StoreSuffix = ParseParamValue("StoreSuffix", "");
        if (bBuildRocket)
        {
            StoreSuffix = StoreSuffix + "Rocket";
        }
        CL = ParseParamInt("CL", 0);
        bool bCleanLocalTempStorage = ParseParam("CleanLocal");
        bool bChanges = ParseParam("Changes") || ParseParam("AllChanges");
        bool bHistory = ParseParam("History") || bChanges;
        bool bListOnly = ParseParam("ListOnly") || bHistory;
        bool bSkipTriggers = ParseParam("SkipTriggers");
        bFake = ParseParam("fake");
        bool bFakeEC = ParseParam("FakeEC");
        int TimeIndex = ParseParamInt("TimeIndex", 0);
        bHackRunIOSCompilesOnMac = !ParseParam("IOSOnRPCUtility") && !HostPlatforms.Contains(UnrealTargetPlatform.Mac);

        bNoIOSOnPC = HostPlatforms.Contains(UnrealTargetPlatform.Mac);

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
                StoreName = "GUBP-" + P4Env.BuildRootEscaped + "-CL-" + CLString;
                bSaveSharedTempStorage = CommandUtils.IsBuildMachine;
                LocalOnly = false;
            }
            else
            {
                StoreName = "TempLocal";
                bSaveSharedTempStorage = false;
            }
        }
        StoreName = StoreName + "-" + StoreSuffix;
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
        foreach (var HostPlatform in HostPlatforms)
        {
            Log("*************************   HostPlatform:		            {0}", HostPlatform.ToString()); 
        }
        Log("************************* StoreName:		                {0}", StoreName.ToString());
        Log("************************* bCleanLocalTempStorage:		    {0}", bCleanLocalTempStorage);
        Log("************************* bSkipTriggers:		            {0}", bSkipTriggers);
        Log("************************* bSaveSharedTempStorage:		    {0}", bSaveSharedTempStorage);
        Log("************************* bSignBuildProducts:		        {0}", bSignBuildProducts);
        Log("************************* bFake:           		        {0}", bFake);
        Log("************************* bFakeEC:           		        {0}", bFakeEC);
        Log("************************* bHistory:           		        {0}", bHistory);
        Log("************************* TimeIndex:           		    {0}", TimeIndex);
        Log("************************* bBuildRocket:           		    {0}", bBuildRocket);


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
        if (bBuildRocket)
        {
            var FilteredActivePlatforms = new List<UnrealTargetPlatform>();
            foreach (var Plat in ActivePlatforms)
            {
                if (Plat != UnrealTargetPlatform.HTML5 && 
                    Plat != UnrealTargetPlatform.Linux &&
                    Plat != UnrealTargetPlatform.PS4 &&
                    Plat != UnrealTargetPlatform.WinRT &&
                    Plat != UnrealTargetPlatform.WinRT_ARM &&
                    Plat != UnrealTargetPlatform.XboxOne
                    )
                {
                    FilteredActivePlatforms.Add(Plat);
                }
            }
            ActivePlatforms = FilteredActivePlatforms;
        }
        foreach (var Plat in ActivePlatforms)
        {
            Log("Active Platform: {0}", Plat.ToString());
        }

        if (HostPlatforms.Count == 2)
        {
            // make sure each project is set up with the right assumptions on monolithics that prefer a platform.
            foreach (var CodeProj in Branch.CodeProjects)
            {
                var OptionsMac = CodeProj.Options(UnrealTargetPlatform.Mac);
                var OptionsPC = CodeProj.Options(UnrealTargetPlatform.Win64);

                var MacMonos = GetMonolithicPlatformsForUProject(UnrealTargetPlatform.Mac, CodeProj, false);
                var PCMonos = GetMonolithicPlatformsForUProject(UnrealTargetPlatform.Win64, CodeProj, false);

                if (!OptionsMac.bIsPromotable && OptionsPC.bIsPromotable && 
                    (MacMonos.Contains(UnrealTargetPlatform.IOS) || PCMonos.Contains(UnrealTargetPlatform.IOS)))
                {
                    throw new AutomationException("Project {0} is promotable for PC, not promotable for Mac and uses IOS monothics. Since Mac is the preferred platform for IOS, please add Mac as a promotable platform.", CodeProj.GameName);
                }
                if (OptionsMac.bIsPromotable && !OptionsPC.bIsPromotable &&
                    (MacMonos.Contains(UnrealTargetPlatform.Android) || PCMonos.Contains(UnrealTargetPlatform.Android)))
                {
                    throw new AutomationException("Project {0} is not promotable for PC, promotable for Mac and uses Android monothics. Since PC is the preferred platform for Android, please add PC as a promotable platform.", CodeProj.GameName);
                }
            }
        }


        AddNode(new VersionFilesNode());


        if (bHackRunIOSCompilesOnMac)
        {
            AddNode(new ToolsForCompileNode(UnrealTargetPlatform.Mac));
        }
        foreach (var HostPlatform in HostPlatforms)
        {
            AddNode(new ToolsForCompileNode(HostPlatform));
            AddNode(new RootEditorNode(HostPlatform));
            AddNode(new ToolsNode(HostPlatform));            
			AddNode(new InternalToolsNode(HostPlatform));
            AddNode(new EditorAndToolsNode(HostPlatform));
            AddNode(new NonUnityTestNode(HostPlatform));

            if (bOrthogonalizeEditorPlatforms)
            {
                foreach (var Plat in ActivePlatforms)
                {
                    if (Plat != HostPlatform && Plat != GetAltHostPlatform(HostPlatform))
                    {
                        if (Platform.Platforms[HostPlatform].CanHostPlatform(Plat))
                        {
                            AddNode(new EditorPlatformNode(HostPlatform, Plat));
                        }
                    }
                }
            }

            bool DoASharedPromotable = false;

            int NumShared = 0;
            foreach (var CodeProj in Branch.CodeProjects)
            {
                var Options = CodeProj.Options(HostPlatform);

                if (Options.bIsPromotable && !Options.bSeparateGamePromotion)
                {
                    NumShared++;
                }
            }
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

            if (!DoASharedPromotable && bBuildRocket)
            {
                throw new AutomationException("we were asked to make a rocket build, but this branch does not have a shared promotable.");
            }

            if (DoASharedPromotable)
            {
                var AgentSharingGroup = "Shared_EditorTests" + HostPlatformNode.StaticGetHostPlatformSuffix(HostPlatform);

                Dictionary<string, List<UnrealTargetPlatform>> NonCodeProjectNames;
                {
                    var Target = Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor];
                    NonCodeProjectNames = Target.Rules.GUBP_NonCodeProjects_BaseEditorTypeOnly(HostPlatform);

                    foreach (var Codeless in NonCodeProjectNames)
                    {
                        var Proj = Branch.FindGame(Codeless.Key);
                        if (Proj == null)
                        {
                            Log(System.Diagnostics.TraceEventType.Warning, "{0} was listed as a codeless project by GUBP_NonCodeProjects_BaseEditorTypeOnly, however it does not exist in this branch.", Codeless.Key);
                        }
                        else if (Proj.Properties.bIsCodeBasedProject)
                        {
                            throw new AutomationException("{0} was listed as a codeless project by GUBP_NonCodeProjects_BaseEditorTypeOnly, however it is a code based project.", Codeless.Key);
                        }
                    }

                    var Options = Branch.BaseEngineProject.Options(HostPlatform);

                    if (!Options.bIsPromotable || Options.bSeparateGamePromotion)
                    {
                        throw new AutomationException("We assume that if we have shared promotable, the base engine is in it.");
                    }
                }
                {
                    var EditorTests = Branch.BaseEngineProject.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetEditorTests_EditorTypeOnly(HostPlatform);
                    var EditorTestNodes = new List<string>();
                    foreach (var Test in EditorTests)
                    {
                        EditorTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, Branch.BaseEngineProject, Test.Key, Test.Value, AgentSharingGroup)));
                        foreach (var NonCodeProject in Branch.NonCodeProjects)
                        {
                            if (!NonCodeProjectNames.ContainsKey(NonCodeProject.GameName))
                            {
                                continue;
                            }
                            if (HostPlatform == UnrealTargetPlatform.Mac) continue; //temp hack till mac automated testing works
                            EditorTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, NonCodeProject, Test.Key, Test.Value, AgentSharingGroup)));
                        }
                    }
                    if (EditorTestNodes.Count > 0)
                    {
                        AddNode(new GameAggregateNode(this, HostPlatform, Branch.BaseEngineProject, "AllEditorTests", EditorTestNodes, 0.0f));
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
                            if (!Platform.Platforms[HostPlatform].CanHostPlatform(Plat))
                            {
                                throw new AutomationException("Project {0} asked for platform {1} with host {2}, but the host platform cannot build that platform.", Branch.BaseEngineProject.GameName, Plat.ToString(), HostPlatform.ToString());
                            }
                            if (bNoIOSOnPC && Plat == UnrealTargetPlatform.IOS && HostPlatform == UnrealTargetPlatform.Win64)
                            {
                                continue;
                            }

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

                var CookedAgentSharingGroup = "Shared_CookedTests" + HostPlatformNode.StaticGetHostPlatformSuffix(HostPlatform);

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
                            if (!Platform.Platforms[HostPlatform].CanHostPlatform(Plat))
                            {
                                throw new AutomationException("Project {0} asked for platform {1} with host {2}, but the host platform cannot build that platform.", Branch.BaseEngineProject.GameName, Plat.ToString(), HostPlatform.ToString());
                            }
                            if (bNoIOSOnPC && Plat == UnrealTargetPlatform.IOS && HostPlatform == UnrealTargetPlatform.Win64)
                            {
                                continue;
                            }

                            if (ActivePlatforms.Contains(Plat))
                            {
                                string CookedPlatform = Platform.Platforms[Plat].GetCookPlatform(Kind == TargetRules.TargetType.Server, Kind == TargetRules.TargetType.Client, "");
                                if (!GUBPNodes.ContainsKey(CookNode.StaticGetFullName(HostPlatform, Branch.BaseEngineProject, CookedPlatform)))
                                {
                                    GameCookNodes.Add(AddNode(new CookNode(this, HostPlatform, Branch.BaseEngineProject, Plat, CookedPlatform)));
                                }
                                if (!GUBPNodes.ContainsKey(GamePlatformCookedAndCompiledNode.StaticGetFullName(HostPlatform, Branch.BaseEngineProject, Plat)))
                                {
                                    AddNode(new GamePlatformCookedAndCompiledNode(this, HostPlatform, Branch.BaseEngineProject, Plat, false));
                                }
                                var GameTests = Target.Rules.GUBP_GetGameTests_MonolithicOnly(HostPlatform, GetAltHostPlatform(HostPlatform), Plat);
                                var RequiredPlatforms = new List<UnrealTargetPlatform> { Plat };
                                {
                                    var ThisMonoGameTestNodes = new List<string>();
                                    foreach (var Test in GameTests)
                                    {
                                        var TestName = Test.Key + "_" + Plat.ToString();
                                        ThisMonoGameTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, Branch.BaseEngineProject, TestName, Test.Value, CookedAgentSharingGroup, false, RequiredPlatforms)));
                                    }
                                    if (ThisMonoGameTestNodes.Count > 0)
                                    {
                                        GameTestNodes.Add(AddNode(new GameAggregateNode(this, HostPlatform, Branch.BaseEngineProject, "CookedTests_" + Plat.ToString() + "_" + Kind.ToString() + HostPlatformNode.StaticGetHostPlatformSuffix(HostPlatform), ThisMonoGameTestNodes, 0.0f)));
                                    }
                                }

                                foreach (var NonCodeProject in Branch.NonCodeProjects)
                                {
                                    if (!NonCodeProjectNames.ContainsKey(NonCodeProject.GameName) || !NonCodeProjectNames[NonCodeProject.GameName].Contains(Plat))
                                    {
                                        continue;
                                    }
                                    if (!GUBPNodes.ContainsKey(CookNode.StaticGetFullName(HostPlatform, NonCodeProject, CookedPlatform)))
                                    {
                                        GameCookNodes.Add(AddNode(new CookNode(this, HostPlatform, NonCodeProject, Plat, CookedPlatform)));
                                    }
                                    if (!GUBPNodes.ContainsKey(GamePlatformCookedAndCompiledNode.StaticGetFullName(HostPlatform, NonCodeProject, Plat)))
                                    {
                                        AddNode(new GamePlatformCookedAndCompiledNode(this, HostPlatform, NonCodeProject, Plat, false));
                                    }

                                    if (HostPlatform == UnrealTargetPlatform.Mac) continue; //temp hack till mac automated testing works
                                    var ThisMonoGameTestNodes = new List<string>();
                                    foreach (var Test in GameTests)
                                    {
                                        var TestName = Test.Key + "_" + Plat.ToString();
                                        ThisMonoGameTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, NonCodeProject, TestName, Test.Value, CookedAgentSharingGroup, false, RequiredPlatforms)));
                                    }
                                    if (ThisMonoGameTestNodes.Count > 0)
                                    {
                                        GameTestNodes.Add(AddNode(new GameAggregateNode(this, HostPlatform, NonCodeProject, "CookedTests_" + Plat.ToString() + "_" + Kind.ToString() + HostPlatformNode.StaticGetHostPlatformSuffix(HostPlatform), ThisMonoGameTestNodes, 0.0f)));
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
                    AddNode(new GameAggregateNode(this, HostPlatform, Branch.BaseEngineProject, "AllCookedTests", GameTestNodes));
                }
            }

            foreach (var CodeProj in Branch.CodeProjects)
            {
                var Options = CodeProj.Options(HostPlatform);

                if (!Options.bIsPromotable && !Options.bTestWithShared)
                {
                    continue; // we skip things that aren't promotable and aren't tested
                }
                var AgentShareName = CodeProj.GameName;
                if (!Options.bSeparateGamePromotion)
                {
                    AgentShareName = "Shared";
                }

                AddNode(new EditorGameNode(this, HostPlatform, CodeProj));
if (HostPlatform != UnrealTargetPlatform.Mac) //temp hack till mac automated testing works
                {
                    var EditorTests = CodeProj.Properties.Targets[TargetRules.TargetType.Editor].Rules.GUBP_GetEditorTests_EditorTypeOnly(HostPlatform);
                    var EditorTestNodes = new List<string>();
                    string AgentSharingGroup = "";
                    if (EditorTests.Count > 1)
                    {
                        AgentSharingGroup = AgentShareName + "_EditorTests" + HostPlatformNode.StaticGetHostPlatformSuffix(HostPlatform);
                    }
                    foreach (var Test in EditorTests)
                    {
                        EditorTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, CodeProj, Test.Key, Test.Value, AgentSharingGroup)));
                    }
                    if (EditorTestNodes.Count > 0)
                    {
                        AddNode(new GameAggregateNode(this, HostPlatform, CodeProj, "AllEditorTests", EditorTestNodes, 0.0f));
                    }
                }

                var CookedAgentSharingGroup = AgentShareName + "_CookedTests" + HostPlatformNode.StaticGetHostPlatformSuffix(HostPlatform);

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
                            if (!Platform.Platforms[HostPlatform].CanHostPlatform(Plat))
                            {
                                throw new AutomationException("Project {0} asked for platform {1} with host {2}, but the host platform cannot build that platform.", CodeProj.GameName, Plat.ToString(), HostPlatform.ToString());
                            }
                            if (bNoIOSOnPC && Plat == UnrealTargetPlatform.IOS && HostPlatform == UnrealTargetPlatform.Win64)
                            {
                                continue;
                            }
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
if (HostPlatform == UnrealTargetPlatform.Mac) continue; //temp hack till mac automated testing works
                                var GameTests = Target.Rules.GUBP_GetGameTests_MonolithicOnly(HostPlatform, GetAltHostPlatform(HostPlatform), Plat);
                                var RequiredPlatforms = new List<UnrealTargetPlatform> { Plat };
                                var ThisMonoGameTestNodes = new List<string>();

                                foreach (var Test in GameTests)
                                {
                                    var TestNodeName = Test.Key + "_" + Plat.ToString();
                                    ThisMonoGameTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, CodeProj, TestNodeName, Test.Value, CookedAgentSharingGroup, false, RequiredPlatforms)));
                                }
                                if (ThisMonoGameTestNodes.Count > 0)
                                {
                                    GameTestNodes.Add(AddNode(new GameAggregateNode(this, HostPlatform, CodeProj, "CookedTests_" + Plat.ToString() + "_" + Kind.ToString() + HostPlatformNode.StaticGetHostPlatformSuffix(HostPlatform), ThisMonoGameTestNodes, 0.0f)));
                                }
                            }
                        }
                    }
                }
                foreach (var ServerPlatform in ServerPlatforms)
                {
                    foreach (var GamePlatform in GamePlatforms)
                    {
if (HostPlatform == UnrealTargetPlatform.Mac) continue; //temp hack till mac automated testing works
                        var Target = CodeProj.Properties.Targets[TargetRules.TargetType.Game];
                        var ClientServerTests = Target.Rules.GUBP_GetClientServerTests_MonolithicOnly(HostPlatform, GetAltHostPlatform(HostPlatform), ServerPlatform, GamePlatform);
                        var RequiredPlatforms = new List<UnrealTargetPlatform> { ServerPlatform };
                        if (ServerPlatform != GamePlatform)
                        {
                            RequiredPlatforms.Add(GamePlatform);
                        }
                        foreach (var Test in ClientServerTests)
                        {
                            var TestNodeName = Test.Key + "_" + GamePlatform.ToString() + "_" + ServerPlatform.ToString();
                            GameTestNodes.Add(AddNode(new UATTestNode(this, HostPlatform, CodeProj, TestNodeName, Test.Value, CookedAgentSharingGroup, false, RequiredPlatforms)));
                        }
                    }
                }
                if (GameTestNodes.Count > 0)
                {
                    AddNode(new GameAggregateNode(this, HostPlatform, CodeProj, "AllCookedTests", GameTestNodes));
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

        foreach (var NodeToDo in GUBPNodes)
        {
            foreach (var Dep in GUBPNodes[NodeToDo.Key].FullNamesOfDependencies)
            {
                if (!GUBPNodes.ContainsKey(Dep))
                {
                    throw new AutomationException("Node {0} is not in the full graph. It is a dependency of {1}.", Dep, NodeToDo.Key);
                }
                if (Dep == NodeToDo.Key)
                {
                    throw new AutomationException("Node {0} has a self arc.", NodeToDo.Key);
                }
            }
            foreach (var Dep in GUBPNodes[NodeToDo.Key].FullNamesOfPseudosependencies)
            {
                if (!GUBPNodes.ContainsKey(Dep))
                {
                    throw new AutomationException("Node {0} is not in the full graph. It is a pseudodependency of {1}.", Dep, NodeToDo.Key);
                }
                if (Dep == NodeToDo.Key)
                {
                    throw new AutomationException("Node {0} has a self pseudoarc.", NodeToDo.Key);
                }
            }
        }
        foreach (var NodeToDo in GUBPNodes)
        {
            ComputeDependentCISFrequencyQuantumShift(NodeToDo.Key);
        }

        var NodesToDo = new HashSet<string>();

        if (bCleanLocalTempStorage)  // shared temp storage can never be wiped
        {
            DeleteLocalTempStorageManifests(CmdEnv);
        }
        Log("******* {0} GUBP Nodes", GUBPNodes.Count);
        foreach (var Node in GUBPNodes)
        {
            Log("  {0}", Node.Key); 
        }

        bool CommanderSetup = ParseParam("CommanderJobSetupOnly");
        bool bOnlyNode = false;
        bool bRelatedToNode = false;
        {
            string NodeSpec = ParseParamValue("Node");
            if (String.IsNullOrEmpty(NodeSpec))
            {
                NodeSpec = ParseParamValue("RelatedToNode");
                if (!String.IsNullOrEmpty(NodeSpec))
                {
                    bRelatedToNode = true;
                }
            }
            if (String.IsNullOrEmpty(NodeSpec) && CommanderSetup)
            {
                NodeSpec = ParseParamValue("SetupNode");
                if (String.IsNullOrEmpty(NodeSpec))
                {
                    NodeSpec = ParseParamValue("SetupRelatedToNode");
                    if (!String.IsNullOrEmpty(NodeSpec))
                    {
                        bRelatedToNode = true;
                    }
                }
            }
            if (String.IsNullOrEmpty(NodeSpec))
            {
                NodeSpec = ParseParamValue("OnlyNode");
                if (!String.IsNullOrEmpty(NodeSpec))
                {
                    bOnlyNode = true;
                }
            }
            if (String.IsNullOrEmpty(NodeSpec) && bBuildRocket)
            {
                // rocket is the shared promotable plus some other stuff and nothing else
                bRelatedToNode = true;
                NodeSpec = SharedAggregatePromotableNode.StaticGetFullName() + "+" + "Rocket_Aggregate";
            }
            if (!String.IsNullOrEmpty(NodeSpec))
            {
                if (NodeSpec.Equals("Noop", StringComparison.InvariantCultureIgnoreCase))
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
        else if (TimeIndex != 0)
        {
            Log("Check to make sure we didn't ask for nodes that will be culled by time index");
            foreach (var NodeToDo in NodesToDo)
            {
                if (TimeIndex % (1 << GUBPNodes[NodeToDo].DependentCISFrequencyQuantumShift()) != 0)
                {
                    throw new AutomationException("You asked specifically for node {0}, but it is culled by the time quantum: TimeIndex = {1}, DependentCISFrequencyQuantumShift = {2}.", NodeToDo, TimeIndex, GUBPNodes[NodeToDo].DependentCISFrequencyQuantumShift());
                }
            }
        }

        Log("Desired Nodes");
        foreach (var NodeToDo in NodesToDo)
        {
            Log("  {0}", NodeToDo);
        }
        // if we are doing related to, then find things that depend on the selected nodes
        if (bRelatedToNode)
        {
            bool bDoneWithDependencies = false;

            while (!bDoneWithDependencies)
            {
                bDoneWithDependencies = true;
                var Fringe = new HashSet<string>();
                foreach (var NodeToDo in GUBPNodes)
                {
                    if (!NodesToDo.Contains(NodeToDo.Key))
                    {
                        foreach (var Dep in GUBPNodes[NodeToDo.Key].FullNamesOfDependencies)
                        {
                            if (!GUBPNodes.ContainsKey(Dep))
                            {
                                throw new AutomationException("Node {0} is not in the graph. It is a dependency of {1}.", Dep, NodeToDo.Key);
                            }
                            if (NodesToDo.Contains(Dep))
                            {
                                Fringe.Add(NodeToDo.Key);
                                bDoneWithDependencies = false;
                            }
                        }
                        foreach (var Dep in GUBPNodes[NodeToDo.Key].FullNamesOfPseudosependencies)
                        {
                            if (!GUBPNodes.ContainsKey(Dep))
                            {
                                throw new AutomationException("Node {0} is not in the graph. It is a pseudodependency of {1}.", Dep, NodeToDo.Key);
                            }
                        }
                    }
                }
                NodesToDo.UnionWith(Fringe);
            }
        }

        // find things that our nodes depend on
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
        if (TimeIndex != 0)
        {
            Log("Culling based on time index");
            var NewNodesToDo = new HashSet<string>();
            foreach (var NodeToDo in NodesToDo)
            {
                if (TimeIndex % (1 << GUBPNodes[NodeToDo].DependentCISFrequencyQuantumShift()) == 0)
                {
                    Log("  Keeping {0}", NodeToDo);
                    NewNodesToDo.Add(NodeToDo);
                }
            }
            NodesToDo = NewNodesToDo;
        }

        string ExplicitTrigger = "";
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

        Log("******* Caching completion");
        GUBPNodesCompleted = new Dictionary<string, bool>();
        GUBPNodesControllingTrigger = new Dictionary<string, string>();
        GUBPNodesControllingTriggerDotName = new Dictionary<string, string>();
        GUBPNodesHistory = new Dictionary<string, NodeHistory>();

        foreach (var Node in NodesToDo)
        {
            Log("** {0}", Node);
            bool bComplete = NodeIsAlreadyComplete(Node, LocalOnly); // cache these now to avoid spam later
            GetControllingTriggerDotName(Node);
            if (!bComplete && CLString != "" && StoreName.Contains(CLString))
            {
                if (GUBPNodes[Node].RunInEC() && !GUBPNodes[Node].TriggerNode())
                {
                    string GameNameIfAny = GUBPNodes[Node].GameNameIfAnyForTempStorage();
                    string NodeStoreWildCard = StoreName.Replace(CLString, "*") + "-" + GUBPNodes[Node].GetFullName();
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
                        GUBPNodesHistory.Add(Node, History);
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
                                if (String.Compare(GUBPNodes[BestNode].AgentSharingGroup, GUBPNodes[NodeToDo].AgentSharingGroup) > 0) //sorted by agent sharing group
                                {
                                    bReady = false;
                                }
                                else if (String.Compare(GUBPNodes[BestNode].AgentSharingGroup, GUBPNodes[NodeToDo].AgentSharingGroup) == 0) //sorted by agent sharing group
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

        //check sorting
        {
            foreach (var NodeToDo in OrdereredToDo)
            {
                if (GUBPNodes[NodeToDo].TriggerNode() && (GUBPNodes[NodeToDo].IsSticky() || NodeIsAlreadyComplete(NodeToDo, LocalOnly))) // these sticky triggers are ok, everything is already completed anyway
                {
                    continue;
                }
                int MyIndex = OrdereredToDo.IndexOf(NodeToDo);
                foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfDependencies)
                {
                    int DepIndex = OrdereredToDo.IndexOf(Dep);
                    if (DepIndex >= MyIndex)
                    {
                        throw new AutomationException("Topological sort error, node {0} has a dependency of {1} which sorted after it.", NodeToDo, Dep);
                    }
                }
                foreach (var Dep in GUBPNodes[NodeToDo].FullNamesOfPseudosependencies)
                {
                    int DepIndex = OrdereredToDo.IndexOf(Dep);
                    if (DepIndex >= MyIndex)
                    {
                        throw new AutomationException("Topological sort error, node {0} has a pseduodependency of {1} which sorted after it.", NodeToDo, Dep);
                    }
                }
            }
        }

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
            var BranchForEmail = "";
            if (P4Enabled)
            {
                BranchForEmail = P4Env.BuildRootP4;
            }

            var AgentGroupChains = new Dictionary<string, List<string>>();
            foreach (var NodeToDo in OrdereredToDo)
            {
                if (GUBPNodes[NodeToDo].RunInEC() && !NodeIsAlreadyComplete(NodeToDo, LocalOnly)) // if something is already finished, we don't put it into EC  
                {
                    string MyAgentGroup = GUBPNodes[NodeToDo].AgentSharingGroup;
                    if (MyAgentGroup != "")
                    {
                        if (!AgentGroupChains.ContainsKey(MyAgentGroup))
                        {
                            AgentGroupChains.Add(MyAgentGroup, new List<string>{NodeToDo});
                        }
                        else
                        {
                            AgentGroupChains[MyAgentGroup].Add(NodeToDo);
                        }
                    }
                }
            }

            foreach (var NodeToDo in OrdereredToDo)
            {
                if (GUBPNodes[NodeToDo].RunInEC() && !NodeIsAlreadyComplete(NodeToDo, LocalOnly)) // if something is already finished, we don't put it into EC  
                {
                    {
                        string EMails = "";
                        string EmailOnly = ParseParamValue("EmailOnly");
                        if (bFake)
                        {
                            EMails = "kellan.carr[epic] gil.gribb[epic]";
                        }
                        else if (!String.IsNullOrEmpty(EmailOnly))
                        {
                            EMails = EmailOnly;
                        }
                        else
                        {
                            EMails = GetEMailListForNode(this, NodeToDo);
                        }
                        string AddEmails = ParseParamValue("AddEmails");
                        if (!String.IsNullOrEmpty(AddEmails))
                        {
                            EMails = GUBPNode.MergeSpaceStrings(AddEmails, EMails);
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
                        var History = GUBPNodesHistory[NodeToDo];

                        ECProps.Add(string.Format("LastGreen/{0}={1}", NodeToDo, History.LastSucceeded));
                        ECProps.Add(string.Format("RedsSince/{0}={1}", NodeToDo, History.FailedString));
                        ECProps.Add(string.Format("InProgress/{0}={1}", NodeToDo, History.InProgressString));

                        string EMails = "";

                        if (History.LastSucceeded > 0 && History.LastSucceeded < P4Env.Changelist)
                        {
                            var ChangeRecords = GetChanges(History.LastSucceeded, P4Env.Changelist, History.LastSucceeded);
                            int NumChanges = 0;
                            foreach (var Record in ChangeRecords)
                            {
                                EMails = GUBPNode.MergeSpaceStrings(EMails, Record.UserEmail);
                                if (++NumChanges > 50)
                                {
                                    EMails = "";
                                    break;
                                }
                            }
                        }
                        ECProps.Add(string.Format("FailCausers/{0}={1}", NodeToDo, EMails));
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
                            if (OrdereredToDo.IndexOf(Dep) > OrdereredToDo.IndexOf(NodeToDo))
                            {
                                throw new AutomationException("Topological sort error, node {0} has a dependency of {1} which sorted after it.", NodeToDo, Dep);
                            }
                            UncompletedEcDeps.Add(Dep);
                        }
                    }

                    string MyAgentGroup = GUBPNodes[NodeToDo].AgentSharingGroup;

                    if (UncompletedEcDeps.Count > 0)
                    {
                        //$[/javascript if(getProperty("$[/myWorkflow/ParentWorkflow]/GUBP_Game_$[/myWorkflow/GameName]_MonoPlatform1_Exists") != "1" || getProperty("$[/myWorkflow/ParentWorkflow]/$[$[/myWorkflow/ParentWorkflow]/GUBP_Game_$[/myWorkflow/GameName]_MonoPlatform1_Name]_EditorPlatform_Success") != "1") true; else false;]
                        {
                            PreCondition = "\"$[/javascript if(";

                            // these run "parallel", but we add preconditions to serialize them
                            if (MyAgentGroup != "")
                            {
                                var MyChain = AgentGroupChains[MyAgentGroup];
                                int MyIndex = MyChain.IndexOf(NodeToDo);
                                if (MyIndex  > 0)
                                {
                                    PreCondition = PreCondition + "getProperty('" + ParentPath + "/jobSteps[" + MyChain[MyIndex - 1] + "]/status') == 'completed'";
                                    if (UncompletedEcDeps.Count > 0)
                                    {
                                        PreCondition = PreCondition + " && ";
                                    }
                                }
                                else
                                {
                                    // to avoid idle agents, we might want to promote dependencies to the first one
                                }
                            }
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
#if false
                    if (GUBPNodes[NodeToDo].TimeoutInMinutes() > 0)
                    {
                        Args = Args + String.Format(" --timeLimitUnits minutes --timeLimit {0}", GUBPNodes[NodeToDo].TimeoutInMinutes());
                    }
#endif
                    if (Sticky && NodeToDo == LastSticky)
                    {
                        Args = Args + " --releaseMode release";
                    }
                    RunECTool(Args);
                    if (bFakeEC && 
                        !UnfinishedTriggers.Contains(NodeToDo) &&
                        (GUBPNodes[NodeToDo].ECProcedure().StartsWith("GUBP_UAT_Node") || GUBPNodes[NodeToDo].ECProcedure().StartsWith("GUBP_Mac_UAT_Node")) // other things we really can't test
                        ) // unfinished triggers are never run directly by EC, rather it does another job setup
                    {
                        string Arg = String.Format("gubp -Node={0} -FakeEC {1} {2} {3} {4} {5}", 
                            NodeToDo,
                            bFake ? "-Fake" : "" , 
                            ParseParam("AllPlatforms") ? "-AllPlatforms" : "",
                            ParseParam("UnfinishedTriggersFirst") ? "-UnfinishedTriggersFirst" : "",
                            ParseParam("UnfinishedTriggersParallel") ? "-UnfinishedTriggersParallel" : "",
                            ParseParam("WithMac") ? "-WithMac" : ""
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

                    if (MyAgentGroup != "")
                    {
                        var MyChain = AgentGroupChains[MyAgentGroup];
                        int MyIndex = MyChain.IndexOf(NodeToDo);
                        if (MyIndex == MyChain.Count - 1)
                        {
                            var RelPreCondition = "\"$[/javascript if(";
                            // this runs "parallel", but we a precondition to serialize it
                            RelPreCondition = RelPreCondition + "getProperty('" + ParentPath + "/jobSteps[" + NodeToDo + "]/status') == 'completed'";
                            RelPreCondition = RelPreCondition + ") true;]\"";
                            // we need to release the resource
                            string RelArgs = String.Format("{0} --subprocedure GUBP_Release_AgentShare --parallel 1 --jobStepName Release_{1} --actualParameter AgentSharingGroup={2} --releaseMode release --precondition {3}",
                                BaseArgs, MyAgentGroup, MyAgentGroup, RelPreCondition);

                            RunECTool(RelArgs);
                        }
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
                if (NodeToDo == VersionFilesNode.StaticGetFullName() && !IsBuildMachine)
                {
                    Log("***** NOT ****** Retrieving GUBP Node {0} from {1}; it is the version files.", GUBPNodes[NodeToDo].GetFullName(), NodeStoreName);
                    GUBPNodes[NodeToDo].BuildProducts = new List<string>();

                }
                else
                {
                    Log("***** Retrieving GUBP Node {0} from {1}", GUBPNodes[NodeToDo].GetFullName(), NodeStoreName);
                    GUBPNodes[NodeToDo].BuildProducts = RetrieveFromTempStorage(CmdEnv, NodeStoreName, GameNameIfAny, StorageRootIfAny);
                }
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
                        Log("End changes since last green");
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
