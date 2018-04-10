The files in this folder are all the files you need to upload to your web server.


The following will help explain what kinds of files these are.


Development Build
=================

Packaging "Development" builds:
	* Only uncompressed files are built
	* Jump down to "Local Testing" to test your project


Shipping Build
==============

Packaging "Shipping" builds:
	* Can be built with or without uncompressed files
	* Jump down to "Local Testing" to test your project

To package "Shipping" build with "compression" enabled, set the following option in UE4Editor:
		Project Settings ->
		Platforms ->
		HTML5 ->
		Packaging ->
		Compress files during shipping packaging



Non-Compressed Files
====================
Use this setting (available on Development or Shipping builds) for the best results on being able to serve your project from most web host providers.

	Files Required for Non-Compressed Files Deployment
	--------------------------------------------------
	<project>.js         - main project Javascript code
	<project>.wasm       - main game code
	<project>.data       - game content
	<project>.data.js    - game content Javascript driver
	<project>.html       - landing page
	<project>.symbols    - symbols (optional: used with debugging)
	Utility.js           - utility Javascript code
	.htaccess            - "distributed configuration file"


Compressed Files (Advanced option)
================
Use this setting (only available on Shipping builds) if you need to help make your project downloads smaller.

	* NOTE: We recommend you smash your texture sizes for the best results -- this may be enough to NOT require using the compression option.
		+ For more information on how to do this, see:
			> https://docs.unrealengine.com/latest/INT/Platforms/Mobile/Textures/
			> For HTML5, try setting Max LOD Size to 256 and go up from there to ensure quality and download size are acceptable.


If you are an advanced web server administrator:
	* Using compression helps improve download times.
	* Using pre-compressed files may help your web server from compressing the files dynamically every time your project is loaded on to a browser.
	* NOTE: Being able to serve pre-compressed files will require configuring your web server to properly serve the UE4 compressed file extention type.
		+ These settings are placed in the .htaccess file
			> This file will work on most Apache web servers (that have the AllowOveride option set to (e.g.) All).
		+ AGAIN, if you are reading this section, we assume you have a deep understanding of web server technology.
			> This is not for beginners.

	Files Required for Compressed Files Deployment - in addition to the Non-Compressed Files (listed above)
	----------------------------------------------
	<project>.jsgz         - compressed main project Javascript code
	<project>.wasmgz       - compressed main game code
	<project>.datagz       - compressed game content
	<project>.data.jsgz    - compressed game content Javascript driver
	<project>.symbolsgz    - compressed symbols (optional: used with debugging)
	Utility.jsgz           - compressed utility Javascript code


Files not needed for Deployment - these are normally used during developement
===============================
	HTML5LaunchHelper.exe            - see Local Testing below
	RunMacHTML5LaunchHelper.command  - see Local Testing below
	Readme.txt                       - this Readme file




Local Testing
=============

	UE4's test web server
	---------------------
	Run HTML5Launcher.exe (on windows) or RunMacHTML5LaunchHelper.command (on Mac) to start a web server which is configured to serve compressed files on localhost.
	* This is NOT a production quality server.
	* Add -ServerPort=XXXX to the command line if necessary to change the serving port.
		> Default port is 8000.
	

	Python web server
	-----------------
	You may use python's built in web server, e.g.:
	* (make sure you run the following command from the same path this Readme and <project>.html files are located)
		python -m SimpleHTTPServer 8000


	Apache web server
	-----------------
	OSX and Linux normally have Apache installed.
	* You can use that for testing which has the closest behavior to most web host providers.


