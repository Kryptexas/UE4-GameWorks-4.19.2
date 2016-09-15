<%-- // Copyright 1998-2016 Epic Games, Inc. All Rights Reserved. --%>

<%@ Page Title="" Language="C#" MasterPageFile="~/Views/Shared/Site.Master" Inherits="System.Web.Mvc.ViewPage<Tools.CrashReporter.CrashReportWebSite.ViewModels.UsersViewModel>" %>

<asp:Content ID="StyleSheet" ContentPlaceHolderID="CssContent" runat="server">
	<link href="../../Content/Site.css" rel="stylesheet" type="text/css" />
   
</asp:Content>

<asp:Content ID="TitleContent" ContentPlaceHolderID="TitleContent" runat="server">
[CR] Edit the groups the users belong to
</asp:Content>

<asp:Content ID="ScriptContent" ContentPlaceHolderID="ScriptContent" runat="server">
     <script src="/Scripts/UserFilterScript.js" type="text/javascript"></script>
</asp:Content>

<asp:Content ID="AboveMainContent"  ContentPlaceHolderID="AboveMainContent" runat="server" >
</asp:Content>

<asp:Content ID="MainContent" ContentPlaceHolderID="MainContent" runat="server">
	<div id='UserGroupBar'>
		<%foreach( var GroupCount in Model.GroupCounts )
		 {%>
			<span class=<%if( Model.UserGroup == GroupCount.Key )
			 { %> "UserGroupTabSelected" <% }
				 else
			 { %>"UserGroupTab"<% } %> id="<%=GroupCount.Key%>Tab">
				<%=Url.UserGroupLink( GroupCount.Key, Model )%>
			<span class="UserGroupResults">
				(<%=GroupCount.Value%>)
			</span>
		</span>
		<%} %>
	</div>
    <div id="UserNameSearchFilter" style="margin:10px auto 10px; width:460px;">
        <p>Enter a user name here to quickly change their usergroup assignment.</p>
        <form action="<%="/Users/Index/" + Model.UserGroup %>" method="POST" id="UserNamesForm" style="text-align: center" title="Enter a user name here to quickly change their usergroup assignment." >
            <label>User Filter : </label><%=Html.TextBox("UserName", Model.User)%>
            <%=Html.DropDownList( "UserGroup", Model.GroupSelectList)%>
            <input type="submit" name="submit" value="submit"/>
        </form>
    </div>
	<div id="UserNames">
		<%foreach( var UserName in Model.Users )
		{%>
        <form action="<%="/Users/Index/" + Model.UserGroup %>" method="POST" id="UserNamesForm" style="text-align: center" title="Enter a user name here to quickly change their usergroup assignment." >
            <%=Html.Hidden("UserName", UserName.Name) %>
            <span style="color:#c3cad0;"><%=UserName.Name %></span> <%=Html.DropDownList( "UserGroup", Model.GroupSelectList)%>
            <input type="submit" name="submit" value="submit"/>
        </form>
		<% } %>
	</div>
    
<div class="PaginationBox">
	<%=Html.PageLinks( Model.PagingInfo, i => Url.Action( "", new 
		{ 
			page = i,
            userGroup = Model.UserGroup
        }) )%>
	<div id="clear"></div>
</div>

</asp:Content>


