// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Linq;

namespace UnrealBuildTool
{
	/// <summary>
	/// Enumerates build action types.
	/// </summary>
	public enum ActionType
	{
		BuildProject,

		Compile,

		CreateAppBundle,

		GenerateDebugInfo,

		Link,
	}

	/** A build action. */
	public class Action
	{
		public List<FileItem> PrerequisiteItems = new List<FileItem>();
		public List<FileItem> ProducedItems = new List<FileItem>();
		public delegate void EventHandler(Action A);

		/** Total number of actions depending on this one. */
		public int NumTotalDependentActions = 0;
		/** Relative cost of producing items for this action. */
		public long RelativeCost = 0;
		public string WorkingDirectory = null;
		public string CommandPath = null;
		public string CommandArguments = null;
		public string StatusDescription = "...";
		public string StatusDetailedDescription = "";
		public bool bCanExecuteRemotely = false;
		public bool bIsVCCompiler = false;
		public bool bIsGCCCompiler = false;
		/** Whether the action is using a pre-compiled header to speed it up. */
		public bool bIsUsingPCH = false;
		/** Whether to delete the prerequisite files on completion */
		public bool bShouldDeletePrereqs = false;
		/** Whether the files in ProducedItems should be deleted before executing this action, when the action is outdated */
		public bool bShouldDeleteProducedItems = false;
		/**
		 * Whether we should log this action, whether executed locally or remotely.  This is useful for actions that take time
		 * but invoke tools without any console output.
		 */
		public bool bShouldOutputStatusDescription = true;
		/** True if we should redirect standard input such that input will only come from the builder (which is none) */
		public bool bShouldBlockStandardInput = false;
		/** True if we should redirect standard output such that text will not be logged */
		public bool bShouldBlockStandardOutput = false;
		/** Start time of action, optionally set by executor. */
		public DateTimeOffset StartTime = DateTimeOffset.MinValue;
		/** End time of action, optionally set by executor. */
		public DateTimeOffset EndTime = DateTimeOffset.MinValue;
		/** Optional custom event handler for standard output. */
		public DataReceivedEventHandler OutputEventHandler = null;
		/** Callback used to perform a special action instead of a generic command line */
		public delegate void BlockingActionHandler(Action Action, out int ExitCode, out string Output);
		public BlockingActionHandler ActionHandler = null;
		/** The type of this action (for debugging purposes). */
		public ActionType ActionType;

		/** True if any libraries produced by this action should be considered 'import libraries' */
		public bool bProducesImportLibrary = false;


		public Action(ActionType InActionType)
		{
			ActionType = InActionType;

			UnrealBuildTool.AllActions.Add(this);
		}

		/**
		 * Compares two actions based on total number of dependent items, descending.
		 * 
		 * @param	A	Action to compare
		 * @param	B	Action to compare
		 */
		public static int Compare( Action A, Action B )
		{
			// Primary sort criteria is total number of dependent files, up to max depth.
			if( B.NumTotalDependentActions != A.NumTotalDependentActions )
			{
				return Math.Sign( B.NumTotalDependentActions - A.NumTotalDependentActions );
			}
			// Secondary sort criteria is relative cost.
			if( B.RelativeCost != A.RelativeCost )
			{
				return Math.Sign( B.RelativeCost - A.RelativeCost );
			}
			// Tertiary sort criteria is number of pre-requisites.
			else
			{
				return Math.Sign( B.PrerequisiteItems.Count - A.PrerequisiteItems.Count );
			}
		}

		public override string ToString()
		{
			string ReturnString = "";
			if (CommandPath != null)
			{
				ReturnString += CommandPath + " - ";
			}
			if (CommandArguments != null)
			{
				ReturnString += CommandArguments;
			}
			return ReturnString;
		}

		/// <summary>
		/// Returns the amount of time that this action is or has been executing in.
		/// </summary>
		public TimeSpan Duration
		{
			get
			{
				if (EndTime == DateTimeOffset.MinValue)
				{
					return DateTimeOffset.Now - StartTime;
				}

				return EndTime - StartTime;
			}
		}
	};

	partial class UnrealBuildTool
	{
		public static List<Action> AllActions = new List<Action>();

		public static void ResetAllActions()
		{
			AllActions = new List<Action>();
		}

