import sys
import inspect

import maya
import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx
import maya.cmds as cmds

#Populate subjects list from c++ (via command LiveLinkSubjects)
def PopulateSubjects():
	Subjects = cmds.LiveLinkSubjects()
	if Subjects is not None:
		for Subject in cmds.LiveLinkSubjects():
			cmds.textScrollList("ActiveSubjects", edit = True, append = Subject)

#Refresh subjects list
def RefreshSubjects():
	if (cmds.window(MayaLiveLinkUI.WindowName , exists=True)):
		cmds.textScrollList("ActiveSubjects", edit=True, removeAll=True)
		PopulateSubjects()

#Connection UI Colours
ConnectionActiveColour = [0.71, 0.9, 0.1]
ConnectionInactiveColour = [1.0, 0.4, 0.4]
ConnectionColourMap = {
	True : ConnectionActiveColour,
	False: ConnectionInactiveColour
}

#Base class for command (common creator method + allows for automatic register/unregister)
class LiveLinkCommand(OpenMayaMPx.MPxCommand):
	def __init__(self):
		OpenMayaMPx.MPxCommand.__init__(self)

	@classmethod
	def Creator(Cls):
		return OpenMayaMPx.asMPxPtr( Cls() )

# Is supplied object a live link command
def IsLiveLinkCommand(InCls):
	return inspect.isclass(InCls) and issubclass(InCls, LiveLinkCommand) and InCls != LiveLinkCommand

# Given a list of strings of names return all the live link commands listed
def GetLiveLinkCommandsFromModule(ModuleItems):
	EvalItems = (eval(Item) for Item in ModuleItems)
	return [Command for Command in EvalItems if IsLiveLinkCommand(Command) ]

# Command to create the Live Link UI
class MayaLiveLinkUI(LiveLinkCommand):
	WindowName = "MayaLiveLinkUI"
	Title = "Maya Live Link UI"
	WindowSize = (500, 350)

	def __init__(self):
		LiveLinkCommand.__init__(self)
        
	# Invoked when the command is run.
	def doIt(self,argList):
		if (cmds.window(self.WindowName , exists=True)):
			cmds.deleteUI(self.WindowName)
		window = cmds.window( self.WindowName, title=self.Title, widthHeight=(self.WindowSize[0], self.WindowSize[1]) )
		
		#Get current connection status
		ConnectionText, ConnectedState = cmds.LiveLinkConnectionStatus()
		
		#Type your UI code here
		cmds.columnLayout( "mainColumn", adjustableColumn=True )
		cmds.rowLayout("HeaderRow", numberOfColumns=3, adjustableColumn=1, parent = "mainColumn")
		cmds.text(label="Unreal Engine Live Link", align="left")
		cmds.text(label="Connection:")
		cmds.text("ConnectionStatusUI", label=ConnectionText, align="center", backgroundColor=ConnectionColourMap[ConnectedState], width=150)
		cmds.textScrollList("ActiveSubjects", allowMultiSelection = True, parent = "mainColumn")

		PopulateSubjects()

		cmds.rowLayout("AddSelectedAsSubject", numberOfColumns=3, parent = "mainColumn")
		cmds.textField( "NewSubjectName", text = "Maya", parent = "AddSelectedAsSubject", width=300)
		cmds.button( label='Add Subject', parent = "AddSelectedAsSubject", command=self.AddSubject)
		cmds.button( label='Remove Subject', parent = "AddSelectedAsSubject", command=self.RemoveSubject)

		cmds.showWindow( self.WindowName )

	def AddSubject(self, *args):
		Name = cmds.textField("NewSubjectName", query = True, text = True)
		cmds.LiveLinkAddSubject(Name)
		RefreshSubjects()

	def RemoveSubject(self, *args):
		for ItemToRemove in cmds.textScrollList("ActiveSubjects", q=1, si=1):
			cmds.LiveLinkRemoveSubject(ItemToRemove)
		RefreshSubjects()

# Command to Refresh the subject UI
class MayaLiveLinkRefreshUI(LiveLinkCommand):
	def __init__(self):
		LiveLinkCommand.__init__(self)
        
	# Invoked when the command is run.
	def doIt(self,argList):
		RefreshSubjects()

# Command to Refresh the connection UI
class MayaLiveLinkRefreshConnectionUI(LiveLinkCommand):
	def __init__(self):
		LiveLinkCommand.__init__(self)
        
	# Invoked when the command is run.
	def doIt(self,argList):
		if (cmds.window(MayaLiveLinkUI.WindowName , exists=True)):
			#Get current connection status
			ConnectionText, ConnectedState = cmds.LiveLinkConnectionStatus()
			cmds.text("ConnectionStatusUI", edit=True, label=ConnectionText, backgroundColor=ConnectionColourMap[ConnectedState])

class MayaLiveLinkGetActiveCamera(LiveLinkCommand):
	def __init__(self):
		LiveLinkCommand.__init__(self)
        
	# Invoked when the command is run.
	def doIt(self,argList):
		self.clearResult()
		try:
			c = cmds.getPanel(wf=1)
			cam = cmds.modelPanel(c, q=True, camera=True)
		except:
			pass
		else:
			self.setResult(cam)

#Grab commands declared
Commands = GetLiveLinkCommandsFromModule(dir())

#Initialize the script plug-in
def initializePlugin(mobject):
	mplugin = OpenMayaMPx.MFnPlugin(mobject)

	print "LiveLink:"
	for Command in Commands:
		try:
			print "\tRegistering Command '%s'"%Command.__name__
			mplugin.registerCommand( Command.__name__, Command.Creator )
		except:
			sys.stderr.write( "Failed to register command: %s\n" % Command.__name__ )
			raise

# Uninitialize the script plug-in
def uninitializePlugin(mobject):
	mplugin = OpenMayaMPx.MFnPlugin(mobject)

	for Command in Commands:
		try:
			mplugin.deregisterCommand( Command.__name__ )
		except:
			sys.stderr.write( "Failed to unregister command: %s\n" % Command.__name__ )