<%-- // Copyright 1998-2014 Epic Games, Inc. All Rights Reserved. --%>

<%@ Page Title="" Language="C#" MasterPageFile="~/Views/Shared/Site.Master" Inherits="System.Web.Mvc.ViewPage<BuggsViewModel>" %>
<%@ Import Namespace="Tools.CrashReporter.CrashReportWebSite.Models" %>

<asp:Content ID="StyleSheet" ContentPlaceHolderID="CssContent" runat="server">
	<link href="../../Content/Site.css" rel="stylesheet" type="text/css" />
</asp:Content>

<asp:Content ID="TitleContent" ContentPlaceHolderID="TitleContent" runat="server">
	Crash Report: Buggs
</asp:Content>

<asp:Content ID="ScriptContent"  ContentPlaceHolderID="ScriptContent" runat="server" >
<script type="text/javascript">
	$(function () {
	    $("#dateFromVisible")
            .datepicker({ maxDate: '+0D' })
            .datepicker('setDate', new Date(parseInt($('#dateFrom').val())));

	    $("#dateToVisible")
            .datepicker({ maxDate: '+0D' })
            .datepicker('setDate', new Date(parseInt($('#dateTo').val())));
	});

	$.datepicker.setDefaults({
		onSelect: function () {
		    $("#dateFrom").val($("#dateFromVisible").datepicker('getDate').getTime());
		    $("#dateTo").val($("#dateToVisible").datepicker('getDate').getTime());
		    $("#FilterBuggsForm").submit();
		}
	});

	$(document).ready(function () {
		//Shift Check box
		$(":checkbox").shiftcheckbox();

		//Zebrastripes
		$("#CrashesTable tr:nth-child(even)").css("background-color", "#C3CAD0");
		$("#CrashesTable tr:nth-child(odd)").css("background-color", "#eeeeee");
	});
</script>
</asp:Content>

<asp:Content ID="AboveMainContent" ContentPlaceHolderID="AboveMainContent" runat="server" >
<div id='SearchForm' style="clear:both;">
<%using( Html.BeginForm( "", "Buggs", FormMethod.Get, new { id = "FilterBuggsForm" } ) )
{ %>
	<%=Html.HiddenFor( u => u.UserGroup )%>
	<%=Html.Hidden( "SortTerm", Model.Term )%>
	<%=Html.Hidden( "SortOrder", Model.Order )%>

	<div id="SearchBox" ><%=Html.TextBox( "SearchQuery", Model.Query, new { width = "1000" })%><input type="submit" value="Search" class='SearchButton' /></div>
	
	<script>$.datepicker.setDefaults($.datepicker.regional['']);</script>

	<span style="margin-left: 10px; font-weight:bold;">Filter by Date </span>
	<span>From: <input id="dateFromVisible" type="text" class="date" AUTOCOMPLETE=OFF /></span>
	<input id="dateFrom" name="dateFrom" type="hidden" value="<%=Model.DateFrom %>" AUTOCOMPLETE=OFF />
	<span>From: <input id="dateToVisible" type="text" class="date" AUTOCOMPLETE=OFF /></span>
	<input id="dateTo" name="dateTo" type="hidden" value="<%=Model.DateTo %>" AUTOCOMPLETE=OFF />
<%} %>
</div>
</asp:Content>

<asp:Content ID="MainContent" ContentPlaceHolderID="MainContent" runat="server">
	<div id='CrashesTableContainer'>
		<div id='UserGroupBar'>
			<%foreach( var GroupCount in Model.GroupCounts )
			{%>
				<span class=<%if( Model.UserGroup == GroupCount.Key ){ %> "UserGroupTabSelected" <%} else {%> "UserGroupTab"<%} %> id="<%=GroupCount.Key%>Tab">
					<%=Url.UserGroupLink( GroupCount.Key, Model )%> 
				<span class="UserGroupResults">
					(<%=GroupCount.Value%>)
				</span>
			</span>
			<%} %>
		</div>

		<div id='CrashesForm'>
			<form action="/" method="POST">
				<div style="background-color: #E8EEF4; margin-bottom: -5px; width: 19.7em;">
					<span style="background-color: #C3CAD0; font-size: medium; padding: 0 1em;"><%=Html.ActionLink( "Crashes", "Index", "Crashes", new { SearchQuery = Model.Query, SortTerm = Model.Term, SortOrder = Model.Order, UserGroup = Model.UserGroup, DateFrom = Model.DateFrom, DateTo = Model.DateTo }, new { style = "color:black; text-decoration:none;" } )%></span>
					<span style="background-color: #E8EEF4; font-size: medium; padding:0 1em;" title='A Bugg is a collection of Crashes that have the exact same callstack.'><%=Html.ActionLink( "CrashGroups", "Index", "Buggs", new { SearchQuery = Model.Query, SortTerm = Model.Term, SortOrder = Model.Order, UserGroup = Model.UserGroup, DateFrom = Model.DateFrom, DateTo = Model.DateTo }, new { style = "color:black; text-decoration:none;" } )%></span>
				</div>
				<% Html.RenderPartial("/Views/Buggs/ViewBuggs.ascx"); %>
			</form>
		</div>
	</div>

	<div class="PaginationBox">
		<%=Html.PageLinks( Model.PagingInfo, i => Url.Action( "", new { page = i, SearchQuery = Model.Query, SortTerm = Model.Term, SortOrder = Model.Order, UserGroup = Model.UserGroup, DateFrom = Model.DateFrom, DateTo = Model.DateTo } ) )%>
		<div id="clear"></div>
	</div>
</asp:Content>
