import maya.cmds as cmds

if cmds.objExists("ik_foot_root"):
	cmds.delete("ik_foot_root")
	
if cmds.objExists("ik_hand_root"):
	cmds.delete("ik_hand_root")