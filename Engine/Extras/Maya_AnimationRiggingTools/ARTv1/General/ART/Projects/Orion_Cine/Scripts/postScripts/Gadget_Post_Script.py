import maya.cmds as cmds
import Tools.ART_CreateHydraulicRigs as hyd
reload(hyd)
import Tools.ART_addSpaceSwitching as space
reload(space)

# ------------------------------------------------------------------
# scale all of the controls to be more visible with the mesh.
def scaleControls():
    sides = ["_l", "_r"]
    for side in sides:
        # feet
        cmds.select(["ik_foot_anim"+side+".cv[0:17]"])
        cmds.scale(1.0, 1.0, 1.0, r = True, ocp = False)
        cmds.move(0, 0, -8.4, r = True)

        cmds.select(["fk_helmet_straps"+side+"_01_anim.cv[0:7]"])
        cmds.scale(0.204714, 0.204714, 0.204714, r = True, ocp = False)
        cmds.move(0, 0, 0, r = True)

        cmds.select(["fk_helmet_straps"+side+"_02_anim.cv[0:7]"])
        cmds.scale(0.204714, 0.204714, 0.204714, r = True, ocp = False)
        cmds.move(0, 0, 0, r = True)

    # Other

    cmds.select(["backpack_gas_01_anim.cv[0:32]"])
    cmds.scale(0.391111, 0.391111, 0.391111, r = True, ocp = True)
    cmds.move(-7.1, 0, 0, r = True, os=True, wd=True)

    cmds.select(["backpack_gas_02_anim.cv[0:32]"])
    cmds.scale(0.391111, 0.391111, 0.391111, r = True, ocp = True)
    cmds.move(-7.1, 0, 0, r = True, os=True, wd=True)

    cmds.select(["thigh_pouch_sml_anim.cv[0:32]"])
    cmds.scale(0.391111, 0.391111, 0.391111, r = True, ocp = True)
    cmds.move(0, -6.6, 0, r = True, os=True, wd=True)

    cmds.select(["thigh_pouch_big_anim.cv[0:32]"])
    cmds.scale(0.391111, 0.391111, 0.391111, r = True, ocp = True)
    cmds.move(0, -6.6, 0, r = True, os=True, wd=True)

    cmds.select(["backpack_base_anim.cv[0:32]"])
    cmds.scale(0.306684, 1, 0.306684, r = True, ocp = True)
    cmds.move(0, 24.6, 0, r = True, os=True, wd=True)

    cmds.select(["shield_bott_anim.cv[0:32]"])
    cmds.scale(0.301673, 0.301673, 0.301673, r = True, ocp = True)
    cmds.move(7.937765, 12.982729, -6.212037, r = True, os=True, wd=True)

    cmds.select(["shield_top_anim.cv[0:32]"])
    cmds.scale(0.301673, 0.301673, 0.301673, r = True, ocp = True)
    cmds.move(7.788894, 16.519037, 10.864125, r = True, os=True, wd=True)

    cmds.select(["spring_anim.cv[0:32]"])
    cmds.scale(1.0, 1.0, 0.001, r = True, ocp = True)

    cmds.select(["canister_main_anim.cv[0:32]"])
    cmds.scale(1.252256, 1.252256, 1.252256, r = True, ocp = True)

    cmds.select(["robo_uparm_pistonrod_anim.cv[0:32]"])
    cmds.scale(0.1, 0.447259, 0.447259, r = True, ocp = True)

    cmds.select(["robo_uparm_pistonrod_01_anim.cv[0:32]"])
    cmds.scale(0.1, 0.35, 0.35, r = True, ocp = True)

    cmds.select(["robo_lowarm_pistonrod_01_anim.cv[0:32]"])
    cmds.scale(0.1, 0.45, 0.45, r = True, ocp = True)

    cmds.select(["robo_lowarm_pistonrod_02_anim.cv[0:32]"])
    cmds.scale(0.1, 0.677192, 0.677192, r = True, ocp = True)

    cmds.select(["fk_robo_arm_01_anim.cv[0:7]"])
    cmds.scale(1.772257, 1.772257, 1.772257, r = True, ocp = False)

    cmds.select(["fk_robo_arm_02_anim.cv[0:7]"])
    cmds.scale(0.699603, 10.699603, 0.699603, r = True, ocp = False)

    cmds.select(["fk_robo_arm_03_anim.cv[0:7]"])
    cmds.scale(1.772257, 1.772257, 1.772257, r = True, ocp = False)

    cmds.select(["fk_robo_arm_04_anim.cv[0:7]"])
    cmds.scale(0.699603, 0.699603, 0.699603, r = True, ocp = False)

    cmds.select(["fk_robo_arm_05_anim.cv[0:7]"])
    cmds.scale(1.772257, 1.772257, 1.772257, r = True, ocp = False)

    cmds.select(["fk_robo_arm_06_anim.cv[0:7]"])
    cmds.scale(0.699603, 0.699603, 0.699603, r = True, ocp = False)

    cmds.select(["fk_robo_arm_07_anim.cv[0:7]"])
    cmds.scale(1.772257, 1.772257, 1.772257, r = True, ocp = False)

    cmds.select(["fk_robo_thumb_01_anim.cv[0:7]"])
    cmds.scale(1.0, 1.0, 1.0, r = True, ocp = True)

    cmds.select(["fk_robo_thumb_02_anim.cv[0:7]"])
    cmds.scale(1.0, 1.0, 1.0, r = True, ocp = True)

    cmds.select(["fk_robo_pointer_01_anim.cv[0:7]"])
    cmds.scale(1.0, 1.0, 1.0, r = True, ocp = True)

    cmds.select(["fk_robo_pointer_02_anim.cv[0:7]"])
    cmds.scale(1.0, 1.0, 1.0, r = True, ocp = True)

    cmds.select(["fk_robo_pinky_01_anim.cv[0:7]"])
    cmds.scale(1.0, 1.0, 1.0, r = True, ocp = True)

    cmds.select(["fk_robo_pinky_02_anim.cv[0:7]"])
    cmds.scale(1.0, 1.0, 1.0, r = True, ocp = True)

    cmds.select(["arm_wire_01_anim.cv[0:32]"])
    cmds.scale(0.5, 0.5, 0.5, r = True, ocp = False)

    cmds.select(["arm_wire_02_anim.cv[0:32]"])
    cmds.scale(0.5, 0.5, 0.5, r = True, ocp = False)

    cmds.select(None)