		/** Builds a list of actions that need to be executed to produce the specified output items. */
		static List<Action> GetActionsToExecute(List<FileItem> OutputItems, List<UEBuildTarget> Targets)
		{
			// Link producing actions to the items they produce.
			LinkActionsAndItems();

			DeleteStaleHotReloadDLLs();

			// Detect cycles in the action graph.
			DetectActionGraphCycles();

			// Sort action list by "cost" in descending order to improve parallelism.
			SortActionList();

			// Build a set of all actions needed for this target.
			var ActionsNeededForThisTarget = new Dictionary<Action, bool>();

			// For now simply treat all object files as the root target.
			foreach (FileItem OutputItem in OutputItems)
			{
				GatherPrerequisiteActions(OutputItem, ref ActionsNeededForThisTarget);
			}
			
			// For all targets, build a set of all actions that are outdated.
			var OutdatedActionDictionary = new Dictionary<Action, bool>();
			var HistoryList = new List<ActionHistory>();
			var OpenHistoryFiles = new HashSet<string>(StringComparer.InvariantCultureIgnoreCase);
			foreach (var BuildTarget in Targets)
			{
				var HistoryFilename = ActionHistory.GeneratePathForTarget(BuildTarget);
				if (!OpenHistoryFiles.Contains(HistoryFilename))
				{
					var History = new ActionHistory(HistoryFilename);
					HistoryList.Add(History);
					OpenHistoryFiles.Add(HistoryFilename);
					var OutdatedActionsForTarget = GatherAllOutdatedActions(History);
					foreach (var OutdatedAction in OutdatedActionsForTarget)
					{
						OutdatedActionDictionary.Add(OutdatedAction.Key, OutdatedAction.Value);
					}
				}
			}
			
			// Delete produced items that are outdated.
			DeleteOutdatedProducedItems(OutdatedActionDictionary, BuildConfiguration.bShouldDeleteAllOutdatedProducedItems);

			// Save the action history.
			// This must happen after deleting outdated produced items to ensure that the action history on disk doesn't have
			// command-lines that don't match the produced items on disk.
			foreach (var TargetHistory in HistoryList)
			{
				TargetHistory.Save();
			}

			// Create directories for the outdated produced items.
			CreateDirectoriesForProducedItems(OutdatedActionDictionary);

			// Build a list of actions that are both needed for this target and outdated.
			List<Action> ActionsToExecute = new List<Action>();
			foreach (Action Action in AllActions)
			{
				if (ActionsNeededForThisTarget.ContainsKey(Action) && OutdatedActionDictionary[Action] && Action.CommandPath != null)
				{
					ActionsToExecute.Add(Action);
				}
			}

			return ActionsToExecute;
		}

		/** Executes a list of actions. */
		static bool ExecuteActions(List<Action> ActionsToExecute, out string ExecutorName)
		{
			bool Result = true;
			bool bUsedXGE = false;
			ExecutorName = "";
			if (ActionsToExecute.Count > 0)
			{
				if (BuildConfiguration.bAllowXGE || BuildConfiguration.bXGEExport)
				{
					XGE.ExecutionResult XGEResult = XGE.ExecutionResult.TasksSucceeded;

					// Batch up XGE execution by actions with the same output event handler.
					List<Action> ActionBatch = new List<Action>();
					ActionBatch.Add(ActionsToExecute[0]);
					for (int ActionIndex = 1; ActionIndex < ActionsToExecute.Count && XGEResult == XGE.ExecutionResult.TasksSucceeded; ++ActionIndex)
					{
						Action CurrentAction = ActionsToExecute[ActionIndex];
						if (CurrentAction.OutputEventHandler == ActionBatch[0].OutputEventHandler)
						{
							ActionBatch.Add(CurrentAction);
						}
						else
						{
							XGEResult = XGE.ExecuteActions(ActionBatch);
							ActionBatch.Clear();
							ActionBatch.Add(CurrentAction);
						}
					}
					if (ActionBatch.Count > 0 && XGEResult == XGE.ExecutionResult.TasksSucceeded)
					{
						XGEResult = XGE.ExecuteActions(ActionBatch);
						ActionBatch.Clear();
					}

					if (XGEResult != XGE.ExecutionResult.Unavailable)
					{
						ExecutorName = "XGE";
						Result = (XGEResult == XGE.ExecutionResult.TasksSucceeded);
						// don't do local compilation
						bUsedXGE = true;
					}
				}

				// If XGE is disallowed or unavailable, execute the commands locally.
				if (!bUsedXGE)
				{
					ExecutorName = "Local";
					Result = LocalExecutor.ExecuteActions(ActionsToExecute);
				}

				if (bUsedXGE && BuildConfiguration.bXGEExport)
				{
					// we exported xge here, we do not test build products
				}
				else
				{
					// Verify the link outputs were created (seems to happen with Win64 compiles)
					foreach (Action BuildAction in ActionsToExecute)
					{
						if (BuildAction.ActionType == ActionType.Link)
						{
							foreach (FileItem Item in BuildAction.ProducedItems)
							{
								bool bExists;
								if (Item.bIsRemoteFile)
								{
									DateTime UnusedTime;
									long UnusedLength;
									bExists = RPCUtilHelper.GetRemoteFileInfo(Item.AbsolutePath, out UnusedTime, out UnusedLength);
								}
								else
								{
									FileInfo ItemInfo = new FileInfo(Item.AbsolutePath);
									bExists = ItemInfo.Exists;
								}
								if (!bExists)
								{
									throw new BuildException("UBT ERROR: Failed to produce item: " + Item.AbsolutePath);
								}
							}
						}
					}
				}
			}
			// Nothing to execute.
			else
			{
				ExecutorName = "NoActionsToExecute";
				Log.TraceInformation("Target is up to date.");
			}

			// Perform any cleanup
			foreach (Action Action in ActionsToExecute)
			{
				if (Action.bShouldDeletePrereqs)
				{
					foreach (FileItem FileItem in Action.PrerequisiteItems)
					{
						if (bUsedXGE && BuildConfiguration.bXGEExport)
						{
							throw new BuildException("We are exporting XGE with a request to delete prerequisites; we need a delete prerequisites thing or just roll this into the XGE actions.");
						}
						else
						{
							FileItem.Delete();
						}
					}
				}
			}

			return Result;
		}

