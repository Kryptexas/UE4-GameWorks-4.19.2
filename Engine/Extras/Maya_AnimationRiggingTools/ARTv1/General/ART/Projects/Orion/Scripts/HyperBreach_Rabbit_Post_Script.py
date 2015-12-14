import maya.cmds as cmds

# ------------------------------------------------------------------
def heelHelper():
    
    sides = ["_l", "_r"]
    for side in sides:
    
        calfJoint = "calf"+side
        footJoint = "foot"+side
        
        heelHelperGrp = "heel_helper"+side+"_anim_grp"
        
        # create the target joint
        heelHelperTarget = cmds.duplicate(heelHelperGrp, name=heelHelperGrp+"_target", renameChildren=True)
        heelHelperTarget[1] = cmds.rename(heelHelperTarget[1], heelHelperTarget[1]+"_target")
        
        if side == "_l":
            cmds.move(0, -13.21673, 0, heelHelperTarget[0], r=True, os=True, wd=True)
        else:
            cmds.move(0, 13.21673, 0, heelHelperTarget[0], r=True, os=True, wd=True)
        
        # constrain the rig together
        cmds.parentConstraint(calfJoint, heelHelperTarget[0], name="heelHelperTargetPrntCnst"+side, mo=True, weight=1)
        cmds.aimConstraint(heelHelperTarget[1], heelHelperGrp, name="heelHelperDriverAimCnst"+side, mo=True, weight=1, aimVector=(0, -1, 0), upVector=(0, 0, -1), worldUpType="object", worldUpObject="foot_l")

heelHelper()


# ------------------------------------------------------------------
def fixUpToes():
    # change the way the Set driven keys on the heel and ball pivots work so that the leg can work more like a quadruped leg.
    cmds.keyframe("ik_foot_heel_pivot_l_rotateZ", edit=True, index=(2,2), absolute=True, valueChange=0)
    cmds.keyframe("ik_foot_ball_pivot_l_rotateZ", edit=True, index=(2,2), absolute=True, valueChange=90)

    cmds.keyframe("ik_foot_heel_pivot_r_rotateZ", edit=True, index=(2,2), absolute=True, valueChange=0)
    cmds.keyframe("ik_foot_ball_pivot_r_rotateZ", edit=True, index=(2,2), absolute=True, valueChange=90)
    
    cmds.setAttr("driver_tarsal_l_parentConstraint1.rig_tarsal_lW0", 0)
    cmds.setAttr("driver_tarsal_r_parentConstraint1.rig_tarsal_rW0", 0)


fixUpToes()

# ------------------------------------------------------------------
# Keep the following 2 lines to find the location of these 2 locators after moving them further away from the leg.
# Tip for finding the best location, duplicate the leg mesh so you have one in the default position.  
# Then just move these locators away from the body until they are plenty clear and the edges from the rigged mesh and duped one line up.
#cmds.xform("thigh_twist_driver_l_PV", q=True, ws=True, piv=True)
#cmds.xform("thigh_twist_driver_l_aimUp", q=True, ws=True, piv=True)

# Fix the twists in the thighs so that they dont twist up so easily when the leg rotates sideways or back.
def fixTwistInThighs():
    sides = ["_l", "_r"]
    for side in sides:
        # Re-create the pole vector for the thigh IK handle
        cmds.delete("thigh_twist_driver"+side+"_ikHandle_poleVectorConstraint1")
        if side == "_l":
            cmds.xform("thigh_twist_driver"+side+"_PV", ws=True, translation=(40.823986719916384, -10.167150062315951, 59.8508899397534))
        else:
            cmds.xform("thigh_twist_driver"+side+"_PV", ws=True, translation=(-40.823986719916384, -10.167150062315951, 59.8508899397534))
        cmds.poleVectorConstraint("thigh_twist_driver"+side+"_PV", "thigh_twist_driver"+side+"_ikHandle")

        # Move the up for the aim constraint and re-create it.
        cmds.delete("thigh"+side+"_twist_joint_aimConstraint1")
        if side == "_l":
            cmds.xform("thigh_twist_driver"+side+"_aimUp", ws=True, translation=(7.615877731473773, 32.26791726115082, 32.355635500592044))
        else:
            cmds.xform("thigh_twist_driver"+side+"_aimUp", ws=True, translation=(-7.615877731473773, 32.26791726115082, 32.355635500592044))
        cmds.aimConstraint("driver_calf"+side, "thigh"+side+"_twist_joint", weight = 1, aimVector = [-1, 0, 0], upVector = [0, 1, 0], worldUpType = "object", worldUpObject = "thigh_twist_driver"+side+"_aimUp")

