import maya.cmds as cmds
import Tools.ART_addSpaceSwitching as space
reload(space)

#NOTE This rig has custom changes to be made during the RigPose placement phase.  See the commented out script at the bottom of this file.


# ------------------------------------------------------------------
# scale all of the controls to be more visible with the mesh.
def scaleControls():
    objects = ["bot_uparm_pistonC_top_anim", "bot_uparm_pistonC_bot_anim", "bot_loarm_pistonB_top_anim", "bot_loarm_pistonB_bot_anim", "bot_loarm_pistonC_top_anim", "bot_loarm_pistonC_bot_anim", "bot_loarm_pistonD_top_anim", "bot_loarm_pistonD_bot_anim", "bot_loarm_anim", "bot_loarm_twist_anim", "bot_hand_anim", "gun_scope_anim", "gun_stock_base_anim", "gun_stock_bott_anim", "gun_barrel_inner_anim", "gun_body_panel_l_anim", "gun_body_panel_r_anim", "gun_stock_base_in_bott_anim", "gun_stock_base_out_bott_anim", "gun_stock_base_in_top_anim", "gun_stock_base_out_top_anim", "gun_stock_in_bott_anim", "gun_stock_in_top_anim", "trap_handle_anim", "jaw_anim", "sniper_can_arm_01_anim"]
    for one in objects:
        cmds.select([one+".cv[0:32]"])
        cmds.scale(0.227057, 0.227057, 0.227057, r = True, ocp = True)
        cmds.move(0, -5, 0, r = True, os=True, wd=True)
        
    objects = ["bot_uparm_anim", "gun_anim", "gun_stock_panel_r_anim", "gun_stock_panel_l_anim", "gun_barrel_anim", "shotgun_base_anim", "shotgun_clip_anim", "shotgun_anim", "ankle_plate_in_l_anim", "ankle_plate_in_r_anim",  "ankle_plate_out_l_anim", "ankle_plate_out_r_anim", "sniper_can_arm_02_anim", "sniper_can_arm_03_anim", "arm_hose_bott_anim", "arm_hose_top_anim", "mask_anim", "helmet_sight_anim", "arm_pouch_l_anim", "pack_hose_out_anim", "pack_hose_in_anim", "heel_plate_l_anim", "heel_plate_r_anim", "wrist_cover_anim"]
    for one in objects:
        cmds.select([one+".cv[0:32]"])
        cmds.scale(0.2, 0.2, 0.2, r = True, ocp = True)
        
    for side in ["_l", "_r"]:
        #ik feet
        cmds.select("ik_foot_anim"+side + ".cv[0:32]")
        cmds.scale(1, 1, 1, r=True)
        cmds.move(0, 0, -4.695816, r = True, os=True, wd=True)
    
        cmds.select(["eyebrow_in"+side+"_anim.cv[0:32]"])
        cmds.scale(0.117744, 0.117744, 0.117744, r=True, ocp = False)
        cmds.move(0, 0, 0, r=True)

        cmds.select(["eyebrow_out"+side+"_anim.cv[0:32]"])
        cmds.scale(0.117744, 0.117744, 0.117744, r=True, ocp = False)
        cmds.move(0, 0, 0, r=True)

        cmds.select(["eyeball"+side+"_anim.cv[0:32]"])
        cmds.scale(0.1, 0.1, 0.05, r=True, ocp = False)
        if side == "_l":
            cmds.move(0, -3.757386, 0, r=True, os=True)
        else:
            cmds.move(0, -3.757386, 0, r=True, os=True)

        cmds.select(["eyelid_bott"+side+"_anim.cv[0:32]"])
        cmds.scale(0.1, 0.1, 0.01, r=True, ocp = False)
        if side == "_l":
            cmds.move(0, -3.757386, 0.5, r=True, os=True)
        else:
            cmds.move(0, 3.757386, 0.5, r=True, os=True)

        cmds.select(["eyelid_top"+side+"_anim.cv[0:32]"])
        cmds.scale(0.1, 0.1, 0.01, r=True, ocp = False)
        if side == "_l":
            cmds.move(0, -3.757386, 0.5, r=True, os=True)
        else:
            cmds.move(0, 3.757386, 0.5, r=True, os=True)
            
    cmds.select(["gun_anim.cv[0:32]"])
    cmds.move(8, 0, 0, r = True, os=True, wd=True)

    cmds.select(["gun_scope_anim.cv[0:32]"])
    cmds.move(0, 10, 0, r = True, os=True, wd=True)

    cmds.select(["gun_stock_panel_r_anim.cv[0:32]"])
    cmds.move(2.5, 0, 0, r = True, os=True, wd=True)

    cmds.select(["gun_stock_panel_l_anim.cv[0:32]"])
    cmds.move(-2.5, 0, 0, r = True, os=True, wd=True)

    cmds.select(["gun_stock_bott_anim.cv[0:32]"])
    cmds.move(0, -1, 0, r = True, os=True, wd=True)

    cmds.select(["gun_barrel_anim.cv[0:32]"])
    cmds.move(0, 0, -5, r = True, os=True, wd=True)

    cmds.select(["gun_stock_base_out_top_anim.cv[0:32]"])
    cmds.move(0, 10, 0, r = True, os=True, wd=True)

    cmds.select(["gun_stock_base_in_top_anim.cv[0:32]"])
    cmds.move(0, 10, 0, r = True, os=True, wd=True)

    cmds.select(["gun_stock_in_top_anim.cv[0:32]"])
    cmds.move(0, 15, 0, r = True, os=True, wd=True)

    cmds.select(["bot_hand_anim.cv[0:32]"])
    cmds.move(0, 0, 6, r = True, os=True, wd=True)

    cmds.select(["bot_loarm_anim.cv[0:32]"])
    cmds.move(0, 10, 0, r = True, os=True, wd=True)

    cmds.select(["bot_uparm_anim.cv[0:32]"])
    cmds.move(0, 5, -8, r = True, os=True, wd=True)

    cmds.select(["shotgun_base_anim.cv[0:32]"])
    cmds.move(7, 0, 0, r = True, os=True, wd=True)

    cmds.select(["shotgun_anim.cv[0:32]"])
    cmds.move(0, 0, 3, r = True, os=True, wd=True)

    cmds.select(["shotgun_clip_anim.cv[0:32]"])
    cmds.move(-5, 0, 0, r = True, os=True, wd=True)

    cmds.select(["jaw_anim.cv[0:32]"])
    cmds.move(0, -5, -8, r = True, os=True, wd=True)

    cmds.select(["sniper_can_arm_01_anim.cv[0:32]"])
    cmds.move(0, 7, 9, r = True, os=True, wd=True)

    cmds.select(["sniper_can_arm_02_anim.cv[0:32]"])
    cmds.move(0, 5, 5, r = True, os=True, wd=True)

    cmds.select(["sniper_can_arm_03_anim.cv[0:32]"])
    cmds.scale(1.0, 6.3, 6.3, r = True, ocp = True)
    cmds.move(0, 5, 0, r = True, os=True, wd=True)

    cmds.select(None)
