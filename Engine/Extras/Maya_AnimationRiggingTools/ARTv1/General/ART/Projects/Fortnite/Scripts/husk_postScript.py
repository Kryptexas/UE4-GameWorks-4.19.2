def addFortniteIKBones():

    try:
		#create the joints
		cmds.select(clear = True)
		ikFootRoot = cmds.joint(name = "ik_foot_root")
		cmds.select(clear = True)

		cmds.select(clear = True)
		ikFootLeft = cmds.joint(name = "ik_foot_l")
		cmds.select(clear = True)

		cmds.select(clear = True)
		ikFootRight = cmds.joint(name = "ik_foot_r")
		cmds.select(clear = True)

		cmds.select(clear = True)
		ikHandRoot = cmds.joint(name = "ik_hand_root")
		cmds.select(clear = True)

		cmds.select(clear = True)
		ikHandGun = cmds.joint(name = "ik_hand_gun")
		cmds.select(clear = True)

		cmds.select(clear = True)
		ikHandLeft = cmds.joint(name = "ik_hand_l")
		cmds.select(clear = True)

		cmds.select(clear = True)
		ikHandRight = cmds.joint(name = "ik_hand_r")
		cmds.select(clear = True)


		#create hierarhcy
		cmds.parent(ikFootRoot, "root")
		cmds.parent(ikHandRoot, "root")
		cmds.parent(ikFootLeft, ikFootRoot)
		cmds.parent(ikFootRight, ikFootRoot)
		cmds.parent(ikHandGun, ikHandRoot)
		cmds.parent(ikHandLeft, ikHandGun)
		cmds.parent(ikHandRight, ikHandGun)

		#constrain the joints
		leftHandConstraint = cmds.parentConstraint("hand_l", ikHandLeft)[0]
		rightHandGunConstraint = cmds.parentConstraint("hand_r", ikHandGun)[0]
		rightHandConstraint = cmds.parentConstraint("hand_r", ikHandRight)[0]
		leftFootConstraint = cmds.parentConstraint("foot_l", ikFootLeft)[0]
		rightFootConstraint = cmds.parentConstraint("foot_r", ikFootRight)[0]

    except:
		cmds.warning("Something went wrong")

#add morph control
ctrl = cmds.textCurves(ch = False, f = "Time New Roman|h-13|w400|c0", t = "M")[0]
cmds.setAttr(ctrl + ".rx", 90)
cmds.setAttr(ctrl + ".sx", 8.3)
cmds.setAttr(ctrl + ".sy", 8.3)
cmds.setAttr(ctrl + ".sz", 8.3)
cmds.setAttr(ctrl + ".tx", -13.58)
cmds.setAttr(ctrl + ".tz", 170)


for attr in [".tx", ".ty", ".tz", ".rx", ".ry", ".rz", ".sx", ".sy", ".sz", ".v"]:
	cmds.setAttr(ctrl + attr, lock = True, keyable = False)

children = cmds.listRelatives(ctrl, ad = True, type = "transform")
for child in children:
	if cmds.nodeType(child) !="nurbsCurve":
		for attr in [".tx", ".ty", ".tz", ".rx", ".ry", ".rz", ".sx", ".sy", ".sz", ".v"]:
			cmds.setAttr(child + attr, lock = False, keyable = False)

cmds.addAttr(children[0], ln = "BreathIn", dv = 0, min = 0, max = 1, keyable = True)
cmds.addAttr(children[0], ln = "L_Toe_Splay", dv = 0, min = 0, max = 1, keyable = True)
cmds.addAttr(children[0], ln = "R_Toe_Splay", dv = 0, min = 0, max = 1, keyable = True)
cmds.addAttr(children[0], ln = "L_Blink", dv = 0, min = 0, max = 1, keyable = True)
cmds.addAttr(children[0], ln = "R_Blink", dv = 0, min = 0, max = 1, keyable = True)

if cmds.objExists("Body_Shapes.BreathIn"):
    cmds.connectAttr(children[0] + ".BreathIn", "Body_Shapes.BreathIn")

if cmds.objExists("Body_Shapes.L_Toe_Splay"):
    cmds.connectAttr(children[0] + ".L_Toe_Splay", "Body_Shapes.L_Toe_Splay")

if cmds.objExists("Body_Shapes.R_Toe_Splay"):
    cmds.connectAttr(children[0] + ".R_Toe_Splay", "Body_Shapes.R_Toe_Splay")

if cmds.objExists("Head_Shapes.Left_Blink"):
    cmds.connectAttr(children[0] + ".L_Blink", "Head_Shapes.Left_Blink")

if cmds.objExists("Head_Shapes.Rigjt_Blink"):
    cmds.connectAttr(children[0] + ".R_Blink", "Head_Shapes.Rigjt_Blink")

if cmds.objExists("Head_Shapes.Right_Blink"):
    cmds.connectAttr(children[0] + ".R_Blink", "Head_Shapes.Right_Blink")


cmds.file(save = True, type = "mayaBinary", force = True)


#open export file
sceneName = cmds.file(q = True, sceneName = True)
exportFile = sceneName.partition("AnimRigs")[0] + "ExportFiles" +  sceneName.partition("AnimRigs")[2].partition(".mb")[0] + "_Export.mb"
cmds.file(exportFile, open = True, force = True)

#add ik bones
addFortniteIKBones()

#save
cmds.file(save = True, type = "mayaBinary", force = True)

