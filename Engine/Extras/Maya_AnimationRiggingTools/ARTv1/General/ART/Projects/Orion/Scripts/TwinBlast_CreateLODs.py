import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## TWINBLAST #########################################################################
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



# TwinBlast LOD1 -------------------------------------------------------------------------------------------------------------
def TwinBlastLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)

        meshes = ["boots_geo_LOD1", "eye_wetness_geo_LOD1", "eyeballs_geo_LOD1", "eyelash_geo_LOD1", "eyesbrow_geo_LOD1", "eyeshadow_geo_LOD1", "fingers_01_geo_LOD1", "fingers_02_geo_LOD1", "forearm_cores_geo_LOD1", "forearm_gears_geo_LOD1", "forearm_grenade_gears_geo_LOD1", "grenade_back_geo_LOD1", "grenade_flare_01_geo_LOD1", "grenade_flare_02_geo_LOD1", "grenade_flare_03_geo_LOD1", "grenade_flare_04_geo_LOD1", "grenade_flare_05_geo_LOD1", "grenade_flare_06_geo_LOD1", "grenade_front_geo_LOD1", "hair_geo_LOD1", "hands_geo_LOD1", "hip_pouches_geo_LOD1", "jacket_geo_LOD1", "lower_forearm_plate_A_geo_LOD1", "lower_forearm_plate_B_geo_LOD1", "lower_forearm_plate_C_geo_LOD1", "lower_forearm_plate_D_geo_LOD1", "lower_teeth_geo_LOD1", "necklace_geo_LOD1", "pants_geo_LOD1", "pistol_barrels_l_geo_LOD1", "pistol_barrels_r_geo_LOD1", "pistol_clip_l_geo_LOD1", "pistol_clip_r_geo_LOD1", "pistol_foresrock_bott_l_geo_LOD1", "pistol_forestock_bott_r_geo_LOD1", "pistol_forestock_top_l_geo_LOD1", "pistol_forestock_top_r_geo_LOD1", "pistol_grip_l_geo_LOD1", "pistol_grip_r_geo_LOD1", "pistol_mag_cylinder_l_geo_LOD1", "pistol_mag_cylinder_r_geo_LOD1", "pistol_mag_l_geo_LOD1", "pistol_mag_r_geo_LOD1", "pistol_stock_l_geo_LOD1", "pistol_stock_r_geo_LOD1", "pistol_trigger_l_geo_LOD1", "pistol_trigger_r_geo_LOD1", "tear_duct_geo_LOD1", "tongue_geo_LOD1", "torso_geo_LOD1", "upper_forearm_plate_A_geo_LOD1", "upper_forearm_plate_B_geo_LOD1", "upper_forearm_plate_C_geo_LOD1", "upper_forearm_plate_D_geo_LOD1", "upper_teeth_geo_LOD1", "wrist_cuffs_geo_LOD1"]

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["collar_back", "collar_in_l", "collar_in_r", "collar_l", "collar_out_flap_bott_l", "collar_out_flap_top_l", "collar_out_flap_bott_r", "collar_out_flap_top_r", "collar_r", "dogtag"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_forestock_bott_l", "weapon_forestock_top_l", "weapon_stock_l"]
        jointToTransferTo = "weapon_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_forestock_bott_r", "weapon_forestock_top_r", "weapon_stock_r"]
        jointToTransferTo = "weapon_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["grenade_fin_01", "grenade_fin_02", "grenade_fin_03", "grenade_fin_04", "grenade_fin_05", "grenade_fin_06", "grenade_front"]
        jointToTransferTo = "grenade"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_mid", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("TwinBlast_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

TwinBlastLOD1()

