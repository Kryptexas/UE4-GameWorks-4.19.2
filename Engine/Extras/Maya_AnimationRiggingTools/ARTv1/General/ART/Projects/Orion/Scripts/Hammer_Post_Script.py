import maya.cmds as cmds

# ------------------------------------------------------------------
# scale all of the controls to be more visible with the mesh.
def scaleControls():
    cmds.setAttr("leg_sys_grp.v", 0)

    '''for side in ["_l", "_r"]:
        cmds.select(["ik_wrist"+side+"_anim.cv[*]"])
        cmds.scale(1, 1, 1, r = True, ocp = False)'''

    objects = ["shoulderpad_l_anim", "shoulderpad_r_anim"]
    for one in objects:
        cmds.select([one+".cv[*]"])
        cmds.scale(0.5, 0.5, 0.5, r=True, ocp=False)
        cmds.move(0, 0, 0, r = True, os=True, wd=True)

    cmds.select(["shoulderA_01_l_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(16, -31, 10, r = True)            

    cmds.select(["shoulderA_01_r_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(-16, -31, 10, r = True)            

    cmds.select(["shoulderA_02_l_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(13, 27, 23, r = True)            

    cmds.select(["shoulderA_02_r_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(-13, 27, 23, r = True)            

    cmds.select(["shoulderB_01_l_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(16, -27, 4, r = True)            

    cmds.select(["shoulderB_01_r_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(-16, -27, 4, r = True)            

    cmds.select(["shoulderC_01_l_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(7, 0, 15, r = True)            

    cmds.select(["shoulderC_01_r_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(-7, 0, 15, r = True)            

    cmds.select(["spintal_cord_01_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(0, 35, 0, r = True)            

    cmds.select(["forearm_armor_01_l_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(0, 0, 14, r = True)            

    cmds.select(["forearm_armor_01_r_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(0, 0, 14, r = True)            

    cmds.select(None)
scaleControls()

# ------------------------------------------------------------------
def shoulderRig():
    for side in ["_l", "_r"]:
        cmds.select("shoulderpad"+side+"_anim.cv[*]")
        cmds.scale(0.75, 0.75, 0.75, r = True)
        if side == "_l":
            cmds.move(0, 0, 35, r = True, os=True, wd=True)
        else:
            cmds.move(0, 0, -35, r = True, os=True, wd=True)
            
        cmds.addAttr("shoulderpad"+side+"_anim", ln = "FollowClav", dv = 0, min = 0, max = 1, keyable = True)
        
        cmds.select("clavicle"+side+"_anim.cv[*]")
        if side == "_l":
            cmds.move(22, 0, 56, r = True, os=True, wd=True)
        else:
            cmds.move(-22, 0, 56, r = True, os=True, wd=True)

        
        shoulder_loc = cmds.duplicate("shoulderpad"+side+"_anim_space_switcher", n="shoulder_loc"+side+"", po=True)[0]
        cmds.parent(shoulder_loc, "ctrl_rig")
        cmds.makeIdentity(shoulder_loc, t=True, r=True, a=True)
        cmds.parentConstraint("driver_upperarm"+side+"", shoulder_loc, mo=True)
        
        const = cmds.pointConstraint(shoulder_loc, "shoulderpad"+side+"_anim_space_switcher", "shoulderpad"+side+"_anim_grp", mo=True, sk=["x"])[0]
        
        reverseNode = cmds.shadingNode("reverse", asUtility=True, name="shoulderpad"+side+"_reverse")
        cmds.connectAttr("shoulderpad"+side+"_anim.FollowClav", const+"."+shoulder_loc+"W0", f=True)
        cmds.connectAttr("shoulderpad"+side+"_anim.FollowClav", reverseNode+".inputX", f=True)
        cmds.connectAttr(reverseNode+".outputX", const+".shoulderpad"+side+"_anim_space_switcherW1", f=True)
    
    # Create the attrs for the stages
    cmds.addAttr("body_anim", ln = "Stages", dv = 0, min = 0, max = 4, keyable = True)
        
    # Set the controls to be in the positions for stage 0
    cmds.setAttr("spintal_cord_01_anim.scale", 0.57, 0.57, 0.57, type="double3")
    cmds.setAttr("forearm_armor_01_l_anim.scale", 0.72, 0.22, 0.14, type="double3")
    cmds.setAttr("forearm_armor_01_r_anim.scale", 0.72, 0.22, 0.14, type="double3")

    cmds.setAttr("shoulderA_01_l_anim.scale", 0.42, 0.61, 0.61, type="double3")
    cmds.setAttr("shoulderA_02_l_anim.scale", 0.42, 0.61, 0.61, type="double3")
    cmds.setAttr("shoulderB_01_l_anim_grp.translateX", -4.97)
    cmds.setAttr("shoulderB_01_l_anim.scale", 0.31, 0.31, 0.31, type="double3")
    cmds.setAttr("shoulderC_01_l_anim.scale", 0.165, 0.165, 0.165, type="double3")

    cmds.setAttr("shoulderA_01_r_anim.scale", 0.42, 0.61, 0.61, type="double3")
    cmds.setAttr("shoulderA_02_r_anim.scale", 0.42, 0.61, 0.61, type="double3")
    cmds.setAttr("shoulderB_01_r_anim_grp.translateX", 4.97)
    cmds.setAttr("shoulderB_01_r_anim.scale", 0.31, 0.31, 0.31, type="double3")
    cmds.setAttr("shoulderC_01_r_anim.scale", 0.165, 0.165, 0.165, type="double3")

    cmds.setAttr("shoulderB_blendshape.stage_3_shoulderB_MORPH", 0)
    cmds.setAttr("shoulderC_blendshape.stage_3_shoulderC_MORPH", 0)
    cmds.setAttr("shoulderC_blendshape.stage_4_shoulderC_MORPH", 0)
    cmds.setAttr("shoulderB_blendshape.stage_4_shoulderB_MORPH", 0)
    cmds.setAttr("spinal_cord_blendshape.stage_4_spinal_cord_MORPH", 0)

    cmds.setAttr("body_anim.Stages", 0)

    # Create SDK's.
    cmds.setDrivenKeyframe(["spintal_cord_01_anim.scale", "forearm_armor_01_l_anim.scale", "forearm_armor_01_r_anim.scale", "shoulderA_01_l_anim.scale", "shoulderA_02_l_anim.scale", "shoulderB_01_l_anim_grp.translateX", "shoulderB_01_l_anim.scale", "shoulderC_01_l_anim.scale", "shoulderA_01_r_anim.scale", "shoulderA_02_r_anim.scale", "shoulderB_01_r_anim_grp.translateX", "shoulderB_01_r_anim.scale", "shoulderC_01_r_anim.scale", "shoulderB_blendshape.stage_3_shoulderB_MORPH", "shoulderC_blendshape.stage_3_shoulderC_MORPH", "shoulderC_blendshape.stage_4_shoulderC_MORPH", "shoulderB_blendshape.stage_4_shoulderB_MORPH", "spinal_cord_blendshape.stage_4_spinal_cord_MORPH"], cd = "body_anim.Stages", itt = "flat", ott = "flat")

    cmds.setAttr("body_anim.Stages", 1)
    cmds.setAttr("shoulderA_01_l_anim.scale", 1, 1, 1, type="double3")
    cmds.setAttr("shoulderA_02_l_anim.scale", 1, 1, 1, type="double3")
    cmds.setAttr("shoulderA_01_r_anim.scale", 1, 1, 1, type="double3")
    cmds.setAttr("shoulderA_02_r_anim.scale", 1, 1, 1, type="double3")
    cmds.setDrivenKeyframe(["spintal_cord_01_anim.scale", "forearm_armor_01_l_anim.scale", "forearm_armor_01_r_anim.scale", "shoulderA_01_l_anim.scale", "shoulderA_02_l_anim.scale", "shoulderB_01_l_anim_grp.translateX", "shoulderB_01_l_anim.scale", "shoulderC_01_l_anim.scale", "shoulderA_01_r_anim.scale", "shoulderA_02_r_anim.scale", "shoulderB_01_r_anim_grp.translateX", "shoulderB_01_r_anim.scale", "shoulderC_01_r_anim.scale", "shoulderB_blendshape.stage_3_shoulderB_MORPH", "shoulderC_blendshape.stage_3_shoulderC_MORPH", "shoulderC_blendshape.stage_4_shoulderC_MORPH", "shoulderB_blendshape.stage_4_shoulderB_MORPH", "spinal_cord_blendshape.stage_4_spinal_cord_MORPH"], cd = "body_anim.Stages", itt = "flat", ott = "flat")


    cmds.setAttr("body_anim.Stages", 2)
    cmds.setAttr("spintal_cord_01_anim.scale", 1, 1, 1, type="double3")
    cmds.setAttr("shoulderB_01_l_anim_grp.translateX", 0)
    cmds.setAttr("shoulderB_01_l_anim.scale", 1, 1, 1, type="double3")
    cmds.setAttr("shoulderB_01_r_anim_grp.translateX", 0)
    cmds.setAttr("shoulderB_01_r_anim.scale", 1, 1, 1, type="double3")
    cmds.setDrivenKeyframe(["spintal_cord_01_anim.scale", "forearm_armor_01_l_anim.scale", "forearm_armor_01_r_anim.scale", "shoulderA_01_l_anim.scale", "shoulderA_02_l_anim.scale", "shoulderB_01_l_anim_grp.translateX", "shoulderB_01_l_anim.scale", "shoulderC_01_l_anim.scale", "shoulderA_01_r_anim.scale", "shoulderA_02_r_anim.scale", "shoulderB_01_r_anim_grp.translateX", "shoulderB_01_r_anim.scale", "shoulderC_01_r_anim.scale", "shoulderB_blendshape.stage_3_shoulderB_MORPH", "shoulderC_blendshape.stage_3_shoulderC_MORPH", "shoulderC_blendshape.stage_4_shoulderC_MORPH", "shoulderB_blendshape.stage_4_shoulderB_MORPH", "spinal_cord_blendshape.stage_4_spinal_cord_MORPH"], cd = "body_anim.Stages", itt = "flat", ott = "flat")

    cmds.setAttr("body_anim.Stages", 3)
    cmds.setAttr("shoulderC_01_l_anim.scale", 1, 1, 1, type="double3")
    cmds.setAttr("shoulderC_01_r_anim.scale", 1, 1, 1, type="double3")
    cmds.setAttr("forearm_armor_01_l_anim.scale", 1, 1, 1, type="double3")
    cmds.setAttr("forearm_armor_01_r_anim.scale", 1, 1, 1, type="double3")
    cmds.setAttr("shoulderB_blendshape.stage_3_shoulderB_MORPH", 1)
    cmds.setAttr("shoulderC_blendshape.stage_3_shoulderC_MORPH", 1)
    cmds.setDrivenKeyframe(["spintal_cord_01_anim.scale", "forearm_armor_01_l_anim.scale", "forearm_armor_01_r_anim.scale", "shoulderA_01_l_anim.scale", "shoulderA_02_l_anim.scale", "shoulderB_01_l_anim_grp.translateX", "shoulderB_01_l_anim.scale", "shoulderC_01_l_anim.scale", "shoulderA_01_r_anim.scale", "shoulderA_02_r_anim.scale", "shoulderB_01_r_anim_grp.translateX", "shoulderB_01_r_anim.scale", "shoulderC_01_r_anim.scale", "shoulderB_blendshape.stage_3_shoulderB_MORPH", "shoulderC_blendshape.stage_3_shoulderC_MORPH", "shoulderC_blendshape.stage_4_shoulderC_MORPH", "shoulderB_blendshape.stage_4_shoulderB_MORPH", "spinal_cord_blendshape.stage_4_spinal_cord_MORPH"], cd = "body_anim.Stages", itt = "flat", ott = "flat")

    cmds.setAttr("body_anim.Stages", 4)
    cmds.setAttr("shoulderB_blendshape.stage_3_shoulderB_MORPH", 0)
    cmds.setAttr("shoulderC_blendshape.stage_3_shoulderC_MORPH", 0)
    cmds.setAttr("shoulderC_blendshape.stage_4_shoulderC_MORPH", 1)
    cmds.setAttr("shoulderB_blendshape.stage_4_shoulderB_MORPH", 1)
    cmds.setAttr("spinal_cord_blendshape.stage_4_spinal_cord_MORPH", 1)
    cmds.setDrivenKeyframe(["spintal_cord_01_anim.scale", "forearm_armor_01_l_anim.scale", "forearm_armor_01_r_anim.scale", "shoulderA_01_l_anim.scale", "shoulderA_02_l_anim.scale", "shoulderB_01_l_anim_grp.translateX", "shoulderB_01_l_anim.scale", "shoulderC_01_l_anim.scale", "shoulderA_01_r_anim.scale", "shoulderA_02_r_anim.scale", "shoulderB_01_r_anim_grp.translateX", "shoulderB_01_r_anim.scale", "shoulderC_01_r_anim.scale", "shoulderB_blendshape.stage_3_shoulderB_MORPH", "shoulderC_blendshape.stage_3_shoulderC_MORPH", "shoulderC_blendshape.stage_4_shoulderC_MORPH", "shoulderB_blendshape.stage_4_shoulderB_MORPH", "spinal_cord_blendshape.stage_4_spinal_cord_MORPH"], cd = "body_anim.Stages", itt = "flat", ott = "flat")

    cmds.setAttr("body_anim.Stages", 0)

shoulderRig()

# ------------------------------------------------------------------
def armsLegsRig():
    cmds.setAttr("Rig_Settings.lUpperarmTwist2Amount", 0.3)
    cmds.setAttr("Rig_Settings.rUpperarmTwist2Amount", 0.3)

    cmds.setAttr("Rig_Settings.lForearmTwist2Amount", 0.3)
    cmds.setAttr("Rig_Settings.rForearmTwist2Amount", 0.3)
armsLegsRig()

# ------------------------------------------------------------------
def dynamicsOff():
    for one in ["hip_cloth_bk_l_dyn_anim", "hip_cloth_bk_r_dyn_anim", "hip_cloth_fr_l_dyn_anim", "hip_cloth_fr_r_dyn_anim", "lowerarm_cloth_bk_l_dyn_anim", "lowerarm_cloth_bk_r_dyn_anim", "lowerarm_cloth_fr_l_dyn_anim", "lowerarm_cloth_fr_r_dyn_anim", "pelvis_cloth_bk_dyn_anim", "pelvis_cloth_fr_dyn_anim", "spine_cloth_bk_l_dyn_anim", "spine_cloth_bk_r_dyn_anim", "spine_cloth_fr_l_dyn_anim", "spine_cloth_fr_r_dyn_anim", "stage1_clav_cloth_l_dyn_anim", "stage1_clav_cloth_r_dyn_anim", "upperarm_cloth_l_dyn_anim", "upperarm_cloth_r_dyn_anim"]:
        cmds.setAttr(one+".chainStartEnvelope", 0)
        cmds.setAttr(one+".chainStartEnvelope", l=True)
dynamicsOff()

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
addCapsule(42, 86)

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

# ------------------------------------------------------------------
#save the export file.
cmds.file(save = True, type = "mayaBinary", force = True)
