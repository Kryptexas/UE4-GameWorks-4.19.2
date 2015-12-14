import maya.cmds as cmds
import maya.mel as mel
import math
import Tools.ART_CreateHydraulicRigs as hyd
reload(hyd)
import Tools.ART_addSpaceSwitching as space
reload(space)
mel.eval("source generateChannelMenu.mel;")

# ------------------------------------------------------------------
def scaleControls():
    objects = ["ik_wrist_l_anim", "ik_wrist_r_anim", "index_finger_fk_ctrl_1_l", "thumb_finger_fk_ctrl_1_l", "body_anim", "mid_ik_anim", "chest_ik_anim", "hip_anim",  "neck_01_fk_anim", "head_fk_anim", "fk_wrist_l_anim" , "fk_wrist_r_anim" , "fk_elbow_l_anim", "fk_elbow_r_anim", "fk_arm_l_anim", "fk_arm_r_anim", "index_finger_fk_ctrl_1_r", "index_finger_fk_ctrl_2_r", "index_finger_fk_ctrl_3_r", "middle_finger_fk_ctrl_1_r", "middle_finger_fk_ctrl_2_r", "middle_finger_fk_ctrl_3_r", "ring_finger_fk_ctrl_1_r", "ring_finger_fk_ctrl_2_r", "ring_finger_fk_ctrl_3_r", "pinky_finger_fk_ctrl_1_r", "thumb_finger_fk_ctrl_1_r", "thumb_finger_fk_ctrl_2_r", "thumb_finger_fk_ctrl_3_r", "spine_01_anim", "spine_02_anim", "spine_03_anim", "neck_01_fk_anim"]
    for one in objects:
        cmds.select([one+".cv[0:*]"])
        cmds.scale(2.3, 2.3, 2.3, r=True, os=True)

    objects = ["arm_piston_A_top_anim", "arm_piston_A_bott_anim", "arm_piston_B_top_anim", "arm_piston_B_bott_anim", "muscle_belly_anim", "toe_big_a_l_anim", "toe_big_a_r_anim", "toe_mid_a_l_anim",  "toe_mid_a_r_anim", "toe_pink_a_l_anim", "toe_pink_a_r_anim"]
    for one in objects:
        cmds.select([one+".cv[0:*]"])
        cmds.scale(0.36, 0.36, 0.36, r=True, os=True)

    # eyes
    for side in ["_l", "_r"]:
        cmds.select(["eyeball"+side+"_anim.cv[0:*]"])
        cmds.scale(0.1, 0.1, 0.05, r=True, ocp = False)
        if side == "_l":
            cmds.move(0, -3.757386, 0, r=True, os=True)
        else:
            cmds.move(0, -3.757386, 0, r=True, os=True)

        cmds.select(["eyelid_bott"+side+"_anim.cv[0:*]"])
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
            cmds.move(0, 3.757386, 0.5, r=True, os=True)

    # jaw
    cmds.select(["jaw_anim.cv[0:*]"])
    cmds.scale(0.25, 0.25, 0.25, r = True, ocp = True)
    cmds.move(0, -24, -16, r = True)

    # ears
    cmds.select(["ear_l_anim.cv[0:*]"])
    cmds.scale(0.25, 0.25, 0.25, r = True, ocp = False)
    cmds.move(6, 8, 12, r = True)

    cmds.select(["ear_r_anim.cv[0:*]"])
    cmds.scale(0.25, 0.25, 0.25, r = True, ocp = False)
    cmds.move(-6, 8, 12, r = True)

    # foot
    cmds.select(["ik_foot_anim_l.cv[0:*]"])
    cmds.scale(1.4, 1.4, 1.4, r = True, ocp = False)
    cmds.move(0, 0, -1.129323, r = True)

    cmds.select(["ik_foot_anim_r.cv[0:*]"])
    cmds.scale(1.4, 1.4, 1.4, r = True, ocp = False)
    cmds.move(0, 0, -1.129323, r = True)

    cmds.select(["head_fk_anim.cv[0:*]"])
    cmds.scale(1.9, 1.9, 1.9, r = True, ocp = False)

    cmds.select(None)

scaleControls()

# ------------------------------------------------------------------
def dynamicsOff():
    for one in ["tongue_dyn_anim", "ponytail_dyn_anim"]:
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
def createPistons():
    import Tools.ART_CreateHydraulicRigs as hyd
    reload(hyd)

    # pistons
    cmds.select("arm_piston_A_top_anim", "arm_piston_A_bott_anim", "driver_index_01_l")
    hyd.CreateHydraulicRig(-1)
    cmds.select("arm_piston_B_top_anim", "arm_piston_B_bott_anim", "driver_index_01_l")
    hyd.CreateHydraulicRig(-1)

    cmds.setAttr("Rig_Settings.lForearmTwistAmount", 1)

createPistons()

# ------------------------------------------------------------------
def fixToes():
    for one in ["driver_toe_mid_a_l_parentConstraint1", "driver_toe_mid_a_r_parentConstraint1", "driver_toe_pink_a_l_parentConstraint1", "driver_toe_pink_a_r_parentConstraint1", "driver_toe_big_a_l_parentConstraint1", "driver_toe_big_a_r_parentConstraint1"]:
        cmds.setAttr(one+".interpType", 2)
fixToes()

# ------------------------------------------------------------------
def stretchArmRig():

    # Turn on the stretch attr.
    cmds.setAttr("ik_wrist_l_anim.stretch", 1)

    # Break the connections for the upperam so it doesn stretch.  Also for the scaleblend on the lower arm.
    mel.eval("source generateChannelMenu.mel;CBdeleteConnection \"ik_upperarm_l.sx\"")
    mel.eval("source generateChannelMenu.mel;CBdeleteConnection \"ik_upperarm_l.sy\"")
    mel.eval("source generateChannelMenu.mel;CBdeleteConnection \"ik_upperarm_l.sz\"")

    mel.eval("source generateChannelMenu.mel;CBdeleteConnection \"lo_arm_scale_blend_l.b\"")
    cmds.setAttr("lo_arm_scale_blend_l.blender", 0)

    # Create a plus minus node, connect the current arm distance into it so I can find the difference between the current arm and the original length.
    plusMinusCurrent = cmds.shadingNode("plusMinusAverage", asUtility=True, name="currentLength_plusMinusNode")
    cmds.connectAttr("ik_arm_distBetween_l.distance", plusMinusCurrent+".input1D[0]", f=True)
    cmds.connectAttr("ik_arm_distBetween_l.distance", plusMinusCurrent+".input1D[1]", f=True)
    mel.eval("CBdeleteConnection \""+plusMinusCurrent+".input1D[1]\"")
    cmds.setAttr(plusMinusCurrent+".operation", 2)

    # Hook the previous node into another node that adds the difference to the original distance.
    plusMinusAdd = cmds.shadingNode("plusMinusAverage", asUtility=True, name="addCurrent_plusMinusNode")
    cmds.connectAttr("ik_arm_distBetween_l.distance", plusMinusAdd+".input1D[0]", f=True)
    cmds.connectAttr(plusMinusCurrent+".output1D", plusMinusAdd+".input1D[1]", f=True)

    # Re-connect these values into the arm to replace the old connections.
    cmds.connectAttr(plusMinusAdd+".output1D", "ik_arm_stretch_condition_l.colorIfFalseR", f=True)
    cmds.connectAttr(plusMinusAdd+".output1D", "ik_arm_stretch_condition_l.secondTerm", f=True)

stretchArmRig()