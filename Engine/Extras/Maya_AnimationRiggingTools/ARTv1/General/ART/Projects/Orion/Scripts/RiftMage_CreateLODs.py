import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## RIFTMAGE #########################################################################
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



# RiftMage LOD1 -------------------------------------------------------------------------------------------------------------
def RiftMageLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)

        meshes = ["arms_geo_LOD1", "beard_geo_LOD1", "brooch_geo_LOD1", "buttstrap_geo_LOD1", "chest_buttons_geo_LOD1", "chest_flaps_geo_LOD1", "codpiece_buttons_geo_LOD1", "codpiece_geo_LOD1", "collar_geo_LOD1", "cowl_geo_LOD1", "elbowpad_geo_LOD1", "eye_tear_duct_geo_LOD1", "eye_wetness_geo_LOD1", "eyeballs_geo_LOD1", "eyelash_geo_LOD1", "eyeshadow_geo_LOD1", "flappendant_geo_LOD1", "forearm_pads_geo_LOD1", "head_geo_LOD1", "hip_parts_geo_LOD1", "hood_geo_LOD1", "inner_ring_geo_LOD1", "kneepads_geo_LOD1", "legplate_geo_LOD1", "legs_geo_LOD1", "midflaps_geo_LOD1", "outer_ring_bott_geo_LOD1", "outer_ring_inside_geo_LOD1", "outer_ring_knobs_geo_LOD1", "outer_ring_top_geo_LOD1", "portal_geo_LOD1", "sideflaps_geo_LOD1", "teeth_lower_geo_LOD1", "teeth_upper_geo_LOD1", "tongue_geo_LOD1", "torso_detail_geo_LOD1", "torso_geo_LOD1", "wrist_armor_geo_LOD1"]

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["cloth_chest_bk_l", "cloth_chest_bk_r", "cloth_chest_fr_l", "cloth_chest_fr_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["cowl_bk", "cowl_l", "cowl_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_inner_l", "eyebrow_inner_r", "eyebrow_mid", "eyebrow_outer_l", "eyebrow_outer_r", "hood_l", "hood_r"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        jointsToRemove = ["outer_ring_A_piston01_l", "outer_ring_A_piston02_l"]
        jointToTransferTo = "outer_ring_A_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["outer_ring_B_piston01_l"]
        jointToTransferTo = "outer_ring_B_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["outer_ring_A_piston01_r", "outer_ring_A_piston02_r"]
        jointToTransferTo = "outer_ring_A_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["outer_ring_B_piston01_r"]
        jointToTransferTo = "outer_ring_B_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)


        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("RiftMage_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

RiftMageLOD1()

# RiftMage LOD2 -------------------------------------------------------------------------------------------------------------
def RiftMageLOD2():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["arms_geo_LOD2", "beard_geo_LOD2", "brooch_geo_LOD2", "buttstrap_geo_LOD2", "chest_buttons_geo_LOD2", "chest_flaps_geo_LOD2", "codpiece_buttons_geo_LOD2", "codpiece_geo_LOD2", "collar_geo_LOD2", "cowl_geo_LOD2", "elbowpad_geo_LOD2", "eye_tear_duct_geo_LOD2", "eye_wetness_geo_LOD2", "eyeballs_geo_LOD2", "eyelash_geo_LOD2", "eyeshadow_geo_LOD2", "flappendant_geo_LOD2", "forearm_pads_geo_LOD2", "head_geo_LOD2", "hip_parts_geo_LOD2", "hood_geo_LOD2", "inner_ring_geo_LOD2", "kneepads_geo_LOD2", "legplate_geo_LOD2", "legs_geo_LOD2", "midflaps_geo_LOD2", "outer_ring_bott_geo_LOD2", "outer_ring_inside_geo_LOD2", "outer_ring_knobs_geo_LOD2", "outer_ring_top_geo_LOD2", "portal_geo_LOD2", "sideflaps_geo_LOD2", "teeth_lower_geo_LOD2", "teeth_upper_geo_LOD2", "tongue_geo_LOD2", "torso_detail_geo_LOD2", "torso_geo_LOD2", "wrist_armor_geo_LOD2"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 23.02290762)
        cmds.setAttr("thumb_02_l.rotateZ", 10.06960964)
        cmds.setAttr("ring_03_l.rotateZ", 29.40776524)
        cmds.setAttr("ring_02_l.rotateZ", 29.40776524)
        cmds.setAttr("pinky_03_l.rotateZ", 39.74899767)
        cmds.setAttr("pinky_02_l.rotateZ", 39.74899767)
        cmds.setAttr("middle_03_l.rotateZ", 23.93256607)
        cmds.setAttr("middle_02_l.rotateZ", 23.93256607)
        cmds.setAttr("index_03_l.rotateZ", 16.68034218)
        cmds.setAttr("index_02_l.rotateZ", 16.68034218)

        cmds.setAttr("thumb_03_r.rotateZ", 23.02290762)
        cmds.setAttr("thumb_02_r.rotateZ", 10.06960964)
        cmds.setAttr("ring_03_r.rotateZ", 29.40776524)
        cmds.setAttr("ring_02_r.rotateZ", 29.40776524)
        cmds.setAttr("pinky_03_r.rotateZ", 39.74899767)
        cmds.setAttr("pinky_02_r.rotateZ", 39.74899767)
        cmds.setAttr("middle_03_r.rotateZ", 23.93256607)
        cmds.setAttr("middle_02_r.rotateZ", 23.93256607)
        cmds.setAttr("index_03_r.rotateZ", 16.68034218)
        cmds.setAttr("index_02_r.rotateZ", 16.68034218)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

         # remove unneeded joints
        jointsToRemove = ["loin_cloth_bk_03"]
        jointToTransferTo = "loin_cloth_bk_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["loin_cloth_fr_03"]
        jointToTransferTo = "loin_cloth_fr_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chest_cloth_03", "chest_cloth_04"]
        jointToTransferTo = "chest_cloth_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["cloth_chest_bk_l", "cloth_chest_bk_r", "cloth_chest_fr_l", "cloth_chest_fr_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["cowl_bk", "cowl_l", "cowl_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_l", "lowerarm_twist_02_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r", "lowerarm_twist_02_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_02_l", "index_03_l"]
        jointToTransferTo = "index_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["middle_02_l", "middle_03_l"]
        jointToTransferTo = "middle_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["pinky_02_l", "pinky_03_l"]
        jointToTransferTo = "pinky_03_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ring_02_l", "ring_03_l"]
        jointToTransferTo = "ring_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "thumb_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_02_r", "index_03_r"]
        jointToTransferTo = "index_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["middle_02_r", "middle_03_r"]
        jointToTransferTo = "middle_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["pinky_02_r", "pinky_03_r"]
        jointToTransferTo = "pinky_03_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ring_02_r", "ring_03_r"]
        jointToTransferTo = "ring_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "thumb_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_inner_l", "eyebrow_inner_r", "eyebrow_mid", "eyebrow_outer_l", "eyebrow_outer_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "hood_l", "hood_r"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["outer_ring_A_piston01_l", "outer_ring_A_piston02_l", "outer_ring_B_l", "outer_ring_B_piston01_l", "outer_ring_C_l"]
        jointToTransferTo = "outer_ring_A_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["outer_ring_A_piston01_r", "outer_ring_A_piston02_r", "outer_ring_B_r", "outer_ring_B_piston01_r", "outer_ring_C_r"]
        jointToTransferTo = "outer_ring_A_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["portal"]
        jointToTransferTo = "portal_ring"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_l", "thigh_twist_02_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_r", "thigh_twist_02_r"]
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
        export.exportLOD("RiftMage_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

RiftMageLOD2()

# RiftMage LOD3 -------------------------------------------------------------------------------------------------------------
def RiftMageLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["arms_geo_LOD3", "beard_geo_LOD3", "brooch_geo_LOD3", "buttstrap_geo_LOD3", "chest_buttons_geo_LOD3", "chest_flaps_geo_LOD3", "codpiece_buttons_geo_LOD3", "codpiece_geo_LOD3", "collar_geo_LOD3", "cowl_geo_LOD3", "elbowpad_geo_LOD3", "eye_tear_duct_geo_LOD3", "eye_wetness_geo_LOD3", "eyeballs_geo_LOD3", "eyelash_geo_LOD3", "eyeshadow_geo_LOD3", "flappendant_geo_LOD3", "forearm_pads_geo_LOD3", "head_geo_LOD3", "hip_parts_geo_LOD3", "hood_geo_LOD3", "inner_ring_geo_LOD3", "kneepads_geo_LOD3", "legplate_geo_LOD3", "legs_geo_LOD3", "midflaps_geo_LOD3", "outer_ring_bott_geo_LOD3", "outer_ring_inside_geo_LOD3", "outer_ring_knobs_geo_LOD3", "outer_ring_top_geo_LOD3", "portal_geo_LOD3", "sideflaps_geo_LOD3", "teeth_lower_geo_LOD3", "teeth_upper_geo_LOD3", "tongue_geo_LOD3", "torso_detail_geo_LOD3", "torso_geo_LOD3", "wrist_armor_geo_LOD3"]
    
        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 23.02290762)
        cmds.setAttr("thumb_02_l.rotateZ", 10.06960964)
        cmds.setAttr("ring_03_l.rotateZ", 29.40776524)
        cmds.setAttr("ring_02_l.rotateZ", 29.40776524)
        cmds.setAttr("pinky_03_l.rotateZ", 39.74899767)
        cmds.setAttr("pinky_02_l.rotateZ", 39.74899767)
        cmds.setAttr("middle_03_l.rotateZ", 23.93256607)
        cmds.setAttr("middle_02_l.rotateZ", 23.93256607)
        cmds.setAttr("index_03_l.rotateZ", 16.68034218)
        cmds.setAttr("index_02_l.rotateZ", 16.68034218)

        cmds.setAttr("thumb_03_r.rotateZ", 23.02290762)
        cmds.setAttr("thumb_02_r.rotateZ", 10.06960964)
        cmds.setAttr("ring_03_r.rotateZ", 29.40776524)
        cmds.setAttr("ring_02_r.rotateZ", 29.40776524)
        cmds.setAttr("pinky_03_r.rotateZ", 39.74899767)
        cmds.setAttr("pinky_02_r.rotateZ", 39.74899767)
        cmds.setAttr("middle_03_r.rotateZ", 23.93256607)
        cmds.setAttr("middle_02_r.rotateZ", 23.93256607)
        cmds.setAttr("index_03_r.rotateZ", 16.68034218)
        cmds.setAttr("index_02_r.rotateZ", 16.68034218)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["hips_cloth_main_l", "hips_cloth_bk_l_01", "hips_cloth_bk_l_02", "hips_cloth_fr_l_01", "hips_cloth_fr_l_02", "hips_cloth_main_r", "hips_cloth_bk_r_01", "hips_cloth_bk_r_02", "hips_cloth_fr_r_01", "hips_cloth_fr_r_02", "loin_cloth_bk_01", "loin_cloth_bk_02", "loin_cloth_bk_03", "loin_cloth_fr_01", "loin_cloth_fr_02", "loin_cloth_fr_03"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chest_cloth_01", "chest_cloth_02", "chest_cloth_03", "chest_cloth_04", "cloth_chest_bk_l", "cloth_chest_bk_r", "cloth_chest_fr_l", "cloth_chest_fr_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["cowl_bk", "cowl_l", "cowl_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_l", "lowerarm_twist_02_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r", "lowerarm_twist_02_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_02_l", "index_03_l"]
        jointToTransferTo = "index_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["middle_02_l", "middle_03_l"]
        jointToTransferTo = "middle_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["pinky_02_l", "pinky_03_l"]
        jointToTransferTo = "pinky_03_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ring_02_l", "ring_03_l"]
        jointToTransferTo = "ring_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "thumb_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_02_r", "index_03_r"]
        jointToTransferTo = "index_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["middle_02_r", "middle_03_r"]
        jointToTransferTo = "middle_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["pinky_02_r", "pinky_03_r"]
        jointToTransferTo = "pinky_03_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ring_02_r", "ring_03_r"]
        jointToTransferTo = "ring_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "thumb_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_inner_l", "eyebrow_inner_r", "eyebrow_mid", "eyebrow_outer_l", "eyebrow_outer_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "hood_l", "hood_r", "jaw"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["outer_ring_A_l", "outer_ring_A_piston01_l", "outer_ring_A_piston02_l", "outer_ring_B_l", "outer_ring_B_piston01_l", "outer_ring_C_l", "outer_ring_A_r", "outer_ring_A_piston01_r", "outer_ring_A_piston02_r", "outer_ring_B_r", "outer_ring_B_piston01_r", "outer_ring_C_r"]
        jointToTransferTo = "outer_ring_base"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["portal"]
        jointToTransferTo = "portal_ring"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_l", "thigh_twist_02_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_r", "thigh_twist_02_r"]
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
        export.exportLOD("RiftMage_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

RiftMageLOD3()

# RiftMage LOD4 -------------------------------------------------------------------------------------------------------------
def RiftMageLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["arms_geo_LOD4", "beard_geo_LOD4", "brooch_geo_LOD4", "buttstrap_geo_LOD4", "chest_buttons_geo_LOD4", "chest_flaps_geo_LOD4", "codpiece_buttons_geo_LOD4", "codpiece_geo_LOD4", "collar_geo_LOD4", "cowl_geo_LOD4", "elbowpad_geo_LOD4", "eye_tear_duct_geo_LOD4", "eye_wetness_geo_LOD4", "eyeballs_geo_LOD4", "eyelash_geo_LOD4", "eyeshadow_geo_LOD4", "flappendant_geo_LOD4", "forearm_pads_geo_LOD4", "head_geo_LOD4", "hip_parts_geo_LOD4", "hood_geo_LOD4", "inner_ring_geo_LOD4", "kneepads_geo_LOD4", "legplate_geo_LOD4", "legs_geo_LOD4", "midflaps_geo_LOD4", "outer_ring_bott_geo_LOD4", "outer_ring_inside_geo_LOD4", "outer_ring_knobs_geo_LOD4", "outer_ring_top_geo_LOD4", "portal_geo_LOD4", "sideflaps_geo_LOD4", "teeth_lower_geo_LOD4", "teeth_upper_geo_LOD4", "tongue_geo_LOD4", "torso_detail_geo_LOD4", "torso_geo_LOD4", "wrist_armor_geo_LOD4"]

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
        jointsToRemove = ["hips_cloth_main_l", "hips_cloth_bk_l_01", "hips_cloth_bk_l_02", "hips_cloth_fr_l_01", "hips_cloth_fr_l_02", "hips_cloth_main_r", "hips_cloth_bk_r_01", "hips_cloth_bk_r_02", "hips_cloth_fr_r_01", "hips_cloth_fr_r_02", "loin_cloth_bk_01", "loin_cloth_bk_02", "loin_cloth_bk_03", "loin_cloth_fr_01", "loin_cloth_fr_02", "loin_cloth_fr_03"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chest_cloth_01", "chest_cloth_02", "chest_cloth_03", "chest_cloth_04", "cloth_chest_bk_l", "cloth_chest_bk_r", "cloth_chest_fr_l", "cloth_chest_fr_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["cowl_bk", "cowl_l", "cowl_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_l", "lowerarm_twist_02_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r", "lowerarm_twist_02_r"]
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

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_inner_l", "eyebrow_inner_r", "eyebrow_mid", "eyebrow_outer_l", "eyebrow_outer_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "hood_l", "hood_r", "jaw"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["outer_ring_A_l", "outer_ring_A_piston01_l", "outer_ring_A_piston02_l", "outer_ring_B_l", "outer_ring_B_piston01_l", "outer_ring_C_l", "outer_ring_A_r", "outer_ring_A_piston01_r", "outer_ring_A_piston02_r", "outer_ring_B_r", "outer_ring_B_piston01_r", "outer_ring_C_r"]
        jointToTransferTo = "outer_ring_base"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["portal"]
        jointToTransferTo = "portal_ring"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_l", "thigh_twist_02_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_r", "thigh_twist_02_r"]
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
        export.exportLOD("RiftMage_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

RiftMageLOD4()


