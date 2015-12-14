import maya.mel as mel
import maya.cmds as cmds
import Tools.ART_CreateHydraulicRigs as hyd
reload(hyd)
import Tools.ART_addSpaceSwitching as space
reload(space)

# ------------------------------------------------------------------
# add ik bones
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
def scaleControls():
    objects = ["dispencer_base_anim", "dispencer_clip_bl_anim", "dispencer_clip_br_anim", "dispencer_clip_fl_anim", "dispencer_clip_fr_anim", "dispencer_door_bott_bk_anim", "dispencer_door_bott_fr_anim", "dispencer_door_mid_bk_anim", "dispencer_door_mid_fr_anim", "dispencer_door_top_bk_anim",  "dispencer_door_top_fr_anim", "dispencer_top_anim", "dispencer_handle_anim" , "dispencer_plug_anim" , "toe_big_a_l_anim", "toe_mid_a_l_anim", "toe_pink_a_l_anim", "toe_big_a_r_anim", "toe_mid_a_r_anim", "toe_pink_a_r_anim", "heel_l_anim", "heel_r_anim", "back_pack_anim", "bottle_01_anim", "bottle_02_anim", "bottle_sack_anim", "bottle_throw_anim", "throttle_pivot_anim", "muscle_thigh_l_anim", "muscle_thigh_r_anim", "muscle_belly_anim", "muscle_tail_bott_anim", "muscle_jowles_anim", "gums_top_anim", "gums_bottom_anim"]
    for one in objects:
        cmds.select([one+".cv[0:*]"])
        cmds.scale(0.2, 0.2, 0.2, r=True, os=True)

    for side in ["_l", "_r"]:
        cmds.select(["eyeball"+side+"_anim.cv[0:*]"])
        cmds.scale(0.1, 0.1, 0.05, r=True, ocp = False)
        if side == "_l":
            cmds.move(0, 0, 3.757386, r=True, os=True)
        else:
            cmds.move(0, 0, 3.757386, r=True, os=True)

        '''cmds.select(["eyelid_bott"+side+"_anim.cv[0:*]"])
        cmds.scale(0.1, 0.1, 0.01, r=True, ocp = False)
        if side == "_l":
            cmds.move(0, -3.757386, 0.5, r=True, os=True)
        else:
            cmds.move(0, 3.757386, 0.5, r=True, os=True)

        cmds.select(["eyelid_top"+side+"_anim.cv[0:*]"])
        cmds.scale(0.1, 0.1, 0.01, r=True, ocp = False)
        if side == "_l":
            cmds.move(0, -3.757386, 0.5, r=True, os=True)
        else:
            cmds.move(0, 3.757386, 0.5, r=True, os=True)'''

    #ik feet
    cmds.select("ik_foot_anim_l.cv[0:*]")
    cmds.scale(2.8, 1.88, 1, p=[17.904671, -8.630238, 2.758035], r=True)
    cmds.move(0, 0, -2.758035, r = True, os=True, wd=True)
    
    cmds.select("ik_foot_anim_r.cv[0:*]")
    cmds.scale(2.8, 1.88, 1, p=[-17.904671, -8.630238, 2.758035], r=True)
    cmds.move(0, 0, -2.758035, r = True, os=True, wd=True)

    # Other
    cmds.select(["jaw_anim.cv[0:*]"])
    cmds.scale(0.2, 0.2, 0.2, r=True, ocp = True)
    cmds.move(0, -24, 80, r=True, os=True)

    cmds.select(["body_anim.cv[0:*]"])
    cmds.scale(3, 3, 3, r=True, os=True)
    cmds.rotate(0, 0, -90, r=True, os=True)
    
    cmds.select(["master_anim.cv[0:*]"])
    cmds.scale(2, 2, 2, r=True, os=True)

    cmds.select(["offset_anim.cv[0:*]"])
    cmds.scale(2, 2, 2, r=True, os=True)

    bodyObjs90 = ["hip_anim", "mid_ik_anim", "chest_ik_anim", "head_fk_anim"]
    for one in bodyObjs90:
        cmds.select([one+".cv[0:*]"])
        cmds.scale(3, 3, 3, r = True)
        cmds.rotate(0, 0, -90, r=True, os=True)

    bodyObjs = ["fk_tail_01_anim", "fk_tail_02_anim", "fk_tail_03_anim", "fk_tail_04_anim",  "spine_01_anim", "spine_02_anim", "spine_03_anim", "fk_thigh_l_anim" , "fk_thigh_r_anim", "fk_foot_l_anim", "fk_foot_r_anim", "neck_01_fk_anim"]
    for one in bodyObjs:
        cmds.select([one+".cv[0:*]"])
        cmds.scale(4, 4, 4, r = True)

    for anim in ["fk_tail_01_anim", "fk_tail_02_anim", "fk_tail_03_anim", "fk_tail_04_anim"]:
        cmds.select(anim + ".cv[*]")
        cmds.scale(1.7,1.7,1.7, r = True)

    cmds.select("bottle_throw_anim.cv[*]")
    cmds.move(0, 0, 0, r = True, os = True, ws = True)
    cmds.scale(1.8, 1.8, 1.8, r = True)

    cmds.select("spine_03_anim.cv[*]")
    cmds.scale(1.5, 1.5, 1.5, r = True)

    cmds.select("head_fk_anim.cv[*]")
    cmds.scale(2.2, 2.2, 2.2, r = True)
    cmds.rotate(0,0,33, r = True)

    cmds.select("throttle_pivot_anim.cv[*]")
    cmds.scale(9,9,0, r = True)
    cmds.rotate(-20,0,0, r = True)

    cmds.setAttr("thigh_twist_l_Tracker.v", 0)
    cmds.setAttr("thigh_twist_r_Tracker.v", 0)

