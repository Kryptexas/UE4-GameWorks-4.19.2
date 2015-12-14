import maya.cmds as cmds

#Hide controls that will be driven
controlsToHide = ["thruster_a_l_anim", "thruster_b_l_anim", "thruster_c_l_anim", "thruster_a_r_anim", "thruster_b_r_anim", "thruster_c_r_anim",
"thruster_casing_l_anim", "thruster_casing_r_anim", "dagger_a_r_anim", "dagger_b_r_anim", "dagger_c_r_anim",
"dagger_a_l_anim", "dagger_b_l_anim", "dagger_c_l_anim", "dagger_base_l_anim", "dagger_base_r_anim", "pivotA_r_anim", "pivotB_r_anim",
"extensionA_r_anim", "extensionB_r_anim", "pivotA_l_anim", "pivotB_l_anim", "extensionA_l_anim", "extensionB_l_anim", "lowerArm_bottomPlateA_r_anim",
"lowerArm_bottomPlateB_r_anim", "lowerArm_topPlateB_r_anim", "lowerArm_topPlateA_r_anim", "lowerArm_bottomPlateA_l_anim", "lowerArm_bottomPlateB_l_anim",
"lowerArm_topPlateB_l_anim", "lowerArm_topPlateA_l_anim"]

specialControls = ["thrusterVent_l_master_ctrl_grp", "thrusterVent_r_master_ctrl_grp"]

for control in controlsToHide:
	grp = control + "_grp"
	cmds.setAttr(grp + ".visibility", 0)
	
for control in specialControls:
	cmds.setAttr(control + ".visibility", 0)
	
#scale weapon and sword root controls
weaponAnimControls = ["weapon_r_anim", "weapon_l_anim"]
swordControls = ["sword_root_r_anim", "sword_root_l_anim"]

for control in weaponAnimControls:
	cmds.select(control + ".cv[0:32]")
	cmds.scale(0, 1.6, 1.6, r = True)

for control in swordControls:
	cmds.select(control + ".cv[0:32]")
	cmds.scale(0, 1.2, 1.2, r = True)
	

#ADD CUSTOM ATTRS FOR TRANSFORMATIONS

#thrusters
cmds.addAttr("body_anim", ln = "l_vent_open", dv = 0, min = 0, max = 10, keyable = True)
cmds.addAttr("body_anim", ln = "r_vent_open", dv = 0, min = 0, max = 10, keyable = True)
cmds.addAttr("body_anim", ln = "l_unit_deploy", dv = 0, min = 0, max = 10, keyable = True)
cmds.addAttr("body_anim", ln = "r_unit_deploy", dv = 0, min = 0, max = 10, keyable = True)

cmds.addAttr("body_anim", ln = "l_ventA_deploy", dv = 0, min = 0, max = 1, keyable = True)
cmds.addAttr("body_anim", ln = "l_ventB_deploy", dv = 0, min = 0, max = 1, keyable = True)
cmds.addAttr("body_anim", ln = "l_ventC_deploy", dv = 0, min = 0, max = 1, keyable = True)

cmds.addAttr("body_anim", ln = "r_ventA_deploy", dv = 0, min = 0, max = 1, keyable = True)
cmds.addAttr("body_anim", ln = "r_ventB_deploy", dv = 0, min = 0, max = 1, keyable = True)
cmds.addAttr("body_anim", ln = "r_ventC_deploy", dv = 0, min = 0, max = 1, keyable = True)



#left arm
cmds.addAttr("weapon_l_anim", ln = "top_open", dv = 0, min = 0, max = 10, keyable = True)
cmds.addAttr("weapon_l_anim", ln = "bottom_open", dv = 0, min = 0, max = 10, keyable = True)

cmds.addAttr("weapon_l_anim", ln = "daggers_deploy", dv = 0, min = 0, max = 10, keyable = True)
cmds.addAttr("weapon_l_anim", ln = "daggerA_swing", dv = 0, keyable = True)
cmds.addAttr("weapon_l_anim", ln = "daggerB_swing", dv = 0, keyable = True)
cmds.addAttr("weapon_l_anim", ln = "daggerC_swing", dv = 0, keyable = True)

#right arm
cmds.addAttr("weapon_r_anim", ln = "top_open", dv = 0, min = 0, max = 10, keyable = True)
cmds.addAttr("weapon_r_anim", ln = "bottom_open", dv = 0, min = 0, max = 10, keyable = True)

cmds.addAttr("weapon_r_anim", ln = "daggers_deploy", dv = 0, min = 0, max = 10, keyable = True)
cmds.addAttr("weapon_r_anim", ln = "daggerA_swing", dv = 0, keyable = True)
cmds.addAttr("weapon_r_anim", ln = "daggerB_swing", dv = 0, keyable = True)
cmds.addAttr("weapon_r_anim", ln = "daggerC_swing", dv = 0, keyable = True)

#left sword
cmds.addAttr("sword_root_l_anim", ln = "sword_deploy", dv = 0, min = 0, max = 10, keyable = True)
cmds.addAttr("sword_root_l_anim", ln = "sword_open", dv = 0, min = 0, max = 10, keyable = True)
cmds.addAttr("sword_root_l_anim", ln = "sword_extend", dv = 0, min = 0, max = 10, keyable = True)

#right sword
cmds.addAttr("sword_root_r_anim", ln = "sword_deploy", dv = 0, min = 0, max = 10, keyable = True)
cmds.addAttr("sword_root_r_anim", ln = "sword_open", dv = 0, min = 0, max = 10, keyable = True)
cmds.addAttr("sword_root_r_anim", ln = "sword_extend", dv = 0, min = 0, max = 10, keyable = True)


#SET DRIVEN KEYFRAMES

#LEFT VENT OPEN
cmds.setDrivenKeyframe(["fk_thrusterVent_l_01_lag_grp.tx", "fk_thrusterVent_l_01_lag_grp.ty", "fk_thrusterVent_l_01_lag_grp.tz", "fk_thrusterVent_l_01_lag_grp.rx", "fk_thrusterVent_l_01_lag_grp.ry", "fk_thrusterVent_l_01_lag_grp.rz"], cd = "body_anim.l_vent_open")
cmds.setDrivenKeyframe(["fk_thrusterVent_l_02_lag_grp.tx", "fk_thrusterVent_l_02_lag_grp.ty", "fk_thrusterVent_l_02_lag_grp.tz", "fk_thrusterVent_l_02_lag_grp.rx", "fk_thrusterVent_l_02_lag_grp.ry", "fk_thrusterVent_l_02_lag_grp.rz"], cd = "body_anim.l_vent_open")

cmds.setAttr("body_anim.l_vent_open", 5)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.tx", 0*0.65)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.ty", .152*0.65)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.tz", -.09*0.65)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.rx", 5.271)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.ry", -.15)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.rz", -19.4)

cmds.setAttr("fk_thrusterVent_l_02_lag_grp.tx", -.653*0.65)
cmds.setAttr("fk_thrusterVent_l_02_lag_grp.ty", 4.404*0.65)
cmds.setAttr("fk_thrusterVent_l_02_lag_grp.tz", -.098*0.65)
cmds.setAttr("fk_thrusterVent_l_02_lag_grp.rx", -4.033)
cmds.setAttr("fk_thrusterVent_l_02_lag_grp.ry", -2.044)
cmds.setAttr("fk_thrusterVent_l_02_lag_grp.rz", -19.602)
cmds.setDrivenKeyframe(["fk_thrusterVent_l_01_lag_grp.tx", "fk_thrusterVent_l_01_lag_grp.ty", "fk_thrusterVent_l_01_lag_grp.tz", "fk_thrusterVent_l_01_lag_grp.rx", "fk_thrusterVent_l_01_lag_grp.ry", "fk_thrusterVent_l_01_lag_grp.rz"], cd = "body_anim.l_vent_open")
cmds.setDrivenKeyframe(["fk_thrusterVent_l_02_lag_grp.tx", "fk_thrusterVent_l_02_lag_grp.ty", "fk_thrusterVent_l_02_lag_grp.tz", "fk_thrusterVent_l_02_lag_grp.rx", "fk_thrusterVent_l_02_lag_grp.ry", "fk_thrusterVent_l_02_lag_grp.rz"], cd = "body_anim.l_vent_open")


cmds.setAttr("body_anim.l_vent_open", 10)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.tx", 0*0.65)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.ty", .152*0.65)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.tz", -.09*0.65)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.rx", 15.093)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.ry", -.37)
cmds.setAttr("fk_thrusterVent_l_01_lag_grp.rz", -55.082)

