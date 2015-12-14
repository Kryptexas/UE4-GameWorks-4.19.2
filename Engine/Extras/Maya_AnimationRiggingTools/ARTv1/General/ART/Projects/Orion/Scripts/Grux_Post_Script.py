import maya.cmds as cmds

# ------------------------------------------------------------------
# scale all of the controls to be more visible with the mesh.
def scaleControls():
    for side in ["_l", "_r"]:
        # feet
        cmds.select(["ik_foot_anim"+side+".cv[0:17]"])
        cmds.scale(1.5, 0.8, 1.0, r = True, ocp = False)
        cmds.move(0, 4.678845, -11.947602, r = True)

        cmds.select(["ik_wrist"+side+"_anim.cv[0:7]"])
        cmds.scale(1.9, 1.9, 1.9, r = True, ocp = False)

        cmds.select(["weapon"+side+"_anim.cv[0:32]"])
        cmds.scale(0.330274, 0.330274, 0.330274, r = True, ocp = False)
        if side == "_l":
            cmds.move(25, 0, 0, r = True)
        else:
            cmds.move(-25, 0, 0, r = True)            

        cmds.select(["eyelid_bott"+side+"_anim.cv[0:32]"])
        cmds.scale(0.1, 0.1, 0.01, r = True, ocp = False)
        if side is "_l":
            cmds.move(5, 0, 0, r = True, os=True)
        else:
            cmds.move(-5, 0, 0, r = True, os=True)

        cmds.select(["eyelid_top"+side+"_anim.cv[0:32]"])
        cmds.scale(0.1, 0.1, 0.01, r = True, ocp = False)
        if side is "_l":
            cmds.move(5, 0, 0, r = True, os=True)
        else:
            cmds.move(-5, 0, 0, r = True, os=True)

        cmds.select(["eyeball"+side+"_anim.cv[0:32]"])
        cmds.scale(0.24, 0.24, 0.24, r = True, ocp = False)
        cmds.move(0, 0, 0, r = True, os=True)

        cmds.select(["ear"+side+"_anim.cv[0:32]"])
        cmds.scale(1.49, 0.67, 0.43, r = True, ocp = False)

    # Other
    cmds.select(["jaw_anim.cv[0:32]"])
    cmds.scale(0.2, 0.2, 0.2, r = True, ocp = True)
    cmds.move(0, -13, -19, r = True)

    cmds.select(["fk_tongue_01_anim.cv[0:32]"])
    cmds.scale(0.246972, 0.246972, 0.246972, r = True, ocp = False)
    cmds.move(0, 0, 0, r = True)
    cmds.select(["fk_tongue_02_anim.cv[0:32]"])
    cmds.scale(0.246972, 0.246972, 0.246972, r = True, ocp = False)
    cmds.move(0, 0, 0, r = True)
    cmds.select(["fk_tongue_03_anim.cv[0:32]"])
    cmds.scale(0.246972, 0.246972, 0.246972, r = True, ocp = False)
    cmds.move(0, 0, 0, r = True)

    cmds.select(["small_pack_l_anim.cv[0:4]"])
    cmds.scale(1, 1, 1, r = True, ocp = True)
    cmds.move(10, 0, 0, r = True, os=True, wd=True)

    cmds.select(["small_pack_r_anim.cv[0:4]"])
    cmds.scale(1, 1, 1, r = True, ocp = True)
    cmds.move(-10, 0, 0, r = True, os=True, wd=True)

    cmds.select(["big_pack_l_anim.cv[0:4]"])
    cmds.scale(1, 1, 1, r = True, ocp = True)
    cmds.move(12, 0, 0, r = True, os=True, wd=True)

    cmds.select(["big_pack_r_anim.cv[0:4]"])
    cmds.scale(1, 1, 1, r = True, ocp = True)
    cmds.move(-12, 0, 0, r = True, os=True, wd=True)

    cmds.select(None)

scaleControls()

