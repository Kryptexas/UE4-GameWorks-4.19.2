import maya.cmds as cmds
import maya.mel as mel
import math
import Tools.ART_addSpaceSwitching as space
reload(space)

# NOTE: This character has special steps to run during Rig Pose creation.  See the following commented out script.
#########################################
# RUN THIS COMMAND DURING THE RIG POSE CREATION STEP TO GET THE SHIELD TO THE CORRECT LOCATION.  LEAVE IT COMMENTED OUT SO THAT IT DOESNT RUN DURING RIGGING.
'''
# The following 2 lines are used to find the locations and then input below.
#cmds.xform(q=True, ws=True, rp=True)
#cmds.xform(q=True, ro=True)


cmds.xform("calf_mover_l", ws=True, t=[19.79804629, 3.337048167, 40.39003741])
cmds.xform("calf_mover_l", ro=[160.36136150000002, -71.79676151, -144.3110928])

cmds.xform("calf_mover_r", ws=True, t=[-19.79807484, 3.337052443, 40.39004631])
cmds.xform("calf_mover_r", ro=[-19.638638459999992, 71.79676151, 144.3110928])

cmds.xform("foot_mover_l", ws=True, t=[26.80774865, 8.872520219, 12.46154229])
cmds.xform("foot_mover_l", ro=[0.0, -90.0, 0.0])

cmds.xform("foot_mover_r", ws=True, t=[-26.80775027, 8.872528025, 12.46154483])
cmds.xform("foot_mover_r", ro=[90.0, 90.0, -90.0])
'''
#########################################

# ------------------------------------------------------------------
# scale all of the controls to be more visible with the mesh.
def scaleControls():
    for side in ["_l", "_r"]:
        # feet
        cmds.select(["ik_foot_anim"+side+".cv[0:17]"])
        cmds.scale(1.5, 1, 1, r = True, ocp = False)
        cmds.move(0, 6, -5.468891, r = True)

        # hands
        cmds.select(["ik_wrist"+side+"_anim.cv[0:7]"])
        cmds.scale(1.9, 1.9, 1.9, r = True, ocp = False)

        # other
        cmds.select(["weapon"+side+"_anim.cv[0:32]"])
        cmds.scale(0.238522, 0.238522, 0.238522, r = True, ocp = True)
        cmds.move(-5.6, 0, 0, r = True, os=True, wd=True)

        cmds.select(["clavicle"+side+"_anim.cv[0:32]"])
        cmds.move(0, 0, 11, r = True, os=True, wd=True)

    for one in ["shoulder_blade_A_l_anim", "shoulder_blade_B_l_anim", "shoulder_blade_A_r_anim", "shoulder_blade_B_r_anim"]:
        cmds.select([one+".cv[*]"])
        cmds.scale(0.3, 0.3, 0.3, r = True, ocp = True)
        cmds.move(0, 0, 23, r = True, os=False, wd=True)

    cmds.select(None)

scaleControls()

# ------------------------------------------------------------------
def shoulderRig():
    cmds.setDrivenKeyframe(["shoulder_blade_A_l_anim_grp.translate", "shoulder_blade_B_l_anim_grp.translate"], cd = "driver_clavicle_l.rotateY", itt = "spline", ott = "spline")
    cmds.setAttr("clavicle_l_anim.translateZ", 12)
    cmds.setAttr("shoulder_blade_A_l_anim_grp.translate", 1.9051, 0, -5.01986, type="double3")
    cmds.setAttr("shoulder_blade_B_l_anim_grp.translate", 0, 0, -2.74608, type="double3")
    cmds.setDrivenKeyframe(["shoulder_blade_A_l_anim_grp.translate", "shoulder_blade_B_l_anim_grp.translate"], cd = "driver_clavicle_l.rotateY", itt = "spline", ott = "spline")
    cmds.setAttr("clavicle_l_anim.translateZ", 0)

    cmds.setAttr("clavicle_l_anim.translateZ", -10)
    cmds.setAttr("shoulder_blade_A_l_anim_grp.translate", -3.99968, 0, 5.47598, type="double3")
    cmds.setAttr("shoulder_blade_B_l_anim_grp.translate", 0, 0, 4.31874, type="double3")
    cmds.setDrivenKeyframe(["shoulder_blade_A_l_anim_grp.translate", "shoulder_blade_B_l_anim_grp.translate"], cd = "driver_clavicle_l.rotateY", itt = "spline", ott = "spline")
    cmds.setAttr("clavicle_l_anim.translateZ", 0)
    
    
    
    
    
    cmds.setDrivenKeyframe(["shoulder_blade_A_r_anim_grp.translate", "shoulder_blade_B_r_anim_grp.translate"], cd = "driver_clavicle_r.rotateY", itt = "spline", ott = "spline")
    cmds.setAttr("clavicle_r_anim.translateZ", 12)
    cmds.setAttr("shoulder_blade_A_r_anim_grp.translate", -1.9051, 0, 5.01986, type="double3")
    cmds.setAttr("shoulder_blade_B_r_anim_grp.translate", 0, 0, 2.74608, type="double3")
    cmds.setDrivenKeyframe(["shoulder_blade_A_r_anim_grp.translate", "shoulder_blade_B_r_anim_grp.translate"], cd = "driver_clavicle_r.rotateY", itt = "spline", ott = "spline")
    cmds.setAttr("clavicle_r_anim.translateZ", 0)

    cmds.setAttr("clavicle_r_anim.translateZ", -10)
    cmds.setAttr("shoulder_blade_A_r_anim_grp.translate", 3.99968, 0, -5.47598, type="double3")
    cmds.setAttr("shoulder_blade_B_r_anim_grp.translate", 0, 0, -4.31874, type="double3")
    cmds.setDrivenKeyframe(["shoulder_blade_A_r_anim_grp.translate", "shoulder_blade_B_r_anim_grp.translate"], cd = "driver_clavicle_r.rotateY", itt = "spline", ott = "spline")
    cmds.setAttr("clavicle_r_anim.translateZ", 0)
shoulderRig()

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
addCapsule(60, 135)

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

#save the export file.
cmds.file(save = True, type = "mayaBinary", force = True)