cmds.setAttr("fk_thrusterVent_l_02_lag_grp.tx", -4.068*0.65)
cmds.setAttr("fk_thrusterVent_l_02_lag_grp.ty", 5.659*0.65)
cmds.setAttr("fk_thrusterVent_l_02_lag_grp.tz", .577*0.65)
cmds.setAttr("fk_thrusterVent_l_02_lag_grp.rx", -10.697)
cmds.setAttr("fk_thrusterVent_l_02_lag_grp.ry", -1.89)
cmds.setAttr("fk_thrusterVent_l_02_lag_grp.rz", -15.521)
cmds.setDrivenKeyframe(["fk_thrusterVent_l_01_lag_grp.tx", "fk_thrusterVent_l_01_lag_grp.ty", "fk_thrusterVent_l_01_lag_grp.tz", "fk_thrusterVent_l_01_lag_grp.rx", "fk_thrusterVent_l_01_lag_grp.ry", "fk_thrusterVent_l_01_lag_grp.rz"], cd = "body_anim.l_vent_open")
cmds.setDrivenKeyframe(["fk_thrusterVent_l_02_lag_grp.tx", "fk_thrusterVent_l_02_lag_grp.ty", "fk_thrusterVent_l_02_lag_grp.tz", "fk_thrusterVent_l_02_lag_grp.rx", "fk_thrusterVent_l_02_lag_grp.ry", "fk_thrusterVent_l_02_lag_grp.rz"], cd = "body_anim.l_vent_open")

cmds.setAttr("body_anim.l_vent_open", 0)



#RIGHT VENT OPEN
cmds.setDrivenKeyframe(["fk_thrusterVent_r_01_lag_grp.tx", "fk_thrusterVent_r_01_lag_grp.ty", "fk_thrusterVent_r_01_lag_grp.tz", "fk_thrusterVent_r_01_lag_grp.rx", "fk_thrusterVent_r_01_lag_grp.ry", "fk_thrusterVent_r_01_lag_grp.rz"], cd = "body_anim.r_vent_open")
cmds.setDrivenKeyframe(["fk_thrusterVent_r_02_lag_grp.tx", "fk_thrusterVent_r_02_lag_grp.ty", "fk_thrusterVent_r_02_lag_grp.tz", "fk_thrusterVent_r_02_lag_grp.rx", "fk_thrusterVent_r_02_lag_grp.ry", "fk_thrusterVent_r_02_lag_grp.rz"], cd = "body_anim.r_vent_open")

cmds.setAttr("body_anim.r_vent_open", 5)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.tx", 0*0.65)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.ty", -.152*0.65)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.tz", .09*0.65)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.rx", 5.271)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.ry", -.15)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.rz", -19.4)

cmds.setAttr("fk_thrusterVent_r_02_lag_grp.tx", .653*0.65)
cmds.setAttr("fk_thrusterVent_r_02_lag_grp.ty", -4.404*0.65)
cmds.setAttr("fk_thrusterVent_r_02_lag_grp.tz", -.098*0.65)
cmds.setAttr("fk_thrusterVent_r_02_lag_grp.rx", -4.033)
cmds.setAttr("fk_thrusterVent_r_02_lag_grp.ry", -2.044)
cmds.setAttr("fk_thrusterVent_r_02_lag_grp.rz", -19.602)
cmds.setDrivenKeyframe(["fk_thrusterVent_r_01_lag_grp.tx", "fk_thrusterVent_r_01_lag_grp.ty", "fk_thrusterVent_r_01_lag_grp.tz", "fk_thrusterVent_r_01_lag_grp.rx", "fk_thrusterVent_r_01_lag_grp.ry", "fk_thrusterVent_r_01_lag_grp.rz"], cd = "body_anim.r_vent_open")
cmds.setDrivenKeyframe(["fk_thrusterVent_r_02_lag_grp.tx", "fk_thrusterVent_r_02_lag_grp.ty", "fk_thrusterVent_r_02_lag_grp.tz", "fk_thrusterVent_r_02_lag_grp.rx", "fk_thrusterVent_r_02_lag_grp.ry", "fk_thrusterVent_r_02_lag_grp.rz"], cd = "body_anim.r_vent_open")

cmds.setAttr("body_anim.r_vent_open", 10)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.tx", 0*0.65)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.ty", -.152*0.65)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.tz", .09*0.65)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.rx", 15.093)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.ry", -.37)
cmds.setAttr("fk_thrusterVent_r_01_lag_grp.rz", -55.082)

cmds.setAttr("fk_thrusterVent_r_02_lag_grp.tx", 4.068*0.65)
cmds.setAttr("fk_thrusterVent_r_02_lag_grp.ty", -5.659*0.65)
cmds.setAttr("fk_thrusterVent_r_02_lag_grp.tz", -.577*0.65)
cmds.setAttr("fk_thrusterVent_r_02_lag_grp.rx", -10.697)
cmds.setAttr("fk_thrusterVent_r_02_lag_grp.ry", -1.89)
cmds.setAttr("fk_thrusterVent_r_02_lag_grp.rz", -15.521)
cmds.setDrivenKeyframe(["fk_thrusterVent_r_01_lag_grp.tx", "fk_thrusterVent_r_01_lag_grp.ty", "fk_thrusterVent_r_01_lag_grp.tz", "fk_thrusterVent_r_01_lag_grp.rx", "fk_thrusterVent_r_01_lag_grp.ry", "fk_thrusterVent_r_01_lag_grp.rz"], cd = "body_anim.r_vent_open")
cmds.setDrivenKeyframe(["fk_thrusterVent_r_02_lag_grp.tx", "fk_thrusterVent_r_02_lag_grp.ty", "fk_thrusterVent_r_02_lag_grp.tz", "fk_thrusterVent_r_02_lag_grp.rx", "fk_thrusterVent_r_02_lag_grp.ry", "fk_thrusterVent_r_02_lag_grp.rz"], cd = "body_anim.r_vent_open")
cmds.setAttr("body_anim.r_vent_open", 0)


#LEFT UNIT DEPLOY
cmds.setAttr("body_anim.l_unit_deploy", 0)
cmds.setAttr("thruster_casing_l_anim_grp.tx", 0*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.ty", -9.046*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.tz", -1.065*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.rx", 95.625)
cmds.setAttr("thruster_casing_l_anim_grp.ry", 0)
cmds.setAttr("thruster_casing_l_anim_grp.rz", 0)
cmds.setAttr("thruster_casing_l_anim_grp.sx", .398)
cmds.setAttr("thruster_casing_l_anim_grp.sy", .398)
cmds.setAttr("thruster_casing_l_anim_grp.sz", .398)
cmds.setDrivenKeyframe(["thruster_casing_l_anim_grp.tx", "thruster_casing_l_anim_grp.ty", "thruster_casing_l_anim_grp.tz", "thruster_casing_l_anim_grp.rx", "thruster_casing_l_anim_grp.ry", "thruster_casing_l_anim_grp.rz", "thruster_casing_l_anim_grp.sx", "thruster_casing_l_anim_grp.sy", "thruster_casing_l_anim_grp.sz"], cd = "body_anim.l_unit_deploy",itt = "flat", ott = "flat")

cmds.setAttr("body_anim.l_unit_deploy", 3.5)
cmds.setAttr("thruster_casing_l_anim_grp.tx", 0*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.ty", .179*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.tz", -.157*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.rx", 95.625)
cmds.setAttr("thruster_casing_l_anim_grp.ry", 0)
cmds.setAttr("thruster_casing_l_anim_grp.rz", 0)
cmds.setAttr("thruster_casing_l_anim_grp.sx", .8)
cmds.setAttr("thruster_casing_l_anim_grp.sy", 1)
cmds.setAttr("thruster_casing_l_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_casing_l_anim_grp.tx", "thruster_casing_l_anim_grp.ty", "thruster_casing_l_anim_grp.tz", "thruster_casing_l_anim_grp.rx", "thruster_casing_l_anim_grp.ry", "thruster_casing_l_anim_grp.rz", "thruster_casing_l_anim_grp.sx", "thruster_casing_l_anim_grp.sy", "thruster_casing_l_anim_grp.sz"], cd = "body_anim.l_unit_deploy",itt = "flat", ott = "flat")

cmds.setAttr("body_anim.l_unit_deploy", 6.5)
cmds.setAttr("thruster_casing_l_anim_grp.tx", 0*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.ty", .179*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.tz", -.157*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.rx", 53.119)
cmds.setAttr("thruster_casing_l_anim_grp.ry", 0)
cmds.setAttr("thruster_casing_l_anim_grp.rz", 0)
cmds.setAttr("thruster_casing_l_anim_grp.sx", .933)
cmds.setAttr("thruster_casing_l_anim_grp.sy", 1)
cmds.setAttr("thruster_casing_l_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_casing_l_anim_grp.tx", "thruster_casing_l_anim_grp.ty", "thruster_casing_l_anim_grp.tz", "thruster_casing_l_anim_grp.rx", "thruster_casing_l_anim_grp.ry", "thruster_casing_l_anim_grp.rz", "thruster_casing_l_anim_grp.sx", "thruster_casing_l_anim_grp.sy", "thruster_casing_l_anim_grp.sz"], cd = "body_anim.l_unit_deploy",itt = "flat", ott = "flat")

cmds.setAttr("body_anim.l_unit_deploy", 10)
cmds.setAttr("thruster_casing_l_anim_grp.tx", 0*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.ty", .179*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.tz", -.157*0.65)
cmds.setAttr("thruster_casing_l_anim_grp.rx", -.451)
cmds.setAttr("thruster_casing_l_anim_grp.ry", 0)
cmds.setAttr("thruster_casing_l_anim_grp.rz", 0)
cmds.setAttr("thruster_casing_l_anim_grp.sx", 1)
cmds.setAttr("thruster_casing_l_anim_grp.sy", 1)
cmds.setAttr("thruster_casing_l_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_casing_l_anim_grp.tx", "thruster_casing_l_anim_grp.ty", "thruster_casing_l_anim_grp.tz", "thruster_casing_l_anim_grp.rx", "thruster_casing_l_anim_grp.ry", "thruster_casing_l_anim_grp.rz", "thruster_casing_l_anim_grp.sx", "thruster_casing_l_anim_grp.sy", "thruster_casing_l_anim_grp.sz"], cd = "body_anim.l_unit_deploy",itt = "flat", ott = "flat")
cmds.setAttr("body_anim.l_unit_deploy", 0)