scaleControls()

# ------------------------------------------------------------------
def createPistons():
    import Tools.ART_CreateHydraulicRigs as hyd
    reload(hyd)
    
    # pistons
    cmds.select("bot_uparm_pistonC_top_anim", "bot_uparm_pistonC_bot_anim", "driver_upperarm_r")
    hyd.CreateHydraulicRig(-1)
    cmds.select("bot_loarm_pistonB_top_anim", "bot_loarm_pistonB_bot_anim", "driver_lowerarm_r")
    hyd.CreateHydraulicRig(-1)
    cmds.select("bot_loarm_pistonC_top_anim", "bot_loarm_pistonC_bot_anim", "driver_lowerarm_r")
    hyd.CreateHydraulicRig(-1)
    cmds.select("bot_loarm_pistonD_top_anim", "bot_loarm_pistonD_bot_anim", "driver_lowerarm_r")
    hyd.CreateHydraulicRig(-1)

    # manually changing the up object for a few of the constraints that were created above so that they rotate at teh correct angles.
    cmds.aimConstraint("bot_loarm_pistonB_bot_anim_sub_aimConstraint1", e=True, mo=True, wuo="driver_bot_hand")
    cmds.aimConstraint("bot_loarm_pistonC_bot_anim_sub_aimConstraint1", e=True, mo=True, u=(0,-1,0), wuo="bot_loarm_pistonD_bot_anim")
    cmds.aimConstraint("bot_loarm_pistonD_bot_anim_sub_aimConstraint1", e=True, mo=True, u=(0,-1,0), wuo="bot_loarm_pistonC_bot_anim")
    cmds.aimConstraint("bot_uparm_pistonC_bot_anim_sub_aimConstraint1", e=True, mo=True, wuo="driver_lowerarm_r")

    # bot arm twist
    cmds.orientConstraint("driver_lowerarm_twist_01_r", "bot_loarm_twist_anim_space_switcher", mo = True)
