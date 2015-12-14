import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## MURIEL #########################################################################
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



# Muriel LOD1 -------------------------------------------------------------------------------------------------------------
def MurielLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)

        meshes = ["corneas_geo_LOD1", "eye_shadow_geo_LOD1", "eyeballs_geo_LOD1", "Head_geo_LOD1", "Legs_Shell_geo_LOD1", "lowerteeth_geo_LOD1", "Soft_geo_LOD1", "tongue_geo_LOD1", "Torso_Shell_geo_LOD1", "Upperteeth_geo_LOD1", "Wings_Blade_A_geo_LOD1", "Wings_Blade_B_geo_LOD1", "Wings_Blade_C_geo_LOD1", "Wings_Blade_D_geo_LOD1", "Wings_Top_Geo_LOD1"]

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["chest_ring"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        jointsToRemove = ["eyeball_l", "eyeball_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Muriel_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

MurielLOD1()

# Muriel LOD2 -------------------------------------------------------------------------------------------------------------
def MurielLOD2():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["corneas_geo_LOD2", "eye_shadow_geo_LOD2", "eyeballs_geo_LOD2", "Head_geo_LOD2", "Legs_Shell_geo_LOD2", "lowerteeth_geo_LOD2", "Soft_geo_LOD2", "tongue_geo_LOD2", "Torso_Shell_geo_LOD2", "Upperteeth_geo_LOD2", "Wings_Blade_A_geo_LOD2", "Wings_Blade_B_geo_LOD2", "Wings_Blade_C_geo_LOD2", "Wings_Blade_D_geo_LOD2", "Wings_Top_Geo_LOD2"]

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
        jointsToRemove = ["chest_ring"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hand_flap_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hand_flap_r"]
        jointToTransferTo = "lowerarm_r"
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

        jointsToRemove = ["thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "thumb_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        jointsToRemove = ["eyeball_l", "eyeball_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_01_r"]
        jointToTransferTo = "thigh_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["foot_flap_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["foot_flap_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)


        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Muriel_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

MurielLOD2()

# Muriel LOD3 -------------------------------------------------------------------------------------------------------------
def MurielLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["corneas_geo_LOD3", "eye_shadow_geo_LOD3", "eyeballs_geo_LOD3", "Head_geo_LOD3", "Legs_Shell_geo_LOD3", "lowerteeth_geo_LOD3", "Soft_geo_LOD3", "tongue_geo_LOD3", "Torso_Shell_geo_LOD3", "Upperteeth_geo_LOD3", "Wings_Blade_A_geo_LOD3", "Wings_Blade_B_geo_LOD3", "Wings_Blade_C_geo_LOD3", "Wings_Blade_D_geo_LOD3", "Wings_Top_Geo_LOD3"]
    
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
        jointsToRemove = ["chest_ring"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["wing_blade_D_l", "wing_blade_A_l", "wing_blade_B_l", "wing_blade_C_l"]
        jointToTransferTo = "wing_blade_track_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["wing_blade_A_r", "wing_blade_B_r", "wing_blade_C_r", "wing_blade_D_r"]
        jointToTransferTo = "wing_blade_track_r"
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

        jointsToRemove = ["hand_flap_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hand_flap_r"]
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

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw"]
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

        jointsToRemove = ["foot_flap_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["foot_flap_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Muriel_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

MurielLOD3()

# Muriel LOD4 -------------------------------------------------------------------------------------------------------------
def MurielLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["corneas_geo_LOD4", "eye_shadow_geo_LOD4", "eyeballs_geo_LOD4", "Head_geo_LOD4", "Legs_Shell_geo_LOD4", "lowerteeth_geo_LOD4", "Soft_geo_LOD4", "tongue_geo_LOD4", "Torso_Shell_geo_LOD4", "Upperteeth_geo_LOD4", "Wings_Blade_A_geo_LOD4", "Wings_Blade_B_geo_LOD4", "Wings_Blade_C_geo_LOD4", "Wings_Blade_D_geo_LOD4", "Wings_Top_Geo_LOD4"]

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
        jointsToRemove = ["chest_ring"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["wing_blade_D_l", "wing_blade_A_l", "wing_blade_B_l", "wing_blade_C_l"]
        jointToTransferTo = "wing_blade_track_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["wing_blade_A_r", "wing_blade_B_r", "wing_blade_C_r", "wing_blade_D_r"]
        jointToTransferTo = "wing_blade_track_r"
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

        jointsToRemove = ["hand_flap_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hand_flap_r"]
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

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "jaw"]
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

        jointsToRemove = ["foot_flap_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["foot_flap_r"]
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
        export.exportLOD("Muriel_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

MurielLOD4()


