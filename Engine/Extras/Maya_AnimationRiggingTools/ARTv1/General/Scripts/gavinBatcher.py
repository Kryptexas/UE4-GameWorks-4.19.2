import maya.cmds as cmds
import maya.mel as mel
from functools import partial
import os, cPickle


class GavinExportBatcher():
    def __init__(self):
	    
	    #create class variables
	    self.widgets = {}
	    
	    #find out which project we are in
	    references = cmds.ls(type = "reference")
	    for ref in references:
		try:
		    self.project = cmds.referenceQuery(ref, filename = True).rpartition("Projects/")[2].partition("/")[0]
		    
		except:
		    pass
	    
	    
	    #get access to our maya tools
	    toolsPath = cmds.internalVar(usd = True) + "mayaTools.txt"
	    if os.path.exists(toolsPath):
		
		f = open(toolsPath, 'r')
		self.mayaToolsDir = f.readline()
		f.close()
		
		
	    #check to see if window exists, if so, delete
	    if cmds.window("gavinExportBatcherUI", exists = True):
		cmds.deleteUI("gavinExportBatcherUI")
		
	    #build window
	    self.widgets["window"] = cmds.window("gavinExportBatcherUI", w = 400, h = 200, title = "Gavin Exporter", sizeable = False)
	    
	    #create the main layout
	    self.widgets["topLevelLayout"] = cmds.rowColumnLayout(nc = 2, cw = [(1, 350),(2,50)], rs = [1,10])
	    
	    #input/output fields
	    cmds.text(label = "Input Path:", align = "left")
	    cmds.text(label = "")
	    
	    self.widgets["inputField"] = cmds.textField(w = 350, h = 30)
	    self.widgets["inputBrowse"] = cmds.symbolButton(w = 30, h = 30, image = self.mayaToolsDir + "/General/Icons/ART/browse.bmp", c = self.browse)
	    

	    #export button
	    self.widgets["exportBtn"] = cmds.button(w = 400, h = 50, label = "Begin Batch Export", c = self.batch)
	    
	    #show window
	    cmds.showWindow(self.widgets["window"])
	    
	    
    def browse(self, *args):
        path = cmds.fileDialog2(fm = 2,  okc = "OK")[0]
        if path == None:
            return
        
        else:
            #edit the text field with the above path passed in
            cmds.textField(self.widgets["inputField"], edit = True, text = path)
	    
	    
    def batch(self, *args):
	
	inputPath = cmds.textField(self.widgets["inputField"], q = True, text = True)
	if os.path.exists(inputPath):
	    files = os.listdir(inputPath)
	    for file in files:
		if file.rpartition(".")[2] == "mb":
		    
		    
		    #export
		    
		    #open file
		    cmds.file(os.path.join(inputPath, file), open = True, force = True)
		    
		    #delete the export skeleton if it exists:
		    if cmds.objExists("root"):
			cmds.delete("root")
			
		    #rename Husky_Female if exists
		    if cmds.objExists("Husky_Female"):
			cmds.rename("Husky_Female", "Husky_Woman")
			
			
		    #get the current time range values
		    startFrame = cmds.playbackOptions(q = True, min = True)
		    endFrame = cmds.playbackOptions(q = True, max = True)


		    #get file name
		    openedFile = cmds.file(q = True, sceneName = True)
		    if openedFile != "":
			fileName = openedFile.rpartition(".")[0]
			fileName += ".fbx"
			
		    #set interp value
		    cmds.optionVar(iv = ("rotationInterpolationDefault", 3))
		    
		    
		    #get characters
		    characters = []
		    references = cmds.ls(type = "reference")
		    
		    for reference in references:
			niceName = reference.rpartition("RN")[0]
			suffix = reference.rpartition("RN")[2]
			if suffix != "":
			    if cmds.objExists(niceName + suffix + ":" + "Skeleton_Settings"):
				characters.append(niceName + suffix)
				
			else:
			    if cmds.objExists(niceName + ":" + "Skeleton_Settings"):
				characters.append(niceName)	
				
		    #loop through characters
		    for character in characters:
			#get the body type
			bodyType = cmds.getAttr(character + ":body_anim.bodyType")
			
			#get blendshapes to export based on bodytype
			if bodyType == 0:
			    blendshapes = [character + ":blendShape1", character + ":blendShape2", character + ":body_blendShape"]
			    
			if bodyType == 1:
			    blendshapes = [character + ":blendShape2", character + ":blendShape4", character + ":blendShape5", character + ":body_blendShape"]
			    
			if bodyType == 2:
			    blendshapes = [character + ":blendShape2", character + ":blendShape4", character + ":blendShape5", character + ":body_blendShape"]
			    
			if bodyType == 3:
			    blendshapes = [character + ":blendShape1", character + ":blendShape2", character + ":body_blendShape"]
			
			if bodyType == 4:
			    blendshapes = [character + ":blendShape1", character + ":blendShape2", character + ":body_blendShape"]
			
			if bodyType == 5:
			    blendshapes = [character + ":blendShape1", character + ":blendShape2", character + ":body_blendShape"]
			    
			    
			#create custom export cube
			if cmds.objExists("custom_export"):
			    cmds.delete("custom_export")		    
			
			cube = cmds.polyCube(name = "custom_export")[0]
			i = 1
			for shape in blendshapes:
			    if cmds.objExists(shape):
				attrs = cmds.listAttr(shape, m = True, string = "weight")
				
				for attr in attrs:
				    keys = cmds.keyframe( shape, attribute=attrs, query=True, timeChange = True )
				    keyValues = cmds.keyframe( shape, attribute=attrs, query=True, valueChange = True )			
				    
				    morph = cmds.polyCube(name = attr)[0]
				    if cmds.objExists("custom_export_shapes"):
					cmds.blendShape("custom_export_shapes", edit = True, t = (cube, i, morph, 1.0))
					
				    else:
					cmds.select([morph, cube], r = True)
					cmds.blendShape(name = "custom_export_shapes")
					cmds.select(clear = True)
					
				    cmds.delete(morph)
				    
				    #transfer keys from original to new morph
				    if keys != None:
					for x in range(int(len(keys))):
					    cmds.setKeyframe("custom_export_shapes." + attr, t = (keys[x]), v = keyValues[x])
					
				    if keys == None:
					for x in range(startFrame, endFrame + 1):
					    value = cmds.getAttr(shape + "." + attr, time = x)
					    cmds.setKeyframe("custom_export_shapes." + attr, t = (x), v = value)
					    
				    i = i + 1
				

			#duplicate the skeleton
			newSkeleton = cmds.duplicate(character + ":" + "root", un = False, ic = False)
			
			joints = []
			for each in newSkeleton:
			    if cmds.nodeType(each) != "joint":
				cmds.delete(each)
				
			    else:
				joints.append(each)
				
				
			constraints = []
			for joint in joints:
			    #do some checks to make sure that this is valid
			    parent = cmds.listRelatives(joint, parent = True)
	
			    if parent != None:
				if parent[0] in joints:
				    constraint = cmds.parentConstraint(character + ":" + parent[0] + "|" + character + ":" + joint, joint)[0]
				    constraints.append(constraint)
				    constraint = cmds.scaleConstraint(character + ":" + parent[0] + "|" + character + ":" + joint, joint)[0]
				    constraints.append(constraint)			    
			    else:
				#root bone?
				if joint == "root":
				    constraint = cmds.parentConstraint(character + ":" + joint, joint)[0]
				    constraints.append(constraint)
				    
				    
			#bake
			cmds.select("root", hi = True)
			cmds.bakeResults(simulation = True, t = (startFrame, endFrame))
			cmds.delete(constraints)
			
			#run an euler filter
			cmds.select("root", hi = True)
			cmds.filterCurve()
			
			
			#cut keys on root
			cmds.cutKey("root")
			for attr in [".tx", ".ty", ".tz", ".rx", ".ry", ".rz"]:
			    cmds.setAttr("root" + attr, 0)
			    
			
			
			#export selected
			#first change some fbx properties
			string = "FBXExportConstraints -v 1;"
			string += "FBXExportCacheFile -v 0;"
			mel.eval(string)
			
			cmds.select("root", hi = True)
			if cmds.objExists("custom_export"):
			    cmds.select("custom_export", add = True)
			    
			#Add character Suffix to animation file name
			prefix = fileName.rpartition(".")[0]
			fileName = prefix + character + ".fbx"
			    
			cmds.file(fileName, es = True, force = True, prompt = False, type = "FBX export")
			
			#clean scene
			cmds.delete("root")
			if cmds.objExists("custom_export"):
			    cmds.delete("custom_export")
			    
	    #export complete
	    cmds.confirmDialog(title = "Export", message = "Export Complete!", button = "OK")	    
			    
			
	else:
	    return
	
	




			

		    


		    
		    



			
			
			
			
			