createPistons()

# ------------------------------------------------------------------
def anklePlatesRig():
    for side in ["_l", "_r"]:
        sidegrp = cmds.group(em=True, n="ankle_plates_grp"+side)
        const = cmds.parentConstraint("driver_calf_twist_01"+side, sidegrp)[0]
        cmds.delete(const)
        cmds.makeIdentity(sidegrp, a=True, t=True, r=True, s=True)
        cmds.parent(sidegrp, "ctrl_rig")
        cmds.parentConstraint("driver_calf_twist_01"+side, sidegrp, mo=True)
        
        for dir in ["_in", "_out"]:
            target = cmds.spaceLocator(n="ankle_plate"+dir+"_target"+side)[0]
            const = cmds.parentConstraint("ankle_plate"+dir+side+"_anim", target)[0]
            cmds.delete(const)
            if side == "_l":
                cmds.move(-10, 0, 0, target, r=True, os=True, wd=True)
            else:
                cmds.move(10, 0, 0, target, r=True, os=True, wd=True)
            
            up = cmds.spaceLocator(n="ankle_plate"+dir+"_up"+side)[0]
            const = cmds.parentConstraint("ankle_plate"+dir+side+"_anim", up)[0]
            cmds.delete(const)
            if side == "_l":
                cmds.move(0, 0, -5, up, r=True, os=True, wd=True)
            else:
                cmds.move(0, 0, 5, up, r=True, os=True, wd=True)

            
            grp = cmds.group(em=True, n="ankle_plate"+dir+"_grp"+side)
            const = cmds.parentConstraint("driver_foot"+side, grp)[0]
            cmds.delete(const)
            cmds.makeIdentity(grp, a=True, t=True, r=True, s=True)
            
            cmds.parent(target, up, grp)
            cmds.parent(grp, sidegrp)
            
            driver_foot_loc = cmds.xform("driver_foot"+side, q=True, ws=True, rp=True)
            cmds.xform("ankle_plate"+dir+side+"_anim_grp", ws=True, rp=driver_foot_loc)

            cmds.parentConstraint("driver_foot"+side, grp, mo=True, sr=["x"])
          
            cmds.aimConstraint(target, "ankle_plate"+dir+side+"_anim_grp", mo=True, aimVector=(-1, 0, 0), upVector=(0, 0, -1), worldUpType="object", worldUpVector=(0, 0, -1), worldUpObject=str(up))
            
  
            cmds.parentConstraint("driver_foot"+side, "ankle_plate"+dir+side+"_anim_grp", mo=True, sr=["x", "y", "z"])
            
            cmds.setAttr("ankle_plate"+dir+"_grp"+side+".rotateOrder", 4)
            
            if side == "_r":
                cmds.setAttr("ankle_plate"+dir+"_grp"+side+"_parentConstraint1.target[0].targetOffsetRotateX", -180)
                cmds.setAttr("ankle_plate"+dir+"_grp"+side+"_parentConstraint1.target[0].targetOffsetRotateY", -90)
            
            cmds.setAttr(grp+".v", 0)
            