scaleControls()

# ------------------------------------------------------------------
# Create the dog legs for the Dino
def dogLegRig():
    mel.eval("ikSpringSolver")
    for side in ["_l", "_r"]:
        ###########################
        # DELETE old IK handle
        cmds.delete("foot_ikHandle"+side)
        
        # Disconnect the attrs driving the foot joint so that we can re-drive it with a parent constraint.
        attrToDisconnect = "foot"+side+".translate"
        connection = cmds.connectionInfo(attrToDisconnect, sfd=True)
        cmds.disconnectAttr(connection, attrToDisconnect)
        cmds.delete("foot"+side+"_orientConstraint1")
        
        # PARENT driver_foot_l to driver_heel_l
        cmds.parent("driver_foot"+side, "driver_heel"+side)
        
        # Parent constrain the deform foot to follow the driver foot
        cmds.parentConstraint("driver_foot"+side, "foot"+side, mo=True)
    
        # DUPLICATE driver_heel_l and rename to ik_leg_heel_l
        ik_leg_heel = cmds.duplicate("driver_heel"+side, n="ik_leg_heel"+side, po=True)[0]
        # PARENT ik_leg_heel_l to ik_leg_calf_l
        cmds.parent(ik_leg_heel, "ik_leg_calf"+side)
        # PARENT ik_leg_foot_l to ik_leg_heel_l(dupe)
        cmds.parent("ik_leg_foot"+side, ik_leg_heel)
        cmds.parent("ik_leg_thigh"+side, w=True)
        cmds.makeIdentity("ik_leg_thigh"+side, a=True, r=True)
        cmds.parentConstraint("leg_joints_grp"+side, "ik_leg_thigh"+side, mo=True)
        cmds.parent("ik_leg_thigh"+side, "rig_grp")
        
        # DUPLICATE driver_heel_l and rename to fk_leg_heel_l
        fk_leg_heel = cmds.duplicate("driver_heel"+side, n="fk_leg_heel"+side, po=True)[0]
        # PARENT fk_leg_heel_l to fk_leg_calf_l
        cmds.parent(fk_leg_heel, "fk_leg_calf"+side)
        # PARENT fk_leg_foot_l to fk_leg_heel_l(dupe)
        cmds.parent("fk_leg_foot"+side, fk_leg_heel)
        # ORIENT CONSTRAIN fk_leg_foot_l to heel_l_anim
        cmds.orientConstraint("heel"+side+"_anim", "fk_leg_heel"+side, mo=True)
        cmds.parentConstraint("heel"+side+"_anim", "fk_foot"+side+"_anim_grp", mo=True)

        #DUPLICATE driver_heel_l and rename to result_leg_heel_l
        result_leg_heel = cmds.duplicate("driver_heel"+side, n="result_leg_heel"+side, po=True)[0]
        # PARENT result_leg_heel_l to result_leg_calf_l
        cmds.parent(result_leg_heel, "result_leg_calf"+side)
        # PARENT result_leg_foot_l to result_leg_heel_l(dupe)
        cmds.parent("result_leg_foot"+side, result_leg_heel)
        
        #LOAD ikSpringSolver;
        # CREATE spring IK from ik_leg_thigh_l to ik_leg_foot_l
        foot_ikhandle = cmds.ikHandle(n="foot_ikHandle"+side, sol="ikSpringSolver", sj="ik_leg_thigh"+side, ee="ik_leg_foot"+side)[0]
        cmds.setAttr(foot_ikhandle+".v", 0)
        
        #PARENT ik handle to ik_foot_ball_pivot_l
        cmds.parent(foot_ikhandle, "ik_foot_ball_pivot"+side)

        # CREATE PV consraint on ik handle to noflip_pv_loc_l
        PVConst = cmds.poleVectorConstraint("noflip_pv_loc"+side, foot_ikhandle)[0]
        if side == "_l":
            cmds.setAttr(PVConst+".offsetX", 200)
        else:
            cmds.setAttr(PVConst+".offsetX", -200)

        # CONNECT the knee twist attr on ik_foot_anim_l to the twist attr on new ik handle
        cmds.connectAttr("ik_foot_anim"+side+".knee_twist", foot_ikhandle+".twist", f=True)
        # DELETE driver_heel_l_parentConstraint1
        cmds.delete("driver_heel"+side+"_parentConstraint1")
        cmds.parentConstraint(result_leg_heel, "driver_heel"+side, mo=True)
        
        # CREATE a parent constraint using ik_leg_heel_l, fk_leg_heel_l and drive result_leg_heel_l
        resultConstraint = cmds.parentConstraint(fk_leg_heel, ik_leg_heel, result_leg_heel, mo=True)[0]
        # CREATE a reverse node and hook up a IK/FK switch between the Rig_Settings.lLegMode and the 2 weights on this new constraint
        shortside = side.replace("_", "")
        reverseNode = cmds.shadingNode("reverse", asUtility=True, name=result_leg_heel+"_reverse")
        cmds.connectAttr("Rig_Settings."+shortside+"LegMode", resultConstraint+"."+ik_leg_heel+"W1", f=True)
        cmds.connectAttr("Rig_Settings."+shortside+"LegMode", reverseNode+".inputX", f=True)
        cmds.connectAttr(reverseNode+".outputX", resultConstraint+"."+fk_leg_heel+"W0", f=True)

        # Create a KneeFlex Attribute
        cmds.addAttr("ik_foot_anim"+side, ln="KneeFlex", at="double", min=0, max=1, dv=0.5, k=True)
        # Connect KneeFlex to :
        reverseNodeBias = cmds.shadingNode("reverse", asUtility=True, name=result_leg_heel+"_reverse")
        cmds.connectAttr("ik_foot_anim"+side+".KneeFlex", foot_ikhandle+".springAngleBias[0].springAngleBias_FloatValue", f=True)
        cmds.connectAttr("ik_foot_anim"+side+".KneeFlex", reverseNodeBias+".inputX", f=True)
        cmds.connectAttr(reverseNodeBias+".outputX", foot_ikhandle+".springAngleBias[1].springAngleBias_FloatValue", f=True)
        
        visReverseNode = cmds.shadingNode("reverse", asUtility=True, name="heel"+side+"_anim_reverse")
        cmds.connectAttr("Rig_Settings."+shortside+"LegMode", visReverseNode+".inputX", f=True)
        cmds.connectAttr(visReverseNode+".outputX", "heel"+side+"_parent_grp.v", f=True)

