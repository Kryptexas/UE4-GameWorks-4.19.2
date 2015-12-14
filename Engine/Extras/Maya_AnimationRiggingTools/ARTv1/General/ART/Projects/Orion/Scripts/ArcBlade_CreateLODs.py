import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## ARCBLADE #########################################################################
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



# ArcBlade LOD1 -------------------------------------------------------------------------------------------------------------
def ArcBladeLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)

        meshes = ["arm_spheres_geo_LOD1", "arms_geo_LOD1", "back_sash_geo_LOD1", "bracelets_geo_LOD1", "eye_tear_duct_geo_LOD1", "eye_wetness_geo_LOD1", "eyeballs_geo_LOD1", "eyelashes_geo_LOD1", "eyeshadow_geo_LOD1", "feet_geo_LOD1", "forearm_armor_geo_LOD1", "hair_ponytail_holder_geo_LOD1", "hair_sheets_geo_LOD1", "hair_tails_geo_LOD1", "head_geo_LOD1", "hip_flaps_geo_LOD1", "hips_armor_geo_LOD1", "knees_geo_LOD1", "loin_cloths_geo_LOD1", "pipe_geo_LOD1", "shoulder_armor_r_geo_LOD1", "shoulder_sash_l_geo_LOD1", "shoulder_underarmor_r_geo_LOD1", "teeth_lower_geo_LOD1", "teeth_upper_geo_LOD1", "thighs_geo_LOD1", "thin_flaps_geo_LOD1", "tongue_geo_LOD1", "torso_geo_LOD1", "upperarm_armor_geo_LOD1", "waist_band_geo_LOD1", "weapon_blade_01_geo_LOD1", "weapon_blade_02_geo_LOD1", "weapon_blade_03_geo_LOD1", "weapon_blade_04_geo_LOD1", "weapon_blade_05_geo_LOD1", "weapon_chain_geo_LOD1", "weapon_handle_geo_LOD1"]

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["hip_eyelet_rear_l_03"]
        jointToTransferTo = "hip_eyelet_rear_l_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hip_eyelet_rear_r_03"]
        jointToTransferTo = "hip_eyelet_rear_r_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hip_eyelet_front_l_03"]
        jointToTransferTo = "hip_eyelet_front_l_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hip_eyelet_front_r_03"]
        jointToTransferTo = "hip_eyelet_front_r_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["back_sash_01", "back_sash_02", "back_sash_03"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulder_cloth_01_l", "shoulder_cloth_02_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["bracelet_A_l", "bracelet_B_l", "bracelet_C_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["wrist_plate_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("ArcBlade_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

ArcBladeLOD1()

# ArcBlade LOD2 -------------------------------------------------------------------------------------------------------------
def ArcBladeLOD2():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["arm_spheres_geo_LOD2", "arms_geo_LOD2", "back_sash_geo_LOD2", "bracelets_geo_LOD2", "eye_tear_duct_geo_LOD2", "eye_wetness_geo_LOD2", "eyeballs_geo_LOD2", "eyelashes_geo_LOD2", "eyeshadow_geo_LOD2", "feet_geo_LOD2", "forearm_armor_geo_LOD2", "hair_ponytail_holder_geo_LOD2", "hair_sheets_geo_LOD2", "hair_tails_geo_LOD2", "head_geo_LOD2", "hip_flaps_geo_LOD2", "hips_armor_geo_LOD2", "knees_geo_LOD2", "loin_cloths_geo_LOD2", "pipe_geo_LOD2", "shoulder_armor_r_geo_LOD2", "shoulder_sash_l_geo_LOD2", "shoulder_underarmor_r_geo_LOD2", "teeth_lower_geo_LOD2", "teeth_upper_geo_LOD2", "thighs_geo_LOD2", "thin_flaps_geo_LOD2", "tongue_geo_LOD2", "torso_geo_LOD2", "upperarm_armor_geo_LOD2", "waist_band_geo_LOD2", "weapon_blade_01_geo_LOD2", "weapon_blade_02_geo_LOD2", "weapon_blade_03_geo_LOD2", "weapon_blade_04_geo_LOD2", "weapon_blade_05_geo_LOD2", "weapon_chain_geo_LOD2", "weapon_handle_geo_LOD1"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 30.05735749)
        cmds.setAttr("thumb_02_l.rotateZ", 22.03618142)
        cmds.setAttr("thumb_02_l.rotateY", 5.675048056)
        cmds.setAttr("thumb_02_l.rotateX", 22.94177697)
        cmds.setAttr("ring_03_l.rotateZ", 50.02688615)
        cmds.setAttr("ring_02_l.rotateZ", 54.79433274)
        cmds.setAttr("pinky_03_l.rotateZ", 50.02688615)
        cmds.setAttr("pinky_02_l.rotateZ", 54.79433274)
        cmds.setAttr("middle_03_l.rotateZ", 55.02709033)
        cmds.setAttr("middle_02_l.rotateZ", 59.79453692)
        cmds.setAttr("index_03_l.rotateZ", 64.64121137)
        cmds.setAttr("index_02_l.rotateZ", 64.20477863)

        cmds.setAttr("thumb_03_r.rotateZ", 30.05735749)
        cmds.setAttr("thumb_02_r.rotateZ", 22.03618142)
        cmds.setAttr("thumb_02_r.rotateY", 5.675048056)
        cmds.setAttr("thumb_02_r.rotateX", 22.94177697)
        cmds.setAttr("ring_03_r.rotateZ", 50.02688615)
        cmds.setAttr("ring_02_r.rotateZ", 54.79433274)
        cmds.setAttr("pinky_03_r.rotateZ", 50.02688615)
        cmds.setAttr("pinky_02_r.rotateZ", 54.79433274)
        cmds.setAttr("middle_03_r.rotateZ", 55.02709033)
        cmds.setAttr("middle_02_r.rotateZ", 59.79453692)
        cmds.setAttr("index_03_r.rotateZ", 64.64121137)
        cmds.setAttr("index_02_r.rotateZ", 64.20477863)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

         # remove unneeded joints
        jointsToRemove = ["hip_eyelet_rear_l_03"]
        jointToTransferTo = "hip_eyelet_rear_l_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hip_eyelet_rear_r_03"]
        jointToTransferTo = "hip_eyelet_rear_r_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hip_eyelet_front_l_03"]
        jointToTransferTo = "hip_eyelet_front_l_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hip_eyelet_front_r_03"]
        jointToTransferTo = "hip_eyelet_front_r_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["back_sash_01", "back_sash_02", "back_sash_03"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw", "pipe", "padawan_l_01", "padawan_l_02", "padawan_r_01", "padawan_r_02", "ponytail_01", "ponytail_02", "ponytail_03"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulder_cloth_01_l", "shoulder_cloth_02_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulder_blade_in_r", "shoulder_blade_mid_r", "shoulder_blade_out_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r", "shoulder_pad_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_l", "bracelet_A_l", "bracelet_B_l", "bracelet_C_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r", "wrist_plate_r"]
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
        jointToTransferTo = "pinky_01_l"
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
        jointToTransferTo = "pinky_01_r"
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

        jointsToRemove = ["weapon_bladeA_03", "weapon_bladeA_04", "weapon_bladeA_05"]
        jointToTransferTo = "weapon_bladeA_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_bladeB_03", "weapon_bladeB_04", "weapon_bladeB_05"]
        jointToTransferTo = "weapon_bladeB_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_bladeC_03"]
        jointToTransferTo = "weapon_bladeC_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_bladeD_02"]
        jointToTransferTo = "weapon_bladeD_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_butt_02", "weapon_butt_03"]
        jointToTransferTo = "weapon_butt_01"
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
        export.exportLOD("ArcBlade_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

ArcBladeLOD2()

# ArcBlade LOD3 -------------------------------------------------------------------------------------------------------------
def ArcBladeLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["arm_spheres_geo_LOD3", "arms_geo_LOD3", "back_sash_geo_LOD3", "bracelets_geo_LOD3", "eye_tear_duct_geo_LOD3", "eye_wetness_geo_LOD3", "eyeballs_geo_LOD3", "eyelashes_geo_LOD3", "eyeshadow_geo_LOD3", "feet_geo_LOD3", "forearm_armor_geo_LOD3", "hair_ponytail_holder_geo_LOD3", "hair_sheets_geo_LOD3", "hair_tails_geo_LOD3", "head_geo_LOD3", "hip_flaps_geo_LOD3", "hips_armor_geo_LOD3", "knees_geo_LOD3", "loin_cloths_geo_LOD3", "pipe_geo_LOD3", "shoulder_armor_r_geo_LOD3", "shoulder_sash_l_geo_LOD3", "shoulder_underarmor_r_geo_LOD3", "teeth_lower_geo_LOD3", "teeth_upper_geo_LOD3", "thighs_geo_LOD3", "thin_flaps_geo_LOD3", "tongue_geo_LOD3", "torso_geo_LOD3", "upperarm_armor_geo_LOD3", "waist_band_geo_LOD3", "weapon_blade_01_geo_LOD3", "weapon_blade_02_geo_LOD3", "weapon_blade_03_geo_LOD3", "weapon_blade_04_geo_LOD3", "weapon_blade_05_geo_LOD3", "weapon_chain_geo_LOD3", "weapon_handle_geo_LOD1"]
    
        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 30.05735749)
        cmds.setAttr("thumb_02_l.rotateZ", 22.03618142)
        cmds.setAttr("thumb_02_l.rotateY", 5.675048056)
        cmds.setAttr("thumb_02_l.rotateX", 22.94177697)
        cmds.setAttr("thumb_01_l.rotateZ", 4.168007795)
        cmds.setAttr("thumb_01_l.rotateY", -1.675941069)
        cmds.setAttr("thumb_01_l.rotateX", 9.067155995)
        cmds.setAttr("ring_03_l.rotateZ", 50.02688615)
        cmds.setAttr("ring_02_l.rotateZ", 54.79433274)
        cmds.setAttr("ring_01_l.rotateZ", 54.76549897)
        cmds.setAttr("pinky_03_l.rotateZ", 50.02688615)
        cmds.setAttr("pinky_02_l.rotateZ", 54.79433274)
        cmds.setAttr("pinky_01_l.rotateZ", 53.17737673)
        cmds.setAttr("middle_03_l.rotateZ", 55.02709033)
        cmds.setAttr("middle_02_l.rotateZ", 59.79453692)
        cmds.setAttr("middle_01_l.rotateZ", 51.05024479)
        cmds.setAttr("index_03_l.rotateZ", 64.64121137)
        cmds.setAttr("index_02_l.rotateZ", 64.20477863)
        cmds.setAttr("index_01_l.rotateZ", 46.94060635)
        cmds.setAttr("index_01_l.rotateY", 3.692762633)
        cmds.setAttr("index_01_l.rotateX", -8.969261741)

        cmds.setAttr("thumb_03_r.rotateZ", 30.05735749)
        cmds.setAttr("thumb_02_r.rotateZ", 22.03618142)
        cmds.setAttr("thumb_02_r.rotateY", 5.675048056)
        cmds.setAttr("thumb_02_r.rotateX", 22.94177697)
        cmds.setAttr("thumb_01_r.rotateZ", 4.168007795)
        cmds.setAttr("thumb_01_r.rotateY", -1.675941069)
        cmds.setAttr("thumb_01_r.rotateX", 9.067155995)
        cmds.setAttr("ring_03_r.rotateZ", 50.02688615)
        cmds.setAttr("ring_02_r.rotateZ", 54.79433274)
        cmds.setAttr("ring_01_r.rotateZ", 54.76549897)
        cmds.setAttr("pinky_03_r.rotateZ", 50.02688615)
        cmds.setAttr("pinky_02_r.rotateZ", 54.79433274)
        cmds.setAttr("pinky_01_r.rotateZ", 53.17737673)
        cmds.setAttr("middle_03_r.rotateZ", 55.02709033)
        cmds.setAttr("middle_02_r.rotateZ", 59.79453692)
        cmds.setAttr("middle_01_r.rotateZ", 51.05024479)
        cmds.setAttr("index_03_r.rotateZ", 64.64121137)
        cmds.setAttr("index_02_r.rotateZ", 64.20477863)
        cmds.setAttr("index_01_r.rotateZ", 46.94060635)
        cmds.setAttr("index_01_r.rotateY", 3.692762633)
        cmds.setAttr("index_01_r.rotateX", -8.969261741)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["hip_eyelet_rear_l_01", "hip_eyelet_rear_l_02", "hip_eyelet_rear_l_03", "hip_eyelet_rear_r_01", "hip_eyelet_rear_r_02", "hip_eyelet_rear_r_03", "loin_cloth_bk_01", "loin_cloth_bk_02"]
        jointToTransferTo = "hip_back_flaps"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hip_eyelet_front_l_01", "hip_eyelet_front_l_02", "hip_eyelet_front_l_03"]
        jointToTransferTo = "leg_shieldPad_A_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hip_eyelet_front_r_01", "hip_eyelet_front_r_02", "hip_eyelet_front_r_03"]
        jointToTransferTo = "leg_shieldPad_A_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["loin_cloth_fr_01", "loin_cloth_fr_02"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["back_sash_01", "back_sash_02", "back_sash_03"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw", "pipe", "padawan_l_01", "padawan_l_02", "padawan_r_01", "padawan_r_02", "ponytail_01", "ponytail_02", "ponytail_03"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulder_cloth_01_l", "shoulder_cloth_02_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulder_blade_in_r", "shoulder_blade_mid_r", "shoulder_blade_out_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r", "shoulder_pad_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_l", "bracelet_A_l", "bracelet_B_l", "bracelet_C_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r", "wrist_plate_r"]
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

        jointsToRemove = ["weapon_bladeA_01", "weapon_bladeA_02", "weapon_bladeA_03", "weapon_bladeA_04", "weapon_bladeA_05", "weapon_bladeB_01", "weapon_bladeB_02", "weapon_bladeB_03", "weapon_bladeB_04", "weapon_bladeB_05", "weapon_bladeC_01", "weapon_bladeC_02", "weapon_bladeC_03", "weapon_bladeD_01", "weapon_bladeD_02", "weapon_bladeE_01", "weapon_butt_01", "weapon_butt_02", "weapon_butt_03"]
        jointToTransferTo = "weapon_l"
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
        export.exportLOD("ArcBlade_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

ArcBladeLOD3()

# ArcBlade LOD4 -------------------------------------------------------------------------------------------------------------
def ArcBladeLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["arm_spheres_geo_LOD4", "arms_geo_LOD4", "back_sash_geo_LOD4", "bracelets_geo_LOD4", "eye_tear_duct_geo_LOD4", "eye_wetness_geo_LOD4", "eyeballs_geo_LOD4", "eyelashes_geo_LOD4", "eyeshadow_geo_LOD4", "feet_geo_LOD4", "forearm_armor_geo_LOD4", "hair_ponytail_holder_geo_LOD4", "hair_sheets_geo_LOD4", "hair_tails_geo_LOD4", "head_geo_LOD4", "hip_flaps_geo_LOD4", "hips_armor_geo_LOD4", "knees_geo_LOD4", "loin_cloths_geo_LOD4", "pipe_geo_LOD4", "shoulder_armor_r_geo_LOD4", "shoulder_sash_l_geo_LOD4", "shoulder_underarmor_r_geo_LOD4", "teeth_lower_geo_LOD4", "teeth_upper_geo_LOD4", "thighs_geo_LOD4", "thin_flaps_geo_LOD4", "tongue_geo_LOD4", "torso_geo_LOD4", "upperarm_armor_geo_LOD4", "waist_band_geo_LOD4", "weapon_blade_01_geo_LOD4", "weapon_blade_02_geo_LOD4", "weapon_blade_03_geo_LOD4", "weapon_blade_04_geo_LOD4", "weapon_blade_05_geo_LOD4", "weapon_chain_geo_LOD4", "weapon_handle_geo_LOD1"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 30.05735749)
        cmds.setAttr("thumb_02_l.rotateZ", 22.03618142)
        cmds.setAttr("thumb_02_l.rotateY", 5.675048056)
        cmds.setAttr("thumb_02_l.rotateX", 22.94177697)
        cmds.setAttr("thumb_01_l.rotateZ", 4.168007795)
        cmds.setAttr("thumb_01_l.rotateY", -1.675941069)
        cmds.setAttr("thumb_01_l.rotateX", 9.067155995)
        cmds.setAttr("ring_03_l.rotateZ", 50.02688615)
        cmds.setAttr("ring_02_l.rotateZ", 54.79433274)
        cmds.setAttr("ring_01_l.rotateZ", 54.76549897)
        cmds.setAttr("pinky_03_l.rotateZ", 50.02688615)
        cmds.setAttr("pinky_02_l.rotateZ", 54.79433274)
        cmds.setAttr("pinky_01_l.rotateZ", 53.17737673)
        cmds.setAttr("middle_03_l.rotateZ", 55.02709033)
        cmds.setAttr("middle_02_l.rotateZ", 59.79453692)
        cmds.setAttr("middle_01_l.rotateZ", 51.05024479)
        cmds.setAttr("index_03_l.rotateZ", 64.64121137)
        cmds.setAttr("index_02_l.rotateZ", 64.20477863)
        cmds.setAttr("index_01_l.rotateZ", 46.94060635)
        cmds.setAttr("index_01_l.rotateY", 3.692762633)
        cmds.setAttr("index_01_l.rotateX", -8.969261741)

        cmds.setAttr("thumb_03_r.rotateZ", 30.05735749)
        cmds.setAttr("thumb_02_r.rotateZ", 22.03618142)
        cmds.setAttr("thumb_02_r.rotateY", 5.675048056)
        cmds.setAttr("thumb_02_r.rotateX", 22.94177697)
        cmds.setAttr("thumb_01_r.rotateZ", 4.168007795)
        cmds.setAttr("thumb_01_r.rotateY", -1.675941069)
        cmds.setAttr("thumb_01_r.rotateX", 9.067155995)
        cmds.setAttr("ring_03_r.rotateZ", 50.02688615)
        cmds.setAttr("ring_02_r.rotateZ", 54.79433274)
        cmds.setAttr("ring_01_r.rotateZ", 54.76549897)
        cmds.setAttr("pinky_03_r.rotateZ", 50.02688615)
        cmds.setAttr("pinky_02_r.rotateZ", 54.79433274)
        cmds.setAttr("pinky_01_r.rotateZ", 53.17737673)
        cmds.setAttr("middle_03_r.rotateZ", 55.02709033)
        cmds.setAttr("middle_02_r.rotateZ", 59.79453692)
        cmds.setAttr("middle_01_r.rotateZ", 51.05024479)
        cmds.setAttr("index_03_r.rotateZ", 64.64121137)
        cmds.setAttr("index_02_r.rotateZ", 64.20477863)
        cmds.setAttr("index_01_r.rotateZ", 46.94060635)
        cmds.setAttr("index_01_r.rotateY", 3.692762633)
        cmds.setAttr("index_01_r.rotateX", -8.969261741)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["hip_back_flaps", "hip_eyelet_rear_l_01", "hip_eyelet_rear_l_02", "hip_eyelet_rear_l_03", "hip_eyelet_rear_r_01", "hip_eyelet_rear_r_02", "hip_eyelet_rear_r_03", "loin_cloth_bk_01", "loin_cloth_bk_02", "leg_shieldPad_A_l", "hip_eyelet_front_l_01", "hip_eyelet_front_l_02", "hip_eyelet_front_l_03", "leg_shieldPad_B_l", "leg_shieldPad_A_r", "hip_eyelet_front_r_01", "hip_eyelet_front_r_02", "hip_eyelet_front_r_03", "leg_shieldPad_B_r", "loin_cloth_fr_01", "loin_cloth_fr_02"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["back_sash_01", "back_sash_02", "back_sash_03"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw", "pipe", "padawan_l_01", "padawan_l_02", "padawan_r_01", "padawan_r_02", "ponytail_01", "ponytail_02", "ponytail_03"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulder_cloth_01_l", "shoulder_cloth_02_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulder_blade_in_r", "shoulder_blade_mid_r", "shoulder_blade_out_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r", "shoulder_pad_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_l", "bracelet_A_l", "bracelet_B_l", "bracelet_C_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_r", "wrist_plate_r"]
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

        jointsToRemove = ["weapon_bladeA_01", "weapon_bladeA_02", "weapon_bladeA_03", "weapon_bladeA_04", "weapon_bladeA_05", "weapon_bladeB_01", "weapon_bladeB_02", "weapon_bladeB_03", "weapon_bladeB_04", "weapon_bladeB_05", "weapon_bladeC_01", "weapon_bladeC_02", "weapon_bladeC_03", "weapon_bladeD_01", "weapon_bladeD_02", "weapon_bladeE_01", "weapon_butt_01", "weapon_butt_02", "weapon_butt_03"]
        jointToTransferTo = "weapon_l"
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
        export.exportLOD("ArcBlade_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

ArcBladeLOD4()