#RIGHT UNIT DEPLOY
cmds.setAttr("body_anim.r_unit_deploy", 0)
cmds.setAttr("thruster_casing_r_anim_grp.tx", 0*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.ty", 9.046*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.tz", 1.065*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.rx", 95.625)
cmds.setAttr("thruster_casing_r_anim_grp.ry", 0)
cmds.setAttr("thruster_casing_r_anim_grp.rz", 0)
cmds.setAttr("thruster_casing_r_anim_grp.sx", .398)
cmds.setAttr("thruster_casing_r_anim_grp.sy", .398)
cmds.setAttr("thruster_casing_r_anim_grp.sz", .398)
cmds.setDrivenKeyframe(["thruster_casing_r_anim_grp.tx", "thruster_casing_r_anim_grp.ty", "thruster_casing_r_anim_grp.tz", "thruster_casing_r_anim_grp.rx", "thruster_casing_r_anim_grp.ry", "thruster_casing_r_anim_grp.rz", "thruster_casing_r_anim_grp.sx", "thruster_casing_r_anim_grp.sy", "thruster_casing_r_anim_grp.sz"], cd = "body_anim.r_unit_deploy",itt = "flat", ott = "flat")

cmds.setAttr("body_anim.r_unit_deploy", 3.5)
cmds.setAttr("thruster_casing_r_anim_grp.tx", 0*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.ty", -.179*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.tz", .157*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.rx", 95.625)
cmds.setAttr("thruster_casing_r_anim_grp.ry", 0)
cmds.setAttr("thruster_casing_r_anim_grp.rz", 0)
cmds.setAttr("thruster_casing_r_anim_grp.sx", .8)
cmds.setAttr("thruster_casing_r_anim_grp.sy", 1)
cmds.setAttr("thruster_casing_r_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_casing_r_anim_grp.tx", "thruster_casing_r_anim_grp.ty", "thruster_casing_r_anim_grp.tz", "thruster_casing_r_anim_grp.rx", "thruster_casing_r_anim_grp.ry", "thruster_casing_r_anim_grp.rz", "thruster_casing_r_anim_grp.sx", "thruster_casing_r_anim_grp.sy", "thruster_casing_r_anim_grp.sz"], cd = "body_anim.r_unit_deploy",itt = "flat", ott = "flat")

cmds.setAttr("body_anim.r_unit_deploy", 6.5)
cmds.setAttr("thruster_casing_r_anim_grp.tx", 0*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.ty", -.179*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.tz", .157*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.rx", 53.119)
cmds.setAttr("thruster_casing_r_anim_grp.ry", 0)
cmds.setAttr("thruster_casing_r_anim_grp.rz", 0)
cmds.setAttr("thruster_casing_r_anim_grp.sx", .933)
cmds.setAttr("thruster_casing_r_anim_grp.sy", 1)
cmds.setAttr("thruster_casing_r_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_casing_r_anim_grp.tx", "thruster_casing_r_anim_grp.ty", "thruster_casing_r_anim_grp.tz", "thruster_casing_r_anim_grp.rx", "thruster_casing_r_anim_grp.ry", "thruster_casing_r_anim_grp.rz", "thruster_casing_r_anim_grp.sx", "thruster_casing_r_anim_grp.sy", "thruster_casing_r_anim_grp.sz"], cd = "body_anim.r_unit_deploy",itt = "flat", ott = "flat")

cmds.setAttr("body_anim.r_unit_deploy", 10)
cmds.setAttr("thruster_casing_r_anim_grp.tx", 0*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.ty", -.179*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.tz", .157*0.65)
cmds.setAttr("thruster_casing_r_anim_grp.rx", -.451)
cmds.setAttr("thruster_casing_r_anim_grp.ry", 0)
cmds.setAttr("thruster_casing_r_anim_grp.rz", 0)
cmds.setAttr("thruster_casing_r_anim_grp.sx", 1)
cmds.setAttr("thruster_casing_r_anim_grp.sy", 1)
cmds.setAttr("thruster_casing_r_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_casing_r_anim_grp.tx", "thruster_casing_r_anim_grp.ty", "thruster_casing_r_anim_grp.tz", "thruster_casing_r_anim_grp.rx", "thruster_casing_r_anim_grp.ry", "thruster_casing_r_anim_grp.rz", "thruster_casing_r_anim_grp.sx", "thruster_casing_r_anim_grp.sy", "thruster_casing_r_anim_grp.sz"], cd = "body_anim.r_unit_deploy",itt = "flat", ott = "flat")
cmds.setAttr("body_anim.r_unit_deploy", 0)



#LEFT THRUSTER A 
cmds.setAttr("body_anim.l_ventA_deploy", 0)
cmds.setAttr("thruster_a_l_anim_grp.tx", -.038*0.65)
cmds.setAttr("thruster_a_l_anim_grp.ty", -5.089*0.65)
cmds.setAttr("thruster_a_l_anim_grp.tz", -0.79*0.65)
cmds.setAttr("thruster_a_l_anim_grp.rx", -.405)
cmds.setAttr("thruster_a_l_anim_grp.ry", -.105)
cmds.setAttr("thruster_a_l_anim_grp.rz", -.167)
cmds.setAttr("thruster_a_l_anim_grp.sx", 1)
cmds.setAttr("thruster_a_l_anim_grp.sy", .434)
cmds.setAttr("thruster_a_l_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_a_l_anim_grp.tx", "thruster_a_l_anim_grp.ty", "thruster_a_l_anim_grp.tz", "thruster_a_l_anim_grp.rx", "thruster_a_l_anim_grp.ry", "thruster_a_l_anim_grp.rz", "thruster_a_l_anim_grp.sx", "thruster_a_l_anim_grp.sy", "thruster_a_l_anim_grp.sz"], cd = "body_anim.l_ventA_deploy")

cmds.setAttr("body_anim.l_ventA_deploy", 1)
cmds.setAttr("thruster_a_l_anim_grp.tx", -.019*0.65)
cmds.setAttr("thruster_a_l_anim_grp.ty", .261*0.65)
cmds.setAttr("thruster_a_l_anim_grp.tz", -0.116*0.65)
cmds.setAttr("thruster_a_l_anim_grp.rx", -.405)
cmds.setAttr("thruster_a_l_anim_grp.ry", -.105)
cmds.setAttr("thruster_a_l_anim_grp.rz", -.167)
cmds.setAttr("thruster_a_l_anim_grp.sx", 1)
cmds.setAttr("thruster_a_l_anim_grp.sy", 1)
cmds.setAttr("thruster_a_l_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_a_l_anim_grp.tx", "thruster_a_l_anim_grp.ty", "thruster_a_l_anim_grp.tz", "thruster_a_l_anim_grp.rx", "thruster_a_l_anim_grp.ry", "thruster_a_l_anim_grp.rz", "thruster_a_l_anim_grp.sx", "thruster_a_l_anim_grp.sy", "thruster_a_l_anim_grp.sz"], cd = "body_anim.l_ventA_deploy")
cmds.setAttr("body_anim.l_ventA_deploy", 0)


#LEFT THRUSTER B 
cmds.setAttr("body_anim.l_ventB_deploy", 0)
cmds.setAttr("thruster_b_l_anim_grp.tx", -.038*0.65)
cmds.setAttr("thruster_b_l_anim_grp.ty", -5.089*0.65)
cmds.setAttr("thruster_b_l_anim_grp.tz", -0.79*0.65)
cmds.setAttr("thruster_b_l_anim_grp.rx", -.405)
cmds.setAttr("thruster_b_l_anim_grp.ry", -.105)
cmds.setAttr("thruster_b_l_anim_grp.rz", -.167)
cmds.setAttr("thruster_b_l_anim_grp.sx", 1)
cmds.setAttr("thruster_b_l_anim_grp.sy", .434)
cmds.setAttr("thruster_b_l_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_b_l_anim_grp.tx", "thruster_b_l_anim_grp.ty", "thruster_b_l_anim_grp.tz", "thruster_b_l_anim_grp.rx", "thruster_b_l_anim_grp.ry", "thruster_b_l_anim_grp.rz", "thruster_b_l_anim_grp.sx", "thruster_b_l_anim_grp.sy", "thruster_b_l_anim_grp.sz"], cd = "body_anim.l_ventB_deploy")

