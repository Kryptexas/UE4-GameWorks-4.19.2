This folder contains pre compressed files ready for deployment, please make sure to use the shipping build for final deployment. 

Files Required for final deployment. 
---------------------------------

*.js.gz -compressed javascript files.
*.data  -compressed game content
*.mem	-compressed memory iniitialization file
*.html 	-uncompressed landing page. 

*.symbols - uncompressed symbols, if necessary. 


Local Testing
------------- 

run HTML5Launcher.exe (via mono on Mac) to start a web sever which is conigured to serve compressed files on localhost. This is not a production quality server.
add -ServerPort=XXXX to the command line if necessary to change the serving port. Default port is 8000. 


How to deploy on Apache 
------------------------ 

Todo. 



