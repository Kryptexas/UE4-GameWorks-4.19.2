#setup constraint on constructor
try:
	cmds.select("Constructor:weapon_r_anim")
	cmds.cutKey()
	constraint = cmds.parentConstraint("blueprint:root", "Constructor:weapon_r_anim")[0]
except:
	print "failed to setup blueprint prop constraint"
	pass
	
for attr in [".tx", ".ty", ".tz", ".rx", ".ry", ".rz", ".sx", ".sy", ".sz"]:
	try:
		cmds.delete("Constructor:weapon_r_anim" + attr, icn = True)
	except:
		pass
	

#blueprint prop export
start = cmds.playbackOptions(q = True, min = True)
end = cmds.playbackOptions(q = True, max = True)
fileName = cmds.file(q = True, sceneName = True)

#BLUEPRINT
try:
	exportPath = fileName.rpartition(".")[0] + "_blueprint.fbx"
	
	cmds.select(["blueprint:shapes", "blueprint:root"], hi = True)
	cmds.bakeResults(simulation = True, time = (start, end))
	cmds.select("blueprint:root", hi = True)
	cmds.delete(constraints = True)
	
	
	cmds.select("blueprint:root")
	cmds.cutKey()
	for attr in ["tx", "ty", "tz", "rx", "ry", "rz"]:
		cmds.setAttr("blueprint:root." + attr, 0)
	
	#run an euler filter
	cmds.select("blueprint:root", hi = True)
	cmds.filterCurve()
				
				
	#first change some fbx properties
	string = "FBXExportConstraints -v 1;"
	string += "FBXExportCacheFile -v 0;"
	mel.eval(string)
				    
	cmds.select("blueprint:root", hi = True)
	cmds.select("blueprint:blueprint_geo", add = True)
	    
	cmds.file(exportPath, es = True, force = True, prompt = False, type = "FBX export")
except:
	pass


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# RAYGUN

if cmds.objExists("RayGunAuto_Rifle:b_AR_Root"):
	cmds.select("RayGunAuto_Rifle:b_AR_Root")
	
	if cmds.objExists("Handle"):
		cmds.rename("Handle", "Handle_Loc")
		
	#duoe
	newSkeleton = cmds.duplicate("RayGunAuto_Rifle:b_AR_Root")
	cmds.select(newSkeleton[0], hi = True)
	cmds.delete(constraints = True)
	
	newSkeletonJoints = cmds.ls(sl = True)
	
	#parent to world
	if cmds.listRelatives(newSkeletonJoints[0], parent = True) != None:
		cmds.parent(newSkeletonJoints[0], world = True)
		
	#constrain new skeleton to ref skeleton
	for j in newSkeletonJoints:
		cmds.parentConstraint("RayGunAuto_Rifle:" + j, j)
		
	#bake
	cmds.select(newSkeletonJoints[0], hi = True)
	cmds.bakeResults(simulation = True, time = (start, end))
	cmds.delete(constraints = True)
	
	cmds.select(newSkeletonJoints[0])
	cmds.cutKey()
	for attr in ["tx", "ty", "tz", "rx", "ry", "rz"]:
		cmds.setAttr(newSkeletonJoints[0] + "." + attr, 0)
	
	#export
	exportPath = fileName.rpartition(".")[0] + "_rayGunRifle.fbx"
	
	#first change some fbx properties
	string = "FBXExportConstraints -v 1;"
	string += "FBXExportCacheFile -v 0;"
	mel.eval(string)
				    
	cmds.select(newSkeletonJoints[0], hi = True)
	cmds.file(exportPath, es = True, force = True, prompt = False, type = "FBX export")
	cmds.delete(newSkeletonJoints[0])



