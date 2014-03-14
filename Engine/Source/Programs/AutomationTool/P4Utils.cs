// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.Linq;
using System.IO;
using System.ComponentModel;
using System.Diagnostics;
using System.Text.RegularExpressions;

namespace AutomationTool
{
	public class RequireP4Attribute : Attribute
	{
	}

	public class P4Exception : AutomationException
	{
		public P4Exception() { }
		public P4Exception(string Msg)
			: base(Msg) { }
		public P4Exception(string Msg, Exception InnerException)
			: base(Msg, InnerException) { }

		public P4Exception(string Format, params object[] Args)
			: base(Format, Args) { }
	}

    [Flags]
    public enum P4LineEnd
    {
        Local = 0,
        Unix = 1,
        Mac = 2,
        Win = 3,
        Share = 4,
    }

    [Flags]
    public enum P4SubmitOption
    {
        SubmitUnchanged = 0,
        RevertUnchanged = 1,
        LeaveUnchanged = 2,
    }

    [Flags]
    public enum P4ClientOption
    {
        None = 0,
        NoAllWrite = 1,
        NoClobber = 2,
        NoCompress = 4,
        NoModTime = 8,
        NoRmDir = 16,
        Unlocked = 32,
        AllWrite = 64,
        Clobber = 128,
        Compress = 256,
        Locked = 512,
        ModTime = 1024,
        RmDir = 2048,
    }

	public class P4ClientInfo
	{
		public string Name;
		public string RootPath;
		public string Host;
		public string Owner;
		public DateTime Access;
        public P4LineEnd LineEnd;
        public P4ClientOption Options;
        public P4SubmitOption SubmitOptions;
        public List<KeyValuePair<string, string>> View = new List<KeyValuePair<string, string>>();

		public override string ToString()
		{
			return Name;
		}
	}

	public enum P4FileType
	{
		[Description("unknown")]
		Unknown,
		[Description("text")]
		Text,
		[Description("binary")]
		Binary,
		[Description("resource")]
		Resource,
		[Description("tempobj")]
		Temp,
		[Description("symlink")]
		Symlink,
		[Description("apple")]
		Apple,
		[Description("unicode")]
		Unicode,
		[Description("utf16")]
		Utf16,
	}

	[Flags]
	public enum P4FileAttributes
	{
		[Description("")]
		None = 0,
		[Description("u")]
		Unicode = 1 << 0,
		[Description("x")]
		Executable = 1 << 1,
		[Description("w")]
		Writeable = 1 << 2,
		[Description("m")]
		LocalModTimes = 1 << 3,
		[Description("k")]
		RCS = 1 << 4,
		[Description("l")]
		Exclusive = 1 << 5,
		[Description("D")]
		DeltasPerRevision = 1 << 6,
		[Description("F")]
		Uncompressed = 1 << 7,
		[Description("C")]
		Compressed = 1 << 8,
		[Description("X")]
		Archive = 1 << 9,
		[Description("S")]
		Revisions = 1 << 10,
	}

	public enum P4Action
	{
		[Description("none")]
		None,
		[Description("add")]
		Add,
		[Description("edit")]
		Edit,
		[Description("delete")]
		Delete,
		[Description("branch")]
		Branch,
		[Description("move/add")]
		MoveAdd,
		[Description("move/delete")]
		MoveDelete,
		[Description("integrate")]
		Integrate,
		[Description("import")]
		Import,
		[Description("purge")]
		Purge,
		[Description("archive")]
		Archive,
		[Description("unknown")]
		Unknown,
	}

	public struct P4FileStat
	{
		public P4FileType Type;
		public P4FileAttributes Attributes;
		public P4Action Action;
		public string Change;
		public bool IsOldType;

		public P4FileStat(P4FileType Type, P4FileAttributes Attributes, P4Action Action)
		{
			this.Type = Type;
			this.Attributes = Attributes;
			this.Action = Action;
			this.Change = String.Empty;
			this.IsOldType = false;
		}

		public static readonly P4FileStat Invalid = new P4FileStat(P4FileType.Unknown, P4FileAttributes.None, P4Action.None);

		public bool IsValid { get { return Type != P4FileType.Unknown; } }
	}

	public partial class CommandUtils
	{
		private static string P4LogPath = "";

		#region Environment Setup

		static private P4Environment PerforceEnvironment;

		/// <summary>
		/// BuildEnvironment to use for this buildcommand. This is initialized by InitBuildEnvironment. As soon
		/// as the script execution in ExecuteBuild begins, the BuildEnv is set up and ready to use.
		/// </summary>
		static public P4Environment P4Env
		{
			get
			{
				if (PerforceEnvironment == null)
				{
					throw new AutomationException("Attempt to use P4Environment before it was initialized or P4 support is disabled.");
				}
				return PerforceEnvironment;
			}
		}

		/// <summary>
		/// Initializes build environment. If the build command needs a specific env-var mapping or
		/// has an extended BuildEnvironment, it must implement this method accordingly.
		/// </summary>
		/// <returns>Initialized and ready to use BuildEnvironment</returns>
		static internal void InitP4Environment()
		{
			CheckP4Enabled();

			P4LogPath = CombinePaths(CmdEnv.LogFolder, "p4.log");
			PerforceEnvironment = Automation.IsBuildMachine ? new P4Environment(CmdEnv) : new LocalP4Environment(CmdEnv);
		}

		#endregion

		/// <summary>
		/// Check is P4 is commands are supported.
		/// </summary>		
		public static bool P4Enabled
		{
			get
			{
				if (!bP4Enabled.HasValue)
				{
					throw new AutomationException("Trying to access P4Enabled property before it was initialized.");
				}
				return (bool)bP4Enabled;
			}
			private set
			{
				bP4Enabled = value;
			}
		}
		private static bool? bP4Enabled;

		/// <summary>
		/// Throws an exception when P4 is disabled. This should be called in every P4 function.
		/// </summary>
		internal static void CheckP4Enabled()
		{
			if (P4Enabled == false)
			{
				throw new AutomationException("P4 is not enabled.");
			}
		}

		/// <summary>
		/// Checks whether commands are allowed to submit files into P4.
		/// </summary>
		public static bool AllowSubmit
		{
			get
			{
				if (!bAllowSubmit.HasValue)
				{
					throw new AutomationException("Trying to access AllowSubmit property before it was initialized.");
				}
				return (bool)bAllowSubmit;
			}
			private set
			{
				bAllowSubmit = value;
			}

		}
		private static bool? bAllowSubmit;

		/// <summary>
		/// Sets up P4Enabled, AllowSubmit properties. Note that this does not intialize P4 environment.
		/// </summary>
		/// <param name="CommandsToExecute">Commands to execute</param>
		/// <param name="Commands">Commands</param>
		internal static void InitP4Support(List<CommandInfo> CommandsToExecute, CaselessDictionary<Type> Commands)
		{
			// Init AllowSubmit
			// If we do not specify on the commandline if submitting is allowed or not, this is 
			// depending on whether we run locally or on a build machine.
			Log("Initializing AllowSubmit.");
			if (GlobalCommandLine.Submit || GlobalCommandLine.NoSubmit)
			{
				AllowSubmit = GlobalCommandLine.Submit;
			}
			else
			{
				AllowSubmit = Automation.IsBuildMachine;
			}
			Log("AllowSubmit={0}", AllowSubmit);

			// Init P4Enabled
			Log("Initializing P4Enabled.");
			if (Automation.IsBuildMachine)
			{
				P4Enabled = !GlobalCommandLine.NoP4;
			}
			else
			{
				P4Enabled = GlobalCommandLine.P4;
				if (!P4Enabled && !GlobalCommandLine.NoP4)
				{
					// Check if any of the commands to execute require P4
					P4Enabled = CheckIfCommandsRequireP4(CommandsToExecute, Commands);
				}
			}
			Log("P4Enabled={0}", P4Enabled);
		}

