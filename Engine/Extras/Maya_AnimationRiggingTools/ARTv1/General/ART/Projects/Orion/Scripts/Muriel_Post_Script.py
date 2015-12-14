import maya.cmds as cmds

# ------------------------------------------------------------------
# New wing control shapes
lWingControls = ["wing_blade_A_l_anim", "wing_blade_B_l_anim", "wing_blade_C_l_anim"]
rWingControls = ["wing_blade_A_r_anim", "wing_blade_B_r_anim", "wing_blade_C_r_anim"]
for list in [lWingControls, rWingControls]:
	val = 0
	for each in list:
		#create new curve shape
		control = cmds.curve(d = 1, p = [(-0.124602, 0, 1.096506), (-0.975917, 0, 1.036319), (-0.560906, 0, 0.946236), (-0.798049, 0, 0.798033), (-1.042985, 0, 0.430511), (-1.128122, 0, -0.00276108), (-1.042702, 0, -0.431934), (-0.798049, 0, -0.798033), (-0.559059, 0, -0.944259), (-0.975917, 0, -1.036319), (-0.124602, 0, -1.096506), (-0.537718, 0, -0.349716), (-0.439199, 0, -0.785581), (-0.652776, 0, -0.652998), (-0.853221, 0, -0.353358), (-0.923366, 0, 0), (-0.856561, 0, 0.336531), (-0.658494, 0, 0.644451), (-0.452392, 0, 0.781229), (-0.537718, 0, 0.349716), (-0.124602, 0, 1.096506)], k = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20])
		cmds.setAttr(control + ".scaleX", 18)
		cmds.setAttr(control + ".scaleY", 18)
		cmds.setAttr(control + ".scaleZ", 18)
		
		if each in lWingControls:
			cmds.setAttr(control + ".rotateZ", 90)
			cmds.setAttr(control + ".translateX", val)
		if each in rWingControls:
			cmds.setAttr(control + ".rotateZ", -90)
			cmds.setAttr(control + ".translateX", val * -1)
			
		val = val + 4
			
		cmds.makeIdentity(control, apply = True, t =1, r =1, s =1)
		parentShape = cmds.listRelatives(each, f = True, s = True)[0]
		cmds.delete(parentShape)
		shape = cmds.listRelatives(control, f = True, s = True)[0]
		cmds.parent(shape, each, add = True, shape = True)
		
		cmds.delete(control)
		
		
		
# ------------------------------------------------------------------
# D Wing
lWingControls = ["wing_blade_D_l_anim"]
rWingControls = ["wing_blade_D_r_anim"]
for list in [lWingControls, rWingControls]:
	for each in list:
		#create new curve shape
		control = cmds.curve(d = 1, p = [(-0.124602, 0, 1.096506), (-0.975917, 0, 1.036319), (-0.560906, 0, 0.946236), (-0.798049, 0, 0.798033), (-1.042985, 0, 0.430511), (-1.128122, 0, -0.00276108), (-1.042702, 0, -0.431934), (-0.798049, 0, -0.798033), (-0.559059, 0, -0.944259), (-0.975917, 0, -1.036319), (-0.124602, 0, -1.096506), (-0.537718, 0, -0.349716), (-0.439199, 0, -0.785581), (-0.652776, 0, -0.652998), (-0.853221, 0, -0.353358), (-0.923366, 0, 0), (-0.856561, 0, 0.336531), (-0.658494, 0, 0.644451), (-0.452392, 0, 0.781229), (-0.537718, 0, 0.349716), (-0.124602, 0, 1.096506)], k = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20])
		cmds.setAttr(control + ".scaleX", 18)
		cmds.setAttr(control + ".scaleY", 18)
		cmds.setAttr(control + ".scaleZ", 18)
		
		if each in lWingControls:
			cmds.setAttr(control + ".rotateZ", -90)
		if each in rWingControls:
			cmds.setAttr(control + ".rotateZ", 90)
			
		cmds.makeIdentity(control, apply = True, t =1, r =1, s =1)
		parentShape = cmds.listRelatives(each, f = True, s = True)[0]
		cmds.delete(parentShape)
		shape = cmds.listRelatives(control, f = True, s = True)[0]
		cmds.parent(shape, each, add = True, shape = True)
		
		cmds.delete(control)
		
		
# ------------------------------------------------------------------
# Hand Sizes

