import maya.cmds as cmds
import maya.mel as mel
import math

# ------------------------------------------------------------------
def scaleControls():
    
    # eyes
    for side in ["_l", "_r"]:
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
    
        cmds.select(["eyebrow_in"+side+"_anim.cv[*]"])
        cmds.scale(0.117744, 0.117744, 0.117744, r=True, ocp = False)
        cmds.move(0, 0, 0, r=True)

        cmds.select(["eyebrow_out"+side+"_anim.cv[*]"])
        cmds.scale(0.117744, 0.117744, 0.117744, r=True, ocp = False)
        cmds.move(0, 0, 0, r=True)

        # ears
        cmds.select(["ear"+side+"_anim.cv[*]"])
        cmds.scale(0.33, 0.33, 0.33, r = True, ocp = False)
    
        # wings
        cmds.select(["wing_bott"+side+"_anim.cv[*]"])
        cmds.scale(0.33, 0.33, 0.33, r = True, ocp = False)
    
        cmds.select(["wing_top"+side+"_anim.cv[*]"])
        cmds.scale(0.33, 0.33, 0.33, r = True, ocp = False)
    
        # toes
        cmds.select(["big_toe"+side+"_anim.cv[*]"])
        cmds.scale(0.15, 0.15, 0.15, r = True, ocp = False)

        cmds.select(["mid_toe"+side+"_anim.cv[*]"])
        cmds.scale(0.15, 0.15, 0.15, r = True, ocp = False)

        cmds.select(["pinky_toe"+side+"_anim.cv[*]"])
        cmds.scale(0.15, 0.15, 0.15, r = True, ocp = False)

    # jaw
    cmds.select(["jaw_anim.cv[*]"])
    cmds.scale(0.294922, 0.294922, 0.294922, r = True, ocp = True)
    cmds.move(0, -12, -10, r = True)
        
    # neck and head
    cmds.select(["neck_01_fk_anim.cv[*]"])
    cmds.scale(2.0, 2.0, 2.0, r = True, ocp = False)

    cmds.select(["head_fk_anim.cv[*]"])
    cmds.scale(3.0, 3.0, 3.0, r = True, ocp = True)
    
    # foot
    cmds.select(["ik_foot_anim_l.cv[*]"])
    cmds.scale(1.4, 1.4, 1.4, r = True, ocp = False)
    cmds.move(0.8, 0, -2, r = True)
    
    cmds.select(["ik_foot_anim_r.cv[*]"])
    cmds.scale(1.4, 1.4, 1.4, r = True, ocp = False)
    cmds.move(-0.8, 0, -2, r = True)

    cmds.select(["eyebrow_mid_anim.cv[*]"])
    cmds.scale(0.117744, 0.117744, 0.117744, r = True, ocp = True)

    cmds.select(["hose_dyn_anim.cv[*]"])
    cmds.scale(0.3, 0.3, 0.3, r = True, ocp = True)

    cmds.select(["generator_base_anim.cv[*]"])
    cmds.scale(1.5, 1.5, 1.5, r = True, ocp = True)

    cmds.select(["generator_fan_anim.cv[*]"])
    cmds.scale(1, 2, 2, r = True, ocp = True)

    cmds.select(None)

scaleControls()

