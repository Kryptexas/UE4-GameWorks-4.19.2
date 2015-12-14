import maya.cmds as cmds
# THIS CHARACTER HAS A CUSTOM CHANGE TO HIS RIG POSE THAT NEEDS TO BE DONE IF STEPPED BACK IN ART!!!!!
# ------------------------------------------------------------------
# IF you have to go back to the joint mover stage, then select the old rigid mesh group and the root joint, and run the rigid mesh script at the bottom of this file.  
# ALSO need to rotate the hands to match the locators for them in the scene.

# ------------------------------------------------------------------
# scale all of the controls to be more visible with the mesh.
def scaleControls():
    for side in ["_l", "_r"]:
        # Arms        
        cmds.select(["ik_wrist"+side+"_anim.cv[0:7]"])
        cmds.scale(2.664125, 2.664125, 2.664125, r=True, ocp=False)

        cmds.select(["thumb_finger_fk_ctrl_1"+side+".cv[0:7]"])
        cmds.scale(5.362914, 5.362914, 5.362914, r=True, ocp=False)

        cmds.select(["thumb_finger_fk_ctrl_2"+side+".cv[0:7]"])
        cmds.scale(5.362914, 5.362914, 5.362914, r=True, ocp=False)

        cmds.select(["index_finger_fk_ctrl_1"+side+".cv[0:7]"])
        cmds.scale(5.362914, 5.362914, 5.362914, r=True, ocp=False)

        cmds.select(["index_finger_fk_ctrl_2"+side+".cv[0:7]"])
        cmds.scale(5.362914, 5.362914, 5.362914, r=True, ocp=False)

        cmds.select(["index_finger_fk_ctrl_3"+side+".cv[0:7]"])
        cmds.scale(5.362914, 5.362914, 5.362914, r=True, ocp=False)

        cmds.select(["pinky_finger_fk_ctrl_1"+side+".cv[0:7]"])
        cmds.scale(5.362914, 5.362914, 5.362914, r=True, ocp=False)

        cmds.select(["pinky_finger_fk_ctrl_2"+side+".cv[0:7]"])
        cmds.scale(5.362914, 5.362914, 5.362914, r=True, ocp=False)

        cmds.select(["pinky_finger_fk_ctrl_3"+side+".cv[0:7]"])
        cmds.scale(5.362914, 5.362914, 5.362914, r=True, ocp=False)

        for one in ["fk_arm", "fk_elbow", "fk_wrist"]:
            cmds.select([one+side+"_anim.cv[0:7]"])
            cmds.scale(2.371997, 2.371997, 2.371997, r=True, ocp=True)

        cmds.select(["fk_clavicle"+side+"_anim.cv[0:32]"])
        cmds.scale(1, 1, 1, r=True, ocp=False)
        cmds.move(0, 0, 13, r=True)

        # Legs        
        cmds.select(["ik_foot_anim"+side+".cv[0:17]"])
        cmds.scale(2.0357, 2.0357, 2.0357, r=True, ocp=False)
        cmds.move(0, 21.158168, 15.019494, r=True)

        cmds.select(["toe_index_01"+side+"_anim.cv[0:32]"])
        cmds.scale(0.514234, 0.514234, 0.514234, r=True, ocp=False)
        cmds.move(0, 0, 3.9, r=True)
        
        cmds.select(["toe_index_02"+side+"_anim.cv[0:32]"])
        cmds.scale(0.48, 0.48, 0.48, r=True, ocp=False)
        cmds.move(0, 0, 2.9, r=True)

        cmds.select(["toe_index_03"+side+"_anim.cv[0:32]"])
        cmds.scale(0.441295, 0.441295, 0.441295, r=True, ocp=False)
        cmds.move(0, 0, 1.9, r=True)

        cmds.select(["toe_mid_01"+side+"_anim.cv[0:32]"])
        cmds.scale(0.514234, 0.514234, 0.514234, r=True, ocp=False)
        cmds.move(0, 0, 3.9, r=True)
        
        cmds.select(["toe_mid_02"+side+"_anim.cv[0:32]"])
        cmds.scale(0.48, 0.48, 0.48, r=True, ocp=False)
        cmds.move(0, 0, 2.9, r=True)

        cmds.select(["toe_mid_03"+side+"_anim.cv[0:32]"])
        cmds.scale(0.441295, 0.441295, 0.441295, r=True, ocp=False)
        cmds.move(0, 0, 1.9, r=True)
        
        cmds.select(["toe_pinky_01"+side+"_anim.cv[0:32]"])
        cmds.scale(0.514234, 0.514234, 0.514234, r=True, ocp=False)
        cmds.move(0, 0, 3.9, r=True)
        
        cmds.select(["toe_pinky_02"+side+"_anim.cv[0:32]"])
        cmds.scale(0.48, 0.48, 0.48, r=True, ocp=False)
        cmds.move(0, 0, 2.9, r=True)

        cmds.select(["toe_pinky_03"+side+"_anim.cv[0:32]"])
        cmds.scale(0.441295, 0.441295, 0.441295, r=True, ocp=False)        
        cmds.move(0, 0, 1.9, r=True)

        # Face Tentacles
        for one in ["a", "c", "d", "e", "f", "g", "h", "i", "k"]:
            cmds.select(["fk_chin_tent_"+one+side+"_01_anim.cv[0:7]"])
            cmds.scale(0.25, 0.25, 0.25, r=True, ocp=False)

            cmds.select(["fk_chin_tent_"+one+side+"_02_anim.cv[0:7]"])
            cmds.scale(0.16, 0.16, 0.16, r=True, ocp=False)
        
        # Fins
        cmds.select(["fin_arm_a"+side+"_anim.cv[0:32]"])
        cmds.scale(0.68, 0.68, 0.68, r=True, ocp=True)

        cmds.select(["fin_scap_d"+side+"_anim.cv[0:32]"])
        cmds.scale(0.68, 0.68, 0.68, r=True, ocp=True)

        # brows
        cmds.select(["eyebrow_in"+side+"_anim.cv[0:32]"])
        cmds.scale(0.132495, 0.132495, 0.132495, r=True, ocp=True)

        # eyes
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

    # jaw
    cmds.select(["jaw_anim.cv[0:32]"])
    cmds.scale(0.294922, 0.294922, 0.294922, r=True, ocp=True)
    cmds.move(0, -12.8, -10.7, r=True)

    cmds.select(["eyebrow_mid_anim.cv[0:32]"])
    cmds.scale(0.132495, 0.132495, 0.132495, r=True, ocp=True)
    
    cmds.select(["l_global_ik_anim.cv[0:4]", "r_global_ik_anim.cv[0:4]"])
    cmds.scale(1, 1, 1, r=True, ocp=False)
    cmds.move(0, 0, -22.886708, r=True)


    cmds.select(None)
