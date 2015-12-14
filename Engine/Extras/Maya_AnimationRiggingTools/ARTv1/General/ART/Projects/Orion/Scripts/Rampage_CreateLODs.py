import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## RAMPAGE #########################################################################
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



# Rampage LOD1 -------------------------------------------------------------------------------------------------------------
def RampageLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["body_geo_LOD1", "chain_link_geo_01_LOD1", "chain_link_geo_02_LOD1", "chain_link_geo_03_LOD1", "chain_link_geo_04_LOD1", "chain_link_geo_05_LOD1", "chain_link_geo_06_LOD1", "chain_link_geo_07_LOD1", "cloth_geo_LOD1", "eyeball_l_geo_LOD1", "eyeball_r_geo_LOD1", "groin_armor_geo_LOD1", "legs_geo_LOD1", "lower_teeth_geo_LOD1", "shackles_geo_LOD1", "shin_armor_geo_LOD1", "thigh_armor_geo_LOD1", "upper_teeth_geo_LOD1"]

        jointsToDelete = []

        # remove unneeded joints

        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Rampage_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

RampageLOD1()

# Rampage LOD2 -------------------------------------------------------------------------------------------------------------
def RampageLOD2():
    if saveResult == "Yes":
        
        cmds.file(save=True)
        
        meshes = ["body_geo_LOD2", "chain_link_geo_01_LOD2", "chain_link_geo_02_LOD2", "chain_link_geo_03_LOD2", "chain_link_geo_04_LOD2", "chain_link_geo_05_LOD2", "chain_link_geo_06_LOD2", "chain_link_geo_07_LOD2", "cloth_geo_LOD2", "eyeball_l_geo_LOD2", "eyeball_r_geo_LOD2", "groin_armor_geo_LOD2", "legs_geo_LOD2", "lower_teeth_geo_LOD2", "shackles_geo_LOD2", "shin_armor_geo_LOD2", "thigh_armor_geo_LOD2", "upper_teeth_geo_LOD2"]

        jointsToDelete = []

         # remove unneeded joints
        jointsToRemove = ["lat_muscle_l", "lat_muscle_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_a_l_02"]
        jointToTransferTo = "fin_scap_a_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_a_r_02"]
        jointToTransferTo = "fin_scap_a_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_b_l_02"]
        jointToTransferTo = "fin_scap_b_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_b_r_02"]
        jointToTransferTo = "fin_scap_b_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_c_l_02"]
        jointToTransferTo = "fin_scap_c_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_c_r_02"]
        jointToTransferTo = "fin_scap_c_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_d_l", "fin_scap_d_r", "pec_muscle_l", "pec_muscle_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_spine_a_02"]
        jointToTransferTo = "fin_spine_a_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_spine_b_02"]
        jointToTransferTo = "fin_spine_b_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_spine_c_02"]
        jointToTransferTo = "fin_spine_c_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_spine_d_02"]
        jointToTransferTo = "fin_spine_d_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l", "bicep_muscle_l", "tricep_muscle_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r", "bicep_muscle_r", "tricep_muscle_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_c_l_02"]
        jointToTransferTo = "fin_arm_c_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_d_l_02"]
        jointToTransferTo = "fin_arm_d_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_b_l_02"]
        jointToTransferTo = "fin_arm_b_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chain_06", "chain_07"]
        jointToTransferTo = "chain_05"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_a_l"]
        jointToTransferTo = "lowerarm_twist_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_c_r_02"]
        jointToTransferTo = "fin_arm_c_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_d_r_02"]
        jointToTransferTo = "fin_arm_d_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_b_r_02"]
        jointToTransferTo = "fin_arm_b_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_a_r"]
        jointToTransferTo = "lowerarm_twist_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_mid", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_k_l_02"]
        jointToTransferTo = "chin_tent_k_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_k_r_02"]
        jointToTransferTo = "chin_tent_k_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_a_l_02"]
        jointToTransferTo = "chin_tent_a_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_c_l_02"]
        jointToTransferTo = "chin_tent_c_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_d_l_02"]
        jointToTransferTo = "chin_tent_d_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_e_l_02"]
        jointToTransferTo = "chin_tent_e_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_f_l_02"]
        jointToTransferTo = "chin_tent_f_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_g_l_02"]
        jointToTransferTo = "chin_tent_g_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_h_l_02"]
        jointToTransferTo = "chin_tent_h_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_i_l_02"]
        jointToTransferTo = "chin_tent_i_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_a_l_02"]
        jointToTransferTo = "fin_head_a_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_b_l_02"]
        jointToTransferTo = "fin_head_b_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_c_l_02"]
        jointToTransferTo = "fin_head_c_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_a_r_02"]
        jointToTransferTo = "fin_head_a_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_b_r_02"]
        jointToTransferTo = "fin_head_b_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_c_r_02"]
        jointToTransferTo = "fin_head_c_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_mid_02"]
        jointToTransferTo = "fin_head_mid_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_index_02_l", "toe_index_03_l"]
        jointToTransferTo = "toe_index_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_mid_02_l", "toe_mid_03_l"]
        jointToTransferTo = "toe_mid_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_pinky_02_l", "toe_pinky_03_l"]
        jointToTransferTo = "toe_pinky_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_index_02_r", "toe_index_03_r"]
        jointToTransferTo = "toe_index_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_mid_02_r", "toe_mid_03_r"]
        jointToTransferTo = "toe_mid_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_pinky_02_r", "toe_pinky_03_r"]
        jointToTransferTo = "toe_pinky_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Rampage_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

