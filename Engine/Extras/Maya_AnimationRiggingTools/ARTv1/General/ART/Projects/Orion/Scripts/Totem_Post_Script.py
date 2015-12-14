
#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
def hideAndResize():
    #hide stuff
    cmds.setAttr("chest_r_anim_grp.v", 0, lock = True)
    cmds.setAttr("chest_l_anim_grp.v", 0, lock = True)
    cmds.setAttr("hose_l_anim_grp.v", 0, lock = True)
    cmds.setAttr("hose_r_anim_grp.v", 0, lock = True)
    cmds.setAttr("collar_clav_l_anim_grp.v", 0, lock = True)
    cmds.setAttr("collar_clav_r_anim_grp.v", 0, lock = True)
    cmds.setAttr("hair_r_anim_grp.v", 0, lock = True)
    cmds.setAttr("hair_l_anim_grp.v", 0, lock = True)
    cmds.setAttr("fk_ponytail_01_lag_grp.v", 0, lock = True)
    cmds.setAttr("back_plate_anim_grp.v", 0, lock = True)
    cmds.setAttr("top_tip_b_anim_grp.v", 0, lock = True)
    cmds.setAttr("top_tip_a_anim_grp.v", 0, lock = True)
    cmds.setAttr("hinge_a_anim_grp.v", 0, lock = True)
    cmds.setAttr("hinge_b_anim_grp.v", 0, lock = True)
    cmds.setAttr("l_pad_end_anim_grp.v", 0, lock = True)
    cmds.setAttr("r_pad_end_anim_grp.v", 0, lock = True)
    cmds.setAttr("l_hose_base_anim_grp.v", 0, lock = True)
    cmds.setAttr("r_hose_base_anim_grp.v", 0, lock = True)
    cmds.setAttr("l_hose_end_anim_grp.v", 0, lock = True)
    cmds.setAttr("r_hose_end_anim_grp.v", 0, lock = True)

    #resize stuff
    cmds.select("shoulder_pad_l_anim.cv[*]")
    cmds.scale(0.35, 1.25, 0.35)

    cmds.select("shoulder_pad_r_anim.cv[*]")
    cmds.scale(0.35, 1.25, 0.35)

    cmds.select("glasses_anim.cv[*]")
    cmds.scale(0.17, .17, 0.17)

    cmds.select("knee_pad_l_anim.cv[*]")
    cmds.scale(0.3, .3, 0.3)

    cmds.select("knee_pad_r_anim.cv[*]")
    cmds.scale(0.3, .3, 0.3)

    cmds.select("weapon_l_anim.cv[*]")
    cmds.scale(0.0, 0.5, 0.5)
    
    cmds.select("weapon_anim.cv[*]")
    cmds.scale(0.4, 0.4, 0.4)
    
    cmds.select("hand_plate_l_anim.cv[*]")
    cmds.scale(0.4, 0.4, 0.0)
    cmds.move(0, 0, 1, r = True, os = True, ws = True)

    cmds.select("hand_plate_r_anim.cv[*]")
    cmds.scale(0.4, 0.4, 0.0)
    cmds.move(0, 0, 1, r = True, os = True, ws = True)

    cmds.select("top_hinge_anim.cv[*]")
    cmds.scale(0.0, 1.0, 1.0)

    cmds.select("staff_top_anim.cv[*]")
    cmds.scale(0.4, 0.4, 0.4)

    cmds.select("staff_bottom_anim.cv[*]")
    cmds.scale(0.4, 0.4, 0.4)

    cmds.select("handle_anim.cv[*]")
    cmds.scale(0.3, 0.3, 0.3)

    cmds.select("tip_spin_anim.cv[*]")
    cmds.scale(1, 0.0, 1)
    
    
    #lock stuff
    for attr in [".tx", ".ty", ".tz", ".ry", ".rz", ".sx", ".sy", ".sz"]:
        cmds.setAttr("top_hinge_anim" + attr, lock = True, keyable = False)
    