scaleControls()

# ------------------------------------------------------------------
def buildRoboArmRigFK():
    import Tools.ART_CreateHydraulicRigs as hyd
    reload(hyd)

    # Delete all of the dynamics for the arms as we dont need them.
    cmds.delete("rig_dyn_robo_arm_01_HairControls","rig_dyn_robo_pinky_01_HairControls","rig_dyn_robo_pointer_01_HairControls","rig_dyn_robo_thumb_01_HairControls")

    # Lock the attrs on the piston controls for the robo arm so they cant accidentally be animated.
    for one in ["fk_robo_arm_06_anim", "fk_robo_arm_04_anim", "fk_robo_arm_02_anim"]:
        cmds.setAttr(one+".tx", lock=True)
        cmds.setAttr(one+".tz", lock=True)
        cmds.setAttr(one+".rx", lock=True)
        cmds.setAttr(one+".rz", lock=True)

    for one in ["fk_robo_arm_03_anim", "fk_robo_arm_05_anim"]:
        cmds.setAttr(one+".tx", lock=True)
        cmds.setAttr(one+".ty", lock=True)
        cmds.setAttr(one+".tz", lock=True)
        cmds.setAttr(one+".ry", lock=True)
        cmds.setAttr(one+".rz", lock=True)

    cmds.select("robo_lowarm_pistonrod_01_anim", "robo_lowarm_pistonrod_02_anim", "driver_robo_arm_05")
    hyd.CreateHydraulicRig(-1)

    cmds.select("robo_uparm_pistonrod_01_anim", "robo_uparm_pistonrod_anim", "driver_robo_arm_02")
    hyd.CreateHydraulicRig(-1)

    # Create a slide mechanism on the forearm part of the lowerarm piston.
    cmds.addAttr("robo_lowarm_pistonrod_02_anim", ln = "Slide", dv = 0, min=0, max=10, keyable = True)
    cmds.setDrivenKeyframe("robo_lowarm_pistonrod_02_anim_grp.translateX", cd="robo_lowarm_pistonrod_02_anim.Slide", itt="spline", ott="spline")
    cmds.setDrivenKeyframe("robo_lowarm_pistonrod_02_anim_grp.translateZ", cd="robo_lowarm_pistonrod_02_anim.Slide", itt="spline", ott="spline")

    cmds.setAttr("robo_lowarm_pistonrod_02_anim.Slide", 10)
    cmds.setAttr("robo_lowarm_pistonrod_02_anim_grp.tx", -6.662443)
    cmds.setAttr("robo_lowarm_pistonrod_02_anim_grp.tz", -5.510393)

    cmds.setDrivenKeyframe("robo_lowarm_pistonrod_02_anim_grp.translateX", cd="robo_lowarm_pistonrod_02_anim.Slide", itt="spline", ott="spline")
    cmds.setDrivenKeyframe("robo_lowarm_pistonrod_02_anim_grp.translateZ", cd="robo_lowarm_pistonrod_02_anim.Slide", itt="spline", ott="spline")
    cmds.setAttr("robo_lowarm_pistonrod_02_anim.Slide", 0)
    
    cmds.setAttr("robo_lowarm_pistonrod_02_anim_grp.ry", -28)
