import maya.cmds as cmds

# Husky Husk postScript


def createCollisionCylinder():
    print "junk"
    capsule = cmds.polyCylinder(n="collision_capsule")
    cmds.setAttr(capsule[1]+'.subdivisionsAxis', 8)
    cmds.setAttr(capsule[0]+'.rx', -90)
    cmds.setAttr(capsule[0]+'.tz', 1)

    cmds.xform(capsule[0], ws=True, piv=(0,0,0))
    cmds.setAttr(capsule[0]+'.sx', 34)
    cmds.setAttr(capsule[0]+'.sy', 73)
    cmds.setAttr(capsule[0]+'.sz', 34)

    #cmds.makeIdentity(capsule[0], t=True, r=True, s=True, a=True)

    cmds.setAttr(capsule[0]+'.overrideEnabled', 1)
    cmds.setAttr(capsule[0]+'.overrideColor', 18)
    cmds.setAttr(capsule[0]+'.overrideShading', 0)

    cmds.setAttr(capsule[0]+'.sx', lock=True, keyable=True, channelBox=False)
    cmds.setAttr(capsule[0]+'.sy', lock=True, keyable=True, channelBox=False)
    cmds.setAttr(capsule[0]+'.sz', lock=True, keyable=True, channelBox=False)

    cmds.addAttr(capsule[0], ln='Shaded', dv=False, min=0, max=1, keyable=True)
    cmds.connectAttr(capsule[0]+'.Shaded', capsule[0]+'.overrideShading')

    cmds.parentConstraint("root", capsule[0], mo=True)

    cmds.createDisplayLayer(name="Collision_Cylinder", number=1, empty=True)
    cmds.editDisplayLayerMembers("Collision_Cylinder", capsule[0], noRecurse=True)

def createMorphCtrls():
    
    #blink ctrls
    cmds.addAttr('head_fk_anim', ln='LidTop_l', dv=0, min=0, max=1, keyable=True)
    cmds.addAttr('head_fk_anim', ln='LidBot_l', dv=0, min=0, max=1, keyable=True)
    cmds.addAttr('head_fk_anim', ln='LidTop_r', dv=0, min=0, max=1, keyable=True)
    cmds.addAttr('head_fk_anim', ln='LidBot_r', dv=0, min=0, max=1, keyable=True)
    
    cmds.connectAttr('head_fk_anim.LidTop_l', "Husky_Head_Shapes.l_TopLid_Full")
    cmds.connectAttr('head_fk_anim.LidBot_l', "Husky_Head_Shapes.l_BottomLid_Full")
    cmds.connectAttr('head_fk_anim.LidTop_r', "Husky_Head_Shapes.r_TopLid_Full")
    cmds.connectAttr('head_fk_anim.LidBot_r', "Husky_Head_Shapes.r_BottomLid_Full")
    
    #toe splay ctrls
    cmds.addAttr('ik_foot_anim_l', ln='ToeSplay_l', dv=0, min=0, max=1, keyable=True)
    cmds.addAttr('ik_foot_anim_r', ln='ToeSplay_r', dv=0, min=0, max=1, keyable=True)
    
    cmds.connectAttr('ik_foot_anim_l.ToeSplay_l', "Husky_Toe_Splay_Morph.husky_toe_splay_l")
    cmds.connectAttr('ik_foot_anim_r.ToeSplay_r', "Husky_Toe_Splay_Morph.husky_toe_splay_r")
    