cmds.setAttr("body_anim.l_ventB_deploy", 1)
cmds.setAttr("thruster_b_l_anim_grp.tx", -.019*0.65)
cmds.setAttr("thruster_b_l_anim_grp.ty", .261*0.65)
cmds.setAttr("thruster_b_l_anim_grp.tz", -0.116*0.65)
cmds.setAttr("thruster_b_l_anim_grp.rx", -.405)
cmds.setAttr("thruster_b_l_anim_grp.ry", -.105)
cmds.setAttr("thruster_b_l_anim_grp.rz", -.167)
cmds.setAttr("thruster_b_l_anim_grp.sx", 1)
cmds.setAttr("thruster_b_l_anim_grp.sy", 1)
cmds.setAttr("thruster_b_l_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_b_l_anim_grp.tx", "thruster_b_l_anim_grp.ty", "thruster_b_l_anim_grp.tz", "thruster_b_l_anim_grp.rx", "thruster_b_l_anim_grp.ry", "thruster_b_l_anim_grp.rz", "thruster_b_l_anim_grp.sx", "thruster_b_l_anim_grp.sy", "thruster_b_l_anim_grp.sz"], cd = "body_anim.l_ventB_deploy")
cmds.setAttr("body_anim.l_ventB_deploy", 0)

#LEFT THRUSTER C 
cmds.setAttr("body_anim.l_ventC_deploy", 0)
cmds.setAttr("thruster_c_l_anim_grp.tx", -.038*0.65)
cmds.setAttr("thruster_c_l_anim_grp.ty", -5.089*0.65)
cmds.setAttr("thruster_c_l_anim_grp.tz", -0.79*0.65)
cmds.setAttr("thruster_c_l_anim_grp.rx", -.405)
cmds.setAttr("thruster_c_l_anim_grp.ry", -.105)
cmds.setAttr("thruster_c_l_anim_grp.rz", -.167)
cmds.setAttr("thruster_c_l_anim_grp.sx", 1)
cmds.setAttr("thruster_c_l_anim_grp.sy", .434)
cmds.setAttr("thruster_c_l_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_c_l_anim_grp.tx", "thruster_c_l_anim_grp.ty", "thruster_c_l_anim_grp.tz", "thruster_c_l_anim_grp.rx", "thruster_c_l_anim_grp.ry", "thruster_c_l_anim_grp.rz", "thruster_c_l_anim_grp.sx", "thruster_c_l_anim_grp.sy", "thruster_c_l_anim_grp.sz"], cd = "body_anim.l_ventC_deploy")

cmds.setAttr("body_anim.l_ventC_deploy", 1)
cmds.setAttr("thruster_c_l_anim_grp.tx", -.019*0.65)
cmds.setAttr("thruster_c_l_anim_grp.ty", .261*0.65)
cmds.setAttr("thruster_c_l_anim_grp.tz", -0.116*0.65)
cmds.setAttr("thruster_c_l_anim_grp.rx", -.405)
cmds.setAttr("thruster_c_l_anim_grp.ry", -.105)
cmds.setAttr("thruster_c_l_anim_grp.rz", -.167)
cmds.setAttr("thruster_c_l_anim_grp.sx", 1)
cmds.setAttr("thruster_c_l_anim_grp.sy", 1)
cmds.setAttr("thruster_c_l_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_c_l_anim_grp.tx", "thruster_c_l_anim_grp.ty", "thruster_c_l_anim_grp.tz", "thruster_c_l_anim_grp.rx", "thruster_c_l_anim_grp.ry", "thruster_c_l_anim_grp.rz", "thruster_c_l_anim_grp.sx", "thruster_c_l_anim_grp.sy", "thruster_c_l_anim_grp.sz"], cd = "body_anim.l_ventC_deploy")
cmds.setAttr("body_anim.l_ventC_deploy", 0)




#RIGHT THRUSTER A 
cmds.setAttr("body_anim.r_ventA_deploy", 0)
cmds.setAttr("thruster_a_r_anim_grp.tx", .038*0.65)
cmds.setAttr("thruster_a_r_anim_grp.ty", 5.089*0.65)
cmds.setAttr("thruster_a_r_anim_grp.tz", 0.79*0.65)
cmds.setAttr("thruster_a_r_anim_grp.rx", -.405)
cmds.setAttr("thruster_a_r_anim_grp.ry", -.105)
cmds.setAttr("thruster_a_r_anim_grp.rz", -.167)
cmds.setAttr("thruster_a_r_anim_grp.sx", 1)
cmds.setAttr("thruster_a_r_anim_grp.sy", .434)
cmds.setAttr("thruster_a_r_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_a_r_anim_grp.tx", "thruster_a_r_anim_grp.ty", "thruster_a_r_anim_grp.tz", "thruster_a_r_anim_grp.rx", "thruster_a_r_anim_grp.ry", "thruster_a_r_anim_grp.rz", "thruster_a_r_anim_grp.sx", "thruster_a_r_anim_grp.sy", "thruster_a_r_anim_grp.sz"], cd = "body_anim.r_ventA_deploy")

cmds.setAttr("body_anim.r_ventA_deploy", 1)
cmds.setAttr("thruster_a_r_anim_grp.tx", .019*0.65)
cmds.setAttr("thruster_a_r_anim_grp.ty", -.261*0.65)
cmds.setAttr("thruster_a_r_anim_grp.tz", 0.116*0.65)
cmds.setAttr("thruster_a_r_anim_grp.rx", -.405)
cmds.setAttr("thruster_a_r_anim_grp.ry", -.105)
cmds.setAttr("thruster_a_r_anim_grp.rz", -.167)
cmds.setAttr("thruster_a_r_anim_grp.sx", 1)
cmds.setAttr("thruster_a_r_anim_grp.sy", 1)
cmds.setAttr("thruster_a_r_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_a_r_anim_grp.tx", "thruster_a_r_anim_grp.ty", "thruster_a_r_anim_grp.tz", "thruster_a_r_anim_grp.rx", "thruster_a_r_anim_grp.ry", "thruster_a_r_anim_grp.rz", "thruster_a_r_anim_grp.sx", "thruster_a_r_anim_grp.sy", "thruster_a_r_anim_grp.sz"], cd = "body_anim.r_ventA_deploy")
cmds.setAttr("body_anim.r_ventA_deploy", 0)


#RIGHT THRUSTER B 
cmds.setAttr("body_anim.r_ventB_deploy", 0)
cmds.setAttr("thruster_b_r_anim_grp.tx", .038*0.65)
cmds.setAttr("thruster_b_r_anim_grp.ty", 5.089*0.65)
cmds.setAttr("thruster_b_r_anim_grp.tz", 0.79*0.65)
cmds.setAttr("thruster_b_r_anim_grp.rx", -.405)
cmds.setAttr("thruster_b_r_anim_grp.ry", -.105)
cmds.setAttr("thruster_b_r_anim_grp.rz", -.167)
cmds.setAttr("thruster_b_r_anim_grp.sx", 1)
cmds.setAttr("thruster_b_r_anim_grp.sy", .434)
cmds.setAttr("thruster_b_r_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_b_r_anim_grp.tx", "thruster_b_r_anim_grp.ty", "thruster_b_r_anim_grp.tz", "thruster_b_r_anim_grp.rx", "thruster_b_r_anim_grp.ry", "thruster_b_r_anim_grp.rz", "thruster_b_r_anim_grp.sx", "thruster_b_r_anim_grp.sy", "thruster_b_r_anim_grp.sz"], cd = "body_anim.r_ventB_deploy")

cmds.setAttr("body_anim.r_ventB_deploy", 1)
cmds.setAttr("thruster_b_r_anim_grp.tx", .019*0.65)
cmds.setAttr("thruster_b_r_anim_grp.ty", -.261*0.65)
cmds.setAttr("thruster_b_r_anim_grp.tz", 0.116*0.65)
cmds.setAttr("thruster_b_r_anim_grp.rx", -.405)
cmds.setAttr("thruster_b_r_anim_grp.ry", -.105)
cmds.setAttr("thruster_b_r_anim_grp.rz", -.167)
cmds.setAttr("thruster_b_r_anim_grp.sx", 1)
cmds.setAttr("thruster_b_r_anim_grp.sy", 1)
cmds.setAttr("thruster_b_r_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_b_r_anim_grp.tx", "thruster_b_r_anim_grp.ty", "thruster_b_r_anim_grp.tz", "thruster_b_r_anim_grp.rx", "thruster_b_r_anim_grp.ry", "thruster_b_r_anim_grp.rz", "thruster_b_r_anim_grp.sx", "thruster_b_r_anim_grp.sy", "thruster_b_r_anim_grp.sz"], cd = "body_anim.r_ventB_deploy")
cmds.setAttr("body_anim.r_ventB_deploy", 0)