dogLegRig()

# ------------------------------------------------------------------
def dispencerRig():
    # Next 2 lines are just for finding the location to use in the 3rd line.
    #cmds.xform("dispencer_base_anim", q=True, t=True)
    #cmds.xform("dispencer_base_anim", q=True, ro=True)
    cmds.xform("dispencer_base_anim_grp", t=[27.842794846757382, 83.38044772340979, -30.319686119574822], ro=[19.709222794231103, -0.026517130554039203, 0.18267468754167737])
dispencerRig()

# ------------------------------------------------------------------
def invertEyelids():
    sides = ["_l", "_r"]
    for side in sides:
        cmds.delete("driver_eyelid_bott"+side+"_parentConstraint1")
        cmds.rotate(0, -180, 0, "eyelid_bott"+side+"_anim_grp", r=True, os=True)
        cmds.parentConstraint("eyelid_bott"+side+"_anim", "driver_eyelid_bott"+side, mo=True)
#invertEyelids()

def invertRightEyeball():
    cmds.delete("driver_eyeball_r_parentConstraint1")
    cmds.rotate(0, -180, -180, "eyeball_r_anim_grp", r=True, os=True)
    cmds.parentConstraint("eyeball_r_anim", "driver_eyeball_r", mo=True)
invertRightEyeball()

