import maya.cmds as cmds
import maya.mel as mel

# ------------------------------------------------------------------
# scale all of the controls to be more visible with the mesh.
def scaleControls():
    controls = ["cowl_l_anim", "cowl_r_anim", "cowl_bk_anim", "outer_ring_A_piston01_l_anim", "outer_ring_A_piston01_r_anim", "outer_ring_A_piston02_l_anim", "outer_ring_A_piston02_r_anim", "outer_ring_B_piston01_l_anim", "outer_ring_B_piston01_r_anim", "hood_l_anim", "hood_r_anim", "outer_ring_A_l_anim", "outer_ring_A_r_anim", "outer_ring_B_l_anim", "outer_ring_B_r_anim", "outer_ring_C_l_anim", "outer_ring_C_r_anim"]
    for control in controls:
        cmds.select(control+".cv[0:32]")
        cmds.scale(0.2, 0.2, 0.2, r=True)


    cmds.select(["portal_anim.cv[0:32]"])
    cmds.scale(1.5, 0.5, 1.5, r = True, ocp = True)

    cmds.select(["outer_ring_base_anim.cv[0:32]"])
    cmds.scale(4.5, 0.001, 4.5, r = True, ocp = True)

    cmds.select(["portal_ring_anim.cv[0:32]"])
    cmds.scale(2.5, 0.001, 2.5, r = True, ocp = True)

    cmds.select(["outer_ring_A_l_anim.cv[0:32]"])
    cmds.move(4, 0, 4, r = True)

    cmds.select(["outer_ring_A_r_anim.cv[0:32]"])
    cmds.move(-4, 0, 4, r = True)

    cmds.select(["outer_ring_B_l_anim.cv[0:32]"])
    cmds.move(4, 0, 0, r = True)

    cmds.select(["outer_ring_B_r_anim.cv[0:32]"])
    cmds.move(-4, 0, 0, r = True)

    cmds.select(["outer_ring_C_l_anim.cv[0:32]"])
    cmds.move(4, 0, -4, r = True)

    cmds.select(["outer_ring_C_r_anim.cv[0:32]"])
    cmds.move(-4, 0, -4, r = True)

    cmds.select(["ik_foot_anim_l.cv[0:17]"])
    cmds.move(0, 3, -2.22651, r = True)

    cmds.select(["ik_foot_anim_r.cv[0:17]"])
    cmds.move(0, 3, -2.22651, r = True)

    cmds.select(["cowl_bk_anim.cv[0:32]"])
    cmds.move(0, 0, 6.5, r = True)

    cmds.select(["cowl_l_anim.cv[0:32]"])
    cmds.move(0, 0, 2, r = True)

    cmds.select(["cowl_r_anim.cv[0:32]"])
    cmds.move(0, 0, 2, r = True)

    cmds.select(None)

scaleControls()

# ------------------------------------------------------------------
# Build the rig for the rings and connect them to the arm rig.
def buildRingRig():
    import Tools.ART_addSpaceSwitching as space
    reload(space)

    #space.addSpaceSwitching("portal_ring_anim", "outer_ring_base_anim", True)
    
    controlsList = {"outer_ring_A_l":"outer_ring_base_anim", "outer_ring_B_l":"outer_ring_A_l_anim", "outer_ring_C_l":"outer_ring_B_l_anim", "outer_ring_A_piston01_l":"outer_ring_A_l_anim", "outer_ring_A_piston02_l":"outer_ring_A_l_anim", "outer_ring_B_piston01_l":"outer_ring_B_l_anim", "outer_ring_A_r":"outer_ring_base_anim", "outer_ring_B_r":"outer_ring_A_r_anim", "outer_ring_C_r":"outer_ring_B_r_anim", "outer_ring_A_piston01_r":"outer_ring_A_r_anim", "outer_ring_A_piston02_r":"outer_ring_A_r_anim", "outer_ring_B_piston01_r":"outer_ring_B_r_anim", "portal":"portal_ring_anim"}
    for one in controlsList:
        # Re-create the scale constraint between the anim control and the driver joint.
        sourceConn = cmds.listConnections("driver_"+one+".s", c=True, p=True)
        cmds.disconnectAttr(sourceConn[1], sourceConn[0])
        cmds.scaleConstraint(one+"_anim", "driver_"+one, mo=True)
        
        # scale constraint the control to the leaf control we want to scale it.
        cmds.scaleConstraint(controlsList[one], one+"_parent_grp", mo=True)

        # set the joints (driver and deform) to have segment scale compensate off.
        cmds.setAttr("driver_"+one+".segmentScaleCompensate", 0)
        cmds.setAttr(one+".segmentScaleCompensate", 0)
            
    cmds.select("outer_ring_base_anim")

buildRingRig()

# ------------------------------------------------------------------
# Delete this once the animators lock down how big they want the ring to be.
def TEMPRINGMORPH():
    cmds.addAttr("outer_ring_base_anim", ln="Thickness", keyable=True)
    cmds.connectAttr("outer_ring_base_anim.Thickness", "blendShape1.outer_ring_top_geo_scaled", f=True)
    cmds.connectAttr("outer_ring_base_anim.Thickness", "blendShape2.outer_ring_bott_geo_scaled", f=True)
    cmds.connectAttr("outer_ring_base_anim.Thickness", "blendShape3.outer_ring_inside_geo_scaled", f=True)
#TEMPRINGMORPH()

# ------------------------------------------------------------------
def dynamicsOff():
    for one in ["cloth_chest_bk_l_anim", "cloth_chest_bk_r_anim", "cloth_chest_fr_l_anim", "cloth_chest_fr_r_anim", "chest_cloth_dyn_anim", "hips_cloth_bk_l_dyn_anim", "hips_cloth_bk_r_dyn_anim", "hips_cloth_fr_l_dyn_anim", "hips_cloth_fr_r_dyn_anim", "loin_cloth_bk_dyn_anim", "loin_cloth_fr_dyn_anim"]:
        cmds.setAttr(one+".chainStartEnvelope", 0)
        cmds.setAttr(one+".chainStartEnvelope", l=True)
dynamicsOff()

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