#RIGHT THRUSTER C 
cmds.setAttr("body_anim.r_ventC_deploy", 0)
cmds.setAttr("thruster_c_r_anim_grp.tx", .038*0.65)
cmds.setAttr("thruster_c_r_anim_grp.ty", 5.089*0.65)
cmds.setAttr("thruster_c_r_anim_grp.tz", 0.79*0.65)
cmds.setAttr("thruster_c_r_anim_grp.rx", -.405)
cmds.setAttr("thruster_c_r_anim_grp.ry", -.105)
cmds.setAttr("thruster_c_r_anim_grp.rz", -.167)
cmds.setAttr("thruster_c_r_anim_grp.sx", 1)
cmds.setAttr("thruster_c_r_anim_grp.sy", .434)
cmds.setAttr("thruster_c_r_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_c_r_anim_grp.tx", "thruster_c_r_anim_grp.ty", "thruster_c_r_anim_grp.tz", "thruster_c_r_anim_grp.rx", "thruster_c_r_anim_grp.ry", "thruster_c_r_anim_grp.rz", "thruster_c_r_anim_grp.sx", "thruster_c_r_anim_grp.sy", "thruster_c_r_anim_grp.sz"], cd = "body_anim.r_ventC_deploy")

cmds.setAttr("body_anim.r_ventC_deploy", 1)
cmds.setAttr("thruster_c_r_anim_grp.tx", .019*0.65)
cmds.setAttr("thruster_c_r_anim_grp.ty", -.261*0.65)
cmds.setAttr("thruster_c_r_anim_grp.tz", 0.116*0.65)
cmds.setAttr("thruster_c_r_anim_grp.rx", -.405)
cmds.setAttr("thruster_c_r_anim_grp.ry", -.105)
cmds.setAttr("thruster_c_r_anim_grp.rz", -.167)
cmds.setAttr("thruster_c_r_anim_grp.sx", 1)
cmds.setAttr("thruster_c_r_anim_grp.sy", 1)
cmds.setAttr("thruster_c_r_anim_grp.sz", 1)
cmds.setDrivenKeyframe(["thruster_c_r_anim_grp.tx", "thruster_c_r_anim_grp.ty", "thruster_c_r_anim_grp.tz", "thruster_c_r_anim_grp.rx", "thruster_c_r_anim_grp.ry", "thruster_c_r_anim_grp.rz", "thruster_c_r_anim_grp.sx", "thruster_c_r_anim_grp.sy", "thruster_c_r_anim_grp.sz"], cd = "body_anim.r_ventC_deploy")
cmds.setAttr("body_anim.r_ventC_deploy", 0)



#LEFT ARM TOP DEPLOY
cmds.setAttr("weapon_l_anim.top_open", 0)
cmds.setDrivenKeyframe(["lowerArm_topPlateA_l_anim_grp.tx", "lowerArm_topPlateA_l_anim_grp.ty", "lowerArm_topPlateA_l_anim_grp.tz", "lowerArm_topPlateA_l_anim_grp.rx", "lowerArm_topPlateA_l_anim_grp.ry", "lowerArm_topPlateA_l_anim_grp.rz"], cd = "weapon_l_anim.top_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_topPlateB_l_anim_grp.tx", "lowerArm_topPlateB_l_anim_grp.ty", "lowerArm_topPlateB_l_anim_grp.tz", "lowerArm_topPlateB_l_anim_grp.rx", "lowerArm_topPlateB_l_anim_grp.ry", "lowerArm_topPlateB_l_anim_grp.rz"], cd = "weapon_l_anim.top_open",itt = "flat", ott = "flat")

cmds.setAttr("weapon_l_anim.top_open", 5)
cmds.setAttr("lowerArm_topPlateA_l_anim_grp.ty", -1.408*0.65)
cmds.setAttr("lowerArm_topPlateB_l_anim_grp.ty", -1.408*0.65)
cmds.setDrivenKeyframe(["lowerArm_topPlateA_l_anim_grp.tx", "lowerArm_topPlateA_l_anim_grp.ty", "lowerArm_topPlateA_l_anim_grp.tz", "lowerArm_topPlateA_l_anim_grp.rx", "lowerArm_topPlateA_l_anim_grp.ry", "lowerArm_topPlateA_l_anim_grp.rz"], cd = "weapon_l_anim.top_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_topPlateB_l_anim_grp.tx", "lowerArm_topPlateB_l_anim_grp.ty", "lowerArm_topPlateB_l_anim_grp.tz", "lowerArm_topPlateB_l_anim_grp.rx", "lowerArm_topPlateB_l_anim_grp.ry", "lowerArm_topPlateB_l_anim_grp.rz"], cd = "weapon_l_anim.top_open",itt = "flat", ott = "flat")

cmds.setAttr("weapon_l_anim.top_open", 10)
cmds.setAttr("lowerArm_topPlateA_l_anim_grp.tx", 4.631*0.65)
cmds.setAttr("lowerArm_topPlateB_l_anim_grp.tx", 4.631*0.65)
cmds.setAttr("lowerArm_topPlateA_l_anim_grp.ty", -1.408*0.65)
cmds.setAttr("lowerArm_topPlateB_l_anim_grp.ty", -1.408*0.65)
cmds.setAttr("lowerArm_topPlateA_l_anim_grp.rz", -30.5)
cmds.setAttr("lowerArm_topPlateB_l_anim_grp.rz", -30.5)
cmds.setDrivenKeyframe(["lowerArm_topPlateA_l_anim_grp.tx", "lowerArm_topPlateA_l_anim_grp.ty", "lowerArm_topPlateA_l_anim_grp.tz", "lowerArm_topPlateA_l_anim_grp.rx", "lowerArm_topPlateA_l_anim_grp.ry", "lowerArm_topPlateA_l_anim_grp.rz"], cd = "weapon_l_anim.top_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_topPlateB_l_anim_grp.tx", "lowerArm_topPlateB_l_anim_grp.ty", "lowerArm_topPlateB_l_anim_grp.tz", "lowerArm_topPlateB_l_anim_grp.rx", "lowerArm_topPlateB_l_anim_grp.ry", "lowerArm_topPlateB_l_anim_grp.rz"], cd = "weapon_l_anim.top_open",itt = "flat", ott = "flat")
cmds.setAttr("weapon_l_anim.top_open", 0)


#LEFT ARM BOTTOM DEPLOY
cmds.setAttr("weapon_l_anim.bottom_open", 0)
cmds.setDrivenKeyframe(["lowerArm_bottomPlateA_l_anim_grp.tx", "lowerArm_bottomPlateA_l_anim_grp.ty", "lowerArm_bottomPlateA_l_anim_grp.tz", "lowerArm_bottomPlateA_l_anim_grp.rx", "lowerArm_bottomPlateA_l_anim_grp.ry", "lowerArm_bottomPlateA_l_anim_grp.rz"], cd = "weapon_l_anim.bottom_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_bottomPlateB_l_anim_grp.tx", "lowerArm_bottomPlateB_l_anim_grp.ty", "lowerArm_bottomPlateB_l_anim_grp.tz", "lowerArm_bottomPlateB_l_anim_grp.rx", "lowerArm_bottomPlateB_l_anim_grp.ry", "lowerArm_bottomPlateB_l_anim_grp.rz"], cd = "weapon_l_anim.bottom_open",itt = "flat", ott = "flat")

cmds.setAttr("weapon_l_anim.bottom_open", 5)
cmds.setAttr("lowerArm_bottomPlateA_l_anim_grp.ty", -2*0.65)
cmds.setAttr("lowerArm_bottomPlateB_l_anim_grp.ty", -2*0.65)
cmds.setDrivenKeyframe(["lowerArm_bottomPlateA_l_anim_grp.tx", "lowerArm_bottomPlateA_l_anim_grp.ty", "lowerArm_bottomPlateA_l_anim_grp.tz", "lowerArm_bottomPlateA_l_anim_grp.rx", "lowerArm_bottomPlateA_l_anim_grp.ry", "lowerArm_bottomPlateA_l_anim_grp.rz"], cd = "weapon_l_anim.bottom_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_bottomPlateB_l_anim_grp.tx", "lowerArm_bottomPlateB_l_anim_grp.ty", "lowerArm_bottomPlateB_l_anim_grp.tz", "lowerArm_bottomPlateB_l_anim_grp.rx", "lowerArm_bottomPlateB_l_anim_grp.ry", "lowerArm_bottomPlateB_l_anim_grp.rz"], cd = "weapon_l_anim.bottom_open",itt = "flat", ott = "flat")