		/** Links actions with their prerequisite and produced items into an action graph. */
		static void LinkActionsAndItems()
		{
			foreach (Action Action in AllActions)
			{
				foreach (FileItem ProducedItem in Action.ProducedItems)
				{
					ProducedItem.ProducingAction = Action;
					Action.RelativeCost += ProducedItem.RelativeCost;
				}
			}
		}
		static string SplitFilename(string Filename, out string PlatformSuffix, out string ConfigSuffix, out string ProducedItemExtension)
		{
			string WorkingString = Filename;
			ProducedItemExtension = Path.GetExtension(WorkingString);
			if (!WorkingString.EndsWith(ProducedItemExtension))
			{
				throw new BuildException("Bogus extension");
			}
			WorkingString = WorkingString.Substring(0, WorkingString.Length - ProducedItemExtension.Length);

			ConfigSuffix = "";
			foreach (UnrealTargetConfiguration CurConfig in Enum.GetValues(typeof(UnrealTargetConfiguration)))
			{
				if( CurConfig != UnrealTargetConfiguration.Unknown )
				{
					string Test = "-" + CurConfig;
					if (WorkingString.EndsWith(Test))
					{
						WorkingString = WorkingString.Substring(0, WorkingString.Length - Test.Length);
						ConfigSuffix = Test;
						break;
					}
				}
			}
			PlatformSuffix = "";
			foreach (var CurPlatform in Enum.GetValues(typeof(UnrealTargetPlatform)))
			{
				string Test = "-" + CurPlatform;
				if (WorkingString.EndsWith(Test))
				{
					WorkingString = WorkingString.Substring(0, WorkingString.Length - Test.Length);
					PlatformSuffix = Test;
					break;
				}
			}
			return WorkingString;
		}
		/** Finds and deletes stale hot reload DLLs. */
		static void DeleteStaleHotReloadDLLs()
		{
			foreach (Action BuildAction in AllActions)
			{
				if (BuildAction.ActionType == ActionType.Link)
				{
					foreach (FileItem Item in BuildAction.ProducedItems)
					{
						if (Item.bNeedsHotReloadNumbersDLLCleanUp)
						{
							string PlatformSuffix, ConfigSuffix, ProducedItemExtension;
							string Base = SplitFilename(Item.AbsolutePath, out PlatformSuffix, out ConfigSuffix, out ProducedItemExtension);
							String WildCard = Base + "-*" + PlatformSuffix + ConfigSuffix + ProducedItemExtension;
							// Log.TraceInformation("Deleting old hot reload wildcard: \"{0}\".", WildCard);
							// Wildcard search and delete
							string DirectoryToLookIn = Path.GetDirectoryName(WildCard);
							string FileName = Path.GetFileName(WildCard);
							if (Directory.Exists(DirectoryToLookIn))
							{
								// Delete all files within the specified folder
								string[] FilesToDelete = Directory.GetFiles(DirectoryToLookIn, FileName, SearchOption.TopDirectoryOnly);
								foreach (string JunkFile in FilesToDelete)
								{

									string JunkPlatformSuffix, JunkConfigSuffix, JunkProducedItemExtension;
									SplitFilename(JunkFile, out JunkPlatformSuffix, out JunkConfigSuffix, out JunkProducedItemExtension);
									// now make sure that this file has the same config and platform
									if (JunkPlatformSuffix == PlatformSuffix && JunkConfigSuffix == ConfigSuffix)
									{
										try
										{
											Log.TraceInformation("Deleting old hot reload file: \"{0}\".", JunkFile);
											File.Delete(JunkFile);
										}
										catch (Exception Ex)
										{
											// Ingore all exceptions
											Log.TraceInformation("Unable to delete old hot reload file: \"{0}\". Error: {0}", JunkFile, Ex.Message);
										}
									}
								}
							}
						}
					}
				}
			}
		}

		/**
		 * Sorts the action list for improved parallelism with local execution.
		 */
		public static void SortActionList()
		{
			// Mapping from action to a list of actions that directly or indirectly depend on it (up to a certain depth).
			Dictionary<Action,HashSet<Action>> ActionToDependentActionsMap = new Dictionary<Action,HashSet<Action>>();
			// Perform multiple passes over all actions to propagate dependencies.
			const int MaxDepth = 5;
			for (int Pass=0; Pass<MaxDepth; Pass++)
			{
				foreach (Action DependendAction in AllActions)
				{
					foreach (FileItem PrerequisiteItem in DependendAction.PrerequisiteItems)
					{
						Action PrerequisiteAction = PrerequisiteItem.ProducingAction;						
						if( PrerequisiteAction != null )
						{
							HashSet<Action> DependentActions = null;
							if( ActionToDependentActionsMap.ContainsKey(PrerequisiteAction) )
							{
								DependentActions = ActionToDependentActionsMap[PrerequisiteAction];
							}
							else
							{
								DependentActions = new HashSet<Action>();
								ActionToDependentActionsMap[PrerequisiteAction] = DependentActions;
							}
							// Add dependent action...
							DependentActions.Add( DependendAction );
							// ... and all actions depending on it.
							if( ActionToDependentActionsMap.ContainsKey(DependendAction) )
							{
								DependentActions.UnionWith( ActionToDependentActionsMap[DependendAction] );
							}
						}
					}
				}

			}
			// At this point we have a list of dependent actions for each action, up to MaxDepth layers deep.
			foreach (KeyValuePair<Action,HashSet<Action>> ActionMap in ActionToDependentActionsMap)
			{
				ActionMap.Key.NumTotalDependentActions = ActionMap.Value.Count;
			}
			// Sort actions by number of actions depending on them, descending. Secondary sort criteria is file size.
			AllActions.Sort( Action.Compare );		
		}

