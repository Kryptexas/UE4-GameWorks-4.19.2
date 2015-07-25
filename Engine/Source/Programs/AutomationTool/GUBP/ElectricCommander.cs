using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;
using UnrealBuildTool;

namespace AutomationTool
{
	class ElectricCommander
	{
		BuildCommand Command;

		public ElectricCommander(BuildCommand InCommand)
		{
			Command = InCommand;
		}

		public string RunECTool(string Args, bool bQuiet = false)
		{
			if (Command.ParseParam("FakeEC"))
			{
				CommandUtils.LogWarning("***** Would have ran ectool {0}", Args);
				return "We didn't actually run ectool";
			}
			else
			{
				CommandUtils.ERunOptions Opts = CommandUtils.ERunOptions.Default;
				if (bQuiet)
				{
					Opts = (Opts & ~CommandUtils.ERunOptions.AllowSpew) | CommandUtils.ERunOptions.NoLoggingOfRunCommand;
				}
				return CommandUtils.RunAndLog("ectool", "--timeout 900 " + Args, Options: Opts);
			}
		}

		enum JobStepExclusiveMode
		{
			None,
			Call,
		}

		enum JobStepReleaseMode
		{
			Keep,
			Release,
		}

		enum JobStepNamePlacement
		{
			Default,
			BeforeParallel,
			BeforeProcedure,
		}

		[DebuggerDisplay("{Name}")]
		class JobStep
		{
			public string ParentPath;
			public string Name;
			public string SubProcedure;
			public bool Parallel;
			public string PreCondition;
			public string RunCondition;
			public JobStepExclusiveMode Exclusive;
			public string ResourceName;
			public List<Tuple<string, string>> Parameters;
			public JobStepReleaseMode ReleaseMode;
			public JobStepNamePlacement NamePlacement;

			public JobStep(string InParentPath, string InName, string InSubProcedure, bool InParallel, string InPreCondition, string InRunCondition, List<Tuple<string, string>> InParameters, JobStepReleaseMode InReleaseMode, JobStepExclusiveMode InExclusive = JobStepExclusiveMode.None, string InResourceName = null, JobStepNamePlacement InNamePlacement = JobStepNamePlacement.Default)
			{
				ParentPath = InParentPath;
				Name = InName;
				SubProcedure = InSubProcedure;
				Parallel = InParallel;
				PreCondition = InPreCondition;
				RunCondition = InRunCondition;
				Parameters = InParameters;
				ReleaseMode = InReleaseMode;
				Exclusive = InExclusive;
				ResourceName = InResourceName;
				NamePlacement = InNamePlacement;
			}

			public string GetCreationStatement()
			{
				StringBuilder Args = new StringBuilder();
				Args.Append("$batch->createJobStep({");
				Args.AppendFormat("parentPath => '{0}'", ParentPath);
				if (!String.IsNullOrEmpty(Name) && NamePlacement == JobStepNamePlacement.BeforeProcedure)
				{
					Args.AppendFormat(", jobStepName => '{0}'", Name);
				}
				if(!String.IsNullOrEmpty(SubProcedure))
				{
					Args.AppendFormat(", subprocedure => '{0}'", SubProcedure);
				}
				if (!String.IsNullOrEmpty(Name) && NamePlacement == JobStepNamePlacement.BeforeParallel)
				{
					Args.AppendFormat(", jobStepName => '{0}'", Name);
				}
				Args.AppendFormat(", parallel => '{0}'", Parallel ? 1 : 0);
				if (!String.IsNullOrEmpty(Name) && NamePlacement == JobStepNamePlacement.Default)
				{
					Args.AppendFormat(", jobStepName => '{0}'", Name);
				}
				if (Exclusive != JobStepExclusiveMode.None)
				{
					Args.AppendFormat(", exclusiveMode => '{0}'", Exclusive.ToString().ToLower());
				}
				if (!String.IsNullOrEmpty(ResourceName))
				{
					Args.AppendFormat(", resourceName => '{0}'", ResourceName);
				}
				if (Parameters != null)
				{
					Args.AppendFormat(", actualParameter => [{0}]", String.Join(", ", Parameters.Select(x => String.Format("{{actualParameterName => '{0}', value => '{1}'}}", x.Item1, x.Item2))));
				}
				if (!String.IsNullOrEmpty(PreCondition))
				{
					Args.AppendFormat(", precondition => {0}", PreCondition);
				}
				if (!String.IsNullOrEmpty(RunCondition))
				{
					Args.AppendFormat(", condition => {0}", RunCondition);
				}
				if (ReleaseMode == JobStepReleaseMode.Release)
				{
					Args.Append(", releaseMode => 'release'");
				}
				Args.Append("});");
				return Args.ToString();
			}

			public string GetCompletedCondition()
			{
				return String.Format("\"\\$\" . \"[/javascript if(getProperty('{0}/jobSteps[{1}]/status') == 'completed') true;]\"", ParentPath, Name);
			}
		}

