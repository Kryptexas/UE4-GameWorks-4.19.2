def addFortniteIKBones():
    
    try:
	#create the joints
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
	
	
	#create hierarhcy
	cmds.parent(ikFootRoot, "root")
	cmds.parent(ikHandRoot, "root")
	cmds.parent(ikFootLeft, ikFootRoot)
	cmds.parent(ikFootRight, ikFootRoot)
	cmds.parent(ikHandGun, ikHandRoot)
	cmds.parent(ikHandLeft, ikHandGun)
	cmds.parent(ikHandRight, ikHandGun)
	
	#constrain the joints
	leftHandConstraint = cmds.parentConstraint("hand_l", ikHandLeft)[0]
	rightHandGunConstraint = cmds.parentConstraint("hand_r", ikHandGun)[0]
	rightHandConstraint = cmds.parentConstraint("hand_r", ikHandRight)[0]
	leftFootConstraint = cmds.parentConstraint("foot_l", ikFootLeft)[0]
	rightFootConstraint = cmds.parentConstraint("foot_r", ikFootRight)[0]
	
    except:
	cmds.warning("Something went wrong")

def addCapsule():
    capsule = cmds.polyCylinder(name = "collision_capsule")
    cmds.setAttr(capsule[1] + ".subdivisionsAxis", 8)
    cmds.setAttr(capsule[0] + ".rx", -90)
    cmds.setAttr(capsule[0] + ".tz", 1)
    
    cmds.xform(capsule[0], ws = True, piv = [0,0,0])
    cmds.setAttr(capsule[0] + ".sx", 42)
    cmds.setAttr(capsule[0] + ".sy", 75)
    cmds.setAttr(capsule[0] + ".sz", 42)
    
    cmds.makeIdentity(capsule[0], t = 1, r = 1, s = 1, apply = True)
    
    cmds.setAttr(capsule[0] + ".overrideEnabled", 1)
    cmds.setAttr(capsule[0] + ".overrideColor", 18)
    cmds.setAttr(capsule[0] + ".overrideShading", 0)
    
    cmds.setAttr(capsule[0] + ".sx", lock = True, keyable = False, channelBox = False)
    cmds.setAttr(capsule[0] + ".sy", lock = True, keyable = False, channelBox = False)
    cmds.setAttr(capsule[0] + ".sz", lock = True, keyable = False, channelBox = False)
    
    cmds.addAttr(capsule[0], ln = "Shaded", dv = 0, min = 0, max = 1, keyable = True)
    cmds.connectAttr(capsule[0] + ".Shaded", capsule[0] + ".overrideShading")
    
    cmds.parentConstraint("root", capsule[0], mo = True)
    cmds.select("collision_capsule", r=True)
    cmds.createDisplayLayer(nr=True, name="Collision Capsule")
    cmds.select(clear=True)
    
