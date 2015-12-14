import maya.cmds as cmds

# NOTE: This character has special steps to run during Rig Pose creation.  See end of script.

# ------------------------------------------------------------------
# scale all of the controls to be more visible with the mesh.
def scaleControls():
    sides = ["_l", "_r"]
    for side in sides:
        # feet
        cmds.select(["ik_foot_anim"+side+".cv[*]"])
        cmds.scale(1, 1, 1, r = True, ocp = False)
        cmds.move(0, 0, 0, r = True)

        cmds.select(["ik_wrist"+side+"_anim.cv[*]"])
        cmds.scale(1, 1, 1, r = True, ocp = False)

    objects = ["hair_top_anim", "fk_ponytail_01_anim", "fk_ponytail_02_anim", "fk_ponytail_03_anim", "fk_ponytail_04_anim", "quiver_anim", "arrow_nock_anim", "bow_base_anim", "bow_shaft_top_01_anim", "bow_shaft_top_02_anim", "bow_shaft_top_03_anim", "bow_shaft_bott_01_anim", "bow_shaft_bott_02_anim", "bow_shaft_bott_03_anim", "bow_string_bott_anim", "bow_string_mid_anim", "bow_string_top_anim", "front_strap_small_anim", "fk_front_strap_01_anim", "fk_front_strap_02_anim", "fk_skirt_back_cloth_01_anim", "fk_skirt_back_cloth_02_anim", "fk_skirt_cloth_l_01_anim", "fk_skirt_cloth_l_02_anim", "fk_skirt_front_cloth_01_anim", "fk_skirt_front_cloth_02_anim", "breast_chain_anim", "feathers_anim", "fk_hair_curls_A_l_01_anim", "fk_hair_curls_A_l_02_anim", "fk_hair_curls_A_r_01_anim", "fk_hair_curls_A_r_02_anim", "fk_hair_curls_B_l_01_anim", "fk_hair_curls_B_l_02_anim", "fk_hair_curls_B_r_01_anim", "fk_hair_curls_B_r_02_anim", "fk_chest_cloth_l_01_anim", "fk_chest_cloth_l_02_anim", "fk_chest_cloth_r_01_anim", "fk_chest_cloth_r_02_anim", "fk_back_tail_cloth_01_anim", "fk_back_tail_cloth_02_anim"]
    for one in objects:
        cmds.select([one+".cv[*]"])
        cmds.scale(0.227057, 0.227057, 0.227057, r=True, ocp=False)
        cmds.move(0, 0, 0, r = True, os=True, wd=True)

    cmds.select(["arrow_nock_anim.cv[*]"])
    cmds.scale(0, 0.3, 4, r = True, ocp = True)

    cmds.select(["bow_string_mid_anim.cv[*]"])
    cmds.scale(0, 0.6, 4, r = True, ocp = True)

    cmds.select(None)
scaleControls()