		static void WriteECPerl(List<JobStep> Steps)
		{
			List<string> Lines = new List<string>();
			Lines.Add("use strict;");
			Lines.Add("use diagnostics;");
			Lines.Add("use ElectricCommander();");
			Lines.Add("my $ec = new ElectricCommander;");
			Lines.Add("$ec->setTimeout(600);");
			Lines.Add("my $batch = $ec->newBatch(\"serial\");");
			foreach (JobStep Step in Steps)
			{
				Lines.Add(Step.GetCreationStatement());
			}
			Lines.Add("$batch->submit();");

			string ECPerlFile = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LogFolder, "jobsteps.pl");
			CommandUtils.WriteAllLines_NoExceptions(ECPerlFile, Lines.ToArray());
		}

		string GetPropertyFromStep(string PropertyPath)
		{
			string Property = "";
			Property = RunECTool("getProperty \"" + PropertyPath + "\"");
			Property = Property.TrimEnd('\r', '\n');
			return Property;
		}

		static string GetJobStepPath(BuildNode Dep)
		{
			if (Dep.AgentSharingGroup == "")
			{
				return "jobSteps[" + Dep.Name + "]";
			}
			else
			{
				return "jobSteps[" + Dep.AgentSharingGroup + "]/jobSteps[" + Dep.Name + "]";
			}
		}
		static string GetJobStep(string ParentPath, BuildNode Dep)
		{
			return ParentPath + "/" + GetJobStepPath(Dep);
		}

		List<string> GetECPropsForNode(BuildNode NodeToDo)
		{
			List<string> ECProps = new List<string>();
			ECProps.Add("FailEmails/" + NodeToDo.Name + "=" + String.Join(" ", NodeToDo.RecipientsForFailureEmails));
	
			string AgentReq = NodeToDo.AgentRequirements;
			if(Command.ParseParamValue("AgentOverride") != "" && !NodeToDo.Name.Contains("OnMac"))
			{
				AgentReq = Command.ParseParamValue("AgentOverride");
			}
			ECProps.Add(string.Format("AgentRequirementString/{0}={1}", NodeToDo.Name, AgentReq));
			ECProps.Add(string.Format("RequiredMemory/{0}={1}", NodeToDo.Name, NodeToDo.AgentMemoryRequirement));
			ECProps.Add(string.Format("Timeouts/{0}={1}", NodeToDo.Name, NodeToDo.TimeoutInMinutes));
			ECProps.Add(string.Format("JobStepPath/{0}={1}", NodeToDo.Name, GetJobStepPath(NodeToDo)));
		
			return ECProps;
		}
	
		public void UpdateECProps(BuildNode NodeToDo)
		{
			try
			{
				CommandUtils.LogConsole("Updating node props for node {0}", NodeToDo.Name);
				RunECTool(String.Format("setProperty \"/myWorkflow/FailEmails/{0}\" \"{1}\"", NodeToDo.Name, String.Join(" ", NodeToDo.RecipientsForFailureEmails)), true);
			}
			catch (Exception Ex)
			{
				CommandUtils.LogWarning("Failed to UpdateECProps.");
				CommandUtils.LogWarning(LogUtils.FormatException(Ex));
			}
		}
		public void UpdateECBuildTime(BuildNode NodeToDo, double BuildDuration)
		{
			try
			{
				CommandUtils.LogConsole("Updating duration prop for node {0}", NodeToDo.Name);
				RunECTool(String.Format("setProperty \"/myWorkflow/NodeDuration/{0}\" \"{1}\"", NodeToDo.Name, BuildDuration.ToString()));
				RunECTool(String.Format("setProperty \"/myJobStep/NodeDuration\" \"{0}\"", BuildDuration.ToString()));
			}
			catch (Exception Ex)
			{
				CommandUtils.LogWarning("Failed to UpdateECBuildTime.");
				CommandUtils.LogWarning(LogUtils.FormatException(Ex));
			}
		}

