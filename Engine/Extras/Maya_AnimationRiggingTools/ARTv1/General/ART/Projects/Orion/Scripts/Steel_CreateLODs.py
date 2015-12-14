import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## STEEL #########################################################################
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

# Steel LOD1 -------------------------------------------------------------------------------------------------------------
def SteelLOD1():
    if saveResult == "Yes":
       
        cmds.file(save=True)

        meshes = ["geo_acc_LOD1", "geo_achiles_LOD1", "geo_arm_shield_bott_l_LOD1", "geo_arm_shield_top_l_LOD1", "geo_boot_cover_LOD1", "geo_chest_plate_LOD1", "geo_eyeballs_LOD1", "geo_fingers_base_LOD1", "geo_fingers_tips_LOD1", "geo_forearms_LOD1", "geo_hand_plates_LOD1", "geo_head_LOD1", "geo_heels_LOD1", "geo_helmet_LOD1", "geo_knee_pads_LOD1", "geo_legs_LOD1", "geo_lower_teeth_LOD1", "geo_nose_ring_LOD1", "geo_shield_LOD1", "geo_shins_LOD1", "geo_shoulders_LOD1", "geo_straps_LOD1", "geo_toe_plates_LOD1", "geo_toes_LOD1", "geo_tongue_LOD1", "geo_torso_LOD1", "geo_turbine_center_LOD1", "geo_turbine_outer_LOD1", "geo_upper_teeth_LOD1"]

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["loarm_shield_back_l", "lowerarm_twist_01_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerback_muscle_l", "lowerback_muscle_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tricep_l"]
        jointToTransferTo = "upperarm_twist_02_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        jointsToRemove = ["tricep_r"]
        jointToTransferTo = "upperarm_twist_02_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        jointsToRemove = ["toe_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        jointsToRemove = ["toe_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        jointsToRemove = ["calf_armor_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        jointsToRemove = ["calf_armor_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Steel_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

SteelLOD1()

# Steel LOD2 -------------------------------------------------------------------------------------------------------------
def SteelLOD2():
    if saveResult == "Yes":
        
        cmds.file(save=True)
        
        meshes = ["geo_acc_LOD2", "geo_achiles_LOD2", "geo_arm_shield_bott_l_LOD2", "geo_arm_shield_top_l_LOD2", "geo_boot_cover_LOD2", "geo_chest_plate_LOD2", "geo_eyeballs_LOD2", "geo_fingers_base_LOD2", "geo_fingers_tips_LOD2", "geo_forearms_LOD2", "geo_hand_plates_LOD2", "geo_head_LOD2", "geo_heels_LOD2", "geo_helmet_LOD2", "geo_knee_pads_LOD2", "geo_legs_LOD2", "geo_shield_LOD2", "geo_shins_LOD2", "geo_shoulders_LOD2", "geo_straps_LOD2", "geo_toe_plates_LOD2", "geo_toes_LOD2", "geo_torso_LOD2", "geo_turbine_center_LOD2", "geo_turbine_outer_LOD2"]

        jointsToDelete = []

         # remove unneeded joints
        jointsToRemove = ["loarm_shield_back_l", "loarm_shield_bot_l", "loarm_shield_top_l", "lowerarm_twist_01_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerback_muscle_l", "lowerback_muscle_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tricep_l"]
        jointToTransferTo = "upperarm_twist_02_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tricep_r"]
        jointToTransferTo = "upperarm_twist_02_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["football_pads_back", "turbine_back_main", "turbine_back_inner", "football_pads_front", "chest_strap_l", "chest_strap_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["flap_back", "flap_front", "hip_strap_l", "hip_strap_r", "tail_pad"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_plate_l"]
        jointToTransferTo = "ball_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_armor_l", "shinpad_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_l", "thigh_twist_02_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shinpad_r", "calf_armor_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_plate_r"]
        jointToTransferTo = "ball_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_r", "thigh_twist_02_r"]
        jointToTransferTo = "thigh_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Steel_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

SteelLOD2()

