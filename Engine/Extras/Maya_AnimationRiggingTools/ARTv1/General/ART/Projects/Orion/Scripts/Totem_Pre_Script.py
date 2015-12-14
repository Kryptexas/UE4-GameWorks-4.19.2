import maya.cmds as cmds

if cmds.objExists("ik_foot_root"):
	cmds.delete("ik_foot_root")
	
if cmds.objExists("ik_hand_root"):
	cmds.delete("ik_hand_root")
	

def hookupShoulerLocs():
        
    cmds.parentConstraint("upperarm_l", "upperarm_l_loc", mo = True)
    cmds.parentConstraint("clavicle_l", "clavicle_l_loc", mo = True)
    cmds.parentConstraint("spine_03", "upperarm_l_up_loc", mo = True)
    cmds.parentConstraint("spine_03", "upperarm_l_forward_loc", mo = True)
    cmds.parentConstraint("spine_03", "upperarm_l_backward_loc", mo = True)
    
    cmds.parentConstraint("lowerarm_l", "lowerarm_l_loc")
    cmds.parentConstraint("clavicle_l", "lowerarm_l_down_loc", mo = True)
    
    cmds.parentConstraint("lowerarm_r", "lowerarm_r_loc")
    cmds.parentConstraint("clavicle_r", "lowerarm_r_down_loc", mo = True)
    
    cmds.parentConstraint("upperarm_r", "upperarm_r_loc", mo = True)
    cmds.parentConstraint("clavicle_r", "clavicle_r_loc", mo = True)
    cmds.parentConstraint("spine_03", "upperarm_r_up_loc", mo = True)
    cmds.parentConstraint("spine_03", "upperarm_r_forward_loc", mo = True)
    cmds.parentConstraint("spine_03", "upperarm_r_backward_loc", mo = True)
    
    
    
def hookupLegCorrectives():
        
    cmds.parentConstraint("calf_l", "calf_l_loc", mo = True)
    cmds.parentConstraint("thigh_l", "thigh_l_loc", mo = True)
    cmds.parentConstraint("pelvis", "calf_l_forward_loc", mo = True)
    cmds.parentConstraint("pelvis", "calf_l_backward_loc", mo = True)
    cmds.parentConstraint("foot_l", "foot_l_loc", mo = True)
    
    cmds.parentConstraint("calf_r", "calf_r_loc", mo = True)
    cmds.parentConstraint("thigh_r", "thigh_r_loc", mo = True)
    cmds.parentConstraint("pelvis", "calf_r_forward_loc", mo = True)
    cmds.parentConstraint("pelvis", "calf_r_backward_loc", mo = True)
    cmds.parentConstraint("foot_r", "foot_r_loc", mo = True)
    
    cmds.setAttr("thigh_l.rotateZ", 0)
    cmds.setAttr("tri_leg_correctives.l_leg_forward", 0)
    cmds.setDrivenKeyframe("tri_leg_correctives.l_leg_forward", cd = "angle_dimensions.lForAngle")
    
    cmds.setAttr("thigh_l.rotateZ", 57)
    cmds.setAttr("tri_leg_correctives.l_leg_forward", 1)
    cmds.setDrivenKeyframe("tri_leg_correctives.l_leg_forward", cd = "angle_dimensions.lForAngle")
    cmds.setAttr("thigh_l.rotateZ", 0)
    
    
    cmds.setAttr("thigh_l.rotateZ", 0)
    cmds.setAttr("tri_leg_correctives.left_butt_scrunch_corrective", 0)
    cmds.setDrivenKeyframe("tri_leg_correctives.left_butt_scrunch_corrective", cd = "angle_dimensions.lBacAngle")
    
    cmds.setAttr("thigh_l.rotateZ", -35)
    cmds.setAttr("tri_leg_correctives.left_butt_scrunch_corrective", 1)
    cmds.setDrivenKeyframe("tri_leg_correctives.left_butt_scrunch_corrective", cd = "angle_dimensions.lBacAngle")
    cmds.setAttr("thigh_l.rotateZ", 0)
    
    
    
    cmds.setAttr("thigh_r.rotateZ", 0)
    cmds.setAttr("tri_leg_correctives.r_leg_forward", 0)
    cmds.setDrivenKeyframe("tri_leg_correctives.r_leg_forward", cd = "angle_dimensions.rForAngle")
    
    cmds.setAttr("thigh_r.rotateZ", 57)
    cmds.setAttr("tri_leg_correctives.r_leg_forward", 1)
    cmds.setDrivenKeyframe("tri_leg_correctives.r_leg_forward", cd = "angle_dimensions.rForAngle")
    cmds.setAttr("thigh_r.rotateZ", 0)
    
    
    cmds.setAttr("thigh_r.rotateZ", 0)
    cmds.setAttr("tri_leg_correctives.right_butt_scrunch_corrective", 0)
    cmds.setDrivenKeyframe("tri_leg_correctives.right_butt_scrunch_corrective", cd = "angle_dimensions.rBacAngle")
    
    cmds.setAttr("thigh_r.rotateZ", -35)
    cmds.setAttr("tri_leg_correctives.right_butt_scrunch_corrective", 1)
    cmds.setDrivenKeyframe("tri_leg_correctives.right_butt_scrunch_corrective", cd = "angle_dimensions.rBacAngle")
    cmds.setAttr("thigh_r.rotateZ", 0)
    
#hookupLegCorrectives()
hookupShoulerLocs()