cmds.setAttr("weapon_l_anim.bottom_open", 10)
cmds.setAttr("lowerArm_bottomPlateA_l_anim_grp.tx", 5.34*0.65)
cmds.setAttr("lowerArm_bottomPlateB_l_anim_grp.tx", 5.34*0.65)
cmds.setAttr("lowerArm_bottomPlateA_l_anim_grp.ty", -2*0.65)
cmds.setAttr("lowerArm_bottomPlateB_l_anim_grp.ty", -2*0.65)
cmds.setAttr("lowerArm_bottomPlateA_l_anim_grp.rx", -12.291)
cmds.setAttr("lowerArm_bottomPlateB_l_anim_grp.rx", 12.291)
cmds.setAttr("lowerArm_bottomPlateA_l_anim_grp.ry", 7.644)
cmds.setAttr("lowerArm_bottomPlateB_l_anim_grp.ry", -7.644)
cmds.setAttr("lowerArm_bottomPlateA_l_anim_grp.rz", -63.777)
cmds.setAttr("lowerArm_bottomPlateB_l_anim_grp.rz", -63.777)
cmds.setDrivenKeyframe(["lowerArm_bottomPlateA_l_anim_grp.tx", "lowerArm_bottomPlateA_l_anim_grp.ty", "lowerArm_bottomPlateA_l_anim_grp.tz", "lowerArm_bottomPlateA_l_anim_grp.rx", "lowerArm_bottomPlateA_l_anim_grp.ry", "lowerArm_bottomPlateA_l_anim_grp.rz"], cd = "weapon_l_anim.bottom_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_bottomPlateB_l_anim_grp.tx", "lowerArm_bottomPlateB_l_anim_grp.ty", "lowerArm_bottomPlateB_l_anim_grp.tz", "lowerArm_bottomPlateB_l_anim_grp.rx", "lowerArm_bottomPlateB_l_anim_grp.ry", "lowerArm_bottomPlateB_l_anim_grp.rz"], cd = "weapon_l_anim.bottom_open",itt = "flat", ott = "flat")
cmds.setAttr("weapon_l_anim.bottom_open", 0)




#RIGHT ARM TOP DEPLOY
cmds.setAttr("weapon_r_anim.top_open", 0)
cmds.setDrivenKeyframe(["lowerArm_topPlateA_r_anim_grp.tx", "lowerArm_topPlateA_r_anim_grp.ty", "lowerArm_topPlateA_r_anim_grp.tz", "lowerArm_topPlateA_r_anim_grp.rx", "lowerArm_topPlateA_r_anim_grp.ry", "lowerArm_topPlateA_r_anim_grp.rz"], cd = "weapon_r_anim.top_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_topPlateB_r_anim_grp.tx", "lowerArm_topPlateB_r_anim_grp.ty", "lowerArm_topPlateB_r_anim_grp.tz", "lowerArm_topPlateB_r_anim_grp.rx", "lowerArm_topPlateB_r_anim_grp.ry", "lowerArm_topPlateB_r_anim_grp.rz"], cd = "weapon_r_anim.top_open",itt = "flat", ott = "flat")

cmds.setAttr("weapon_r_anim.top_open", 5)
cmds.setAttr("lowerArm_topPlateA_r_anim_grp.ty", 1.408*0.65)
cmds.setAttr("lowerArm_topPlateB_r_anim_grp.ty", 1.408*0.65)
cmds.setDrivenKeyframe(["lowerArm_topPlateA_r_anim_grp.tx", "lowerArm_topPlateA_r_anim_grp.ty", "lowerArm_topPlateA_r_anim_grp.tz", "lowerArm_topPlateA_r_anim_grp.rx", "lowerArm_topPlateA_r_anim_grp.ry", "lowerArm_topPlateA_r_anim_grp.rz"], cd = "weapon_r_anim.top_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_topPlateB_r_anim_grp.tx", "lowerArm_topPlateB_r_anim_grp.ty", "lowerArm_topPlateB_r_anim_grp.tz", "lowerArm_topPlateB_r_anim_grp.rx", "lowerArm_topPlateB_r_anim_grp.ry", "lowerArm_topPlateB_r_anim_grp.rz"], cd = "weapon_r_anim.top_open",itt = "flat", ott = "flat")

cmds.setAttr("weapon_r_anim.top_open", 10)
cmds.setAttr("lowerArm_topPlateA_r_anim_grp.tx", -4.631*0.65)
cmds.setAttr("lowerArm_topPlateB_r_anim_grp.tx", -4.631*0.65)
cmds.setAttr("lowerArm_topPlateA_r_anim_grp.ty", 1.408*0.65)
cmds.setAttr("lowerArm_topPlateB_r_anim_grp.ty", 1.408*0.65)
cmds.setAttr("lowerArm_topPlateA_r_anim_grp.rz", -30.5)
cmds.setAttr("lowerArm_topPlateB_r_anim_grp.rz", -30.5)
cmds.setDrivenKeyframe(["lowerArm_topPlateA_r_anim_grp.tx", "lowerArm_topPlateA_r_anim_grp.ty", "lowerArm_topPlateA_r_anim_grp.tz", "lowerArm_topPlateA_r_anim_grp.rx", "lowerArm_topPlateA_r_anim_grp.ry", "lowerArm_topPlateA_r_anim_grp.rz"], cd = "weapon_r_anim.top_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_topPlateB_r_anim_grp.tx", "lowerArm_topPlateB_r_anim_grp.ty", "lowerArm_topPlateB_r_anim_grp.tz", "lowerArm_topPlateB_r_anim_grp.rx", "lowerArm_topPlateB_r_anim_grp.ry", "lowerArm_topPlateB_r_anim_grp.rz"], cd = "weapon_r_anim.top_open",itt = "flat", ott = "flat")
cmds.setAttr("weapon_r_anim.top_open", 0)


#RIGHT ARM BOTTOM DEPLOY
cmds.setAttr("weapon_r_anim.bottom_open", 0)
cmds.setDrivenKeyframe(["lowerArm_bottomPlateA_r_anim_grp.tx", "lowerArm_bottomPlateA_r_anim_grp.ty", "lowerArm_bottomPlateA_r_anim_grp.tz", "lowerArm_bottomPlateA_r_anim_grp.rx", "lowerArm_bottomPlateA_r_anim_grp.ry", "lowerArm_bottomPlateA_r_anim_grp.rz"], cd = "weapon_r_anim.bottom_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_bottomPlateB_r_anim_grp.tx", "lowerArm_bottomPlateB_r_anim_grp.ty", "lowerArm_bottomPlateB_r_anim_grp.tz", "lowerArm_bottomPlateB_r_anim_grp.rx", "lowerArm_bottomPlateB_r_anim_grp.ry", "lowerArm_bottomPlateB_r_anim_grp.rz"], cd = "weapon_r_anim.bottom_open",itt = "flat", ott = "flat")

cmds.setAttr("weapon_r_anim.bottom_open", 5)
cmds.setAttr("lowerArm_bottomPlateA_r_anim_grp.ty", 2*0.65)
cmds.setAttr("lowerArm_bottomPlateB_r_anim_grp.ty", 2*0.65)
cmds.setDrivenKeyframe(["lowerArm_bottomPlateA_r_anim_grp.tx", "lowerArm_bottomPlateA_r_anim_grp.ty", "lowerArm_bottomPlateA_r_anim_grp.tz", "lowerArm_bottomPlateA_r_anim_grp.rx", "lowerArm_bottomPlateA_r_anim_grp.ry", "lowerArm_bottomPlateA_r_anim_grp.rz"], cd = "weapon_r_anim.bottom_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_bottomPlateB_r_anim_grp.tx", "lowerArm_bottomPlateB_r_anim_grp.ty", "lowerArm_bottomPlateB_r_anim_grp.tz", "lowerArm_bottomPlateB_r_anim_grp.rx", "lowerArm_bottomPlateB_r_anim_grp.ry", "lowerArm_bottomPlateB_r_anim_grp.rz"], cd = "weapon_r_anim.bottom_open",itt = "flat", ott = "flat")