#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
def weaponRig():
    #weapon SDKs

    #add attrs
    cmds.addAttr("top_hinge_anim", ln = "tip_a_open", keyable = True, dv = 0, min = 0, max = 10)
    cmds.addAttr("top_hinge_anim", ln = "tip_b_open", keyable = True, dv = 0, min = 0, max = 10)
    cmds.addAttr("top_hinge_anim", ln = "tip_a_hinge", keyable = True, dv = 0, min = 0, max = 10)
    cmds.addAttr("top_hinge_anim", ln = "tip_b_hinge", keyable = True, dv = 0, min = 0, max = 10)
    cmds.addAttr("top_hinge_anim", ln = "reload", keyable = True, dv = 0, min = 0, max = 10)
    cmds.addAttr("top_hinge_anim", ln = "bottom_a_open", keyable = True, dv = 0)
    cmds.addAttr("top_hinge_anim", ln = "bottom_b_open", keyable = True, dv = 0)
    cmds.addAttr("top_hinge_anim", ln = "pieceSlider", keyable = True, dv = 0, min = 0, max = 10)
    cmds.addAttr("top_hinge_anim", ln = "coreScale", keyable = True, dv = 1, min = 0, max = 1)


    #core scale
    cmds.setAttr("top_hinge_anim.coreScale", 1)
    cmds.setAttr("tri_weapon_corrective.core_shrink", 0)
    cmds.setDrivenKeyframe("tri_weapon_corrective.core_shrink", cd = "top_hinge_anim.coreScale")


    cmds.setAttr("top_hinge_anim.coreScale", 0)
    cmds.setAttr("tri_weapon_corrective.core_shrink", 1)
    cmds.setDrivenKeyframe("tri_weapon_corrective.core_shrink", cd = "top_hinge_anim.coreScale")
    cmds.setAttr("top_hinge_anim.coreScale", 1)


    #piece morph
    cmds.setAttr("top_hinge_anim.pieceSlider", 0)
    cmds.setAttr("tri_weapon_corrective.weapon_piece_move", 0)
    cmds.setDrivenKeyframe("tri_weapon_corrective.weapon_piece_move", cd = "top_hinge_anim.pieceSlider")
    
    cmds.setAttr("top_hinge_anim.pieceSlider", 10)
    cmds.setAttr("tri_weapon_corrective.weapon_piece_move", 1)
    cmds.setDrivenKeyframe("tri_weapon_corrective.weapon_piece_move", cd = "top_hinge_anim.pieceSlider")
    cmds.setAttr("top_hinge_anim.pieceSlider", 0)
    
    #tip a hinge
    cmds.setAttr("top_hinge_anim.tip_a_hinge", 0)
    cmds.setAttr("top_tip_a_anim_grp.rx", 0)
    cmds.setDrivenKeyframe("top_tip_a_anim_grp.rx", cd = "top_hinge_anim.tip_a_hinge")
    
    
    cmds.setAttr("top_hinge_anim.tip_a_hinge", 10)
    cmds.setAttr("top_tip_a_anim_grp.rx", -45)

    cmds.setDrivenKeyframe("top_tip_a_anim_grp.rx", cd = "top_hinge_anim.tip_a_hinge")    
    cmds.setAttr("top_hinge_anim.tip_a_hinge", 0)
    
    
    #tip b hinge
    cmds.setAttr("top_hinge_anim.tip_b_hinge", 0)
    cmds.setAttr("top_tip_b_anim_grp.rx", 0)
    cmds.setDrivenKeyframe("top_tip_b_anim_grp.rx", cd = "top_hinge_anim.tip_b_hinge")
    
    
    cmds.setAttr("top_hinge_anim.tip_b_hinge", 10)
    cmds.setAttr("top_tip_b_anim_grp.rx", 45)

    cmds.setDrivenKeyframe("top_tip_b_anim_grp.rx", cd = "top_hinge_anim.tip_b_hinge")    
    cmds.setAttr("top_hinge_anim.tip_b_hinge", 0)
    
    
    #tip a open
    cmds.setAttr("top_hinge_anim.tip_a_open", 0)
    cmds.setAttr("top_tip_a_anim_grp.rx", 0)
    cmds.setAttr("top_tip_a_anim_grp.ty", 0)
    cmds.setAttr("top_tip_a_anim_grp.tz", 0)
    cmds.setDrivenKeyframe(["top_tip_a_anim_grp.rx", "top_tip_a_anim_grp.ty", "top_tip_a_anim_grp.tz"], cd = "top_hinge_anim.tip_a_open")

    cmds.setAttr("top_hinge_anim.tip_a_open", 10)
    cmds.setAttr("top_tip_a_anim_grp.rx", -8.388)
    cmds.setAttr("top_tip_a_anim_grp.ty", -0.134)
    cmds.setAttr("top_tip_a_anim_grp.tz", 5.166)
    cmds.setDrivenKeyframe(["top_tip_a_anim_grp.rx", "top_tip_a_anim_grp.ty", "top_tip_a_anim_grp.tz"], cd = "top_hinge_anim.tip_a_open")
    cmds.setAttr("top_hinge_anim.tip_a_open", 0)

    #tip b open
    cmds.setAttr("top_hinge_anim.tip_b_open", 0)
    cmds.setAttr("top_tip_b_anim_grp.rx", 0)
    cmds.setAttr("top_tip_b_anim_grp.ty", 0)
    cmds.setAttr("top_tip_b_anim_grp.tz", 0)
    cmds.setDrivenKeyframe(["top_tip_b_anim_grp.rx", "top_tip_b_anim_grp.ty", "top_tip_b_anim_grp.tz"], cd = "top_hinge_anim.tip_b_open")

    cmds.setAttr("top_hinge_anim.tip_b_open", 10)
    cmds.setAttr("top_tip_b_anim_grp.rx", 5)
    cmds.setAttr("top_tip_b_anim_grp.ty", 1.923)
    cmds.setAttr("top_tip_b_anim_grp.tz", -4.408)
    cmds.setDrivenKeyframe(["top_tip_b_anim_grp.rx", "top_tip_b_anim_grp.ty", "top_tip_b_anim_grp.tz"], cd = "top_hinge_anim.tip_b_open")
    cmds.setAttr("top_hinge_anim.tip_b_open", 0)


    #reload
    cmds.setAttr("top_hinge_anim.reload", 0)
    cmds.setAttr("weapon_anim_grp.rx", 0)
    cmds.setAttr("weapon_anim_grp.tx", 0)
    cmds.setAttr("weapon_anim_grp.ty", 0)
    cmds.setAttr("weapon_anim_grp.tz", 0)
    cmds.setDrivenKeyframe(["weapon_anim_grp.rx", "weapon_anim_grp.tx", "weapon_anim_grp.ty", "weapon_anim_grp.tz"], cd = "top_hinge_anim.reload")

    cmds.setAttr("top_hinge_anim.reload", 10)
    cmds.setAttr("weapon_anim_grp.rx", 0)
    cmds.setAttr("weapon_anim_grp.tx", 0)
    cmds.setAttr("weapon_anim_grp.ty", -9.631)
    cmds.setAttr("weapon_anim_grp.tz", 0.379)
    cmds.setDrivenKeyframe(["weapon_anim_grp.rx", "weapon_anim_grp.tx", "weapon_anim_grp.ty", "weapon_anim_grp.tz"], cd = "top_hinge_anim.reload")
    cmds.setAttr("top_hinge_anim.reload", 0)

    #bottom open a
    cmds.connectAttr("top_hinge_anim.bottom_a_open", "hinge_a_anim_grp.rx")
    #bottom open b
    cmds.connectAttr("top_hinge_anim.bottom_b_open", "hinge_b_anim_grp.rx")
    
    
    #TEXT
    shape = cmds.listRelatives("top_hinge_anim", shapes = True)[0]

    text = cmds.textCurves(ch = 0, f = "Times New Roman|h-13|w400|c0", t = "W")[0]
    cmds.setAttr(text + ".rz", 90)
    cmds.setAttr(text + ".scale", 9,9,9, type = "double3")

    cmds.select(text, hi = True)
    textNodes = cmds.ls(sl = True)

    for node in textNodes:
        textShape = cmds.listRelatives(node, shapes = True)
        if textShape != None:
            cmds.parent(node, world = True)
            cmds.makeIdentity(node, t = 1, r = 1, s = 1, apply = True)
            
            cmds.parent(textShape[0], "top_hinge_anim", shape = True, r = True)
            cmds.parent(shape, shape = True, rm = True)
            cmds.delete(node)
            cmds.delete(text)
        