# ------------------------------------------------------------------
def generatorRig():
    import Tools.ART_addSpaceSwitching as space
    reload(space)

    #cmds.xform(q=True, ws=True, rp=True)
    #cmds.xform(q=True, ro=True)

    generator_handle_l = cmds.group(em=True, name="generator_handle_l")
    cmds.xform(generator_handle_l, ws=True, t=[16.44044082967427, -14.782081604003906, 44.71940612792969])
    cmds.parent(generator_handle_l, "ctrl_rig")
    cmds.parentConstraint("generator_fan_anim", generator_handle_l, sr=["x", "y", "z"], mo=True)

    generator_handle_r = cmds.group(em=True, name="generator_handle_r")
    cmds.xform(generator_handle_r, ws=True, t=[-16.44044082967427, -27.95550537109375, 44.71940612792969])
    cmds.parent(generator_handle_r, "ctrl_rig")
    cmds.parentConstraint("generator_fan_anim", generator_handle_r, sr=["x", "y", "z"], mo=True)

    space.addSpaceSwitching("ik_wrist_l_anim", "generator_handle_l", True)
    space.addSpaceSwitching("ik_wrist_r_anim", "generator_handle_r", True)
    
    # Place the hands on the generator
    cmds.xform("ik_elbow_l_anim", ws=True, t=[28.889416847590216, 12.120787176539988, 37.228147086712276])
    cmds.xform("ik_wrist_l_anim", ws=True, t=[17.738075812323117, -9.968905939784053, 47.72013535089419])
    cmds.xform("ik_wrist_l_anim", ro=[-0.46996602627768336, 1.9568351642166377, -112.63950206642606])
    cmds.xform("thumb_finger_fk_ctrl_1_l", ro=[12.075687159499754, 33.10338499825681, -5.23485186300789])
    cmds.xform("thumb_finger_fk_ctrl_2_l", ro=[0.0, 0.0, 41.779196684726855])
    cmds.xform("thumb_finger_fk_ctrl_3_l", ro=[0.0, 0.0, 41.779196684726855])
    cmds.xform("index_finger_fk_ctrl_1_l", ro=[-9.24319208128754, -7.908051417286156, 40.30188751921624])
    cmds.xform("index_finger_fk_ctrl_2_l", ro=[0.0, 0.0, 44.66026243565067])
    cmds.xform("index_finger_fk_ctrl_3_l", ro=[0.0, 0.0, 44.66026243565067])
    cmds.xform("middle_finger_fk_ctrl_1_l", ro=[-6.688554300411708, 0.7306575322531735, 68.79939049033274])
    cmds.xform("middle_finger_fk_ctrl_2_l", ro=[0.0, 0.0, 44.66026243565067])
    cmds.xform("middle_finger_fk_ctrl_3_l", ro=[0.0, 0.0, 44.66026243565067])
    cmds.xform("ring_finger_fk_ctrl_1_l", ro=[0.0, 0.0, 65.7073020190219])
    cmds.xform("ring_finger_fk_ctrl_2_l", ro=[0.0, 0.0, 33.707222572016974])
    cmds.xform("ring_finger_fk_ctrl_3_l", ro=[0.0, 0.0, 44.66026243565067])
    
    cmds.xform("ik_elbow_r_anim", ws=True, t=[-28.889412928660853, 12.120864738790573, 37.22729369223536])
    cmds.xform("ik_wrist_r_anim", ws=True, t=[-17.738119585719378, -23.10619865611388, 47.72015503747302])
    cmds.xform("ik_wrist_r_anim", ro=[-0.46996602627768336, -1.95684, 112.6395])
    cmds.xform("thumb_finger_fk_ctrl_1_r", ro=[12.075687159499754, 33.10338499825681, -5.23485186300789])
    cmds.xform("thumb_finger_fk_ctrl_2_r", ro=[0.0, 0.0, 41.779196684726855])
    cmds.xform("thumb_finger_fk_ctrl_3_r", ro=[0.0, 0.0, 41.779196684726855])
    cmds.xform("index_finger_fk_ctrl_1_r", ro=[-9.24319208128754, -7.908051417286156, 40.30188751921624])
    cmds.xform("index_finger_fk_ctrl_2_r", ro=[0.0, 0.0, 44.66026243565067])
    cmds.xform("index_finger_fk_ctrl_3_r", ro=[0.0, 0.0, 44.66026243565067])
    cmds.xform("middle_finger_fk_ctrl_1_r", ro=[-6.688554300411708, 0.7306575322531735, 68.79939049033274])
    cmds.xform("middle_finger_fk_ctrl_2_r", ro=[0.0, 0.0, 44.66026243565067])
    cmds.xform("middle_finger_fk_ctrl_3_r", ro=[0.0, 0.0, 44.66026243565067])
    cmds.xform("ring_finger_fk_ctrl_1_r", ro=[0.0, 0.0, 65.7073020190219])
    cmds.xform("ring_finger_fk_ctrl_2_r", ro=[0.0, 0.0, 33.707222572016974])
    cmds.xform("ring_finger_fk_ctrl_3_r", ro=[0.0, 0.0, 44.66026243565067])

