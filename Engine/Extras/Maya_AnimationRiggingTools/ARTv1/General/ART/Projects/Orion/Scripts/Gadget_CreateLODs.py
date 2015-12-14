import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## GADGET #########################################################################
# Warn the user that this operation will save their file.
saveResult = cmds.confirmDialog(title = "Save Warning", message="In order to continue your file must be saved.  Would you like to save it?  If yes it will be saved and after the operation is complete your file will be re-opened.", button = ["Yes", "Cancel"], defaultButton='Yes', cancelButton='Cancel', dismissString='Cancel')

# If they are ok with saving the file, warn them that it will also delete all of the current files in their skin weights directory.
if saveResult == "Yes":
    deleteSkinFilesResult = cmds.confirmDialog(title = "SkinWeights Warning", message="In order to continue we must delete all of the files in your ..\General\ART\SkinWeights\ directory.  Would you like to continue?.", button = ["Yes", "Cancel"], defaultButton='Yes', cancelButton='Cancel', dismissString='Cancel')
    if deleteSkinFilesResult == "Yes":
        toolsPath = cmds.internalVar(usd = True) + "mayaTools.txt"
        if os.path.exists(toolsPath):
            f = open(toolsPath, 'r')
            mayaToolsDir = f.readline()
            f.close()

        path = mayaToolsDir + "\General\ART\SkinWeights\\"
        files = os.listdir(path)
        #delete all weight cache files in the system folder
        for file in files:
            os.remove(path + file)
            #print path + file



# Gadget LOD1 -------------------------------------------------------------------------------------------------------------
def GadgetLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)


        meshes = ["Corneas_Geo_LOD1", "Eyeballs_Geo_LOD1", "LowerBody_Ankle_Geo_LOD1", "LowerBody_Boots_Geo_LOD1", "LowerBody_Gas_Geo_LOD1", "LowerBody_Gren_Geo_LOD1", "LowerBody_KneePad_Geo_LOD1", "LowerBody_Pak_Geo_LOD1", "LowerBody_Pants_Geo_LOD1", "LowerBody_Torso_Geo_LOD1", "Machine_Arm_DogBone_Geo_LOD1", "Machine_Arm_Elbow1_Bott_Geo_LOD1", "Machine_Arm_Elbow1_Top_Geo_LOD1", "Machine_Arm_ElbowPiston_Inner_Geo_LOD1", "Machine_Arm_ElbowPiston_Outer_Geo_LOD1", "Machine_Arm_ForeArm_Geo_LOD1", "Machine_Arm_Hand_Geo_LOD1", "Machine_Arm_HandFiller_Geo_LOD1", "Machine_Arm_KNmid_Geo_LOD1", "Machine_Arm_Pinky_Geo_LOD1", "Machine_Arm_PistonsMid_Geo_LOD1", "Machine_Arm_PistonUpper_Geo_LOD1", "Machine_Arm_PistonUpper_Inner_Geo_LOD1", "Machine_Arm_PistonUpperSmall_Geo_LOD1", "Machine_Arm_PistonUpperSmall_Inner_Geo_LOD1", "Machine_Arm_PistonUpperSmallBoot_Geo_LOD1", "Machine_Arm_Pointer_Geo_LOD1", "Machine_Arm_Thumb_Geo_LOD1", "Machine_Arm_Wires_Geo_LOD1", "Machine_Arm_WristPiston_Inner_Geo_LOD1", "Machine_Arm_WristPiston_Outer_Geo_LOD1", "Machine_Base_Ball_Geo_LOD1", "Machine_Base_Base_Geo_LOD1", "Machine_Base_Pipes_Geo_LOD1", "Machine_Base_ShoulderPadBottom_Geo_LOD1", "Machine_Base_ShoulderPadTop_Geo_LOD1", "Machine_Base_Wire_Big_Geo_LOD1", "Machine_Base_Wire_Small_Geo_LOD1", "Machine_Shoulder_Geo_LOD1", "spring_Geo_LOD1", "Teeth_Lower_Geo_LOD1", "TeethUpper_Geo_LOD1", "Tongue_Geo_LOD1", "turkey_Geo_LOD1", "UpperBody_Arms_Geo_LOD1", "UpperBody_EyeLashes_Geo_LOD1", "UpperBody_ShoulderPad_Geo_LOD1", "UpperBody_Torso_Geo_LOD1"]

        jointsToDelete = []

        # remove unneeded joints

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Gadget_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

GadgetLOD1()