		/// <summary>
		/// Checks if any of the commands to execute has [RequireP4] attribute.
		/// </summary>
		/// <param name="CommandsToExecute">List of commands to be executed.</param>
		/// <param name="Commands">Commands.</param>
		/// <returns>True if any of the commands to execute has [RequireP4] attribute.</returns>
		private static bool CheckIfCommandsRequireP4(List<CommandInfo> CommandsToExecute, CaselessDictionary<Type> Commands)
		{
			foreach (var CommandInfo in CommandsToExecute)
			{
				Type Command;
				if (Commands.TryGetValue(CommandInfo.CommandName, out Command))
				{
					var RequireP4Attributes = Command.GetCustomAttributes(typeof(RequireP4Attribute), true);
					if (!CommandUtils.IsNullOrEmpty(RequireP4Attributes))
					{
						Log("Command {0} requires P4 functionality.", Command.Name);
						return true;
					}
				}
			}
			return false;
		}

		/// <summary>
		/// Shortcut to Run but with P4.exe as the program name.
		/// </summary>
		/// <param name="CommandLine">Command line</param>
		/// <param name="Input">Stdin</param>
		/// <param name="AllowSpew">true for spew</param>
		/// <returns>Exit code</returns>
		public static ProcessResult P4(string CommandLine, string Input = null, bool AllowSpew = true)
		{
			CheckP4Enabled();
			return Run(HostPlatform.Current.P4Exe, CommandLine, Input, AllowSpew ? ERunOptions.AllowSpew : ERunOptions.NoLoggingOfRunCommand);
		}

		/// <summary>
		/// Calls p4 and returns the output.
		/// </summary>
		/// <param name="Output">Output of the comman.</param>
		/// <param name="CommandLine">Commandline for p4.</param>
		/// <param name="Input">Stdin input.</param>
		/// <param name="AllowSpew">Whether the command should spew.</param>
		/// <returns>True if succeeded, otherwise false.</returns>
		public static bool P4Output(out string Output, string CommandLine, string Input = null, bool AllowSpew = true)
		{
			CheckP4Enabled();
			Output = "";

			var Result = P4(CommandLine, Input, AllowSpew);

			Output = Result.Output;
			return Result == 0;
		}

		/// <summary>
		/// Calls p4 command and writes the output to a logfile.
		/// </summary>
		/// <param name="CommandLine">Commandline to pass to p4.</param>
		/// <param name="Input">Stdin input.</param>
		/// <param name="AllowSpew">Whether the command is allowed to spew.</param>
		public static void LogP4(string CommandLine, string Input = null, bool AllowSpew = true)
		{
			CheckP4Enabled();
			string Output;
			if (!LogP4Output(out Output, CommandLine, Input, AllowSpew))
			{
				throw new P4Exception("p4.exe {0} failed.", CommandLine);
			}
		}

		/// <summary>
		/// Calls p4 and returns the output and writes it also to a logfile.
		/// </summary>
		/// <param name="Output">Output of the comman.</param>
		/// <param name="CommandLine">Commandline for p4.</param>
		/// <param name="Input">Stdin input.</param>
		/// <param name="AllowSpew">Whether the command should spew.</param>
		/// <returns>True if succeeded, otherwise false.</returns>
		public static bool LogP4Output(out string Output, string CommandLine, string Input = null, bool AllowSpew = true)
		{
			CheckP4Enabled();
			Output = "";

			if (String.IsNullOrEmpty(P4LogPath))
			{
				Log(TraceEventType.Error, "P4Utils.SetupP4() must be called before issuing Peforce commands");
				return false;
			}

			var Result = P4(CommandLine, Input, AllowSpew);

			WriteToFile(P4LogPath, CommandLine + "\n");
			WriteToFile(P4LogPath, Result.Output);
			Output = Result.Output;
			return Result == 0;
		}

		/// <summary>
		/// Invokes p4 login command.
		/// </summary>
		public static string GetAuthenticationToken()
		{
			string AuthenticationToken = null;

			string Output;
			string P4Passwd = InternalUtils.GetEnvironmentVariable("P4PASSWD", "", true) + '\n';
			P4Output(out Output, "login -a -p", P4Passwd);

			// Validate output.
			const string PasswordPromptString = "Enter password: \r\n";
			if (Output.Substring(0, PasswordPromptString.Length) == PasswordPromptString)
			{
				int AuthenticationResultStartIndex = PasswordPromptString.Length;
				Regex TokenRegex = new Regex("[0-9A-F]{32}");
				Match TokenMatch = TokenRegex.Match(Output, AuthenticationResultStartIndex);
				if (TokenMatch.Success)
				{
					AuthenticationToken = Output.Substring(TokenMatch.Index, TokenMatch.Length);
				}
			}

			return AuthenticationToken;
		}