		/** Checks for cycles in the action graph. */
		static void DetectActionGraphCycles()
		{
			// Starting with actions that only depend on non-produced items, iteratively expand a set of actions that are only dependent on
			// non-cyclical actions.
			Dictionary<Action, bool> ActionIsNonCyclical = new Dictionary<Action, bool>();
			while (true)
			{
				bool bFoundNewNonCyclicalAction = false;

				foreach (Action Action in AllActions)
				{
					if (!ActionIsNonCyclical.ContainsKey(Action))
					{
						// Determine if the action depends on only actions that are already known to be non-cyclical.
						bool bActionOnlyDependsOnNonCyclicalActions = true;
						foreach (FileItem PrerequisiteItem in Action.PrerequisiteItems)
						{
							if (PrerequisiteItem.ProducingAction != null)
							{
								if (!ActionIsNonCyclical.ContainsKey(PrerequisiteItem.ProducingAction))
								{
									bActionOnlyDependsOnNonCyclicalActions = false;
								}
							}
						}

						// If the action only depends on known non-cyclical actions, then add it to the set of known non-cyclical actions.
						if (bActionOnlyDependsOnNonCyclicalActions)
						{
							ActionIsNonCyclical.Add(Action, true);
							bFoundNewNonCyclicalAction = true;
						}
					}
				}

				// If this iteration has visited all actions without finding a new non-cyclical action, then all non-cyclical actions have
				// been found.
				if (!bFoundNewNonCyclicalAction)
				{
					break;
				}
			}

			// If there are any cyclical actions, throw an exception.
			if (ActionIsNonCyclical.Count < AllActions.Count)
			{
				// Describe the cyclical actions.
				string CycleDescription = "";
				foreach (Action Action in AllActions)
				{
					if (!ActionIsNonCyclical.ContainsKey(Action))
					{
						CycleDescription += string.Format("Action: {0}\r\n", Action.CommandPath);
						CycleDescription += string.Format("\twith arguments: {0}\r\n", Action.CommandArguments);
						foreach (FileItem PrerequisiteItem in Action.PrerequisiteItems)
						{
							CycleDescription += string.Format("\tdepends on: {0}\r\n", PrerequisiteItem.AbsolutePath);
						}
						foreach (FileItem ProducedItem in Action.ProducedItems)
						{
							CycleDescription += string.Format("\tproduces:   {0}\r\n", ProducedItem.AbsolutePath);
						}
						CycleDescription += "\r\n\r\n";
					}
				}

				throw new BuildException("Action graph contains cycle!\r\n\r\n{0}", CycleDescription);
			}
		}

		/**
		 * Determines the full set of actions that must be built to produce an item.
		 * @param OutputItem - The item to be built.
		 * @param PrerequisiteActions - The actions that must be built and the root action are 
		 */
		static void GatherPrerequisiteActions(
			FileItem OutputItem,
			ref Dictionary<Action, bool> PrerequisiteActions
			)
		{
			if (OutputItem != null && OutputItem.ProducingAction != null)
			{
				if (!PrerequisiteActions.ContainsKey(OutputItem.ProducingAction))
				{
					PrerequisiteActions.Add(OutputItem.ProducingAction, true);
					foreach (FileItem PrerequisiteItem in OutputItem.ProducingAction.PrerequisiteItems)
					{
						GatherPrerequisiteActions(PrerequisiteItem, ref PrerequisiteActions);
					}
				}
			}
		}

