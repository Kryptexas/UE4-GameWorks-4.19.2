import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## SPARROW #########################################################################
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



# Sparrow LOD1 -------------------------------------------------------------------------------------------------------------
def SparrowLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)

        meshes = ["arm_l_detail_geo_LOD1", "arms_geo_LOD1", "arrow_geo_LOD1", "belt_geo_LOD1", "bow_geo_LOD1", "brooch_chain_geo_LOD1", "brooch_geo_LOD1", "eye_tear_duct_geo_LOD1", "eye_wetness_geo_LOD1", "eyeballs_geo_LOD1", "eyebrows_geo_LOD1", "eyelashes_geo_LOD1", "eyeshadow_geo_LOD1", "feathers_geo_LOD1", "hair_core_geo_LOD1", "hair_ponytail_holder_geo_LOD1", "hair_sheets_geo_LOD1", "head_geo_LOD1", "legs_details_geo_LOD1", "legs_geo_LOD1", "skirt_geo_LOD1", "teeth_lower_geo_LOD1", "teeth_upper_geo_LOD1", "tongue_geo_LOD1", "torso_geo_LOD1"]

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["skirt_back_cloth_01", "skirt_back_cloth_02", "skirt_cloth_l_01", "skirt_cloth_l_02", "skirt_front_cloth_01", "skirt_front_cloth_02"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_mid", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["quiver"]
        jointToTransferTo = "thigh_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Sparrow_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

SparrowLOD1()

# Sparrow LOD2 -------------------------------------------------------------------------------------------------------------
def SparrowLOD2():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["arm_l_detail_geo_LOD2", "arms_geo_LOD2", "arrow_geo_LOD2", "belt_geo_LOD2", "bow_geo_LOD2", "brooch_chain_geo_LOD2", "brooch_geo_LOD2", "eye_tear_duct_geo_LOD2", "eye_wetness_geo_LOD2", "eyeballs_geo_LOD2", "eyebrows_geo_LOD2", "eyelashes_geo_LOD2", "eyeshadow_geo_LOD2", "feathers_geo_LOD2", "hair_core_geo_LOD2", "hair_ponytail_holder_geo_LOD2", "hair_sheets_geo_LOD2", "head_geo_LOD2", "legs_details_geo_LOD2", "legs_geo_LOD2", "skirt_geo_LOD2", "teeth_lower_geo_LOD2", "teeth_upper_geo_LOD2", "tongue_geo_LOD2", "torso_geo_LOD2"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_r.rotateZ", 17.55735447)
        cmds.setAttr("thumb_03_r.rotateY", 2.752475475)
        cmds.setAttr("thumb_03_r.rotateX", -8.614445938)
        cmds.setAttr("ring_03_r.rotateZ", 58.10260223)
        cmds.setAttr("pinky_03_r.rotateZ", 58.10260223)
        cmds.setAttr("middle_03_r.rotateZ", 60.1391109)
        cmds.setAttr("index_03_r.rotateZ", 60.1391109)

        cmds.setAttr("thumb_03_l.rotateZ", 49.02926105)
        cmds.setAttr("ring_03_l.rotateZ", 60.61060457)
        cmds.setAttr("pinky_03_l.rotateZ", 60.61060457)
        cmds.setAttr("middle_03_l.rotateZ", 60.61060457)
        cmds.setAttr("index_03_l.rotateZ", 60.61060457)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

         # remove unneeded joints
        jointsToRemove = ["front_strap_01", "front_strap_02", "front_strap_small", "skirt_back_cloth_01", "skirt_back_cloth_02", "skirt_cloth_l_01", "skirt_cloth_l_02", "skirt_front_cloth_01", "skirt_front_cloth_02"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["breast_chain", "brooch_back_l", "brooch_back_r", "brooch_front_l", "brooch_front_r", "chest_cloth_l_01", "chest_cloth_l_02", "chest_cloth_r_01", "chest_cloth_r_02"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_mid", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "feathers", "hair_curls_A_l_01", "hair_curls_A_r_01", "hair_curls_B_l_01", "hair_curls_B_r_01", "hair_top"]
        jointToTransferTo = "head"
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

        jointsToRemove = ["lowerarm_twist_02_l"]
        jointToTransferTo = "lowerarm_twist_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_02_r"]
        jointToTransferTo = "lowerarm_twist_01_r"
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

        jointsToRemove = ["quiver"]
        jointToTransferTo = "thigh_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Sparrow_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

SparrowLOD2()

# Sparrow LOD3 -------------------------------------------------------------------------------------------------------------
def SparrowLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["arm_l_detail_geo_LOD3", "arms_geo_LOD3", "arrow_geo_LOD3", "belt_geo_LOD3", "bow_geo_LOD3", "brooch_chain_geo_LOD3", "brooch_geo_LOD3", "eye_tear_duct_geo_LOD3", "eye_wetness_geo_LOD3", "eyeballs_geo_LOD3", "eyebrows_geo_LOD3", "eyelashes_geo_LOD3", "eyeshadow_geo_LOD3", "feathers_geo_LOD3", "hair_core_geo_LOD3", "hair_ponytail_holder_geo_LOD3", "hair_sheets_geo_LOD3", "head_geo_LOD3", "legs_details_geo_LOD3", "legs_geo_LOD3", "skirt_geo_LOD3", "teeth_lower_geo_LOD3", "teeth_upper_geo_LOD3", "tongue_geo_LOD3", "torso_geo_LOD3"]
    
        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_r.rotateZ", 17.55735447)
        cmds.setAttr("thumb_03_r.rotateY", 2.752475475)
        cmds.setAttr("thumb_03_r.rotateX", -8.614445938)
        cmds.setAttr("thumb_02_r.rotateZ", 15.15922943)
        cmds.setAttr("thumb_01_r.rotateZ", 18.03248389)
        cmds.setAttr("thumb_01_r.rotateY", 7.009164082)
        cmds.setAttr("thumb_01_r.rotateX", -0.3484935255)
        cmds.setAttr("ring_03_r.rotateZ", 58.10260223)
        cmds.setAttr("ring_02_r.rotateZ", 58.10260223)
        cmds.setAttr("ring_01_r.rotateZ", 58.10260223)
        cmds.setAttr("pinky_03_r.rotateZ", 58.10260223)
        cmds.setAttr("pinky_02_r.rotateZ", 64.22260217)
        cmds.setAttr("pinky_01_r.rotateZ", 71.7111965)
        cmds.setAttr("middle_03_r.rotateZ", 60.1391109)
        cmds.setAttr("middle_02_r.rotateZ", 50.6928414)
        cmds.setAttr("middle_01_r.rotateZ", 6.782023659)
        cmds.setAttr("index_03_r.rotateZ", 60.1391109)
        cmds.setAttr("index_02_r.rotateZ", 50.6928414)
        cmds.setAttr("index_01_r.rotateZ", 6.782023659)

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
        cmds.setAttr("index_02_l.rotateZ", 60.61060457)
        cmds.setAttr("index_01_l.rotateZ", 60.61060457)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["front_strap_01", "front_strap_02", "front_strap_small", "skirt_back_cloth_01", "skirt_back_cloth_02", "skirt_cloth_l_01", "skirt_cloth_l_02", "skirt_front_cloth_01", "skirt_front_cloth_02"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["back_tail_cloth_01", "back_tail_cloth_02", "breast_chain", "brooch_back_l", "brooch_back_r", "brooch_front_l", "brooch_front_r", "chest_cloth_l_01", "chest_cloth_l_02", "chest_cloth_r_01", "chest_cloth_r_02"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_mid", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "feathers", "hair_curls_A_l_01", "hair_curls_A_r_01", "hair_curls_B_l_01", "hair_curls_B_r_01", "hair_top", "jaw", "ponytail_01", "ponytail_02", "ponytail_03", "ponytail_04"]
        jointToTransferTo = "head"
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

        jointsToRemove = ["lowerarm_twist_01_l", "lowerarm_twist_02_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_02_r", "lowerarm_twist_01_r"]
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

        jointsToRemove = ["bow_shaft_bott_02", "bow_shaft_bott_03", "bow_string_bott"]
        jointToTransferTo = "bow_shaft_bott_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["bow_shaft_top_02", "bow_shaft_top_03", "bow_string_top"]
        jointToTransferTo = "bow_shaft_top_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_l", "thigh_twist_02_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_r", "thigh_twist_02_r", "quiver"]
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
        export.exportLOD("Sparrow_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

SparrowLOD3()

# Sparrow LOD4 -------------------------------------------------------------------------------------------------------------
def SparrowLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["arm_l_detail_geo_LOD4", "arms_geo_LOD4", "arrow_geo_LOD4", "belt_geo_LOD4", "bow_geo_LOD4", "brooch_chain_geo_LOD4", "brooch_geo_LOD4", "eye_tear_duct_geo_LOD4", "eye_wetness_geo_LOD4", "eyeballs_geo_LOD4", "eyebrows_geo_LOD4", "eyelashes_geo_LOD4", "eyeshadow_geo_LOD4", "feathers_geo_LOD4", "hair_core_geo_LOD4", "hair_ponytail_holder_geo_LOD4", "hair_sheets_geo_LOD4", "head_geo_LOD4", "legs_details_geo_LOD4", "legs_geo_LOD4", "skirt_geo_LOD4", "teeth_lower_geo_LOD4", "teeth_upper_geo_LOD4", "tongue_geo_LOD4", "torso_geo_LOD4"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_r.rotateZ", 17.55735447)
        cmds.setAttr("thumb_03_r.rotateY", 2.752475475)
        cmds.setAttr("thumb_03_r.rotateX", -8.614445938)
        cmds.setAttr("thumb_02_r.rotateZ", 15.15922943)
        cmds.setAttr("thumb_01_r.rotateZ", 18.03248389)
        cmds.setAttr("thumb_01_r.rotateY", 7.009164082)
        cmds.setAttr("thumb_01_r.rotateX", -0.3484935255)
        cmds.setAttr("ring_03_r.rotateZ", 58.10260223)
        cmds.setAttr("ring_02_r.rotateZ", 58.10260223)
        cmds.setAttr("ring_01_r.rotateZ", 58.10260223)
        cmds.setAttr("pinky_03_r.rotateZ", 58.10260223)
        cmds.setAttr("pinky_02_r.rotateZ", 64.22260217)
        cmds.setAttr("pinky_01_r.rotateZ", 71.7111965)
        cmds.setAttr("middle_03_r.rotateZ", 60.1391109)
        cmds.setAttr("middle_02_r.rotateZ", 50.6928414)
        cmds.setAttr("middle_01_r.rotateZ", 6.782023659)
        cmds.setAttr("index_03_r.rotateZ", 60.1391109)
        cmds.setAttr("index_02_r.rotateZ", 50.6928414)
        cmds.setAttr("index_01_r.rotateZ", 6.782023659)

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
        cmds.setAttr("index_02_l.rotateZ", 60.61060457)
        cmds.setAttr("index_01_l.rotateZ", 60.61060457)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["front_strap_01", "front_strap_02", "front_strap_small", "skirt_back_cloth_01", "skirt_back_cloth_02", "skirt_cloth_l_01", "skirt_cloth_l_02", "skirt_front_cloth_01", "skirt_front_cloth_02"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["back_tail_cloth_01", "back_tail_cloth_02", "breast_chain", "brooch_back_l", "brooch_back_r", "brooch_front_l", "brooch_front_r", "chest_cloth_l_01", "chest_cloth_l_02", "chest_cloth_r_01", "chest_cloth_r_02"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_mid", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "feathers", "hair_curls_A_l_01", "hair_curls_A_r_01", "hair_curls_B_l_01", "hair_curls_B_r_01", "hair_top", "jaw", "ponytail_01", "ponytail_02", "ponytail_03", "ponytail_04"]
        jointToTransferTo = "head"
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

        jointsToRemove = ["lowerarm_twist_01_l", "lowerarm_twist_02_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_02_r", "lowerarm_twist_01_r"]
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

        jointsToRemove = ["bow_shaft_bott_01", "bow_shaft_bott_02", "bow_shaft_bott_03", "bow_string_bott", "bow_shaft_top_01", "bow_shaft_top_02", "bow_shaft_top_03", "bow_string_top"]
        jointToTransferTo = "bow_base"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_l", "thigh_twist_02_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_r", "thigh_twist_02_r", "quiver"]
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
        export.exportLOD("Sparrow_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

SparrowLOD4()


