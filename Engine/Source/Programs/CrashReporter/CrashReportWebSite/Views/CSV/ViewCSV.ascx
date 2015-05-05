<%-- // Copyright 1998-2015 Epic Games, Inc. All Rights Reserved. --%>

<%@ Control Language="C#" Inherits="System.Web.Mvc.ViewUserControl<CSV_ViewModel>" %>
<%@ Import Namespace="Tools.CrashReporter.CrashReportWebSite.Models" %>
<%@ Import Namespace="Tools.CrashReporter.CrashReportWebSite.Controllers" %>

<style>

table, table tr, table th, table td
{
	border: solid;
	border-width: 1px;

	color: white;
	background-color: black;

	font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
	font-size: 12px;

	padding: 2px;
	margin: 2px;
}

</style>
	
<body>
<form runat="server">
	
<table >
	<tr>
		<td>
			Total Crashes:&nbsp;<%=Model.TotalCrashes %>
		</td>
	</tr>
	<tr>
		<td>
			Total Crashes YearToDate:&nbsp;<%=Model.TotalCrashesYearToDate %>
		</td>
	</tr>

	<tr>
		<td>
			<strong>
			Filtered = CrashOrAssert, Anonymous, EpicId != 0, BuggId != 0,
			</strong>
		</td>
	</tr>

	<tr>
		<td>
			Total Unique Crashes:&nbsp;<%=Model.UniqueCrashesFiltered %>
		</td>
	</tr>
	<tr>
		<td>
			Total Affected Users:&nbsp;<%=Model.AffectedUsersFiltered %>
		</td>
	</tr>
		

	<tr>
		<td>
			<strong>
			Filtered = CrashOrAssert, Anonymous, EpicId != 0, BuggId != 0, TimeOfCrash within the time range, from <%=Model.DateTimeFrom.ToString()%> to <%=Model.DateTimeTo.ToString()%>
			</strong>
		</td>
	</tr>

	<tr>
		<td>
			Crashes:&nbsp;<%=Model.CrashesFilteredWithDate %>
		</td>
	</tr>


	<tr>
		<td>
			File:&nbsp;<a href="<%=Model.CSVPathname%>" target="_blank"><%=Model.GetCSVFilename()%></a>
		</td>
	</tr>

	<tr>
		<td>
			Directory:&nbsp;<a href="<%=Model.GetCSVDirectory()%>" target="_blank"><%=Model.GetCSVDirectory()%></a>
		</td>
	</tr>

	<tr>
		<td title="Copy the link and open in the explorer">
			Public Directory:&nbsp;<strong><%=Model.GetCSVPublicDirectory() %></strong>
		</td>
	</tr>
</table>
<br />

<p>Displaying top 100 records, ordered by the date of a crash</p>
<table style="width: 100%">
	<tr>
		<th>
			Time of crash
		</th>
		<th>
			EpicId
		</th>
		<%--<th>
			User's email
		</th>--%>
		<th>
			BuggId
		</th>
		<th>
			Engine version
		</th>
	</tr>

	<%
		for (int Index = 0; Index < Model.CSVRows.Count; Index++)
		{
			FCSVRow CSVRow = Model.CSVRows[Index];
			if( Index > 100 )
			{
				break;
			}
	%>
	<tr>
		<td>
			<%=CSVRow.TimeOfCrash.ToString()%>
		</td>
		<td>
			<%=CSVRow.EpicId%>
		</td>
		<%--<td>
			<%=CSVRow.UserEmail%>
		</td>--%>
		<td>
			<%=CSVRow.BuggId%>
		</td>
		<td>
			<%=CSVRow.EngineVersion%>
		</td>		
	</tr>
	<%
		}	
	%>

</table>

</form>
</body>