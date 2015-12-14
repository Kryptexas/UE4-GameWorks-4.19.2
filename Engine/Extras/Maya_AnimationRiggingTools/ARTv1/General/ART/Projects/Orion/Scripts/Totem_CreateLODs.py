import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## TOTEM #########################################################################
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



# Totem LOD1 -------------------------------------------------------------------------------------------------------------
def TotemLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)

        meshes = ["Tri_Totem_Body_LOD1", "Tri_Totem_Collar_LOD1", "Tri_Totem_Cornea_LOD1", "Tri_Totem_DogBones_LOD1", "Tri_Totem_Eyebrows_LOD1", "Tri_Totem_Eyelashes_LOD1", "Tri_Totem_Eyes_LOD1", "Tri_Totem_EyeShadow_LOD1", "Tri_Totem_Hair_LOD1", "Tri_Totem_HairBag_LOD1", "Tri_Totem_HairTie_LOD1", "Tri_Totem_Hands_LOD1", "Tri_Totem_HandTech_LOD1", "Tri_Totem_Head_LOD1", "Tri_Totem_Knees_LOD1", "Tri_Totem_Legs_LOD1", "Tri_Totem_Mouth_LOD1", "Tri_Totem_ShoulderPads_LOD1", "Tri_Totem_Weapon_LOD1", "Tri_Totem_Weapon_Bottom_FrontBase_LOD1", "Tri_Totem_Weapon_Bottom_FrontEnd_LOD1", "Tri_Totem_Weapon_Handle_LOD1", "Tri_Totem_Weapon_SupportBar_LOD1", "Tri_Totem_Weapon_TopBack_LOD1", "Tri_Totem_Weapon_TopFront_LOD1"]

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["glasses", "hair_l", "hair_r", "ponytail_01", "ponytail_02", "ponytail_03"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["knee_pad_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["knee_pad_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Totem_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

TotemLOD1()

# Totem LOD2 -------------------------------------------------------------------------------------------------------------
def TotemLOD2():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["Tri_Totem_Body_LOD2", "Tri_Totem_Collar_LOD2", "Tri_Totem_Cornea_LOD2", "Tri_Totem_DogBones_LOD2", "Tri_Totem_Eyebrows_LOD2", "Tri_Totem_Eyelashes_LOD2", "Tri_Totem_Eyes_LOD2", "Tri_Totem_EyeShadow_LOD2", "Tri_Totem_Hair_LOD2", "Tri_Totem_HairBag_LOD2", "Tri_Totem_HairTie_LOD2", "Tri_Totem_Hands_LOD2", "Tri_Totem_HandTech_LOD2", "Tri_Totem_Head_LOD2", "Tri_Totem_Knees_LOD2", "Tri_Totem_Legs_LOD2", "Tri_Totem_Mouth_LOD2", "Tri_Totem_ShoulderPads_LOD2", "Tri_Totem_Weapon_LOD2", "Tri_Totem_Weapon_Bottom_FrontBase_LOD2", "Tri_Totem_Weapon_Bottom_FrontEnd_LOD2", "Tri_Totem_Weapon_Handle_LOD2", "Tri_Totem_Weapon_SupportBar_LOD2", "Tri_Totem_Weapon_TopBack_LOD2", "Tri_Totem_Weapon_TopFront_LOD2"]

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
        jointsToRemove = ["back_plate", "chest_l", "chest_r", "collar", "collar_clav_l", "hose_l", "l_hose_base", "collar_clav_r", "hose_r", "r_hose_base"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["glasses", "hair_l", "hair_r", "ponytail_01", "ponytail_02", "ponytail_03"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "shoulder_pad_l", "l_hose_end", "l_pad_end", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "shoulder_pad_r", "r_hose_end", "r_pad_end", "upperarm_twist_02_r"]
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

        jointsToRemove = ["hand_plate_l"]
        jointToTransferTo = "hand_l"
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

        jointsToRemove = ["hand_plate_r"]
        jointToTransferTo = "hand_r"
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

        jointsToRemove = ["thigh_twist_02_l"]
        jointToTransferTo = "thigh_twist_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["thigh_twist_02_r"]
        jointToTransferTo = "thigh_twist_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_l", "calf_twist_02_l", "knee_pad_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_r", "calf_twist_02_r", "knee_pad_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Totem_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

TotemLOD2()

# Totem LOD3 -------------------------------------------------------------------------------------------------------------
def TotemLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["Tri_Totem_Body_LOD3", "Tri_Totem_Collar_LOD3", "Tri_Totem_Cornea_LOD3", "Tri_Totem_DogBones_LOD3", "Tri_Totem_Eyebrows_LOD3", "Tri_Totem_Eyelashes_LOD3", "Tri_Totem_Eyes_LOD3", "Tri_Totem_EyeShadow_LOD3", "Tri_Totem_Hair_LOD3", "Tri_Totem_HairBag_LOD3", "Tri_Totem_HairTie_LOD3", "Tri_Totem_Hands_LOD3", "Tri_Totem_HandTech_LOD3", "Tri_Totem_Head_LOD3", "Tri_Totem_Knees_LOD3", "Tri_Totem_Legs_LOD3", "Tri_Totem_Mouth_LOD3", "Tri_Totem_ShoulderPads_LOD3", "Tri_Totem_Weapon_LOD3", "Tri_Totem_Weapon_Bottom_FrontBase_LOD3", "Tri_Totem_Weapon_Bottom_FrontEnd_LOD3", "Tri_Totem_Weapon_Handle_LOD3", "Tri_Totem_Weapon_SupportBar_LOD3", "Tri_Totem_Weapon_TopBack_LOD3", "Tri_Totem_Weapon_TopFront_LOD3"]
    
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
        cmds.setAttr("index_02_l.rotateZ", 60.61060457)
        cmds.setAttr("index_01_l.rotateZ", 60.61060457)

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
        cmds.setAttr("index_02_r.rotateZ", 60.61060457)
        cmds.setAttr("index_01_r.rotateZ", 60.61060457)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["back_plate", "chest_l", "chest_r", "collar", "collar_clav_l", "hose_l", "l_hose_base", "collar_clav_r", "hose_r", "r_hose_base"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["glasses", "hair_l", "hair_r", "ponytail_01", "ponytail_02", "ponytail_03"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "shoulder_pad_l", "l_hose_end", "l_pad_end", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "shoulder_pad_r", "r_hose_end", "r_pad_end", "upperarm_twist_02_r"]
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

        jointsToRemove = ["hand_plate_l", "index_01_l", "index_02_l", "index_03_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "ring_01_l", "ring_02_l", "ring_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hand_plate_r", "index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "ring_01_r", "ring_02_r", "ring_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["staff_bottom", "handle", "hinge_a", "hinge_b", "staff_top", "tip_spin", "top_tip_a", "top_tip_b", "top_hinge"]
        jointToTransferTo = "weapon"
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

        jointsToRemove = ["calf_twist_01_l", "calf_twist_02_l", "knee_pad_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_r", "calf_twist_02_r", "knee_pad_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Totem_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

TotemLOD3()

# Totem LOD4 -------------------------------------------------------------------------------------------------------------
def TotemLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["Tri_Totem_Body_LOD4", "Tri_Totem_Collar_LOD4", "Tri_Totem_Cornea_LOD4", "Tri_Totem_DogBones_LOD4", "Tri_Totem_Eyebrows_LOD4", "Tri_Totem_Eyelashes_LOD4", "Tri_Totem_Eyes_LOD4", "Tri_Totem_EyeShadow_LOD4", "Tri_Totem_Hair_LOD4", "Tri_Totem_HairBag_LOD4", "Tri_Totem_HairTie_LOD4", "Tri_Totem_Hands_LOD4", "Tri_Totem_HandTech_LOD4", "Tri_Totem_Head_LOD4", "Tri_Totem_Knees_LOD4", "Tri_Totem_Legs_LOD4", "Tri_Totem_Mouth_LOD4", "Tri_Totem_ShoulderPads_LOD4", "Tri_Totem_Weapon_LOD4", "Tri_Totem_Weapon_Bottom_FrontBase_LOD4", "Tri_Totem_Weapon_Bottom_FrontEnd_LOD4", "Tri_Totem_Weapon_Handle_LOD4", "Tri_Totem_Weapon_SupportBar_LOD4", "Tri_Totem_Weapon_TopBack_LOD4", "Tri_Totem_Weapon_TopFront_LOD4"]

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
        cmds.setAttr("index_02_l.rotateZ", 60.61060457)
        cmds.setAttr("index_01_l.rotateZ", 60.61060457)

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
        cmds.setAttr("index_02_r.rotateZ", 60.61060457)
        cmds.setAttr("index_01_r.rotateZ", 60.61060457)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["back_plate", "chest_l", "chest_r", "collar", "collar_clav_l", "hose_l", "l_hose_base", "collar_clav_r", "hose_r", "r_hose_base"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["glasses", "hair_l", "hair_r", "ponytail_01", "ponytail_02", "ponytail_03"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "shoulder_pad_l", "l_hose_end", "l_pad_end", "upperarm_twist_02_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "shoulder_pad_r", "r_hose_end", "r_pad_end", "upperarm_twist_02_r"]
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

        jointsToRemove = ["hand_plate_l", "index_01_l", "index_02_l", "index_03_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "ring_01_l", "ring_02_l", "ring_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hand_plate_r", "index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "ring_01_r", "ring_02_r", "ring_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["staff_bottom", "handle", "hinge_a", "hinge_b", "staff_top", "tip_spin", "top_tip_a", "top_tip_b", "top_hinge"]
        jointToTransferTo = "weapon"
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

        jointsToRemove = ["calf_twist_01_l", "calf_twist_02_l", "knee_pad_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_r", "calf_twist_02_r", "knee_pad_r"]
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
        export.exportLOD("Totem_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

TotemLOD4()