# Gadget LOD2 -------------------------------------------------------------------------------------------------------------
def GadgetLOD2():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["Corneas_Geo_LOD2", "Eyeballs_Geo_LOD2", "LowerBody_Ankle_Geo_LOD2", "LowerBody_Boots_Geo_LOD2", "LowerBody_Gas_Geo_LOD2", "LowerBody_Gren_Geo_LOD2", "LowerBody_KneePad_Geo_LOD2", "LowerBody_Pak_Geo_LOD2", "LowerBody_Pants_Geo_LOD2", "LowerBody_Torso_Geo_LOD2", "Machine_Arm_DogBone_Geo_LOD2", "Machine_Arm_Elbow1_Bott_Geo_LOD2", "Machine_Arm_Elbow1_Top_Geo_LOD2", "Machine_Arm_ElbowPiston_Inner_Geo_LOD2", "Machine_Arm_ElbowPiston_Outer_Geo_LOD2", "Machine_Arm_ForeArm_Geo_LOD2", "Machine_Arm_Hand_Geo_LOD2", "Machine_Arm_HandFiller_Geo_LOD2", "Machine_Arm_KNmid_Geo_LOD2", "Machine_Arm_Pinky_Geo_LOD2", "Machine_Arm_PistonsMid_Geo_LOD2", "Machine_Arm_PistonUpper_Geo_LOD2", "Machine_Arm_PistonUpper_Inner_Geo_LOD2", "Machine_Arm_PistonUpperSmall_Geo_LOD2", "Machine_Arm_PistonUpperSmall_Inner_Geo_LOD2", "Machine_Arm_PistonUpperSmallBoot_Geo_LOD2", "Machine_Arm_Pointer_Geo_LOD2", "Machine_Arm_Thumb_Geo_LOD2", "Machine_Arm_Wires_Geo_LOD2", "Machine_Arm_WristPiston_Inner_Geo_LOD2", "Machine_Arm_WristPiston_Outer_Geo_LOD2", "Machine_Base_Ball_Geo_LOD2", "Machine_Base_Base_Geo_LOD2", "Machine_Base_Pipes_Geo_LOD2", "Machine_Base_ShoulderPadBottom_Geo_LOD2", "Machine_Base_ShoulderPadTop_Geo_LOD2", "Machine_Base_Wire_Big_Geo_LOD2", "Machine_Base_Wire_Small_Geo_LOD2", "Machine_Shoulder_Geo_LOD2", "spring_Geo_LOD2", "Teeth_Lower_Geo_LOD2", "TeethUpper_Geo_LOD2", "Tongue_Geo_LOD2", "turkey_Geo_LOD2", "UpperBody_Arms_Geo_LOD2", "UpperBody_EyeLashes_Geo_LOD2", "UpperBody_ShoulderPad_Geo_LOD2", "UpperBody_Torso_Geo_LOD2"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 23.02290762)
        cmds.setAttr("ring_03_l.rotateZ", 29.40776524)
        cmds.setAttr("pinky_03_l.rotateZ", 39.74899767)
        cmds.setAttr("middle_03_l.rotateZ", 23.93256607)
        cmds.setAttr("index_03_l.rotateZ", 16.68034218)

        cmds.setAttr("thumb_03_r.rotateZ", 23.02290762)
        cmds.setAttr("ring_03_r.rotateZ", 29.40776524)
        cmds.setAttr("pinky_03_r.rotateZ", 39.74899767)
        cmds.setAttr("middle_03_r.rotateZ", 23.93256607)
        cmds.setAttr("index_03_r.rotateZ", 16.68034218)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

         # remove unneeded joints
        jointsToRemove = ["arm_wire_01", "backpack_gas_01", "backpack_gas_02", "shield_bott", "shield_top", "spring"]
        jointToTransferTo = "backpack_base"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_pinky_02"]
        jointToTransferTo = "robo_pinky_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_pointer_02"]
        jointToTransferTo = "robo_pointer_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_thumb_02"]
        jointToTransferTo = "robo_thumb_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_03_l"]
        jointToTransferTo = "index_02_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["middle_03_l"]
        jointToTransferTo = "middle_02_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["pinky_03_l"]
        jointToTransferTo = "pinky_02_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ring_03_l"]
        jointToTransferTo = "ring_02_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thumb_03_l"]
        jointToTransferTo = "thumb_02_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_03_r"]
        jointToTransferTo = "index_02_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["middle_03_r"]
        jointToTransferTo = "middle_02_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["pinky_03_r"]
        jointToTransferTo = "pinky_02_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ring_03_r"]
        jointToTransferTo = "ring_02_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thumb_03_r"]
        jointToTransferTo = "thumb_02_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_mid", "eyebrow_mid_l", "eyebrow_mid_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "helmet_straps_l_01", "helmet_straps_l_02", "helmet_straps_r_01", "helmet_straps_r_02", "jaw"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_pouch_big"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_pouch_sml"]
        jointToTransferTo = "thigh_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Gadget_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