anklePlatesRig()

# ------------------------------------------------------------------
def heelRig():
    for side in ["_l", "_r"]:
        cmds.setDrivenKeyframe(["heel_plate"+side+"_anim_grp.rx"], cd = "driver_foot"+side+".rz", itt = "spline", ott = "spline")
        cmds.setAttr("ik_foot_anim"+side+".rx", 40)
        cmds.setAttr("heel_plate"+side+"_anim_grp.rx", -17.471)
        cmds.setDrivenKeyframe(["heel_plate"+side+"_anim_grp.rx"], cd = "driver_foot"+side+".rz", itt = "spline", ott = "spline")
        cmds.setAttr("ik_foot_anim"+side+".rx", 80)
        cmds.setAttr("heel_plate"+side+"_anim_grp.rx", -59.01)
        cmds.setDrivenKeyframe(["heel_plate"+side+"_anim_grp.rx"], cd = "driver_foot"+side+".rz", itt = "spline", ott = "spline")
        cmds.setAttr("ik_foot_anim"+side+".rx", 0)
heelRig()

# ------------------------------------------------------------------
def trapHandleRig():
    cmds.parentConstraint("driver_spine_02", "driver_spine_03", "trap_handle_parent_grp", mo=True)
    cmds.setAttr("trap_handle_parent_grp_parentConstraint1.driver_spine_01W0", 0.12)
    cmds.setAttr("trap_handle_parent_grp_parentConstraint1.driver_spine_02W1", 0.18)
    cmds.setAttr("trap_handle_parent_grp_parentConstraint1.driver_spine_03W2", 0.7)

trapHandleRig()

# ------------------------------------------------------------------
def wristPlatesRig():
    cmds.rotate(-10.523881, 0, 0, "wrist_cover_anim_grp")
    
    target = cmds.spaceLocator(n="wrist_cover_target")[0]
    const = cmds.parentConstraint("driver_bot_loarm_pistonB_bot", target)[0]
    cmds.delete(const)
    
    up = cmds.spaceLocator(n="wrist_cover_up")[0]
    const = cmds.parentConstraint("driver_wrist_cover", up)[0]
    cmds.delete(const)
    cmds.move(0, 5, 0, up, r=True, os=True, wd=True)
    
    grp = cmds.group(em=True, n="wrist_cover_grp")
    const = cmds.parentConstraint("driver_hand_r", grp)[0]
    cmds.delete(const)
    cmds.makeIdentity(grp, a=True, t=True, r=True, s=True)
    
    cmds.parent(target, up, grp)
    cmds.parent(grp, "ctrl_rig")
    
    prntConst = cmds.parentConstraint("driver_bot_loarm_pistonB_bot", "driver_hand_r", grp, mo=True)[0]
    #cmds.pointConstraint("driver_bot_loarm_pistonB_bot", "driver_hand_r", grp, mo=True)
    #ornConst = cmds.orientConstraint("driver_bot_loarm_pistonB_bot", "driver_hand_r", grp, mo=True)[0]
    cmds.setAttr(prntConst+".interpType", 2)

    cmds.aimConstraint(target, "wrist_cover_anim_grp", mo=True, aimVector=(-1, 0, 0), upVector=(0, 1, 0), worldUpType="object", worldUpVector=(0, 1, 0), worldUpObject=str(up))
    cmds.parentConstraint("driver_bot_loarm_pistonB_bot", "driver_hand_r", "wrist_cover_anim_grp", mo=True, sr=["x", "y", "z"])
    #cmds.pointConstraint("driver_bot_loarm_pistonB_bot", "driver_hand_r", "wrist_cover_anim_grp", mo=True)
    
    cmds.setAttr(grp+".v", 0)
wristPlatesRig()

