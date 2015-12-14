import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## GRUX #########################################################################
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



# Grux LOD1 -------------------------------------------------------------------------------------------------------------
def GruxLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["belt_geo_LOD1", "big_packs_geo_LOD1", "body_geo_LOD1", "eye_tear_duct_geo_LOD1", "eye_wetness_geo_LOD1", "eyeballs_geo_LOD1", "eyeshadow_geo_LOD1", "horn_ring_geo_LOD1", "hornwrap_01_geo_LOD1", "hornwrap_02_geo_LOD1", "loin_cloth_middle_geo_LOD1", "loin_cloth_sides_geo_LOD1", "small_packs_geo_LOD1", "teeth_geo_LOD1", "underwear_geo_LOD1", "weapon_left_geo_LOD1", "weapon_right_geo_LOD1", "wristwrap_geo_LOD1"]

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["tongue_01", "tongue_02", "tongue_03"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Grux_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

GruxLOD1()

# Grux LOD2 -------------------------------------------------------------------------------------------------------------
def GruxLOD2():
    if saveResult == "Yes":
        
        cmds.file(save=True)
        
        meshes = ["belt_geo_LOD2", "big_packs_geo_LOD2", "body_geo_LOD2", "eye_tear_duct_geo_LOD2", "eye_wetness_geo_LOD2", "eyeballs_geo_LOD2", "eyeshadow_geo_LOD2", "horn_ring_geo_LOD2", "hornwrap_01_geo_LOD2", "hornwrap_02_geo_LOD2", "loin_cloth_middle_geo_LOD2", "loin_cloth_sides_geo_LOD2", "small_packs_geo_LOD2", "teeth_geo_LOD2", "underwear_geo_LOD2", "weapon_left_geo_LOD2", "weapon_right_geo_LOD2", "wristwrap_geo_LOD2"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", -1.887)
        cmds.setAttr("thumb_03_l.rotateY", -1.789)
        cmds.setAttr("thumb_03_l.rotateX", -2.92)
        cmds.setAttr("thumb_02_l.rotateZ", 8.435338581)
        cmds.setAttr("middle_03_l.rotateZ", 56.68)
        cmds.setAttr("middle_02_l.rotateZ", 70.719)
        cmds.setAttr("index_03_l.rotateZ", 56.687)
        cmds.setAttr("index_02_l.rotateZ", 56.687)

        cmds.setAttr("thumb_03_r.rotateZ", -1.887)
        cmds.setAttr("thumb_03_r.rotateY", -1.789)
        cmds.setAttr("thumb_03_r.rotateX", -2.92)
        cmds.setAttr("thumb_02_r.rotateZ", 8.435338581)
        cmds.setAttr("middle_03_r.rotateZ", 56.68)
        cmds.setAttr("middle_02_r.rotateZ", 70.719)
        cmds.setAttr("index_03_r.rotateZ", 56.687)
        cmds.setAttr("index_02_r.rotateZ", 56.687)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

         # remove unneeded joints
        jointsToRemove = ["belt", "big_pack_l", "big_pack_r", "loin_cloth_bk_l_01", "loin_cloth_bk_l_02", "loin_cloth_bk_l_03", "loin_cloth_bk_r_01", "loin_cloth_bk_r_02", "loin_cloth_bk_r_03", "loin_cloth_fr_l_01", "loin_cloth_fr_l_02", "loin_cloth_fr_l_03", "loin_cloth_fr_r_01", "loin_cloth_fr_r_02", "loin_cloth_fr_r_03", "small_pack_l", "small_pack_r"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["muscle_gut"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["muscle_pec_l", "muscle_pec_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["scapula_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["scapula_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_03_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_03_r"]
        jointToTransferTo = "upperarm_r"
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

        jointsToRemove = ["thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "thumb_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ear_l", "horn_ring", "ear_r", "eyeball_l", "eyeball_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "tongue_01", "tongue_02", "tongue_03"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Grux_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

GruxLOD2()

# Grux LOD3 -------------------------------------------------------------------------------------------------------------
def GruxLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["belt_geo_LOD3", "big_packs_geo_LOD3", "body_geo_LOD3", "eye_tear_duct_geo_LOD3", "eye_wetness_geo_LOD3", "eyeballs_geo_LOD3", "eyeshadow_geo_LOD3", "horn_ring_geo_LOD3", "hornwrap_01_geo_LOD3", "hornwrap_02_geo_LOD3", "loin_cloth_middle_geo_LOD3", "loin_cloth_sides_geo_LOD3", "small_packs_geo_LOD3", "teeth_geo_LOD3", "underwear_geo_LOD3", "weapon_left_geo_LOD3", "weapon_right_geo_LOD3", "wristwrap_geo_LOD3"]
    
        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", -1.887)
        cmds.setAttr("thumb_03_l.rotateY", -1.789)
        cmds.setAttr("thumb_03_l.rotateX", -2.92)
        cmds.setAttr("thumb_02_l.rotateZ", 8.435338581)
        cmds.setAttr("thumb_01_l.rotateZ", 3.76)
        cmds.setAttr("thumb_01_l.rotateY", -4.45)
        cmds.setAttr("thumb_01_l.rotateX", 0.74)
        cmds.setAttr("middle_03_l.rotateZ", 56.68)
        cmds.setAttr("middle_02_l.rotateZ", 70.719)
        cmds.setAttr("middle_01_l.rotateZ", 56.687)
        cmds.setAttr("middle_metacarpal_l.rotateY", 2.497)
        cmds.setAttr("index_03_l.rotateZ", 56.687)
        cmds.setAttr("index_02_l.rotateZ", 56.687)
        cmds.setAttr("index_01_l.rotateZ", 56.687)
        cmds.setAttr("index_metacarpal_l.rotateZ", 1.0784)
        cmds.setAttr("index_metacarpal_l.rotateY", -1.0436)
        cmds.setAttr("index_metacarpal_l.rotateX", 1.5033)

        cmds.setAttr("thumb_03_r.rotateZ", -1.887)
        cmds.setAttr("thumb_03_r.rotateY", -1.789)
        cmds.setAttr("thumb_03_r.rotateX", -2.92)
        cmds.setAttr("thumb_02_r.rotateZ", 8.435338581)
        cmds.setAttr("thumb_01_r.rotateZ", 3.76)
        cmds.setAttr("thumb_01_r.rotateY", -4.45)
        cmds.setAttr("thumb_01_r.rotateX", 0.74)
        cmds.setAttr("middle_03_r.rotateZ", 56.68)
        cmds.setAttr("middle_02_r.rotateZ", 70.719)
        cmds.setAttr("middle_01_r.rotateZ", 56.687)
        cmds.setAttr("middle_metacarpal_r.rotateY", 2.497)
        cmds.setAttr("index_03_r.rotateZ", 56.687)
        cmds.setAttr("index_02_r.rotateZ", 56.687)
        cmds.setAttr("index_01_r.rotateZ", 56.687)
        cmds.setAttr("index_metacarpal_r.rotateZ", 1.0784)
        cmds.setAttr("index_metacarpal_r.rotateY", -1.0436)
        cmds.setAttr("index_metacarpal_r.rotateX", 1.5033)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["belt", "big_pack_l", "big_pack_r", "loin_cloth_bk_l_01", "loin_cloth_bk_l_02", "loin_cloth_bk_l_03", "loin_cloth_bk_r_01", "loin_cloth_bk_r_02", "loin_cloth_bk_r_03", "loin_cloth_fr_l_01", "loin_cloth_fr_l_02", "loin_cloth_fr_l_03", "loin_cloth_fr_r_01", "loin_cloth_fr_r_02", "loin_cloth_fr_r_03", "small_pack_l", "small_pack_r"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["muscle_gut"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["muscle_pec_l", "muscle_pec_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["scapula_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["scapula_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l", "upperarm_twist_03_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r", "upperarm_twist_03_r"]
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

        jointsToRemove = ["index_metacarpal_l", "index_01_l", "index_02_l", "index_03_l", "middle_metacarpal_l", "middle_01_l", "middle_02_l", "middle_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_metacarpal_r", "index_01_r", "index_02_r", "index_03_r", "middle_metacarpal_r", "middle_01_r", "middle_02_r", "middle_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ear_l", "horn_ring", "ear_r", "eyeball_l", "eyeball_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw", "tongue_01", "tongue_02", "tongue_03"]
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
        export.exportLOD("Grux_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

GruxLOD3()

# Grux LOD4 -------------------------------------------------------------------------------------------------------------
def GruxLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["belt_geo_LOD4", "big_packs_geo_LOD4", "body_geo_LOD4", "eye_tear_duct_geo_LOD4", "eye_wetness_geo_LOD4", "eyeballs_geo_LOD4", "eyeshadow_geo_LOD4", "horn_ring_geo_LOD4", "hornwrap_01_geo_LOD4", "hornwrap_02_geo_LOD4", "loin_cloth_middle_geo_LOD4", "loin_cloth_sides_geo_LOD4", "small_packs_geo_LOD4", "teeth_geo_LOD4", "underwear_geo_LOD4", "weapon_left_geo_LOD4", "weapon_right_geo_LOD4", "wristwrap_geo_LOD4"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", -1.887)
        cmds.setAttr("thumb_03_l.rotateY", -1.789)
        cmds.setAttr("thumb_03_l.rotateX", -2.92)
        cmds.setAttr("thumb_02_l.rotateZ", 8.435338581)
        cmds.setAttr("thumb_01_l.rotateZ", 3.76)
        cmds.setAttr("thumb_01_l.rotateY", -4.45)
        cmds.setAttr("thumb_01_l.rotateX", 0.74)
        cmds.setAttr("middle_03_l.rotateZ", 56.68)
        cmds.setAttr("middle_02_l.rotateZ", 70.719)
        cmds.setAttr("middle_01_l.rotateZ", 56.687)
        cmds.setAttr("middle_metacarpal_l.rotateY", 2.497)
        cmds.setAttr("index_03_l.rotateZ", 56.687)
        cmds.setAttr("index_02_l.rotateZ", 56.687)
        cmds.setAttr("index_01_l.rotateZ", 56.687)
        cmds.setAttr("index_metacarpal_l.rotateZ", 1.0784)
        cmds.setAttr("index_metacarpal_l.rotateY", -1.0436)
        cmds.setAttr("index_metacarpal_l.rotateX", 1.5033)

        cmds.setAttr("thumb_03_r.rotateZ", -1.887)
        cmds.setAttr("thumb_03_r.rotateY", -1.789)
        cmds.setAttr("thumb_03_r.rotateX", -2.92)
        cmds.setAttr("thumb_02_r.rotateZ", 8.435338581)
        cmds.setAttr("thumb_01_r.rotateZ", 3.76)
        cmds.setAttr("thumb_01_r.rotateY", -4.45)
        cmds.setAttr("thumb_01_r.rotateX", 0.74)
        cmds.setAttr("middle_03_r.rotateZ", 56.68)
        cmds.setAttr("middle_02_r.rotateZ", 70.719)
        cmds.setAttr("middle_01_r.rotateZ", 56.687)
        cmds.setAttr("middle_metacarpal_r.rotateY", 2.497)
        cmds.setAttr("index_03_r.rotateZ", 56.687)
        cmds.setAttr("index_02_r.rotateZ", 56.687)
        cmds.setAttr("index_01_r.rotateZ", 56.687)
        cmds.setAttr("index_metacarpal_r.rotateZ", 1.0784)
        cmds.setAttr("index_metacarpal_r.rotateY", -1.0436)
        cmds.setAttr("index_metacarpal_r.rotateX", 1.5033)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["belt", "big_pack_l", "big_pack_r", "loin_cloth_bk_l_01", "loin_cloth_bk_l_02", "loin_cloth_bk_l_03", "loin_cloth_bk_r_01", "loin_cloth_bk_r_02", "loin_cloth_bk_r_03", "loin_cloth_fr_l_01", "loin_cloth_fr_l_02", "loin_cloth_fr_l_03", "loin_cloth_fr_r_01", "loin_cloth_fr_r_02", "loin_cloth_fr_r_03", "small_pack_l", "small_pack_r"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["muscle_gut"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["muscle_pec_l", "muscle_pec_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["scapula_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["scapula_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l", "upperarm_twist_03_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r", "upperarm_twist_03_r"]
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

        jointsToRemove = ["index_metacarpal_l", "index_01_l", "index_02_l", "index_03_l", "middle_metacarpal_l", "middle_01_l", "middle_02_l", "middle_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_metacarpal_r", "index_01_r", "index_02_r", "index_03_r", "middle_metacarpal_r", "middle_01_r", "middle_02_r", "middle_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ear_l", "horn_ring", "ear_r", "eyeball_l", "eyeball_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw", "tongue_01", "tongue_02", "tongue_03"]
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
        export.exportLOD("Grux_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

GruxLOD4()