        /// <summary>
        /// Invokes p4 changes command.
        /// </summary>
        /// <param name="CommandLine">CommandLine to pass on to the command.</param>
        public class ChangeRecord
        {
            public int CL = 0;
            public string User = "";
            public string UserEmail = "";
            public string Summary = "";
            public static int Compare(ChangeRecord A, ChangeRecord B)
            {
                return (A.CL < B.CL) ? -1 : (A.CL > B.CL) ? 1 : 0;
            }
        }
        static Dictionary<string, string> UserToEmailCache = new Dictionary<string, string>();
        public static string UserToEmail(string User)
        {
            if (UserToEmailCache.ContainsKey(User))
            {
                return UserToEmailCache[User];
            }
            string Result = "";
            try
            {
                var P4Result = CommandUtils.P4(String.Format("user -o {0}", User), AllowSpew: false);
			    if (P4Result == 0)
			    {
				    var Tags = ParseTaggedP4Output(P4Result.Output);
                    Tags.TryGetValue("Email", out Result);
                }
            }
            catch(Exception)
            {
            }
            if (Result == "")
            {
                Log(TraceEventType.Warning, "Could not find email for P4 user {0}", User);
            }
            UserToEmailCache.Add(User, Result);
            return Result;
        }
        static Dictionary<string, List<ChangeRecord>> ChangesCache = new Dictionary<string, List<ChangeRecord>>();
        public static bool Changes(out List<ChangeRecord> ChangeRecords, string CommandLine, bool AllowSpew = true, bool UseCaching = false)
        {
            if (UseCaching && ChangesCache.ContainsKey(CommandLine))
            {
                ChangeRecords = ChangesCache[CommandLine];
                return true;
            }
            ChangeRecords = new List<ChangeRecord>();
            CheckP4Enabled();
            try
            {
                // Change 1999345 on 2014/02/16 by buildmachine@BuildFarm_BUILD-23_buildmachine_++depot+UE4 'GUBP Node Shadow_LabelPromotabl'
                string Output;
                if (!LogP4Output(out Output, "changes " + CommandLine, null, AllowSpew))
                {
                    return false;
                }
                var Lines = Output.Split(new string[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
                foreach (var Line in Lines)
                {
                    ChangeRecord Change = new ChangeRecord();
                    string MatchChange = "Change ";
                    string MatchOn = " on "; 
                    string MatchBy = " by ";

                    int ChangeAt = Line.IndexOf(MatchChange);
                    int OnAt = Line.IndexOf(MatchOn);
                    int ByAt = Line.IndexOf(MatchBy);
                    int AtAt = Line.IndexOf("@");
                    int TickAt = Line.IndexOf("'");
                    int EndTick = Line.LastIndexOf("'");
                    if (ChangeAt >= 0 && OnAt > ChangeAt && ByAt > OnAt && TickAt > ByAt && EndTick > TickAt)
                    {
                        var ChangeString = Line.Substring(ChangeAt + MatchChange.Length, OnAt - ChangeAt - MatchChange.Length);
                        Change.CL = int.Parse(ChangeString);
                        if (Change.CL < 1990000)
                        {
                            throw new AutomationException("weird CL {0} in {1}", Change.CL, Line);
                        }
                        Change.User = Line.Substring(ByAt + MatchBy.Length, AtAt - ByAt - MatchBy.Length);
                        Change.Summary = Line.Substring(TickAt + 1, EndTick - TickAt - 1);
                        Change.UserEmail = UserToEmail(Change.User);
                        ChangeRecords.Add(Change);
                    }
                }
            }
            catch (Exception)
            {
                return false;
            }
            ChangeRecords.Sort((A, B) => ChangeRecord.Compare(A, B));
            ChangesCache.Add(CommandLine, ChangeRecords);
            return true;
        }
		/// <summary>
		/// Invokes p4 sync command.
		/// </summary>
		/// <param name="CommandLine">CommandLine to pass on to the command.</param>
        public static void Sync(string CommandLine, bool AllowSpew = true)
		{
			CheckP4Enabled();
			LogP4("sync " + CommandLine, null, AllowSpew);
		}

		/// <summary>
		/// Invokes p4 unshelve command.
		/// </summary>
		/// <param name="FromCL">Changelist to unshelve.</param>
		/// <param name="ToCL">Changelist where the checked out files should be added.</param>
		/// <param name="CommandLine">Commandline for the command.</param>
		public static void Unshelve(int FromCL, int ToCL, string CommandLine = "")
		{
			CheckP4Enabled();
			LogP4("unshelve " + String.Format("-s {0} ", FromCL) + String.Format("-c {0} ", ToCL) + CommandLine);
		}

		/// <summary>
		/// Invokes p4 edit command.
		/// </summary>
		/// <param name="CL">Changelist where the checked out files should be added.</param>
		/// <param name="CommandLine">Commandline for the command.</param>
		public static void Edit(int CL, string CommandLine)
		{
			CheckP4Enabled();
			LogP4("edit " + String.Format("-c {0} ", CL) + CommandLine);
		}

		/// <summary>
		/// Invokes p4 edit command, no exceptions
		/// </summary>
		/// <param name="CL">Changelist where the checked out files should be added.</param>
		/// <param name="CommandLine">Commandline for the command.</param>
		public static bool Edit_NoExceptions(int CL, string CommandLine)
		{
			try
			{
				CheckP4Enabled();
				string Output;
				if (!LogP4Output(out Output, "edit " + String.Format("-c {0} ", CL) + CommandLine, null, true))
				{
					return false;
				}
				if (Output.IndexOf("- opened for edit") < 0)
				{
					return false;
				}
				return true;
			}
			catch (Exception)
			{
				return false;
			}
		}

		/// <summary>
		/// Invokes p4 add command.
		/// </summary>
		/// <param name="CL">Changelist where the files should be added to.</param>
		/// <param name="CommandLine">Commandline for the command.</param>
		public static void Add(int CL, string CommandLine)
		{
			CheckP4Enabled();
			LogP4("add " + String.Format("-c {0} ", CL) + CommandLine);
		}

		/// <summary>
		/// Invokes p4 reconcile command.
		/// </summary>
		/// <param name="CL">Changelist to check the files out.</param>
		/// <param name="CommandLine">Commandline for the command.</param>
		public static void Reconcile(int CL, string CommandLine)
		{
			CheckP4Enabled();
			LogP4("reconcile " + String.Format("-c {0} -ead -f ", CL) + CommandLine);
		}

		/// <summary>
		/// Invokes p4 reconcile command.
		/// Ignores files that were removed.
		/// </summary>
		/// <param name="CL">Changelist to check the files out.</param>
		/// <param name="CommandLine">Commandline for the command.</param>
		public static void ReconcileNoDeletes(int CL, string CommandLine)
		{
			CheckP4Enabled();
			LogP4("reconcile " + String.Format("-c {0} -ea ", CL) + CommandLine);
		}

		/// <summary>
		/// Invokes p4 resolve command.
		/// Resolves all files by accepting yours and ignoring theirs.
		/// </summary>
		/// <param name="CL">Changelist to resolve.</param>
		/// <param name="CommandLine">Commandline for the command.</param>
		public static void Resolve(int CL, string CommandLine)
		{
			CheckP4Enabled();
			LogP4("resolve -ay " + String.Format("-c {0} ", CL) + CommandLine);
		}

		/// <summary>
		/// Invokes revert command.
		/// </summary>
		/// <param name="CommandLine">Commandline for the command.</param>
		public static void Revert(string CommandLine)
		{
			CheckP4Enabled();
			LogP4("revert " + CommandLine);
		}

		/// <summary>
		/// Invokes revert command.
		/// </summary>
		/// <param name="CL">Changelist to revert</param>
		/// <param name="CommandLine">Commandline for the command.</param>
		public static void Revert(int CL, string CommandLine)
		{
			CheckP4Enabled();
			LogP4("revert " + String.Format("-c {0} ", CL) + CommandLine);
		}

		/// <summary>
		/// Reverts all unchanged file from the specified changelist.
		/// </summary>
		/// <param name="CL">Changelist to revert the unmodified files from.</param>
		public static void RevertUnchanged(int CL)
		{
			CheckP4Enabled();
			// caution this is a really bad idea if you hope to force submit!!!
			LogP4("revert -a " + String.Format("-c {0} ", CL));
		}

		/// <summary>
		/// Reverts all files from the specified changelist.
		/// </summary>
		/// <param name="CL">Changelist to revert.</param>
		public static void RevertAll(int CL)
		{
			CheckP4Enabled();
			LogP4("revert " + String.Format("-c {0} //...", CL));
		}

		/// <summary>
		/// Submits the specified changelist.
		/// </summary>
		/// <param name="CL">Changelist to submit.</param>
		/// <param name="SubmittedCL">Will be set to the submitted changelist number.</param>
		/// <param name="Force">If true, the submit will be forced even if resolve is needed.</param>
		/// <param name="RevertIfFail">If true, if the submit fails, revert the CL.</param>
		public static void Submit(int CL, out int SubmittedCL, bool Force = false, bool RevertIfFail = false)
		{
			CheckP4Enabled();
			if (!AllowSubmit)
			{
				throw new P4Exception("Submit is not allowed currently. Please use the -Submit switch to override that.");
			}

			SubmittedCL = 0;
			int Retry = 0;
			string LastCmdOutput = "none?";
			while (Retry++ < 24)
			{
				bool Pending;
				if (!ChangeExists(CL, out Pending))
				{
					throw new P4Exception("Change {0} does not exist.", CL);
				}
				if (!Pending)
				{
					throw new P4Exception("Change {0} was not pending.", CL);
				}

				string CmdOutput;
				if (!LogP4Output(out CmdOutput, String.Format("submit -c {0}", CL)))
				{
					if (!Force)
					{
						throw new P4Exception("Change {0} failed to submit.\n{1}", CL, CmdOutput);
					}
					Log(TraceEventType.Information, "**** P4 Returned\n{0}\n*******", CmdOutput);

					LastCmdOutput = CmdOutput;
					bool DidSomething = false;
					string HashStr1 = " - must resolve";
					string HashStr2 = " - already locked by";
					if (CmdOutput.IndexOf(HashStr1) > 0 || CmdOutput.IndexOf(HashStr2) > 0)
					{
						string Work = CmdOutput;
						while (Work.Length > 0)
						{
							string SlashSlashStr = "//";
							int SlashSlash = Work.IndexOf(SlashSlashStr);
							if (SlashSlash < 0)
							{
								break;
							}
							Work = Work.Substring(SlashSlash);
							int Hash1 = Work.IndexOf(HashStr1);
							int Hash2 = Work.IndexOf(HashStr2);
							int Hash;
							string HashStr;
							if (Hash1 >= 0 && (Hash1 < Hash2 || Hash2 < 0))
							{
								Hash = Hash1;
								HashStr = HashStr1;
							}
							else
							{
								Hash = Hash2;
								HashStr = HashStr2;
							}
							if (Hash < 0)
							{
								break;
							}
							string File = Work.Substring(0, Hash).Trim();
							if (File.IndexOf(SlashSlashStr) != File.LastIndexOf(SlashSlashStr))
							{
								// this is some other line about the same line, we ignore it, removing the first // so we advance
								Work = Work.Substring(SlashSlashStr.Length);
							}
							else
							{
								Work = Work.Substring(Hash);

								Log(TraceEventType.Information, "Brutal 'resolve' on {0} to force submit.\n", File);

								Revert(CL, "-k " + File);  // revert the file without overwriting the local one
								Sync("-f -k " + File + "#head", false); // sync the file without overwriting local one
								ReconcileNoDeletes(CL, File);  // re-check out, if it changed, or add
								DidSomething = true;
							}
						}

					}
					if (!DidSomething)
					{
						Log(TraceEventType.Information, "Change {0} failed to submit for reasons we do not recognize.\n{1}\nWaiting and retrying.", CL, CmdOutput);
					}
					System.Threading.Thread.Sleep(15000);
				}
				else
				{
					LastCmdOutput = CmdOutput;
					if (CmdOutput.Trim().EndsWith("submitted."))
					{
						if (CmdOutput.Trim().EndsWith(" and submitted."))
						{
							string EndStr = " and submitted.";
							string ChangeStr = "renamed change ";
							int Offset = CmdOutput.LastIndexOf(ChangeStr);
							int EndOffset = CmdOutput.LastIndexOf(EndStr);
							if (Offset >= 0 && Offset < EndOffset)
							{
								SubmittedCL = int.Parse(CmdOutput.Substring(Offset + ChangeStr.Length, EndOffset - Offset - ChangeStr.Length));
							}
						}
						else
						{
							string EndStr = " submitted.";
							string ChangeStr = "Change ";
							int Offset = CmdOutput.LastIndexOf(ChangeStr);
							int EndOffset = CmdOutput.LastIndexOf(EndStr);
							if (Offset >= 0 && Offset < EndOffset)
							{
								SubmittedCL = int.Parse(CmdOutput.Substring(Offset + ChangeStr.Length, EndOffset - Offset - ChangeStr.Length));
							}
						}

						Log(TraceEventType.Information, "Submitted CL {0} which became CL {1}\n", CL, SubmittedCL);
					}

					if (SubmittedCL < CL)
					{
						throw new P4Exception("Change {0} submission seemed to succeed, but did not look like it.\n{1}", CL, CmdOutput);
					}

					// Change submitted OK!  No need to retry.
					return;
				}
			}
			if (RevertIfFail)
			{
				Log(TraceEventType.Error, "Submit CL {0} failed, reverting files\n", CL);
				RevertAll(CL);
				Log(TraceEventType.Error, "Submit CL {0} failed, reverting files\n", CL);
			}
			throw new P4Exception("Change {0} failed to submit after 12 retries??.\n{1}", CL, LastCmdOutput);
		}

		/// <summary>
		/// Creates a new changelist with the specified owner and description.
		/// </summary>
		/// <param name="Owner">Owner of the changelist.</param>
		/// <param name="Description">Description of the changelist.</param>
		/// <returns>Id of the created changelist.</returns>
		public static int CreateChange(string Owner = null, string Description = null)
		{
			CheckP4Enabled();
			var ChangeSpec = "Change: new" + "\n";
			ChangeSpec += "Client: " + ((Owner != null) ? Owner : "") + "\n";
			ChangeSpec += "Description: \n " + ((Description != null) ? Description : "(none)") + "\n";
			string CmdOutput;
			int CL = 0;
			Log(TraceEventType.Information, "Creating Change\n {0}\n", ChangeSpec);
			if (LogP4Output(out CmdOutput, "change -i", Input: ChangeSpec))
			{
				string EndStr = " created.";
				string ChangeStr = "Change ";
				int Offset = CmdOutput.LastIndexOf(ChangeStr);
				int EndOffset = CmdOutput.LastIndexOf(EndStr);
				if (Offset >= 0 && Offset < EndOffset)
				{
					CL = int.Parse(CmdOutput.Substring(Offset + ChangeStr.Length, EndOffset - Offset - ChangeStr.Length));
				}
			}
			if (CL <= 0)
			{
				throw new P4Exception("Failed to create Changelist. Owner: {0} Desc: {1}", Owner, Description);
			}
			else
			{
				Log(TraceEventType.Information, "Returned CL {0}\n", CL);
			}
			return CL;
		}

		/// <summary>
		/// Deletes the specified changelist.
		/// </summary>
		/// <param name="CL">Changelist to delete.</param>
		/// <param name="RevertFiles">Indicates whether files in that changelist should be reverted.</param>
		public static void DeleteChange(int CL, bool RevertFiles = true)
		{
			CheckP4Enabled();
			if (RevertFiles)
			{
				RevertAll(CL);
			}

			string CmdOutput;
			if (LogP4Output(out CmdOutput, String.Format("change -d {0}", CL)))
			{
				string EndStr = " deleted.";
				string ChangeStr = "Change ";
				int Offset = CmdOutput.LastIndexOf(ChangeStr);
				int EndOffset = CmdOutput.LastIndexOf(EndStr);
				if (Offset == 0 && Offset < EndOffset)
				{
					return;
				}
			}
			throw new P4Exception("Could not delete change {0} output follows\n{1}", CL, CmdOutput);
		}

		/// <summary>
		/// Tries to delete the specified empty changelist.
		/// </summary>
		/// <param name="CL">Changelist to delete.</param>
		/// <returns>True if the changelist was deleted, false otherwise.</returns>
		public static bool TryDeleteEmptyChange(int CL)
		{
			CheckP4Enabled();

			string CmdOutput;
			if (LogP4Output(out CmdOutput, String.Format("change -d {0}", CL)))
			{
				string EndStr = " deleted.";
				string ChangeStr = "Change ";
				int Offset = CmdOutput.LastIndexOf(ChangeStr);
				int EndOffset = CmdOutput.LastIndexOf(EndStr);
				if (Offset == 0 && Offset < EndOffset && !CmdOutput.Contains("can't be deleted."))
				{
					return true;
				}
			}

			return false;
		}

		/// <summary>
		/// Returns the changelist specification.
		/// </summary>
		/// <param name="CL">Changelist to get the specification from.</param>
		/// <returns>Specification of the changelist.</returns>
		public static string ChangeOutput(int CL)
		{
			CheckP4Enabled();
			string CmdOutput;
			if (LogP4Output(out CmdOutput, String.Format("change -o {0}", CL)))
			{
				return CmdOutput;
			}
			throw new P4Exception("ChangeOutput failed {0} output follows\n{1}", CL, CmdOutput);
		}

		/// <summary>
		/// Checks whether the specified changelist exists.
		/// </summary>
		/// <param name="CL">Changelist id.</param>
		/// <param name="Pending">Whether it is a pending changelist.</param>
		/// <returns>Returns whether the changelist exists.</returns>
		public static bool ChangeExists(int CL, out bool Pending)
		{
			CheckP4Enabled();
			string CmdOutput = ChangeOutput(CL);
			Pending = false;
			if (CmdOutput.Length > 0)
			{
				string EndStr = " unknown.";
				string ChangeStr = "Change ";
				int Offset = CmdOutput.LastIndexOf(ChangeStr);
				int EndOffset = CmdOutput.LastIndexOf(EndStr);
				if (Offset == 0 && Offset < EndOffset)
				{
					Log(TraceEventType.Information, "Change {0} does not exist", CL);
					return false;
				}

				string StatusStr = "Status:";
				int StatusOffset = CmdOutput.LastIndexOf(StatusStr);
				string DescStr = "Description:";
				int DescOffset = CmdOutput.LastIndexOf(DescStr);

				if (StatusOffset < 1 || DescOffset < 1 || StatusOffset > DescOffset)
				{
					Log(TraceEventType.Error, "Change {0} could not be parsed\n{1}", CL, CmdOutput);
					return false;
				}

				string Status = CmdOutput.Substring(StatusOffset + StatusStr.Length, DescOffset - StatusOffset - StatusStr.Length).Trim();
				Log(TraceEventType.Information, "Change {0} exists ({1})", CL, Status);
				Pending = (Status == "pending");
				return true;
			}
			Log(TraceEventType.Error, "Change exists failed {0} no output?", CL, CmdOutput);
			return false;
		}

		/// <summary>
		/// Returns a list of files contained in the specified changelist.
		/// </summary>
		/// <param name="CL">Changelist to get the files from.</param>
		/// <param name="Pending">Whether the changelist is a pending one.</param>
		/// <returns>List of the files contained in the changelist.</returns>
		public static List<string> ChangeFiles(int CL, out bool Pending)
		{
			CheckP4Enabled();
			var Result = new List<string>();

			if (ChangeExists(CL, out Pending))
			{
				string CmdOutput = ChangeOutput(CL);
				if (CmdOutput.Length > 0)
				{

					string FilesStr = "Files:";
					int FilesOffset = CmdOutput.LastIndexOf(FilesStr);
					if (FilesOffset < 0)
					{
						throw new P4Exception("Change {0} returned bad output\n{1}", CL, CmdOutput);
					}
					else
					{
						CmdOutput = CmdOutput.Substring(FilesOffset + FilesStr.Length);
						while (CmdOutput.Length > 0)
						{
							string SlashSlashStr = "//";
							int SlashSlash = CmdOutput.IndexOf(SlashSlashStr);
							if (SlashSlash < 0)
							{
								break;
							}
							CmdOutput = CmdOutput.Substring(SlashSlash);
							string HashStr = "#";
							int Hash = CmdOutput.IndexOf(HashStr);
							if (Hash < 0)
							{
								break;
							}
							string File = CmdOutput.Substring(0, Hash).Trim();
							CmdOutput = CmdOutput.Substring(Hash);

							Log(TraceEventType.Error, "TEst {0}", File);
							Result.Add(File);
						}
					}
				}
			}
			else
			{
				throw new P4Exception("Change {0} did not exist.", CL);
			}
			return Result;
		}

		/// <summary>
		/// Deletes the specified label.
		/// </summary>
		/// <param name="LabelName">Label to delete.</param>
        public static void DeleteLabel(string LabelName, bool AllowSpew = true)
		{
			CheckP4Enabled();
			var CommandLine = "label -d " + LabelName;

			// NOTE: We don't throw exceptions when trying to delete a label
			string Output;
			if (!LogP4Output(out Output, CommandLine, null, AllowSpew))
			{
				Log(TraceEventType.Information, "Couldn't delete label '{0}'.  It may not have existed in the first place.", LabelName);
			}
		}

		/// <summary>
		/// Creates a new label.
		/// </summary>
		/// <param name="Name">Name of the label.</param>
		/// <param name="Options">Options for the label. Valid options are "locked", "unlocked", "autoreload" and "noautoreload".</param>
		/// <param name="View">View mapping for the label.</param>
		/// <param name="Owner">Owner of the label.</param>
		/// <param name="Description">Description of the label.</param>
		/// <param name="Date">Date of the label creation.</param>
		/// <param name="Time">Time of the label creation</param>
		public static void CreateLabel(string Name, string Options, string View, string Owner = null, string Description = null, string Date = null, string Time = null)
		{
			CheckP4Enabled();
			var LabelSpec = "Label: " + Name + "\n";
			LabelSpec += "Owner: " + ((Owner != null) ? Owner : "") + "\n";
			LabelSpec += "Description: " + ((Description != null) ? Description : "") + "\n";
			if (Date != null)
			{
				LabelSpec += " Date: " + Date + "\n";
			}
			if (Time != null)
			{
				LabelSpec += " Time: " + Time + "\n";
			}
			LabelSpec += "Options: " + Options + "\n";
			LabelSpec += "View: \n";
			LabelSpec += " " + View;

			Log(TraceEventType.Information, "Creating Label\n {0}\n", LabelSpec);
			LogP4("label -i", Input: LabelSpec);
		}

		/// <summary>
		/// Invokes p4 tag command.
		/// Associates a named label with a file revision.
		/// </summary>
		/// <param name="LabelName">Name of the label.</param>
		/// <param name="FilePath">Path to the file.</param>
		/// <param name="AllowSpew">Whether the command is allowed to spew.</param>
		public static void Tag(string LabelName, string FilePath, bool AllowSpew = true)
		{
			CheckP4Enabled();
			LogP4("tag -l " + LabelName + " " + FilePath, null, AllowSpew);
		}

		/// <summary>
		/// Syncs a label to the current content of the client.
		/// </summary>
		/// <param name="LabelName">Name of the label.</param>
		/// <param name="AllowSpew">Whether the command is allowed to spew.</param>
		public static void LabelSync(string LabelName, bool AllowSpew = true, string FileToLabel = "")
		{
			CheckP4Enabled();
			string Quiet = "";
			if (!AllowSpew)
			{
				Quiet = "-q ";
			}
			if (FileToLabel == "")
			{
				LogP4("labelsync " + Quiet + "-l " + LabelName);
			}
			else
			{
				LogP4("labelsync " + Quiet + "-l" + LabelName + " " + FileToLabel);
			}
		}

		/// <summary>
		/// Syncs a label from another label.
		/// </summary>
		/// <param name="FromLabelName">Source label name.</param>
		/// <param name="ToLabelName">Target label name.</param>
		/// <param name="AllowSpew">Whether the command is allowed to spew.</param>
		public static void LabelToLabelSync(string FromLabelName, string ToLabelName, bool AllowSpew = true)
		{
			CheckP4Enabled();
			string Quiet = "";
			if (!AllowSpew)
			{
				Quiet = "-q ";
			}
			LogP4("labelsync -a " + Quiet + "-l " + ToLabelName + " //...@" + FromLabelName);
		}

		/// <summary>
		/// Checks whether the specified label exists and has any files.
		/// </summary>
		/// <param name="Name">Name of the label.</param>
		/// <returns>Whether there is an label with files.</returns>
		public static bool LabelExistsAndHasFiles(string Name)
		{
			CheckP4Enabled();
			string Output;
			return LogP4Output(out Output, "files -m 1 //...@" + Name);
		}

		/// <summary>
		/// Returns the label description.
		/// </summary>
		/// <param name="Name">Name of the label.</param>
		/// <param name="Description">Description of the label.</param>
		/// <returns>Returns whether the label description could be retrieved.</returns>
		public static bool LabelDescription(string Name, out string Description)
		{
			CheckP4Enabled();
			string Output;
			Description = "";
			if (LogP4Output(out Output, "label -o " + Name))
			{
				string Desc = "Description:";
				int Start = Output.LastIndexOf(Desc);
				if (Start > 0)
				{
					Start += Desc.Length;
				}
				int End = Output.LastIndexOf("Options:");
				if (Start > 0 && End > 0 && End > Start)
				{
					Description = Output.Substring(Start, End - Start);
					Description = Description.Trim();
					return true;
				}
			}
			return false;
		}
		/// <summary>
        /// returns the full name of a label. //depot/UE4/TEST-GUBP-Promotable-OrionGame-CL-198160
		/// </summary>
		/// <param name="BuildNamePrefix">Label Prefix</param>
        public static string FullLabelName(string BuildNamePrefix)
        {
            CheckP4Enabled();
            var Label = P4Env.LabelPrefix + BuildNamePrefix + "-CL-" + P4Env.ChangelistString;
            Log("Label prefix {0}", BuildNamePrefix);
            Log("Full Label name {0}", Label); 
            return Label;
        }

		/// <summary>
		/// Creates a downstream label.
		/// </summary>
		/// <param name="BuildNamePrefix">Label Prefix</param>
		public static void MakeDownstreamLabel(string BuildNamePrefix, List<string> Files = null)
		{
			CheckP4Enabled();
			string DOWNSTREAM_LabelPrefix = GetEnvVar("DOWNSTREAM_LabelPrefix");
			if (!String.IsNullOrEmpty(DOWNSTREAM_LabelPrefix))
			{
				BuildNamePrefix = DOWNSTREAM_LabelPrefix;
			}
			if (String.IsNullOrEmpty(BuildNamePrefix))
			{
				throw new P4Exception("Need a downstream label");
			}

			{
                Log("Making downstream label");
                var Label = FullLabelName(BuildNamePrefix);

                Log("Deleting old label {0} (if any)...", Label);
                DeleteLabel(Label, false);

				Log("Creating new label...");
				CreateLabel(
					Name: Label,
					Description: "BVT Time " + CmdEnv.TimestampAsString + "  CL " + P4Env.ChangelistString,
					Options: "unlocked noautoreload",
					View: CombinePaths(PathSeparator.Depot, P4Env.BuildRootP4, "...")
					);
				if (Files == null)
				{
					Log("Adding all files to new label {0}...", Label);
					LabelSync(Label, false);
				}
				else
				{
					Log("Adding build products to new label {0}...", Label);
					foreach (string LabelFile in Files)
					{
						LabelSync(Label, false, LabelFile);
					}
				}
			}
		}

        /// <summary>
        /// Creates a downstream label.
        /// </summary>
        /// <param name="BuildNamePrefix">Label Prefix</param>
        public static void MakeDownstreamLabelFromLabel(string BuildNamePrefix, string CopyFromBuildNamePrefix)
        {
            CheckP4Enabled();
            string DOWNSTREAM_LabelPrefix = GetEnvVar("DOWNSTREAM_LabelPrefix");
            if (!String.IsNullOrEmpty(DOWNSTREAM_LabelPrefix))
            {
                BuildNamePrefix = DOWNSTREAM_LabelPrefix;
            }
            if (String.IsNullOrEmpty(BuildNamePrefix) || String.IsNullOrEmpty(CopyFromBuildNamePrefix))
            {
                throw new P4Exception("Need a downstream label");
            }

            {
                Log("Making downstream label");
                var Label = FullLabelName(BuildNamePrefix);

                Log("Deleting old label {0} (if any)...", Label);
                DeleteLabel(Label, false);

                Log("Creating new label...");
                CreateLabel(
                    Name: Label,
                    Description: "BVT Time " + CmdEnv.TimestampAsString + "  CL " + P4Env.ChangelistString,
                    Options: "unlocked noautoreload",
                    View: CombinePaths(PathSeparator.Depot, P4Env.BuildRootP4, "...")
                    );
                LabelToLabelSync(FullLabelName(CopyFromBuildNamePrefix), Label, false);
            }
        }

        /// <summary>
        /// Given a file path in the depot, returns the local disk mapping for the current view
        /// </summary>
        /// <param name="DepotFilepath">The full file path in depot naming form</param>
        /// <returns>The file's first reported path on disk or null if no mapping was found</returns>
        public static string DepotToLocalPath(string DepotFilepath)
        {
            CheckP4Enabled();
            string Output;
            string Command = "where " + DepotFilepath;
            if (!LogP4Output(out Output, Command))
            {
                throw new P4Exception("p4.exe {0} failed.", Command);
            }

            string[] mappings = Output.Split(new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (string mapping in mappings)
            {
                if (mapping.EndsWith("not in client view."))
                {
                    return null;
                }
                string[] files = mapping.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                if (files.Length > 0)
                {
                    return files[files.Length - 1];
                }
            }

            return null;
        }

		/// <summary>
		/// Gets file stats.
		/// </summary>
		/// <param name="Filename">Filenam</param>
		/// <returns>File stats (invalid if the file does not exist in P4)</returns>
		public static P4FileStat FStat(string Filename)
		{
			CheckP4Enabled();
			string Output;
			string Command = "fstat " + Filename;
			if (!LogP4Output(out Output, Command))
			{
				throw new P4Exception("p4.exe {0} failed.", Command);
			}

			P4FileStat Stat = P4FileStat.Invalid;

			if (Output.Contains("no such file(s)") == false)
			{
				Output = Output.Replace("\r", "");
				var FormLines = Output.Split('\n');
				foreach (var Line in FormLines)
				{
					var StatAttribute = Line.StartsWith("... ") ? Line.Substring(4) : Line;
					var StatPair = StatAttribute.Split(' ');
					if (StatPair.Length == 2 && !String.IsNullOrEmpty(StatPair[1]))
					{
						switch (StatPair[0])
						{
							case "type":
								// Use type (current CL if open) if possible
								ParseFileType(StatPair[1], ref Stat);
								break;
							case "headType":
								if (Stat.Type == P4FileType.Unknown)
								{
									ParseFileType(StatPair[1], ref Stat);
								}
								break;
							case "action":
								Stat.Action = ParseAction(StatPair[1]);
								break;
							case "change":
								Stat.Change = StatPair[1];
								break;
						}
					}
				}
				if (Stat.IsValid == false)
				{
					throw new AutomationException("Unable to parse fstat result for {0} (unknown file type).", Filename);
				}
			}
			return Stat;
		}

		/// <summary>
		/// Set file attributes (additively)
		/// </summary>
		/// <param name="Filename">File to change the attributes of.</param>
		/// <param name="Attributes">Attributes to set.</param>
		public static void ChangeFileType(string Filename, P4FileAttributes Attributes, string Changelist = null)
		{
			Log("ChangeFileType({0}, {1}, {2})", Filename, Attributes, String.IsNullOrEmpty(Changelist) ? "null" : Changelist);

			var Stat = FStat(Filename);
			if (String.IsNullOrEmpty(Changelist))
			{
				Changelist = (Stat.Action != P4Action.None) ? Stat.Change : "default";
			}
			// Only update attributes if necessary
			if ((Stat.Attributes & Attributes) != Attributes)
			{
				var CmdLine = String.Format("{0} -c {1} -t {2} {3}",
					(Stat.Action != P4Action.None) ? "reopen" : "open",
					Changelist, FileAttributesToString(Attributes | Stat.Attributes), Filename);
				LogP4(CmdLine);
			}
		}

		/// <summary>
		/// Parses P4 forms and stores them as a key/value pairs.
		/// </summary>
		/// <param name="Output">P4 command output (must be a form).</param>
		/// <returns>Parsed output.</returns>
		public static CaselessDictionary<string> ParseTaggedP4Output(string Output)
		{
			var Tags = new CaselessDictionary<string>();
			var Lines = Output.Split(new string[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
            string DelayKey = "";
            int DelayIndex = 0;
			foreach (var Line in Lines)
			{
				var TrimmedLine = Line.Trim();
                if (TrimmedLine.StartsWith("#") == false)
				{
                    if (DelayKey != "")
                    {
                        if (Line.StartsWith("\t"))
                        {
                            if (DelayIndex > 0)
                            {
                                Tags.Add(String.Format("{0}{1}", DelayKey, DelayIndex), TrimmedLine);
                            }
                            else
                            {
                                Tags.Add(DelayKey, TrimmedLine);
                            }
                            DelayIndex++;
                            continue;
                        }
                        DelayKey = "";
                        DelayIndex = 0;
                    }
					var KeyEndIndex = TrimmedLine.IndexOf(':');
					if (KeyEndIndex >= 0)
					{
						var Key = TrimmedLine.Substring(0, KeyEndIndex);
						var Value = TrimmedLine.Substring(KeyEndIndex + 1).Trim();
                        if (Value == "")
                        {
                            DelayKey = Key;
                        }
                        else
                        {
                            Tags.Add(Key, Value);
                        }
					}
				}
			}
			return Tags;
		}

		/// <summary>
		/// Checks if the client exists in P4.
		/// </summary>
		/// <param name="ClientName">Client name</param>
		/// <returns>True if the client exists.</returns>
		public static bool DoesClientExist(string ClientName)
		{
			CheckP4Enabled();
			Log("Checking if client {0} exists", ClientName);
			var P4Result = P4(String.Format("-c {0} where //...", ClientName), AllowSpew: false);
			return P4Result.Output.IndexOf("unknown - use 'client' command", StringComparison.InvariantCultureIgnoreCase) < 0;
		}

		/// <summary>
		/// Gets client info.
		/// </summary>
		/// <param name="ClientName">Name of the client.</param>
		/// <returns></returns>
		public static P4ClientInfo GetClientInfo(string ClientName)
		{
			CheckP4Enabled();

			Log("Getting info for client {0}", ClientName);
			if (!DoesClientExist(ClientName))
			{
				return null;
			}

			return GetClientInfoInternal(ClientName);
		}
        /// <summary>
        /// Parses a string with enum values separated with spaces.
        /// </summary>
        /// <param name="ValueText"></param>
        /// <param name="EnumType"></param>
        /// <returns></returns>
        private static object ParseEnumValues(string ValueText, Type EnumType)
        {
            ValueText = ValueText.Replace(' ', ',');
            return Enum.Parse(EnumType, ValueText, true);
        }

		/// <summary>
		/// Gets client info (does not check if the client exists)
		/// </summary>
		/// <param name="ClientName">Name of the client.</param>
		/// <returns></returns>
		public static P4ClientInfo GetClientInfoInternal(string ClientName)
		{
			P4ClientInfo Info = new P4ClientInfo();
			var P4Result = CommandUtils.P4(String.Format("client -o {0}", ClientName), AllowSpew: false);
			if (P4Result == 0)
			{
				var Tags = ParseTaggedP4Output(P4Result.Output);
                Info.Name = ClientName;
				Tags.TryGetValue("Host", out Info.Host);
				Tags.TryGetValue("Root", out Info.RootPath);
				if (!String.IsNullOrEmpty(Info.RootPath))
				{
					Info.RootPath = ConvertSeparators(PathSeparator.Default, Info.RootPath);
				}
				Tags.TryGetValue("Owner", out Info.Owner);
				string AccessTime;
				Tags.TryGetValue("Access", out AccessTime);
				if (!String.IsNullOrEmpty(AccessTime))
				{
					DateTime.TryParse(AccessTime, out Info.Access);
				}
				else
				{
					Info.Access = DateTime.MinValue;
				}
                string LineEnd;
                Tags.TryGetValue("LineEnd", out LineEnd);
                if (!String.IsNullOrEmpty(LineEnd))
                {
                    Info.LineEnd = (P4LineEnd)ParseEnumValues(LineEnd, typeof(P4LineEnd));
                }
                string ClientOptions;
                Tags.TryGetValue("Options", out ClientOptions);
                if (!String.IsNullOrEmpty(ClientOptions))
                {
                    Info.Options = (P4ClientOption)ParseEnumValues(ClientOptions, typeof(P4ClientOption));
                }
                string SubmitOptions;
                Tags.TryGetValue("SubmitOptions", out SubmitOptions);
                if (!String.IsNullOrEmpty(SubmitOptions))
                {
                    Info.SubmitOptions = (P4SubmitOption)ParseEnumValues(SubmitOptions, typeof(P4SubmitOption));
                }
                string ClientMappingRoot = "//" + ClientName;
                foreach (var Pair in Tags)
                {
                    if (Pair.Key.StartsWith("View", StringComparison.InvariantCultureIgnoreCase))
                    {
                        string Mapping = Pair.Value;
                        int ClientStartIndex = Mapping.IndexOf(ClientMappingRoot, StringComparison.InvariantCultureIgnoreCase);
                        if (ClientStartIndex > 0)
                        {
                            var ViewPair = new KeyValuePair<string, string>(
                                Mapping.Substring(0, ClientStartIndex - 1),
                                Mapping.Substring(ClientStartIndex + ClientMappingRoot.Length));
                            Info.View.Add(ViewPair);
                        }
                    }
                }
			}
			else
			{
				throw new AutomationException("p4 client -o {0} failed!", ClientName);
			}
			return Info;
		}

		/// <summary>
		/// Gets all clients owned by the user.
		/// </summary>
		/// <param name="UserName"></param>
		/// <returns>List of clients owned by the user.</returns>
		public static P4ClientInfo[] P4GetClientsForUser(string UserName)
		{
			CheckP4Enabled();

			var ClientList = new List<P4ClientInfo>();

			// Get all clients for this user
			var P4Result = CommandUtils.P4(String.Format("clients -u {0}", UserName), AllowSpew: false);
			if (P4Result != 0)
			{
				throw new AutomationException("p4 clients -u {0} failed.", UserName);
			}

			// Parse output.
			var Lines = P4Result.Output.Split(new string[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
			foreach (string Line in Lines)
			{
				var Tokens = Line.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
				P4ClientInfo Info = null;

				// Retrieve the client name and info.
				for (int i = 0; i < Tokens.Length; ++i)
				{
					if (Tokens[i] == "Client")
					{
						var ClientName = Tokens[++i];
						Info = CommandUtils.GetClientInfoInternal(ClientName);
						break;
					}
				}

				if (Info == null || String.IsNullOrEmpty(Info.Name) || String.IsNullOrEmpty(Info.RootPath))
				{
					throw new AutomationException("Failed to retrieve p4 client info for user {0}. Unable to set up local environment", UserName);
				}

				ClientList.Add(Info);
			}
			return ClientList.ToArray();
		}
        /// <summary>
        /// Deletes a client.
        /// </summary>
        /// <param name="Name">Client name.</param>
        /// <param name="Force">Forces the operation (-f)</param>
        public void DeleteClient(string Name, bool Force = false)
        {
            CheckP4Enabled();
            LogP4(String.Format("client -d {0} {1}", (Force ? "-f" : ""), Name));
        }

        /// <summary>
        /// Creates a new client.
        /// </summary>
        /// <param name="ClientSpec">Client specification.</param>
        /// <returns></returns>
        public P4ClientInfo CreateClient(P4ClientInfo ClientSpec)
        {
            string SpecInput = "Client: " + ClientSpec.Name + Environment.NewLine;
            SpecInput += "Owner: " + ClientSpec.Owner + Environment.NewLine;
            SpecInput += "Host: " + ClientSpec.Host + Environment.NewLine;
            SpecInput += "Root: " + ClientSpec.RootPath + Environment.NewLine;
            SpecInput += "Options: " + ClientSpec.Options.ToString().ToLowerInvariant().Replace(",", "") + Environment.NewLine;
            SpecInput += "SubmitOptions: " + ClientSpec.SubmitOptions.ToString().ToLowerInvariant().Replace(",", "") + Environment.NewLine;
            SpecInput += "LineEnd: " + ClientSpec.LineEnd.ToString().ToLowerInvariant() + Environment.NewLine;
            SpecInput += "View:" + Environment.NewLine;
            foreach (var Mapping in ClientSpec.View)
            {
                SpecInput += "\t" + Mapping.Key + " //" + ClientSpec.Name + Mapping.Value + Environment.NewLine;
            }
            Log(SpecInput);
            LogP4("client -i", SpecInput);
            return ClientSpec;
        }


		/// <summary>
		/// Lists immediate sub-directories of the specified directory.
		/// </summary>
		/// <param name="CommandLine"></param>
		/// <returns>List of sub-directories of the specified direcories.</returns>
		public static List<string> Dirs(string CommandLine)
		{
			CheckP4Enabled();
			var DirsCmdLine = String.Format("dirs {0}", CommandLine);
			var P4Result = CommandUtils.P4(DirsCmdLine, AllowSpew: false);
			if (P4Result != 0)
			{
				throw new AutomationException("{0} failed.", DirsCmdLine);
			}
			var Result = new List<string>();
			var Lines = P4Result.Output.Split(new string[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
			foreach (string Line in Lines)
			{
				if (!Line.Contains("no such file"))
				{
					Result.Add(Line);
				}
			}
			return Result;
		}

		#region Utilities

		private static object[] OldStyleBinaryFlags = new object[]
		{
			P4FileAttributes.Uncompressed,
			P4FileAttributes.Executable,
			P4FileAttributes.Compressed,
			P4FileAttributes.RCS
		};

		private static void ParseFileType(string Filetype, ref P4FileStat Stat)
		{
			var AllFileTypes = GetEnumValuesAndKeywords(typeof(P4FileType));
			var AllAttributes = GetEnumValuesAndKeywords(typeof(P4FileAttributes));

			Stat.Type = P4FileType.Unknown;
			Stat.Attributes = P4FileAttributes.None;

			// Parse file flags
			var OldFileFlags = GetEnumValuesAndKeywords(typeof(P4FileAttributes), OldStyleBinaryFlags);
			foreach (var FileTypeFlag in OldFileFlags)
			{
				if ((!String.IsNullOrEmpty(FileTypeFlag.Value) && Char.ToLowerInvariant(FileTypeFlag.Value[0]) == Char.ToLowerInvariant(Filetype[0]))
					// @todo: This is a nasty hack to get .ipa files to work - RobM plz fix?
					|| (FileTypeFlag.Value == "F" && Filetype == "ubinary"))
				{
					Stat.IsOldType = true;
					Stat.Attributes |= (P4FileAttributes)FileTypeFlag.Key;
					break;
				}
			}
			if (Stat.IsOldType)
			{
				Filetype = Filetype.Substring(1);
			}
			// Parse file type
			var TypeAndAttributes = Filetype.Split('+');
			foreach (var FileType in AllFileTypes)
			{
				if (FileType.Value == TypeAndAttributes[0])
				{
					Stat.Type = (P4FileType)FileType.Key;
					break;
				}
			}
			// Parse attributes
			if (TypeAndAttributes.Length > 1 && !String.IsNullOrEmpty(TypeAndAttributes[1]))
			{
				var FileAttributes = TypeAndAttributes[1];
				for (int AttributeIndex = 0; AttributeIndex < FileAttributes.Length; ++AttributeIndex)
				{
					char Attr = FileAttributes[AttributeIndex];
					foreach (var FileAttribute in AllAttributes)
					{
						if (!String.IsNullOrEmpty(FileAttribute.Value) && FileAttribute.Value[0] == Attr)
						{
							Stat.Attributes |= (P4FileAttributes)FileAttribute.Key;
							break;
						}
					}
				}
			}
		}

		private static P4Action ParseAction(string Action)
		{
			P4Action Result = P4Action.Unknown;
			var AllActions = GetEnumValuesAndKeywords(typeof(P4Action));
			foreach (var ActionKeyword in AllActions)
			{
				if (ActionKeyword.Value == Action)
				{
					Result = (P4Action)ActionKeyword.Key;
					break;
				}
			}
			return Result;
		}

		private static KeyValuePair<object, string>[] GetEnumValuesAndKeywords(Type EnumType)
		{
			var Values = Enum.GetValues(EnumType);
			KeyValuePair<object, string>[] ValuesAndKeywords = new KeyValuePair<object, string>[Values.Length];
			int ValueIndex = 0;
			foreach (var Value in Values)
			{
				ValuesAndKeywords[ValueIndex++] = new KeyValuePair<object, string>(Value, GetEnumDescription(EnumType, Value));
			}
			return ValuesAndKeywords;
		}

		private static KeyValuePair<object, string>[] GetEnumValuesAndKeywords(Type EnumType, object[] Values)
		{
			KeyValuePair<object, string>[] ValuesAndKeywords = new KeyValuePair<object, string>[Values.Length];
			int ValueIndex = 0;
			foreach (var Value in Values)
			{
				ValuesAndKeywords[ValueIndex++] = new KeyValuePair<object, string>(Value, GetEnumDescription(EnumType, Value));
			}
			return ValuesAndKeywords;
		}

		private static string GetEnumDescription(Type EnumType, object Value)
		{
			var MemberInfo = EnumType.GetMember(Value.ToString());
			var Atributes = MemberInfo[0].GetCustomAttributes(typeof(DescriptionAttribute), false);
			return ((DescriptionAttribute)Atributes[0]).Description;
		}

		private static string FileAttributesToString(P4FileAttributes Attributes)
		{
			var AllAttributes = GetEnumValuesAndKeywords(typeof(P4FileAttributes));
			string Text = "";
			foreach (var Attr in AllAttributes)
			{
				var AttrValue = (P4FileAttributes)Attr.Key;
				if ((Attributes & AttrValue) == AttrValue)
				{
					Text += Attr.Value;
				}
			}
			if (String.IsNullOrEmpty(Text) == false)
			{
				Text = "+" + Text;
			}
			return Text;
		}

		#endregion
	}
}
