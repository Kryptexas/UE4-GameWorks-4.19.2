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
</table>
<br />

<p>Displaying top 100 records, ordered by # of crashes</p>
<table style="width: 100%">
	<tr>
		<th>
			#
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
			Versions Affected
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
			<%=CSVRow.NumberOfCrashes%>
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
			<%=CSVRow.AffectedVersionsString%>
		</td>		
	</tr>
	<%
		}	
	%>

</table>

</form>
</body>