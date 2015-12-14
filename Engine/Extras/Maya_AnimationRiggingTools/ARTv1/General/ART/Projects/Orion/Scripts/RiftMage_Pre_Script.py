import maya.cmds as cmds

#cmds.parent("portal", "ring_base", "root")
#cmds.delete("portal_ring_master")


if cmds.objExists("ik_foot_root"):
	cmds.delete("ik_foot_root")
	
if cmds.objExists("ik_hand_root"):
	cmds.delete("ik_hand_root")