cmds.select(["hand_flap_l_anim.cv[0:32]", "hand_flap_r_anim.cv[0:32]"])
cmds.scale(0.294922, 0.294922, 0.294922, r = True, ocp = True)

    
cmds.select(["wing_blade_B_l_anim.cv[0:32]", "wing_blade_A_r_anim.cv[0:32]", "wing_ext_l_anim.cv[0:32]", "wing_ext_r_anim.cv[0:32]", "wing_blade_main_l_anim.cv[0:32]", "chest_ring_anim.cv[0:32]", "wing_blade_A_l_anim.cv[0:32]", "wing_blade_B_r_anim.cv[0:32]", "wing_main_r_anim.cv[0:32]", "wing_blade_C_l_anim.cv[0:32]", "wing_blade_C_r_anim.cv[0:32]", "wing_blade_D_l_anim.cv[0:32]", "wing_blade_main_r_anim.cv[0:32]", "wing_blade_D_r_anim.cv[0:32]", "wing_main_l_anim.cv[0:32]"])
cmds.scale(0.55, 0.55, 0.55, r = True, ocp = True)

cmds.select(["foot_flap_l_anim.cv[0:32]", "foot_flap_r_anim.cv[0:32]"])
cmds.scale(0.248359, 0.248359, 0.248359, r = True, ocp = True)

# ------------------------------------------------------------------
def scaleControls():
    sides = ["_l", "_r"]
    for side in sides:
        # feet
        cmds.select(["ik_foot_anim"+side+".cv[0:17]"])
        cmds.scale(1.5, 0.8, 1.0, r = True, ocp = False)
        cmds.move(0, 4.678845, -1.187034, r = True)

        cmds.select(["ik_wrist"+side+"_anim.cv[0:7]"])
        cmds.scale(1.9, 1.9, 1.9, r = True, ocp = False)

        cmds.select(["eyelid_bott"+side+"_anim.cv[0:32]"])
        cmds.scale(0.1, 0.1, 0.01, r = True, ocp = False)
        cmds.move(0, -3.757386, 0.654023, r = True)

        cmds.select(["eyelid_top"+side+"_anim.cv[0:32]"])
        cmds.scale(0.1, 0.1, 0.01, r = True, ocp = False)
        cmds.move(0, -3.757386, 0.654023, r = True)

    # Other
    cmds.select(["eyeball_l_anim.cv[0:32]"])
    cmds.scale(0.1, 0.1, 0.05, r = True, ocp = False)
    cmds.move(0, -3.757386, 0, r = True)

    cmds.select(["eyeball_r_anim.cv[0:32]"])
    cmds.scale(0.1, 0.1, 0.05, r = True, ocp = False)
    cmds.move(0, 3.757386, 0, r = True)

    cmds.select(["jaw_anim.cv[0:32]"])
    cmds.scale(0.2, 0.2, 0.2, r = True, ocp = True)
    cmds.move(0, -6, -8, r = True)

    cmds.select(None)

scaleControls()

# ------------------------------------------------------------------
def invertEyelids():
    sides = ["_l", "_r"]
    for side in sides:
        cmds.delete("driver_eyelid_bott"+side+"_parentConstraint1")
        cmds.rotate(0, -180, 0, "eyelid_bott"+side+"_anim_grp", r=True, os=True)
        cmds.parentConstraint("eyelid_bott"+side+"_anim", "driver_eyelid_bott"+side, mo=True)
invertEyelids()

def invertRightEyeball():
    cmds.delete("driver_eyeball_r_parentConstraint1")
    cmds.rotate(0, -180, -180, "eyeball_r_anim_grp", r=True, os=True)
    cmds.parentConstraint("eyeball_r_anim", "driver_eyeball_r", mo=True)
invertRightEyeball()

# ------------------------------------------------------------------
def createGameCam():
    gameCam = cmds.camera(centerOfInterest=5, focalLength=18, lensSqueezeRatio=1, cameraScale=1, horizontalFilmAperture=1.41732, horizontalFilmOffset=0, verticalFilmAperture=0.94488, verticalFilmOffset=0, filmFit="Overscan", overscan=1.05, motionBlur=0, shutterAngle=144, nearClipPlane=1.0, farClipPlane=10000, orthographic=0, orthographicWidth=30, panZoomEnabled=0, horizontalPan=0, verticalPan=0, zoom=1, displayFilmGate=True)[0]
    print gameCam
    gameCam = cmds.rename(gameCam, "GameCam")
    cmds.camera(gameCam, e=True, horizontalFilmAperture=1.6724376, focalLength=21.2399857011)
    cmds.setAttr(gameCam+".displayGateMaskOpacity", 1)
    cmds.setAttr(gameCam+".displayGateMaskColor", 0, 0, 0, type="double3")
    cmds.setAttr(gameCam+".r", 90, 0, 180, type="double3")
    
    # Create the control
    control = cmds.circle(name=gameCam+"_Ctrl", ch=True, o=True, r=15)[0]
    cmds.makeIdentity(control, a=True, t=True, r=True, s=True)
    # Create groups for the sdk and the constraints
    const_grp = cmds.group(control, name=control+"_grp")
    if cmds.objExists("rig_grp"):
        cmds.parent(const_grp, "rig_grp")
    # Parent Constrain the cam to the control
    cmds.parentConstraint(control, gameCam, mo=True)
    
    cmds.connectAttr(control+".visibility", gameCam+".visibility", f=True)
    cmds.setAttr(const_grp+".v", 0)
    
    cmds.setAttr(const_grp+".t", 0, 500, 200, type="double3")