RampageLOD2()

# Rampage LOD3 -------------------------------------------------------------------------------------------------------------
def RampageLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["body_geo_LOD3", "chain_link_geo_01_LOD3", "chain_link_geo_02_LOD3", "chain_link_geo_03_LOD3", "chain_link_geo_04_LOD3", "chain_link_geo_05_LOD3", "chain_link_geo_06_LOD3", "chain_link_geo_07_LOD3", "cloth_geo_LOD3", "eyeball_l_geo_LOD3", "eyeball_r_geo_LOD3", "groin_armor_geo_LOD3", "legs_geo_LOD3", "lower_teeth_geo_LOD3", "shackles_geo_LOD3", "shin_armor_geo_LOD3", "thigh_armor_geo_LOD3", "upper_teeth_geo_LOD3"]
    
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["lat_muscle_l", "lat_muscle_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_a_l_02"]
        jointToTransferTo = "fin_scap_a_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_a_r_02"]
        jointToTransferTo = "fin_scap_a_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_b_l_02"]
        jointToTransferTo = "fin_scap_b_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_b_r_02"]
        jointToTransferTo = "fin_scap_b_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_c_l_02"]
        jointToTransferTo = "fin_scap_c_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_c_r_02"]
        jointToTransferTo = "fin_scap_c_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_d_l", "fin_scap_d_r", "pec_muscle_l", "pec_muscle_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_spine_a_02"]
        jointToTransferTo = "fin_spine_a_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_spine_b_02"]
        jointToTransferTo = "fin_spine_b_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_spine_c_02"]
        jointToTransferTo = "fin_spine_c_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_spine_d_02"]
        jointToTransferTo = "fin_spine_d_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l", "bicep_muscle_l", "tricep_muscle_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r", "bicep_muscle_r", "tricep_muscle_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_c_l_02"]
        jointToTransferTo = "fin_arm_c_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_d_l_02"]
        jointToTransferTo = "fin_arm_d_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_b_l_02"]
        jointToTransferTo = "fin_arm_b_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chain_06", "chain_07"]
        jointToTransferTo = "chain_05"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_a_l"]
        jointToTransferTo = "lowerarm_twist_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_c_r_02"]
        jointToTransferTo = "fin_arm_c_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_d_r_02"]
        jointToTransferTo = "fin_arm_d_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_b_r_02"]
        jointToTransferTo = "fin_arm_b_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_a_r"]
        jointToTransferTo = "lowerarm_twist_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_k_l_01", "chin_tent_k_l_02", "chin_tent_k_r_01", "chin_tent_k_r_02", "eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_mid", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw", "chin_tent_a_l_01", "chin_tent_a_l_02", "chin_tent_a_r_01", "chin_tent_a_r_02", "chin_tent_c_l_01", "chin_tent_c_l_02", "chin_tent_c_r_01", "chin_tent_c_r_02", "chin_tent_d_l_01", "chin_tent_d_l_02", "chin_tent_d_r_01", "chin_tent_d_r_02", "chin_tent_e_l_01", "chin_tent_e_l_02", "chin_tent_e_r_01", "chin_tent_e_r_02", "chin_tent_f_l_01", "chin_tent_f_l_02", "chin_tent_f_r_01", "chin_tent_f_r_02", "chin_tent_g_l_01", "chin_tent_g_l_02", "chin_tent_g_r_01", "chin_tent_g_r_02", "chin_tent_h_l_01", "chin_tent_h_l_02", "chin_tent_h_r_01", "chin_tent_h_r_02", "chin_tent_i_l_01", "chin_tent_i_l_02", "chin_tent_i_r_01", "chin_tent_i_r_02"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_a_l_02"]
        jointToTransferTo = "fin_head_a_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_b_l_02"]
        jointToTransferTo = "fin_head_b_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_c_l_02"]
        jointToTransferTo = "fin_head_c_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_a_r_02"]
        jointToTransferTo = "fin_head_a_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_b_r_02"]
        jointToTransferTo = "fin_head_b_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_c_r_02"]
        jointToTransferTo = "fin_head_c_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_head_mid_02"]
        jointToTransferTo = "fin_head_mid_01"
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

        jointsToRemove = ["toe_index_01_l", "toe_index_02_l", "toe_index_03_l", "toe_mid_01_l", "toe_mid_02_l", "toe_mid_03_l", "toe_pinky_01_l", "toe_pinky_02_l", "toe_pinky_03_l"]
        jointToTransferTo = "ball_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_index_01_r", "toe_index_02_r", "toe_index_03_r", "toe_mid_01_r", "toe_mid_02_r", "toe_mid_03_r", "toe_pinky_01_r", "toe_pinky_02_r", "toe_pinky_03_r"]
        jointToTransferTo = "ball_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)


        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Rampage_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

