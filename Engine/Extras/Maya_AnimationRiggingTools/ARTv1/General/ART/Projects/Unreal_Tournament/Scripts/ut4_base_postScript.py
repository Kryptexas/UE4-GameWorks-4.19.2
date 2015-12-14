def addCapsule():
    capsule = cmds.polyCylinder(name = "collision_capsule", ch=1, r=46, h=95, cuv=3, sc=3, rcp=1)
    cmds.setAttr(capsule[1] + ".subdivisionsAxis", 8)
    cmds.setAttr(capsule[0] + ".rx", -90)
    cmds.setAttr(capsule[0] + ".tz", 1)
    
    cmds.move(0, 0, 93.5, capsule, wd=1)
    cmds.xform(capsule[0], ws = True, piv = [0,0,0])
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

def ikSetup():
    cmds.select(clear = True)
    ikFootRoot = cmds.joint(name = "ik_foot_root")
    cmds.select(clear = True)
    ikFootLeft = cmds.joint(name = "ik_foot_l")
    cmds.select(clear = True)
    ikFootRight = cmds.joint(name = "ik_foot_r")
    cmds.select(clear = True)
    ikHandRoot = cmds.joint(name = "ik_hand_root")
    cmds.select(clear = True)
    ikHandGun = cmds.joint(name = "ik_hand_gun")
    cmds.select(clear = True)
    ikHandLeft = cmds.joint(name = "ik_hand_l")
    cmds.select(clear = True)
    ikHandRight = cmds.joint(name = "ik_hand_r")

    #create hierarhcy
    cmds.parent(ikFootRoot, "root")
    cmds.parent(ikHandRoot, "root")
    cmds.parent(ikFootLeft, ikFootRoot)
    cmds.parent(ikFootRight, ikFootRoot)
    cmds.parent(ikHandGun, ikHandRoot)
    cmds.parent(ikHandLeft, ikHandGun)
    cmds.parent(ikHandRight, ikHandGun)

    leftHandConstraint = cmds.parentConstraint("hand_l", 'ik_hand_l')[0]
    rightHandGunConstraint = cmds.parentConstraint("hand_r", 'ik_hand_gun')[0]
    rightHandConstraint = cmds.parentConstraint("hand_r", 'ik_hand_r')[0]
    leftFootConstraint = cmds.parentConstraint("foot_l", 'ik_foot_l')[0]
    rightFootConstraint = cmds.parentConstraint("foot_r", 'ik_foot_r')[0]

    cmds.select(clear = True)
    ikFootRoot = cmds.joint(name = "ikalt_foot_root")
    cmds.select(clear = True)
    ikFootLeft = cmds.joint(name = "ikalt_foot_l")
    cmds.select(clear = True)
    ikFootRight = cmds.joint(name = "ikalt_foot_r")
    cmds.select(clear = True)
    ikHandRoot = cmds.joint(name = "ikalt_hand_root")
    cmds.select(clear = True)
    ikHandLeft = cmds.joint(name = "ikalt_hand_l")
    cmds.select(clear = True)
    ikHandRight = cmds.joint(name = "ikalt_hand_r")

    #create hierarhcy
    cmds.parent(ikFootRoot, "root")
    cmds.parent(ikHandRoot, "root")
    cmds.parent(ikFootLeft, ikFootRoot)
    cmds.parent(ikFootRight, ikFootRoot)
    cmds.parent(ikHandLeft, ikHandRoot)
    cmds.parent(ikHandRight, ikHandRoot)

    leftHandConstraint = cmds.parentConstraint("hand_l", 'ik_hand_l')[0]
    rightHandConstraint = cmds.parentConstraint("hand_r", 'ik_hand_r')[0]
    leftFootConstraint = cmds.parentConstraint("foot_l", 'ik_foot_l')[0]
    rightFootConstraint = cmds.parentConstraint("foot_r", 'ik_foot_r')[0]
    
    cmds.parentConstraint("hand_l", 'ikalt_hand_l')
    cmds.parentConstraint("hand_r", 'ikalt_hand_r')
    cmds.parentConstraint("foot_l", 'ikalt_foot_l')
    cmds.parentConstraint("foot_r", 'ikalt_foot_r')

ikSetup()
addCapsule()