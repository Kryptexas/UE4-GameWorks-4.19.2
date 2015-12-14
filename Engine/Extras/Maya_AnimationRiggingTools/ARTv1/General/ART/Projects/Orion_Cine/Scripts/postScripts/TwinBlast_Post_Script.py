import maya.cmds as cmds
# NOTE: If you mirror his movers, lock the weapon controls first so that they dont mirror because they are set up to not.  And putting them back is a pain.
# NOTE: Use the "GUN_locator___USE_FOR_RIG_POSE" in the scene during the Create Rig Pose phase to put the weapon movers adn the grenade movers back to the correct location.

# ------------------------------------------------------------------
# scale all of the controls to be more visible with the mesh.
def scaleControls():
    sides = ["_l", "_r"]
    for side in sides:
        # feet
        cmds.select(["ik_foot_anim"+side+".cv[0:17]"])
        cmds.scale(1, 1, 1, r = True, ocp = False)
        cmds.move(0, 0, -4.78  , r = True)

        cmds.select(["q_weapon"+side+"_anim.cv[0:32]"])
        cmds.scale(0.4, 0.4, 0.4, r = True, ocp = False)
        if side == "_l":
            cmds.move(0, 20, 0, r = True)
        else:
            cmds.move(0, 20, 0, r = True)            

        cmds.select(["ult_weapon"+side+"_anim.cv[0:32]"])
        cmds.scale(0.5, 0.5, 0.5, r = True, ocp = False)
        if side == "_l":
            cmds.move(0, 25, 0, r = True)
        else:
            cmds.move(0, 25, 0, r = True)            

        cmds.select(["forearm_gears"+side+"_anim.cv[0:32]"])
        cmds.scale(0, 1, 1, r = True, ocp = False)

    # Other
    objects = ["weapon_forestock_bott_l_anim", "weapon_forestock_bott_r_anim", "weapon_forestock_top_l_anim", "weapon_forestock_top_r_anim", "weapon_l_anim", "weapon_r_anim", "weapon_stock_l_anim", "weapon_stock_r_anim", "low_lowarm_plate_A_l_anim", "low_lowarm_plate_B_l_anim", "low_lowarm_plate_C_l_anim", "low_lowarm_plate_D_l_anim", "up_lowarm_plate_A_l_anim", "up_lowarm_plate_B_l_anim", "up_lowarm_plate_C_l_anim", "up_lowarm_plate_D_l_anim", "grenade_anim", "grenade_front_anim", "grenade_fin_01_anim", "grenade_fin_02_anim", "grenade_fin_03_anim", "grenade_fin_04_anim", "grenade_fin_05_anim", "grenade_fin_06_anim", "collar_in_l_anim", "collar_out_l_anim", "collar_out_flap_bott_l_anim", "collar_out_flap_top_l_anim", "low_lowarm_plate_A_r_anim", "low_lowarm_plate_B_r_anim", "low_lowarm_plate_C_r_anim", "low_lowarm_plate_D_r_anim", "up_lowarm_plate_A_r_anim", "up_lowarm_plate_B_r_anim", "up_lowarm_plate_C_r_anim", "up_lowarm_plate_D_r_anim", "collar_in_r_anim", "collar_out_r_anim", "collar_out_flap_bott_r_anim", "collar_out_flap_top_r_anim", "dogtag_anim", "collar_back_anim", "collar_l_anim", "collar_r_anim"]
    for one in objects:
        cmds.select([one+".cv[*]"])
        cmds.scale(0.227057, 0.227057, 0.227057, r = True, ocp = True)
        cmds.move(0, 0, 0, r = True, os=True, wd=True)


    cmds.select(None)

scaleControls()

# ------------------------------------------------------------------
def rigWeapons():
    cmds.move(0, 4.4, 24.558962, "ult_weapon_l_anim", r=True, os=True, wd=True)
    cmds.scale(0.05, 0.05, 0.05, "ult_weapon_l_anim", r=True)

    cmds.move(0, -4.4, -24.558962, "ult_weapon_r_anim", r=True, os=True, wd=True)
    cmds.scale(0.05, 0.05, 0.05, "ult_weapon_r_anim", r=True)

    cmds.move(0, 4.4, 24.558962, "q_weapon_l_anim", r=True, os=True, wd=True)
    cmds.scale(0.05, 0.05, 0.05, "q_weapon_l_anim", r=True)

    cmds.move(0, -4.4, -24.558962, "q_weapon_r_anim", r=True, os=True, wd=True)
    cmds.scale(0.05, 0.05, 0.05, "q_weapon_r_anim", r=True)

    cmds.xform("weapon_l_anim_grp", t=[82.96438708999413, 10.607007776552306, 147.1500662787879], ro=[37.910151258451656, -65.27865699485243, 49.67785824895247])
    cmds.xform("weapon_r_anim_grp", t=[-82.96438708999413, 10.607007776552306, 147.1500662787879], ro=[37.910151258451656, 65.27865699485243, -49.67785824895247])
    cmds.xform("grenade_front_anim", t=[0, 0.4509, 0], ro=[0, 0, 0])
    cmds.xform("grenade_fin_01_anim", t=[0, 0, 0], ro=[-180, 0, 0])
    cmds.xform("grenade_fin_02_anim", t=[0, 0, 0], ro=[-180, 0, 0])
    cmds.xform("grenade_fin_03_anim", t=[0, 0, 0], ro=[-180, 0, 0])
    cmds.xform("grenade_fin_04_anim", t=[0, 0, 0], ro=[-180, 0, 0])
    cmds.xform("grenade_fin_05_anim", t=[0, 0, 0], ro=[-180, 0, 0])
    cmds.xform("grenade_fin_06_anim", t=[0, 0, 0], ro=[-180, 0, 0])
    cmds.xform("grenade_anim_grp", t=[-59.36194438801096, 12.504397019491371, 150.4395268893561], ro=[0.0, 0.0, -90])
    
    cmds.setAttr("Rig_Settings.lUpperarmTwistAmount", 0.9)
    cmds.setAttr("Rig_Settings.lUpperarmTwist2Amount", 0.3)
    cmds.setAttr("Rig_Settings.rUpperarmTwistAmount", 0.9)
    cmds.setAttr("Rig_Settings.rUpperarmTwist2Amount", 0.3)

    cmds.setAttr("Rig_Settings.lForearmTwistAmount", 1)
    cmds.setAttr("Rig_Settings.rForearmTwistAmount", 1)
    
rigWeapons()

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
addCapsule(42, 86)

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