		/**
		 * Determines whether an action is outdated based on the modification times for its prerequisite
		 * and produced items.
		 * @param RootAction - The action being considered.
		 * @param OutdatedActionDictionary - 
		 * @return true if outdated
		 */
		static public bool IsActionOutdated(Action RootAction, ref Dictionary<Action, bool> OutdatedActionDictionary,ActionHistory ActionHistory)
		{
			// Only compute the outdated-ness for actions that don't aren't cached in the outdated action dictionary.
			bool bIsOutdated = false;
			if (!OutdatedActionDictionary.TryGetValue(RootAction, out bIsOutdated))
			{
				// Determine the last time the action was run based on the write times of its produced files.
				string LatestUpdatedProducedItemName = null;
				DateTimeOffset LastExecutionTime = DateTimeOffset.MaxValue;
				foreach (FileItem ProducedItem in RootAction.ProducedItems)
				{
					// Optionally skip the action history check, as this only works for local builds
					if (BuildConfiguration.bUseActionHistory)
					{
						// Check if the command-line of the action previously used to produce the item is outdated.
						string OldProducingCommandLine = "";
						string NewProducingCommandLine = RootAction.CommandPath + " " + RootAction.CommandArguments;
						if (!ActionHistory.GetProducingCommandLine(ProducedItem, out OldProducingCommandLine)
						|| OldProducingCommandLine != NewProducingCommandLine)
						{
							Log.TraceVerbose(
								"{0}: Produced item \"{1}\" was produced by outdated command-line.\r\nOld command-line: {2}\r\nNew command-line: {3}",
								RootAction.StatusDescription,
								Path.GetFileName(ProducedItem.AbsolutePath),
								OldProducingCommandLine,
								NewProducingCommandLine
								);

							bIsOutdated = true;

							// Update the command-line used to produce this item in the action history.
							ActionHistory.SetProducingCommandLine(ProducedItem, NewProducingCommandLine);
						}
					}

					if (ProducedItem.bIsRemoteFile)
					{
						DateTime LastWriteTime;
						long UnusedLength;
						ProducedItem.bExists = RPCUtilHelper.GetRemoteFileInfo(ProducedItem.AbsolutePath, out LastWriteTime, out UnusedLength);
						ProducedItem.LastWriteTime = LastWriteTime;
					}

					// If the produced file doesn't exist or has zero size, consider it outdated.  The zero size check is to detect cases
					// where aborting an earlier compile produced invalid zero-sized obj files, but that may cause actions where that's
					// legitimate output to always be considered outdated.
					if (ProducedItem.bExists && (ProducedItem.bIsRemoteFile || ProducedItem.Length > 0 || ProducedItem.IsDirectory))
					{
						// When linking incrementally, don't use LIB, EXP pr PDB files when checking for the oldest produced item,
						// as those files aren't always touched.
						if( BuildConfiguration.bUseIncrementalLinking )
						{
							String ProducedItemExtension = Path.GetExtension( ProducedItem.AbsolutePath ).ToUpperInvariant();
							if( ProducedItemExtension == ".LIB" || ProducedItemExtension == ".EXP" || ProducedItemExtension == ".PDB" )
							{
								continue;
							}
						}

						// Use the oldest produced item's time as the last execution time.
						if (ProducedItem.LastWriteTime < LastExecutionTime)
						{
							LastExecutionTime = ProducedItem.LastWriteTime;
							LatestUpdatedProducedItemName = ProducedItem.AbsolutePath;
						}
					}
					else
					{
						// If any of the produced items doesn't exist, the action is outdated.
						Log.TraceVerbose(
							"{0}: Produced item \"{1}\" doesn't exist.",
							RootAction.StatusDescription,
							Path.GetFileName(ProducedItem.AbsolutePath)
							);
						bIsOutdated = true;
					}
				}

				Log.WriteLineIf(BuildConfiguration.bLogDetailedActionStats && !String.IsNullOrEmpty( LatestUpdatedProducedItemName ),
					TraceEventType.Verbose, "{0}: Oldest produced item is {1}", RootAction.StatusDescription, LatestUpdatedProducedItemName);

				if(!bIsOutdated)
				{
					// Check if any of the prerequisite items are produced by outdated actions, or have changed more recently than
					// the oldest produced item.
					foreach (FileItem PrerequisiteItem in RootAction.PrerequisiteItems)
					{
						// Only check for outdated import libraries if we were configured to do so.  Often, a changed import library
						// won't affect a dependency unless a public header file was also changed, in which case we would be forced
						// to recompile anyway.  This just allows for faster iteration when working on a subsystem in a DLL, as we
						// won't have to wait for dependent targets to be relinked after each change.
						bool bIsImportLibraryFile = false;
						if( PrerequisiteItem.ProducingAction != null && PrerequisiteItem.ProducingAction.bProducesImportLibrary )
						{
							bIsImportLibraryFile = PrerequisiteItem.AbsolutePath.EndsWith( ".LIB", StringComparison.InvariantCultureIgnoreCase );
						}
						if( !bIsImportLibraryFile || !BuildConfiguration.bIgnoreOutdatedImportLibraries )
						{
							// If the prerequisite is produced by an outdated action, then this action is outdated too.
							if( PrerequisiteItem.ProducingAction != null )
							{
								if(IsActionOutdated(PrerequisiteItem.ProducingAction,ref OutdatedActionDictionary,ActionHistory))
								{
									Log.TraceVerbose(
										"{0}: Prerequisite {1} is produced by outdated action.",
										RootAction.StatusDescription,
										Path.GetFileName(PrerequisiteItem.AbsolutePath)
										);
									bIsOutdated = true;
								}
							}

							if( PrerequisiteItem.bExists )
							{
								// allow a 1 second slop for network copies
								TimeSpan TimeDifference = PrerequisiteItem.LastWriteTime - LastExecutionTime;
								bool bPrerequisiteItemIsNewerThanLastExecution = TimeDifference.TotalSeconds > 1;
								if (bPrerequisiteItemIsNewerThanLastExecution)
								{
									Log.TraceVerbose(
										"{0}: Prerequisite {1} is newer than the last execution of the action: {2} vs {3}",
										RootAction.StatusDescription,
										Path.GetFileName(PrerequisiteItem.AbsolutePath),
										PrerequisiteItem.LastWriteTime.LocalDateTime,
										LastExecutionTime.LocalDateTime
										);
									bIsOutdated = true;
								}
							}

							// GatherAllOutdatedActions will ensure all actions are checked for outdated-ness, so we don't need to recurse with
							// all this action's prerequisites once we've determined it's outdated.
							if (bIsOutdated)
							{
								break;
							}
						}
					}
				}

				// Cache the outdated-ness of this action.
				OutdatedActionDictionary.Add(RootAction, bIsOutdated);
			}

			return bIsOutdated;
		}

		/**
		 * Builds a dictionary containing the actions from AllActions that are outdated by calling
		 * IsActionOutdated.
		 */
		static Dictionary<Action,bool> GatherAllOutdatedActions(ActionHistory ActionHistory)
		{
			var OutdatedActionDictionary = new Dictionary<Action, bool>();
			foreach (var Action in AllActions)
			{
				IsActionOutdated(Action, ref OutdatedActionDictionary,ActionHistory);
			}

			return OutdatedActionDictionary;
		}