# ------------------------------------------------------------------
def shotgunRig():
    import Tools.ART_addSpaceSwitching as space
    reload(space)

    cmds.addAttr("shotgun_anim", ln = "shotgun_rotate", dv = 0, min = 0, max = 10, keyable = True)
    cmds.addAttr("shotgun_anim", ln = "shotgun_rail", dv = 0, min = 0, max = 10, keyable = True)

    shotgun_rail_grp = cmds.group("shotgun_anim", n="shotgun_anim_rail_grp")
    cmds.xform(shotgun_rail_grp, ws=True, piv=[16.745853637042327, 26.165296554565593, 153.62670823479255])

    cmds.setDrivenKeyframe(["shotgun_anim_rail_grp.rz"], cd = "shotgun_anim.shotgun_rotate", itt = "auto", ott = "auto")
    cmds.setAttr("shotgun_anim.shotgun_rotate", 10)
    cmds.setAttr("shotgun_anim_rail_grp.rz", -90)
    cmds.setDrivenKeyframe(["shotgun_anim_rail_grp.rz"], cd = "shotgun_anim.shotgun_rotate", itt = "auto", ott = "auto")
    cmds.setAttr("shotgun_anim.shotgun_rotate", 0)

    cmds.setDrivenKeyframe(["shotgun_base_anim_grp.tx", "shotgun_base_anim_grp.ty", "shotgun_base_anim_grp.tz", "shotgun_base_anim_grp.rx"], cd = "shotgun_anim.shotgun_rail", itt = "auto", ott = "auto")
    cmds.setAttr("shotgun_anim.shotgun_rail", 5)
    cmds.setAttr("shotgun_base_anim_grp.tx", 3.14)
    cmds.setAttr("shotgun_base_anim_grp.ty", -3.774)
    cmds.setAttr("shotgun_base_anim_grp.tz", 9.402)
    cmds.setAttr("shotgun_base_anim_grp.rx", 13.9)
    cmds.setDrivenKeyframe(["shotgun_base_anim_grp.tx", "shotgun_base_anim_grp.ty", "shotgun_base_anim_grp.tz", "shotgun_base_anim_grp.rx"], cd = "shotgun_anim.shotgun_rail", itt = "auto", ott = "auto")
    cmds.setAttr("shotgun_anim.shotgun_rail", 10)
    cmds.setAttr("shotgun_base_anim_grp.tx", 4.999)
    cmds.setAttr("shotgun_base_anim_grp.ty", -8.562)
    cmds.setAttr("shotgun_base_anim_grp.tz", 12.609)
    cmds.setAttr("shotgun_base_anim_grp.rx", 72.809)
    cmds.setDrivenKeyframe(["shotgun_base_anim_grp.tx", "shotgun_base_anim_grp.ty", "shotgun_base_anim_grp.tz", "shotgun_base_anim_grp.rx"], cd = "shotgun_anim.shotgun_rail", itt = "auto", ott = "auto")
    cmds.setAttr("shotgun_anim.shotgun_rail", 0)

    space.addSpaceSwitching("shotgun_anim", "driver_hand_l", False)
shotgunRig()