cmds.setAttr("weapon_r_anim.bottom_open", 10)
cmds.setAttr("lowerArm_bottomPlateA_r_anim_grp.tx", -5.34*0.65)
cmds.setAttr("lowerArm_bottomPlateB_r_anim_grp.tx", -5.34*0.65)
cmds.setAttr("lowerArm_bottomPlateA_r_anim_grp.ty", 2*0.65)
cmds.setAttr("lowerArm_bottomPlateB_r_anim_grp.ty", 2*0.65)
cmds.setAttr("lowerArm_bottomPlateA_r_anim_grp.rx", -12.291)
cmds.setAttr("lowerArm_bottomPlateB_r_anim_grp.rx", 12.291)
cmds.setAttr("lowerArm_bottomPlateA_r_anim_grp.ry", 7.644)
cmds.setAttr("lowerArm_bottomPlateB_r_anim_grp.ry", -7.644)
cmds.setAttr("lowerArm_bottomPlateA_r_anim_grp.rz", -63.777)
cmds.setAttr("lowerArm_bottomPlateB_r_anim_grp.rz", -63.777)
cmds.setDrivenKeyframe(["lowerArm_bottomPlateA_r_anim_grp.tx", "lowerArm_bottomPlateA_r_anim_grp.ty", "lowerArm_bottomPlateA_r_anim_grp.tz", "lowerArm_bottomPlateA_r_anim_grp.rx", "lowerArm_bottomPlateA_r_anim_grp.ry", "lowerArm_bottomPlateA_r_anim_grp.rz"], cd = "weapon_r_anim.bottom_open",itt = "flat", ott = "flat")
cmds.setDrivenKeyframe(["lowerArm_bottomPlateB_r_anim_grp.tx", "lowerArm_bottomPlateB_r_anim_grp.ty", "lowerArm_bottomPlateB_r_anim_grp.tz", "lowerArm_bottomPlateB_r_anim_grp.rx", "lowerArm_bottomPlateB_r_anim_grp.ry", "lowerArm_bottomPlateB_r_anim_grp.rz"], cd = "weapon_r_anim.bottom_open",itt = "flat", ott = "flat")
cmds.setAttr("weapon_r_anim.bottom_open", 0)


#LEFT DAGGERS
cmds.setAttr("weapon_l_anim.daggers_deploy", 0)
cmds.setDrivenKeyframe("dagger_base_l_anim_grp.tz", cd = "weapon_l_anim.daggers_deploy")

cmds.setAttr("weapon_l_anim.daggers_deploy", 10)
cmds.setAttr("dagger_base_l_anim_grp.tz", -30*0.65)
cmds.setDrivenKeyframe("dagger_base_l_anim_grp.tz", cd = "weapon_l_anim.daggers_deploy")
cmds.setAttr("weapon_l_anim.daggers_deploy", 0)

cmds.connectAttr("weapon_l_anim.daggerA_swing", "dagger_a_l_anim_grp.rx")
cmds.connectAttr("weapon_l_anim.daggerB_swing", "dagger_b_l_anim_grp.rx")
cmds.connectAttr("weapon_l_anim.daggerC_swing", "dagger_c_l_anim_grp.rx")


#RIGHT DAGGERS
cmds.setAttr("weapon_r_anim.daggers_deploy", 0)
cmds.setDrivenKeyframe("dagger_base_r_anim_grp.tz", cd = "weapon_r_anim.daggers_deploy")

cmds.setAttr("weapon_r_anim.daggers_deploy", 10)
cmds.setAttr("dagger_base_r_anim_grp.tz", 30*0.65)
cmds.setDrivenKeyframe("dagger_base_r_anim_grp.tz", cd = "weapon_r_anim.daggers_deploy")
cmds.setAttr("weapon_r_anim.daggers_deploy", 0)

cmds.connectAttr("weapon_r_anim.daggerA_swing", "dagger_a_r_anim_grp.rx")
cmds.connectAttr("weapon_r_anim.daggerB_swing", "dagger_b_r_anim_grp.rx")
cmds.connectAttr("weapon_r_anim.daggerC_swing", "dagger_c_r_anim_grp.rx")


#LEFT SWORD DEPLOY
cmds.setAttr("sword_root_l_anim.sword_deploy", 0)
cmds.setDrivenKeyframe("sword_root_l_anim_grp.tx", cd = "sword_root_l_anim.sword_deploy")

cmds.setAttr("sword_root_l_anim.sword_deploy", 10)
cmds.setAttr("sword_root_l_anim_grp.tx", 43*0.65)
cmds.setDrivenKeyframe("sword_root_l_anim_grp.tx", cd = "sword_root_l_anim.sword_deploy")

cmds.setAttr("sword_root_l_anim.sword_deploy", 0)


#LEFT SWORD OPEN
cmds.setAttr("sword_root_l_anim.sword_open", 0)
cmds.setDrivenKeyframe(["pivotA_l_anim_grp.ry", "pivotB_l_anim_grp.ry", "pivotA_l_anim_grp.rz", "pivotB_l_anim_grp.rz"], cd = "sword_root_l_anim.sword_open")

cmds.setAttr("sword_root_l_anim.sword_open", 10)
cmds.setAttr("pivotA_l_anim_grp.ry", 179.7)
cmds.setAttr("pivotB_l_anim_grp.ry", 179.7)
cmds.setAttr("pivotA_l_anim_grp.rz", 0.1)
cmds.setAttr("pivotB_l_anim_grp.rz", 0.1)
cmds.setDrivenKeyframe(["pivotA_l_anim_grp.ry", "pivotB_l_anim_grp.ry", "pivotA_l_anim_grp.rz", "pivotB_l_anim_grp.rz"], cd = "sword_root_l_anim.sword_open")

cmds.setAttr("sword_root_l_anim.sword_open", 0)

#LEFT SWORD EXTEND
cmds.setAttr("sword_root_l_anim.sword_extend", 0)
cmds.setDrivenKeyframe(["extensionA_l_anim_grp.tx", "extensionA_l_anim_grp.ty", "extensionB_l_anim_grp.tx", "extensionB_l_anim_grp.ty"], cd = "sword_root_l_anim.sword_extend")

cmds.setAttr("sword_root_l_anim.sword_extend", 10)
cmds.setAttr("extensionA_l_anim_grp.tx", -16.469)
cmds.setAttr("extensionA_l_anim_grp.ty", 0.0738)
cmds.setAttr("extensionB_l_anim_grp.tx", -16.469)
cmds.setAttr("extensionB_l_anim_grp.ty", -0.0794)
cmds.setDrivenKeyframe(["extensionA_l_anim_grp.tx", "extensionA_l_anim_grp.ty", "extensionB_l_anim_grp.tx", "extensionB_l_anim_grp.ty"], cd = "sword_root_l_anim.sword_extend")

cmds.setAttr("sword_root_l_anim.sword_extend", 0)



#RIGHT SWORD DEPLOY
cmds.setAttr("sword_root_r_anim.sword_deploy", 0)
cmds.setDrivenKeyframe("sword_root_r_anim_grp.tx", cd = "sword_root_r_anim.sword_deploy")

cmds.setAttr("sword_root_r_anim.sword_deploy", 10)
cmds.setAttr("sword_root_r_anim_grp.tx", -43*0.65)
cmds.setDrivenKeyframe("sword_root_r_anim_grp.tx", cd = "sword_root_r_anim.sword_deploy")

cmds.setAttr("sword_root_r_anim.sword_deploy", 0)


#RIGHT SWORD OPEN
cmds.setAttr("sword_root_r_anim.sword_open", 0)
cmds.setDrivenKeyframe(["pivotA_r_anim_grp.ry", "pivotB_r_anim_grp.ry", "pivotA_r_anim_grp.rz", "pivotB_r_anim_grp.rz"], cd = "sword_root_r_anim.sword_open")

cmds.setAttr("sword_root_r_anim.sword_open", 10)
cmds.setAttr("pivotA_r_anim_grp.ry", 179.7)
cmds.setAttr("pivotB_r_anim_grp.ry", 179.7)
cmds.setAttr("pivotA_r_anim_grp.rz", 0.1)
cmds.setAttr("pivotB_r_anim_grp.rz", 0.1)
cmds.setDrivenKeyframe(["pivotA_r_anim_grp.ry", "pivotB_r_anim_grp.ry", "pivotA_r_anim_grp.rz", "pivotB_r_anim_grp.rz"], cd = "sword_root_r_anim.sword_open")

cmds.setAttr("sword_root_r_anim.sword_open", 0)

#RIGHT SWORD EXTEND
cmds.setAttr("sword_root_r_anim.sword_extend", 0)
cmds.setDrivenKeyframe(["extensionA_r_anim_grp.tx", "extensionA_r_anim_grp.ty", "extensionB_r_anim_grp.tx", "extensionB_r_anim_grp.ty"], cd = "sword_root_r_anim.sword_extend")

cmds.setAttr("sword_root_r_anim.sword_extend", 10)
cmds.setAttr("extensionA_r_anim_grp.tx", 16.469)
cmds.setAttr("extensionA_r_anim_grp.ty", 0.101)
cmds.setAttr("extensionB_r_anim_grp.tx", 16.469)
cmds.setAttr("extensionB_r_anim_grp.ty", -0.0965)
cmds.setDrivenKeyframe(["extensionA_r_anim_grp.tx", "extensionA_r_anim_grp.ty", "extensionB_r_anim_grp.tx", "extensionB_r_anim_grp.ty"], cd = "sword_root_r_anim.sword_extend")

cmds.setAttr("sword_root_r_anim.sword_extend", 0)


