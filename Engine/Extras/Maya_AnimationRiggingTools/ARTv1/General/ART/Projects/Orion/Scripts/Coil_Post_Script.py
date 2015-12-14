import maya.mel as mel
import maya.cmds as cmds


# NOTE: for building this rig, because of the spine_03_alt joint being oriented to match exactly spine_03, the children of spine_03_alt when mirrored have to have their mirror settings set to 1, -1, 1, 1, -1, 1.  Otherwise they wont mirror.  When you go back to the Skeleton Creation step these values likely wont persist and will have to be manually added.  The following script can be run to set these values.
'''
cmds.setAttr("big_wire_l_mover.mirrorTX", 1)
cmds.setAttr("big_wire_l_mover.mirrorTY", -1)
cmds.setAttr("big_wire_l_mover.mirrorTZ", 1)
cmds.setAttr("big_wire_l_mover.mirrorRX", 1)
cmds.setAttr("big_wire_l_mover.mirrorRY", -1)
cmds.setAttr("big_wire_l_mover.mirrorRZ", 1)
cmds.setAttr("shoulderlamp_l_mover.mirrorTX", 1)
cmds.setAttr("shoulderlamp_l_mover.mirrorTY", -1)
cmds.setAttr("shoulderlamp_l_mover.mirrorTZ", 1)
cmds.setAttr("shoulderlamp_l_mover.mirrorRX", -1)
cmds.setAttr("shoulderlamp_l_mover.mirrorRY", 1)
cmds.setAttr("shoulderlamp_l_mover.mirrorRZ", -1)
cmds.setAttr("shoulderpad_l_mover.mirrorTX", 1)
cmds.setAttr("shoulderpad_l_mover.mirrorTY", -1)
cmds.setAttr("shoulderpad_l_mover.mirrorTZ", 1)
cmds.setAttr("shoulderpad_l_mover.mirrorRX", 1)
cmds.setAttr("shoulderpad_l_mover.mirrorRY", -1)
cmds.setAttr("shoulderpad_l_mover.mirrorRZ", 1)
cmds.setAttr("sideplate_l_mover.mirrorTX", 1)
cmds.setAttr("sideplate_l_mover.mirrorTY", -1)
cmds.setAttr("sideplate_l_mover.mirrorTZ", 1)
cmds.setAttr("sideplate_l_mover.mirrorRX", 1)
cmds.setAttr("sideplate_l_mover.mirrorRY", -1)
cmds.setAttr("sideplate_l_mover.mirrorRZ", 1)
cmds.setAttr("small_wire_l_mover.mirrorTX", 1)
cmds.setAttr("small_wire_l_mover.mirrorTY", -1)
cmds.setAttr("small_wire_l_mover.mirrorTZ", 1)
cmds.setAttr("small_wire_l_mover.mirrorRX", 1)
cmds.setAttr("small_wire_l_mover.mirrorRY", -1)
cmds.setAttr("small_wire_l_mover.mirrorRZ", 1)
cmds.setAttr("turbineplate_l_mover.mirrorTX", 1)
cmds.setAttr("turbineplate_l_mover.mirrorTY", -1)
cmds.setAttr("turbineplate_l_mover.mirrorTZ", 1)
cmds.setAttr("turbineplate_l_mover.mirrorRX", -1)
cmds.setAttr("turbineplate_l_mover.mirrorRY", 1)
cmds.setAttr("turbineplate_l_mover.mirrorRZ", -1)
'''