GadgetLOD2()

# Gadget LOD3 -------------------------------------------------------------------------------------------------------------
def GadgetLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["Corneas_Geo_LOD3", "Eyeballs_Geo_LOD3", "LowerBody_Ankle_Geo_LOD3", "LowerBody_Boots_Geo_LOD3", "LowerBody_Gas_Geo_LOD3", "LowerBody_Gren_Geo_LOD3", "LowerBody_KneePad_Geo_LOD3", "LowerBody_Pak_Geo_LOD3", "LowerBody_Pants_Geo_LOD3", "LowerBody_Torso_Geo_LOD3", "Machine_Arm_DogBone_Geo_LOD3", "Machine_Arm_Elbow1_Bott_Geo_LOD3", "Machine_Arm_Elbow1_Top_Geo_LOD3", "Machine_Arm_ElbowPiston_Inner_Geo_LOD3", "Machine_Arm_ElbowPiston_Outer_Geo_LOD3", "Machine_Arm_ForeArm_Geo_LOD3", "Machine_Arm_Hand_Geo_LOD3", "Machine_Arm_HandFiller_Geo_LOD3", "Machine_Arm_KNmid_Geo_LOD3", "Machine_Arm_Pinky_Geo_LOD3", "Machine_Arm_PistonsMid_Geo_LOD3", "Machine_Arm_PistonUpper_Geo_LOD3", "Machine_Arm_PistonUpper_Inner_Geo_LOD3", "Machine_Arm_PistonUpperSmall_Geo_LOD3", "Machine_Arm_PistonUpperSmall_Inner_Geo_LOD3", "Machine_Arm_PistonUpperSmallBoot_Geo_LOD3", "Machine_Arm_Pointer_Geo_LOD3", "Machine_Arm_Thumb_Geo_LOD3", "Machine_Arm_Wires_Geo_LOD3", "Machine_Arm_WristPiston_Inner_Geo_LOD3", "Machine_Arm_WristPiston_Outer_Geo_LOD3", "Machine_Base_Ball_Geo_LOD3", "Machine_Base_Base_Geo_LOD3", "Machine_Base_Pipes_Geo_LOD3", "Machine_Base_ShoulderPadBottom_Geo_LOD3", "Machine_Base_ShoulderPadTop_Geo_LOD3", "Machine_Base_Wire_Big_Geo_LOD3", "Machine_Base_Wire_Small_Geo_LOD3", "Machine_Shoulder_Geo_LOD3", "spring_Geo_LOD3", "Teeth_Lower_Geo_LOD3", "TeethUpper_Geo_LOD3", "Tongue_Geo_LOD3", "turkey_Geo_LOD3", "UpperBody_Arms_Geo_LOD3", "UpperBody_EyeLashes_Geo_LOD3", "UpperBody_ShoulderPad_Geo_LOD3", "UpperBody_Torso_Geo_LOD3"]
    
        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 23.02290762)
        cmds.setAttr("thumb_02_l.rotateZ", 10.06960964)
        cmds.setAttr("thumb_01_l.rotateZ", 8.338113921)
        cmds.setAttr("thumb_01_l.rotateY", 3.002087702)
        cmds.setAttr("thumb_01_l.rotateX", -3.231699475)
        cmds.setAttr("ring_03_l.rotateZ", 29.40776524)
        cmds.setAttr("ring_02_l.rotateZ", 29.40776524)
        cmds.setAttr("ring_01_l.rotateZ", 29.40776524)
        cmds.setAttr("pinky_03_l.rotateZ", 39.74899767)
        cmds.setAttr("pinky_02_l.rotateZ", 39.74899767)
        cmds.setAttr("pinky_01_l.rotateZ", 39.74899767)
        cmds.setAttr("middle_03_l.rotateZ", 23.93256607)
        cmds.setAttr("middle_02_l.rotateZ", 23.93256607)
        cmds.setAttr("middle_01_l.rotateZ", 23.93256607)
        cmds.setAttr("index_03_l.rotateZ", 16.68034218)
        cmds.setAttr("index_02_l.rotateZ", 16.68034218)
        cmds.setAttr("index_01_l.rotateZ", 16.68034218)

        cmds.setAttr("thumb_03_r.rotateZ", 23.02290762)
        cmds.setAttr("thumb_02_r.rotateZ", 10.06960964)
        cmds.setAttr("thumb_01_r.rotateZ", 8.338113921)
        cmds.setAttr("thumb_01_r.rotateY", 3.002087702)
        cmds.setAttr("thumb_01_r.rotateX", -3.231699475)
        cmds.setAttr("ring_03_r.rotateZ", 29.40776524)
        cmds.setAttr("ring_02_r.rotateZ", 29.40776524)
        cmds.setAttr("ring_01_r.rotateZ", 29.40776524)
        cmds.setAttr("pinky_03_r.rotateZ", 39.74899767)
        cmds.setAttr("pinky_02_r.rotateZ", 39.74899767)
        cmds.setAttr("pinky_01_r.rotateZ", 39.74899767)
        cmds.setAttr("middle_03_r.rotateZ", 23.93256607)
        cmds.setAttr("middle_02_r.rotateZ", 23.93256607)
        cmds.setAttr("middle_01_r.rotateZ", 23.93256607)
        cmds.setAttr("index_03_r.rotateZ", 16.68034218)
        cmds.setAttr("index_02_r.rotateZ", 16.68034218)
        cmds.setAttr("index_01_r.rotateZ", 16.68034218)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["arm_wire_01", "backpack_gas_01", "backpack_gas_02", "shield_bott", "shield_top", "spring"]
        jointToTransferTo = "backpack_base"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_uparm_pistonrod_01"]
        jointToTransferTo = "robo_arm_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_uparm_pistonrod"]
        jointToTransferTo = "robo_arm_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_lowarm_pistonrod_01"]
        jointToTransferTo = "robo_arm_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_lowarm_pistonrod_02"]
        jointToTransferTo = "robo_arm_05"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_pinky_02"]
        jointToTransferTo = "robo_pinky_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_pointer_02"]
        jointToTransferTo = "robo_pointer_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_thumb_02"]
        jointToTransferTo = "robo_thumb_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_02_r", "upperarm_twist_01_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_l", "index_02_l", "index_03_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "ring_01_l", "ring_02_l", "ring_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "ring_01_r", "ring_02_r", "ring_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_mid", "eyebrow_mid_l", "eyebrow_mid_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "helmet_straps_l_01", "helmet_straps_l_02", "helmet_straps_r_01", "helmet_straps_r_02", "jaw"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_pouch_big", "thigh_twist_01_l", "thigh_twist_02_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_pouch_sml", "thigh_twist_01_r", "thigh_twist_02_r"]
        jointToTransferTo = "thigh_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Gadget_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

