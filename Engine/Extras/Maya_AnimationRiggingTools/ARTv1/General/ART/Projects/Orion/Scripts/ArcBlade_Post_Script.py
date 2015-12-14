import maya.cmds as cmds

###############################TEMP####################
# Constrain the new weapon_l control to the old weapon control.  Only apply this manually AFTER you have built the rig successfully.
'''
cmds.parentConstraint("weapon_anim", "weapon_l_anim_grp", mo=False)
'''

# ------------------------------------------------------------------
# scale all of the controls to be more visible with the mesh.
def scaleControls():
    sides = ["_l", "_r"]
    for side in sides:
        # feet
        cmds.select(["ik_foot_anim"+side+".cv[*]"])
        cmds.scale(1, 1, 1, r = True, ocp = False)
        cmds.move(0, 0, -6, r = True)

        cmds.select(["ik_wrist"+side+"_anim.cv[*]"])
        cmds.scale(1.9, 1.9, 1.9, r = True, ocp = False)

        cmds.select(["eyebrow_in"+side+"_anim.cv[*]"])
        cmds.scale(0.117744, 0.117744, 0.117744, r=True, ocp = False)
        cmds.move(0, 0, 0, r=True)

        cmds.select(["eyebrow_out"+side+"_anim.cv[*]"])
        cmds.scale(0.117744, 0.117744, 0.117744, r=True, ocp = False)
        cmds.move(0, 0, 0, r=True)

        cmds.select(["eyeball"+side+"_anim.cv[*]"])
        cmds.scale(0.1, 0.1, 0.05, r=True, ocp = False)
        if side == "_l":
            cmds.move(0, -3.757386, 0, r=True, os=True)
        else:
            cmds.move(0, -3.757386, 0, r=True, os=True)

        cmds.select(["eyelid_bott"+side+"_anim.cv[*]"])
        cmds.scale(0.1, 0.1, 0.01, r=True, ocp = False)
        if side == "_l":
            cmds.move(0, -3.757386, 0.5, r=True, os=True)
        else:
            cmds.move(0, 3.757386, 0.5, r=True, os=True)

        cmds.select(["eyelid_top"+side+"_anim.cv[*]"])
        cmds.scale(0.1, 0.1, 0.01, r=True, ocp = False)
        if side == "_l":
            cmds.move(0, -3.757386, 0.5, r=True, os=True)
        else:
            cmds.move(0, 3.757386, 0.5, r=True, os=True)

    objects = ["shoulder_pad_r_anim", "weapon_bladeA_01_anim", "weapon_bladeA_02_anim", "weapon_bladeA_03_anim", "weapon_bladeA_04_anim", "weapon_bladeA_05_anim", "weapon_bladeB_01_anim", "weapon_bladeB_02_anim", "weapon_bladeB_03_anim", "weapon_bladeB_04_anim", "weapon_bladeB_05_anim", "weapon_bladeC_01_anim", "weapon_bladeC_02_anim", "weapon_bladeC_03_anim", "weapon_bladeD_01_anim", "weapon_bladeD_02_anim", "weapon_bladeE_01_anim", "pipe_anim", "hip_back_flaps_anim", "shoulder_cloth_01_l_anim", "shoulder_cloth_02_l_anim", "bracelet_A_l_anim", "bracelet_B_l_anim", "bracelet_C_l_anim", "fk_padawan_l_01_anim", "fk_padawan_l_02_anim", "fk_padawan_r_01_anim", "fk_padawan_r_02_anim", "shoulder_blade_in_r_anim", "shoulder_blade_mid_r_anim", "shoulder_blade_out_r_anim", "wrist_plate_r_anim", "fk_loin_cloth_fr_01_anim", "fk_loin_cloth_fr_02_anim", "fk_loin_cloth_bk_01_anim", "fk_loin_cloth_bk_02_anim", "fk_back_sash_01_anim", "fk_back_sash_02_anim", "fk_back_sash_03_anim"]
    for one in objects:
        cmds.select([one+".cv[*]"])
        cmds.scale(0.227057, 0.227057, 0.227057, r = True, ocp = True)
        cmds.move(0, 0, 0, r = True, os=True, wd=True)

    objects = ["fk_hip_eyelet_front_l_01_anim", "fk_hip_eyelet_front_l_02_anim", "fk_hip_eyelet_front_l_03_anim", "fk_hip_eyelet_front_r_01_anim", "fk_hip_eyelet_front_r_02_anim", "fk_hip_eyelet_front_r_03_anim", "fk_hip_eyelet_rear_l_01_anim", "fk_hip_eyelet_rear_l_02_anim", "fk_hip_eyelet_rear_l_03_anim", "fk_hip_eyelet_rear_r_01_anim", "fk_hip_eyelet_rear_r_02_anim", "fk_hip_eyelet_rear_r_03_anim", "fk_weapon_butt_01_anim", "fk_weapon_butt_02_anim", "fk_weapon_butt_03_anim", "fk_ponytail_01_anim", "fk_ponytail_02_anim", "fk_ponytail_03_anim"]
    for one in objects:
        cmds.select([one+".cv[*]"])
        cmds.scale(0.227057, 0.227057, 0.227057, r = True, ocp = True)
        cmds.move(0, 0, 0, r = True, os=True, wd=True)

    objects = ["leg_shieldPad_A_l_anim", "leg_shieldPad_A_r_anim", "leg_shieldPad_B_l_anim", "leg_shieldPad_B_r_anim"]
    for one in objects:
        cmds.select([one+".cv[*]"])
        cmds.scale(0.227057, 0.227057, 0.227057, r = True, ocp = True)
        cmds.move(0, 0, 0, r = True, os=True, wd=True)
        
    cmds.select(["weapon_l_anim.cv[*]"])
    cmds.scale(0.330274, 0.330274, 0.330274, r = True, ocp = False)

    cmds.select(["weapon_r_anim.cv[*]"])
    cmds.scale(0.330274, 0.330274, 0.330274, r = True, ocp = False)

    cmds.select(["weapon_bladeA_01_anim.cv[*]"])
    cmds.move(0, 0, -6.5, r = True)            

    cmds.select(["weapon_bladeB_01_anim.cv[*]"])
    cmds.move(0, 5, -5, r = True)            

    cmds.select(["weapon_bladeC_01_anim.cv[*]"])
    cmds.move(0, -6, 0, r = True)            

    cmds.select(["weapon_bladeD_01_anim.cv[*]"])
    cmds.move(0, 5, 5, r = True)            

    # Other
    cmds.select(["jaw_anim.cv[*]"])
    cmds.scale(0.2, 0.2, 0.2, r = True, ocp = True)
    cmds.move(0, -8, -12, r = True)

    cmds.select(None)