def hookupSpine():
    #Spine 4 : Rotate X
    cmds.select("spine_04_anim")
    cmds.addAttr(longName='spine_5_Influence', defaultValue=1, minValue=-1, maxValue=5, keyable = True)
    cmds.addAttr(longName='spine_3_Influence', defaultValue=1, minValue=-1, maxValue=5, keyable = True)
    
    spine4MultXA = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_04_xa_mult")
    cmds.connectAttr("spine_05_anim.rx", spine4MultXA + ".input1X")
    cmds.connectAttr("spine_04_anim.spine_5_Influence", spine4MultXA + ".input2X")
    
    spine4MultXB = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_04_xb_mult")
    cmds.connectAttr("spine_03_anim.rx", spine4MultXB+ ".input1X")
    cmds.connectAttr("spine_04_anim.spine_3_Influence", spine4MultXB + ".input2X")
    
    spine4MultX = cmds.shadingNode("plusMinusAverage", asUtility = True, name = "spine04_drivenX_avg")
    cmds.setAttr(spine4MultX + ".operation", 3)
    cmds.connectAttr(spine4MultXA + ".outputX", spine4MultX + ".input1D[0]")
    cmds.connectAttr(spine4MultXB + ".outputX", spine4MultX + ".input1D[1]")
    cmds.connectAttr(spine4MultX + ".output1D", "spine_04_anim.rotateX")
    
    #Spine 4 : Rotate Y
    spine4MultYA = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_04_ya_mult")
    cmds.connectAttr("spine_05_anim.ry", spine4MultYA + ".input1X")
    cmds.connectAttr("spine_04_anim.spine_5_Influence", spine4MultYA + ".input2X")
    
    spine4MultYB = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_04_yb_mult")
    cmds.connectAttr("spine_03_anim.ry", spine4MultYB+ ".input1X")
    cmds.connectAttr("spine_04_anim.spine_3_Influence", spine4MultYB + ".input2X")
    
    spine4MultY = cmds.shadingNode("plusMinusAverage", asUtility = True, name = "spine04_drivenY_avg")
    cmds.setAttr(spine4MultY + ".operation", 3)
    cmds.connectAttr(spine4MultYA + ".outputX", spine4MultY + ".input1D[0]")
    cmds.connectAttr(spine4MultYB + ".outputX", spine4MultY + ".input1D[1]")
    cmds.connectAttr(spine4MultY + ".output1D", "spine_04_anim.rotateY")
    
    
    #Spine 4 : Rotate Z
    spine4MultZA = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_04_za_mult")
    cmds.connectAttr("spine_05_anim.rz", spine4MultZA + ".input1X")
    cmds.connectAttr("spine_04_anim.spine_5_Influence", spine4MultZA + ".input2X")
    
    spine4MultZB = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_04_zb_mult")
    cmds.connectAttr("spine_03_anim.rz", spine4MultZB+ ".input1X")
    cmds.connectAttr("spine_04_anim.spine_3_Influence", spine4MultZB + ".input2X")
    
    spine4MultZ = cmds.shadingNode("plusMinusAverage", asUtility = True, name = "spine04_drivenZ_avg")
    cmds.setAttr(spine4MultZ + ".operation", 3)
    cmds.connectAttr(spine4MultZA + ".outputX", spine4MultZ + ".input1D[0]")
    cmds.connectAttr(spine4MultZB + ".outputX", spine4MultZ + ".input1D[1]")
    cmds.connectAttr(spine4MultZ + ".output1D", "spine_04_anim.rotateZ")
    
    # # # # 
    
    #Spine 2 : Rotate X
    cmds.select("spine_02_anim")
    cmds.addAttr(longName='spine_3_Influence', defaultValue=1, minValue=-1, maxValue=5, keyable = True)
    cmds.addAttr(longName='spine_1_Influence', defaultValue=1, minValue=-1, maxValue=5, keyable = True)
    
    spine2MultXA = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_02_xa_mult")
    cmds.connectAttr("spine_03_anim.rx", spine2MultXA + ".input1X")
    cmds.connectAttr("spine_02_anim.spine_3_Influence", spine2MultXA + ".input2X")
    
    spine2MultXB = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_02_xb_mult")
    cmds.connectAttr("spine_01_anim.rx", spine2MultXB+ ".input1X")
    cmds.connectAttr("spine_02_anim.spine_1_Influence", spine2MultXB + ".input2X")
    
    spine2MultX = cmds.shadingNode("plusMinusAverage", asUtility = True, name = "spine02_drivenX_avg")
    cmds.setAttr(spine2MultX + ".operation", 3)
    cmds.connectAttr(spine2MultXA + ".outputX", spine2MultX + ".input1D[0]")
    cmds.connectAttr(spine2MultXB + ".outputX", spine2MultX + ".input1D[1]")
    cmds.connectAttr(spine2MultX + ".output1D", "spine_02_anim.rotateX")
    
    #Spine 2 : Rotate Y
    spine2MultYA = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_02_ya_mult")
    cmds.connectAttr("spine_03_anim.ry", spine2MultYA + ".input1X")
    cmds.connectAttr("spine_02_anim.spine_3_Influence", spine2MultYA + ".input2X")
    
    spine2MultYB = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_02_yb_mult")
    cmds.connectAttr("spine_01_anim.ry", spine2MultYB+ ".input1X")
    cmds.connectAttr("spine_02_anim.spine_1_Influence", spine2MultYB + ".input2X")
    
    spine2MultY = cmds.shadingNode("plusMinusAverage", asUtility = True, name = "spine02_drivenY_avg")
    cmds.setAttr(spine2MultY + ".operation", 3)
    cmds.connectAttr(spine2MultYA + ".outputX", spine2MultY + ".input1D[0]")
    cmds.connectAttr(spine2MultYB + ".outputX", spine2MultY + ".input1D[1]")
    cmds.connectAttr(spine2MultY + ".output1D", "spine_02_anim.rotateY")

    
    
    #Spine 2 : Rotate Z
    spine2MultZA = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_02_za_mult")
    cmds.connectAttr("spine_03_anim.rz", spine2MultZA + ".input1X")
    cmds.connectAttr("spine_02_anim.spine_3_Influence", spine2MultZA + ".input2X")
    
    spine2MultZB = cmds.shadingNode("multiplyDivide", asUtility = True, name = "spine_02_zb_mult")
    cmds.connectAttr("spine_01_anim.rz", spine2MultZB+ ".input1X")
    cmds.connectAttr("spine_02_anim.spine_1_Influence", spine2MultZB + ".input2X")
    
    spine2MultZ = cmds.shadingNode("plusMinusAverage", asUtility = True, name = "spine02_drivenZ_avg")
    cmds.setAttr(spine2MultZ + ".operation", 3)
    cmds.connectAttr(spine2MultZA + ".outputX", spine2MultZ + ".input1D[0]")
    cmds.connectAttr(spine2MultZB + ".outputX", spine2MultZ + ".input1D[1]")
    cmds.connectAttr(spine2MultZ + ".output1D", "spine_02_anim.rotateZ")




    
    cmds.setKeyframe("spine_02_anim.rotate")
    cmds.setKeyframe("spine_04_anim.rotate")
    
    cmds.select("spine_02_anim.cv[*]")
    cmds.scale(0, 0, 0,  relative = True)
    
    cmds.select("spine_04_anim.cv[*]")
    cmds.scale(0, 0, 0,  relative = True)
    
    
    #check for new blendUnitConversion attr and alias attr it
    spine4Connections = cmds.listConnections("spine_04_anim", source = True, type = "pairBlend")
    for each in spine4Connections:
	
	conversions = cmds.listConnections(each, type = "unitConversion")
	if conversions != None:
	    attr = conversions[0].partition("Conversion")[2]
	    attr = "blendUnitConversion" + attr
	    
	    try:
			cmds.aliasAttr("driven", "spine_04_anim." + attr)
			cmds.setAttr("spine_04_anim.driven", 1)
	    except:
			pass
	    
	    
    spine2Connections = cmds.listConnections("spine_02_anim", source = True, type = "pairBlend")
    for each in spine2Connections:
	
	conversions = cmds.listConnections(each, type = "unitConversion")
	if conversions != None:
	    attr = conversions[0].partition("Conversion")[2]
	    attr = "blendUnitConversion" + attr
	    
	    try:
			cmds.aliasAttr("driven", "spine_02_anim." + attr)
			cmds.setAttr("spine_02_anim.driven", 1)
	    except:
			pass
			
			