fixTwistInThighs()


# ------------------------------------------------------------------
def scaleControls():
    # brows
    cmds.select(["brow_out_l_anim.cv[0:32]", "brow_out_r_anim.cv[0:32]"])
    cmds.scale(0.132495, 0.132495, 0.132495, r = True, ocp = True)
    
    cmds.select(["brow_in_l_anim.cv[0:32]", "brow_in_r_anim.cv[0:32]"])
    cmds.scale(0.132495, 0.132495, 0.132495, r = True, ocp = True)

    cmds.select(["brow_mid_anim.cv[0:32]"])
    cmds.scale(0.132495, 0.132495, 0.132495, r = True, ocp = True)
    
    # eyes
    cmds.select(["eyelid_top_l_anim.cv[0:32]", "eyelid_top_r_anim.cv[0:32]"])
    cmds.scale(0.0483613, 0.0483613, 0.0483613, r = True, ocp = True)
    cmds.move(0, -4.024, 0.759295, r = True)
    
    cmds.select(["eyelid_bott_l_anim.cv[0:32]", "eyelid_bott_r_anim.cv[0:32]"])
    cmds.scale(0.0483613, 0.0483613, 0.0483613, r = True, ocp = True)
    cmds.move(0, -3.759804, -1.558685, r = True)

    cmds.select(["eyeball_l_anim.cv[0:32]", "eyeball_r_anim.cv[0:32]"])
    cmds.scale(0.0483613, 0.0483613, 0.0483613, r = True, ocp = True)
    cmds.move(0, -4.077843, -0.412389, r = True)

    # jaw
    cmds.select(["jaw_anim.cv[0:32]"])
    cmds.scale(0.294922, 0.294922, 0.294922, r = True, ocp = True)
    cmds.move(0, -12, -10, r = True)
    
    # holster
    cmds.select(["gun_anim.cv[0:32]"])
    cmds.scale(0.294922, 0.294922, 0.294922, r = True, ocp = True)
    
    # heel helpers
    cmds.select(["heel_helper_l_anim.cv[0:32]", "heel_helper_r_anim.cv[0:32]"])
    cmds.scale(0.294922, 0.294922, 0.294922, r = True, ocp = True)
    
    cmds.select(["heel_helper_l_anim1_target.cv[0:32]", "heel_helper_r_anim1_target.cv[0:32]"])
    cmds.scale(0.2, 0.2, 0.2, r = True, ocp = True)
    
    # toes
    cmds.select(["tarsal_l_anim.cv[0:32]", "tarsal_r_anim.cv[0:32]"])
    cmds.scale(0.294922, 0.294922, 0.294922, r = True, ocp = True)
    
    # ears
    cmds.select(["fk_earMain_l_01_anim.cv[0:7]", "fk_earMain_r_01_anim.cv[0:7]"])
    cmds.scale(1.522128, 1.522128, 1.522128, r = True, ocp = False)

    cmds.select(["fk_earMain_l_03_anim.cv[0:7]", "fk_earMain_r_03_anim.cv[0:7]"])
    cmds.scale(0.696643, 0.696643, 0.696643, r = True, ocp = False)
    
    # face fur
    cmds.select(["fk_headfur_mid_l_02_anim.cv[0:7]", "fk_headfur_mid_r_02_anim.cv[0:7]"])
    cmds.scale(0.439838, 0.439838, 0.439838, r = True, ocp = True)
    
    # neck and head
    cmds.select(["neck_01_fk_anim.cv[0:7]"])
    cmds.scale(2.18565, 2.18565, 2.18565, r = True, ocp = False)

    cmds.select(["neck_02_fk_anim.cv[0:7]"])
    cmds.scale(2.18565, 2.18565, 2.18565, r = True, ocp = False)

    cmds.select(["head_fk_anim.cv[0:7]"])
    cmds.scale(2.483665, 2.483665, 2.483665, r = True, ocp = True)
    
    # foot
    cmds.select(["ik_foot_anim_l.cv[0:17]"])
    cmds.scale(1.4, 1.4, 1.4, r = True, ocp = False)
    cmds.move(0.8, 2.63, -16, r = True)
    
    cmds.select(["ik_foot_anim_r.cv[0:17]"])
    cmds.scale(1.4, 1.4, 1.4, r = True, ocp = False)
    cmds.move(-0.8, 2.63, -16, r = True)

    cmds.select(None)

scaleControls()