scaleControls()

cmds.setAttr("Rig_Settings.lUpperarmTwistAmount", 1)
cmds.setAttr("Rig_Settings.lUpperarmTwist2Amount", 0.4)
cmds.setAttr("Rig_Settings.rUpperarmTwistAmount", 1)
cmds.setAttr("Rig_Settings.rUpperarmTwist2Amount", 0.4)

# ------------------------------------------------------------------
# Adding more constraints to the Fins on the arms to follow the forearms.
def makeFinsFollowForearms():
    for side in ["_l", "_r"]:
        cmds.parentConstraint("driver_lowerarm"+side, "fin_arm_a"+side+"_parent_grp", mo=True, weight=1)
        cmds.parentConstraint("driver_lowerarm"+side, "fk_fin_arm_b"+side+"_01_grp", mo=True, weight=1)
        cmds.parentConstraint("driver_lowerarm_twist_01"+side, "fk_fin_arm_c"+side+"_01_grp", mo=True, weight=1)
        cmds.parentConstraint("driver_lowerarm_twist_01"+side, "fk_fin_arm_d"+side+"_01_grp", mo=True, weight=1)

        cmds.setAttr("fin_arm_a"+side+"_parent_grp_parentConstraint1.interpType", 2)
        cmds.setAttr("fk_fin_arm_b"+side+"_01_grp_parentConstraint1.interpType", 2)
        cmds.setAttr("fk_fin_arm_c"+side+"_01_grp_parentConstraint1.interpType", 2)
        cmds.setAttr("fk_fin_arm_d"+side+"_01_grp_parentConstraint1.interpType", 2)

        cmds.setAttr("fin_arm_a"+side+"_parent_grp_parentConstraint1.driver_lowerarm_twist_01"+side+"W0", 10)
        cmds.setAttr("fk_fin_arm_b"+side+"_01_grp_parentConstraint1.driver_lowerarm_twist_01"+side+"W0", 1.2)
        cmds.setAttr("fk_fin_arm_c"+side+"_01_grp_parentConstraint1.driver_lowerarm"+side+"W0", 2.2)
        cmds.setAttr("fk_fin_arm_d"+side+"_01_grp_parentConstraint1.driver_lowerarm"+side+"W0", 100)       
makeFinsFollowForearms()