def adjustCtrls():
    
    #resize spineIK ctrls
    cmds.select('mid_ik_animShape.cv[0:7]', r=True)
    cmds.scale(2.034977, 2.034977, 2.034977, pivot=(-0.136193, 0.090223, 108.581967), r=True)
    cmds.select(clear=True)
    
    cmds.select('chest_ik_animShape.cv[0:7]', r=True)
    cmds.scale(1.753816, 1.753816, 1.753816, pivot=(-0.136176, 0.221525, 122.576471), r=True)
    cmds.select(clear=True)
    
    cmds.select('hip_animShape.cv[0:7]', r=True)
    cmds.scale(1.499092, 1.499092, 1.499092, pivot=(-0.13621, 3.004541, 84.266245), r=True)
    cmds.select(clear=True)
    
    cmds.select('body_animShape.cv[0:4]', r=True)
    cmds.scale(1.475488, 1.475488, 1.475488, pivot=(-0.13621, 3.004541, 84.266245), r=True)
    cmds.select(clear=True)
    
    #resize & move spine adjust ctrls
    cmds.select('spine_adj_01_anim.cv[0:32]', r=True)
    cmds.move(0, 32, 0, r=True)
    cmds.scale(.6, .6, .6, pivot=(-0.136211, 32, 95.467457), r=True)
    cmds.select(clear=True)
    
    cmds.select('spine_adj_02_anim.cv[0:32]', r=True)
    cmds.move(0, 34, 0, r=True)
    cmds.scale(.6, .6, .6, pivot=(-0.136194, 34, 109.881409), r=True)
    cmds.select(clear=True)
    
    cmds.select('spine_adj_03_anim.cv[0:32]', r=True)
    cmds.move(0, 36, 0, r=True)
    cmds.scale(.6, .6, .6, pivot=(-0.136176, 36, 123.437057), r=True)
    cmds.select(clear=True)
    
    #resize sleeve ctrls
    cmds.select('sleeve_l_anim.cv[0:32]', r=True)
    cmds.scale(.4, .4, .4, pivot=(40.815332, -5.415218, 121.590066), r=True)
    cmds.select(clear=True)
    
    cmds.select('sleeve_r_anim.cv[0:32]', r=True)
    cmds.scale(.4, .4, .4, pivot=(-41.087688, -5.415208, 121.590056), r=True)
    cmds.select(clear=True)
    
    cmds.select('sleeve_back_l_anim.cv[0:32]', r=True)
    cmds.scale(.4, .4, .4, pivot=(42.602682, 14.579446, 125.683627), r=True)
    cmds.select(clear=True)
    
    cmds.select('sleeve_back_r_anim.cv[0:32]', r=True)
    cmds.scale(.4, .4, .4, pivot=(-43.067121, 15.516725, 125.934779), r=True)
    cmds.select(clear=True)
    
    cmds.select('sleeve_r_out_anim.cv[0:32]', r=True)
    cmds.scale(.4, .4, .4, pivot=(-43.258822, 3.005118, 133.475517), r=True)
    cmds.select(clear=True)
    
    cmds.select('sleeve_r_in_anim.cv[0:32]', r=True)
    cmds.scale(.4, .4, .4, pivot=(-43.397472, 4.945993, 110.550748), r=True)
    cmds.select(clear=True)
    
    cmds.select('sleeve_l_out_anim.cv[0:32]', r=True)
    cmds.scale(.4, .4, .4, pivot=(43.263298, 1.710193, 134.724607), r=True)
    cmds.select(clear=True)
    
    cmds.select('sleeve_l_in_anim.cv[0:32]', r=True)
    cmds.scale(.4, .4, .4, pivot=(43.060389, 4.686691, 111.214648), r=True)
    cmds.select(clear=True)
    
    #sleeve master ctrls
    cmds.select('sleeve_r_mstr_anim.cv[0:32]', r=True)
    cmds.scale(2, 2, 0, pivot=(-41.777692, 2.147906, 121.991776), r=True)
    cmds.select(clear=True)
    
    cmds.select('sleeve_l_mstr_anim.cv[0:32]', r=True)
    cmds.scale(2, 2, 0, pivot=(41.442621, 1.98828, 122.658464), r=True)
    cmds.select(clear=True)
    
    #resize shirt ctrls
    cmds.select('shirt_l_anim.cv[0:32]', r=True)
    cmds.scale(0.210532, 0.210532, 0.210532, pivot=(27.096686, -0.310665, 90.881367), r=True)
    cmds.select(clear=True)

    cmds.select('shirt_front_anim.cv[0:32]', r=True)
    cmds.scale(0.210532, 0.210532, 0.210532, pivot=(-0.136207, -30.539953, 96.175555), r=True)
    cmds.select(clear=True)
    
    cmds.select('shirt_r_anim.cv[0:32]', r=True)
    cmds.scale(0.210532, 0.210532, 0.210532, pivot=(-27.369117, -0.310671, 90.881429), r=True)
    cmds.select(clear=True)
    
    cmds.select('shirt_back_anim.cv[0:32]', r=True)
    cmds.scale(0.210532, 0.210532, 0.210532, pivot=(-0.136214, 19.377428, 93.954475), r=True)
    cmds.select(clear=True)
    
    #shirt master ctrl
    cmds.select('shirt_mstr_anim.cv[0:32]', r=True)
    cmds.scale(0, 5, 5, pivot=(-0.136211, -0.0442781, 95.467457), r=True)
    cmds.select(clear=True)
    
    #cowl ctrls
    cmds.hide('cowl_front_anim.cv[0:32]')
    # cmds.select('cowl_front_anim.cv[0:32]', r=True)
    # cmds.scale(.3, .3, .3, pivot=(-0.136176, -16.521395, 135.26962), r=True)
    # cmds.select(clear=True)
    
    cmds.hide('cowl_right_anim.cv[0:32]')
    # cmds.select('cowl_right_anim.cv[0:32]', r=True)
    # cmds.scale(.3, .3, .3, pivot=(-11, -6.246503, 140.462624), r=True)
    # cmds.select(clear=True)
    
    cmds.hide('cowl_left_anim.cv[0:32]')
    # cmds.select('cowl_left_anim.cv[0:32]', r=True)
    # cmds.scale(.3, .3, .3, pivot=(11, -6.246504, 140.462624), r=True)
    # cmds.select(clear=True)
    
    #head ctrl
    cmds.select('head_fk_anim.cv[0:7]', r=True)
    cmds.scale(1.5, 1.5, 1.5, pivot=(-0.136135, 0.0957095, 147.128491), r=True)
    cmds.select(clear=True)
    
    