#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
def setupShoulderRig():
    
    #Set Settings
    cmds.setAttr("Rig_Settings.lArmMode", 0)
    cmds.setAttr("Rig_Settings.lClavMode", 0)
    cmds.setAttr("Rig_Settings.rArmMode", 0)
    cmds.setAttr("Rig_Settings.rClavMode", 0)
    
    # LEFT UP OFF
    cmds.setAttr("fk_clavicle_l_anim.rx", 0)
    cmds.setAttr("fk_clavicle_l_anim.ry", 0)
    cmds.setAttr("fk_clavicle_l_anim.rz", 0)
    
    cmds.setAttr("collar_clav_l_anim_grp.tx", 0)
    cmds.setAttr("collar_clav_l_anim_grp.ty", 0)
    cmds.setAttr("collar_clav_l_anim_grp.tz", 0)
    cmds.setAttr("collar_clav_l_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_l_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_l_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", 0)
    
    cmds.setAttr("back_plate_anim_grp.ty", 0)
    cmds.setAttr("back_plate_anim_grp.rz", 0)
    
    cmds.setAttr("shoulder_pad_l_anim_grp.ry", 0)


    driven = ["collar_clav_l_anim_grp.tx", "collar_clav_l_anim_grp.ty", "collar_clav_l_anim_grp.tz"]
    driven.extend(["collar_clav_l_anim_grp.rx", "collar_clav_l_anim_grp.ry", "collar_clav_l_anim_grp.rz"])
    driven.extend(["collar_anim_grp.tz", "back_plate_anim_grp.ty", "back_plate_anim_grp.rz", "shoulder_pad_l_anim_grp.ry"])
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.lArmUpAngle", itt = "spline", ott = "spline")
    
    
    #LEFT UP ON
    cmds.setAttr("fk_clavicle_l_anim.rx", 0)
    cmds.setAttr("fk_clavicle_l_anim.ry", -53.513)
    cmds.setAttr("fk_clavicle_l_anim.rz", 0)
    
    cmds.setAttr("collar_clav_l_anim_grp.tx", 0)
    cmds.setAttr("collar_clav_l_anim_grp.ty", 0)
    cmds.setAttr("collar_clav_l_anim_grp.tz", 2.962)
    cmds.setAttr("collar_clav_l_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_l_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_l_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", 1.71)
    
    cmds.setAttr("back_plate_anim_grp.ty", 0)
    cmds.setAttr("back_plate_anim_grp.rz", 0)
    
    cmds.setAttr("shoulder_pad_l_anim_grp.ry", 50)
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.lArmUpAngle", itt = "spline", ott = "spline")
    
    cmds.selectKey("shoulder_pad_l_anim_grp_rotateY")
    cmds.setInfinity(pri = "constant", poi = "linear")
    
    cmds.setAttr("fk_clavicle_l_anim.rx", 0)
    cmds.setAttr("fk_clavicle_l_anim.ry", 0)
    cmds.setAttr("fk_clavicle_l_anim.rz", 0)
    
    
    
    # LEFT FORWARD OFF
    cmds.setAttr("fk_clavicle_l_anim.rx", 0)
    cmds.setAttr("fk_clavicle_l_anim.ry", 0)
    cmds.setAttr("fk_clavicle_l_anim.rz", 0)
    
    cmds.setAttr("collar_clav_l_anim_grp.tx", 0)
    cmds.setAttr("collar_clav_l_anim_grp.ty", 0)
    cmds.setAttr("collar_clav_l_anim_grp.tz", 0)
    cmds.setAttr("collar_clav_l_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_l_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_l_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", 0)
    
    cmds.setAttr("back_plate_anim_grp.ty", 0)
    cmds.setAttr("back_plate_anim_grp.rz", 0)
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.lArmForAngle", itt = "spline", ott = "spline")
    
    
    # LEFT FORWARD ON
    cmds.setAttr("fk_clavicle_l_anim.rx", 0)
    cmds.setAttr("fk_clavicle_l_anim.ry", 0)
    cmds.setAttr("fk_clavicle_l_anim.rz", -54.225)
    
    cmds.setAttr("collar_clav_l_anim_grp.tx", .78)
    cmds.setAttr("collar_clav_l_anim_grp.ty", -1.46)
    cmds.setAttr("collar_clav_l_anim_grp.tz", .52)
    cmds.setAttr("collar_clav_l_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_l_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_l_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", .34)
    
    cmds.setAttr("back_plate_anim_grp.ty", 0)
    cmds.setAttr("back_plate_anim_grp.rz", 0)
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.lArmForAngle", itt = "spline", ott = "spline")
    cmds.setAttr("fk_clavicle_l_anim.rx", 0)
    cmds.setAttr("fk_clavicle_l_anim.ry", 0)
    cmds.setAttr("fk_clavicle_l_anim.rz", 0)
    
    # LEFT BACK OFF
    cmds.setAttr("fk_clavicle_l_anim.rx", 0)
    cmds.setAttr("fk_clavicle_l_anim.ry", 0)
    cmds.setAttr("fk_clavicle_l_anim.rz", 0)
    
    cmds.setAttr("collar_clav_l_anim_grp.tx", 0)
    cmds.setAttr("collar_clav_l_anim_grp.ty", 0)
    cmds.setAttr("collar_clav_l_anim_grp.tz", 0)
    cmds.setAttr("collar_clav_l_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_l_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_l_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", 0)
    
    cmds.setAttr("back_plate_anim_grp.ty", 0)
    cmds.setAttr("back_plate_anim_grp.rz", 0)
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.lArmBackAngle", itt = "spline", ott = "spline")
    
    
    # LEFT BACK ON
    cmds.setAttr("fk_clavicle_l_anim.rx", 0)
    cmds.setAttr("fk_clavicle_l_anim.ry", 0)
    cmds.setAttr("fk_clavicle_l_anim.rz", 37.417)
    
    cmds.setAttr("collar_clav_l_anim_grp.tx", .637)
    cmds.setAttr("collar_clav_l_anim_grp.ty", 5.048)
    cmds.setAttr("collar_clav_l_anim_grp.tz", 0)
    cmds.setAttr("collar_clav_l_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_l_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_l_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", 0)
    
    cmds.setAttr("back_plate_anim_grp.ty", 2.522)
    cmds.setAttr("back_plate_anim_grp.rz", 4.665)
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.lArmBackAngle", itt = "spline", ott = "spline")
    cmds.setAttr("fk_clavicle_l_anim.rx", 0)
    cmds.setAttr("fk_clavicle_l_anim.ry", 0)
    cmds.setAttr("fk_clavicle_l_anim.rz", 0)
    
    
    # SHOULDER PAD
    cmds.setAttr("fk_arm_l_anim.ry", 30)
    cmds.setAttr("shoulder_pad_l_anim_grp.ry", 0)
    cmds.setDrivenKeyframe("shoulder_pad_l_anim_grp.ry", cd = "angle_dimensions.lArmDownAngle", itt = "spline", ott = "spline")
    
    cmds.setAttr("fk_arm_l_anim.ry", 0)
    cmds.setAttr("shoulder_pad_l_anim_grp.ry", 27.895)
    cmds.setDrivenKeyframe("shoulder_pad_l_anim_grp.ry", cd = "angle_dimensions.lArmDownAngle", itt = "spline", ott = "spline")
    
    
    for each in ["collar_clav_l_anim_grp", "shoulder_pad_l_anim_grp", "collar_anim_grp"]:
        blendWeighht = cmds.listConnections(each, skipConversionNodes = True, type = "blendWeighted")
        if blendWeighht != None:
            curves = cmds.listConnections(blendWeighht[0], skipConversionNodes = True, type = "animCurve")
            
            for curve in curves:
                cmds.selectKey(curve)
                if each != "shoulder_pad_l_anim_grp":
                    cmds.setInfinity(pri = "linear", poi = "linear")
                else:
                    cmds.setInfinity(pri = "constant", poi = "linear")

    
    
    #SHOULDER PAD END
    cmds.setAttr("fk_arm_l_anim.ry", 30)
    cmds.setAttr("l_pad_end_anim_grp.ry", 0)
    cmds.setAttr("tri_shoulder_pad_correctives.l_shoulder_pad_collapse",0)
    cmds.setDrivenKeyframe(["l_pad_end_anim_grp.ry", "tri_shoulder_pad_correctives.l_shoulder_pad_collapse"], cd = "shoulder_pad_l_anim_grp.ry", itt = "spline", ott = "spline")
    
    cmds.setAttr("fk_arm_l_anim.ry", 0)
    cmds.setAttr("l_pad_end_anim_grp.ry", -27.895)
    cmds.setAttr("tri_shoulder_pad_correctives.l_shoulder_pad_collapse",.4)
    cmds.setDrivenKeyframe(["l_pad_end_anim_grp.ry", "tri_shoulder_pad_correctives.l_shoulder_pad_collapse"], cd = "shoulder_pad_l_anim_grp.ry", itt = "spline", ott = "spline")
    
    
    cmds.selectKey("l_pad_end_anim_grp_rotateY")
    cmds.selectKey("tri_shoulder_pad_correctives_l_shoulder_pad_collapse", add = True)
    cmds.setInfinity(pri = "constant", poi = "linear")
    
    
    
    
    # RIGHT UP OFF
    cmds.setAttr("fk_clavicle_r_anim.rx", 0)
    cmds.setAttr("fk_clavicle_r_anim.ry", 0)
    cmds.setAttr("fk_clavicle_r_anim.rz", 0)
    
    cmds.setAttr("collar_clav_r_anim_grp.tx", 0)
    cmds.setAttr("collar_clav_r_anim_grp.ty", 0)
    cmds.setAttr("collar_clav_r_anim_grp.tz", 0)
    cmds.setAttr("collar_clav_r_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_r_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_r_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", 0)
    
    cmds.setAttr("back_plate_anim_grp.ty", 0)
    cmds.setAttr("back_plate_anim_grp.rz", 0)
    
    cmds.setAttr("shoulder_pad_r_anim_grp.ry", 0)


    driven = ["collar_clav_r_anim_grp.tx", "collar_clav_r_anim_grp.ty", "collar_clav_r_anim_grp.tz"]
    driven.extend(["collar_clav_r_anim_grp.rx", "collar_clav_r_anim_grp.ry", "collar_clav_r_anim_grp.rz"])
    driven.extend(["collar_anim_grp.tz", "back_plate_anim_grp.ty", "back_plate_anim_grp.rz", "shoulder_pad_r_anim_grp.ry"])
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.rArmUpAngle", itt = "spline", ott = "spline")
    
    
    #RIGHT UP ON
    cmds.setAttr("fk_clavicle_r_anim.rx", 0)
    cmds.setAttr("fk_clavicle_r_anim.ry", -53.513)
    cmds.setAttr("fk_clavicle_r_anim.rz", 0)
    
    cmds.setAttr("collar_clav_r_anim_grp.tx", 0)
    cmds.setAttr("collar_clav_r_anim_grp.ty", 0)
    cmds.setAttr("collar_clav_r_anim_grp.tz", -2.962)
    cmds.setAttr("collar_clav_r_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_r_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_r_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", 1.71)
    
    cmds.setAttr("back_plate_anim_grp.ty", 0)
    cmds.setAttr("back_plate_anim_grp.rz", 0)
    
    cmds.setAttr("shoulder_pad_r_anim_grp.ry", 50)
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.rArmUpAngle", itt = "spline", ott = "spline")
    
    cmds.selectKey("shoulder_pad_r_anim_grp_rotateY")
    cmds.setInfinity(pri = "constant", poi = "linear")
    
    cmds.setAttr("fk_clavicle_r_anim.rx", 0)
    cmds.setAttr("fk_clavicle_r_anim.ry", 0)
    cmds.setAttr("fk_clavicle_r_anim.rz", 0)
    
    
    
    # RIGHT FORWARD OFF
    cmds.setAttr("fk_clavicle_r_anim.rx", 0)
    cmds.setAttr("fk_clavicle_r_anim.ry", 0)
    cmds.setAttr("fk_clavicle_r_anim.rz", 0)
    
    cmds.setAttr("collar_clav_r_anim_grp.tx", 0)
    cmds.setAttr("collar_clav_r_anim_grp.ty", 0)
    cmds.setAttr("collar_clav_r_anim_grp.tz", 0)
    cmds.setAttr("collar_clav_r_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_r_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_r_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", 0)
    
    cmds.setAttr("back_plate_anim_grp.ty", 0)
    cmds.setAttr("back_plate_anim_grp.rz", 0)
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.rArmForAngle", itt = "spline", ott = "spline")
    
    
    # RIGHT FORWARD ON
    cmds.setAttr("fk_clavicle_r_anim.rx", 0)
    cmds.setAttr("fk_clavicle_r_anim.ry", 0)
    cmds.setAttr("fk_clavicle_r_anim.rz", -54.225)
    
    cmds.setAttr("collar_clav_r_anim_grp.tx", -.78)
    cmds.setAttr("collar_clav_r_anim_grp.ty", 1.46)
    cmds.setAttr("collar_clav_r_anim_grp.tz", -.52)
    cmds.setAttr("collar_clav_r_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_r_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_r_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", .34)
    
    cmds.setAttr("back_plate_anim_grp.ty", 0)
    cmds.setAttr("back_plate_anim_grp.rz", 0)
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.rArmForAngle", itt = "spline", ott = "spline")
    cmds.setAttr("fk_clavicle_r_anim.rx", 0)
    cmds.setAttr("fk_clavicle_r_anim.ry", 0)
    cmds.setAttr("fk_clavicle_r_anim.rz", 0)
    
    # RIGHT BACK OFF
    cmds.setAttr("fk_clavicle_r_anim.rx", 0)
    cmds.setAttr("fk_clavicle_r_anim.ry", 0)
    cmds.setAttr("fk_clavicle_r_anim.rz", 0)
    
    cmds.setAttr("collar_clav_r_anim_grp.tx", 0)
    cmds.setAttr("collar_clav_r_anim_grp.ty", 0)
    cmds.setAttr("collar_clav_r_anim_grp.tz", 0)
    cmds.setAttr("collar_clav_r_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_r_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_r_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", 0)
    
    cmds.setAttr("back_plate_anim_grp.ty", 0)
    cmds.setAttr("back_plate_anim_grp.rz", 0)
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.rArmBackAngle", itt = "spline", ott = "spline")
    
    
    # RIGHT BACK ON
    cmds.setAttr("fk_clavicle_r_anim.rx", 0)
    cmds.setAttr("fk_clavicle_r_anim.ry", 0)
    cmds.setAttr("fk_clavicle_r_anim.rz", 37.417)
    
    cmds.setAttr("collar_clav_r_anim_grp.tx", -.637)
    cmds.setAttr("collar_clav_r_anim_grp.ty", -5.048)
    cmds.setAttr("collar_clav_r_anim_grp.tz", 0)
    cmds.setAttr("collar_clav_r_anim_grp.rx", 0)
    cmds.setAttr("collar_clav_r_anim_grp.ry", 0)
    cmds.setAttr("collar_clav_r_anim_grp.rz", 0)
    
    cmds.setAttr("collar_anim_grp.tz", 0)
    
    cmds.setAttr("back_plate_anim_grp.ty", 2.522)
    cmds.setAttr("back_plate_anim_grp.rz", -4.665)
    
    cmds.setDrivenKeyframe(driven, cd = "angle_dimensions.rArmBackAngle", itt = "spline", ott = "spline")
    cmds.setAttr("fk_clavicle_r_anim.rx", 0)
    cmds.setAttr("fk_clavicle_r_anim.ry", 0)
    cmds.setAttr("fk_clavicle_r_anim.rz", 0)
    
    
    # SHOULDER PAD
    cmds.setAttr("fk_arm_r_anim.ry", 30)
    cmds.setAttr("shoulder_pad_r_anim_grp.ry", 0)
    cmds.setDrivenKeyframe("shoulder_pad_r_anim_grp.ry", cd = "angle_dimensions.rArmDownAngle", itt = "spline", ott = "spline")
    
    cmds.setAttr("fk_arm_r_anim.ry", 0)
    cmds.setAttr("shoulder_pad_r_anim_grp.ry", 27.895)
    cmds.setDrivenKeyframe("shoulder_pad_r_anim_grp.ry", cd = "angle_dimensions.rArmDownAngle", itt = "spline", ott = "spline")
    
    
    for each in ["collar_clav_r_anim_grp", "shoulder_pad_r_anim_grp", "collar_anim_grp"]:
        blendWeighht = cmds.listConnections(each, skipConversionNodes = True, type = "blendWeighted")
        if blendWeighht != None:
            curves = cmds.listConnections(blendWeighht[0], skipConversionNodes = True, type = "animCurve")
            
            for curve in curves:
                cmds.selectKey(curve)
                if each != "shoulder_pad_r_anim_grp":
                    cmds.setInfinity(pri = "linear", poi = "linear")
                else:
                    cmds.setInfinity(pri = "constant", poi = "linear")

    
    
    #SHOULDER PAD END
    cmds.setAttr("fk_arm_r_anim.ry", 30)
    cmds.setAttr("r_pad_end_anim_grp.ry", 0)
    cmds.setAttr("tri_shoulder_pad_correctives.r_shoulder_pad_collapse",0)
    cmds.setDrivenKeyframe(["r_pad_end_anim_grp.ry", "tri_shoulder_pad_correctives.r_shoulder_pad_collapse"], cd = "shoulder_pad_r_anim_grp.ry", itt = "spline", ott = "spline")
    
    cmds.setAttr("fk_arm_r_anim.ry", 0)
    cmds.setAttr("r_pad_end_anim_grp.ry", -27.895)
    cmds.setAttr("tri_shoulder_pad_correctives.r_shoulder_pad_collapse",.4)
    cmds.setDrivenKeyframe(["r_pad_end_anim_grp.ry", "tri_shoulder_pad_correctives.r_shoulder_pad_collapse"], cd = "shoulder_pad_r_anim_grp.ry", itt = "spline", ott = "spline")
    
    cmds.selectKey("r_pad_end_anim_grp_rotateY")
    cmds.selectKey("tri_shoulder_pad_correctives_r_shoulder_pad_collapse", add = True)
    cmds.setInfinity(pri = "constant", poi = "linear")
    
    
    #Set Settings
    cmds.setAttr("Rig_Settings.lArmMode", 1)
    cmds.setAttr("Rig_Settings.rArmMode", 1)
    
