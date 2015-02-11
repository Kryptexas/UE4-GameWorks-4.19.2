<%-- // Copyright 1998-2015 Epic Games, Inc. All Rights Reserved. --%>

<%@ Control Language="C#" Inherits="System.Web.Mvc.ViewUserControl<ReportsViewModel>" %>
<%@ Import Namespace="Tools.CrashReporter.CrashReportWebSite.Models" %>

<style>

table, table tr, table th, table td
{
	border: solid;
	border-width: 1px;

	color: white;
	background-color: black;

	font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
	font-size: 12px;
}

</style>

<table >
	<tr>
		<td>
			TotalAnonymousCrashes:&nbsp;<%=Model.TotalAnonymousCrashes %>
		</td>
	</tr>

	<tr>
		<td>
			TotalUniqueAnonymousCrashes:&nbsp;<%=Model.TotalUniqueAnonymousCrashes %>
		</td>
	</tr>

	<tr>
		<td>
			TotalAffectedUsers:&nbsp;<%=Model.TotalAffectedUsers %>
		</td>
	</tr>
</table>
<br />
<%--
	
Number
NewBugg.Id = RealBugg.Id;								// CrashGroup URL (a link to the Bugg)
NewBugg.TTPID = RealBugg.TTPID;							// JIRA
NewBugg.FixedChangeList = RealBugg.FixedChangeList;		// FixCL
NewBugg.NumberOfCrashes = Top.Value;					// # Occurrences
NewBugg.AffectedBuilds = new SortedSet<string>();
NewBugg.BuildVersion = NewBugg.AffectedBuilds.Last();	// Latest Version Affected
NewBugg.NumberOfUniqueMachines = MachineIds.Count;		// # Affected Users
NewBugg.LatestCLAffected = ILatestCLAffected;			// Latest CL Affected
NewBugg.LatestOSAffected = LatestOSAffected;			// Latest Environment Affected
NewBugg.TimeOfFirstCrash = RealBugg.TimeOfFirstCrash;	// First Crash Timestamp	

#From XML JIRA	
FixVersion
Status
FixedCL


--%>
<table>
	<tr>
		<th>
			#
		</th>
		<th>
			URL
		</th>
		<th>
			JIRA
		</th>
		<th>
			FixCL
		</th>
		<th>
			# Occurrences
		</th>
		<th>
			Latest Version Affected
		</th>
		<th>
			# Affected Users
		</th>
		<th>
			Latest CL Affected
		</th>
		<th>
			Latest Environment Affected
		</th>
		<th>
			First Crash Timestamp
		</th>
	</tr>

	<%
		for( int Index = 0; Index < Model.Buggs.Count; Index ++ )
		{
			Bugg Bugg = Model.Buggs[Index];
	%>
	<tr>
		<td>
			<%--Number--%>
			<%=Index%>
		</td>
		<td>
			<%--NewBugg.Id = RealBugg.Id;								// CrashGroup URL (a link to the Bugg)--%>
			<%--<a href="https://jira.ol.epicgames.net/browse/<%=Model.Bugg.TTPID%>" target="_blank"><%=Model.Bugg.TTPID%></a>--%>
			<%=Html.ActionLink( Bugg.Id.ToString(), "Show", new { controller = "Buggs", id = Bugg.Id }, null )%>
		</td>
		<td>
			<%--NewBugg.TTPID = RealBugg.TTPID;							// JIRA--%>
			<a href="https://jira.ol.epicgames.net/browse/<%=Bugg.TTPID%>" target="_blank"><%=Bugg.TTPID%></a>
		</td>
		<td>
			<%--NewBugg.FixedChangeList = RealBugg.FixedChangeList;		// FixCL--%>
			<%=Bugg.FixedChangeList%>
		</td>
		<td>
			<%--NewBugg.NumberOfCrashes = Top.Value;					// # Occurrences--%>
			<%=Bugg.NumberOfCrashes%>
		</td>
		<td>
			<%--NewBugg.BuildVersion = NewBugg.AffectedBuilds.Last();	// Latest Version Affected--%>
			<%=Bugg.BuildVersion%>
		</td>
		<td>
			<%--NewBugg.NumberOfUniqueMachines = MachineIds.Count;		// # Affected Users--%>
			<%=Bugg.NumberOfUniqueMachines%>
		</td>
		<td>
			<%--NewBugg.LatestCLAffected = ILatestCLAffected;			// Latest CL Affected--%>
			<%=Bugg.LatestCLAffected%>
		</td>
		<td>
			<%--NewBugg.LatestOSAffected = LatestOSAffected;			// Latest Environment Affected--%>
			<%=Bugg.LatestOSAffected%>
		</td>
		<td>
			<%--NewBugg.TimeOfFirstCrash = RealBugg.TimeOfFirstCrash;	// First Crash Timestamp	--%>
			<%=Bugg.TimeOfFirstCrash%>
		</td>
	</tr>
	<%
		}	
	%>

</table>

<small>Generated in <%=Model.GenerationTime%> second(s)</small>