# ------------------------------------------------------------------
def fixToes():
    for one in ["driver_toe_mid_a_l_parentConstraint1", "driver_toe_mid_a_r_parentConstraint1", "driver_toe_pink_a_l_parentConstraint1", "driver_toe_pink_a_r_parentConstraint1", "driver_toe_big_a_l_parentConstraint1", "driver_toe_big_a_r_parentConstraint1"]:
        cmds.setAttr(one+".interpType", 2)
fixToes()

# ------------------------------------------------------------------
def dynamicsOff():
    for one in ["tail_dyn_anim", "tail_armor_dyn_anim", "tongue_dyn_anim"]:
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
addCapsule(72, 108)
    
# ------------------------------------------------------------------
#The destination attribute 'Goblin:ik_foot_anim_l_space_switcher_follow_parentConstraint1.W2' cannot be found.
# Import in and hook up the Goblin for Pyro
def hookUpGoblin():
    import Tools.ART_addSpaceSwitching as space
    reload(space)
    
    # import in Goblin
    cmds.file("D:/Build/usr/jeremy_ernst/MayaTools/General/ART/Projects/Orion/AnimRigs/Pyro_Goblin.mb", i=True, options='v=0', type="mayaBinary", namespace="Goblin", mergeNamespacesOnClash=False)

    cmds.setAttr("Goblin:Rig_Settings.spine_fk", 1)
    cmds.setAttr("Goblin:Rig_Settings.spine_ik", 0)

    # set elbows to body space
    cmds.setAttr("Goblin:ik_elbow_l_anim_space_switcher.space_body_anim", 1)
    cmds.setAttr("Goblin:ik_elbow_r_anim_space_switcher.space_body_anim", 1)

    cmds.setAttr("Goblin:ik_wrist_l_anim_space_switcher.space_body_anim", 1)
    cmds.setAttr("Goblin:ik_wrist_r_anim_space_switcher.space_body_anim", 1)

    cmds.setAttr("Goblin:ik_foot_anim_l_space_switcher.space_body_anim", 1)
    cmds.setAttr("Goblin:ik_foot_anim_r_space_switcher.space_body_anim", 1)

    # turn on auto clavicles
    #cmds.setAttr("Goblin:clavicle_l_anim.autoShoulders", 1)
    #cmds.setAttr("Goblin:clavicle_r_anim.autoShoulders", 1)

    # set head to world space
    cmds.setAttr("Goblin:head_fk_anim.fkOrientation", 2)

    # position Goblin
    cmds.setAttr("Goblin:body_anim.tx", 75.168)

    cmds.setAttr("Goblin:ik_foot_anim_l.tx", 17.359)
    cmds.setAttr("Goblin:ik_foot_anim_l.ty", -16.157)
    cmds.setAttr("Goblin:ik_foot_anim_l.tz", 10.935)
    cmds.setAttr("Goblin:ik_foot_anim_l.rx", -52.374)
    cmds.setAttr("Goblin:ik_foot_anim_l.ry", 6.81)
    cmds.setAttr("Goblin:ik_foot_anim_l.rz", 45.2873)
    cmds.setAttr("Goblin:ik_foot_anim_l.knee_twist", 47.2)

    cmds.setAttr("Goblin:ik_foot_anim_r.tx", -17.359)
    cmds.setAttr("Goblin:ik_foot_anim_r.ty", -16.157)
    cmds.setAttr("Goblin:ik_foot_anim_r.tz", 10.935)
    cmds.setAttr("Goblin:ik_foot_anim_r.rx", -52.374)
    cmds.setAttr("Goblin:ik_foot_anim_r.ry", -6.81)
    cmds.setAttr("Goblin:ik_foot_anim_r.rz", -45.2873)
    cmds.setAttr("Goblin:ik_foot_anim_r.knee_twist", 47.2)

    # connect Goblin to the seat
    space.addSpaceSwitching("Goblin:body_anim", "pelvis", True)
    space.addSpaceSwitching("Goblin:body_anim", "spine_01", False)

    #cmds.setAttr("hip_anim.autoHips", 1)

    # expose knee locs for Goblin and make knees follow body
    cmds.select(["Goblin:noflip_aim_grp_l", "Goblin:noflip_aim_grp_r"])
    cmds.delete(constraints = True)

    cmds.parentConstraint("body_anim", "Goblin:noflip_aim_grp_l", mo = True)
    cmds.parentConstraint("body_anim", "Goblin:noflip_aim_grp_r", mo = True)

    # Space switching for the hands to the throttle
    space.addSpaceSwitching("Goblin:ik_wrist_l_anim", "throttle_pivot_anim", False)
    space.addSpaceSwitching("Goblin:ik_wrist_r_anim", "throttle_pivot_anim", False)

    cmds.setAttr("ik_wrist_l_anim_space_switcher.space_body_anim", 1)
    cmds.setAttr("ik_wrist_r_anim_space_switcher.space_body_anim", 1)

    # join skeletons
    cmds.select("Goblin:root", hi = True)
    joints = cmds.ls(sl = True)
    for joint in joints:
        newName = joint.partition(":")[2]
        cmds.rename(joint, "Goblin_" + newName)
    cmds.parent("Goblin_root", "root")    

    # Disconnect the attrs driving the Goblin pelvis joint so that we can re-drive it with a parent constraint.
    attrToDisconnect = "Goblin_pelvis.translate"
    connection = cmds.connectionInfo(attrToDisconnect, sfd=True)
    cmds.disconnectAttr(connection, attrToDisconnect)
    cmds.delete("Goblin_pelvis_orientConstraint1")

    # Parent constrain the deform pelvis to follow the driver pelvis
    cmds.parentConstraint("Goblin:driver_pelvis", "Goblin_pelvis", mo=True)

    cmds.select(None)