#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
def setupHoses():
    
    #Left Side
    cmds.aimConstraint("l_hose_base_anim", "l_hose_end_anim", aimVector = [-1, 0, 0], upVector = [0,0,1], worldUpType = "scene")
    cmds.aimConstraint("l_hose_end_anim", "l_hose_base_anim_grp", aimVector = [1, 0, 0], upVector = [0,0,1], worldUpType = "scene")
    cmds.pointConstraint(["l_hose_base_anim", "l_hose_end_anim"], "hose_l_anim")
    cmds.aimConstraint("l_hose_end_anim", "hose_l_anim", aimVector = [1, 0, 0], upVector = [0,0,1], worldUpType = "scene")
    
    
    #Right Side
    cmds.aimConstraint("r_hose_base_anim", "r_hose_end_anim", aimVector = [1, 0, 0], upVector = [0,0,-1], worldUpType = "scene")
    cmds.aimConstraint("r_hose_end_anim", "r_hose_base_anim_grp", aimVector = [-1, 0, 0], upVector = [0,0,-1], worldUpType = "scene")
    cmds.pointConstraint(["r_hose_base_anim", "r_hose_end_anim"], "hose_r_anim")
    cmds.aimConstraint("r_hose_end_anim", "hose_r_anim", aimVector = [-1, 0, 0], upVector = [0,0,-1], worldUpType = "scene")
    
    
    