# ------------------------------------------------------------------
# Create all of the blendshapes that are needed an hook them up.
def createBlendshapes():
    # Create the blendshape node for the biceps - Run the following line in the export file.  This way the blendshapes are in the export to the game.
    #blendshape = cmds.blendShape("body_geo__bicep_l", "body_geo__bicep_r", "body_geo__shoulder_up_l", "body_geo__shoulder_up_r", "body_geo", name="body_geo_blendShape", foc=True, tc=True, origin="local")

    cmds.setAttr("Rig_Settings.lArmMode", 0)
    cmds.setAttr("Rig_Settings.lClavMode", 0)
    cmds.setAttr("Rig_Settings.rArmMode", 0)
    cmds.setAttr("Rig_Settings.rClavMode", 0)

    for side in ["_l", "_r"]:
        # Create SDK's to drive the shoulder blendshape by thex clavicle joint
        cmds.setAttr("fk_clavicle"+side+"_anim.ry", 7)
        cmds.setAttr("body_geo_blendShape.body_geo__shoulder_up"+side, 0)
        cmds.setDrivenKeyframe("body_geo_blendShape.body_geo__shoulder_up"+side, cd="clavicle"+side+".rotateY", itt="flat", ott="flat")
        cmds.setAttr("fk_clavicle"+side+"_anim.ry", -46)
        cmds.setAttr("body_geo_blendShape.body_geo__shoulder_up"+side, 1)
        cmds.setDrivenKeyframe("body_geo_blendShape.body_geo__shoulder_up"+side, cd="clavicle"+side+".rotateY", itt="flat", ott="flat")
        cmds.setAttr("fk_clavicle"+side+"_anim.ry", 0)

        # Create SDK's to drive the bicep blendshape by the elbow joint
        cmds.setAttr("fk_elbow"+side+"_anim.rz", 0)
        cmds.setAttr("body_geo_blendShape.body_geo__bicep"+side, 0)
        cmds.setDrivenKeyframe("body_geo_blendShape.body_geo__bicep"+side, cd="lowerarm"+side+".rotateZ", itt="flat", ott="flat")
        cmds.setAttr("fk_elbow"+side+"_anim.rz", -100)
        cmds.setAttr("body_geo_blendShape.body_geo__bicep"+side, 1)
        cmds.setDrivenKeyframe("body_geo_blendShape.body_geo__bicep"+side, cd="lowerarm"+side+".rotateZ", itt="flat", ott="flat")
        cmds.setAttr("fk_elbow"+side+"_anim.rz", 0)

    cmds.setAttr("Rig_Settings.lArmMode", 1)
    cmds.setAttr("Rig_Settings.lClavMode", 1)
    cmds.setAttr("Rig_Settings.rArmMode", 1)
    cmds.setAttr("Rig_Settings.rClavMode", 1)
createBlendshapes()

# ------------------------------------------------------------------
# Adding more constraints to the chin tentacles on the face so they can partially follow the head more.
def chinTentaclesOnJaw():
    for side in ["_l", "_r"]:
        tentacleDict = {"e":0.1, "f":0.5, "g":0.6, "h":1, "i":5}
        for one in tentacleDict:
            cmds.parentConstraint("driver_head", "fk_chin_tent_"+one+side+"_01_grp", mo=True, weight=1)
            cmds.setAttr("fk_chin_tent_"+one+side+"_01_grp_parentConstraint1.interpType", 2)
            cmds.setAttr("fk_chin_tent_"+one+side+"_01_grp_parentConstraint1.driver_headW1", tentacleDict[one])
chinTentaclesOnJaw()

# ------------------------------------------------------------------
def fixUpToes():
    # change the way the Set driven keys on the heel and ball pivots work so that the leg can work more like a quadruped leg.
    cmds.keyframe("ik_foot_heel_pivot_l_rotateZ", edit=True, index=(2,2), absolute=True, valueChange=0)
    cmds.keyframe("ik_foot_ball_pivot_l_rotateZ", edit=True, index=(2,2), absolute=True, valueChange=90)

    cmds.keyframe("ik_foot_heel_pivot_r_rotateZ", edit=True, index=(2,2), absolute=True, valueChange=0)
    cmds.keyframe("ik_foot_ball_pivot_r_rotateZ", edit=True, index=(2,2), absolute=True, valueChange=90)
    
    cmds.delete("toe_rig_l_grp", "toe_rig_r_grp")
fixUpToes()