		public void SaveStatus(BuildNode NodeToDo, string Suffix, string NodeStoreName, bool bSaveSharedTempStorage, string GameNameIfAny, string JobStepIDForFailure = null)
		{
			string Contents = "Just a status record: " + Suffix;
			if (!String.IsNullOrEmpty(JobStepIDForFailure) && CommandUtils.IsBuildMachine)
			{
				try
				{
					Contents = RunECTool(String.Format("getProperties --jobStepId {0} --recurse 1", JobStepIDForFailure), true);
				}
				catch (Exception Ex)
				{
					CommandUtils.LogWarning("Failed to get properties for jobstep to save them.");
					CommandUtils.LogWarning(LogUtils.FormatException(Ex));
				}
			}
			string RecordOfSuccess = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LocalRoot, "Engine", "Saved", "Logs", NodeToDo.Name + Suffix +".log");
			CommandUtils.CreateDirectory(Path.GetDirectoryName(RecordOfSuccess));
			CommandUtils.WriteAllText(RecordOfSuccess, Contents);		
			TempStorage.StoreToTempStorage(CommandUtils.CmdEnv, NodeStoreName + Suffix, new List<string> { RecordOfSuccess }, !bSaveSharedTempStorage, GameNameIfAny);		
		}

		public void UpdateEmailProperties(BuildNode NodeToDo, int LastSucceededCL, string FailedString, string FailCauserEMails, string EMailNote, bool SendSuccessForGreenAfterRed)
		{
			RunECTool(String.Format("setProperty \"/myWorkflow/LastGreen/{0}\" \"{1}\"", NodeToDo.Name, LastSucceededCL), true);
			RunECTool(String.Format("setProperty \"/myWorkflow/LastGreen/{0}\" \"{1}\"", NodeToDo.Name, FailedString), true);
			RunECTool(String.Format("setProperty \"/myWorkflow/FailCausers/{0}\" \"{1}\"", NodeToDo.Name, FailCauserEMails));
			RunECTool(String.Format("setProperty \"/myWorkflow/EmailNotes/{0}\" \"{1}\"", NodeToDo.Name, EMailNote));
			{
				HashSet<string> Emails = new HashSet<string>(NodeToDo.RecipientsForFailureEmails);
				if (Command.ParseParam("CIS") && !NodeToDo.SendSuccessEmail && NodeToDo.AddSubmittersToFailureEmails)
				{
					Emails.UnionWith(FailCauserEMails.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries));
				}
				RunECTool(String.Format("setProperty \"/myWorkflow/FailEmails/{0}\" \"{1}\"", NodeToDo.Name, String.Join(" ", Emails)));
			}
			if (NodeToDo.SendSuccessEmail || SendSuccessForGreenAfterRed)
			{
				RunECTool(String.Format("setProperty \"/myWorkflow/SendSuccessEmail/{0}\" \"{1}\"", NodeToDo.Name, "1"));
			}
			else
			{
				RunECTool(String.Format("setProperty \"/myWorkflow/SendSuccessEmail/{0}\" \"{1}\"", NodeToDo.Name, "0"));
			}
		}

		public void DoCommanderSetup(IEnumerable<BuildNode> AllNodes, IEnumerable<AggregateNode> AllAggregates, List<BuildNode> OrdereredToDo, List<BuildNode> SortedNodes, int TimeIndex, int TimeQuantum, bool bSkipTriggers, bool bFake, bool bFakeEC, string CLString, TriggerNode ExplicitTrigger, List<TriggerNode> UnfinishedTriggers, string FakeFail)
		{
			List<AggregateNode> SeparatePromotables = FindPromotables(AllAggregates);
			Dictionary<BuildNode, List<AggregateNode>> DependentPromotions = FindDependentPromotables(AllNodes, SeparatePromotables);

			Dictionary<BuildNode, int> FullNodeListSortKey = GetDisplayOrder(SortedNodes);

			if (OrdereredToDo.Count == 0)
			{
				throw new AutomationException("No nodes to do!");
			}
			List<string> ECProps = new List<string>();
			ECProps.Add(String.Format("TimeIndex={0}", TimeIndex));
			foreach (BuildNode Node in SortedNodes)
			{
				ECProps.Add(string.Format("AllNodes/{0}={1}", Node.Name, GetNodeForAllNodesProperty(Node, TimeQuantum)));
			}
			foreach (KeyValuePair<BuildNode, int> NodePair in FullNodeListSortKey)
			{
				ECProps.Add(string.Format("SortKey/{0}={1}", NodePair.Key.Name, NodePair.Value));
			}
			foreach (KeyValuePair<BuildNode, List<AggregateNode>> NodePair in DependentPromotions)
			{
				ECProps.Add(string.Format("DependentPromotions/{0}={1}", NodePair.Key.Name, String.Join(" ", NodePair.Value.Select(x => x.Name))));
			}
			foreach (AggregateNode Node in SeparatePromotables)
			{
				ECProps.Add(string.Format("PossiblePromotables/{0}={1}", Node.Name, ""));
			}
			List<string> ECJobProps = new List<string>();
			if (ExplicitTrigger != null)
			{
				ECJobProps.Add("IsRoot=0");
			}
			else
			{
				ECJobProps.Add("IsRoot=1");
			}

			// here we are just making sure everything before the explicit trigger is completed.
			if (ExplicitTrigger != null)
			{
				foreach (BuildNode NodeToDo in OrdereredToDo)
				{
					if (!NodeToDo.IsComplete && NodeToDo != ExplicitTrigger && !NodeToDo.DependsOn(ExplicitTrigger)) // if something is already finished, we don't put it into EC
					{
						throw new AutomationException("We are being asked to process node {0}, however, this is an explicit trigger {1}, so everything before it should already be handled. It seems likely that you waited too long to run the trigger. You will have to do a new build from scratch.", NodeToDo.Name, ExplicitTrigger.Name);
					}
				}
			}

			BuildNode LastSticky = null;
			bool HitNonSticky = false;
			bool bHaveECNodes = false;
			// sticky nodes are ones that we run on the main agent. We run then first and they must not be intermixed with parallel jobs
			foreach (BuildNode NodeToDo in OrdereredToDo)
			{
				if (!NodeToDo.IsComplete) // if something is already finished, we don't put it into EC
				{
					bHaveECNodes = true;
					if (NodeToDo.IsSticky)
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

			List<JobStep> Steps = new List<JobStep>();
			using(CommandUtils.TelemetryStopwatch PerlOutputStopwatch = new CommandUtils.TelemetryStopwatch("PerlOutput"))
			{
				string ParentPath = Command.ParseParamValue("ParentPath");

				bool bHasNoop = false;
				if (LastSticky == null && bHaveECNodes)
				{
					// if we don't have any sticky nodes and we have other nodes, we run a fake noop just to release the resource 
					List<Tuple<string, string>> NoopParameters = new List<Tuple<string, string>>();
					NoopParameters.Add(new Tuple<string, string>("NodeName", "Noop"));
					NoopParameters.Add(new Tuple<string, string>("Sticky", "1"));
					Steps.Add(new JobStep(ParentPath, "Noop", "GUBP_UAT_Node", false, null, null, NoopParameters, JobStepReleaseMode.Release));

					bHasNoop = true;
				}

				Dictionary<string, List<BuildNode>> AgentGroupChains = new Dictionary<string, List<BuildNode>>();
				List<BuildNode> StickyChain = new List<BuildNode>();
				foreach (BuildNode NodeToDo in OrdereredToDo)
				{
					if (!NodeToDo.IsComplete) // if something is already finished, we don't put it into EC  
					{
						string MyAgentGroup = NodeToDo.AgentSharingGroup;
						if (MyAgentGroup != "")
						{
							if (!AgentGroupChains.ContainsKey(MyAgentGroup))
							{
								AgentGroupChains.Add(MyAgentGroup, new List<BuildNode> { NodeToDo });
							}
							else
							{
								AgentGroupChains[MyAgentGroup].Add(NodeToDo);
							}
						}
					}
					if (NodeToDo.IsSticky)
					{
						if (!StickyChain.Contains(NodeToDo))
						{
							StickyChain.Add(NodeToDo);
						}
					}
				}
				foreach (BuildNode NodeToDo in OrdereredToDo)
				{
					if (!NodeToDo.IsComplete) // if something is already finished, we don't put it into EC  
					{
						List<string> NodeProps = GetECPropsForNode(NodeToDo);
						ECProps.AddRange(NodeProps);

						bool Sticky = NodeToDo.IsSticky;
						if(NodeToDo.IsSticky)
						{
							if(NodeToDo.AgentSharingGroup != "")
							{
								throw new AutomationException("Node {0} is both agent sharing and sitcky.", NodeToDo.Name);
							}
							if(NodeToDo.AgentPlatform != UnrealTargetPlatform.Win64)
							{
								throw new AutomationException("Node {0} is sticky, but {1} hosted. Sticky nodes must be PC hosted.", NodeToDo.Name, NodeToDo.AgentPlatform);
							}
							if(NodeToDo.AgentRequirements != "")
							{
								throw new AutomationException("Node {0} is sticky but has agent requirements.", NodeToDo.Name);
							}
						}

						string ProcedureInfix = "";
						if(NodeToDo.AgentPlatform != UnrealTargetPlatform.Unknown && NodeToDo.AgentPlatform != UnrealTargetPlatform.Win64)
						{
							ProcedureInfix = "_" + NodeToDo.AgentPlatform.ToString();
						}

						bool DoParallel = !Sticky || NodeToDo.IsParallelAgentShareEditor;
						
						TriggerNode TriggerNodeToDo = NodeToDo as TriggerNode;

						List<Tuple<string, string>> Parameters = new List<Tuple<string,string>>();

						Parameters.Add(new Tuple<string, string>("NodeName", NodeToDo.Name));
						Parameters.Add(new Tuple<string, string>("Sticky", NodeToDo.IsSticky ? "1" : "0"));

						if (NodeToDo.AgentSharingGroup != "")
						{
							Parameters.Add(new Tuple<string, string>("AgentSharingGroup", NodeToDo.AgentSharingGroup));
						}

						string Procedure;
						if(TriggerNodeToDo == null || TriggerNodeToDo.IsTriggered)
						{
							if (NodeToDo.IsParallelAgentShareEditor)
							{
								if(NodeToDo.AgentSharingGroup == "")
								{
									Procedure = "GUBP_UAT_Node_Parallel_AgentShare_Editor";
								}
								else
								{
									Procedure = "GUBP_UAT_Node_Parallel_AgentShare3_Editor";
								}
							}
							else
							{
								if (NodeToDo.IsSticky)
								{
									Procedure = "GUBP" + ProcedureInfix + "_UAT_Node";
								}
								else if (NodeToDo.AgentSharingGroup == "")
								{
									Procedure = String.Format("GUBP{0}_UAT_Node_Parallel", ProcedureInfix);
								}
								else
								{
									Procedure = String.Format("GUBP{0}_UAT_Node_Parallel_AgentShare3", ProcedureInfix);
								}
							}
							if (NodeToDo.IsSticky && NodeToDo == LastSticky)
							{
								Procedure += "_Release";
							}
						}
						else
						{
							if(TriggerNodeToDo.RequiresRecursiveWorkflow)
							{
				                Procedure = "GUBP_UAT_Trigger"; //here we run a recursive workflow to wait for the trigger
							}
							else
							{
								Procedure = "GUBP_Hardcoded_Trigger"; //here we advance the state in the hardcoded workflow so folks can approve
							}

							Parameters.Add(new Tuple<string, string>("TriggerState", TriggerNodeToDo.StateName));
							Parameters.Add(new Tuple<string, string>("ActionText", TriggerNodeToDo.ActionText));
							Parameters.Add(new Tuple<string, string>("DescText", TriggerNodeToDo.DescriptionText));

							if (NodeToDo.RecipientsForFailureEmails.Length > 0)
							{
								Parameters.Add(new Tuple<string, string>("EmailsForTrigger", String.Join(" ", NodeToDo.RecipientsForFailureEmails)));
							}
						}

						List<BuildNode> UncompletedEcDeps = new List<BuildNode>();
						foreach (BuildNode Dep in NodeToDo.AllDirectDependencies)
						{
							if (!Dep.IsComplete && OrdereredToDo.Contains(Dep)) // if something is already finished, we don't put it into EC
							{
								if (OrdereredToDo.IndexOf(Dep) > OrdereredToDo.IndexOf(NodeToDo))
								{
									throw new AutomationException("Topological sort error, node {0} has a dependency of {1} which sorted after it.", NodeToDo.Name, Dep.Name);
								}
								UncompletedEcDeps.Add(Dep);
							}
						}

						string PreCondition = GetPreConditionForNode(OrdereredToDo, ParentPath, bHasNoop, AgentGroupChains, StickyChain, NodeToDo, UncompletedEcDeps);
						string RunCondition = GetRunConditionForNode(UncompletedEcDeps, ParentPath);

						// Create the jobs to setup the agent sharing group if necessary
						string NodeParentPath = ParentPath;
						if (NodeToDo.AgentSharingGroup != "")
						{
							NodeParentPath = String.Format("{0}/jobSteps[{1}]", NodeParentPath, NodeToDo.AgentSharingGroup);

							List<BuildNode> MyChain = AgentGroupChains[NodeToDo.AgentSharingGroup];
							if(MyChain.IndexOf(NodeToDo) <= 0)
							{
								// Create the parent job step for this group
								Steps.Add(new JobStep(ParentPath, NodeToDo.AgentSharingGroup, null, true, PreCondition, null, null, JobStepReleaseMode.Keep, InNamePlacement: JobStepNamePlacement.BeforeParallel));

								// Get the resource pool
								List<Tuple<string, string>> GetPoolParameters = new List<Tuple<string,string>>();
								GetPoolParameters.Add(new Tuple<string, string>("AgentSharingGroup", NodeToDo.AgentSharingGroup));
								GetPoolParameters.Add(new Tuple<string,string>("NodeName", NodeToDo.Name));
								Steps.Add(new JobStep(NodeParentPath, String.Format("{0}_GetPool", NodeToDo.AgentSharingGroup), String.Format("GUBP{0}_AgentShare_GetPool", ProcedureInfix), true, PreCondition, null, GetPoolParameters, JobStepReleaseMode.Keep, InNamePlacement: JobStepNamePlacement.BeforeProcedure));

								// Get the agent for this sharing group
								List<Tuple<string, string>> GetAgentParameters = new List<Tuple<string, string>>();
								GetAgentParameters.Add(new Tuple<string, string>("AgentSharingGroup", NodeToDo.AgentSharingGroup));
								GetAgentParameters.Add(new Tuple<string, string>("NodeName", NodeToDo.Name));
								Steps.Add(new JobStep(NodeParentPath, String.Format("{0}_GetAgent", NodeToDo.AgentSharingGroup), String.Format("GUBP{0}_AgentShare_GetAgent", ProcedureInfix), true, Steps.Last().GetCompletedCondition(), null, GetAgentParameters, JobStepReleaseMode.Keep, InExclusive: JobStepExclusiveMode.Call, InResourceName: String.Format("$[/myJob/jobSteps[{0}]/ResourcePool]", NodeToDo.AgentSharingGroup), InNamePlacement: JobStepNamePlacement.BeforeProcedure));

								// Also add the agent being set up to the existing precondition
								PreCondition = Steps.Last().GetCompletedCondition();
							}
						}

						// Build the job step for this node
						Steps.Add(new JobStep(NodeParentPath, NodeToDo.Name, Procedure, DoParallel, PreCondition, RunCondition, Parameters, (NodeToDo.IsSticky && NodeToDo == LastSticky) ? JobStepReleaseMode.Release : JobStepReleaseMode.Keep));
					}
				}
				WriteECPerl(Steps);
			}
			bool bHasTests = OrdereredToDo.Any(x => x.Node.IsTest());
			RunECTool(String.Format("setProperty \"/myWorkflow/HasTests\" \"{0}\"", bHasTests));
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
		}

		private string GetPreConditionForNode(List<BuildNode> OrdereredToDo, string PreconditionParentPath, bool bHasNoop, Dictionary<string, List<BuildNode>> AgentGroupChains, List<BuildNode> StickyChain, BuildNode NodeToDo, List<BuildNode> UncompletedEcDeps)
		{
			List<BuildNode> PreConditionUncompletedEcDeps = new List<BuildNode>();
			if(NodeToDo.AgentSharingGroup == "")
			{
				PreConditionUncompletedEcDeps = new List<BuildNode>(UncompletedEcDeps);
			}
			else 
			{
				List<BuildNode> MyChain = AgentGroupChains[NodeToDo.AgentSharingGroup];
				int MyIndex = MyChain.IndexOf(NodeToDo);
				if (MyIndex > 0)
				{
					PreConditionUncompletedEcDeps.Add(MyChain[MyIndex - 1]);
				}
				else
				{
					// to avoid idle agents (and also EC doesn't actually reserve our agent!), we promote all dependencies to the first one
					foreach (BuildNode Chain in MyChain)
					{
						foreach (BuildNode Dep in Chain.AllDirectDependencies)
						{
							if (!Dep.IsComplete && OrdereredToDo.Contains(Dep)) // if something is already finished, we don't put it into EC
							{
								if (OrdereredToDo.IndexOf(Dep) > OrdereredToDo.IndexOf(Chain))
								{
									throw new AutomationException("Topological sort error, node {0} has a dependency of {1} which sorted after it.", Chain.Name, Dep.Name);
								}
								if (!MyChain.Contains(Dep) && !PreConditionUncompletedEcDeps.Contains(Dep))
								{
									PreConditionUncompletedEcDeps.Add(Dep);
								}
							}
						}
					}
				}
			}
			if (NodeToDo.IsSticky)
			{
				List<BuildNode> MyChain = StickyChain;
				int MyIndex = MyChain.IndexOf(NodeToDo);
				if (MyIndex > 0)
				{
					if (!PreConditionUncompletedEcDeps.Contains(MyChain[MyIndex - 1]) && !MyChain[MyIndex - 1].IsComplete)
					{
						PreConditionUncompletedEcDeps.Add(MyChain[MyIndex - 1]);
					}
				}
				else
				{
					foreach (BuildNode Dep in NodeToDo.AllDirectDependencies)
					{
						if (!Dep.IsComplete && OrdereredToDo.Contains(Dep)) // if something is already finished, we don't put it into EC
						{
							if (OrdereredToDo.IndexOf(Dep) > OrdereredToDo.IndexOf(NodeToDo))
							{
								throw new AutomationException("Topological sort error, node {0} has a dependency of {1} which sorted after it.", NodeToDo.Name, Dep.Name);
							}
							if (!MyChain.Contains(Dep) && !PreConditionUncompletedEcDeps.Contains(Dep))
							{
								PreConditionUncompletedEcDeps.Add(Dep);
							}
						}
					}
				}
			}

			List<string> JobStepNames = new List<string>();
			if (bHasNoop && PreConditionUncompletedEcDeps.Count == 0)
			{
				JobStepNames.Add(PreconditionParentPath + "/jobSteps[Noop]");
			}
			else
			{
				JobStepNames.AddRange(PreConditionUncompletedEcDeps.Select(x => GetJobStep(PreconditionParentPath, x)));
			}

			string PreCondition = "";
			if (JobStepNames.Count > 0)
			{
				PreCondition = String.Format("\"\\$\" . \"[/javascript if({0}) true;]\"", String.Join(" && ", JobStepNames.Select(x => String.Format("getProperty('{0}/status') == 'completed'", x))));
			}
			return PreCondition;
		}

		private static string GetNodeForAllNodesProperty(BuildNode Node, int TimeQuantum)
		{
			string Note = Node.ControllingTriggerDotName;
			if (Note == "")
			{
				int Minutes = TimeQuantum << Node.FrequencyShift;
				if (Minutes == TimeQuantum)
				{
					Note = "always";
				}
				else if (Minutes < 60)
				{
					Note = String.Format("{0}m", Minutes);
				}
				else
				{
					Note = String.Format("{0}h{1}m", Minutes / 60, Minutes % 60);
				}
			}
			return Note;
		}

		private static string GetRunConditionForNode(List<BuildNode> UncompletedEcDeps, string PreconditionParentPath)
		{
			string RunCondition = "";
			if (UncompletedEcDeps.Count > 0)
			{
				RunCondition = "\"\\$\" . \"[/javascript if(";
				int Index = 0;
				foreach (BuildNode Dep in UncompletedEcDeps)
				{
					RunCondition = RunCondition + "((\'\\$\" . \"[" + GetJobStep(PreconditionParentPath, Dep) + "/outcome]\' == \'success\') || ";
					RunCondition = RunCondition + "(\'\\$\" . \"[" + GetJobStep(PreconditionParentPath, Dep) + "/outcome]\' == \'warning\'))";

					Index++;
					if (Index != UncompletedEcDeps.Count)
					{
						RunCondition = RunCondition + " && ";
					}
				}
				RunCondition = RunCondition + ")true; else false;]\"";
			}
			return RunCondition;
		}

		private static List<AggregateNode> FindPromotables(IEnumerable<AggregateNode> NodesToDo)
		{
			List<AggregateNode> SeparatePromotables = new List<AggregateNode>();
			foreach (AggregateNode NodeToDo in NodesToDo)
			{
				if (NodeToDo.IsSeparatePromotable)
				{
					SeparatePromotables.Add(NodeToDo);
				}
			}
			return SeparatePromotables;
		}

		private static Dictionary<BuildNode, List<AggregateNode>> FindDependentPromotables(IEnumerable<BuildNode> NodesToDo, IEnumerable<AggregateNode> SeparatePromotions)
		{
			Dictionary<BuildNode, List<AggregateNode>> DependentPromotions = NodesToDo.ToDictionary(x => x, x => new List<AggregateNode>());
			foreach (AggregateNode SeparatePromotion in SeparatePromotions)
			{
				BuildNode[] Dependencies = SeparatePromotion.Dependencies.SelectMany(x => x.AllIndirectDependencies).Distinct().ToArray();
				foreach (BuildNode Dependency in Dependencies)
				{
					DependentPromotions[Dependency].Add(SeparatePromotion);
				}
			}
			return DependentPromotions;
		}

		/// <summary>
		/// Sorts a list of nodes to display in EC. The default order is based on execution order and agent groups, whereas this function arranges nodes by
		/// frequency then execution order, while trying to group nodes on parallel paths (eg. Mac/Windows editor nodes) together.
		/// </summary>
		static Dictionary<BuildNode, int> GetDisplayOrder(List<BuildNode> Nodes)
		{
			// Split the nodes into separate lists for each frequency
			SortedDictionary<int, List<BuildNode>> NodesByFrequency = new SortedDictionary<int,List<BuildNode>>();
			foreach(BuildNode Node in Nodes)
			{
				List<BuildNode> NodesByThisFrequency;
				if(!NodesByFrequency.TryGetValue(Node.FrequencyShift, out NodesByThisFrequency))
				{
					NodesByThisFrequency = new List<BuildNode>();
					NodesByFrequency.Add(Node.FrequencyShift, NodesByThisFrequency);
				}
				NodesByThisFrequency.Add(Node);
			}

			// Build the output list by scanning each frequency in order
			HashSet<BuildNode> VisitedNodes = new HashSet<BuildNode>();
			Dictionary<BuildNode, int> SortedNodes = new Dictionary<BuildNode,int>();
			foreach(List<BuildNode> NodesByThisFrequency in NodesByFrequency.Values)
			{
				// Find a list of nodes in each display group. If the group name matches the node name, put that node at the front of the list.
				Dictionary<string, List<BuildNode>> DisplayGroups = new Dictionary<string,List<BuildNode>>();
				foreach(BuildNode Node in NodesByThisFrequency)
				{
					string GroupName = Node.Node.GetDisplayGroupName();
					if(!DisplayGroups.ContainsKey(GroupName))
					{
						DisplayGroups.Add(GroupName, new List<BuildNode>{ Node });
					}
					else if(GroupName == Node.Name)
					{
						DisplayGroups[GroupName].Insert(0, Node);
					}
					else
					{
						DisplayGroups[GroupName].Add(Node);
					}
				}

				// Build a list of ordering dependencies, putting all Mac nodes after Windows nodes with the same names.
				Dictionary<BuildNode, List<BuildNode>> NodeDependencies = new Dictionary<BuildNode,List<BuildNode>>(Nodes.ToDictionary(x => x, x => x.AllDirectDependencies.ToList()));
				foreach(KeyValuePair<string, List<BuildNode>> DisplayGroup in DisplayGroups)
				{
					List<BuildNode> GroupNodes = DisplayGroup.Value;
					for (int Idx = 1; Idx < GroupNodes.Count; Idx++)
					{
						NodeDependencies[GroupNodes[Idx]].Add(GroupNodes[0]);
					}
				}

				// Add nodes for each frequency into the master list, trying to match up different groups along the way
				foreach(BuildNode FirstNode in NodesByThisFrequency)
				{
					List<BuildNode> GroupNodes = DisplayGroups[FirstNode.Node.GetDisplayGroupName()];
					foreach(BuildNode GroupNode in GroupNodes)
					{
						AddNodeAndDependencies(GroupNode, NodeDependencies, VisitedNodes, SortedNodes);
					}
				}
			}
			return SortedNodes;
		}

		static void AddNodeAndDependencies(BuildNode Node, Dictionary<BuildNode, List<BuildNode>> NodeDependencies, HashSet<BuildNode> VisitedNodes, Dictionary<BuildNode, int> SortedNodes)
		{
			if(!VisitedNodes.Contains(Node))
			{
				VisitedNodes.Add(Node);
				foreach (BuildNode NodeDependency in NodeDependencies[Node])
				{
					AddNodeAndDependencies(NodeDependency, NodeDependencies, VisitedNodes, SortedNodes);
				}
				SortedNodes.Add(Node, SortedNodes.Count);
			}
		}
	}
}