def cowlSDK():

    #set up setDrivenKey for cowl ctrls
    
    #make driver locator
    jawPos = cmds.xform('jaw_anim', q=True, ws=True, t=True)
    cmds.spaceLocator(n="cowlDriver")
    cmds.xform("cowlDriver", ws=True, t=(jawPos[0], jawPos[1], jawPos[2]))
    cmds.makeIdentity('cowlDriver', apply=True, t=1, r=1, s=1, n=0, pn=1)
    
    def resetCowlCtrls():
    #reset the loc and ctrls
        cmds.setAttr('cowlDriver.rx', 0)
        cmds.setAttr('cowlDriver.ry', 0)
        cmds.setAttr('cowlDriver.rz', 0)
        cmds.setAttr('cowl_front_anim_grp.tx', 0)
        cmds.setAttr('cowl_front_anim_grp.ty', 0)
        cmds.setAttr('cowl_front_anim_grp.tz', 0)
        cmds.setAttr('cowl_left_anim_grp.tx', 0)
        cmds.setAttr('cowl_left_anim_grp.ty', 0)
        cmds.setAttr('cowl_left_anim_grp.tz', 0)
        cmds.setAttr('cowl_right_anim_grp.tx', 0)
        cmds.setAttr('cowl_right_anim_grp.ty', 0)
        cmds.setAttr('cowl_right_anim_grp.tz', 0)
    
    #make SDKs
    #driver pos rotX ---------------------------
    #front
    cmds.setAttr('cowlDriver.rx', 0)
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tx', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.ty', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tz', cd='cowlDriver.rx')
    cmds.setAttr('cowlDriver.rx', 20)
    cmds.setAttr('cowl_front_anim_grp.tx', -4)
    cmds.setAttr('cowl_front_anim_grp.ty', 2)
    cmds.setAttr('cowl_front_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tx', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.ty', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tz', cd='cowlDriver.rx')
    #left
    cmds.setAttr('cowlDriver.rx', 0)
    cmds.setAttr('cowl_left_anim_grp.tx', 0)
    cmds.setAttr('cowl_left_anim_grp.ty', 0)
    cmds.setAttr('cowl_left_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tx', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.ty', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tz', cd='cowlDriver.rx')
    cmds.setAttr('cowlDriver.rx', 20)
    cmds.setAttr('cowl_left_anim_grp.tx', -2)
    cmds.setAttr('cowl_left_anim_grp.ty', 0)
    cmds.setAttr('cowl_left_anim_grp.tz', -1)
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tx', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.ty', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tz', cd='cowlDriver.rx')
    #right
    cmds.setAttr('cowlDriver.rx', 0)
    cmds.setAttr('cowl_right_anim_grp.tx', 0)
    cmds.setAttr('cowl_right_anim_grp.ty', 0)
    cmds.setAttr('cowl_right_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tx', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.ty', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tz', cd='cowlDriver.rx')
    cmds.setAttr('cowlDriver.rx', 20)
    cmds.setAttr('cowl_right_anim_grp.tx', -2)
    cmds.setAttr('cowl_right_anim_grp.ty', 0)
    cmds.setAttr('cowl_right_anim_grp.tz', 1)
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tx', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.ty', cd='cowlDriver.rx')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tz', cd='cowlDriver.rx')
    
    resetCowlCtrls()
    #---------------------------------
 
    #driver pos rotZ ---------------------------
    #front
    cmds.setAttr('cowlDriver.rz', 0)
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tz', cd='cowlDriver.rz')
    cmds.setAttr('cowlDriver.rz', 45)
    cmds.setAttr('cowl_front_anim_grp.tx', -1.6)
    cmds.setAttr('cowl_front_anim_grp.ty', 0)
    cmds.setAttr('cowl_front_anim_grp.tz', -1.1)
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tz', cd='cowlDriver.rz')
    #left
    cmds.setAttr('cowlDriver.rz', 0)
    cmds.setAttr('cowl_left_anim_grp.tx', 0)
    cmds.setAttr('cowl_left_anim_grp.ty', 0)
    cmds.setAttr('cowl_left_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tz', cd='cowlDriver.rz')
    cmds.setAttr('cowlDriver.rz', 45)
    cmds.setAttr('cowl_left_anim_grp.tx', -.7)
    cmds.setAttr('cowl_left_anim_grp.ty', 3.5)
    cmds.setAttr('cowl_left_anim_grp.tz', -2)
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tz', cd='cowlDriver.rz')
    #right
    cmds.setAttr('cowlDriver.rz', 0)
    cmds.setAttr('cowl_right_anim_grp.tx', 0)
    cmds.setAttr('cowl_right_anim_grp.ty', 0)
    cmds.setAttr('cowl_right_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tz', cd='cowlDriver.rz')
    cmds.setAttr('cowlDriver.rz', 0)
    cmds.setAttr('cowl_right_anim_grp.tx', 0)
    cmds.setAttr('cowl_right_anim_grp.ty', 0)
    cmds.setAttr('cowl_right_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tz', cd='cowlDriver.rz')
    
    resetCowlCtrls()
    #---------------------------------
    
    
    #driver neg rotZ ---------------------------
    #front
    cmds.setAttr('cowlDriver.rz', 0)
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tz', cd='cowlDriver.rz')
    cmds.setAttr('cowlDriver.rz', -45)
    cmds.setAttr('cowl_front_anim_grp.tx', -1.6)
    cmds.setAttr('cowl_front_anim_grp.ty', 0)
    cmds.setAttr('cowl_front_anim_grp.tz', 1.1)
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tz', cd='cowlDriver.rz')
    #left
    cmds.setAttr('cowlDriver.rz', 0)
    cmds.setAttr('cowl_left_anim_grp.tx', 0)
    cmds.setAttr('cowl_left_anim_grp.ty', 0)
    cmds.setAttr('cowl_left_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tz', cd='cowlDriver.rz')
    cmds.setAttr('cowlDriver.rz', 0)
    cmds.setAttr('cowl_left_anim_grp.tx', 0)
    cmds.setAttr('cowl_left_anim_grp.ty', 0)
    cmds.setAttr('cowl_left_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tz', cd='cowlDriver.rz')
    #right
    cmds.setAttr('cowlDriver.rz', 0)
    cmds.setAttr('cowl_right_anim_grp.tx', 0)
    cmds.setAttr('cowl_right_anim_grp.ty', 0)
    cmds.setAttr('cowl_right_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tz', cd='cowlDriver.rz')
    cmds.setAttr('cowlDriver.rz', -45)
    cmds.setAttr('cowl_right_anim_grp.tx', -.7)
    cmds.setAttr('cowl_right_anim_grp.ty', 3.5)
    cmds.setAttr('cowl_right_anim_grp.tz', 2)
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tx', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.ty', cd='cowlDriver.rz')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tz', cd='cowlDriver.rz')
    
    resetCowlCtrls()
    #---------------------------------
    
    
    #driver pos rotY ---------------------------
    #front
    cmds.setAttr('cowlDriver.ry', 0)
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tz', cd='cowlDriver.ry')
    cmds.setAttr('cowlDriver.ry', 20)
    cmds.setAttr('cowl_front_anim_grp.tx', -2)
    cmds.setAttr('cowl_front_anim_grp.ty', 0)
    cmds.setAttr('cowl_front_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tz', cd='cowlDriver.ry')
    #left
    cmds.setAttr('cowlDriver.ry', 0)
    cmds.setAttr('cowl_left_anim_grp.tx', 0)
    cmds.setAttr('cowl_left_anim_grp.ty', 0)
    cmds.setAttr('cowl_left_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tz', cd='cowlDriver.ry')
    cmds.setAttr('cowlDriver.ry', 20)
    cmds.setAttr('cowl_left_anim_grp.tx', -2)
    cmds.setAttr('cowl_left_anim_grp.ty', 0)
    cmds.setAttr('cowl_left_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tz', cd='cowlDriver.ry')
    #right
    cmds.setAttr('cowlDriver.ry', 0)
    cmds.setAttr('cowl_right_anim_grp.tx', 0)
    cmds.setAttr('cowl_right_anim_grp.ty', 0)
    cmds.setAttr('cowl_right_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tz', cd='cowlDriver.ry')
    cmds.setAttr('cowlDriver.ry', 20)
    cmds.setAttr('cowl_right_anim_grp.tx', -1.3)
    cmds.setAttr('cowl_right_anim_grp.ty', 0)
    cmds.setAttr('cowl_right_anim_grp.tz', 1)
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tz', cd='cowlDriver.ry')
    
    resetCowlCtrls()
    #---------------------------------
    
    
    #driver neg rotY ---------------------------
    #front
    cmds.setAttr('cowlDriver.ry', 0)
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tz', cd='cowlDriver.ry')
    cmds.setAttr('cowlDriver.ry', -20)
    cmds.setAttr('cowl_front_anim_grp.tx', -1.3)
    cmds.setAttr('cowl_front_anim_grp.ty', 0)
    cmds.setAttr('cowl_front_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_front_anim_grp.tz', cd='cowlDriver.ry')
    #left
    cmds.setAttr('cowlDriver.ry', 0)
    cmds.setAttr('cowl_left_anim_grp.tx', 0)
    cmds.setAttr('cowl_left_anim_grp.ty', 0)
    cmds.setAttr('cowl_left_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tz', cd='cowlDriver.ry')
    cmds.setAttr('cowlDriver.ry', -20)
    cmds.setAttr('cowl_left_anim_grp.tx', -1.3)
    cmds.setAttr('cowl_left_anim_grp.ty', 0)
    cmds.setAttr('cowl_left_anim_grp.tz', -1)
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_left_anim_grp.tz', cd='cowlDriver.ry')
    #right
    cmds.setAttr('cowlDriver.ry', 0)
    cmds.setAttr('cowl_right_anim_grp.tx', 0)
    cmds.setAttr('cowl_right_anim_grp.ty', 0)
    cmds.setAttr('cowl_right_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tz', cd='cowlDriver.ry')
    cmds.setAttr('cowlDriver.ry', -20)
    cmds.setAttr('cowl_right_anim_grp.tx', -2)
    cmds.setAttr('cowl_right_anim_grp.ty', 0)
    cmds.setAttr('cowl_right_anim_grp.tz', 0)
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tx', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.ty', cd='cowlDriver.ry')
    cmds.setDrivenKeyframe('cowl_right_anim_grp.tz', cd='cowlDriver.ry')
    
    resetCowlCtrls()
    #---------------------------------
  
    #constrain the loc
    cmds.parentConstraint('jaw_anim', 'cowlDriver', mo=True) 
    

#call functions
createCollisionCylinder()
createMorphCtrls()
adjustCtrls()
#cowlSDK()

#save
cmds.file(save = True, type = "mayaBinary", force = True)