# ------------------------------------------------------------------
# scale all of the controls to be more visible with the mesh.
def scaleControls():
    for side in ["_l", "_r"]:
        # feet
        cmds.select(["ik_foot_anim"+side+".cv[*]"])
        cmds.move(0, 8.796488, 5.948312, r = True)

        cmds.select(["ik_wrist"+side+"_anim.cv[*]"])
        cmds.scale(1.6, 1.6, 1.6, r = True, ocp = False)
        
        cmds.select(["weapon"+side+"_anim.cv[*]"])
        cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
        cmds.move(0, -30.39, 0, r = True)            

    cmds.select(["hip_l_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(16, 0, 15, r = True)            

    cmds.select(["hip_r_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(-16, 0, 15, r = True)            

    cmds.select(["shoulderpad_l_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(5, 0, 21, r = True)            

    cmds.select(["shoulderpad_r_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(-5, 0, 21, r = True)            

    cmds.select(["sideplate_l_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(15, 0, 0, r = True)            

    cmds.select(["sideplate_r_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(-15, 0, 0, r = True)            

    cmds.select(["spine_03_alt_anim.cv[*]"])
    cmds.scale(1, 5, 5, r = True, ocp = False)
    cmds.move(0, -19, 12, r = True)            

    cmds.select(["weapon_stock_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(25, 0, 0, r = True)            

    cmds.select(["weapon_zapper_top_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(21, 0, 0, r = True)            

    cmds.select(["weapon_zapper_bott_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(-21, 0, 0, r = True)            

    cmds.select(["neckbackplate_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(0, 8, 4, r = True)            

    cmds.select(["chestplate_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(0, -6, 11, r = True)            

    cmds.select(["turbine_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
    cmds.move(0, 9, 0, r = True)            

    cmds.select(["weapon_wires_anim.cv[*]"])
    cmds.scale(0.48, 0.48, 0.48, r = True, ocp = False)
   
    objects = ["sixpack_bott_anim", "sixpack_mid_anim", "sixpack_top_anim", "small_wire_l_anim", "small_wire_r_anim", "big_wire_l_anim", "big_wire_r_anim", "shoulderlamp_l_anim", "shoulderlamp_r_anim", "turbineplate_l_anim", "turbineplate_r_anim"]
    for one in objects:
        cmds.select([one+".cv[*]"])
        cmds.scale(0.4, 0.4, 0.4, r = True, ocp = True)
        cmds.move(0, 0, 0, r = True, os=True, wd=True)

    # Other
    cmds.select(["jaw_anim.cv[*]"])
    cmds.scale(0.2, 0.2, 0.2, r = True, ocp = True)
    cmds.move(0, -7, -2, r = True)

    cmds.select(None)
scaleControls()

# ------------------------------------------------------------------
# Move the panels on the torso and hook up the clav to move with them.
def panelsRig():

    cmds.setAttr("Rig_Settings.lClavMode", 0)
    cmds.setAttr("Rig_Settings.rClavMode", 0)
    
    cmds.parentConstraint("sideplate_l_anim", "fk_clavicle_l_anim_grp", mo=True)
    cmds.parentConstraint("sideplate_r_anim", "fk_clavicle_r_anim_grp", mo=True)

panelsRig()

# ------------------------------------------------------------------
def shoulderRig():
    for side in ["_l", "_r"]:
        cmds.addAttr("shoulderpad"+side+"_anim", ln = "FollowClav", dv = 1, min = 0, max = 1, keyable = True)

        shoulder_loc = cmds.duplicate("shoulderpad"+side+"_anim_space_switcher", n="shoulder_loc"+side+"", po=True)[0]
        cmds.parent(shoulder_loc, "ctrl_rig")
        cmds.makeIdentity(shoulder_loc, t=True, r=True, a=True)
        cmds.parentConstraint("driver_upperarm"+side+"", shoulder_loc, mo=True)

        const = cmds.pointConstraint(shoulder_loc, "shoulderpad"+side+"_anim_space_switcher", "shoulderpad"+side+"_anim_grp", mo=True)[0]

        reverseNode = cmds.shadingNode("reverse", asUtility=True, name="shoulderpad"+side+"_reverse")
        cmds.connectAttr("shoulderpad"+side+"_anim.FollowClav", const+"."+shoulder_loc+"W0", f=True)
        cmds.connectAttr("shoulderpad"+side+"_anim.FollowClav", reverseNode+".inputX", f=True)
        cmds.connectAttr(reverseNode+".outputX", const+".shoulderpad"+side+"_anim_space_switcherW1", f=True)

        if side == "_l":
            cmds.transformLimits("shoulderpad"+side+"_anim_grp", tz=[0, 1], etz=[1, 0])
        else:
            cmds.transformLimits("shoulderpad"+side+"_anim_grp", tz=[-1, 0], etz=[0, 1])

    cmds.select(None)
    
shoulderRig()

# ------------------------------------------------------------------
# Modify the hips so that the legs follow the hip_*_anim control.
def hipsRig():
    cmds.parentConstraint("hip_l_anim", "hip_redirect_l", mo=True)        
    cmds.parentConstraint("hip_r_anim", "hip_redirect_r", mo=True)        
hipsRig()

# ------------------------------------------------------------------
def spineRig():
    cmds.setAttr("Rig_Settings.spine_ik", 0)
    cmds.setAttr("Rig_Settings.spine_fk", 1)

    # Create the rig for the sixpack stuff.
    for one in ["sixpack_top", "sixpack_mid", "sixpack_bott"]:
        cmds.select("neck_spine_geo", r=True)
        cmds.ClosestPointOn(1, 1)
        surfaceLoc = cmds.rename("cpConstraintPos", one+"_surfaceLoc")
        worldLoc = cmds.rename("cpConstraintIn", one+"worldLoc")
        cmds.parent(surfaceLoc, worldLoc, "rig_grp")
        const = cmds.parentConstraint(one, worldLoc, mo=False)[0]
        cmds.delete(const)
        
        cmds.parentConstraint("spine_03", worldLoc, mo=True)[0]
        

        if one == "sixpack_top":
            cmds.normalConstraint("neck_spine_geo", surfaceLoc, aimVector=[0, 0, 1], upVector=[0, 1, 0], worldUpType="object", worldUpObject="spine_03_alt")
        if one == "sixpack_mid":
            cmds.normalConstraint("neck_spine_geo", surfaceLoc, aimVector=[0, 0, 1], upVector=[0, 1, 0], worldUpType="object", worldUpObject="sixpack_top")
        if one == "sixpack_bott":
            cmds.normalConstraint("neck_spine_geo", surfaceLoc, aimVector=[0, 0, 1], upVector=[0, 1, 0], worldUpType="object", worldUpObject="sixpack_mid")

        cmds.parentConstraint(surfaceLoc, one+"_anim_grp", mo=True)[0]

spineRig()

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
addCapsule(72, 108)

# ------------------------------------------------------------------
# add ik bones
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
# ------------------------------------------------------------------
# Import in and hook up the Goblin
def hookUpGoblin():
    import Tools.ART_addSpaceSwitching as space
    reload(space)
    
    # import in goblin
    cmds.file("D:/Build/usr/jeremy_ernst/MayaTools/General/ART/Projects/Orion/AnimRigs/Coil_Goblin.mb", i=True, options='v=0', type="mayaBinary", namespace="Goblin", mergeNamespacesOnClash=False)

    cmds.setAttr("Goblin:Rig_Settings.spine_fk", 1)
    cmds.setAttr("Goblin:Rig_Settings.spine_ik", 0)

    # set elbows to body space
    cmds.setAttr("Goblin:ik_elbow_l_anim_space_switcher.space_body_anim", 1)
    cmds.setAttr("Goblin:ik_elbow_r_anim_space_switcher.space_body_anim", 1)

    '''
    REMOVING THIS SO THAT THE HANDS FOLLOW THE MECHANISM THING!
    cmds.setAttr("Goblin:ik_wrist_l_anim_space_switcher.space_body_anim", 1)
    cmds.setAttr("Goblin:ik_wrist_r_anim_space_switcher.space_body_anim", 1)
    '''
    
    cmds.setAttr("Goblin:ik_foot_anim_l_space_switcher.space_body_anim", 1)
    cmds.setAttr("Goblin:ik_foot_anim_r_space_switcher.space_body_anim", 1)

    # turn on auto clavicles
    #cmds.setAttr("Goblin:clavicle_l_anim.autoShoulders", 1)
    #cmds.setAttr("Goblin:clavicle_r_anim.autoShoulders", 1)

    # set head to world space
    cmds.setAttr("Goblin:head_fk_anim.fkOrientation", 3)

    # Add space switching to the feet on the foot pedals
    #space.addSpaceSwitching("Goblin:ik_foot_anim_l", "cockpit_foot_pedal_l_anim", True)
    #space.addSpaceSwitching("Goblin:ik_foot_anim_r", "cockpit_foot_pedal_r_anim", True)

    #position goblin
    cmds.setAttr("Goblin:body_anim.tx", 219.57371)
    cmds.setAttr("Goblin:body_anim.ty", 92.45555)
    cmds.setAttr("Goblin:body_anim.tz", -67.72273)

    # join skeletons
    cmds.select("Goblin:root", hi = True)
    joints = cmds.ls(sl = True)
    for joint in joints:
        newName = joint.partition(":")[2]
        cmds.rename(joint, "goblin_" + newName)
    cmds.parent("goblin_root", "root")    

    # Disconnect the attrs driving the goblin pelvis joint so that we can re-drive it with a parent constraint.
    attrToDisconnect = "goblin_pelvis.translate"
    connection = cmds.connectionInfo(attrToDisconnect, sfd=True)
    cmds.disconnectAttr(connection, attrToDisconnect)
    cmds.delete("goblin_pelvis_orientConstraint1")

    # Parent constrain the deform pelvis to follow the driver pelvis
    cmds.parentConstraint("Goblin:driver_pelvis", "goblin_pelvis", mo=True)

    cmds.select(None)
hookUpGoblin()

# ------------------------------------------------------------------
# save the AnimRig file
cmds.file(save = True, type = "mayaBinary", force = True)

# add IK Bones here so that they are included in the export file.  But dont save becasue we dont want them in the rig file.  Thats why we saved in the line before.
addIKBones()

# ------------------------------------------------------------------
# Creeate the export file that goes into source so it can be exported to the game
def createExportFile():
    # Import the export file from the reference so that it can be edited.
    cmds.file("D:/Build/usr/jeremy_ernst/MayaTools/General/ART/Projects/Orion/ExportFiles/Coil_Export.mb", importReference=True)

    # select the entire skeleton and make a list of it.
    cmds.select("root", hi=True)
    selection = cmds.ls(sl=True)
    cmds.showHidden(a=True)

    # for each object in the selection, disconnect translation and scale.
    for each in selection:
        attrs = ["translate", "scale"]
        cmds.lockNode(each, lock=False)

        for attr in attrs:
            try:
                conn = cmds.listConnections(each + "." + attr, s=True, p=True)[0]
                cmds.disconnectAttr(conn, each + "." + attr)
            except:
                pass

    # find all of the constraints in the skeleton and delete them
    cmds.select("root", hi=True)
    constraints = cmds.ls(sl=True, type="constraint")
    cmds.select(constraints)
    cmds.delete(constraints)

    # delete the control rig so that it severs any connections between it and the geo or skeleton.
    cmds.select("Goblin:rig_grp", "rig_grp", hi=True)
    rigs = cmds.ls(sl=True)
    for one in rigs:
        cmds.lockNode(one, lock=False)
    cmds.delete("Goblin:rig_grp", "rig_grp")

    # Remove the Goblin namespace and set the bindposes so that Creating LODs will work later on.
    cmds.namespace(rm="Goblin", mnp=True)
    cmds.dagPose("root", bp=True, r=True) 
    cmds.dagPose("goblin_root", bp=True, r=True)     

    # select the skeleton and all of the geo and export it to the export file.  Remember to go export an fbx from this.
    cmds.select("Goblin:Coil_Goblin_Geo", "Coil_Bot_Geo", "root", hi=True)
    cmds.file("D:/Build/ArtSource/Orion/Characters/Heroes/Coil/Export/Coil_Export.mb", force=True, typ="mayaBinary", pr=True, es=True)
createExportFile()

# ------------------------------------------------------------------
# ADD IK BONES to the export file
sceneName = cmds.file(q = True, sceneName = True)
exportFile = sceneName.partition("AnimRigs")[0] + "ExportFiles" +  sceneName.partition("AnimRigs")[2].partition(".mb")[0] + "_Export.mb"
cmds.file(exportFile, open = True, force = True)


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

# save the export file
cmds.file(save = True, type = "mayaBinary", force = True)