# MAKE THE GRP NODES SCALE DRIVE THE CONTROLS SCALE
for control in controlsToHide:
	grp = control + "_grp"
	cmds.connectAttr(grp+".sx", control+".sx", f=True)
	cmds.connectAttr(grp+".sy", control+".sy", f=True)
	cmds.connectAttr(grp+".sz", control+".sz", f=True)
	cmds.setAttr(control + ".sx", l=True)
	cmds.setAttr(control + ".sy", l=True)
	cmds.setAttr(control + ".sz", l=True)
    


#CREATE SPACES FOR WEAPONS ANIMS
pairs = [["weapon_l_anim", "driver_hand_l"], ["weapon_r_anim", "driver_hand_r"]]

for pair in pairs:
    control = pair[0]
    spaceObj = pair[1]

    spaceSwitchFollow = control + "_space_switcher_follow"
    spaceSwitchNode = control + "_space_switcher"

    #add attr to the space switcher node
    cmds.select(spaceSwitchNode)
    cmds.addAttr(ln = "space_" + spaceObj, minValue = 0, maxValue = 1, dv = 0, keyable = True)

    #add constraint to the new object on the follow node
    constraint = cmds.parentConstraint(spaceObj, spaceSwitchFollow, mo = True)[0]

    #hook up connections
    targets = cmds.parentConstraint(constraint, q = True, targetList = True)
    weight = 0
    for i in range(int(len(targets))):
        if targets[i].find(spaceObj) != -1:
            weight = i
            
    cmds.connectAttr(spaceSwitchNode + ".space_" + spaceObj, constraint + "." + spaceObj + "W" + str(weight))

    #lockNode on space object so it cannot be deleted by the user (if node is not a referenced node)
    if spaceObj.find(":") == -1:
        cmds.lockNode(spaceObj)
	    

const = cmds.parentConstraint("fk_tentacle_l_01_anim", "tentacle_spring_l_anim_grp", mo=False)
cmds.move(0, 17, 0, "tentacle_spring_l_anim", os=True, wd=True)
const = cmds.parentConstraint("fk_tentacle_r_01_anim", "tentacle_spring_r_anim_grp", mo=False)
cmds.move(0, -17, 0, "tentacle_spring_r_anim", os=True, wd=True)

	    
#SET SPACES
cmds.setAttr("weapon_l_anim_space_switcher.space_driver_hand_l", 1)
cmds.setAttr("weapon_r_anim_space_switcher.space_driver_hand_r", 1)


#SET ROTATE ORDER ON SWORD HANDLE
cmds.setAttr("sword_handle_l_anim.rotateOrder", 3)
cmds.setAttr("sword_handle_r_anim.rotateOrder", 3)

#DEPLOY SWORD
cmds.setAttr("sword_root_l_anim.sword_deploy", 10)
cmds.setAttr("sword_root_l_anim.sword_open", 10)
cmds.setAttr("sword_root_l_anim.sword_extend", 10)

cmds.setAttr("sword_root_r_anim.sword_deploy", 10)
cmds.setAttr("sword_root_r_anim.sword_open", 10)
cmds.setAttr("sword_root_r_anim.sword_extend", 10)


#TURN SWORD
cmds.setAttr("sword_handle_l_anim.rotateY", 90)
cmds.setAttr("sword_handle_r_anim.rotateY", 90)

#TURN OFF SHOULDER PADS
cmds.setAttr("shoulderPad_l_anim.chainStartEnvelope", 0)
cmds.setAttr("shoulderPad_r_anim.chainStartEnvelope", 0)


#SWORD HANDLE 
cmds.setAttr("sword_handle_l_anim.tx", -7.5*0.65)
cmds.setAttr("sword_handle_r_anim.tx", 7.5*0.65)
cmds.setAttr("sword_handle_l_anim.ty", 1*0.65)
cmds.setAttr("sword_handle_r_anim.ty", 1*0.65)

# ------------------------------------------------------------------
def shoulderRig():
    # Left Shoulder
    cmds.setAttr("shoulderflap_l_anim_grp.translate", -0.03089, 0, 0.30622, type="double3")
    cmds.setAttr("shoulderflap_l_anim_grp.rotate", -0.45756, -8.57763, -3.24899, type="double3")
    cmds.setDrivenKeyframe(["shoulderflap_l_anim_grp.translate", "shoulderflap_l_anim_grp.rotate"], cd = "driver_clavicle_l.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("clavicle_l_anim.translateZ", 6.5)
    cmds.setAttr("shoulderflap_l_anim_grp.translate", -0.09202, 0.05725, 0.27608, type="double3")
    cmds.setAttr("shoulderflap_l_anim_grp.rotate", -8.73922, -16.32952, -7.35928, type="double3")
    cmds.setDrivenKeyframe(["shoulderflap_l_anim_grp.translate", "shoulderflap_l_anim_grp.rotate"], cd = "driver_clavicle_l.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("clavicle_l_anim.translateZ", -8)
    cmds.setAttr("shoulderflap_l_anim_grp.translate", -1.36702, -0.35488, 2.09206, type="double3")
    cmds.setAttr("shoulderflap_l_anim_grp.rotate", 12.18621, -2.83324, 3.00603, type="double3")
    cmds.setDrivenKeyframe(["shoulderflap_l_anim_grp.translate", "shoulderflap_l_anim_grp.rotate"], cd = "driver_clavicle_l.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("clavicle_l_anim.translateZ", 0)

    # Right Shoulder
    cmds.setAttr("shoulderflap_r_anim_grp.translate", 0.03089, 0, -0.30622, type="double3")
    cmds.setAttr("shoulderflap_r_anim_grp.rotate", -0.45756, -8.57763, -3.24899, type="double3")
    cmds.setDrivenKeyframe(["shoulderflap_r_anim_grp.translate", "shoulderflap_r_anim_grp.rotate"], cd = "driver_clavicle_r.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("clavicle_r_anim.translateZ", 6.5)
    cmds.setAttr("shoulderflap_r_anim_grp.translate", 0.09202, -0.05725, -0.27608, type="double3")
    cmds.setAttr("shoulderflap_r_anim_grp.rotate", -8.73922, -16.32952, -7.35928, type="double3")
    cmds.setDrivenKeyframe(["shoulderflap_r_anim_grp.translate", "shoulderflap_r_anim_grp.rotate"], cd = "driver_clavicle_r.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("clavicle_r_anim.translateZ", -8)
    cmds.setAttr("shoulderflap_r_anim_grp.translate", 1.36702, 0.35488, -2.09206, type="double3")
    cmds.setAttr("shoulderflap_r_anim_grp.rotate", 12.18621, -2.83324, 3.00603, type="double3")
    cmds.setDrivenKeyframe(["shoulderflap_r_anim_grp.translate", "shoulderflap_r_anim_grp.rotate"], cd = "driver_clavicle_r.rotateY", itt = "spline", ott = "spline")

    cmds.setAttr("clavicle_r_anim.translateZ", 0)

shoulderRig()


# ------------------------------------------------------------------
def dynamicsOff():
    for one in ["shoulderPad_l_anim", "shoulderPad_r_anim", "tentacle_l_dyn_anim", "tentacle_r_dyn_anim", "thrusterVent_l_dyn_anim", "thrusterVent_r_dyn_anim"]:
        cmds.setAttr(one+".chainStartEnvelope", 0)
        cmds.setAttr(one+".chainStartEnvelope", l=True)
dynamicsOff()

# ------------------------------------------------------------------
def legMorphsRig():
    cmds.setAttr("Rig_Settings.lLegMode", 0)
    cmds.setAttr("Rig_Settings.rLegMode", 0)
    # SDKS
    for side in ["_l", "_r"]:
        cmds.setAttr("driver_calf"+side+".rotateOrder", 2)
        cmds.setDrivenKeyframe(["Legs_geo_blendShape.Legs_geo_knee_bend"+side], cd = "driver_calf"+side+".rz", itt = "linear", ott = "linear")
        cmds.setAttr("fk_calf"+side+"_anim.rz", 100)
        cmds.setDrivenKeyframe(["Legs_geo_blendShape.Legs_geo_knee_bend"+side], cd = "driver_calf"+side+".rz", itt = "linear", ott = "linear")
        cmds.setAttr("fk_calf"+side+"_anim.rz", 140)
        cmds.setAttr("Legs_geo_blendShape.Legs_geo_knee_bend"+side, 1)
        cmds.setDrivenKeyframe(["Legs_geo_blendShape.Legs_geo_knee_bend"+side], cd = "driver_calf"+side+".rz", itt = "linear", ott = "linear")
        cmds.setAttr("fk_calf"+side+"_anim.rz", 0)
    cmds.setAttr("Rig_Settings.lLegMode", 1)
    cmds.setAttr("Rig_Settings.rLegMode", 1)

legMorphsRig()
# ------------------------------------------------------------------

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
addCapsule(46, 72)

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