#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
def dynamicsOff():

        cmds.setAttr("ponytail_dyn_anim.chainStartEnvelope", 0)
        cmds.setAttr("ponytail_dyn_anim.chainStartEnvelope", l=True)


#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
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


#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
def setupKnees():
    
    #L LEG
    cmds.setAttr("ik_foot_anim_l.tz", 0)
    cmds.setAttr("knee_pad_l_anim_grp.tx", 0)
    cmds.setAttr("knee_pad_l_anim_grp.ty", 0)
    cmds.setAttr("knee_pad_l_anim_grp.tz", 0)
    cmds.setAttr("knee_pad_l_anim_grp.rx", 0)
    cmds.setAttr("knee_pad_l_anim_grp.ry", 0)
    cmds.setAttr("knee_pad_l_anim_grp.rz", 0)
    
    cmds.setDrivenKeyframe(["knee_pad_l_anim_grp.tx", "knee_pad_l_anim_grp.ty", "knee_pad_l_anim_grp.tz", "knee_pad_l_anim_grp.rx", "knee_pad_l_anim_grp.ry", "knee_pad_l_anim_grp.rz"], cd = "angle_dimensions.knee_l_bend_angle", itt = "spline", ott = "spline")

    cmds.setAttr("ik_foot_anim_l.tz", 66)
    cmds.setAttr("knee_pad_l_anim_grp.tx", 0)
    cmds.setAttr("knee_pad_l_anim_grp.ty", 3.74)
    cmds.setAttr("knee_pad_l_anim_grp.tz", .914)
    cmds.setAttr("knee_pad_l_anim_grp.rx", -53.492)
    cmds.setAttr("knee_pad_l_anim_grp.ry", 0)
    cmds.setAttr("knee_pad_l_anim_grp.rz", 0)
    
    cmds.setDrivenKeyframe(["knee_pad_l_anim_grp.tx", "knee_pad_l_anim_grp.ty", "knee_pad_l_anim_grp.tz", "knee_pad_l_anim_grp.rx", "knee_pad_l_anim_grp.ry", "knee_pad_l_anim_grp.rz"], cd = "angle_dimensions.knee_l_bend_angle", itt = "spline", ott = "spline")

    blendWeighht = cmds.listConnections("knee_pad_l_anim_grp", skipConversionNodes = True, type = "blendWeighted")
    if blendWeighht != None:
        curves = cmds.listConnections(blendWeighht[0], skipConversionNodes = True, type = "animCurve")
        
        for curve in curves:
            cmds.selectKey(curve)
            cmds.setInfinity(pri = "constant", poi = "linear")
    
    cmds.setAttr("ik_foot_anim_l.tz", 0)
                
                
    
    #R LEG
    cmds.setAttr("ik_foot_anim_r.tz", 0)
    cmds.setAttr("knee_pad_r_anim_grp.tx", 0)
    cmds.setAttr("knee_pad_r_anim_grp.ty", 0)
    cmds.setAttr("knee_pad_r_anim_grp.tz", 0)
    cmds.setAttr("knee_pad_r_anim_grp.rx", 0)
    cmds.setAttr("knee_pad_r_anim_grp.ry", 0)
    cmds.setAttr("knee_pad_r_anim_grp.rz", 0)
    
    cmds.setDrivenKeyframe(["knee_pad_r_anim_grp.tx", "knee_pad_r_anim_grp.ty", "knee_pad_r_anim_grp.tz", "knee_pad_r_anim_grp.rx", "knee_pad_r_anim_grp.ry", "knee_pad_r_anim_grp.rz"], cd = "angle_dimensions.knee_r_bend_angle", itt = "spline", ott = "spline")

    cmds.setAttr("ik_foot_anim_r.tz", 66)
    cmds.setAttr("knee_pad_r_anim_grp.tx", 0)
    cmds.setAttr("knee_pad_r_anim_grp.ty", -3.74)
    cmds.setAttr("knee_pad_r_anim_grp.tz", -.914)
    cmds.setAttr("knee_pad_r_anim_grp.rx", -53.492)
    cmds.setAttr("knee_pad_r_anim_grp.ry", 0)
    cmds.setAttr("knee_pad_r_anim_grp.rz", 0)
    
    cmds.setDrivenKeyframe(["knee_pad_r_anim_grp.tx", "knee_pad_r_anim_grp.ty", "knee_pad_r_anim_grp.tz", "knee_pad_r_anim_grp.rx", "knee_pad_r_anim_grp.ry", "knee_pad_r_anim_grp.rz"], cd = "angle_dimensions.knee_r_bend_angle", itt = "spline", ott = "spline")

    blendWeighht = cmds.listConnections("knee_pad_r_anim_grp", skipConversionNodes = True, type = "blendWeighted")
    if blendWeighht != None:
        curves = cmds.listConnections(blendWeighht[0], skipConversionNodes = True, type = "animCurve")
        
        for curve in curves:
            cmds.selectKey(curve)
            cmds.setInfinity(pri = "constant", poi = "linear")
    cmds.setAttr("ik_foot_anim_r.tz", 0)
    