# ------------------------------------------------------------------
def bowAndArrowRig():
    import Tools.ART_addSpaceSwitching as space
    reload(space)
        
    # Create Arrow base and tip controls
    arrow_base_anim = cmds.duplicate("arrow_nock_anim", n="arrow_base_anim")
    cmds.xform(arrow_base_anim, t=[0, 0, -35.51354])
    cmds.makeIdentity(arrow_base_anim, a=True, t=True)

    arrow_tip_anim = cmds.duplicate("arrow_nock_anim", n="arrow_tip_anim")
    cmds.xform(arrow_tip_anim, t=[0, 0, -78.67631])
    cmds.makeIdentity(arrow_tip_anim, a=True, t=True)

    cmds.parent(arrow_base_anim, "arrow_nock_anim")
    cmds.parent(arrow_tip_anim, arrow_base_anim)

    cmds.delete("driver_arrow_nock_parentConstraint1")
    cmds.parentConstraint(arrow_tip_anim, "driver_arrow_nock", mo=True)
    
    # Create aimConstraints for the ends of the strings
    cmds.aimConstraint("bow_string_mid_anim", "bow_string_bott_anim_grp", mo=True, aimVector=(0, 0, -1), upVector=(0, -1, 0), worldUpType="object", worldUpObject="bow_base_anim")
    cmds.aimConstraint("bow_string_mid_anim", "bow_string_top_anim_grp", mo=True, aimVector=(0, 0, -1), upVector=(0, -1, 0), worldUpType="object", worldUpObject="bow_base_anim")

    # Create SDK's for the bow shafts to flex when the string is pulled.
    cmds.setAttr("bow_string_mid_anim.translateY", 0)
    cmds.setAttr("bow_shaft_top_01_anim_grp.rotateX", 0)
    cmds.setAttr("bow_shaft_top_02_anim_grp.rotateX", 0)
    cmds.setAttr("bow_shaft_top_03_anim_grp.rotateX", 0)
    cmds.setAttr("bow_shaft_bott_01_anim_grp.rotateX", 0)
    cmds.setAttr("bow_shaft_bott_02_anim_grp.rotateX", 0)
    cmds.setAttr("bow_shaft_bott_03_anim_grp.rotateX", 0)
    cmds.setDrivenKeyframe(["bow_shaft_top_01_anim_grp.rx", "bow_shaft_top_02_anim_grp.rx", "bow_shaft_top_03_anim_grp.rx", "bow_shaft_bott_01_anim_grp.rx", "bow_shaft_bott_02_anim_grp.rx", "bow_shaft_bott_03_anim_grp.rx"], cd = "bow_string_mid_anim.translateY", itt = "spline", ott = "spline")

    cmds.setAttr("bow_string_mid_anim.translateY", 120)
    cmds.setAttr("bow_shaft_top_01_anim_grp.rotateX", -10)
    cmds.setAttr("bow_shaft_top_02_anim_grp.rotateX", -14)
    cmds.setAttr("bow_shaft_top_03_anim_grp.rotateX", -19.4)
    cmds.setAttr("bow_shaft_bott_01_anim_grp.rotateX", -10)
    cmds.setAttr("bow_shaft_bott_02_anim_grp.rotateX", -14)
    cmds.setAttr("bow_shaft_bott_03_anim_grp.rotateX", -19.4)
    cmds.setDrivenKeyframe(["bow_shaft_top_01_anim_grp.rx", "bow_shaft_top_02_anim_grp.rx", "bow_shaft_top_03_anim_grp.rx", "bow_shaft_bott_01_anim_grp.rx", "bow_shaft_bott_02_anim_grp.rx", "bow_shaft_bott_03_anim_grp.rx"], cd = "bow_string_mid_anim.translateY", itt = "spline", ott = "spline")

    cmds.setAttr("bow_string_mid_anim.translateY", 0)
    cmds.setAttr("bow_shaft_top_01_anim_grp.rotateX", 0)
    cmds.setAttr("bow_shaft_top_02_anim_grp.rotateX", 0)
    cmds.setAttr("bow_shaft_top_03_anim_grp.rotateX", 0)
    cmds.setAttr("bow_shaft_bott_01_anim_grp.rotateX", 0)
    cmds.setAttr("bow_shaft_bott_02_anim_grp.rotateX", 0)
    cmds.setAttr("bow_shaft_bott_03_anim_grp.rotateX", 0)

    # Move the arrow to the string
    cmds.setAttr("arrow_nock_anim_grp.translateX", 0.05019)
    cmds.setAttr("arrow_nock_anim_grp.translateY", 15.07779)
    cmds.setAttr("arrow_nock_anim_grp.translateZ", -35.42169)
    cmds.setAttr("arrow_nock_anim_grp.rotateX", -90)
    cmds.setAttr("arrow_nock_anim_grp.rotateY", 0)
    cmds.setAttr("arrow_nock_anim_grp.rotateZ", 0)

    # Make the arrow follow the string using space switching
    space.addSpaceSwitching("arrow_nock_anim", "bow_string_mid_anim", True)

    #Move the string to an appropriate position relative to where it might be aligned when shooting.
    cmds.setAttr("bow_string_mid_anim.translateX", -2.45216)
    cmds.setAttr("bow_string_mid_anim.translateZ", 5.9665)

    # Create an aim constraint rig so that the arrow will always stay aimed at its position on the shaft.
    arrow_aim_target = cmds.circle(n="arrow_notched_target", r=3.4)[0]
    cmds.xform(arrow_aim_target, t=(-2.18154, 0.91122, 6.11286))
    arrow_up_target = cmds.spaceLocator(n="arrow_up_target")
    cmds.xform(arrow_up_target, t=(-2.18154, 0.91122, 36))
    cmds.parent(arrow_aim_target, arrow_up_target, "bow_base_anim")
    cmds.makeIdentity(arrow_aim_target, a=True, t=True, r=True)
    cmds.aimConstraint(arrow_aim_target, "arrow_nock_anim_grp", mo=True, aimVector=(0, 0, -1), upVector=(0, -1, 0), worldUpType="object", worldUpObject=arrow_up_target[0])

    # Add space switching for the string to follow the hand.  Off by default.
    space.addSpaceSwitching("bow_string_mid_anim", "driver_hand_r", False)
    
    # Move the bow to the left hand.
    cmds.setAttr("bow_base_anim.translateX", 74.70517)
    cmds.setAttr("bow_base_anim.translateY", 6.12268)
    cmds.setAttr("bow_base_anim.translateZ", 144.79125)
    cmds.setAttr("bow_base_anim.rotateX", 183.95171)
    cmds.setAttr("bow_base_anim.rotateY", -86.42158)
    cmds.setAttr("bow_base_anim.rotateZ", -83.33129)