#set time to -100
cmds.currentTime(-100)

#hookup spine
hookupSpine()

#control cv scale/position adjustments
cmds.select("curveShape56.cv[0:17]")
cmds.move(1.28, 1.45, -3.19, r = True, os = True, wd = False)

cmds.select("curveShape61.cv[0:17]")
cmds.move(-1.28, 1.45, -3.19, r = True, os = True, wd = False)

cmds.select("curveShape59.cv[0:32]")
cmds.move(4.1, 4.38, 0, r = True, os = True, wd = False)

cmds.select("curveShape64.cv[0:32]")
cmds.move(-4.1, -4.38, 0, r = True, os = True, wd = False)

cmds.select("weapon_l_anim.cv[0:32]")
cmds.scale(0, 1, 1, r = True)

cmds.select("weapon_r_anim.cv[0:32]")
cmds.scale(0, 1, 1, r = True)

cmds.select("attach_anim.cv[0:32]")
cmds.scale(.5, .5, .5, r = True)

#cmds.select("head_fk_anim.cv[0:7]")
cmds.rotate(0, 0, 34.20467, "head_fk_anim.cv[0:7]", r=True, p=(0, 1.787412, 152.070201))

#setup capsule
if cmds.objExists("collision_capsule"):
    cmds.delete("collision_capsule")
    
addCapsule()

#chris evans' area, do not mess wiff my stuff!
import maya.mel as mel
ce_fpath = __file__.replace('\\','/')
ce_fpath = ce_fpath.replace(ce_fpath.split('/')[-1],'').replace('/Scripts/', '/ART/Projects/Fortnite/Scripts/')
meval = 'source \"' + ce_fpath + 'medium_player_new_hand_ctrls.mel\";'
print meval
mel.eval(meval)

#set time back to 0, save
cmds.currentTime(0)
cmds.file(save = True, type = "mayaBinary", force = True)

#open export file
sceneName = cmds.file(q = True, sceneName = True)
exportFile = sceneName.partition("AnimRigs")[0] + "ExportFiles" +  sceneName.partition("AnimRigs")[2].partition(".mb")[0] + "_Export.mb"
cmds.file(exportFile, open = True, force = True)

#add ik bones
addFortniteIKBones()

#save
cmds.file(save = True, type = "mayaBinary", force = True)