#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
def createGameCam():
    gameCam = cmds.camera(centerOfInterest=5, focalLength=18, lensSqueezeRatio=1, cameraScale=1, horizontalFilmAperture=1.41732, horizontalFilmOffset=0, verticalFilmAperture=0.94488, verticalFilmOffset=0, filmFit="Overscan", overscan=1.05, motionBlur=0, shutterAngle=144, nearClipPlane=1.0, farClipPlane=10000, orthographic=0, orthographicWidth=30, panZoomEnabled=0, horizontalPan=0, verticalPan=0, zoom=1, displayFilmGate=True)[0]

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
    
#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
def addCustomAttrs():
    attrs = cmds.listAttr("root")
    exists = False

    for each in attrs:
        if each.find("DistanceCurve") != -1:
            exists = True
    if exists == False:
        cmds.addAttr("root", ln="DistanceCurve", keyable=True)
        
        
#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################


def addIKtoExport():
    
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

    #save the export file.
    cmds.file(save = True, type = "mayaBinary", force = True)


    #delete stuff from export file
    cmds.delete("Totem_Geo")
    cmds.select(["correctives", "rig_pose_locs"], hi = True)
    selection = cmds.ls(sl = True)
    for each in selection:
        cmds.lockNode(each, lock = False)
    cmds.select(clear = True)

    cmds.delete("correctives")
    cmds.delete("rig_pose_locs")

    #save the export file.
    cmds.file(save = True, type = "mayaBinary", force = True)


#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
#run functions
hideAndResize()
weaponRig()
setupShoulderRig()
setupHoses()
setupKnees()

#common
dynamicsOff()
addCapsule(42, 86)
createGameCam()
addCustomAttrs()
addIKtoExport()