scaleControls()

# ------------------------------------------------------------------
def armsLegsRig():
    cmds.setAttr("Rig_Settings.lUpperarmTwist2Amount", 0.3)
    cmds.setAttr("Rig_Settings.rUpperarmTwist2Amount", 0.3)

    cmds.setAttr("Rig_Settings.lThighTwist2Amount", 0.3)
    cmds.setAttr("Rig_Settings.rThighTwist2Amount", 0.3)
armsLegsRig()

# ------------------------------------------------------------------
def dynamicsOff():
    for one in ["leg_shieldPad_A_l_anim", "leg_shieldPad_A_r_anim", "leg_shieldPad_B_l_anim", "leg_shieldPad_B_r_anim", "back_sash_dyn_anim", "hip_eyelet_front_l_dyn_anim", "hip_eyelet_front_r_dyn_anim", "hip_eyelet_rear_l_dyn_anim", "hip_eyelet_rear_r_dyn_anim", "loin_cloth_bk_dyn_anim", "loin_cloth_fr_dyn_anim", "padawan_l_dyn_anim", "padawan_r_dyn_anim", "ponytail_dyn_anim", "weapon_butt_dyn_anim"]:
        cmds.setAttr(one+".chainStartEnvelope", 0)
        cmds.setAttr(one+".chainStartEnvelope", l=True)
dynamicsOff()

# ------------------------------------------------------------------
def pipeRig():
    pipe_lip_anim = cmds.duplicate("pipe_anim", n="pipe_lip_anim")[0]
    cmds.parent(pipe_lip_anim, "pipe_anim_space_switcher")
    cmds.setAttr(pipe_lip_anim+".ty", 20.67896)
    cmds.setAttr(pipe_lip_anim+".tz", 14.37459)
    cmds.makeIdentity(pipe_lip_anim, a=True, t=True, r=True, s=True)
    cmds.parent("pipe_anim_grp", pipe_lip_anim)
pipeRig()