		/**
		 * Deletes all the items produced by actions in the provided outdated action dictionary. 
		 * 
		 * @param	OutdatedActionDictionary	Dictionary of outdated actions
		 * @param	bShouldDeleteAllFiles		Whether to delete all files associated with outdated items or just ones required
		 */
		static void DeleteOutdatedProducedItems(Dictionary<Action, bool> OutdatedActionDictionary, bool bShouldDeleteAllFiles)
		{
			foreach (KeyValuePair<Action,bool> OutdatedActionInfo in OutdatedActionDictionary)
			{
				if (OutdatedActionInfo.Value)
				{
					Action OutdatedAction = OutdatedActionInfo.Key;
					foreach (FileItem ProducedItem in OutdatedActionInfo.Key.ProducedItems)
					{
						if( ProducedItem.bExists
						&&	(	bShouldDeleteAllFiles
							// Delete PDB files as incremental updates are slower than full ones.
							||	(!BuildConfiguration.bUseIncrementalLinking && ProducedItem.AbsolutePath.EndsWith(".PDB", StringComparison.InvariantCultureIgnoreCase)) 
							||	OutdatedAction.bShouldDeleteProducedItems) )
						{
							Log.TraceVerbose("Deleting outdated item: {0}", ProducedItem.AbsolutePath);
							ProducedItem.Delete();
						}
					}
				}
			}
		}

		/**
		 * Creates directories for all the items produced by actions in the provided outdated action
		 * dictionary.
		 */
		static void CreateDirectoriesForProducedItems(Dictionary<Action, bool> OutdatedActionDictionary)
		{
			foreach (KeyValuePair<Action, bool> OutdatedActionInfo in OutdatedActionDictionary)
			{
				if (OutdatedActionInfo.Value)
				{
					foreach (FileItem ProducedItem in OutdatedActionInfo.Key.ProducedItems)
					{
						if (ProducedItem.bIsRemoteFile)
						{
							try
							{
								RPCUtilHelper.MakeDirectory(Path.GetDirectoryName(ProducedItem.AbsolutePath).Replace("\\", "/"));
							}
							catch (System.Exception Ex)
							{
								throw new BuildException( Ex, "Error while creating remote directory for '{0}'.  (Exception: {1})", ProducedItem.AbsolutePath, Ex.Message );
							}
						}
						else
						{
							string DirectoryPath = Path.GetDirectoryName(ProducedItem.AbsolutePath);
							if (!Directory.Exists(DirectoryPath))
							{
								Log.TraceVerbose("Creating directory for produced item: {0}", DirectoryPath);
								Directory.CreateDirectory(DirectoryPath);
							}
						}
					}
				}
			}
		}


	
		/// <summary>
		/// Checks if the specified file is a C++ source file
		/// </summary>
		/// <param name="FileItem">The file to check</param>
		/// <returns>True if this is a C++ source file</returns>
		private static bool IsCPPFile( FileItem FileItem )
		{
			if( FileItem.AbsolutePath.EndsWith( ".h", StringComparison.InvariantCultureIgnoreCase ) ||
				FileItem.AbsolutePath.EndsWith( ".inl", StringComparison.InvariantCultureIgnoreCase ) )
			{
				// Header file
				return true;
			}
			else if( FileItem.AbsolutePath.EndsWith( ".cpp", StringComparison.InvariantCultureIgnoreCase ) ||
						FileItem.AbsolutePath.EndsWith( ".c", StringComparison.InvariantCultureIgnoreCase ) ||
						FileItem.AbsolutePath.EndsWith( ".mm", StringComparison.InvariantCultureIgnoreCase ) )
			{
				// C++ file
				return true;
			}

			else if( FileItem.AbsolutePath.EndsWith( ".rc", StringComparison.InvariantCultureIgnoreCase ) )
			{
				// .rc file
				return true;
			}

			return false;
		}



		/// <summary>
		/// Types of action graph visualizations that we can emit
		/// </summary>
		public enum ActionGraphVisualizationType
		{
			OnlyActions,
			ActionsWithFiles,
			ActionsWithFilesAndHeaders,
			OnlyFilesAndHeaders,
			OnlyCPlusPlusFilesAndHeaders
		}