public class ECJobPropsUtils
{
    public static HashSet<string> ErrorsFromProps(string Filename)
    {
        var Result = new HashSet<string>();
        XmlDocument Doc = new XmlDocument();
        Doc.Load(Filename);
        foreach (XmlElement ChildNode in Doc.FirstChild.ChildNodes)
        {
            if (ChildNode.Name == "propertySheet")
            {
                foreach (XmlElement PropertySheetChild in ChildNode.ChildNodes)
                {
                    if (PropertySheetChild.Name == "property")
                    {
                        bool IsDiag = false;
                        foreach (XmlElement PropertySheetChildDiag in PropertySheetChild.ChildNodes)
                        {
                            if (PropertySheetChildDiag.Name == "propertyName" && PropertySheetChildDiag.InnerText == "ec_diagnostics")
                            {
                                IsDiag = true;
                            }
                            if (IsDiag && PropertySheetChildDiag.Name == "propertySheet")
                            {
                                foreach (XmlElement PropertySheetChildDiagSheet in PropertySheetChildDiag.ChildNodes)
                                {
                                    if (PropertySheetChildDiagSheet.Name == "property")
                                    {
                                        bool IsError = false;
                                        foreach (XmlElement PropertySheetChildDiagSheetElem in PropertySheetChildDiagSheet.ChildNodes)
                                        {
                                            if (PropertySheetChildDiagSheetElem.Name == "propertyName" && PropertySheetChildDiagSheetElem.InnerText.StartsWith("error-"))
                                            {
                                                IsError = true;
                                            }
                                            if (IsError && PropertySheetChildDiagSheetElem.Name == "propertySheet")
                                            {
                                                foreach (XmlElement PropertySheetChildDiagSheetElemInner in PropertySheetChildDiagSheetElem.ChildNodes)
                                                {
                                                    if (PropertySheetChildDiagSheetElemInner.Name == "property")
                                                    {
                                                        bool IsMessage = false;
                                                        foreach (XmlElement PropertySheetChildDiagSheetElemInner2 in PropertySheetChildDiagSheetElemInner.ChildNodes)
                                                        {
                                                            if (PropertySheetChildDiagSheetElemInner2.Name == "propertyName" && PropertySheetChildDiagSheetElemInner2.InnerText == "message")
                                                            {
                                                                IsMessage = true;
                                                            }
                                                            if (IsMessage && PropertySheetChildDiagSheetElemInner2.Name == "value")
                                                            {
                                                                if (!PropertySheetChildDiagSheetElemInner2.InnerText.Contains("LogTailsAndChanges")
                                                                    && !PropertySheetChildDiagSheetElemInner2.InnerText.Contains("-MyJobStepId=")
                                                                    && !PropertySheetChildDiagSheetElemInner2.InnerText.Contains("CommandUtils.Run: Run: Took ")
                                                                    )
                                                                {
                                                                    Result.Add(PropertySheetChildDiagSheetElemInner2.InnerText);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return Result;
    }
}

public class TestECJobErrorParse : BuildCommand
{
    public override void ExecuteBuild()
    {
		LogConsole("*********************** TestECJobErrorParse");

        string Filename = CombinePaths(@"P:\Builds\UE4\GUBP\++depot+UE4-2104401-RootEditor_Failed\Engine\Saved\Logs", "RootEditor_Failed.log");
        var Errors = ECJobPropsUtils.ErrorsFromProps(Filename);
        foreach (var ThisError in Errors)
        {
            LogError(" {0}", ThisError);
        }
    }
}