generatorRig()

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
# Create selection sets for the Goblin since the animation UI will not be able to find him.
def createSelectionSets():
    cmds.select("head_fk_anim", "neck_01_fk_anim", "spine_01_anim", "spine_02_anim", "spine_03_anim", "mid_ik_anim", "chest_ik_anim", "body_anim", "hip_anim", "clavicle_l_anim", "clavicle_r_anim", "fk_arm_l_anim", "fk_arm_r_anim", "fk_elbow_l_anim", "fk_elbow_r_anim", "fk_wrist_l_anim", "fk_wrist_r_anim", "ik_elbow_l_anim", "ik_elbow_r_anim", "ik_wrist_l_anim", "ik_wrist_r_anim", "fk_thigh_l_anim", "fk_thigh_r_anim", "fk_calf_l_anim", "fk_calf_r_anim", "fk_foot_l_anim", "fk_foot_r_anim", "ik_foot_anim_l", "ik_foot_anim_r", "heel_ctrl_l", "heel_ctrl_r", "toe_wiggle_ctrl_l", "toe_wiggle_ctrl_r", "toe_tip_ctrl_l", "toe_tip_ctrl_r", "master_anim", "offset_anim", "root_anim", "upperarm_l_twist_anim", "upperarm_l_twist_2_anim", "upperarm_r_twist_anim", "upperarm_r_twist_2_anim", "l_thigh_twist_01_anim", "r_thigh_twist_01_anim", "ring_finger_fk_ctrl_1_l", "ring_finger_fk_ctrl_1_r", "ring_finger_fk_ctrl_2_l", "ring_finger_fk_ctrl_2_r", "ring_finger_fk_ctrl_3_l", "ring_finger_fk_ctrl_3_r", "middle_finger_fk_ctrl_1_l", "middle_finger_fk_ctrl_1_r", "middle_finger_fk_ctrl_2_l", "middle_finger_fk_ctrl_2_r", "middle_finger_fk_ctrl_3_l", "middle_finger_fk_ctrl_3_r", "index_finger_fk_ctrl_1_l", "index_finger_fk_ctrl_1_r", "index_finger_fk_ctrl_2_l", "index_finger_fk_ctrl_2_r", "index_finger_fk_ctrl_3_l", "index_finger_fk_ctrl_3_r", "thumb_finger_fk_ctrl_1_l", "thumb_finger_fk_ctrl_1_r", "thumb_finger_fk_ctrl_2_l", "thumb_finger_fk_ctrl_2_r", "thumb_finger_fk_ctrl_3_l", "thumb_finger_fk_ctrl_3_r", "index_l_ik_anim", "index_r_ik_anim", "middle_l_ik_anim", "middle_r_ik_anim", "ring_l_ik_anim", "ring_r_ik_anim", "thumb_l_ik_anim", "thumb_r_ik_anim", "index_l_poleVector", "index_r_poleVector", "middle_l_poleVector", "middle_r_poleVector", "ring_l_poleVector", "ring_r_poleVector", "thumb_l_poleVector", "thumb_r_poleVector", "l_global_ik_anim", "r_global_ik_anim", "lowerarm_l_twist_anim", "lowerarm_r_twist_anim", "fk_clavicle_l_anim", "fk_clavicle_r_anim", "big_toe_l_anim", "big_toe_r_anim", "ear_l_anim", "ear_r_anim", "eyeball_l_anim", "eyeball_r_anim", "eyebrow_in_l_anim", "eyebrow_in_r_anim", "eyebrow_mid_anim", "eyebrow_out_l_anim", "eyebrow_out_r_anim", "eyelid_bott_l_anim", "eyelid_bott_r_anim", "eyelid_top_l_anim", "eyelid_top_r_anim", "generator_base_anim", "generator_fan_anim", "hose_dyn_anim", "jaw_anim", "mid_toe_l_anim", "mid_toe_r_anim", "pinky_toe_l_anim", "pinky_toe_r_anim", "wing_bott_l_anim", "wing_bott_r_anim", "wing_top_l_anim", "wing_top_r_anim", "Rig_Settings", r=True)
    cmds.sets(name="All_Controls")

    cmds.select("head_fk_anim", "neck_01_fk_anim", "spine_01_anim", "spine_02_anim", "spine_03_anim", "mid_ik_anim", "chest_ik_anim", "body_anim", "hip_anim", "clavicle_l_anim", "clavicle_r_anim", "fk_arm_l_anim", "fk_arm_r_anim", "fk_elbow_l_anim", "fk_elbow_r_anim", "fk_wrist_l_anim", "fk_wrist_r_anim", "ik_elbow_l_anim", "ik_elbow_r_anim", "ik_wrist_l_anim", "ik_wrist_r_anim", "fk_thigh_l_anim", "fk_thigh_r_anim", "fk_calf_l_anim", "fk_calf_r_anim", "fk_foot_l_anim", "fk_foot_r_anim", "ik_foot_anim_l", "ik_foot_anim_r", "heel_ctrl_l", "heel_ctrl_r", "toe_wiggle_ctrl_l", "toe_wiggle_ctrl_r", "toe_tip_ctrl_l", "toe_tip_ctrl_r", "master_anim", "offset_anim", "root_anim", "upperarm_l_twist_anim", "upperarm_l_twist_2_anim", "upperarm_r_twist_anim", "upperarm_r_twist_2_anim", "l_thigh_twist_01_anim", "r_thigh_twist_01_anim", "ring_finger_fk_ctrl_1_l", "ring_finger_fk_ctrl_1_r", "ring_finger_fk_ctrl_2_l", "ring_finger_fk_ctrl_2_r", "ring_finger_fk_ctrl_3_l", "ring_finger_fk_ctrl_3_r", "middle_finger_fk_ctrl_1_l", "middle_finger_fk_ctrl_1_r", "middle_finger_fk_ctrl_2_l", "middle_finger_fk_ctrl_2_r", "middle_finger_fk_ctrl_3_l", "middle_finger_fk_ctrl_3_r", "index_finger_fk_ctrl_1_l", "index_finger_fk_ctrl_1_r", "index_finger_fk_ctrl_2_l", "index_finger_fk_ctrl_2_r", "index_finger_fk_ctrl_3_l", "index_finger_fk_ctrl_3_r", "thumb_finger_fk_ctrl_1_l", "thumb_finger_fk_ctrl_1_r", "thumb_finger_fk_ctrl_2_l", "thumb_finger_fk_ctrl_2_r", "thumb_finger_fk_ctrl_3_l", "thumb_finger_fk_ctrl_3_r", "index_l_ik_anim", "index_r_ik_anim", "middle_l_ik_anim", "middle_r_ik_anim", "ring_l_ik_anim", "ring_r_ik_anim", "thumb_l_ik_anim", "thumb_r_ik_anim", "index_l_poleVector", "index_r_poleVector", "middle_l_poleVector", "middle_r_poleVector", "ring_l_poleVector", "ring_r_poleVector", "thumb_l_poleVector", "thumb_r_poleVector", "l_global_ik_anim", "r_global_ik_anim", "lowerarm_l_twist_anim", "lowerarm_r_twist_anim", "fk_clavicle_l_anim", "fk_clavicle_r_anim", "big_toe_l_anim", "big_toe_r_anim", "ear_l_anim", "ear_r_anim", "eyeball_l_anim", "eyeball_r_anim", "eyebrow_in_l_anim", "eyebrow_in_r_anim", "eyebrow_mid_anim", "eyebrow_out_l_anim", "eyebrow_out_r_anim", "eyelid_bott_l_anim", "eyelid_bott_r_anim", "eyelid_top_l_anim", "eyelid_top_r_anim", "generator_base_anim", "generator_fan_anim", "hose_dyn_anim", "jaw_anim", "mid_toe_l_anim", "mid_toe_r_anim", "pinky_toe_l_anim", "pinky_toe_r_anim", "wing_bott_l_anim", "wing_bott_r_anim", "wing_top_l_anim", "wing_top_r_anim", "Rig_Settings", "fk_orient_world_loc_l", "fk_orient_world_loc_r", "fk_orient_body_loc_l", "fk_orient_body_loc_r", "head_fk_orient_neck", "head_fk_orient_shoulder", "head_fk_orient_body", "head_fk_orient_world", "big_toe_l_anim_space_switcher", "big_toe_r_anim_space_switcher", "body_anim_space_switcher", "chest_ik_anim_space_switcher", "ear_l_anim_space_switcher", "ear_r_anim_space_switcher", "eyeball_l_anim_space_switcher", "eyeball_r_anim_space_switcher", "eyebrow_in_l_anim_space_switcher", "eyebrow_in_r_anim_space_switcher", "eyebrow_mid_anim_space_switcher", "eyebrow_out_l_anim_space_switcher", "eyebrow_out_r_anim_space_switcher", "eyelid_bott_l_anim_space_switcher", "eyelid_bott_r_anim_space_switcher", "eyelid_top_l_anim_space_switcher", "eyelid_top_r_anim_space_switcher", "generator_base_anim_space_switcher", "generator_fan_anim_space_switcher", "hose_dyn_anim_space_switcher", "ik_elbow_l_anim_space_switcher", "ik_elbow_r_anim_space_switcher", "ik_foot_anim_l_space_switcher", "ik_foot_anim_r_space_switcher", "ik_wrist_l_anim_space_switcher", "ik_wrist_r_anim_space_switcher", "jaw_anim_space_switcher", "l_global_ik_anim_space_switcher", "master_anim_space_switcher", "mid_toe_l_anim_space_switcher", "mid_toe_r_anim_space_switcher", "pinky_toe_l_anim_space_switcher", "pinky_toe_r_anim_space_switcher", "r_global_ik_anim_space_switcher", "spine_01_space_switcher", "wing_bott_l_anim_space_switcher", "wing_bott_r_anim_space_switcher", "wing_top_l_anim_space_switcher", "wing_top_r_anim_space_switcher", r=True) 
    cmds.sets(name="All_Controls_and_Spaces")
createSelectionSets()





