# ------------------------------------------------------------------
def backFins():
    for side in ["_l", "_r"]:
        cmds.parentConstraint("driver_spine_02", "fk_fin_scap_b"+side+"_01_grp", mo=True, weight=1)
        cmds.setAttr("fk_fin_scap_b"+side+"_01_grp_parentConstraint1.interpType", 2)
        cmds.setAttr("fk_fin_scap_b"+side+"_01_grp_parentConstraint1.driver_spine_02W1", 0.4)

        cmds.parentConstraint("driver_spine_02", "fk_fin_scap_c"+side+"_01_grp", mo=True, weight=1)
        cmds.setAttr("fk_fin_scap_c"+side+"_01_grp_parentConstraint1.interpType", 2)
        cmds.setAttr("fk_fin_scap_c"+side+"_01_grp_parentConstraint1.driver_spine_02W1", 0.7)

        cmds.parentConstraint("driver_spine_02", "fin_scap_d"+side+"_parent_grp", mo=True, weight=1)
        cmds.setAttr("fin_scap_d"+side+"_parent_grp_parentConstraint1.interpType", 2)
        cmds.setAttr("fin_scap_d"+side+"_parent_grp_parentConstraint1.driver_spine_02W1", 1.5)

    tentacleDict = {"b":0.4, "c":0.7, "d":1.5}
    for one in tentacleDict:
        cmds.parentConstraint("driver_spine_02", "fk_fin_spine_"+one+"_01_grp", mo=True, weight=1)
        cmds.setAttr("fk_fin_spine_"+one+"_01_grp_parentConstraint1.interpType", 2)
        cmds.setAttr("fk_fin_spine_"+one+"_01_grp_parentConstraint1.driver_spine_02W1", tentacleDict[one])
backFins()

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
def dynamicsOff():
    for one in ["chain_dyn_anim", "chin_tent_a_l_dyn_anim", "chin_tent_a_r_dyn_anim", "chin_tent_c_l_dyn_anim", "chin_tent_c_r_dyn_anim", "chin_tent_d_l_dyn_anim", "chin_tent_d_r_dyn_anim", "chin_tent_e_l_dyn_anim", "chin_tent_e_r_dyn_anim", "chin_tent_f_l_dyn_anim", "chin_tent_f_r_dyn_anim", "chin_tent_g_l_dyn_anim", "chin_tent_g_r_dyn_anim", "chin_tent_h_l_dyn_anim", "chin_tent_h_r_dyn_anim", "chin_tent_i_l_dyn_anim", "chin_tent_i_r_dyn_anim", "chin_tent_k_l_dyn_anim", "chin_tent_k_r_dyn_anim", "fin_arm_b_l_dyn_anim", "fin_arm_b_r_dyn_anim", "fin_arm_c_l_dyn_anim", "fin_arm_c_r_dyn_anim", "fin_arm_d_l_dyn_anim", "fin_arm_d_r_dyn_anim", "fin_head_a_l_dyn_anim", "fin_head_a_r_dyn_anim", "fin_head_b_l_dyn_anim", "fin_head_b_r_dyn_anim", "fin_head_c_l_dyn_anim", "fin_head_c_r_dyn_anim", "fin_head_mid_dyn_anim", "fin_scap_a_l_dyn_anim", "fin_scap_a_r_dyn_anim", "fin_scap_b_l_dyn_anim", "fin_scap_b_r_dyn_anim", "fin_scap_c_l_dyn_anim", "fin_scap_c_r_dyn_anim", "fin_spine_a_dyn_anim", "fin_spine_b_dyn_anim", "fin_spine_c_dyn_anim", "fin_spine_d_dyn_anim"]:
        cmds.setAttr(one+".chainStartEnvelope", 0)
        cmds.setAttr(one+".chainStartEnvelope", l=True)
dynamicsOff()

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
addCapsule(80, 90)

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



# --------------------------------ONLY FOR MANUAL USE.  NOT PART OF THE POST SCRIPT BUILD----------------------------------
# Select a group of geo and a skeleton heirarcy and run this script and it will search in the skeleton
# for a joint with the same name minus "_geo".  If it finds one it will parent constrain the geo to the joint.
def ConstrainRigidMesh():
    selected = cmds.ls(selection=True)
    
    if not len(selected) is 2:
        cmds.confirmDialog(icon = "warning", title = "Rigid Mesh Constrainer", message = "Select only 2 objects.  Geo group then skeleton root.   Geo names need to match the joint+_geo.")
        return
    
    geoList = cmds.listRelatives(selected[0], ad=True, type="transform")
    print geoList
    skelList = cmds.listRelatives(selected[1], ad=True, type="joint")
        
    for geo in geoList:
        for i in range(0,len(skelList)):
            if skelList[i]+"_rigidgeo" == geo:
                const = cmds.parentConstraint(skelList[i], geo, name=geo+"_PrntCnst", mo=True, weight=1)[0]
#ConstrainRigidMesh()