# ------------------------------------------------------------------
def shoulderPadRig():
    cmds.setAttr("Rig_Settings.rArmMode", 0)

    shoulderPad_grp = cmds.group("bot_uparm_anim_grp", n="bot_uparm_anim_pivot")
    cmds.xform(shoulderPad_grp, ws=True, piv=[16.745853637042327, 26.165296554565593, 153.62670823479255])

    driver_upperarm_r_loc = cmds.xform("driver_upperarm_r", q=True, ws=True, rp=True)
    print driver_upperarm_r_loc
    cmds.xform(shoulderPad_grp, ws=True, rp=driver_upperarm_r_loc)

    cmds.setAttr("fk_arm_r_anim.ry", 0)
    cmds.setAttr("bot_uparm_anim_pivot.ry", 19.153)
    cmds.setAttr("bot_uparm_anim_grp.tx", 2.209)
    cmds.setAttr("bot_uparm_anim_grp.tz", -2.583)
    cmds.setAttr("bot_uparm_anim_grp.ry", -38.873)
    cmds.setDrivenKeyframe(["bot_uparm_anim_pivot.ry", "bot_uparm_anim_grp.tx", "bot_uparm_anim_grp.tz", "bot_uparm_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setAttr("fk_arm_r_anim.ry", 45)
    cmds.setAttr("bot_uparm_anim_pivot.ry", 0)
    cmds.setAttr("bot_uparm_anim_grp.tx", 0)
    cmds.setAttr("bot_uparm_anim_grp.tz", 0)
    cmds.setAttr("bot_uparm_anim_grp.ry", 0)
    cmds.setDrivenKeyframe(["bot_uparm_anim_pivot.ry", "bot_uparm_anim_grp.tx", "bot_uparm_anim_grp.tz", "bot_uparm_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setAttr("fk_arm_r_anim.ry", -20)
    cmds.setAttr("bot_uparm_anim_pivot.ry", 27.813)
    cmds.setAttr("bot_uparm_anim_grp.tx", 6.98)
    cmds.setAttr("bot_uparm_anim_grp.tz", -1.586)
    cmds.setAttr("bot_uparm_anim_grp.ry", -58.887)
    cmds.setDrivenKeyframe(["bot_uparm_anim_pivot.ry", "bot_uparm_anim_grp.tx", "bot_uparm_anim_grp.tz", "bot_uparm_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setAttr("fk_arm_r_anim.ry", 0)
    
    cmds.setAttr("Rig_Settings.rArmMode", 1)

shoulderPadRig()

# ------------------------------------------------------------------
def sniperCanisterRig():
    import Tools.ART_addSpaceSwitching as space
    reload(space)

    space.addSpaceSwitching("sniper_can_arm_03_anim", "driver_hand_l", False)
    space.addSpaceSwitching("sniper_can_arm_03_anim", "driver_gun", False)
sniperCanisterRig()

# ------------------------------------------------------------------
def trapRig():
    import Tools.ART_addSpaceSwitching as space
    reload(space)

    space.addSpaceSwitching("trap_handle_anim", "driver_hand_l", False)
trapRig()

# ------------------------------------------------------------------
def leftHandRig():
    import Tools.ART_addSpaceSwitching as space
    reload(space)

    space.addSpaceSwitching("ik_wrist_l_anim", "driver_gun_stock_bott", False)
    
    cmds.setAttr("Rig_Settings.lForearmTwistAmount", 0.9)
    cmds.setAttr("Rig_Settings.lForearmTwist2Amount", 0.6)
    
leftHandRig()

# ------------------------------------------------------------------
def maskRig():
    import Tools.ART_addSpaceSwitching as space
    reload(space)

    space.addSpaceSwitching("mask_anim", "driver_hand_l", False)
    space.addSpaceSwitching("mask_anim", "driver_hand_r", False)
maskRig()

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
    for one in ["hip_pack_a_anim", "hip_pack_b_anim", "hip_pack_c_anim", "hip_pack_d_anim"]:
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
addCapsule(42, 86)

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

######################################################################################
######################################################################################
'''
# This script is only used for aligning the pistons to each other when placing the joint movers.   It is also to be used while creating the Rig Pose.  Don't use it in the post script running.
# for each piston end, place the mover at the center of the piston disk (visually), and place the UP locator along the +Z axis of the top part of the piston. (+X should be the down axis, -Y should point up and out of the piston.)
# This script will aim both piston parts at the locator and then rotate (at the end) the bott one to align with the angle it should use if its different.
def alignPistons():
    const = cmds.aimConstraint("bot_uparm_pistonC_top_mover", "bot_uparm_pistonC_bot_mover", weight=1, mo=False, aimVector=(-1, 0, 0), upVector=(0, 0, 1), worldUpType="object", worldUpVector=(0, 0, 1), worldUpObject="bot_uparm_pistonC_top_mover_UP")
    cmds.delete(const)
    const = cmds.aimConstraint("bot_uparm_pistonC_bot_mover", "bot_uparm_pistonC_top_mover", weight=1, mo=False, aimVector=(1, 0, 0), upVector=(0, 0, 1), worldUpType="object", worldUpVector=(0, 0, 1), worldUpObject="bot_uparm_pistonC_top_mover_UP")
    cmds.delete(const)        
    cmds.rotate(77.41, 0, 0, "bot_uparm_pistonC_bot_mover", r=True, os=True)

    const = cmds.aimConstraint("bot_loarm_pistonB_bot_mover", "bot_loarm_pistonB_top_mover", weight=1, mo=False, aimVector=(1, 0, 0), upVector=(0, 0, 1), worldUpType="object", worldUpVector=(0, 0, 1), worldUpObject="bot_loarm_pistonB_top_mover_UP")
    cmds.delete(const)
    const = cmds.aimConstraint("bot_loarm_pistonB_top_mover", "bot_loarm_pistonB_bot_mover", weight=1, mo=False, aimVector=(-1, 0, 0), upVector=(0, 0, 1), worldUpType="object", worldUpVector=(0, 0, 1), worldUpObject="bot_loarm_pistonB_top_mover_UP")
    cmds.delete(const)
    cmds.rotate(15.8, 0, 0, "bot_loarm_pistonB_bot_mover", r=True, os=True)

    const = cmds.aimConstraint("bot_loarm_pistonC_bot_mover", "bot_loarm_pistonC_top_mover", weight=1, mo=False, aimVector=(-1, 0, 0), upVector=(0, 0, 1), worldUpType="object", worldUpVector=(0, 0, 1), worldUpObject="bot_loarm_pistonC_bot_mover_UP")
    cmds.delete(const)
    const = cmds.aimConstraint("bot_loarm_pistonC_top_mover", "bot_loarm_pistonC_bot_mover", weight=1, mo=False, aimVector=(1, 0, 0), upVector=(0, 0, 1), worldUpType="object", worldUpVector=(0, 0, 1), worldUpObject="bot_loarm_pistonC_bot_mover_UP")
    cmds.delete(const)
    cmds.rotate(0, 0, 0, "bot_loarm_pistonC_top_mover", r=True, os=True)
            
    const = cmds.aimConstraint("bot_loarm_pistonD_bot_mover", "bot_loarm_pistonD_top_mover", weight=1, mo=False, aimVector=(-1, 0, 0), upVector=(0, 0, 1), worldUpType="object", worldUpVector=(0, 0, 1), worldUpObject="bot_loarm_pistonD_bot_mover_UP")
    cmds.delete(const)
    const = cmds.aimConstraint("bot_loarm_pistonD_top_mover", "bot_loarm_pistonD_bot_mover", weight=1, mo=False, aimVector=(1, 0, 0), upVector=(0, 0, 1), worldUpType="object", worldUpVector=(0, 0, 1), worldUpObject="bot_loarm_pistonD_bot_mover_UP")
    cmds.delete(const)
    cmds.rotate(-84.5, 0, 0, "bot_loarm_pistonD_top_mover", r=True, os=True)
alignPistons()


# Then make sure that the UP locators are parent constrained to the uparm and loarm or loarm twist movers. (the ones that are leafs, not the actual skeleton ones.) Use the following script to do this.
# REMINDER: This must be done BEFORE you move on to Deformation Setup phase.
cmds.parentConstraint("bot_uparm_mover", "bot_uparm_pistonC_top_mover_UP", weight=1, mo=True)
cmds.parentConstraint("bot_loarm_mover", "bot_loarm_pistonB_top_mover_UP", weight=1, mo=True)
cmds.parentConstraint("bot_loarm_twist_mover", "bot_loarm_pistonC_bot_mover_UP", weight=1, mo=True)
cmds.parentConstraint("bot_loarm_twist_mover", "bot_loarm_pistonD_bot_mover_UP", weight=1, mo=True)
'''