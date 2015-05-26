/*
 * Copyright (C) 2015 Epic Games, Inc. All Rights Reserved.
 */
package com.epicgames.replayserver;

import java.io.IOException;
import java.util.logging.Level;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;


public class StartUploading extends HttpServlet 
{
	private static final long serialVersionUID = 1L;

	public StartUploading()
	{		
	}

    protected void doPost( HttpServletRequest request, HttpServletResponse response ) throws ServletException, IOException
    {
    	try
    	{
        	final String appName 		= request.getParameter( "App" );
        	final String versionString 	= request.getParameter( "Version" );
        	final String clString 		= request.getParameter( "CL" );
    		final String friendlyName 	= request.getParameter( "Friendly" );
    		final String metaString 	= request.getParameter( "Meta" );
    		
    		if ( appName == null )
    		{
        		ReplayLogger.log( Level.SEVERE, "StartUploading. appName == null." );
                response.sendError( HttpServletResponse.SC_BAD_REQUEST );
                return;
    		}
        	
    		if ( versionString == null )
    		{
        		ReplayLogger.log( Level.SEVERE, "StartUploading. versionString == null." );
                response.sendError( HttpServletResponse.SC_BAD_REQUEST );
                return;
    		}

    		if ( clString == null )
    		{
        		ReplayLogger.log( Level.SEVERE, "StartUploading. clString == null." );
                response.sendError( HttpServletResponse.SC_BAD_REQUEST );
                return;
    		}

    		if ( friendlyName == null )
    		{
        		ReplayLogger.log( Level.SEVERE, "StartUploading. FriendlyName == null." );
                response.sendError( HttpServletResponse.SC_BAD_REQUEST );
                return;
    		}

    		final int version = Integer.parseUnsignedInt( versionString );
        	final int changelist = Integer.parseUnsignedInt( clString );

    		// Start a live session
    		final String SessionName = ReplayDB.createSession( appName, version, changelist, friendlyName, metaString );
      	
    		ReplayLogger.log( Level.FINE, "StartUploading. Success: " + appName + "/" + versionString + "/" + clString + "/" + SessionName + "/" + friendlyName + "/" + metaString );

    		response.setHeader( "Session",  SessionName );   		
    	}
    	catch ( ReplayException e )
    	{
    		ReplayLogger.log( Level.SEVERE, "StartUploading. ReplayException: " + e.getMessage() );
            response.sendError( HttpServletResponse.SC_BAD_REQUEST );
    	}
    	catch ( Exception e )
    	{
    		ReplayLogger.log( Level.SEVERE, "StartUploading. Exception: " + e.getMessage() );
    		e.printStackTrace();
            response.sendError( HttpServletResponse.SC_BAD_REQUEST );
    	}
    }
}