		/// <summary>
		/// Saves the action graph (and include dependency network) to a graph gile
		/// </summary>
		/// <param name="Filename">File name to emit</param>
		/// <param name="Description">Description to be stored in graph metadata</param>
		/// <param name="VisualizationType">Type of graph to create</param>
		/// <param name="Actions">All actions</param>
		/// <param name="IncludeCompileActions">True if we should include compile actions.  If disabled, only the static link actions will be shown, which is useful to see module relationships</param>
		public static void SaveActionGraphVisualization( string Filename, string Description, ActionGraphVisualizationType VisualizationType, List<Action> Actions, bool IncludeCompileActions = true )
		{
			// True if we should include individual files in the graph network, or false to include only the build actions
			var IncludeFiles = VisualizationType != ActionGraphVisualizationType.OnlyActions;
			var OnlyIncludeCPlusPlusFiles = VisualizationType == ActionGraphVisualizationType.OnlyCPlusPlusFilesAndHeaders;

			// True if want to show actions in the graph, otherwise we're only showing files
			var IncludeActions = VisualizationType != ActionGraphVisualizationType.OnlyFilesAndHeaders && VisualizationType != ActionGraphVisualizationType.OnlyCPlusPlusFilesAndHeaders;

			// True if C++ header dependencies should be expanded into the graph, or false to only have .cpp files
			var ExpandCPPHeaderDependencies = IncludeFiles && ( VisualizationType == ActionGraphVisualizationType.ActionsWithFilesAndHeaders || VisualizationType == ActionGraphVisualizationType.OnlyFilesAndHeaders || VisualizationType == ActionGraphVisualizationType.OnlyCPlusPlusFilesAndHeaders );
			
			var TimerStartTime = DateTime.UtcNow;

			var GraphNodes = new List<GraphNode>();

			var FileToGraphNodeMap = new Dictionary< FileItem, GraphNode >();

			// Filter our list of actions
			var FilteredActions = new List<Action>();
			{
				for( var ActionIndex = 0; ActionIndex < Actions.Count; ++ActionIndex )
				{
					var Action = Actions[ ActionIndex ];

					if( !IncludeActions || IncludeCompileActions || ( Action.ActionType != ActionType.Compile ) )
					{
						FilteredActions.Add( Action );
					}
				}
			}
			

			var FilesToCreateNodesFor = new HashSet<FileItem>();
			for( var ActionIndex = 0; ActionIndex < FilteredActions.Count; ++ActionIndex )
			{
				var Action = FilteredActions[ ActionIndex ];

				if( IncludeActions )
				{ 
					var GraphNode = new GraphNode()
					{
						Id = GraphNodes.Count,

						// Don't bother including "Link" text if we're excluding compile actions
						Label = IncludeCompileActions ? ( Action.ActionType.ToString() + " " + Action.StatusDescription ) : Action.StatusDescription
					};

					switch( Action.ActionType )
					{
						case ActionType.BuildProject:
							GraphNode.Color = new GraphColor() { R = 0.3f, G = 1.0f, B = 1.0f, A = 1.0f };
							GraphNode.Size = 1.1f;
							break;

						case ActionType.Compile:
							GraphNode.Color = new GraphColor() { R = 0.3f, G = 1.0f, B = 0.3f, A = 1.0f };
							break;

						case ActionType.Link:
							GraphNode.Color = new GraphColor() { R = 0.3f, G = 0.3f, B = 1.0f, A = 1.0f };
							GraphNode.Size = 1.2f;
							break;
					}

					GraphNodes.Add( GraphNode );
				}
	
				if( IncludeFiles )
				{
					foreach( var ProducedFileItem in Action.ProducedItems )
					{
						if( !OnlyIncludeCPlusPlusFiles || IsCPPFile( ProducedFileItem ) )
						{
							FilesToCreateNodesFor.Add( ProducedFileItem );
						}
					}

					foreach( var PrerequisiteFileItem in Action.PrerequisiteItems )
					{
						if( !OnlyIncludeCPlusPlusFiles || IsCPPFile( PrerequisiteFileItem ) )
						{
							FilesToCreateNodesFor.Add( PrerequisiteFileItem );
						}
					}
				}
			}


			var OverriddenPrerequisites = new Dictionary<FileItem, List<FileItem>>();

			// Determine the average size of all of the C++ source files
			Int64 AverageCPPFileSize;
			{
				Int64 TotalFileSize = 0;
				int CPPFileCount = 0;
				foreach( var FileItem in FilesToCreateNodesFor )
				{
					if( IsCPPFile( FileItem ) )
					{
						++CPPFileCount;
						TotalFileSize += new FileInfo( FileItem.AbsolutePath ).Length;
					}
				}

				if( CPPFileCount > 0 )
				{
					AverageCPPFileSize = TotalFileSize / CPPFileCount;
				}
				else
				{
					AverageCPPFileSize = 1;
				}
			}


			foreach( var FileItem in FilesToCreateNodesFor )
			{
				var FileGraphNode = new GraphNode()
				{
					Id = GraphNodes.Count,
					Label = Path.GetFileName( FileItem.AbsolutePath )
				};

				if( FileItem.AbsolutePath.EndsWith( ".h", StringComparison.InvariantCultureIgnoreCase ) ||
					FileItem.AbsolutePath.EndsWith( ".inl", StringComparison.InvariantCultureIgnoreCase ) )
				{
					// Header file
					FileGraphNode.Color = new GraphColor() { R = 0.9f, G = 0.2f, B = 0.9f, A = 1.0f };
				}
				else if( FileItem.AbsolutePath.EndsWith( ".cpp", StringComparison.InvariantCultureIgnoreCase ) ||
						 FileItem.AbsolutePath.EndsWith( ".c", StringComparison.InvariantCultureIgnoreCase ) ||
						 FileItem.AbsolutePath.EndsWith( ".mm", StringComparison.InvariantCultureIgnoreCase ) )
				{
					// C++ file
					FileGraphNode.Color = new GraphColor() { R = 1.0f, G = 1.0f, B = 0.3f, A = 1.0f };
				}
				else
				{
					// Other file
					FileGraphNode.Color = new GraphColor() { R = 0.4f, G = 0.4f, B = 0.1f, A = 1.0f };
				}

				// Set the size of the file node based on the size of the file on disk
				var bIsCPPFile = IsCPPFile( FileItem );
				if( bIsCPPFile )
				{
					var MinNodeSize = 0.25f;
					var MaxNodeSize = 2.0f;
					var FileSize = new FileInfo( FileItem.AbsolutePath ).Length;
					float FileSizeScale = (float)( (double)FileSize / (double)AverageCPPFileSize );

					var SourceFileSizeScaleFactor = 0.1f;		// How much to make nodes for files bigger or larger based on their difference from the average file's size
					FileGraphNode.Size = Math.Min( Math.Max( 1.0f + SourceFileSizeScaleFactor * FileSizeScale, MinNodeSize ), MaxNodeSize );
				}

				//@todo: Testing out attribute support.  Replace with an attribute that is actually useful!
				//if( FileItem.PrecompiledHeaderIncludeFilename != null )
				//{ 
					//FileGraphNode.Attributes[ "PCHFile" ] = Path.GetFileNameWithoutExtension( FileItem.PrecompiledHeaderIncludeFilename );
				//}

				FileToGraphNodeMap[ FileItem ] = FileGraphNode;
				GraphNodes.Add( FileGraphNode );

				if( ExpandCPPHeaderDependencies && bIsCPPFile )
				{
					List<DependencyInclude> DirectlyIncludedFilenames = CPPEnvironment.GetDirectIncludeDependencies( FileItem, CPPTargetPlatform.Win64, bHackHeaderGenerator:false );

					// Resolve the included file name to an actual file.
					var DirectlyIncludedFiles =
						DirectlyIncludedFilenames
						.Where(DirectlyIncludedFilename => !string.IsNullOrEmpty(DirectlyIncludedFilename.IncludeResolvedName))
						.Select(DirectlyIncludedFilename => DirectlyIncludedFilename.IncludeResolvedName)
						// Skip same include over and over (.inl files)
						.Distinct()
						.Select(FileItem.GetItemByFullPath)
						.ToList();

					OverriddenPrerequisites[ FileItem ] = DirectlyIncludedFiles;
				}
			}


			// Connect everything together
			var GraphEdges = new List<GraphEdge>();

			if( IncludeActions )
			{
				for( var ActionIndex = 0; ActionIndex < FilteredActions.Count; ++ActionIndex )
				{
					var Action = FilteredActions[ ActionIndex ];
					var ActionGraphNode = GraphNodes[ ActionIndex ];

					List<FileItem> ActualPrerequisiteItems = Action.PrerequisiteItems;
					if( IncludeFiles && ExpandCPPHeaderDependencies && Action.ActionType == ActionType.Compile )
					{
						// The first prerequisite is always the .cpp file to compile
						var CPPFile = Action.PrerequisiteItems[ 0 ];
						if( !IsCPPFile( CPPFile ) )
						{
							throw new BuildException( "Was expecting a C++ file as the first prerequisite for a Compile action" );
						}

						ActualPrerequisiteItems = new List<FileItem>();
						ActualPrerequisiteItems.Add( CPPFile );
					}


					foreach( var PrerequisiteFileItem in ActualPrerequisiteItems )
					{
						if( IncludeFiles )
						{
							GraphNode PrerequisiteFileGraphNode;
							if( FileToGraphNodeMap.TryGetValue( PrerequisiteFileItem, out PrerequisiteFileGraphNode ) )
							{
								// Connect a file our action is dependent on, to our action itself
								var GraphEdge = new GraphEdge()
								{
									Id = GraphEdges.Count,
									Source = PrerequisiteFileGraphNode,
									Target = ActionGraphNode,
								};

								GraphEdges.Add( GraphEdge );
							}
							else
							{
								// Not a file we were tracking
								// Console.WriteLine( "Unknown file: " + PrerequisiteFileItem.AbsolutePath );
							}
						}
						else if( PrerequisiteFileItem.ProducingAction != null ) 
						{
							// Not showing files, so connect the actions together
							var ProducingActionIndex = FilteredActions.IndexOf( PrerequisiteFileItem.ProducingAction );
							if( ProducingActionIndex != -1 )
							{
								var SourceGraphNode = GraphNodes[ ProducingActionIndex ];

								var GraphEdge = new GraphEdge()
								{
									Id = GraphEdges.Count,
									Source = SourceGraphNode,
									Target = ActionGraphNode,
								};

								GraphEdges.Add( GraphEdge );
							}
							else
							{
								// Our producer action was filtered out
							}
						}
					}

					foreach( var ProducedFileItem in Action.ProducedItems )
					{
						if( IncludeFiles )
						{
							if( !OnlyIncludeCPlusPlusFiles || IsCPPFile( ProducedFileItem ) )
							{ 
								var ProducedFileGraphNode = FileToGraphNodeMap[ ProducedFileItem ];

								var GraphEdge = new GraphEdge()
								{
									Id = GraphEdges.Count,
									Source = ActionGraphNode,
									Target = ProducedFileGraphNode,
								};

								GraphEdges.Add( GraphEdge );
							}
						}
					}
				}
			}

			if( IncludeFiles && ExpandCPPHeaderDependencies )
			{
				// Fill in overridden prerequisites
				foreach( var FileAndPrerequisites in OverriddenPrerequisites )
				{
					var FileItem = FileAndPrerequisites.Key;
					var FilePrerequisites = FileAndPrerequisites.Value;

					var FileGraphNode = FileToGraphNodeMap[ FileItem ];
					foreach( var PrerequisiteFileItem in FilePrerequisites )
					{
						GraphNode PrerequisiteFileGraphNode;
						if( FileToGraphNodeMap.TryGetValue( PrerequisiteFileItem, out PrerequisiteFileGraphNode ) )
						{
							var GraphEdge = new GraphEdge()
							{
								Id = GraphEdges.Count,
								Source = PrerequisiteFileGraphNode,
								Target = FileGraphNode,
							};

							GraphEdges.Add( GraphEdge );
						}
						else
						{
							// Some other header that we don't track directly
							//Console.WriteLine( "File not known: " + PrerequisiteFileItem.AbsolutePath );
						}
					}
				}
			}

			GraphVisualization.WriteGraphFile( Filename, Description, GraphNodes, GraphEdges );

			if( BuildConfiguration.bPrintPerformanceInfo )
			{
				var TimerDuration = DateTime.UtcNow - TimerStartTime;
				Log.TraceInformation( "Generating and saving ActionGraph took " + TimerDuration.TotalSeconds + "s" );
			}
		}
	};
}