# ------------------------------------------------------------------
def shoulderRig():
    
    cmds.setAttr("Rig_Settings.lArmMode", 0)
    cmds.setAttr("Rig_Settings.lClavMode", 0)
    cmds.setAttr("Rig_Settings.rArmMode", 0)
    cmds.setAttr("Rig_Settings.rClavMode", 0)

    # Left Side
    cmds.setDrivenKeyframe(["scapula_l_anim_grp.translate", "scapula_l_anim_grp.rotate"], cd = "driver_clavicle_l.rotateZ", itt = "spline", ott = "spline")

    # Clav back and forward
    cmds.setAttr("fk_clavicle_l_anim.rotateZ", 30)
    cmds.setAttr("scapula_l_anim_grp.translate", 8.81813, 17.39541, 0, type="double3")
    cmds.setAttr("scapula_l_anim_grp.rotate", 0, 0, 10.86792, type="double3")
    cmds.setDrivenKeyframe(["scapula_l_anim_grp.translate", "scapula_l_anim_grp.rotate"], cd = "driver_clavicle_l.rotateZ", itt = "spline", ott = "spline")

    cmds.setAttr("fk_clavicle_l_anim.rotateZ", -30)
    cmds.setAttr("scapula_l_anim_grp.translate", -14.71763, -1.43961, 1.38045, type="double3")
    cmds.setAttr("scapula_l_anim_grp.rotate", 0, 0, 36.12318, type="double3")
    cmds.setDrivenKeyframe(["scapula_l_anim_grp.translate", "scapula_l_anim_grp.rotate"], cd = "driver_clavicle_l.rotateZ", itt = "spline", ott = "spline")

    cmds.setAttr("fk_clavicle_l_anim.rotateZ", 0)

    # Clav up and down
    cmds.setDrivenKeyframe(["scapula_l_anim_grp.translate", "scapula_l_anim_grp.rotate"], cd = "driver_clavicle_l.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("fk_clavicle_l_anim.rotateY", -35)
    cmds.setAttr("scapula_l_anim_grp.translate", 1.02309, 4.7537, 5.18273, type="double3")
    cmds.setAttr("scapula_l_anim_grp.rotate", 5.18273, -9.00623, 26.123, type="double3")
    cmds.setDrivenKeyframe(["scapula_l_anim_grp.translate", "scapula_l_anim_grp.rotate"], cd = "driver_clavicle_l.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("fk_clavicle_l_anim.rotateY", 20)
    cmds.setAttr("scapula_l_anim_grp.translate", 0, 0, 4.3, type="double3")
    cmds.setAttr("scapula_l_anim_grp.rotate", 0, -4.7, 0, type="double3")
    cmds.setDrivenKeyframe(["scapula_l_anim_grp.translate", "scapula_l_anim_grp.rotate"], cd = "driver_clavicle_l.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("fk_clavicle_l_anim.rotateY", 0)

    # Right Side
    cmds.setDrivenKeyframe(["scapula_r_anim_grp.translate", "scapula_r_anim_grp.rotate"], cd = "driver_clavicle_r.rotateZ", itt = "spline", ott = "spline")

    # Clav back and forward
    cmds.setAttr("fk_clavicle_r_anim.rotateZ", 30)
    cmds.setAttr("scapula_r_anim_grp.translate", -8.81813, -17.39541, 0, type="double3")
    cmds.setAttr("scapula_r_anim_grp.rotate", 0, 0, -10.86792, type="double3")
    cmds.setDrivenKeyframe(["scapula_r_anim_grp.translate", "scapula_r_anim_grp.rotate"], cd = "driver_clavicle_r.rotateZ", itt = "spline", ott = "spline")

    cmds.setAttr("fk_clavicle_r_anim.rotateZ", -30)
    cmds.setAttr("scapula_r_anim_grp.translate", 14.71763, 1.43961, -1.38045, type="double3")
    cmds.setAttr("scapula_r_anim_grp.rotate", 0, 0, -36.12318, type="double3")
    cmds.setDrivenKeyframe(["scapula_r_anim_grp.translate", "scapula_r_anim_grp.rotate"], cd = "driver_clavicle_r.rotateZ", itt = "spline", ott = "spline")

    cmds.setAttr("fk_clavicle_r_anim.rotateZ", 0)

    # Clav up and down
    cmds.setDrivenKeyframe(["scapula_r_anim_grp.translate", "scapula_r_anim_grp.rotate"], cd = "driver_clavicle_r.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("fk_clavicle_r_anim.rotateY", -35)
    cmds.setAttr("scapula_r_anim_grp.translate", -1.02309, -4.7537, -5.18273, type="double3")
    cmds.setAttr("scapula_r_anim_grp.rotate", -5.18273, 9.00623, -26.123, type="double3")
    cmds.setDrivenKeyframe(["scapula_r_anim_grp.translate", "scapula_r_anim_grp.rotate"], cd = "driver_clavicle_r.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("fk_clavicle_r_anim.rotateY", 20)
    cmds.setAttr("scapula_r_anim_grp.translate", 0, 0, -4.3, type="double3")
    cmds.setAttr("scapula_r_anim_grp.rotate", 0, 4.7, 0, type="double3")
    cmds.setDrivenKeyframe(["scapula_r_anim_grp.translate", "scapula_r_anim_grp.rotate"], cd = "driver_clavicle_r.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("fk_clavicle_r_anim.rotateY", 0)

    cmds.setAttr("Rig_Settings.lArmMode", 1)
    cmds.setAttr("Rig_Settings.lClavMode", 1)
    cmds.setAttr("Rig_Settings.rArmMode", 1)
    cmds.setAttr("Rig_Settings.rClavMode", 1)

shoulderRig()

# ------------------------------------------------------------------
# Create all of the blendshapes that are needed an hook them up.
def createBlendshapes():
    cmds.setAttr("Rig_Settings.lArmMode", 0)
    cmds.setAttr("Rig_Settings.lClavMode", 0)
    cmds.setAttr("Rig_Settings.rArmMode", 0)
    cmds.setAttr("Rig_Settings.rClavMode", 0)

    for side in ["_l", "_r"]:
        # Create SDK's to drive the shoulder blendshape by the clavicle joint
        cmds.setAttr("fk_arm"+side+"_anim.ry", 0)
        cmds.setAttr("body_geo_blendShape.body_shoulders"+side+"_up_morph", 1)
        cmds.setDrivenKeyframe("body_geo_blendShape.body_shoulders"+side+"_up_morph", cd="driver_upperarm"+side+".rotateY", itt="flat", ott="flat")
        cmds.setAttr("fk_arm"+side+"_anim.ry", 67)
        cmds.setAttr("body_geo_blendShape.body_shoulders"+side+"_up_morph", 0)
        cmds.setDrivenKeyframe("body_geo_blendShape.body_shoulders"+side+"_up_morph", cd="driver_upperarm"+side+".rotateY", itt="flat", ott="flat")
        cmds.setAttr("fk_arm"+side+"_anim.ry", 0)

    cmds.setAttr("Rig_Settings.lArmMode", 1)
    cmds.setAttr("Rig_Settings.lClavMode", 1)
    cmds.setAttr("Rig_Settings.rArmMode", 1)
    cmds.setAttr("Rig_Settings.rClavMode", 1)
createBlendshapes()

# ------------------------------------------------------------------
def dynamicsOff():
    for one in ["big_pack_l_anim", "small_pack_l_anim", "big_pack_r_anim", "small_pack_r_anim", "horn_ring_anim", "tongue_dyn_anim", "loin_cloth_fr_l_dyn_anim", "loin_cloth_fr_r_dyn_anim", "loin_cloth_bk_l_dyn_anim", "loin_cloth_bk_r_dyn_anim"]:
        cmds.setAttr(one+".chainStartEnvelope", 0)
        cmds.setAttr(one+".chainStartEnvelope", l=True)
dynamicsOff()

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
addCapsule(60, 90)

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