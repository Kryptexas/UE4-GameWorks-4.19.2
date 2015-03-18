<Query Kind="Program">
  <Connection>
    <ID>e40cf789-b18a-4b65-bae6-a0f5440c1ce2</ID>
    <Persist>true</Persist>
    <Server>db-09</Server>
    <Database>CrashReport</Database>
    <IsProduction>true</IsProduction>
  </Connection>
  <Output>DataGrids</Output>
</Query>

// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

// Removes all crashes without pattern.
// Removes all crashes without branch
// Crashes with invalid callstack are removed
//	First pattern is set to null to disconnect from a bugg
//  Crash is removed in the next iteration of the script

static string InvalidFuncsStr ="63039+63164+63165+63391+63392+63164+63165+63391+63392+63039+63039+63164+69428+69429+69430+103713+70829+63164+79166+69432+69433+69434+69435+69436+63164+69428+98671+69430+70883+69432+69433+69434+69435+69436+63164+69428";
static List<string> InvalidModules = new List<string>(){"uxtheme!","user32!","ntdll!","kernel32!","KERNELBASE!","ntdll!","wer!","<Unknown>"};

HashSet<int> GetInvalidFuncIds()
{
	HashSet<int> Result = new HashSet<int>();
	var SplitString = InvalidFuncsStr.Split("+".ToCharArray(), StringSplitOptions.RemoveEmptyEntries );
	foreach (var Id in SplitString)
	{
		int IntId = int.Parse(Id);
		Result.Add( IntId);
	}
	return Result;
}

static Stopwatch Timer = Stopwatch.StartNew();
static DateTime Date = DateTime.UtcNow.AddMonths(-2);
static TimeSpan Tick = TimeSpan.FromDays(3);
const int NUM_OPS_PER_BATCH = 32;

void WriteLine( string Line )
{
	Debug.WriteLine( string.Format( "{0,5}: {1}", (long)Timer.Elapsed.TotalSeconds, Line ) );
}

int NoPatternTotal = 0;

void DeleteNoPatternBatch( List<Crashes> NoPatternBatch )
{
	if (NoPatternBatch.Count>0)
	{
		Crashes.DeleteAllOnSubmit( NoPatternBatch );	
		Crashes.Context.SubmitChanges();
		NoPatternTotal += NoPatternBatch.Count;
		WriteLine( string.Format( "NoPattern deleted: {0}", NoPatternTotal ) );
		NoPatternBatch.Clear();
	}
}

int BuggCrashTotal = 0;

void DeleteBuggCrash( List<Buggs_Crashes> BuggCrashBatch )
{
	try
	{
		if (BuggCrashBatch.Count>0)
		{
			Buggs_Crashes.DeleteAllOnSubmit( BuggCrashBatch );
			Buggs_Crashes.Context.SubmitChanges();
			BuggCrashTotal += BuggCrashBatch.Count;
			WriteLine( string.Format( "BuggCrashBatch deleted: {0}", BuggCrashTotal ) );
			BuggCrashBatch.Clear();
		}
	}
	catch( Exception E )
	{
		WriteLine( E.ToString() );
	}
}


int BuggsTotal = 0;

void DeleteBuggs( List<int> BuggsBatch )
{
	if (BuggsBatch.Count>0)
	{
		var Batch = Buggs.Where (b => BuggsBatch.Contains(b.Id));
		BuggsTotal += Batch.Count (b => b.Id>0);
		Buggs.DeleteAllOnSubmit( Batch );
		Buggs.Context.SubmitChanges();
		WriteLine( string.Format( "DeleteBuggs deleted: {0}", BuggsTotal ) );
		BuggsBatch.Clear();
	}
}

// Deletes all crashes without pattern.
void DeleteNoPatternAll()
{
	var NoPattern = Crashes
	.Where( c => c.Pattern == null )
	.Where (c => c.TimeOfCrash> Date)
	.ToList();
	
	var CrashesIds = new HashSet<int>( NoPattern.Select (np => np.Id).ToList() );
	
	int NumBuggCrass = Buggs_Crashes.Count (bc => bc.BuggId>0);
	
	// Unlink from the bugg.	
	List<Buggs_Crashes> ToRemove = new List<Buggs_Crashes>();
	HashSet<int> BuggIds = new HashSet<int>();
	for (int n = 0; n < NoPattern.Count; n ++)
	{
		var Crash = NoPattern[n];
	
		var BC = Buggs_Crashes.Where (bc => bc.CrashId==Crash.Id).FirstOrDefault();
		if(BC!=null)
		{
			ToRemove.Add(BC);
			BuggIds.Add(BC.BuggId);
		}
		
		if (ToRemove.Count == NUM_OPS_PER_BATCH)
		{
			DeleteBuggCrash(ToRemove);
		}
	}
	
	DeleteBuggCrash(ToRemove);
	
	List<int> BuggsToRemove = new List<int>();
	foreach (var BuggId in BuggIds)
	{
		var CrashesForBugg = Buggs_Crashes.Where (bc => bc.BuggId==BuggId).Select (bc => bc.CrashId);
		int CrashCount = CrashesForBugg.Count (cfb => cfb>0);
		if(CrashCount==0)
		{
			// Remove the bugg.
			BuggsToRemove.Add(BuggId);
		}
		
		if (BuggsToRemove.Count > NUM_OPS_PER_BATCH)
		{
			DeleteBuggs( BuggsToRemove );
		}
	}
	
	DeleteBuggs( BuggsToRemove );
	
	WriteLine( string.Format( "NoPattern: {0}", NoPattern.Count ) );
	
	var NoPatternBatch = new List<Crashes>(NUM_OPS_PER_BATCH);
	for (int n = 0; n < NoPattern.Count; n ++)
	{
		NoPatternBatch.Add(NoPattern[n]);
		
		if (NoPatternBatch.Count == NUM_OPS_PER_BATCH)
		{
			DeleteNoPatternBatch(NoPatternBatch);
		}
	}
	
	// Last batch
	DeleteNoPatternBatch(NoPatternBatch);
}