createGameCam()
# ------------------------------------------------------------------

##################################################################################
##################################################################################

def addCapsule(radius, halfHeight):
    trueHalfHeight = (halfHeight*2)-(radius*2)
    capsule = cmds.polyCylinder(name="collision_capsule", r=radius, h=trueHalfHeight, sx=18, sy=1, sz=5, ax=[0,0,1], rcp=True, cuv=3, ch=True)
    cmds.setAttr(capsule[0]+".tz", halfHeight+2.2)
    cmds.setAttr(capsule[0]+".rz", 90)

    cmds.addAttr(capsule[0], ln="Radius", dv=radius, keyable=True)
    cmds.addAttr(capsule[0], ln="HalfHeight", dv=halfHeight, keyable=True)

    expr = capsule[1]+".radius = "+capsule[0]+".Radius;\n"+capsule[1]+".height = ("+capsule[0]+".HalfHeight*2) - ("+capsule[0]+".Radius*2);"
    cmds.expression(s=expr, o=capsule[1], ae=1, uc="all")

    cmds.setAttr(capsule[0]+".overrideEnabled", 1)
    cmds.setAttr(capsule[0]+".overrideColor", 18)
    cmds.setAttr(capsule[0]+".overrideShading", 0)

    cmds.addAttr(capsule[0], ln="Shaded", dv=0, min=0, max=1, keyable=True)
    cmds.connectAttr(capsule[0]+".Shaded", capsule[0]+".overrideShading")

    cmds.parentConstraint("root", capsule[0], mo=True)
    cmds.select(capsule[0], r=True)
    capsuleDisplayLayer = cmds.createDisplayLayer(nr=True, name="Collision Capsule")
    cmds.setAttr(capsuleDisplayLayer+".displayType", 2)
    cmds.setAttr(capsuleDisplayLayer+".v", 0)
    
    for attr in ["tx", "ty", "tz", "rx", "ry", "rz", "sx", "sy", "sz"]:
        cmds.setAttr(capsule[0]+"."+attr, lock=True, keyable=False, channelBox=False)

    cmds.select(clear=True)
addCapsule(42, 106)

# ------------------------------------------------------------------
# add ik bones Function that is called later after we open the export file.
def addIKBones():
    try:
		# create the joints
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
		
		# create hierarhcy
		cmds.parent(ikFootRoot, "root")
		cmds.parent(ikHandRoot, "root")
		cmds.parent(ikFootLeft, ikFootRoot)
		cmds.parent(ikFootRight, ikFootRoot)
		cmds.parent(ikHandGun, ikHandRoot)
		cmds.parent(ikHandLeft, ikHandGun)
		cmds.parent(ikHandRight, ikHandGun)
		
		# constrain the joints
		leftHandConstraint = cmds.parentConstraint("hand_l", ikHandLeft)[0]
		rightHandGunConstraint = cmds.parentConstraint("hand_r", ikHandGun)[0]
		rightHandConstraint = cmds.parentConstraint("hand_r", ikHandRight)[0]
		leftFootConstraint = cmds.parentConstraint("foot_l", ikFootLeft)[0]
		rightFootConstraint = cmds.parentConstraint("foot_r", ikFootRight)[0]
	
    except:
		cmds.warning("Something went wrong")

# ------------------------------------------------------------------
# ADD the IK bones to the Export file.
#save the AnimRig file before opening the export file.
cmds.file(save = True, type = "mayaBinary", force = True)

#ADD IK BONES
#open export file
sceneName = cmds.file(q = True, sceneName = True)
exportFile = sceneName.partition("AnimRigs")[0] + "ExportFiles" +  sceneName.partition("AnimRigs")[2].partition(".mb")[0] + "_Export.mb"
cmds.file(exportFile, open = True, force = True)

#add ik bones
addIKBones()

# ------------------------------------------------------------------
def addCustomAttrs():
    attrs = cmds.listAttr("root")
    exists = False
    for one in attrs:
        if one.find("DistanceCurve") != -1:
            exists = True
    if exists == False:
        cmds.addAttr("root", ln="DistanceCurve", keyable=True)
addCustomAttrs()

#save the export file.
cmds.file(save = True, type = "mayaBinary", force = True)