# ------------------------------------------------------------------
def weaponRig():
    import Tools.ART_addSpaceSwitching as space
    reload(space)
    
    weapon_blades_master_anim = cmds.duplicate("weapon_bladeC_01_anim", n="weapon_blades_master_anim")[0]
    cmds.lockNode(weapon_blades_master_anim, l=False)
    weapon_blades_master_anim_grp = cmds.group(weapon_blades_master_anim, n=weapon_blades_master_anim+"_grp")
    cmds.parent(weapon_blades_master_anim_grp, "ctrl_rig")
    cmds.parentConstraint("weapon_l_anim", weapon_blades_master_anim_grp, mo=True)

    cmds.select([weapon_blades_master_anim+".cv[*]"])
    cmds.scale(5, 1, 1, r = True, ocp = False)
    cmds.move(0, 6, 0, r = True)            

    space.addSpaceSwitching("weapon_bladeA_01_anim", weapon_blades_master_anim, True)
    space.addSpaceSwitching("weapon_bladeB_01_anim", weapon_blades_master_anim, True)
    space.addSpaceSwitching("weapon_bladeC_01_anim", weapon_blades_master_anim, True)
    space.addSpaceSwitching("weapon_bladeD_01_anim", weapon_blades_master_anim, True)
    
    for one in ["weapon_bladeA_02_anim", "weapon_bladeB_02_anim"]:
        cmds.setAttr(one+".tx", l=True)
        cmds.setAttr(one+".ty", l=True)
        cmds.setAttr(one+".rx", l=True)
        cmds.setAttr(one+".ry", l=True)
        cmds.setAttr(one+".rz", l=True)

    cmds.setAttr("weapon_bladeC_02_anim.tx", l=True)
    cmds.setAttr("weapon_bladeC_02_anim.tz", l=True)
    cmds.setAttr("weapon_bladeC_02_anim.rx", l=True)
    cmds.setAttr("weapon_bladeC_02_anim.ry", l=True)
    cmds.setAttr("weapon_bladeC_02_anim.rz", l=True)

weaponRig()