hookUpGoblin()

# ------------------------------------------------------------------
# This needs to be added after the Goblin has been brought in and hooked up.
def bottlesRig():
    import Tools.ART_addSpaceSwitching as space
    reload(space)

    space.addSpaceSwitching("bottle_throw_anim", "Goblin_hand_r", False)
    space.addSpaceSwitching("bottle_01_anim", "Goblin_hand_r", False)
    space.addSpaceSwitching("bottle_02_anim", "Goblin_hand_r", False)
bottlesRig()


# ------------------------------------------------------------------
# save the AnimRig file
cmds.file(save = True, type = "mayaBinary", force = True)

# add IK Bones here so that they are included in the export file.  But dont save becasue we dont want them in the rig file.  Thats why we saved in the line before.
addIKBones()

# ------------------------------------------------------------------
# Creeate the export file that goes into source so it can be exported to the game
def createExportFile():
    # Import the export file from the reference so that it can be edited.
    cmds.file("D:/Build/usr/jeremy_ernst/MayaTools/General/ART/Projects/Orion/ExportFiles/Pyro_Export.mb", importReference=True)

    # select the entire skeleton and make a list of it.
    cmds.select("root", hi=True)
    selection = cmds.ls(sl=True)
    cmds.showHidden(a=True)

    # for each object in the selection, disconnect translation and scale.
    for each in selection:
        attrs = ["translate", "scale"]
        cmds.lockNode(each, lock=False)

        for attr in attrs:
            try:
                conn = cmds.listConnections(each + "." + attr, s=True, p=True)[0]
                cmds.disconnectAttr(conn, each + "." + attr)
            except:
                pass

    # find all of the constraints in the skeleton and delete them
    cmds.select("root", hi=True)
    constraints = cmds.ls(sl=True, type="constraint")
    cmds.select(constraints)
    cmds.delete(constraints)

    # delete the control rig so that it severs any connections between it and the geo or skeleton.
    cmds.select("Goblin:rig_grp", "rig_grp", hi=True)
    rigs = cmds.ls(sl=True)
    for one in rigs:
        cmds.lockNode(one, lock=False)
    cmds.delete("Goblin:rig_grp", "rig_grp")

    # Remove the Goblin namespace and set the bindposes so that Creating LODs will work later on.
    cmds.namespace(rm="Goblin", mnp=True)
    cmds.dagPose("root", bp=True, r=True) 
    cmds.dagPose("Goblin_root", bp=True, r=True)
    
    # select the skeleton and all of the geo and export it to the export file.  Remember to go export an fbx from this.
    cmds.select("Goblin:Pyro_Goblin_Geo", "Pyro_Dino_Geo", "root", hi=True)
    cmds.file("D:/Build/ArtSource/Orion/Characters/Heroes/Pyro/Export/Pyro_Export.mb", force=True, typ="mayaBinary", pr=True, es=True)
createExportFile()

# ------------------------------------------------------------------
# ADD IK BONES to the export file
sceneName = cmds.file(q = True, sceneName = True)
exportFile = sceneName.partition("AnimRigs")[0] + "ExportFiles" +  sceneName.partition("AnimRigs")[2].partition(".mb")[0] + "_Export.mb"
cmds.file(exportFile, open = True, force = True)


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

# save the export file
cmds.file(save = True, type = "mayaBinary", force = True)


