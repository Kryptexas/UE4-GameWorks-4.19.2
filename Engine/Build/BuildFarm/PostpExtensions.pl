# These are patterns we want to ignore
@::gExcludePatterns = (
	# UAT spam that just adds noise
	".*Exception in AutomationTool: BUILD FAILED.*",
	"RunUAT ERROR: AutomationTool was unable to run successfully.",

	# Linux error caused by linking against libfbx
	".*the use of `tempnam' is dangerous.*",

	# Warning about object files not definining any public symbols in non-unity builds
	".*warning LNK4221:.*",

#	".*ERROR: The process.*not found",
#	".*ERROR: This operation returned because the timeout period expired.*",
#	".*Sync.VerifyKnownFileInManifest: ERROR:.*",
#	".*to reconcile.*",
#	".*Fatal Error: Failed to initiate build.*",
#	".*Error: 0.*",
#	".*build system error(s).*",
#	".*VerifyFilesExist.*",
#	".* Error: (.+) replacing existing signature",
#	".*0 error.*",
#	".*ERROR: Unable to load manifest file.*",
#	".*Unable to delete junk file.*",
#	".*SourceControl.SyncInternal: ERROR:.*",
#	".*Error: ACDB.*",
#	".*xgConsole: BuildSystem failed to start with error code 217.*",
#	".*error: Unhandled error code.*",
#	".*621InnerException.*",
#	".* GUBP.PrintDetailedChanges:.*",
#	".* Failed to produce item:.*",
#	".*major version 51 is newer than 50.*",
#	"Cannot find project '.+' in branch",
#	".*==== Writing to template target file.*",
#	".*==== Writing to OBB data file.*",
#	".*==== Writing to shim file.*",
#	".*==== Template target file up to date so not writing.*",
#	".*==== Shim data file up to date so not writing.*",
#	".*Initialzation failed while creating the TSF thread manager",
#	".*SignTool Error: The specified timestamp server either could not be reached or.*",
#	".*SignTool Error: An error occurred while attempting to sign:.*",
); 

# TCL rules match any lines beginning with '====', which are output by Android toolchain.
$::gDontCheck .= "," if $::gDontCheck;
$::gDontCheck .= "tclTestFail,tclClusterTestTimeout";

# These are patterns we want to process
# NOTE: order is important because the file is processed line by line 
# After an error is processed on a line that line is considered processed
# and no other errors will be found for that line, so keep the generic
# matchers toward the bottom so the specific matchers can be processed.
# NOTE: also be aware that the look ahead lines are considered processed