# ------------------------------------------------------------------
def rightShoulderRig():
    cmds.setAttr("Rig_Settings.rArmMode", 0)

    # frame - Arm up
    cmds.setAttr("fk_arm_r_anim.ry", -45)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.tx", 0.81311)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.ty", -0.00492)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.tz", -1.23185)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.ry", -25.06673)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.tx", 2.19854)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.ty", -0.03492)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.tz", -4.35591)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.ry", -42.79489)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.tx", 2.89611)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.ty", -0.10336)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.tz", -7.86719)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.ry", -56.77536)
    cmds.setDrivenKeyframe(["shoulder_blade_in_r_anim_grp.tx","shoulder_blade_in_r_anim_grp.ty","shoulder_blade_in_r_anim_grp.tz","shoulder_blade_in_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setDrivenKeyframe(["shoulder_blade_mid_r_anim_grp.tx","shoulder_blade_mid_r_anim_grp.ty","shoulder_blade_mid_r_anim_grp.tz","shoulder_blade_mid_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setDrivenKeyframe(["shoulder_blade_out_r_anim_grp.tx","shoulder_blade_out_r_anim_grp.ty","shoulder_blade_out_r_anim_grp.tz","shoulder_blade_out_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setAttr("shoulder_pad_r_anim_grp.tx", -2.9675)
    cmds.setAttr("shoulder_pad_r_anim_grp.ty", 0)
    cmds.setAttr("shoulder_pad_r_anim_grp.tz", 6.70308)
    cmds.setAttr("shoulder_pad_r_anim_grp.ry", -8.42354)
    cmds.setDrivenKeyframe(["shoulder_pad_r_anim_grp.tx","shoulder_pad_r_anim_grp.ty","shoulder_pad_r_anim_grp.tz","shoulder_pad_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")

    # frame - Rig Pose
    cmds.setAttr("fk_arm_r_anim.ry", 0)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.tx", 0)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.ty", 0)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.tz", 0)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.ry", -19.78132)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.tx", 0.51636)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.ty", -0.03)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.tz", -2.18934)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.ry", -33.24852)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.tx", 0.6353)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.ty", -0.09843)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.tz", -4.75068)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.ry", -44.83553)
    cmds.setDrivenKeyframe(["shoulder_blade_in_r_anim_grp.tx","shoulder_blade_in_r_anim_grp.ty","shoulder_blade_in_r_anim_grp.tz","shoulder_blade_in_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setDrivenKeyframe(["shoulder_blade_mid_r_anim_grp.tx","shoulder_blade_mid_r_anim_grp.ty","shoulder_blade_mid_r_anim_grp.tz","shoulder_blade_mid_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setDrivenKeyframe(["shoulder_blade_out_r_anim_grp.tx","shoulder_blade_out_r_anim_grp.ty","shoulder_blade_out_r_anim_grp.tz","shoulder_blade_out_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setAttr("shoulder_pad_r_anim_grp.tx", -2.38193)
    cmds.setAttr("shoulder_pad_r_anim_grp.ty", 0)
    cmds.setAttr("shoulder_pad_r_anim_grp.tz", 3.01712)
    cmds.setAttr("shoulder_pad_r_anim_grp.ry", -3.31378)
    cmds.setDrivenKeyframe(["shoulder_pad_r_anim_grp.tx","shoulder_pad_r_anim_grp.ty","shoulder_pad_r_anim_grp.tz","shoulder_pad_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")

    # frame - Model Pose
    cmds.setAttr("fk_arm_r_anim.ry", 47)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.tx", 0)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.ty", 0)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.tz", 1.03919)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.ry", 0)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.tx", 0)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.ty", 0)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.tz", 0.19052)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.ry", 0)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.tx", 0)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.ty", 0)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.tz", -0.2598)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.ry", 0)
    cmds.setDrivenKeyframe(["shoulder_blade_in_r_anim_grp.tx","shoulder_blade_in_r_anim_grp.ty","shoulder_blade_in_r_anim_grp.tz","shoulder_blade_in_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setDrivenKeyframe(["shoulder_blade_mid_r_anim_grp.tx","shoulder_blade_mid_r_anim_grp.ty","shoulder_blade_mid_r_anim_grp.tz","shoulder_blade_mid_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setDrivenKeyframe(["shoulder_blade_out_r_anim_grp.tx","shoulder_blade_out_r_anim_grp.ty","shoulder_blade_out_r_anim_grp.tz","shoulder_blade_out_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setAttr("shoulder_pad_r_anim_grp.tx", 0)
    cmds.setAttr("shoulder_pad_r_anim_grp.ty", 0)
    cmds.setAttr("shoulder_pad_r_anim_grp.tz", 0)
    cmds.setAttr("shoulder_pad_r_anim_grp.ry", 0)
    cmds.setDrivenKeyframe(["shoulder_pad_r_anim_grp.tx","shoulder_pad_r_anim_grp.ty","shoulder_pad_r_anim_grp.tz","shoulder_pad_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")

    # frame - Arm down
    cmds.setAttr("fk_arm_r_anim.ry", 100)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.tx", -0.01968)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.ty", 0.06663)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.tz", 2.23831)
    cmds.setAttr("shoulder_blade_in_r_anim_grp.ry", 35.207)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.tx", 0.17108)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.ty", 0.07195)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.tz", 4.84536)
    cmds.setAttr("shoulder_blade_mid_r_anim_grp.ry", 55.64349)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.tx", 0.63292)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.ty", 0.10683)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.tz", 7.36452)
    cmds.setAttr("shoulder_blade_out_r_anim_grp.ry", 78.63055)
    cmds.setDrivenKeyframe(["shoulder_blade_in_r_anim_grp.tx","shoulder_blade_in_r_anim_grp.ty","shoulder_blade_in_r_anim_grp.tz","shoulder_blade_in_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setDrivenKeyframe(["shoulder_blade_mid_r_anim_grp.tx","shoulder_blade_mid_r_anim_grp.ty","shoulder_blade_mid_r_anim_grp.tz","shoulder_blade_mid_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setDrivenKeyframe(["shoulder_blade_out_r_anim_grp.tx","shoulder_blade_out_r_anim_grp.ty","shoulder_blade_out_r_anim_grp.tz","shoulder_blade_out_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")
    cmds.setAttr("shoulder_pad_r_anim_grp.tx", 0.4533)
    cmds.setAttr("shoulder_pad_r_anim_grp.ty", 0)
    cmds.setAttr("shoulder_pad_r_anim_grp.tz", 0.62524)
    cmds.setAttr("shoulder_pad_r_anim_grp.ry", 2.32903)
    cmds.setDrivenKeyframe(["shoulder_pad_r_anim_grp.tx","shoulder_pad_r_anim_grp.ty","shoulder_pad_r_anim_grp.tz","shoulder_pad_r_anim_grp.ry"], cd = "driver_upperarm_r.ry", itt = "spline", ott = "spline")

    cmds.setAttr("fk_arm_r_anim.ry", 0)
    cmds.setAttr("Rig_Settings.rArmMode", 1)
rightShoulderRig()

# ------------------------------------------------------------------
def rightWristRig():
    cmds.orientConstraint("driver_hand_r", "wrist_plate_r_anim_grp", mo=True)
rightWristRig()

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