import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## KUROHANE #########################################################################
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



# Kurohane LOD1 -------------------------------------------------------------------------------------------------------------
def KurohaneLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)

        meshes = ["geo_dagger_mechanisms_LOD1", "geo_eyes_LOD1", "geo_helmet_LOD1", "geo_l_wrist_LOD1", "geo_legs_LOD1", "geo_lower_arm_casings_LOD1", "geo_lower_arms_LOD1", "geo_r_wrist_LOD1", "geo_shoulder_plates_LOD1", "geo_sword_l_LOD1", "geo_sword_r_LOD1", "geo_tentacles_LOD1", "geo_thruster_a_LOD1", "geo_thruster_b_LOD1", "geo_thruster_backplate_LOD1", "geo_thruster_c_LOD1", "geo_thruster_home_LOD1", "geo_thruster_vents_LOD1", "geo_thruster_wheel_LOD1", "geo_torso_LOD1", "geo_upper_arms_LOD1"]

        jointsToDelete = []

        # remove unneeded joints
        # NONE
        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Kurohane_LOD1.fbx", "Kurohane_Geo_LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

KurohaneLOD1()

# Kurohane LOD2 -------------------------------------------------------------------------------------------------------------
def KurohaneLOD2():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["geo_dagger_mechanisms_LOD2", "geo_eyes_LOD2", "geo_helmet_LOD2", "geo_l_wrist_LOD2", "geo_legs_LOD2", "geo_lower_arm_casings_LOD2", "geo_lower_arms_LOD2", "geo_r_wrist_LOD2", "geo_shoulder_plates_LOD2", "geo_sword_l_LOD2", "geo_sword_r_LOD2", "geo_tentacles_LOD2", "geo_thruster_a_LOD2", "geo_thruster_b_LOD2", "geo_thruster_c_LOD2", "geo_thruster_home_LOD2", "geo_thruster_vents_LOD2", "geo_torso_LOD2", "geo_upper_arms_LOD2"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 0)
        cmds.setAttr("thumb_03_l.rotateY", -25.5635)
        cmds.setAttr("thumb_03_l.rotateX", 0)
        cmds.setAttr("pinky_03_l.rotateZ", 50.45)
        cmds.setAttr("pinky_03_l.rotateY", 0)
        cmds.setAttr("pinky_03_l.rotateX", 0)
        cmds.setAttr("middle_03_l.rotateZ", 50.4503)
        cmds.setAttr("middle_03_l.rotateY", 0)
        cmds.setAttr("middle_03_l.rotateX", 0)
        cmds.setAttr("index_03_l.rotateZ", 50.4503)
        cmds.setAttr("index_03_l.rotateY", 0)
        cmds.setAttr("index_03_l.rotateX", 0)

        cmds.setAttr("thumb_03_r.rotateZ", 0)
        cmds.setAttr("thumb_03_r.rotateY", -25.5635)
        cmds.setAttr("thumb_03_r.rotateX", 0)
        cmds.setAttr("pinky_03_r.rotateZ", 50.45)
        cmds.setAttr("pinky_03_r.rotateY", 0)
        cmds.setAttr("pinky_03_r.rotateX", 0)
        cmds.setAttr("middle_03_r.rotateZ", 50.4503)
        cmds.setAttr("middle_03_r.rotateY", 0)
        cmds.setAttr("middle_03_r.rotateX", 0)
        cmds.setAttr("index_03_r.rotateZ", 50.4503)
        cmds.setAttr("index_03_r.rotateY", 0)
        cmds.setAttr("index_03_r.rotateX", 0)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

         # remove unneeded joints
        jointsToRemove = ["thrusterVent_l_01", "thrusterVent_l_02", "thrusterVent_r_01", "thrusterVent_r_02", "thruster_casing_l", "thruster_a_l", "thruster_b_l", "thruster_c_l", "thruster_casing_r", "thruster_a_r", "thruster_b_r", "thruster_c_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulderflap_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulderflap_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulderPad_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulderPad_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["dagger_c_l", "dagger_b_l", "dagger_a_l"]
        jointToTransferTo = "dagger_base_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["dagger_c_r", "dagger_b_r", "dagger_a_r"]
        jointToTransferTo = "dagger_base_r"
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

        jointsToRemove = ["thumb_03_r"]
        jointToTransferTo = "thumb_02_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tentacle_spring_l", "tentacle_spring_r"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Kurohane_LOD2.fbx", "Kurohane_Geo_LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

KurohaneLOD2()

# Kurohane LOD3 -------------------------------------------------------------------------------------------------------------
def KurohaneLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["geo_dagger_mechanisms_LOD3", "geo_helmet_LOD3", "geo_l_wrist_LOD3", "geo_legs_LOD3", "geo_lower_arm_casings_LOD3", "geo_lower_arms_LOD3", "geo_r_wrist_LOD3", "geo_shoulder_plates_LOD3", "geo_sword_l_LOD3", "geo_sword_r_LOD3", "geo_tentacles_LOD3", "geo_thruster_a_LOD3", "geo_thruster_b_LOD3", "geo_thruster_c_LOD3", "geo_thruster_home_LOD3", "geo_thruster_vents_LOD3", "geo_torso_LOD3", "geo_upper_arms_LOD3"]
    
        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 0)
        cmds.setAttr("thumb_03_l.rotateY", -35.5635)
        cmds.setAttr("thumb_03_l.rotateX", 0)
        cmds.setAttr("thumb_02_l.rotateZ", 0)
        cmds.setAttr("thumb_02_l.rotateY", -14.1631)
        cmds.setAttr("thumb_02_l.rotateX", 0)
        cmds.setAttr("thumb_01_l.rotateZ", -13.26)
        cmds.setAttr("thumb_01_l.rotateY", 38.59)
        cmds.setAttr("thumb_01_l.rotateX", -8.362)
        cmds.setAttr("pinky_03_l.rotateZ", 77.45)
        cmds.setAttr("pinky_03_l.rotateY", 0)
        cmds.setAttr("pinky_03_l.rotateX", 0)
        cmds.setAttr("pinky_02_l.rotateZ", 120.683)
        cmds.setAttr("pinky_02_l.rotateY", 0)
        cmds.setAttr("pinky_02_l.rotateX", 0)
        cmds.setAttr("pinky_01_l.rotateZ", 32.2949)
        cmds.setAttr("pinky_01_l.rotateY", 0)
        cmds.setAttr("pinky_01_l.rotateX", 0)
        cmds.setAttr("middle_03_l.rotateZ", 77.4503)
        cmds.setAttr("middle_03_l.rotateY", 0)
        cmds.setAttr("middle_03_l.rotateX", 0)
        cmds.setAttr("middle_02_l.rotateZ", 120.683)
        cmds.setAttr("middle_02_l.rotateY", 0)
        cmds.setAttr("middle_02_l.rotateX", 0)
        cmds.setAttr("middle_01_l.rotateZ", 32.294)
        cmds.setAttr("middle_01_l.rotateY", 0)
        cmds.setAttr("middle_01_l.rotateX", 0)
        cmds.setAttr("index_03_l.rotateZ", 77.4503)
        cmds.setAttr("index_03_l.rotateY", 0)
        cmds.setAttr("index_03_l.rotateX", 0)
        cmds.setAttr("index_02_l.rotateZ", 120.683)
        cmds.setAttr("index_02_l.rotateY", 0)
        cmds.setAttr("index_02_l.rotateX", 0)
        cmds.setAttr("index_01_l.rotateZ", 32.294)
        cmds.setAttr("index_01_l.rotateY", 0)
        cmds.setAttr("index_01_l.rotateX", 0)

        cmds.setAttr("thumb_03_r.rotateZ", 0)
        cmds.setAttr("thumb_03_r.rotateY", -35.5635)
        cmds.setAttr("thumb_03_r.rotateX", 0)
        cmds.setAttr("thumb_02_r.rotateZ", 0)
        cmds.setAttr("thumb_02_r.rotateY", -14.1631)
        cmds.setAttr("thumb_02_r.rotateX", 0)
        cmds.setAttr("thumb_01_r.rotateZ", -13.26)
        cmds.setAttr("thumb_01_r.rotateY", 38.59)
        cmds.setAttr("thumb_01_r.rotateX", -8.362)
        cmds.setAttr("pinky_03_r.rotateZ", 77.45)
        cmds.setAttr("pinky_03_r.rotateY", 0)
        cmds.setAttr("pinky_03_r.rotateX", 0)
        cmds.setAttr("pinky_02_r.rotateZ", 120.683)
        cmds.setAttr("pinky_02_r.rotateY", 0)
        cmds.setAttr("pinky_02_r.rotateX", 0)
        cmds.setAttr("pinky_01_r.rotateZ", 32.2949)
        cmds.setAttr("pinky_01_r.rotateY", 0)
        cmds.setAttr("pinky_01_r.rotateX", 0)
        cmds.setAttr("middle_03_r.rotateZ", 77.4503)
        cmds.setAttr("middle_03_r.rotateY", 0)
        cmds.setAttr("middle_03_r.rotateX", 0)
        cmds.setAttr("middle_02_r.rotateZ", 120.683)
        cmds.setAttr("middle_02_r.rotateY", 0)
        cmds.setAttr("middle_02_r.rotateX", 0)
        cmds.setAttr("middle_01_r.rotateZ", 32.294)
        cmds.setAttr("middle_01_r.rotateY", 0)
        cmds.setAttr("middle_01_r.rotateX", 0)
        cmds.setAttr("index_03_r.rotateZ", 77.4503)
        cmds.setAttr("index_03_r.rotateY", 0)
        cmds.setAttr("index_03_r.rotateX", 0)
        cmds.setAttr("index_02_r.rotateZ", 120.683)
        cmds.setAttr("index_02_r.rotateY", 0)
        cmds.setAttr("index_02_r.rotateX", 0)
        cmds.setAttr("index_01_r.rotateZ", 32.294)
        cmds.setAttr("index_01_r.rotateY", 0)
        cmds.setAttr("index_01_r.rotateX", 0)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["thrusterVent_l_01", "thrusterVent_l_02", "thrusterVent_r_01", "thrusterVent_r_02", "thruster_casing_l", "thruster_a_l", "thruster_b_l", "thruster_c_l", "thruster_casing_r", "thruster_a_r", "thruster_b_r", "thruster_c_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulderflap_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulderflap_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "shoulderPad_l", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "shoulderPad_r", "upperarm_twist_02_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerArm_bottomPlateA_l", "lowerArm_bottomPlateB_l", "lowerArm_topPlateA_l", "lowerArm_topPlateB_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["dagger_c_l", "dagger_b_l", "dagger_a_l"]
        jointToTransferTo = "dagger_base_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerArm_bottomPlateA_r", "lowerArm_bottomPlateB_r", "lowerArm_topPlateA_r", "lowerArm_topPlateB_r"]
        jointToTransferTo = "dagger_base_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["dagger_c_r", "dagger_b_r", "dagger_a_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_l", "index_02_l", "index_03_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["sword_root_l", "sword_handle_l", "pivotA_l", "extensionA_l", "pivotB_l", "extensionB_l"]
        jointToTransferTo = "weapon_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["sword_root_r", "sword_handle_r", "pivotA_r", "extensionA_r", "pivotB_r", "extensionB_r"]
        jointToTransferTo = "weapon_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tentacle_spring_l", "tentacle_spring_r"]
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
        export.exportLOD("Kurohane_LOD3.fbx", "Kurohane_Geo_LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

KurohaneLOD3()

# Kurohane LOD4 -------------------------------------------------------------------------------------------------------------
def KurohaneLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["geo_dagger_mechanisms_LOD4", "geo_helmet_LOD4", "geo_legs_LOD4", "geo_lower_arm_casings_LOD4", "geo_lower_arms_LOD4", "geo_shoulder_plates_LOD4", "geo_sword_l_LOD4", "geo_sword_r_LOD4", "geo_thruster_vents_LOD4", "geo_torso_LOD4", "geo_upper_arms_LOD4"]


        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 0)
        cmds.setAttr("thumb_03_l.rotateY", -35.5635)
        cmds.setAttr("thumb_03_l.rotateX", 0)
        cmds.setAttr("thumb_02_l.rotateZ", 0)
        cmds.setAttr("thumb_02_l.rotateY", -14.1631)
        cmds.setAttr("thumb_02_l.rotateX", 0)
        cmds.setAttr("thumb_01_l.rotateZ", -13.26)
        cmds.setAttr("thumb_01_l.rotateY", 38.59)
        cmds.setAttr("thumb_01_l.rotateX", -8.362)
        cmds.setAttr("pinky_03_l.rotateZ", 77.45)
        cmds.setAttr("pinky_03_l.rotateY", 0)
        cmds.setAttr("pinky_03_l.rotateX", 0)
        cmds.setAttr("pinky_02_l.rotateZ", 120.683)
        cmds.setAttr("pinky_02_l.rotateY", 0)
        cmds.setAttr("pinky_02_l.rotateX", 0)
        cmds.setAttr("pinky_01_l.rotateZ", 32.2949)
        cmds.setAttr("pinky_01_l.rotateY", 0)
        cmds.setAttr("pinky_01_l.rotateX", 0)
        cmds.setAttr("middle_03_l.rotateZ", 77.4503)
        cmds.setAttr("middle_03_l.rotateY", 0)
        cmds.setAttr("middle_03_l.rotateX", 0)
        cmds.setAttr("middle_02_l.rotateZ", 120.683)
        cmds.setAttr("middle_02_l.rotateY", 0)
        cmds.setAttr("middle_02_l.rotateX", 0)
        cmds.setAttr("middle_01_l.rotateZ", 32.294)
        cmds.setAttr("middle_01_l.rotateY", 0)
        cmds.setAttr("middle_01_l.rotateX", 0)
        cmds.setAttr("index_03_l.rotateZ", 77.4503)
        cmds.setAttr("index_03_l.rotateY", 0)
        cmds.setAttr("index_03_l.rotateX", 0)
        cmds.setAttr("index_02_l.rotateZ", 120.683)
        cmds.setAttr("index_02_l.rotateY", 0)
        cmds.setAttr("index_02_l.rotateX", 0)
        cmds.setAttr("index_01_l.rotateZ", 32.294)
        cmds.setAttr("index_01_l.rotateY", 0)
        cmds.setAttr("index_01_l.rotateX", 0)

        cmds.setAttr("thumb_03_r.rotateZ", 0)
        cmds.setAttr("thumb_03_r.rotateY", -35.5635)
        cmds.setAttr("thumb_03_r.rotateX", 0)
        cmds.setAttr("thumb_02_r.rotateZ", 0)
        cmds.setAttr("thumb_02_r.rotateY", -14.1631)
        cmds.setAttr("thumb_02_r.rotateX", 0)
        cmds.setAttr("thumb_01_r.rotateZ", -13.26)
        cmds.setAttr("thumb_01_r.rotateY", 38.59)
        cmds.setAttr("thumb_01_r.rotateX", -8.362)
        cmds.setAttr("pinky_03_r.rotateZ", 77.45)
        cmds.setAttr("pinky_03_r.rotateY", 0)
        cmds.setAttr("pinky_03_r.rotateX", 0)
        cmds.setAttr("pinky_02_r.rotateZ", 120.683)
        cmds.setAttr("pinky_02_r.rotateY", 0)
        cmds.setAttr("pinky_02_r.rotateX", 0)
        cmds.setAttr("pinky_01_r.rotateZ", 32.2949)
        cmds.setAttr("pinky_01_r.rotateY", 0)
        cmds.setAttr("pinky_01_r.rotateX", 0)
        cmds.setAttr("middle_03_r.rotateZ", 77.4503)
        cmds.setAttr("middle_03_r.rotateY", 0)
        cmds.setAttr("middle_03_r.rotateX", 0)
        cmds.setAttr("middle_02_r.rotateZ", 120.683)
        cmds.setAttr("middle_02_r.rotateY", 0)
        cmds.setAttr("middle_02_r.rotateX", 0)
        cmds.setAttr("middle_01_r.rotateZ", 32.294)
        cmds.setAttr("middle_01_r.rotateY", 0)
        cmds.setAttr("middle_01_r.rotateX", 0)
        cmds.setAttr("index_03_r.rotateZ", 77.4503)
        cmds.setAttr("index_03_r.rotateY", 0)
        cmds.setAttr("index_03_r.rotateX", 0)
        cmds.setAttr("index_02_r.rotateZ", 120.683)
        cmds.setAttr("index_02_r.rotateY", 0)
        cmds.setAttr("index_02_r.rotateX", 0)
        cmds.setAttr("index_01_r.rotateZ", 32.294)
        cmds.setAttr("index_01_r.rotateY", 0)
        cmds.setAttr("index_01_r.rotateX", 0)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["thrusterVent_l_01", "thrusterVent_l_02", "thrusterVent_r_01", "thrusterVent_r_02", "thruster_casing_l", "thruster_a_l", "thruster_b_l", "thruster_c_l", "thruster_casing_r", "thruster_a_r", "thruster_b_r", "thruster_c_r"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulderflap_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulderflap_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "shoulderPad_l", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "shoulderPad_r", "upperarm_twist_02_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerArm_bottomPlateA_l", "lowerArm_bottomPlateB_l", "lowerArm_topPlateA_l", "lowerArm_topPlateB_l", "dagger_c_l", "dagger_b_l", "dagger_a_l", "dagger_base_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerArm_bottomPlateA_r", "lowerArm_bottomPlateB_r", "lowerArm_topPlateA_r", "lowerArm_topPlateB_r", "dagger_c_r", "dagger_b_r", "dagger_a_r", "dagger_base_r"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_l", "index_02_l", "index_03_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["sword_root_l", "sword_handle_l", "pivotA_l", "extensionA_l", "pivotB_l", "extensionB_l"]
        jointToTransferTo = "weapon_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["sword_root_r", "sword_handle_r", "pivotA_r", "extensionA_r", "pivotB_r", "extensionB_r"]
        jointToTransferTo = "weapon_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tentacle_l_01", "tentacle_l_02", "tentacle_l_03", "tentacle_l_04", "tentacle_l_05", "tentacle_l_06", "tentacle_l_07", "tentacle_l_08", "tentacle_l_09", "tentacle_l_010", "tentacle_r_01", "tentacle_r_02", "tentacle_r_03", "tentacle_r_04", "tentacle_r_05", "tentacle_r_06", "tentacle_r_07", "tentacle_r_08", "tentacle_r_09", "tentacle_r_010", "tentacle_spring_l", "tentacle_spring_r"]
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
        export.exportLOD("Kurohane_LOD4.fbx", "Kurohane_Geo_LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

KurohaneLOD4()