// Sets patterns for all crashes without branch, will be deleted after running the script again.
void SetNoPatternForNoBranchAll()
{
	var NoBranch = Crashes
	.Where( c => c.Branch == null )
	.Where (c => c.TimeOfCrash> Date)
	.ToList();
	
	WriteLine( string.Format( "NoBranch: {0}", NoBranch.Count ) );
	
	var NoBranchBatch = new List<Crashes>(NUM_OPS_PER_BATCH);
	for (int n = 0; n < NoBranch.Count; n ++)
	{
		NoBranchBatch.Add(NoBranch[n]);		
		NoBranch[n].Pattern = null;
		NoBranch[n].Branch = "INVALID";
		
		if (n % NUM_OPS_PER_BATCH == 0)
		{
			SumbitChanges(NoBranchBatch);
		}
	}
	
	// Last batch
	SumbitChanges(NoBranchBatch);
}

void SumbitChanges( List<Crashes> CrashesBatch )
{
	//Crashes.InsertAllOnSubmit( CrashesBatch );
	Crashes.Context.SubmitChanges();
	WriteLine( string.Format( "SubmitBatchChanges: {0}", CrashesBatch.Count ) );
}

class CustomFuncComparer : IEqualityComparer<string>
{
    public bool Equals(string x, string y)
    {
        return y.StartsWith( x, StringComparison.InvariantCultureIgnoreCase );
    }

    public int GetHashCode(string obj)
    {
        return obj.GetHashCode();
    }
}

void SetNoPatternForInvalidPattern()
{
	//HashSet<int> InvalidFuncs = GetInvalidFuncIds();
	var AllFunctionNames = FunctionCalls
	//.Where (fc => InvalidModules.ToList().Contains(fc.Call, new CustomFuncComparer()))
	.ToList();
	
	var InvalidFuncs = new List<FunctionCalls>();
	foreach( var Func in AllFunctionNames )
	{
		bool bInvalid = InvalidModules.Contains(Func.Call, new CustomFuncComparer() );
		if (bInvalid)
		{
			InvalidFuncs.Add(Func);
		}
	}
	
	var InvalidFuncIds = new HashSet<int>();
	foreach (var Func in InvalidFuncs)
	{
		InvalidFuncIds.Add(Func.Id);
	}

	InvalidFuncs.OrderBy (i => i.Call);
	InvalidFuncIds.OrderBy (ifi => ifi);
	
	DateTime StartDate = Date;
	DateTime EndDate = Date.Add( Tick );
	while( true )
	{
		var AllCrashes = Crashes
		.Where (c => c.TimeOfCrash > StartDate && c.TimeOfCrash <= EndDate)
		.Where (c => c.Pattern != null)
		.ToList();
		
		if( AllCrashes.Count == 0 )
		{
			break;
		}
		
		WriteLine( string.Format( "AllCrashes: {0} {1} -> {2}", AllCrashes.Count, StartDate.ToString("yyyy-MM-dd"), EndDate.ToString("yyyy-MM-dd") ) );
		StartDate = EndDate;
	 	EndDate = EndDate.Add( Tick );
	
		var InvalidCrashes = new List<Crashes>();
		int CrashIndex = 0;
		foreach (var Crash in AllCrashes)
		{
			
			HashSet<int> PatternIds = new HashSet<int>();
			if(Crash.Pattern==null)
			{
				continue;
			}
			
			var SplitString = Crash.Pattern.Split("+".ToCharArray(), StringSplitOptions.RemoveEmptyEntries );
			foreach (var Id in SplitString)
			{
				int IntId = int.Parse(Id);
				PatternIds.Add( IntId);
			}
		
			int NumInvalidInPattern = 0;
			foreach (var Id in PatternIds)
			{
				if (InvalidFuncIds.Contains(Id))
				{
					NumInvalidInPattern++;
				}
			}
		
			bool bInvalid = NumInvalidInPattern>0 && (PatternIds.Count - NumInvalidInPattern) < 4;
			if (bInvalid)
			{
				CrashIndex++;
				InvalidCrashes.Add(Crash);
				Crash.Pattern = null;
			}
		}
		
		if (InvalidCrashes.Count > 0)
		{
			WriteLine( string.Format( "InvalidCrashes: {0}", InvalidCrashes.Count ) );
			SumbitChanges(InvalidCrashes);
			InvalidCrashes.Clear();
		}
	}
}

void Main()
{	
	int NumCrashes = Crashes.Count (c => c.Id>0);
	int MaxCrashId = Crashes.OrderByDescending (c => c.TimeOfCrash).Take(1).FirstOrDefault().Id;
	int Diff = MaxCrashId-NumCrashes;
	WriteLine( string.Format( "NumCrashes: {0}", NumCrashes ) );
	WriteLine( string.Format( "MaxCrashId: {0}", MaxCrashId ) );
	WriteLine( string.Format( "Diff:       {0}", Diff ) );
	SetNoPatternForNoBranchAll();
	SetNoPatternForInvalidPattern();
	DeleteNoPatternAll();
}

// Define other methods and classes here