unshift @::gMatchers, (
    {
        id =>               "clErrorMultiline",
        pattern =>          q{([^(]+)(\([\d,]+\))? ?: (fatal )?error [a-zA-Z]+[\d]+},
        action =>           q{incValue("errors"); my ($file_only) = ($1 =~ /([^\\\\]+)$/); diagnostic($file_only || $1, "error", 0, forwardWhile("^    "))},
    },
    {
        id =>               "clWarningMultiline",
        pattern =>          q{([^(]+)\([\d,]+\) ?: warning },
        action =>           q{incValue("warnings"); my ($file_only) = ($1 =~ /([^\\\\]+)$/); diagnostic($file_only || $1, "warning", 0, forwardWhile("^    "))},
    },
    {
        id =>               "ubtFailedToProduceItem",
        pattern =>          q{ERROR: UBT ERROR: Failed to produce item: },
		action =>           q{incValue("errors"); diagnostic("UnrealBuildTool", "error")}
    },
    {
        id =>               "changesSinceLastSucceeded",
        pattern =>          q{\*\*\*\* Changes since last succeeded},
		action =>           q{$::gCurrentLine += forwardTo('^\\\*', 1);}
    },
	{
		id =>               "editorLogChannelError",
		pattern =>          q{^([a-zA-Z_][a-zA-Z0-9_]*):Error: },
		action =>           q{incValue("errors"); diagnostic($1, "error")}
	},
	{
		id =>               "editorLogChannelWarning",
		pattern =>          q{^([a-zA-Z_][a-zA-Z0-9_]*):Warning: },
		action =>           q{incValue("warnings"); diagnostic($1, "warning")}
	},
	{
		id =>               "editorLogChannelDisplay",
		pattern =>          q{^([a-zA-Z_][a-zA-Z0-9_]*):Display: },
		action =>           q{ }
	}
);

push @::gMatchers, (
	{
		id => "AutomationToolException",
		pattern => q{AutomationTool terminated with exception:},
		action => q{incValue("errors"); diagnostic("$1", "error", 0,  forwardTo(q{AutomationTool exiting with ExitCode=}));}
	},	
	{
		id => "UnableToConnectToVerisignSigningServer",
		pattern => q{Failed to sign executables .+ times over a period of .+},
		action => q{incValue("errors"); diagnostic("$1", "error", 0,  forwardTo(q{returned an invalid response.}));}
	},	
	{
		id => "VerisignTimestampServerConnectionError",
		pattern => q{The specified timestamp server either could not be reached or},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 5);}
	},
	{
		id => "OverlappingBuildProducts",
		pattern => q{Overlapping build product:},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, forwardTo(q{Stacktrace:}));}
	},
	{
		id => "CookCommandlet",
		pattern => q{:Error:},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, forwardTo(q{0}));}
	},
	{
		id => "clang",
		pattern => q{clang: error:},
		action => q{incValue("errors"); diagnostic("$1", "error", -5, forwardTo(q{0}));}
	},
	{
		id => "symbols",
		pattern => q{Undefined symbols for architecture},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, forwardTo(q{ld: symbol}));}
	},
	{
		id => "couldNotOpen",
		pattern => q{Error: Could not open:},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, forwardTo(q{0}));}
	},
	{
		id => "errorError",
		pattern => q{Error: Error},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, forwardTo(q{0}));}
	},
	{
		id => "APIDocFail",
		pattern => q{Unhandled Exception: System.IO.DirectoryNotFoundException:},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "ProjFind",
		pattern => q{Exception in AutomationTool: Cannot find project},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "buildMissing",
		pattern => q{build is missing or did not complete},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "duplicateNode",
		pattern => q{Attempt to add a duplicate node},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "LogError",
		pattern =>q{LogWindows: === Critical error: ===},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, forwardTo(q{Executing StaticShutdownAfterError}));}
	},
	{
		id => "tempStore",
		pattern => q{Could not create an appropriate shared temp folder},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "noClientLog",
		pattern => q{Client exited without creating a log file},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "UATException",
		pattern => q{ERROR: Exception in AutomationTool:},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "LNK",
		pattern => q{LINK : fatal error},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "NoLog",
		pattern => q{Client exited without creating a log file},
		action => action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "link",
		pattern => q{Error executing},
		action => q{incValue("errors"); diagnostic("$1", "error", -1, 0);}
	},
	{
		id => "failedToMove",
		pattern => q{Exception in AutomationTool: Failed to rename/move file},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "MSCORLIB",
		pattern => q{Exception in mscorlib},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "ObjectRef",
		pattern => q{Object reference not set to an instance of an object},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "XGEFailed",
		pattern => q{XGE failed on try 1},
		action => q{incValue("errors"); diagnostic("$1", "error", -5, 0);}
	},
	{
		id => "UHTfailed",
		pattern => q{UnrealHeaderTool failed for target},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "InvalidAsset",
		pattern => q{The following asset pack names are invalid},
		q{incValue("errors"); diagnostic("$1", "error", 0, forwardTo(q{StackTrace}));}
	},
	{
		id => "Distill",
		pattern => q{Sample uproject should have already been distilled to the correct location},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "ExitWithoutLog",
		pattern => q{Client exited without creating a log file},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "failedtoconvert",
		pattern => q{failed to convert due to errors},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "MSBuildError",
		pattern => q{targets\(\d+,\d+\): error},
		action => q{incValue("errors"); diagnostic("$1", "error", -2, 0);}
	},
	{
		id => "ChangesSinceLastGreen",
		pattern => q{PrintChanges:},
		action => q{incValue("info"); diagnostic("$1", "info", 0, forwardTo(q{GUBP.PrintDetailedChanges: Took}));}
	},
	{
		id => "Timeout",
		pattern => q{ectool: ectool timeout: The command did not complete before the requested timeout},
		action => q{incValue("errors"); diagnostic("$1", "error",-1, forwardTo(q{ExitCode=}));}
	},
	{
		id => "UnrealBuildToolException",
		pattern => q{UnrealBuildTool Exception:},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 5);}
	},
	{
		id => "ExitCode3",
		pattern => q{ExitCode=3},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "SegmentationFault",
		pattern => q{ExitCode=139},
		action => q{incValue("errors"); diagnostic("$1", "error", -4, 1);}
	},
	{
		id => "ExitCode255",
		pattern => q{ExitCode=255},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 0);}
	},
	{
		id => "MissingFileError",
		pattern => q{build is missing or did not complete because this file is missing:},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 1);}
	},
	{
		id => "DerivedDataCacheError",
		pattern => q{Failed while running DerivedDataCache},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, 1);}
	},
	{
		id => "Stack dump",
		pattern => q{Stack dump},
		action => q{incValue("errors"); diagnostic("$1", "error", -4, forwardWhile('0', 1));}
	},
	{
		id => "UATErrStack",
		pattern => q{AutomationTool: Stack:},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, forwardTo(q{AutomationTool: \n}));}
	},
	{
		id => "UATErrStack",
		pattern => q{begin: stack for UAT},
		action => q{incValue("errors"); diagnostic("$1", "error", 0, forwardTo(q{end: stack for UAT}));}
	},
	{
		id => "BuildFailed",
		pattern => q{BuildCommand.Execute: ERROR:},
		action => q{incValue("errors"); diagnostic("Build Failed", "error", 0, 0);}
	},
	{
		id => "StraightFailed",
		pattern => q{BUILD FAILED},
		action => q{incValue("errors");}
	}
);

1;