buildRoboArmRigFK()


# ------------------------------------------------------------------
def buildRoboArmRigIK():
    import Modules.ART_rigUtils as utils
    reload(utils)
    import Tools.ART_addSpaceSwitching as space
    reload(space)

    # create an empty group for the Roboarm rig 
    RoboArmGrp = cmds.group(em=True, name="RoboArm_grp")

    # Create an IK arm control
    ik_robowrist_anim = utils.createControl("sphere", 15, "ik_robowrist_anim")
    constraint = cmds.parentConstraint("driver_robo_arm_07", ik_robowrist_anim)[0]
    cmds.delete(constraint)
    cmds.makeIdentity(ik_robowrist_anim, r = 1, s = 1, apply =True)
    cmds.setAttr(ik_robowrist_anim + ".sx", 1.8)
    cmds.setAttr(ik_robowrist_anim + ".sy", 1.8)
    cmds.setAttr(ik_robowrist_anim + ".sz", 0.3)
    cmds.makeIdentity(ik_robowrist_anim, r = 1, s = 1, apply =True)

    ik_robowrist_anim_grp = cmds.group(empty=True, name=ik_robowrist_anim + "_grp")
    constraint = cmds.pointConstraint("driver_robo_arm_07", ik_robowrist_anim_grp)[0]
    cmds.delete(constraint)

    ik_robowrist_animSpaceSwitchFollow = cmds.duplicate(ik_robowrist_anim_grp, po=True, n=ik_robowrist_anim+"_space_switcher_follow")[0]
    ik_robowrist_animSpaceSwitch = cmds.duplicate(ik_robowrist_anim_grp, po=True, n=ik_robowrist_anim+"_space_switcher")[0]

    cmds.parent(ik_robowrist_animSpaceSwitchFollow, RoboArmGrp)
    cmds.parent(ik_robowrist_animSpaceSwitch, ik_robowrist_animSpaceSwitchFollow)
    cmds.parent(ik_robowrist_anim_grp, ik_robowrist_animSpaceSwitch)

    cmds.parent(ik_robowrist_anim, ik_robowrist_anim_grp)
    cmds.makeIdentity(ik_robowrist_anim_grp, t=1, r=1, s=1, apply=True)

    # create a pole vector control for the elbow
    ik_roboelbow_anim = utils.createControl("sphere", 6, "ik_roboelbow_anim")
    constraint = cmds.parentConstraint("driver_robo_arm_03", ik_roboelbow_anim)[0]
    cmds.delete(constraint)
    cmds.makeIdentity(ik_roboelbow_anim, t=1, r=1, s=1, apply=True)
    cmds.setAttr(ik_roboelbow_anim + ".tz", -30)
    cmds.makeIdentity(ik_roboelbow_anim, t=1, r=1, s=1, apply=True)	    
    #cmds.parent(ik_roboelbowx_anim, RoboArmGrp)

    ik_roboelbow_anim_grp = cmds.group(empty=True, name=ik_roboelbow_anim + "_grp")
    constraint = cmds.pointConstraint(ik_roboelbow_anim, ik_roboelbow_anim_grp)[0]
    cmds.delete(constraint)

    ik_roboelbow_animSpaceSwitchFollow = cmds.duplicate(ik_roboelbow_anim_grp, po=True, n=ik_roboelbow_anim+"_space_switcher_follow")[0]
    ik_roboelbow_animSpaceSwitch = cmds.duplicate(ik_roboelbow_anim_grp, po=True, n=ik_roboelbow_anim+"_space_switcher")[0]

    cmds.parent(ik_roboelbow_animSpaceSwitchFollow, RoboArmGrp)
    cmds.parent(ik_roboelbow_animSpaceSwitch, ik_roboelbow_animSpaceSwitchFollow)
    cmds.parent(ik_roboelbow_anim_grp, ik_roboelbow_animSpaceSwitch)

    cmds.parent(ik_roboelbow_anim, ik_roboelbow_anim_grp)
    cmds.makeIdentity(ik_roboelbow_anim_grp, t=1, r=1, s=1, apply=True)

    # Duplicate the driver Robo Arm to make a new IK robo arm
    roboarm_01_joint = cmds.duplicate("driver_robo_arm_01", name="rig_robo_arm_01_ik", po=True)[0]
    cmds.parent(roboarm_01_joint, RoboArmGrp)
    roboarm_03_joint = cmds.duplicate("driver_robo_arm_03", name="rig_robo_arm_03_ik", po=True)[0]
    cmds.parent(roboarm_03_joint, roboarm_01_joint)
    roboarm_05_joint = cmds.duplicate("driver_robo_arm_05", name="rig_robo_arm_05_ik", po=True)[0]
    cmds.parent(roboarm_05_joint, roboarm_03_joint)
    roboarm_07_joint = cmds.duplicate("driver_robo_arm_07", name="rig_robo_arm_07_ik", po=True)[0]
    cmds.parent(roboarm_07_joint, roboarm_05_joint)

    # create an ikHandle for the arm and parent it under the IK arm control
    roboarmIKHandle = cmds.ikHandle(name="roboarmIKHandle", sol="ikRPsolver", sj=roboarm_01_joint, ee=roboarm_07_joint)[0]
    cmds.parent(roboarmIKHandle, ik_robowrist_anim)

    # create a pole vector constraint for the ikhandle to the pv control
    cmds.poleVectorConstraint(ik_roboelbow_anim, roboarmIKHandle)

    # Orient Constrain the last joint in the Roboarm IK chain to the IK control for hand control.
    cmds.orientConstraint(ik_robowrist_anim, roboarm_07_joint, mo=True)
    
    # Parent constrain the first joint of the IK chain to the driver skeleton.
    cmds.parentConstraint("driver_backpack_base", roboarm_01_joint, mo=True)

    # parent the roboarm group under the ctrl_rig node
    cmds.parent(RoboArmGrp, "ctrl_rig")

    # create new parent constraints with MO on for each of the original driver joints to be following the new ik joints
    for one in ["01", "03", "05", "07"]:
        cmds.parentConstraint("rig_robo_arm_"+one+"_ik", "driver_robo_arm_"+one, mo=True)
        
    Rig_SettingsConnection = cmds.listConnections("Rig_Settings.robo_arm_ik", p=True)
    for one in Rig_SettingsConnection:
        cmds.disconnectAttr("Rig_Settings.robo_arm_ik", one)

    cmds.connectAttr("Rig_Settings.robo_arm_ik", "driver_robo_arm_01_parentConstraint1.rig_robo_arm_01_ikW3")
    cmds.connectAttr("Rig_Settings.robo_arm_ik", "driver_robo_arm_03_parentConstraint1.rig_robo_arm_03_ikW3")
    cmds.connectAttr("Rig_Settings.robo_arm_ik", "driver_robo_arm_05_parentConstraint1.rig_robo_arm_05_ikW3")
    cmds.connectAttr("Rig_Settings.robo_arm_ik", "driver_robo_arm_07_parentConstraint1.rig_robo_arm_07_ikW3")
    cmds.connectAttr("Rig_Settings.robo_arm_ik", "RoboArm_grp.visibility")

    space.addSpaceSwitching(ik_robowrist_anim, "master_anim", False)
    space.addSpaceSwitching(ik_robowrist_anim, "body_anim", False)
    space.addSpaceSwitching(ik_robowrist_anim, "backpack_base_anim", False)

    space.addSpaceSwitching(ik_roboelbow_anim, "master_anim", False)
    space.addSpaceSwitching(ik_roboelbow_anim, "body_anim", False)
    space.addSpaceSwitching(ik_roboelbow_anim, "backpack_base_anim", False)

    cmds.setAttr("Rig_Settings.robo_arm_ik", 1)
    cmds.setAttr("Rig_Settings.robo_arm_fk", 0)