# TwinBlast LOD2 -------------------------------------------------------------------------------------------------------------
def TwinBlastLOD2():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["boots_geo_LOD2", "eye_wetness_geo_LOD2", "eyeballs_geo_LOD2", "eyelash_geo_LOD2", "eyesbrow_geo_LOD2", "eyeshadow_geo_LOD2", "fingers_01_geo_LOD2", "fingers_02_geo_LOD2", "forearm_cores_geo_LOD2", "forearm_gears_geo_LOD2", "forearm_grenade_gears_geo_LOD2", "grenade_back_geo_LOD2", "grenade_flare_01_geo_LOD2", "grenade_flare_02_geo_LOD2", "grenade_flare_03_geo_LOD2", "grenade_flare_04_geo_LOD2", "grenade_flare_05_geo_LOD2", "grenade_flare_06_geo_LOD2", "grenade_front_geo_LOD2", "hair_geo_LOD2", "hands_geo_LOD2", "hip_pouches_geo_LOD2", "jacket_geo_LOD2", "lower_forearm_plate_A_geo_LOD2", "lower_forearm_plate_B_geo_LOD2", "lower_forearm_plate_C_geo_LOD2", "lower_forearm_plate_D_geo_LOD2", "lower_teeth_geo_LOD2", "necklace_geo_LOD2", "pants_geo_LOD2", "pistol_barrels_l_geo_LOD2", "pistol_barrels_r_geo_LOD2", "pistol_clip_l_geo_LOD2", "pistol_clip_r_geo_LOD2", "pistol_foresrock_bott_l_geo_LOD2", "pistol_forestock_bott_r_geo_LOD2", "pistol_forestock_top_l_geo_LOD2", "pistol_forestock_top_r_geo_LOD2", "pistol_grip_l_geo_LOD2", "pistol_grip_r_geo_LOD2", "pistol_mag_cylinder_l_geo_LOD2", "pistol_mag_cylinder_r_geo_LOD2", "pistol_mag_l_geo_LOD2", "pistol_mag_r_geo_LOD2", "pistol_stock_l_geo_LOD2", "pistol_stock_r_geo_LOD2", "pistol_trigger_l_geo_LOD2", "pistol_trigger_r_geo_LOD2", "tear_duct_geo_LOD2", "tongue_geo_LOD2", "torso_geo_LOD2", "upper_forearm_plate_A_geo_LOD2", "upper_forearm_plate_B_geo_LOD2", "upper_forearm_plate_C_geo_LOD2", "upper_forearm_plate_D_geo_LOD2", "upper_teeth_geo_LOD2", "wrist_cuffs_geo_LOD2"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 49.02926105)
        cmds.setAttr("ring_03_l.rotateZ", 60.61060457)
        cmds.setAttr("pinky_03_l.rotateZ", 60.61060457)
        cmds.setAttr("middle_03_l.rotateZ", 60.61060457)
        cmds.setAttr("index_03_l.rotateZ", 60.61060457)

        cmds.setAttr("thumb_03_r.rotateZ", 49.02926105)
        cmds.setAttr("ring_03_r.rotateZ", 60.61060457)
        cmds.setAttr("pinky_03_r.rotateZ", 60.61060457)
        cmds.setAttr("middle_03_r.rotateZ", 60.61060457)
        cmds.setAttr("index_03_r.rotateZ", 60.61060457)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

         # remove unneeded joints
        jointsToRemove = ["collar_back", "collar_in_l", "collar_in_r", "collar_l", "collar_out_l", "collar_out_flap_bott_l", "collar_out_flap_top_l", "collar_out_r", "collar_out_flap_bott_r", "collar_out_flap_top_r", "collar_r", "dogtag"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_twist_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_02_r"]
        jointToTransferTo = "upperarm_twist_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["forearm_gears_l", "low_lowarm_plate_A_l", "low_lowarm_plate_B_l", "low_lowarm_plate_C_l", "low_lowarm_plate_D_l", "lowerarm_twist_01_l", "lowerarm_twist_02_l", "lowerarm_twist_03_l", "up_lowarm_plate_A_l", "up_lowarm_plate_B_l", "up_lowarm_plate_C_l", "up_lowarm_plate_D_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["forearm_gears_r", "low_lowarm_plate_A_r", "low_lowarm_plate_B_r", "low_lowarm_plate_C_r", "low_lowarm_plate_D_r", "lowerarm_twist_01_r", "lowerarm_twist_02_r", "lowerarm_twist_03_r", "up_lowarm_plate_A_r", "up_lowarm_plate_B_r", "up_lowarm_plate_C_r", "up_lowarm_plate_D_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_forestock_bott_l", "weapon_forestock_top_l", "weapon_stock_l"]
        jointToTransferTo = "weapon_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_forestock_bott_r", "weapon_forestock_top_r", "weapon_stock_r"]
        jointToTransferTo = "weapon_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["grenade_fin_01", "grenade_fin_02", "grenade_fin_03", "grenade_fin_04", "grenade_fin_05", "grenade_fin_06", "grenade_front"]
        jointToTransferTo = "grenade"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_mid", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_02_l"]
        jointToTransferTo = "thigh_twist_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_02_r"]
        jointToTransferTo = "thigh_twist_01_r"
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

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("TwinBlast_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

TwinBlastLOD2()

# TwinBlast LOD3 -------------------------------------------------------------------------------------------------------------
def TwinBlastLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["boots_geo_LOD3", "eye_wetness_geo_LOD3", "eyeballs_geo_LOD3", "eyelash_geo_LOD3", "eyesbrow_geo_LOD3", "eyeshadow_geo_LOD3", "fingers_01_geo_LOD3", "fingers_02_geo_LOD3", "forearm_cores_geo_LOD3", "forearm_gears_geo_LOD3", "forearm_grenade_gears_geo_LOD3", "grenade_back_geo_LOD3", "grenade_flare_01_geo_LOD3", "grenade_flare_02_geo_LOD3", "grenade_flare_03_geo_LOD3", "grenade_flare_04_geo_LOD3", "grenade_flare_05_geo_LOD3", "grenade_flare_06_geo_LOD3", "grenade_front_geo_LOD3", "hair_geo_LOD3", "hands_geo_LOD3", "hip_pouches_geo_LOD3", "jacket_geo_LOD3", "lower_forearm_plate_A_geo_LOD3", "lower_forearm_plate_B_geo_LOD3", "lower_forearm_plate_C_geo_LOD3", "lower_forearm_plate_D_geo_LOD3", "lower_teeth_geo_LOD3", "necklace_geo_LOD3", "pants_geo_LOD3", "pistol_barrels_l_geo_LOD3", "pistol_barrels_r_geo_LOD3", "pistol_clip_l_geo_LOD3", "pistol_clip_r_geo_LOD3", "pistol_foresrock_bott_l_geo_LOD3", "pistol_forestock_bott_r_geo_LOD3", "pistol_forestock_top_l_geo_LOD3", "pistol_forestock_top_r_geo_LOD3", "pistol_grip_l_geo_LOD3", "pistol_grip_r_geo_LOD3", "pistol_mag_cylinder_l_geo_LOD3", "pistol_mag_cylinder_r_geo_LOD3", "pistol_mag_l_geo_LOD3", "pistol_mag_r_geo_LOD3", "pistol_stock_l_geo_LOD3", "pistol_stock_r_geo_LOD3", "pistol_trigger_l_geo_LOD3", "pistol_trigger_r_geo_LOD3", "tear_duct_geo_LOD3", "tongue_geo_LOD3", "torso_geo_LOD3", "upper_forearm_plate_A_geo_LOD3", "upper_forearm_plate_B_geo_LOD3", "upper_forearm_plate_C_geo_LOD3", "upper_forearm_plate_D_geo_LOD3", "upper_teeth_geo_LOD3", "wrist_cuffs_geo_LOD3"]
    
        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 49.02926105)
        cmds.setAttr("thumb_02_l.rotateZ", 29.37554526)
        cmds.setAttr("thumb_01_l.rotateZ", -1.060679943)
        cmds.setAttr("thumb_01_l.rotateY", 16.78500021)
        cmds.setAttr("thumb_01_l.rotateX", 5.616322924)
        cmds.setAttr("ring_03_l.rotateZ", 60.61060457)
        cmds.setAttr("ring_02_l.rotateZ", 60.61060457)
        cmds.setAttr("ring_01_l.rotateZ", 60.61060457)
        cmds.setAttr("pinky_03_l.rotateZ", 60.61060457)
        cmds.setAttr("pinky_02_l.rotateZ", 60.61060457)
        cmds.setAttr("pinky_01_l.rotateZ", 60.61060457)
        cmds.setAttr("middle_03_l.rotateZ", 60.61060457)
        cmds.setAttr("middle_02_l.rotateZ", 60.61060457)
        cmds.setAttr("middle_01_l.rotateZ", 60.61060457)
        cmds.setAttr("index_03_l.rotateZ", 60.61060457)
        cmds.setAttr("index_02_l.rotateZ", 29.68388)
        cmds.setAttr("index_01_l.rotateZ", -15.38361)

        cmds.setAttr("thumb_03_r.rotateZ", 49.02926105)
        cmds.setAttr("thumb_02_r.rotateZ", 29.37554526)
        cmds.setAttr("thumb_01_r.rotateZ", -1.060679943)
        cmds.setAttr("thumb_01_r.rotateY", 16.78500021)
        cmds.setAttr("thumb_01_r.rotateX", 5.616322924)
        cmds.setAttr("ring_03_r.rotateZ", 60.61060457)
        cmds.setAttr("ring_02_r.rotateZ", 60.61060457)
        cmds.setAttr("ring_01_r.rotateZ", 60.61060457)
        cmds.setAttr("pinky_03_r.rotateZ", 60.61060457)
        cmds.setAttr("pinky_02_r.rotateZ", 60.61060457)
        cmds.setAttr("pinky_01_r.rotateZ", 60.61060457)
        cmds.setAttr("middle_03_r.rotateZ", 60.61060457)
        cmds.setAttr("middle_02_r.rotateZ", 60.61060457)
        cmds.setAttr("middle_01_r.rotateZ", 60.61060457)
        cmds.setAttr("index_03_r.rotateZ", 60.61060457)
        cmds.setAttr("index_02_r.rotateZ", 29.68388)
        cmds.setAttr("index_01_r.rotateZ", -15.38361)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["collar_back", "collar_in_l", "collar_in_r", "collar_l", "collar_out_l", "collar_out_flap_bott_l", "collar_out_flap_top_l", "collar_out_r", "collar_out_flap_bott_r", "collar_out_flap_top_r", "collar_r", "dogtag"]
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

        jointsToRemove = ["forearm_gears_l", "low_lowarm_plate_A_l", "low_lowarm_plate_B_l", "low_lowarm_plate_C_l", "low_lowarm_plate_D_l", "lowerarm_twist_01_l", "lowerarm_twist_02_l", "lowerarm_twist_03_l", "up_lowarm_plate_A_l", "up_lowarm_plate_B_l", "up_lowarm_plate_C_l", "up_lowarm_plate_D_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["forearm_gears_r", "low_lowarm_plate_A_r", "low_lowarm_plate_B_r", "low_lowarm_plate_C_r", "low_lowarm_plate_D_r", "lowerarm_twist_01_r", "lowerarm_twist_02_r", "lowerarm_twist_03_r", "up_lowarm_plate_A_r", "up_lowarm_plate_B_r", "up_lowarm_plate_C_r", "up_lowarm_plate_D_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_l", "index_02_l", "index_03_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "q_weapon_l", "ring_01_l", "ring_02_l", "ring_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "q_weapon_r", "ring_01_r", "ring_02_r", "ring_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_forestock_bott_l", "weapon_forestock_top_l", "weapon_stock_l"]
        jointToTransferTo = "weapon_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_forestock_bott_r", "weapon_forestock_top_r", "weapon_stock_r"]
        jointToTransferTo = "weapon_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["grenade_fin_01", "grenade_fin_02", "grenade_fin_03", "grenade_fin_04", "grenade_fin_05", "grenade_fin_06", "grenade_front"]
        jointToTransferTo = "grenade"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_mid", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw"]
        jointToTransferTo = "head"
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
        export.exportLOD("TwinBlast_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

TwinBlastLOD3()

# TwinBlast LOD4 -------------------------------------------------------------------------------------------------------------
def TwinBlastLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["boots_geo_LOD4", "eye_wetness_geo_LOD4", "eyeballs_geo_LOD4", "eyelash_geo_LOD4", "eyesbrow_geo_LOD4", "eyeshadow_geo_LOD4", "fingers_01_geo_LOD4", "fingers_02_geo_LOD4", "forearm_cores_geo_LOD4", "forearm_gears_geo_LOD4", "forearm_grenade_gears_geo_LOD4", "grenade_back_geo_LOD4", "grenade_flare_01_geo_LOD4", "grenade_flare_02_geo_LOD4", "grenade_flare_03_geo_LOD4", "grenade_flare_04_geo_LOD4", "grenade_flare_05_geo_LOD4", "grenade_flare_06_geo_LOD4", "grenade_front_geo_LOD4", "hair_geo_LOD4", "hands_geo_LOD4", "hip_pouches_geo_LOD4", "jacket_geo_LOD4", "lower_forearm_plate_A_geo_LOD4", "lower_forearm_plate_B_geo_LOD4", "lower_forearm_plate_C_geo_LOD4", "lower_forearm_plate_D_geo_LOD4", "lower_teeth_geo_LOD4", "necklace_geo_LOD4", "pants_geo_LOD4", "pistol_barrels_l_geo_LOD4", "pistol_barrels_r_geo_LOD4", "pistol_clip_l_geo_LOD4", "pistol_clip_r_geo_LOD4", "pistol_foresrock_bott_l_geo_LOD4", "pistol_forestock_bott_r_geo_LOD4", "pistol_forestock_top_l_geo_LOD4", "pistol_forestock_top_r_geo_LOD4", "pistol_grip_l_geo_LOD4", "pistol_grip_r_geo_LOD4", "pistol_mag_cylinder_l_geo_LOD4", "pistol_mag_cylinder_r_geo_LOD4", "pistol_mag_l_geo_LOD4", "pistol_mag_r_geo_LOD4", "pistol_stock_l_geo_LOD4", "pistol_stock_r_geo_LOD4", "pistol_trigger_l_geo_LOD4", "pistol_trigger_r_geo_LOD4", "tear_duct_geo_LOD4", "tongue_geo_LOD4", "torso_geo_LOD4", "upper_forearm_plate_A_geo_LOD4", "upper_forearm_plate_B_geo_LOD4", "upper_forearm_plate_C_geo_LOD4", "upper_forearm_plate_D_geo_LOD4", "upper_teeth_geo_LOD4", "wrist_cuffs_geo_LOD4"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 49.02926105)
        cmds.setAttr("thumb_02_l.rotateZ", 29.37554526)
        cmds.setAttr("thumb_01_l.rotateZ", -1.060679943)
        cmds.setAttr("thumb_01_l.rotateY", 16.78500021)
        cmds.setAttr("thumb_01_l.rotateX", 5.616322924)
        cmds.setAttr("ring_03_l.rotateZ", 60.61060457)
        cmds.setAttr("ring_02_l.rotateZ", 60.61060457)
        cmds.setAttr("ring_01_l.rotateZ", 60.61060457)
        cmds.setAttr("pinky_03_l.rotateZ", 60.61060457)
        cmds.setAttr("pinky_02_l.rotateZ", 60.61060457)
        cmds.setAttr("pinky_01_l.rotateZ", 60.61060457)
        cmds.setAttr("middle_03_l.rotateZ", 60.61060457)
        cmds.setAttr("middle_02_l.rotateZ", 60.61060457)
        cmds.setAttr("middle_01_l.rotateZ", 60.61060457)
        cmds.setAttr("index_03_l.rotateZ", 60.61060457)
        cmds.setAttr("index_02_l.rotateZ", 29.68388)
        cmds.setAttr("index_01_l.rotateZ", -15.38361)

        cmds.setAttr("thumb_03_r.rotateZ", 49.02926105)
        cmds.setAttr("thumb_02_r.rotateZ", 29.37554526)
        cmds.setAttr("thumb_01_r.rotateZ", -1.060679943)
        cmds.setAttr("thumb_01_r.rotateY", 16.78500021)
        cmds.setAttr("thumb_01_r.rotateX", 5.616322924)
        cmds.setAttr("ring_03_r.rotateZ", 60.61060457)
        cmds.setAttr("ring_02_r.rotateZ", 60.61060457)
        cmds.setAttr("ring_01_r.rotateZ", 60.61060457)
        cmds.setAttr("pinky_03_r.rotateZ", 60.61060457)
        cmds.setAttr("pinky_02_r.rotateZ", 60.61060457)
        cmds.setAttr("pinky_01_r.rotateZ", 60.61060457)
        cmds.setAttr("middle_03_r.rotateZ", 60.61060457)
        cmds.setAttr("middle_02_r.rotateZ", 60.61060457)
        cmds.setAttr("middle_01_r.rotateZ", 60.61060457)
        cmds.setAttr("index_03_r.rotateZ", 60.61060457)
        cmds.setAttr("index_02_r.rotateZ", 29.68388)
        cmds.setAttr("index_01_r.rotateZ", -15.38361)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["collar_back", "collar_in_l", "collar_in_r", "collar_l", "collar_out_l", "collar_out_flap_bott_l", "collar_out_flap_top_l", "collar_out_r", "collar_out_flap_bott_r", "collar_out_flap_top_r", "collar_r", "dogtag"]
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

        jointsToRemove = ["forearm_gears_l", "low_lowarm_plate_A_l", "low_lowarm_plate_B_l", "low_lowarm_plate_C_l", "low_lowarm_plate_D_l", "lowerarm_twist_01_l", "lowerarm_twist_02_l", "lowerarm_twist_03_l", "up_lowarm_plate_A_l", "up_lowarm_plate_B_l", "up_lowarm_plate_C_l", "up_lowarm_plate_D_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["forearm_gears_r", "low_lowarm_plate_A_r", "low_lowarm_plate_B_r", "low_lowarm_plate_C_r", "low_lowarm_plate_D_r", "lowerarm_twist_01_r", "lowerarm_twist_02_r", "lowerarm_twist_03_r", "up_lowarm_plate_A_r", "up_lowarm_plate_B_r", "up_lowarm_plate_C_r", "up_lowarm_plate_D_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_l", "index_02_l", "index_03_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "q_weapon_l", "ring_01_l", "ring_02_l", "ring_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "q_weapon_r", "ring_01_r", "ring_02_r", "ring_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_forestock_bott_l", "weapon_forestock_top_l", "weapon_stock_l"]
        jointToTransferTo = "weapon_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["weapon_forestock_bott_r", "weapon_forestock_top_r", "weapon_stock_r"]
        jointToTransferTo = "weapon_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["grenade", "grenade_fin_01", "grenade_fin_02", "grenade_fin_03", "grenade_fin_04", "grenade_fin_05", "grenade_fin_06", "grenade_front"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_mid", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw"]
        jointToTransferTo = "head"
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
        export.exportLOD("TwinBlast_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

TwinBlastLOD4()