GadgetLOD3()

# Gadget LOD4 -------------------------------------------------------------------------------------------------------------
def GadgetLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["Corneas_Geo_LOD4", "Eyeballs_Geo_LOD4", "LowerBody_Ankle_Geo_LOD4", "LowerBody_Boots_Geo_LOD4", "LowerBody_Gas_Geo_LOD4", "LowerBody_Gren_Geo_LOD4", "LowerBody_KneePad_Geo_LOD4", "LowerBody_Pak_Geo_LOD4", "LowerBody_Pants_Geo_LOD4", "LowerBody_Torso_Geo_LOD4", "Machine_Arm_DogBone_Geo_LOD4", "Machine_Arm_Elbow1_Bott_Geo_LOD4", "Machine_Arm_Elbow1_Top_Geo_LOD4", "Machine_Arm_ElbowPiston_Inner_Geo_LOD4", "Machine_Arm_ElbowPiston_Outer_Geo_LOD4", "Machine_Arm_ForeArm_Geo_LOD4", "Machine_Arm_Hand_Geo_LOD4", "Machine_Arm_HandFiller_Geo_LOD4", "Machine_Arm_KNmid_Geo_LOD4", "Machine_Arm_Pinky_Geo_LOD4", "Machine_Arm_PistonsMid_Geo_LOD4", "Machine_Arm_PistonUpper_Geo_LOD4", "Machine_Arm_PistonUpper_Inner_Geo_LOD4", "Machine_Arm_PistonUpperSmall_Geo_LOD4", "Machine_Arm_PistonUpperSmall_Inner_Geo_LOD4", "Machine_Arm_PistonUpperSmallBoot_Geo_LOD4", "Machine_Arm_Pointer_Geo_LOD4", "Machine_Arm_Thumb_Geo_LOD4", "Machine_Arm_Wires_Geo_LOD4", "Machine_Arm_WristPiston_Inner_Geo_LOD4", "Machine_Arm_WristPiston_Outer_Geo_LOD4", "Machine_Base_Ball_Geo_LOD4", "Machine_Base_Base_Geo_LOD4", "Machine_Base_Pipes_Geo_LOD4", "Machine_Base_ShoulderPadBottom_Geo_LOD4", "Machine_Base_ShoulderPadTop_Geo_LOD4", "Machine_Base_Wire_Big_Geo_LOD4", "Machine_Base_Wire_Small_Geo_LOD4", "Machine_Shoulder_Geo_LOD4", "spring_Geo_LOD4", "Teeth_Lower_Geo_LOD4", "TeethUpper_Geo_LOD4", "Tongue_Geo_LOD4", "turkey_Geo_LOD4", "UpperBody_Arms_Geo_LOD4", "UpperBody_EyeLashes_Geo_LOD4", "UpperBody_ShoulderPad_Geo_LOD4", "UpperBody_Torso_Geo_LOD4"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 23.02290762)
        cmds.setAttr("thumb_02_l.rotateZ", 10.06960964)
        cmds.setAttr("thumb_01_l.rotateZ", 8.338113921)
        cmds.setAttr("thumb_01_l.rotateY", 3.002087702)
        cmds.setAttr("thumb_01_l.rotateX", -3.231699475)
        cmds.setAttr("ring_03_l.rotateZ", 29.40776524)
        cmds.setAttr("ring_02_l.rotateZ", 29.40776524)
        cmds.setAttr("ring_01_l.rotateZ", 29.40776524)
        cmds.setAttr("pinky_03_l.rotateZ", 39.74899767)
        cmds.setAttr("pinky_02_l.rotateZ", 39.74899767)
        cmds.setAttr("pinky_01_l.rotateZ", 39.74899767)
        cmds.setAttr("middle_03_l.rotateZ", 23.93256607)
        cmds.setAttr("middle_02_l.rotateZ", 23.93256607)
        cmds.setAttr("middle_01_l.rotateZ", 23.93256607)
        cmds.setAttr("index_03_l.rotateZ", 16.68034218)
        cmds.setAttr("index_02_l.rotateZ", 16.68034218)
        cmds.setAttr("index_01_l.rotateZ", 16.68034218)

        cmds.setAttr("thumb_03_r.rotateZ", 23.02290762)
        cmds.setAttr("thumb_02_r.rotateZ", 10.06960964)
        cmds.setAttr("thumb_01_r.rotateZ", 8.338113921)
        cmds.setAttr("thumb_01_r.rotateY", 3.002087702)
        cmds.setAttr("thumb_01_r.rotateX", -3.231699475)
        cmds.setAttr("ring_03_r.rotateZ", 29.40776524)
        cmds.setAttr("ring_02_r.rotateZ", 29.40776524)
        cmds.setAttr("ring_01_r.rotateZ", 29.40776524)
        cmds.setAttr("pinky_03_r.rotateZ", 39.74899767)
        cmds.setAttr("pinky_02_r.rotateZ", 39.74899767)
        cmds.setAttr("pinky_01_r.rotateZ", 39.74899767)
        cmds.setAttr("middle_03_r.rotateZ", 23.93256607)
        cmds.setAttr("middle_02_r.rotateZ", 23.93256607)
        cmds.setAttr("middle_01_r.rotateZ", 23.93256607)
        cmds.setAttr("index_03_r.rotateZ", 16.68034218)
        cmds.setAttr("index_02_r.rotateZ", 16.68034218)
        cmds.setAttr("index_01_r.rotateZ", 16.68034218)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["arm_wire_01", "backpack_gas_01", "backpack_gas_02", "shield_bott", "shield_top", "spring"]
        jointToTransferTo = "backpack_base"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_uparm_pistonrod_01"]
        jointToTransferTo = "robo_arm_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_uparm_pistonrod"]
        jointToTransferTo = "robo_arm_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_lowarm_pistonrod_01"]
        jointToTransferTo = "robo_arm_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_lowarm_pistonrod_02"]
        jointToTransferTo = "robo_arm_05"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["robo_pinky_01", "robo_pinky_02", "robo_pointer_01", "robo_pointer_02", "robo_thumb_01", "robo_thumb_02"]
        jointToTransferTo = "robo_arm_07"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_02_r", "upperarm_twist_01_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_l", "index_02_l", "index_03_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "ring_01_l", "ring_02_l", "ring_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "ring_01_r", "ring_02_r", "ring_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_mid", "eyebrow_mid_l", "eyebrow_mid_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "helmet_straps_l_01", "helmet_straps_l_02", "helmet_straps_r_01", "helmet_straps_r_02", "jaw"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_pouch_big", "thigh_twist_01_l", "thigh_twist_02_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_pouch_sml", "thigh_twist_01_r", "thigh_twist_02_r"]
        jointToTransferTo = "thigh_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ball_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ball_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Gadget_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

GadgetLOD4()