buildRoboArmRigIK()

# ------------------------------------------------------------------
def buildBackpackRig():
    # Move the spring into position.
    cmds.setAttr("spring_anim.ty", 0.058)
    cmds.setAttr("spring_anim.tz", -28.7)
    cmds.setAttr("spring_anim.sx", 0.749)
    cmds.setAttr("spring_anim.sy", 0.749)
    cmds.setAttr("spring_anim.sz", 0.081)
    
    # Move the canister into position
    cmds.setAttr("canister_main_anim.ty", 1)
    cmds.setAttr("canister_main_anim.tz", -49.4)
    cmds.setAttr("canister_main_anim.sx", 0.585)
    cmds.setAttr("canister_main_anim.sy", 0.585)
    cmds.setAttr("canister_main_anim.sz", 0.314)
    
    # Rig the gas cans to be staying between the pelvis and the backpack base.
    gascan_02_const = cmds.parentConstraint("driver_pelvis", "backpack_gas_02_parent_grp", mo=True)[0]
    cmds.setAttr(gascan_02_const+".interpType", 2)
    gascan_01_const = cmds.parentConstraint("driver_backpack_gas_02", "backpack_gas_01_parent_grp", mo=True)[0]
    cmds.setAttr(gascan_01_const+".interpType", 2)

buildBackpackRig()

# ------------------------------------------------------------------
def dynamicsOff():
    for one in ["thigh_pouch_big_anim", "thigh_pouch_sml_anim", "helmet_straps_l_dyn_anim", "helmet_straps_r_dyn_anim"]:
        cmds.setAttr(one+".chainStartEnvelope", 0)
        cmds.setAttr(one+".chainStartEnvelope", l=True)
dynamicsOff()

# ------------------------------------------------------------------
def helmetStraps():
    const = cmds.parentConstraint("driver_C_jaw", "helmet_straps_l_ik_base_anim_grp", mo=True)[0]
    cmds.setAttr(const+".interpType", 2)
    const = cmds.parentConstraint("driver_C_jaw", "helmet_straps_r_ik_base_anim_grp", mo=True)[0]
    cmds.setAttr(const+".interpType", 2)
    const = cmds.parentConstraint("driver_C_jaw", "fk_helmet_straps_l_01_grp", mo=True)[0]
    cmds.setAttr(const+".interpType", 2)
    const = cmds.parentConstraint("driver_C_jaw", "fk_helmet_straps_r_01_grp", mo=True)[0]
    cmds.setAttr(const+".interpType", 2)
helmetStraps()


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
addCapsule(42, 85)

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