# Steel LOD3 -------------------------------------------------------------------------------------------------------------
def SteelLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["geo_achiles_LOD3", "geo_arm_shield_bott_l_LOD3", "geo_arm_shield_top_l_LOD3", "geo_boot_cover_LOD3", "geo_chest_plate_LOD3", "geo_fingers_base_LOD3", "geo_fingers_tips_LOD3", "geo_forearms_LOD3", "geo_hand_plates_LOD3", "geo_head_LOD3", "geo_heels_LOD3", "geo_helmet_LOD3", "geo_knee_pads_LOD3", "geo_legs_LOD3", "geo_shield_LOD3", "geo_shins_LOD3", "geo_shoulders_LOD3", "geo_toe_plates_LOD3", "geo_toes_LOD3", "geo_torso_LOD3", "geo_turbine_center_LOD3", "geo_turbine_outer_LOD3"]
    
        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_02_r.rotateZ", 14.0)
        cmds.setAttr("ring_02_r.rotateZ", 45.0)
        cmds.setAttr("pinky_02_r.rotateZ", 45.0)
        cmds.setAttr("middle_02_r.rotateZ", 45.0)
        cmds.setAttr("index_02_r.rotateZ", 45.0)
        cmds.setAttr("pinky_02_l.rotateZ", 45.0)
        cmds.setAttr("middle_02_l.rotateZ", 45.0)
        cmds.setAttr("index_02_l.rotateZ", 45.0)
        cmds.setAttr("ring_02_l.rotateZ", 45.0)
        cmds.setAttr("thumb_02_l.rotateZ", 14.0)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["flap_back", "flap_front", "hip_strap_l", "hip_strap_r", "tail_pad"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerback_muscle_l", "lowerback_muscle_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["football_pads_back", "turbine_back_main", "turbine_back_inner", "football_pads_front", "chest_strap_l", "chest_strap_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l", "arm_strap_bott_l", "arm_strap_top_l", "tricep_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r", "arm_strap_bott_r", "arm_strap_top_r", "tricep_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["loarm_shield_back_l", "loarm_shield_bot_l", "loarm_shield_top_l", "lowerarm_twist_01_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_02_l"]
        jointToTransferTo = "index_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["middle_02_l"]
        jointToTransferTo = "middle_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["pinky_02_l"]
        jointToTransferTo = "pinky_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ring_02_l"]
        jointToTransferTo = "ring_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thumb_02_l"]
        jointToTransferTo = "thumb_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_02_r"]
        jointToTransferTo = "index_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["middle_02_r"]
        jointToTransferTo = "middle_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["pinky_02_r"]
        jointToTransferTo = "pinky_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ring_02_r"]
        jointToTransferTo = "ring_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thumb_02_r"]
        jointToTransferTo = "thumb_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        jointsToRemove = ["thigh_twist_01_l", "thigh_twist_02_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_armor_l", "shinpad_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_plate_l", "toe_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_r", "thigh_twist_02_r"]
        jointToTransferTo = "thigh_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_armor_r", "shinpad_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_plate_r", "toe_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Steel_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

SteelLOD3()

# Steel LOD4 -------------------------------------------------------------------------------------------------------------
def SteelLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["geo_acc_LOD4", "geo_achiles_LOD4", "geo_arm_shield_bott_l_LOD4", "geo_arm_shield_top_l_LOD4", "geo_boot_cover_LOD4", "geo_chest_plate_LOD4", "geo_fingers_base_LOD4", "geo_fingers_tips_LOD4", "geo_forearms_LOD4", "geo_hand_plates_LOD4", "geo_head_LOD4", "geo_heels_LOD4", "geo_helmet_LOD4", "geo_knee_pads_LOD4", "geo_legs_LOD4", "geo_shield_LOD4", "geo_shins_LOD4", "geo_shoulders_LOD4", "geo_toe_plates_LOD4", "geo_toes_LOD4", "geo_torso_LOD4"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_02_r.rotateZ", 27.33517469)
        cmds.setAttr("thumb_01_r.rotateZ", 27.33517469)
        cmds.setAttr("ring_02_r.rotateZ", 81.26462407)
        cmds.setAttr("ring_01_r.rotateZ", 81.26462407)
        cmds.setAttr("pinky_02_r.rotateZ", 81.26462407)
        cmds.setAttr("pinky_01_r.rotateZ", 81.26462407)
        cmds.setAttr("middle_02_r.rotateZ", 81.26462407)
        cmds.setAttr("middle_01_r.rotateZ", 81.26462407)
        cmds.setAttr("index_02_r.rotateZ", 81.26462407)
        cmds.setAttr("index_01_r.rotateZ", 81.26462407)
        cmds.setAttr("pinky_02_l.rotateZ", 77.13683837)
        cmds.setAttr("pinky_01_l.rotateZ", 77.13683837)
        cmds.setAttr("middle_02_l.rotateZ", 77.13683837)
        cmds.setAttr("middle_01_l.rotateZ", 77.13683837)
        cmds.setAttr("index_02_l.rotateZ", 77.13683837)
        cmds.setAttr("index_01_l.rotateZ", 77.13683837)
        cmds.setAttr("ring_02_l.rotateZ", 77.13683837)
        cmds.setAttr("ring_01_l.rotateZ", 77.13683837)
        cmds.setAttr("thumb_02_l.rotateZ", 31.92338728)
        cmds.setAttr("thumb_01_l.rotateZ", 31.92338728)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["flap_back", "flap_front", "hip_strap_l", "hip_strap_r", "tail_pad"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerback_muscle_l", "lowerback_muscle_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["football_pads_back", "turbine_back_main", "turbine_back_inner", "football_pads_front", "chest_strap_l", "chest_strap_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l", "arm_strap_bott_l", "arm_strap_top_l", "tricep_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r", "arm_strap_bott_r", "arm_strap_top_r", "tricep_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["loarm_shield_back_l", "loarm_shield_bot_l", "loarm_shield_top_l", "lowerarm_twist_01_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_l", "index_02_l", "middle_01_l", "middle_02_l", "pinky_01_l", "pinky_02_l", "ring_01_l", "ring_02_l", "thumb_01_l", "thumb_02_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_r", "index_02_r", "middle_01_r", "middle_02_r", "pinky_01_r", "pinky_02_r", "ring_01_r", "ring_02_r", "thumb_01_r", "thumb_02_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_l", "thigh_twist_02_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_armor_l", "shinpad_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ball_l", "toe_plate_l", "toe_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_r", "thigh_twist_02_r"]
        jointToTransferTo = "thigh_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_armor_r", "shinpad_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ball_r", "toe_plate_r", "toe_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Steel_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

SteelLOD4()


