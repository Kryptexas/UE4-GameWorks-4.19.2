import maya.cmds as cmds

# NOTE: This character has special steps to run during Rig Pose creation.  See end of script.
# ------------------------------------------------------------------
def shieldRig():
    #Move Shield to correct position
    cmds.setAttr("sheild_main_anim_grp.tx", 190.409*0.65)
    cmds.setAttr("sheild_main_anim_grp.ty", -101.619*0.65)
    cmds.setAttr("sheild_main_anim_grp.tz", -130.408*0.65)
    cmds.setAttr("sheild_main_anim_grp.ry", 90)

    #Shield Anim Control Scale
    cmds.select("sheild_main_anim" + ".cv[0:32]")
    cmds.scale(6*0.65, 6*0.65, 0, r=True)

    #add attrs to shield anim
    attrsToAdd = ["shieldDeploy1", "shieldDeploy2", "shieldDeploy3", "shieldDeploy4", "panelA_deploy1", "panelA_deploy2", "panelB_deploy1", "panelB_deploy2", "panelC_deploy1", "panelC_deploy2", "panelD_deploy1", "panelD_deploy2"] 
    for attr in attrsToAdd:
        cmds.addAttr("sheild_main_anim", ln = attr, dv = 0, min = 0, keyable = True)

    #Shield SDKS
    #deploy1
    cmds.setAttr("sheild_main_anim.shieldDeploy1", 0)
    cmds.setDrivenKeyframe(["loarm_shield_top_l_anim.tz", "loarm_shield_top_l_anim.ry", "loarm_shield_bot_l_anim.tz", "loarm_shield_bot_l_anim.ry"], cd = "sheild_main_anim.shieldDeploy1", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.shieldDeploy1", 10)
    cmds.setAttr("loarm_shield_top_l_anim.tz", 13.22*0.65)
    cmds.setAttr("loarm_shield_top_l_anim.ry", -11.846)
    cmds.setAttr("loarm_shield_bot_l_anim.tz", -14.016*0.65)
    cmds.setAttr("loarm_shield_bot_l_anim.ry", 14.991)
    cmds.setDrivenKeyframe(["loarm_shield_top_l_anim.tz", "loarm_shield_top_l_anim.ry", "loarm_shield_bot_l_anim.tz", "loarm_shield_bot_l_anim.ry"], cd = "sheild_main_anim.shieldDeploy1", itt = "spline", ott = "spline")
    cmds.setAttr("sheild_main_anim.shieldDeploy1", 0)

    cmds.setInfinity("loarm_shield_top_l_anim.tz", pri = "linear", poi = "linear")
    cmds.setInfinity("loarm_shield_top_l_anim.ry", pri = "linear", poi = "linear")
    cmds.setInfinity("loarm_shield_bot_l_anim.tz", pri = "linear", poi = "linear")
    cmds.setInfinity("loarm_shield_bot_l_anim.ry", pri = "linear", poi = "linear")

    cmds.setAttr("loarm_shield_top_l_anim.tz", keyable = False, channelBox = False)
    cmds.setAttr("loarm_shield_top_l_anim.ry", keyable = False, channelBox = False)
    cmds.setAttr("loarm_shield_bot_l_anim.tz", keyable = False, channelBox = False)
    cmds.setAttr("loarm_shield_bot_l_anim.ry", keyable = False, channelBox = False)

    #deploy2
    cmds.setAttr("sheild_main_anim.shieldDeploy2", 0)
    cmds.setDrivenKeyframe(["sheild_main_anim_grp.ty"], cd = "sheild_main_anim.shieldDeploy2", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.shieldDeploy2", 10)
    cmds.setAttr("sheild_main_anim_grp.ty", -55*0.65)
    cmds.setDrivenKeyframe(["sheild_main_anim_grp.ty"], cd = "sheild_main_anim.shieldDeploy2", itt = "spline", ott = "spline")
    cmds.setAttr("sheild_main_anim.shieldDeploy2", 0)
    cmds.setInfinity("sheild_main_anim_grp.ty", pri = "linear", poi = "linear")
    cmds.setAttr("sheild_main_anim_grp.ty", keyable = False, channelBox = False)

    #deploy3
    cmds.setAttr("sheild_main_anim.shieldDeploy3", 10)
    cmds.setDrivenKeyframe(["panel_bot_root_l_anim.sy", "panel_bot_root_r_anim.sy", "panel_top_root_l_anim.sy", "panel_top_root_r_anim.sy"], cd = "sheild_main_anim.shieldDeploy3", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.shieldDeploy3", 0)

    cmds.setAttr("panel_bot_root_l_anim.sy", .5*0.65)
    cmds.setAttr("panel_bot_root_r_anim.sy", .5*0.65)
    cmds.setAttr("panel_top_root_l_anim.sy", .5*0.65)
    cmds.setAttr("panel_top_root_r_anim.sy", .5*0.65)
    cmds.setDrivenKeyframe(["panel_bot_root_l_anim.sy", "panel_bot_root_r_anim.sy", "panel_top_root_l_anim.sy", "panel_top_root_r_anim.sy"], cd = "sheild_main_anim.shieldDeploy3", itt = "spline", ott = "spline")
    cmds.setInfinity("panel_bot_root_l_anim.sy", pri = "linear", poi = "linear")
    cmds.setInfinity("panel_bot_root_r_anim.sy", pri = "linear", poi = "linear")
    cmds.setInfinity("panel_top_root_l_anim.sy", pri = "linear", poi = "linear")
    cmds.setInfinity("panel_top_root_r_anim.sy", pri = "linear", poi = "linear")

    cmds.setAttr("panel_bot_root_l_anim.sy", keyable = False, channelBox = False)
    cmds.setAttr("panel_bot_root_r_anim.sy", keyable = False, channelBox = False)
    cmds.setAttr("panel_top_root_l_anim.sy", keyable = False, channelBox = False)
    cmds.setAttr("panel_top_root_r_anim.sy", keyable = False, channelBox = False)

    #deploy4
    cmds.setAttr("sheild_main_anim.shieldDeploy4", 10)
    cmds.setDrivenKeyframe(["panel_bot_r_anim.sz", "panel_bot_l_anim.sz", "panel_top_r_anim.sz", "panel_top_l_anim.sz"], cd = "sheild_main_anim.shieldDeploy4", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.shieldDeploy4", 0)

    cmds.setAttr("panel_bot_r_anim.sz", .3*0.65)
    cmds.setAttr("panel_bot_l_anim.sz", .3*0.65)
    cmds.setAttr("panel_top_r_anim.sz", .3*0.65)
    cmds.setAttr("panel_top_l_anim.sz", .3*0.65)
    cmds.setDrivenKeyframe(["panel_bot_r_anim.sz", "panel_bot_l_anim.sz", "panel_top_r_anim.sz", "panel_top_l_anim.sz"], cd = "sheild_main_anim.shieldDeploy4", itt = "spline", ott = "spline")
    cmds.setInfinity("panel_bot_r_anim.sz", pri = "linear", poi = "linear")
    cmds.setInfinity("panel_bot_l_anim.sz", pri = "linear", poi = "linear")
    cmds.setInfinity("panel_top_r_anim.sz", pri = "linear", poi = "linear")
    cmds.setInfinity("panel_top_l_anim.sz", pri = "linear", poi = "linear")

    cmds.setAttr("panel_bot_r_anim.sz", keyable = False, channelBox = False)
    cmds.setAttr("panel_bot_l_anim.sz", keyable = False, channelBox = False)
    cmds.setAttr("panel_top_r_anim.sz", keyable = False, channelBox = False)
    cmds.setAttr("panel_top_l_anim.sz", keyable = False, channelBox = False)

    #panelA 1
    cmds.setAttr("sheild_main_anim.panelA_deploy1", 10)
    cmds.setDrivenKeyframe(["panel_bot_root_l_anim.rz"], cd = "sheild_main_anim.panelA_deploy1", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.panelA_deploy1", 0)
    cmds.setAttr("panel_bot_root_l_anim.rz", -90)
    cmds.setDrivenKeyframe(["panel_bot_root_l_anim.rz"], cd = "sheild_main_anim.panelA_deploy1", itt = "spline", ott = "spline")
    cmds.setInfinity("panel_bot_root_l_anim.rz", pri = "linear", poi = "linear")
    cmds.setAttr("panel_bot_root_l_anim.rz", keyable = False, channelBox = False)

    #panelA 2
    cmds.setAttr("sheild_main_anim.panelA_deploy2", 10)
    cmds.setDrivenKeyframe(["panel_bot_l_anim.sx"], cd = "sheild_main_anim.panelA_deploy2", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.panelA_deploy2", 0)
    cmds.setAttr("panel_bot_l_anim.sx", 0.05*0.65)
    cmds.setDrivenKeyframe(["panel_bot_l_anim.sx"], cd = "sheild_main_anim.panelA_deploy2", itt = "spline", ott = "spline")
    cmds.setInfinity("panel_bot_l_anim.sx", pri = "linear", poi = "linear")
    cmds.setAttr("panel_bot_l_anim.sx", keyable = False, channelBox = False)

    #panelB 1
    cmds.setAttr("sheild_main_anim.panelB_deploy1", 10)
    cmds.setDrivenKeyframe(["panel_top_root_l_anim.rz"], cd = "sheild_main_anim.panelB_deploy1", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.panelB_deploy1", 0)
    cmds.setAttr("panel_top_root_l_anim.rz", -90)
    cmds.setDrivenKeyframe(["panel_top_root_l_anim.rz"], cd = "sheild_main_anim.panelB_deploy1", itt = "spline", ott = "spline")
    cmds.setInfinity("panel_top_root_l_anim.rz", pri = "linear", poi = "linear")
    cmds.setAttr("panel_top_root_l_anim.rz", keyable = False, channelBox = False)

    #panelB 2
    cmds.setAttr("sheild_main_anim.panelB_deploy2", 10)
    cmds.setDrivenKeyframe(["panel_top_l_anim.sx"], cd = "sheild_main_anim.panelB_deploy2", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.panelB_deploy2", 0)
    cmds.setAttr("panel_top_l_anim.sx", 0.05*0.65)
    cmds.setDrivenKeyframe(["panel_top_l_anim.sx"], cd = "sheild_main_anim.panelB_deploy2", itt = "spline", ott = "spline")
    cmds.setInfinity("panel_top_l_anim.sx", pri = "linear", poi = "linear")
    cmds.setAttr("panel_top_l_anim.sx", keyable = False, channelBox = False)

    #panelC 1
    cmds.setAttr("sheild_main_anim.panelC_deploy1", 10)
    cmds.setDrivenKeyframe(["panel_bot_root_r_anim.rz"], cd = "sheild_main_anim.panelC_deploy1", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.panelC_deploy1", 0)
    cmds.setAttr("panel_bot_root_r_anim.rz", -90)
    cmds.setDrivenKeyframe(["panel_bot_root_r_anim.rz"], cd = "sheild_main_anim.panelC_deploy1", itt = "spline", ott = "spline")
    cmds.setInfinity("panel_bot_root_r_anim.rz", pri = "linear", poi = "linear")
    cmds.setAttr("panel_bot_root_r_anim.rz", keyable = False, channelBox = False)

    #panelC 2
    cmds.setAttr("sheild_main_anim.panelC_deploy2", 10)
    cmds.setDrivenKeyframe(["panel_bot_r_anim.sx"], cd = "sheild_main_anim.panelC_deploy2", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.panelC_deploy2", 0)
    cmds.setAttr("panel_bot_r_anim.sx", 0.05*0.65)
    cmds.setDrivenKeyframe(["panel_bot_r_anim.sx"], cd = "sheild_main_anim.panelC_deploy2", itt = "spline", ott = "spline")
    cmds.setInfinity("panel_bot_r_anim.sx", pri = "linear", poi = "linear")
    cmds.setAttr("panel_bot_r_anim.sx", keyable = False, channelBox = False)

    #panelD 1
    cmds.setAttr("sheild_main_anim.panelD_deploy1", 10)
    cmds.setDrivenKeyframe(["panel_top_root_r_anim.rz"], cd = "sheild_main_anim.panelD_deploy1", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.panelD_deploy1", 0)
    cmds.setAttr("panel_top_root_r_anim.rz", -90)
    cmds.setDrivenKeyframe(["panel_top_root_r_anim.rz"], cd = "sheild_main_anim.panelD_deploy1", itt = "spline", ott = "spline")
    cmds.setInfinity("panel_top_root_r_anim.rz", pri = "linear", poi = "linear")
    cmds.setAttr("panel_top_root_r_anim.rz", keyable = False, channelBox = False)

    #panelD 2
    cmds.setAttr("sheild_main_anim.panelD_deploy2", 10)
    cmds.setDrivenKeyframe(["panel_top_r_anim.sx"], cd = "sheild_main_anim.panelD_deploy2", itt = "spline", ott = "spline")

    cmds.setAttr("sheild_main_anim.panelD_deploy2", 0)
    cmds.setAttr("panel_top_r_anim.sx", 0.05*0.65)
    cmds.setDrivenKeyframe(["panel_top_r_anim.sx"], cd = "sheild_main_anim.panelD_deploy2", itt = "spline", ott = "spline")
    cmds.setInfinity("panel_top_r_anim.sx", pri = "linear", poi = "linear")
    cmds.setAttr("panel_top_r_anim.sx", keyable = False, channelBox = False)

shieldRig()

# ------------------------------------------------------------------
def legRig():
    #Boot SDKS
    for side in ["_l", "_r"]:
        cmds.setDrivenKeyframe(["calf_armor"+side+"_anim_grp.rx", "toe_plate"+side+"_anim_grp.rz"], cd = "driver_foot"+side+".rz", itt = "linear", ott = "linear")
        cmds.setAttr("driver_foot"+side+".rz", 30)
        cmds.setAttr("calf_armor"+side+"_anim_grp.rx", -90)
        cmds.setAttr("toe_plate"+side+"_anim_grp.rz", -90)
        cmds.setDrivenKeyframe(["calf_armor"+side+"_anim_grp.rx", "toe_plate"+side+"_anim_grp.rz"], cd = "driver_foot"+side+".rz", itt = "linear", ott = "linear")
        cmds.setAttr("driver_foot"+side+".rz", 0)

        cmds.setDrivenKeyframe(["calf_armor"+side+"_anim_grp.rx", "calf_armor"+side+"_anim_grp.ry", "calf_armor"+side+"_anim_grp.rz"], cd = "driver_foot"+side+".rx", itt = "linear", ott = "linear")
        cmds.setAttr("driver_foot"+side+".rx", 45)
        cmds.setAttr("calf_armor"+side+"_anim_grp.rx", -11.7)
        cmds.setAttr("calf_armor"+side+"_anim_grp.ry", -41)
        cmds.setAttr("calf_armor"+side+"_anim_grp.rz", 24.9)
        cmds.setDrivenKeyframe(["calf_armor"+side+"_anim_grp.rx", "calf_armor"+side+"_anim_grp.ry", "calf_armor"+side+"_anim_grp.rz"], cd = "driver_foot"+side+".rx", itt = "linear", ott = "linear")
        cmds.setAttr("driver_foot"+side+".rx", -45)
        cmds.setAttr("calf_armor"+side+"_anim_grp.rx", -11.7)
        cmds.setAttr("calf_armor"+side+"_anim_grp.ry", 41)
        cmds.setAttr("calf_armor"+side+"_anim_grp.rz", -24.9)
        cmds.setDrivenKeyframe(["calf_armor"+side+"_anim_grp.rx", "calf_armor"+side+"_anim_grp.ry", "calf_armor"+side+"_anim_grp.rz"], cd = "driver_foot"+side+".rx", itt = "linear", ott = "linear")
        cmds.setAttr("driver_foot"+side+".rx", 0)
        
        cmds.setDrivenKeyframe(["calf_armor"+side+"_anim_grp.rx", "calf_armor"+side+"_anim_grp.ry", "calf_armor"+side+"_anim_grp.rz"], cd = "driver_foot"+side+".ry", itt = "linear", ott = "linear")
        cmds.setAttr("driver_foot"+side+".ry", 45)
        cmds.setAttr("calf_armor"+side+"_anim_grp.rx", -17)
        cmds.setAttr("calf_armor"+side+"_anim_grp.ry", 24)
        cmds.setAttr("calf_armor"+side+"_anim_grp.rz", 0.3)
        cmds.setDrivenKeyframe(["calf_armor"+side+"_anim_grp.rx", "calf_armor"+side+"_anim_grp.ry", "calf_armor"+side+"_anim_grp.rz"], cd = "driver_foot"+side+".ry", itt = "linear", ott = "linear")
        cmds.setAttr("driver_foot"+side+".ry", -45)
        cmds.setAttr("calf_armor"+side+"_anim_grp.rx", -17)
        cmds.setAttr("calf_armor"+side+"_anim_grp.ry", -24)
        cmds.setAttr("calf_armor"+side+"_anim_grp.rz", -0.3)
        cmds.setDrivenKeyframe(["calf_armor"+side+"_anim_grp.rx", "calf_armor"+side+"_anim_grp.ry", "calf_armor"+side+"_anim_grp.rz"], cd = "driver_foot"+side+".ry", itt = "linear", ott = "linear")
        cmds.setAttr("driver_foot"+side+".ry", 0)
    
legRig()

# ------------------------------------------------------------------
def tailFlapRig():
    cmds.setDrivenKeyframe(["tail_pad_anim_grp.rx"], cd = "driver_spine_01.rz", itt = "linear", ott = "linear")
    cmds.setAttr("driver_spine_01.rz", 90)
    cmds.setAttr("tail_pad_anim_grp.rx", -90)
    cmds.setDrivenKeyframe(["tail_pad_anim_grp.rx"], cd = "driver_spine_01.rz", itt = "linear", ott = "linear")
    cmds.setAttr("driver_spine_01.rz", 15)
    cmds.setAttr("tail_pad_anim_grp.rx", -38)
    cmds.setDrivenKeyframe(["tail_pad_anim_grp.rx"], cd = "driver_spine_01.rz", itt = "spline", ott = "spline")
    cmds.setAttr("driver_spine_01.rz", 0)

    cmds.setDrivenKeyframe(["tail_pad_anim_grp.ry"], cd = "driver_spine_01.ry", itt = "linear", ott = "linear")
    cmds.setAttr("driver_spine_01.ry", 90)
    cmds.setAttr("tail_pad_anim_grp.ry", 25)
    cmds.setDrivenKeyframe(["tail_pad_anim_grp.ry"], cd = "driver_spine_01.ry", itt = "linear", ott = "linear")
    cmds.setAttr("driver_spine_01.ry", -90)
    cmds.setAttr("tail_pad_anim_grp.ry", -25)
    cmds.setDrivenKeyframe(["tail_pad_anim_grp.ry"], cd = "driver_spine_01.ry", itt = "linear", ott = "linear")
    cmds.setAttr("driver_spine_01.ry", 0)

    cmds.setDrivenKeyframe(["tail_pad_anim_grp.rz"], cd = "driver_spine_01.rx", itt = "linear", ott = "linear")
    cmds.setAttr("driver_spine_01.rx", 90)
    cmds.setAttr("tail_pad_anim_grp.rz", 20)
    cmds.setDrivenKeyframe(["tail_pad_anim_grp.rz"], cd = "driver_spine_01.rx", itt = "linear", ott = "linear")
    cmds.setAttr("driver_spine_01.rx", -90)
    cmds.setAttr("tail_pad_anim_grp.rz", -20)
    cmds.setDrivenKeyframe(["tail_pad_anim_grp.rz"], cd = "driver_spine_01.rx", itt = "linear", ott = "linear")
    cmds.setAttr("driver_spine_01.rx", 0)
tailFlapRig()

# ------------------------------------------------------------------
def scaleControls():
    #Hide controls
    '''controlsToHide = ["toe_l_anim", "toe_r_anim", "loarm_shield_top_l_anim", "loarm_shield_bot_l_anim", "loarm_shield_back_l_anim", "panels_l_anim", "panels_main_anim", "panels_r_anim", "panel_top_root_l_anim",
    "panel_top_root_r_anim",  "panel_bot_root_l_anim", "panel_bot_root_r_anim", "panel_bot_l_anim", "panel_bot_r_anim", "panel_top_l_anim", "panel_top_r_anim", "toe_plate_l_anim", "toe_plate_r_anim"]
    for ctrl in controlsToHide:
        if cmds.objExists(ctrl):
            grp = ctrl+"_grp"
            cmds.setAttr(grp + ".visibility", 0)
            cmds.setAttr(grp + ".visibility", lock = True)'''

    for side in ["_l", "_r"]:
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

        #scale shin controls
        cmds.select("shinpad"+side+"_anim" + ".cv[0:32]")
        cmds.scale(0, 2*0.65, 2*0.65, r=True)

        #calf armor controls
        cmds.select("calf_armor"+side+"_anim" + ".cv[0:32]")
        cmds.scale(2*0.65, 2*0.65, 0, r=True)

        #straps controls
        cmds.select("arm_strap_top"+side+"_anim" + ".cv[0:32]")
        cmds.scale(0.22, 0.22, 0.22, r=True)

        cmds.select("arm_strap_bott"+side+"_anim" + ".cv[0:32]")
        cmds.scale(0.22, 0.22, 0.22, r=True)

        cmds.select("chest_strap"+side+"_anim" + ".cv[0:32]")
        cmds.scale(0.22, 0.22, 0.22, r=True)

        cmds.select("hip_strap"+side+"_anim" + ".cv[0:32]")
        cmds.scale(0.22, 0.22, 0.22, r=True)

        #wrist anims
        cmds.select("ik_wrist"+side+"_anim" + ".cv[0:32]")
        cmds.scale(2.8*0.65, 2.8*0.65, 2.8*0.65, r=True)

        cmds.select("ik_elbow"+side+"_anim" + ".cv[0:32]")
        cmds.scale(3*0.65, 3*0.65, 3*0.65, r=True)

        #ik feet
        cmds.select("ik_foot_anim"+side + ".cv[0:32]")
        cmds.scale(1.7*0.65, 1.75*0.65, 1*0.65, r=True)
    
        #fingers
        fingers = ["index_finger_fk_ctrl_1", "index_finger_fk_ctrl_2", "middle_finger_fk_ctrl_1", "middle_finger_fk_ctrl_2", "ring_finger_fk_ctrl_1", "ring_finger_fk_ctrl_2", "pinky_finger_fk_ctrl_1", "pinky_finger_fk_ctrl_2", "thumb_finger_fk_ctrl_1", "thumb_finger_fk_ctrl_2"]
        for finger in fingers:
            cmds.select(finger+side+".cv[0:32]")
            cmds.scale(3.7*0.65, 3.7*0.65, 3.7*0.65, r=True)

    # Other
    cmds.select(["jaw_anim.cv[0:32]"])
    cmds.scale(0.2, 0.2, 0.2, r=True, ocp = True)
    cmds.move(0, -9, -6, r=True)

    cmds.select("tail_pad_anim" + ".cv[0:32]")
    cmds.scale(0.3, 0.3, 0.3, r=True)
    cmds.move(0, 3, 7.5, r=True)

    cmds.select("flap_back_anim" + ".cv[0:32]")
    cmds.scale(0.3, 0.3, 0.3, r=True)
    cmds.move(0, 5, 0, r=True)

    cmds.select("flap_front_anim" + ".cv[0:32]")
    cmds.scale(0.3, 0.3, 0.3, r=True)
    cmds.move(0, -7, 0, r=True)

    #turbine controls
    cmds.select("turbine_back_main_anim" + ".cv[0:32]")
    cmds.scale(1, 0.07, 1, r=True)
    cmds.move(0, 5.6, 0, r=True)

    cmds.select("turbine_back_inner_anim" + ".cv[0:32]")
    cmds.scale(0.75, 0.07, 0.75, r=True)
    cmds.move(0, -0.6, 0, r=True)

    #chest ik
    cmds.select("chest_ik_anim" + ".cv[0:32]")
    cmds.scale(1.5*0.65, 1.5*0.65, 1.5*0.65, r=True)
    
    #fk spine
    cmds.select("spine_01_anim" + ".cv[0:32]")
    cmds.scale(1.28*0.65, 1.28*0.65, 1.28*0.65, r=True)
        
    cmds.select("spine_02_anim" + ".cv[0:32]")
    cmds.scale(1.32*0.65, 1.32*0.65, 1.32*0.65, r=True)

    cmds.select("spine_03_anim" + ".cv[0:32]")
    cmds.scale(1.54*0.65, 1.54*0.65, 1.54*0.65, r=True)    
    
    #neck
    cmds.select("neck_01_fk_anim" + ".cv[0:32]")
    cmds.scale(2.3*0.65, 2.3*0.65, 2.3*0.65, r=True)
    
    # football pads
    cmds.select("football_pads_front_anim" + ".cv[0:32]")
    cmds.scale(0.2, 0.2, 0.2, r=True)
    cmds.move(0, 9, 16, r=True)

    cmds.select("football_pads_back_anim" + ".cv[0:32]")
    cmds.scale(0.2, 0.2, 0.2, r=True)
    cmds.move(0, 9, 16, r=True)

scaleControls()

# ------------------------------------------------------------------
def rigArms():
    cmds.setAttr("Rig_Settings.lUpperarmTwistAmount", 1)
    cmds.setAttr("Rig_Settings.lUpperarmTwist2Amount", 0.5)
    cmds.setAttr("Rig_Settings.rUpperarmTwistAmount", 1)
    cmds.setAttr("Rig_Settings.rUpperarmTwist2Amount", 0.5)    

    cmds.setAttr("Rig_Settings.lArmMode", 0)
    cmds.setAttr("Rig_Settings.lClavMode", 0)
    cmds.setAttr("Rig_Settings.rArmMode", 0)
    cmds.setAttr("Rig_Settings.rClavMode", 0)

    for side in ["_l", "_r"]:
        # Create SDK's to drive the shoulder blendshapes by thex upperarm joint
        cmds.setAttr("fk_arm"+side+"_anim.ry", 62)
        cmds.setDrivenKeyframe(["shoulders_geo_blendShape.shoulders_geo_shoulders_up"+side], cd="upperarm"+side+".rotateY", itt="flat", ott="flat")
        cmds.setDrivenKeyframe(["shoulders_geo_blendShape.shoulders_geo_shoulders_down"+side], cd="upperarm"+side+".rotateY", itt="flat", ott="flat")

        cmds.setAttr("fk_arm"+side+"_anim.ry", 0)
        cmds.setAttr("shoulders_geo_blendShape.shoulders_geo_shoulders_up"+side, 1)
        cmds.setDrivenKeyframe(["shoulders_geo_blendShape.shoulders_geo_shoulders_up"+side], cd="upperarm"+side+".rotateY", itt="flat", ott="flat")
        cmds.setDrivenKeyframe(["shoulders_geo_blendShape.shoulders_geo_shoulders_down"+side], cd="upperarm"+side+".rotateY", itt="flat", ott="flat")
        cmds.setAttr("shoulders_geo_blendShape.shoulders_geo_shoulders_up"+side, 0)

        cmds.setAttr("fk_arm"+side+"_anim.ry", 82)
        cmds.setAttr("shoulders_geo_blendShape.shoulders_geo_shoulders_down"+side, 1)
        cmds.setDrivenKeyframe(["shoulders_geo_blendShape.shoulders_geo_shoulders_up"+side], cd="upperarm"+side+".rotateY", itt="flat", ott="flat")
        cmds.setDrivenKeyframe(["shoulders_geo_blendShape.shoulders_geo_shoulders_down"+side], cd="upperarm"+side+".rotateY", itt="flat", ott="flat")
        cmds.setAttr("shoulders_geo_blendShape.shoulders_geo_shoulders_down"+side, 0)

        cmds.setAttr("fk_arm"+side+"_anim.ry", 0)

        # Create SDK's to drive the bicep blendshape by the elbow joint
        cmds.setAttr("fk_elbow"+side+"_anim.rz", 0)
        cmds.setAttr("shoulders_geo_blendShape.shoulders_geo_elbows_bent"+side, 0)
        cmds.setDrivenKeyframe(["shoulders_geo_blendShape.shoulders_geo_elbows_bent"+side], cd="lowerarm"+side+".rotateZ", itt="flat", ott="flat")
        
        cmds.setAttr("fk_elbow"+side+"_anim.rz", -100)
        cmds.setAttr("shoulders_geo_blendShape.shoulders_geo_elbows_bent"+side, 1)
        cmds.setDrivenKeyframe(["shoulders_geo_blendShape.shoulders_geo_elbows_bent"+side], cd="lowerarm"+side+".rotateZ", itt="flat", ott="flat")
        
        cmds.setAttr("fk_elbow"+side+"_anim.rz", 0)

    cmds.setAttr("Rig_Settings.lArmMode", 1)
    cmds.setAttr("Rig_Settings.lClavMode", 1)
    cmds.setAttr("Rig_Settings.rArmMode", 1)
    cmds.setAttr("Rig_Settings.rClavMode", 1)
rigArms()

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
addCapsule(64, 110)

# ------------------------------------------------------------------
# Add in a temporary face emote for the rabbit
def addFaceEmote():
   
    # morphs
    cmds.addAttr("head_fk_anim", ln = "face_emote", dv = 0, min = 0, max = 1, keyable = True)

    cmds.connectAttr("head_fk_anim.face_emote", "blendShape2.head_geo_emote")
    cmds.connectAttr("head_fk_anim.face_emote", "blendShape1.helmet_geo_emote")
    cmds.connectAttr("head_fk_anim.face_emote", "blendShape3.eye_shadow_geo_emote")

    cmds.select(None)
#addFaceEmote()

# ------------------------------------------------------------------
# add ik bones Function that is called later after we open the export file.
def addIKBones():
    try:
		# create the joints
		cmds.select(clear=True)
		ikFootRoot = cmds.joint(name = "ik_foot_root")
		cmds.select(clear=True)
		
		cmds.select(clear=True)
		ikFootLeft = cmds.joint(name = "ik_foot_l")
		cmds.select(clear=True)
		
		cmds.select(clear=True)
		ikFootRight = cmds.joint(name = "ik_foot_r")
		cmds.select(clear=True)
		
		cmds.select(clear=True)
		ikHandRoot = cmds.joint(name = "ik_hand_root")
		cmds.select(clear=True)
		
		cmds.select(clear=True)
		ikHandGun = cmds.joint(name = "ik_hand_gun")
		cmds.select(clear=True)
		
		cmds.select(clear=True)
		ikHandLeft = cmds.joint(name = "ik_hand_l")
		cmds.select(clear=True)
		
		cmds.select(clear=True)
		ikHandRight = cmds.joint(name = "ik_hand_r")
		cmds.select(clear=True)
		
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



#########################################
# RUN THIS COMMAND DURING THE RIG POSE CREATION STEP TO GET THE SHIELD TO THE CORRECT LOCATION.  LEAVE IT COMMENTED OUT SO THAT IT DOESNT RUN DURING RIGGING.
'''
#cmds.xform(q=True, ws=True, rp=True)
#cmds.xform(q=True, ro=True)
cmds.xform("sheild_main_mover", ws=True, t=[-0.018884450479987436, 59.40691643000001, 272.98635249999995])
cmds.xform("sheild_main_mover", ro=[-14.885476763734184, 61.20693988721698, -15.277192149145698])
'''
#########################################
















