bowAndArrowRig()

# ------------------------------------------------------------------
def armsLegsRig():
    cmds.setAttr("Rig_Settings.lUpperarmTwist2Amount", 0.3)
    cmds.setAttr("Rig_Settings.rUpperarmTwist2Amount", 0.3)

    cmds.setAttr("Rig_Settings.lForearmTwist2Amount", 0.3)
    cmds.setAttr("Rig_Settings.rForearmTwist2Amount", 0.3)

    cmds.setAttr("Rig_Settings.lThighTwist2Amount", 0.3)
    cmds.setAttr("Rig_Settings.rThighTwist2Amount", 0.3)

    cmds.setAttr("Rig_Settings.lCalfTwistAmount", 0.7)
    cmds.setAttr("Rig_Settings.rCalfTwistAmount", 0.7)

armsLegsRig()

# ------------------------------------------------------------------
def dynamicsOff():
    for one in ["back_tail_cloth_dyn_anim", "chest_cloth_l_dyn_anim", "chest_cloth_r_dyn_anim", "front_strap_dyn_anim", "hair_curls_A_l_dyn_anim", "hair_curls_A_r_dyn_anim", "hair_curls_B_l_dyn_anim", "hair_curls_B_r_dyn_anim", "ponytail_dyn_anim", "skirt_back_cloth_dyn_anim", "skirt_cloth_l_dyn_anim", "skirt_front_cloth_dyn_anim"]:
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



#########################################
# RUN THIS COMMAND DURING THE RIG POSE CREATION STEP TO GET THE SHIELD TO THE CORRECT LOCATION.  LEAVE IT COMMENTED OUT SO THAT IT DOESNT RUN DURING RIGGING.
'''
# The following 2 lines are used to find the locations and then input below.
#cmds.xform(q=True, ws=True, rp=True)
#cmds.xform(q=True, ro=True)


cmds.xform("arrow_nock_mover", ws=True, t=[9.611692837947271e-14, -2.0994091418383203e-16, 35.41695785522444])
cmds.xform("arrow_nock_mover", ro=[-13.78281392154952, -64.119833503541, 16.763202342514436])

cmds.xform("bow_base_mover", ws=True, t=[3.3590546269413172e-15, -3.762654592984341e-15, -7.492745461319391e-15])
cmds.xform("bow_base_mover", ro=[-13.782813921549694, 64.11983350354099, -16.763202342514536])

'''
#########################################