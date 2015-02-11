<%-- // Copyright 1998-2015 Epic Games, Inc. All Rights Reserved. --%>

<%@ Control Language="C#" Inherits="System.Web.Mvc.ViewUserControl<BuggsViewModel>" %>
<%@ Import Namespace="Tools.CrashReporter.CrashReportWebSite.Models" %>

<style>

table, tr, td, th
{
	border: solid;
	border-width: 1px;

	color: white;

	font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
	font-size: 12px;
}

</style>

<table>
	<tr>
		<td>
			TotalAnonymousCrashes:&nbsp;<%Model.TotalAnonymousCrashes %>
		</td>
	</tr>

	<tr>
		<td>
			TotalUniqueAnonymousCrashes:&nbsp;<%Model.TotalUniqueAnonymousCrashes %>
		</td>
	</tr>

	<tr>
		<td>
			TotalAffectedUsers:&nbsp;<%Model.TotalAffectedUsers %>
		</td>
	</tr>
</table>
