import maya.cmds as cmds
import maya.mel as mel
import math

# ------------------------------------------------------------------
def scaleControls():
    
    # eyes
    for side in ["_l", "_r"]:
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
    
        cmds.select(["heel"+side+"_anim.cv[0:32]"])
        cmds.scale(0.34, 0.34, 0.34, r = True, ocp = True)

    # jaw
    cmds.select(["jaw_anim.cv[0:32]"])
    cmds.scale(0.294922, 0.294922, 0.294922, r = True, ocp = True)
    cmds.move(0, -12, -10, r = True)
    
    # ears
    cmds.select(["fk_earMain_l_01_anim.cv[0:7]", "fk_earMain_r_01_anim.cv[0:7]"])
    cmds.scale(1.522128, 1.522128, 1.522128, r = True, ocp = False)

    cmds.select(["fk_earMain_l_03_anim.cv[0:7]", "fk_earMain_r_03_anim.cv[0:7]"])
    cmds.scale(0.696643, 0.696643, 0.696643, r = True, ocp = False)
    
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
    cmds.move(0.8, 0, -2, r = True)
    
    cmds.select(["ik_foot_anim_r.cv[0:17]"])
    cmds.scale(1.4, 1.4, 1.4, r = True, ocp = False)
    cmds.move(-0.8, 0, -2, r = True)

    cmds.select(None)

scaleControls()

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
            cmds.setAttr(PVConst+".offsetX", -200)
        else:
            cmds.setAttr(PVConst+".offsetX", 200)

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