RampageLOD3()

# Rampage LOD4 -------------------------------------------------------------------------------------------------------------
def RampageLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["body_geo_LOD4", "chain_link_geo_01_LOD4", "chain_link_geo_02_LOD4", "chain_link_geo_03_LOD4", "chain_link_geo_04_LOD4", "chain_link_geo_05_LOD4", "chain_link_geo_06_LOD4", "chain_link_geo_07_LOD4", "cloth_geo_LOD4", "eyeball_l_geo_LOD4", "eyeball_r_geo_LOD4", "groin_armor_geo_LOD4", "legs_geo_LOD4", "lower_teeth_geo_LOD4", "shackles_geo_LOD4", "shin_armor_geo_LOD4", "thigh_armor_geo_LOD4", "upper_teeth_geo_LOD4"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_02_l.rotateZ", 28.70053097)
        cmds.setAttr("thumb_01_l.rotateZ", 6.473285809)
        cmds.setAttr("thumb_01_l.rotateY", 2.036665823)
        cmds.setAttr("thumb_01_l.rotateX", -34.15792635)
        cmds.setAttr("pinky_03_l.rotateZ", 64.09065997)
        cmds.setAttr("pinky_02_l.rotateZ", 45.60472889)
        cmds.setAttr("pinky_01_l.rotateZ", -1.391492673e-015)
        cmds.setAttr("pinky_01_l.rotateY", -1.630034274e-014)
        cmds.setAttr("pinky_01_l.rotateX", 1.192708006e-014)
        cmds.setAttr("index_03_l.rotateZ", 64.09065997)
        cmds.setAttr("index_02_l.rotateZ", 45.60472889)
        cmds.setAttr("index_01_l.rotateZ", -8.945310042e-015)
        cmds.setAttr("index_01_l.rotateY", -2.981770014e-014)
        cmds.setAttr("index_01_l.rotateX", -2.266145211e-014)

        cmds.setAttr("thumb_02_r.rotateZ", 28.70053097)
        cmds.setAttr("thumb_01_r.rotateZ", 6.473285809)
        cmds.setAttr("thumb_01_r.rotateY", 2.036665823)
        cmds.setAttr("thumb_01_r.rotateX", -34.15792635)
        cmds.setAttr("pinky_03_r.rotateZ", 64.09065997)
        cmds.setAttr("pinky_02_r.rotateZ", 45.60472889)
        cmds.setAttr("pinky_01_r.rotateZ", -1.391492673e-015)
        cmds.setAttr("pinky_01_r.rotateY", -1.630034274e-014)
        cmds.setAttr("pinky_01_r.rotateX", 1.192708006e-014)
        cmds.setAttr("index_03_r.rotateZ", 64.09065997)
        cmds.setAttr("index_02_r.rotateZ", 45.60472889)
        cmds.setAttr("index_01_r.rotateZ", -8.945310042e-015)
        cmds.setAttr("index_01_r.rotateY", -2.981770014e-014)
        cmds.setAttr("index_01_r.rotateX", -2.266145211e-014)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["lat_muscle_l", "lat_muscle_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_scap_a_l_01", "fin_scap_a_l_02", "fin_scap_a_r_01", "fin_scap_a_r_02", "fin_scap_b_l_01", "fin_scap_b_l_02", "fin_scap_b_r_01", "fin_scap_b_r_02", "fin_scap_c_l_01", "fin_scap_c_l_02", "fin_scap_c_r_01", "fin_scap_c_r_02", "fin_scap_d_l", "fin_scap_d_r", "fin_spine_a_01", "fin_spine_a_02", "fin_spine_b_01", "fin_spine_b_02", "fin_spine_c_01", "fin_spine_c_02", "fin_spine_d_01", "fin_spine_d_02", "pec_muscle_l", "pec_muscle_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l", "bicep_muscle_l", "tricep_muscle_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "upperarm_twist_02_r", "bicep_muscle_r", "tricep_muscle_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_c_l_01", "fin_arm_c_l_02", "fin_arm_d_l_01", "fin_arm_d_l_02", "lowerarm_twist_01_l", "chain_01", "chain_02", "chain_03", "chain_04", "chain_05", "chain_06", "chain_07", "fin_arm_a_l", "fin_arm_b_l_01", "fin_arm_b_l_02"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["fin_arm_c_r_01", "fin_arm_c_r_02", "fin_arm_d_r_01", "fin_arm_d_r_02", "lowerarm_twist_01_r", "fin_arm_a_r", "fin_arm_b_r_01", "fin_arm_b_r_02"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_l", "index_02_l", "index_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "thumb_01_l", "thumb_02_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_r", "index_02_r", "index_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "thumb_01_r", "thumb_02_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["chin_tent_k_l_01", "chin_tent_k_l_02", "chin_tent_k_r_01", "chin_tent_k_r_02", "eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_mid", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "fin_head_a_l_01", "fin_head_a_l_02", "fin_head_a_r_01", "fin_head_a_r_02", "fin_head_b_l_01", "fin_head_b_l_02", "fin_head_b_r_01", "fin_head_b_r_02", "fin_head_c_l_01", "fin_head_c_l_02", "fin_head_c_r_01", "fin_head_c_r_02", "fin_head_mid_01", "fin_head_mid_02", "jaw", "chin_tent_a_l_01", "chin_tent_a_l_02", "chin_tent_a_r_01", "chin_tent_a_r_02", "chin_tent_c_l_01", "chin_tent_c_l_02", "chin_tent_c_r_01", "chin_tent_c_r_02", "chin_tent_d_l_01", "chin_tent_d_l_02", "chin_tent_d_r_01", "chin_tent_d_r_02", "chin_tent_e_l_01", "chin_tent_e_l_02", "chin_tent_e_r_01", "chin_tent_e_r_02", "chin_tent_f_l_01", "chin_tent_f_l_02", "chin_tent_f_r_01", "chin_tent_f_r_02", "chin_tent_g_l_01", "chin_tent_g_l_02", "chin_tent_g_r_01", "chin_tent_g_r_02", "chin_tent_h_l_01", "chin_tent_h_l_02", "chin_tent_h_r_01", "chin_tent_h_r_02", "chin_tent_i_l_01", "chin_tent_i_l_02", "chin_tent_i_r_01", "chin_tent_i_r_02"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_l", "thigh_twist_02_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_02_r", "thigh_twist_01_r"]
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

        jointsToRemove = ["toe_index_01_l", "toe_index_02_l", "toe_index_03_l", "toe_mid_01_l", "toe_mid_02_l", "toe_mid_03_l", "toe_pinky_01_l", "toe_pinky_02_l", "toe_pinky_03_l"]
        jointToTransferTo = "ball_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["toe_index_01_r", "toe_index_02_r", "toe_index_03_r", "toe_mid_01_r", "toe_mid_02_r", "toe_mid_03_r", "toe_pinky_01_r", "toe_pinky_02_r", "toe_pinky_03_r"]
        jointToTransferTo = "ball_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Rampage_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

RampageLOD4()


