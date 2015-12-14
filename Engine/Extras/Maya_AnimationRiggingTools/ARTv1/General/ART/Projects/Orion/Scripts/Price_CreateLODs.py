import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## PRICE #########################################################################
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



# Price LOD1 -------------------------------------------------------------------------------------------------------------
def PriceLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)

        meshes = ["abdomen_geo_LOD1", "abdomen_plate_geo_LOD1", "arm_hoses_big_geo_LOD1", "arm_hoses_sml_geo_LOD1", "arms_geo_LOD1", "cornea_geo_LOD1", "elbow_pads_geo_LOD1", "eye_shadow_geo_LOD1", "eyeballs_geo_LOD1", "feet_geo_LOD1", "forearm_armor_geo_LOD1", "gun_barrel_geo_LOD1", "gun_barrel_inner_geo_LOD1", "gun_body_panels_geo_LOD1", "gun_geo_LOD1", "gun_handle_geo_LOD1", "gun_scope_geo_LOD1", "gun_scope_holder_geo_LOD1", "gun_stock_base_geo_LOD1", "gun_stock_base_in_geo_LOD1", "gun_stock_base_out_geo_LOD1", "gun_stock_bott_geo_LOD1", "gun_stock_in_geo_LOD1", "gun_stock_panels_geo_LOD1", "hand_cover_r_geo_LOD1", "hands_geo_LOD1", "head_geo_LOD1", "knuckles_geo_LOD1", "loarm_pistonB_bott_geo_LOD1", "loarm_pistonB_top_geo_LOD1", "loarm_pistonC_bott_geo_LOD1", "loarm_pistonC_top_geo_LOD1", "loarm_pistonD_bot_geo_LOD1", "loarm_pistonD_top_geo_LOD1", "lower_teeth_geo_LOD1", "mask_geo_LOD1", "packs_geo_LOD1", "shins_geo_LOD1", "shotgun_arm_geo_LOD1", "shotgun_geo_LOD1", "shoulderpad_r_geo_LOD1", "sniper_canister_arm_01_geo_LOD1", "sniper_canister_arm_02_geo_LOD1", "sniper_canister_arm_03_geo_LOD1", "sniper_canister_geo_LOD1", "tongue_geo_LOD1", "torso_geo_LOD1", "trap_geo_LOD1", "uparm_pistonC_bicep_bott_geo_LOD1", "uparm_pistonC_bicep_top_geo_LOD1", "upper_legs_geo_LOD1", "upper_teeth_geo_LOD1"]

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["shotgun_clip"]
        jointToTransferTo = "shotgun"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["arm_pouch_l"]
        jointToTransferTo = "upperarm_twist_02_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["wrist_cover"]
        jointToTransferTo = "lowerarm_twist_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "helmet_sight", "mask"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Price_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

PriceLOD1()

# Price LOD2 -------------------------------------------------------------------------------------------------------------
def PriceLOD2():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["abdomen_geo_LOD2", "abdomen_plate_geo_LOD2", "arm_hoses_big_geo_LOD2", "arm_hoses_sml_geo_LOD2", "arms_geo_LOD2", "cornea_geo_LOD2", "elbow_pads_geo_LOD2", "eye_shadow_geo_LOD2", "eyeballs_geo_LOD2", "feet_geo_LOD2", "forearm_armor_geo_LOD2", "gun_barrel_geo_LOD2", "gun_barrel_inner_geo_LOD2", "gun_body_panels_geo_LOD2", "gun_geo_LOD2", "gun_handle_geo_LOD2", "gun_scope_geo_LOD2", "gun_scope_holder_geo_LOD2", "gun_stock_base_geo_LOD2", "gun_stock_base_in_geo_LOD2", "gun_stock_base_out_geo_LOD2", "gun_stock_bott_geo_LOD2", "gun_stock_in_geo_LOD2", "gun_stock_panels_geo_LOD2", "hand_cover_r_geo_LOD2", "hands_geo_LOD2", "head_geo_LOD2", "knuckles_geo_LOD2", "loarm_pistonB_bott_geo_LOD2", "loarm_pistonB_top_geo_LOD2", "loarm_pistonC_bott_geo_LOD2", "loarm_pistonC_top_geo_LOD2", "loarm_pistonD_bot_geo_LOD2", "loarm_pistonD_top_geo_LOD2", "lower_teeth_geo_LOD2", "mask_geo_LOD2", "packs_geo_LOD2", "shins_geo_LOD2", "shotgun_arm_geo_LOD2", "shotgun_geo_LOD2", "shoulderpad_r_geo_LOD2", "sniper_canister_arm_01_geo_LOD2", "sniper_canister_arm_02_geo_LOD2", "sniper_canister_arm_03_geo_LOD2", "sniper_canister_geo_LOD2", "tongue_geo_LOD2", "torso_geo_LOD2", "trap_geo_LOD2", "uparm_pistonC_bicep_bott_geo_LOD2", "uparm_pistonC_bicep_top_geo_LOD2", "upper_legs_geo_LOD2", "upper_teeth_geo_LOD2"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 30.05735749)
        cmds.setAttr("thumb_03_l.rotateY", 4.575153365e-015)
        cmds.setAttr("thumb_03_l.rotateX", 2.807367528e-014)
        cmds.setAttr("ring_03_l.rotateZ", 50.02688615)
        cmds.setAttr("ring_03_l.rotateY", -4.767840667e-014)
        cmds.setAttr("ring_03_l.rotateX", 2.313919533e-014)
        cmds.setAttr("pinky_03_l.rotateZ", 50.02688615)
        cmds.setAttr("pinky_03_l.rotateY", -1.296376071e-015)
        cmds.setAttr("pinky_03_l.rotateX", 4.230075606e-014)
        cmds.setAttr("middle_03_l.rotateZ", 55.02709033)
        cmds.setAttr("middle_03_l.rotateY", -1.652397549e-014)
        cmds.setAttr("middle_03_l.rotateX", 2.297826517e-014)
        cmds.setAttr("index_03_l.rotateZ", 64.64121137)
        cmds.setAttr("index_03_l.rotateY", -2.480634643e-014)
        cmds.setAttr("index_03_l.rotateX", 1.077164418e-014)

        cmds.setAttr("thumb_03_r.rotateZ", 30.05735749)
        cmds.setAttr("thumb_03_r.rotateY", 4.575153365e-015)
        cmds.setAttr("thumb_03_r.rotateX", 2.807367528e-014)
        cmds.setAttr("ring_03_r.rotateZ", 50.02688615)
        cmds.setAttr("ring_03_r.rotateY", -4.767840667e-014)
        cmds.setAttr("ring_03_r.rotateX", 2.313919533e-014)
        cmds.setAttr("pinky_03_r.rotateZ", 50.02688615)
        cmds.setAttr("pinky_03_r.rotateY", -1.296376071e-015)
        cmds.setAttr("pinky_03_r.rotateX", 4.230075606e-014)
        cmds.setAttr("middle_03_r.rotateZ", 55.02709033)
        cmds.setAttr("middle_03_r.rotateY", -1.652397549e-014)
        cmds.setAttr("middle_03_r.rotateX", 2.297826517e-014)
        cmds.setAttr("index_03_r.rotateZ", 64.64121137)
        cmds.setAttr("index_03_r.rotateY", -2.480634643e-014)
        cmds.setAttr("index_03_r.rotateX", 1.077164418e-014)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

         # remove unneeded joints
        jointsToRemove = ["hip_pack_a", "hip_pack_b", "hip_pack_c", "hip_pack_d"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["sniper_can_arm_01", "sniper_can_arm_02", "sniper_can_arm_03", "pack_hose_out", "pack_hose_in"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shotgun_clip"]
        jointToTransferTo = "shotgun"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["arm_pouch_l"]
        jointToTransferTo = "upperarm_twist_02_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["bot_uparm", "bot_uparm_pistonC_top"]
        jointToTransferTo = "upperarm_twist_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["arm_hose_top", "bot_loarm", "bot_loarm_pistonB_top", "bot_loarm_pistonC_top", "bot_loarm_pistonD_top", "bot_loarm_twist", "bot_loarm_pistonC_bot", "bot_loarm_pistonD_bot", "bot_uparm_pistonC_bot", "arm_hose_bott", "wrist_cover"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["arm_hose_bott", "wrist_cover"]
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

        jointsToRemove = ["gun_barrel", "gun_body_panel_l", "gun_body_panel_r", "gun_scope", "gun_stock_base", "gun_barrel_inner", "gun_stock_base_in_bott", "gun_stock_base_out_bott", "gun_stock_base_in_top", "gun_stock_base_out_top", "gun_stock_in_bott", "gun_stock_in_top", "gun_stock_bott", "gun_stock_panel_l", "gun_stock_panel_r"]
        jointToTransferTo = "gun"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["bot_loarm_pistonB_bot"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "helmet_sight", "jaw", "mask"]
        jointToTransferTo = "head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ankle_plate_in_l", "ankle_plate_out_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ankle_plate_in_r", "ankle_plate_out_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["heel_plate_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["heel_plate_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Price_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

PriceLOD2()

# Price LOD3 -------------------------------------------------------------------------------------------------------------
def PriceLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["abdomen_geo_LOD3", "abdomen_plate_geo_LOD3", "arm_hoses_big_geo_LOD3", "arm_hoses_sml_geo_LOD3", "arms_geo_LOD3", "cornea_geo_LOD3", "elbow_pads_geo_LOD3", "eye_shadow_geo_LOD3", "eyeballs_geo_LOD3", "feet_geo_LOD3", "forearm_armor_geo_LOD3", "gun_barrel_geo_LOD3", "gun_barrel_inner_geo_LOD3", "gun_body_panels_geo_LOD3", "gun_geo_LOD3", "gun_handle_geo_LOD3", "gun_scope_geo_LOD3", "gun_scope_holder_geo_LOD3", "gun_stock_base_geo_LOD3", "gun_stock_base_in_geo_LOD3", "gun_stock_base_out_geo_LOD3", "gun_stock_bott_geo_LOD3", "gun_stock_in_geo_LOD3", "gun_stock_panels_geo_LOD3", "hand_cover_r_geo_LOD3", "hands_geo_LOD3", "head_geo_LOD3", "knuckles_geo_LOD3", "loarm_pistonB_bott_geo_LOD3", "loarm_pistonB_top_geo_LOD3", "loarm_pistonC_bott_geo_LOD3", "loarm_pistonC_top_geo_LOD3", "loarm_pistonD_bot_geo_LOD3", "loarm_pistonD_top_geo_LOD3", "lower_teeth_geo_LOD3", "mask_geo_LOD3", "packs_geo_LOD3", "shins_geo_LOD3", "shotgun_arm_geo_LOD3", "shotgun_geo_LOD3", "shoulderpad_r_geo_LOD3", "sniper_canister_arm_01_geo_LOD3", "sniper_canister_arm_02_geo_LOD3", "sniper_canister_arm_03_geo_LOD3", "sniper_canister_geo_LOD3", "tongue_geo_LOD3", "torso_geo_LOD3", "trap_geo_LOD3", "uparm_pistonC_bicep_bott_geo_LOD3", "uparm_pistonC_bicep_top_geo_LOD3", "upper_legs_geo_LOD3", "upper_teeth_geo_LOD3"]
    
        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 30.05735749)
        cmds.setAttr("thumb_03_l.rotateY", 4.575153365e-015)
        cmds.setAttr("thumb_03_l.rotateX", 2.807367528e-014)
        cmds.setAttr("thumb_02_l.rotateZ", 22.03618142)
        cmds.setAttr("thumb_02_l.rotateY", 5.675048056)
        cmds.setAttr("thumb_02_l.rotateX", 22.94177697)
        cmds.setAttr("thumb_01_l.rotateZ", -9.145300774)
        cmds.setAttr("thumb_01_l.rotateY", 15.64425403)
        cmds.setAttr("thumb_01_l.rotateX", 14.45073388)
        cmds.setAttr("ring_03_l.rotateZ", 50.02688615)
        cmds.setAttr("ring_03_l.rotateY", -4.767840667e-014)
        cmds.setAttr("ring_03_l.rotateX", 2.313919533e-014)
        cmds.setAttr("ring_02_l.rotateZ", 54.79433274)
        cmds.setAttr("ring_02_l.rotateY", -7.710913552e-015)
        cmds.setAttr("ring_02_l.rotateX", 2.923377018e-014)
        cmds.setAttr("ring_01_l.rotateZ", 54.76549897)
        cmds.setAttr("ring_01_l.rotateY", -1.525905275e-014)
        cmds.setAttr("ring_01_l.rotateX", 4.56459293e-014)
        cmds.setAttr("pinky_03_l.rotateZ", 50.02688615)
        cmds.setAttr("pinky_03_l.rotateY", -1.296376071e-015)
        cmds.setAttr("pinky_03_l.rotateX", 4.230075606e-014)
        cmds.setAttr("pinky_02_l.rotateZ", 54.79433274)
        cmds.setAttr("pinky_02_l.rotateY", -3.788711524e-014)
        cmds.setAttr("pinky_02_l.rotateX", 1.292100339e-014)
        cmds.setAttr("pinky_01_l.rotateZ", 53.17737673)
        cmds.setAttr("pinky_01_l.rotateY", -10.46408531)
        cmds.setAttr("pinky_01_l.rotateX", 10.56158491)
        cmds.setAttr("middle_03_l.rotateZ", 55.02709033)
        cmds.setAttr("middle_03_l.rotateY", -1.652397549e-014)
        cmds.setAttr("middle_03_l.rotateX", 2.297826517e-014)
        cmds.setAttr("middle_02_l.rotateZ", 59.79453692)
        cmds.setAttr("middle_02_l.rotateY", -6.951251345e-015)
        cmds.setAttr("middle_02_l.rotateX", 2.166752877e-014)
        cmds.setAttr("middle_01_l.rotateZ", 51.05024479)
        cmds.setAttr("middle_01_l.rotateY", -1.133072605e-014)
        cmds.setAttr("middle_01_l.rotateX", 5.819421144e-014)
        cmds.setAttr("index_03_l.rotateZ", 64.64121137)
        cmds.setAttr("index_03_l.rotateY", -2.480634643e-014)
        cmds.setAttr("index_03_l.rotateX", 1.077164418e-014)
        cmds.setAttr("index_02_l.rotateZ", 64.20477863)
        cmds.setAttr("index_02_l.rotateY", -2.481003482e-014)
        cmds.setAttr("index_02_l.rotateX", 5.153492507e-014)
        cmds.setAttr("index_01_l.rotateZ", 46.94060635)
        cmds.setAttr("index_01_l.rotateY", 3.692762633)
        cmds.setAttr("index_01_l.rotateX", -8.969261741)

        cmds.setAttr("thumb_03_r.rotateZ", 30.05735749)
        cmds.setAttr("thumb_03_r.rotateY", 4.575153365e-015)
        cmds.setAttr("thumb_03_r.rotateX", 2.807367528e-014)
        cmds.setAttr("thumb_02_r.rotateZ", 22.03618142)
        cmds.setAttr("thumb_02_r.rotateY", 5.675048056)
        cmds.setAttr("thumb_02_r.rotateX", 22.94177697)
        cmds.setAttr("thumb_01_r.rotateZ", -9.145300774)
        cmds.setAttr("thumb_01_r.rotateY", 15.64425403)
        cmds.setAttr("thumb_01_r.rotateX", 14.45073388)
        cmds.setAttr("ring_03_r.rotateZ", 50.02688615)
        cmds.setAttr("ring_03_r.rotateY", -4.767840667e-014)
        cmds.setAttr("ring_03_r.rotateX", 2.313919533e-014)
        cmds.setAttr("ring_02_r.rotateZ", 54.79433274)
        cmds.setAttr("ring_02_r.rotateY", -7.710913552e-015)
        cmds.setAttr("ring_02_r.rotateX", 2.923377018e-014)
        cmds.setAttr("ring_01_r.rotateZ", 54.76549897)
        cmds.setAttr("ring_01_r.rotateY", -1.525905275e-014)
        cmds.setAttr("ring_01_r.rotateX", 4.56459293e-014)
        cmds.setAttr("pinky_03_r.rotateZ", 50.02688615)
        cmds.setAttr("pinky_03_r.rotateY", -1.296376071e-015)
        cmds.setAttr("pinky_03_r.rotateX", 4.230075606e-014)
        cmds.setAttr("pinky_02_r.rotateZ", 54.79433274)
        cmds.setAttr("pinky_02_r.rotateY", -3.788711524e-014)
        cmds.setAttr("pinky_02_r.rotateX", 1.292100339e-014)
        cmds.setAttr("pinky_01_r.rotateZ", 53.17737673)
        cmds.setAttr("pinky_01_r.rotateY", -10.46408531)
        cmds.setAttr("pinky_01_r.rotateX", 10.56158491)
        cmds.setAttr("middle_03_r.rotateZ", 55.02709033)
        cmds.setAttr("middle_03_r.rotateY", -1.652397549e-014)
        cmds.setAttr("middle_03_r.rotateX", 2.297826517e-014)
        cmds.setAttr("middle_02_r.rotateZ", 59.79453692)
        cmds.setAttr("middle_02_r.rotateY", -6.951251345e-015)
        cmds.setAttr("middle_02_r.rotateX", 2.166752877e-014)
        cmds.setAttr("middle_01_r.rotateZ", 51.05024479)
        cmds.setAttr("middle_01_r.rotateY", -1.133072605e-014)
        cmds.setAttr("middle_01_r.rotateX", 5.819421144e-014)
        cmds.setAttr("index_03_r.rotateZ", 64.64121137)
        cmds.setAttr("index_03_r.rotateY", -2.480634643e-014)
        cmds.setAttr("index_03_r.rotateX", 1.077164418e-014)
        cmds.setAttr("index_02_r.rotateZ", 64.20477863)
        cmds.setAttr("index_02_r.rotateY", -2.481003482e-014)
        cmds.setAttr("index_02_r.rotateX", 5.153492507e-014)
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
        jointsToRemove = ["hip_pack_a", "hip_pack_b", "hip_pack_c", "hip_pack_d"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["trap_handle"]
        jointToTransferTo = "spine_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["sniper_can_arm_01", "sniper_can_arm_02", "sniper_can_arm_03", "pack_hose_out", "pack_hose_in"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shotgun_clip"]
        jointToTransferTo = "shotgun"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l", "arm_pouch_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "bot_uparm", "bot_uparm_pistonC_top", "upperarm_twist_02_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_l", "lowerarm_twist_02_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["arm_hose_top", "bot_loarm", "bot_loarm_pistonB_top", "bot_loarm_pistonC_top", "bot_loarm_pistonD_top", "bot_loarm_twist", "bot_loarm_pistonC_bot", "bot_loarm_pistonD_bot", "bot_uparm_pistonC_bot", "lowerarm_twist_01_r", "arm_hose_bott", "wrist_cover"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_l", "index_02_l", "index_03_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "ring_01_l", "ring_02_l", "ring_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["gun_barrel", "gun_body_panel_l", "gun_body_panel_r", "gun_scope", "gun_stock_base", "gun_barrel_inner", "gun_stock_base_in_bott", "gun_stock_base_out_bott", "gun_stock_base_in_top", "gun_stock_base_out_top", "gun_stock_in_bott", "gun_stock_in_top", "gun_stock_bott", "gun_stock_panel_l", "gun_stock_panel_r"]
        jointToTransferTo = "gun"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["bot_loarm_pistonB_bot", "index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "ring_01_r", "ring_02_r", "ring_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "helmet_sight", "jaw", "mask"]
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

        jointsToRemove = ["ankle_plate_in_l", "ankle_plate_out_l", "calf_twist_01_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ankle_plate_in_r", "ankle_plate_out_r", "calf_twist_01_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["heel_plate_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["heel_plate_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Price_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

PriceLOD3()

# Price LOD4 -------------------------------------------------------------------------------------------------------------
def PriceLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["abdomen_geo_LOD4", "abdomen_plate_geo_LOD4", "arm_hoses_big_geo_LOD4", "arm_hoses_sml_geo_LOD4", "arms_geo_LOD4", "cornea_geo_LOD4", "elbow_pads_geo_LOD4", "eye_shadow_geo_LOD4", "eyeballs_geo_LOD4", "feet_geo_LOD4", "forearm_armor_geo_LOD4", "gun_barrel_geo_LOD4", "gun_barrel_inner_geo_LOD4", "gun_body_panels_geo_LOD4", "gun_geo_LOD4", "gun_handle_geo_LOD4", "gun_scope_geo_LOD4", "gun_scope_holder_geo_LOD4", "gun_stock_base_geo_LOD4", "gun_stock_base_in_geo_LOD4", "gun_stock_base_out_geo_LOD4", "gun_stock_bott_geo_LOD4", "gun_stock_in_geo_LOD4", "gun_stock_panels_geo_LOD4", "hand_cover_r_geo_LOD4", "hands_geo_LOD4", "head_geo_LOD4", "knuckles_geo_LOD4", "loarm_pistonB_bott_geo_LOD4", "loarm_pistonB_top_geo_LOD4", "loarm_pistonC_bott_geo_LOD4", "loarm_pistonC_top_geo_LOD4", "loarm_pistonD_bot_geo_LOD4", "loarm_pistonD_top_geo_LOD4", "lower_teeth_geo_LOD4", "mask_geo_LOD4", "packs_geo_LOD4", "shins_geo_LOD4", "shotgun_arm_geo_LOD4", "shotgun_geo_LOD4", "shoulderpad_r_geo_LOD4", "sniper_canister_arm_01_geo_LOD4", "sniper_canister_arm_02_geo_LOD4", "sniper_canister_arm_03_geo_LOD4", "sniper_canister_geo_LOD4", "tongue_geo_LOD4", "torso_geo_LOD4", "trap_geo_LOD4", "uparm_pistonC_bicep_bott_geo_LOD4", "uparm_pistonC_bicep_top_geo_LOD4", "upper_legs_geo_LOD4", "upper_teeth_geo_LOD4"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("thumb_03_l.rotateZ", 30.05735749)
        cmds.setAttr("thumb_03_l.rotateY", 4.575153365e-015)
        cmds.setAttr("thumb_03_l.rotateX", 2.807367528e-014)
        cmds.setAttr("thumb_02_l.rotateZ", 22.03618142)
        cmds.setAttr("thumb_02_l.rotateY", 5.675048056)
        cmds.setAttr("thumb_02_l.rotateX", 22.94177697)
        cmds.setAttr("thumb_01_l.rotateZ", -9.145300774)
        cmds.setAttr("thumb_01_l.rotateY", 15.64425403)
        cmds.setAttr("thumb_01_l.rotateX", 14.45073388)
        cmds.setAttr("ring_03_l.rotateZ", 50.02688615)
        cmds.setAttr("ring_03_l.rotateY", -4.767840667e-014)
        cmds.setAttr("ring_03_l.rotateX", 2.313919533e-014)
        cmds.setAttr("ring_02_l.rotateZ", 54.79433274)
        cmds.setAttr("ring_02_l.rotateY", -7.710913552e-015)
        cmds.setAttr("ring_02_l.rotateX", 2.923377018e-014)
        cmds.setAttr("ring_01_l.rotateZ", 54.76549897)
        cmds.setAttr("ring_01_l.rotateY", -1.525905275e-014)
        cmds.setAttr("ring_01_l.rotateX", 4.56459293e-014)
        cmds.setAttr("pinky_03_l.rotateZ", 50.02688615)
        cmds.setAttr("pinky_03_l.rotateY", -1.296376071e-015)
        cmds.setAttr("pinky_03_l.rotateX", 4.230075606e-014)
        cmds.setAttr("pinky_02_l.rotateZ", 54.79433274)
        cmds.setAttr("pinky_02_l.rotateY", -3.788711524e-014)
        cmds.setAttr("pinky_02_l.rotateX", 1.292100339e-014)
        cmds.setAttr("pinky_01_l.rotateZ", 53.17737673)
        cmds.setAttr("pinky_01_l.rotateY", -10.46408531)
        cmds.setAttr("pinky_01_l.rotateX", 10.56158491)
        cmds.setAttr("middle_03_l.rotateZ", 55.02709033)
        cmds.setAttr("middle_03_l.rotateY", -1.652397549e-014)
        cmds.setAttr("middle_03_l.rotateX", 2.297826517e-014)
        cmds.setAttr("middle_02_l.rotateZ", 59.79453692)
        cmds.setAttr("middle_02_l.rotateY", -6.951251345e-015)
        cmds.setAttr("middle_02_l.rotateX", 2.166752877e-014)
        cmds.setAttr("middle_01_l.rotateZ", 51.05024479)
        cmds.setAttr("middle_01_l.rotateY", -1.133072605e-014)
        cmds.setAttr("middle_01_l.rotateX", 5.819421144e-014)
        cmds.setAttr("index_03_l.rotateZ", 64.64121137)
        cmds.setAttr("index_03_l.rotateY", -2.480634643e-014)
        cmds.setAttr("index_03_l.rotateX", 1.077164418e-014)
        cmds.setAttr("index_02_l.rotateZ", 64.20477863)
        cmds.setAttr("index_02_l.rotateY", -2.481003482e-014)
        cmds.setAttr("index_02_l.rotateX", 5.153492507e-014)
        cmds.setAttr("index_01_l.rotateZ", 46.94060635)
        cmds.setAttr("index_01_l.rotateY", 3.692762633)
        cmds.setAttr("index_01_l.rotateX", -8.969261741)

        cmds.setAttr("thumb_03_r.rotateZ", 30.05735749)
        cmds.setAttr("thumb_03_r.rotateY", 4.575153365e-015)
        cmds.setAttr("thumb_03_r.rotateX", 2.807367528e-014)
        cmds.setAttr("thumb_02_r.rotateZ", 22.03618142)
        cmds.setAttr("thumb_02_r.rotateY", 5.675048056)
        cmds.setAttr("thumb_02_r.rotateX", 22.94177697)
        cmds.setAttr("thumb_01_r.rotateZ", -9.145300774)
        cmds.setAttr("thumb_01_r.rotateY", 15.64425403)
        cmds.setAttr("thumb_01_r.rotateX", 14.45073388)
        cmds.setAttr("ring_03_r.rotateZ", 50.02688615)
        cmds.setAttr("ring_03_r.rotateY", -4.767840667e-014)
        cmds.setAttr("ring_03_r.rotateX", 2.313919533e-014)
        cmds.setAttr("ring_02_r.rotateZ", 54.79433274)
        cmds.setAttr("ring_02_r.rotateY", -7.710913552e-015)
        cmds.setAttr("ring_02_r.rotateX", 2.923377018e-014)
        cmds.setAttr("ring_01_r.rotateZ", 54.76549897)
        cmds.setAttr("ring_01_r.rotateY", -1.525905275e-014)
        cmds.setAttr("ring_01_r.rotateX", 4.56459293e-014)
        cmds.setAttr("pinky_03_r.rotateZ", 50.02688615)
        cmds.setAttr("pinky_03_r.rotateY", -1.296376071e-015)
        cmds.setAttr("pinky_03_r.rotateX", 4.230075606e-014)
        cmds.setAttr("pinky_02_r.rotateZ", 54.79433274)
        cmds.setAttr("pinky_02_r.rotateY", -3.788711524e-014)
        cmds.setAttr("pinky_02_r.rotateX", 1.292100339e-014)
        cmds.setAttr("pinky_01_r.rotateZ", 53.17737673)
        cmds.setAttr("pinky_01_r.rotateY", -10.46408531)
        cmds.setAttr("pinky_01_r.rotateX", 10.56158491)
        cmds.setAttr("middle_03_r.rotateZ", 55.02709033)
        cmds.setAttr("middle_03_r.rotateY", -1.652397549e-014)
        cmds.setAttr("middle_03_r.rotateX", 2.297826517e-014)
        cmds.setAttr("middle_02_r.rotateZ", 59.79453692)
        cmds.setAttr("middle_02_r.rotateY", -6.951251345e-015)
        cmds.setAttr("middle_02_r.rotateX", 2.166752877e-014)
        cmds.setAttr("middle_01_r.rotateZ", 51.05024479)
        cmds.setAttr("middle_01_r.rotateY", -1.133072605e-014)
        cmds.setAttr("middle_01_r.rotateX", 5.819421144e-014)
        cmds.setAttr("index_03_r.rotateZ", 64.64121137)
        cmds.setAttr("index_03_r.rotateY", -2.480634643e-014)
        cmds.setAttr("index_03_r.rotateX", 1.077164418e-014)
        cmds.setAttr("index_02_r.rotateZ", 64.20477863)
        cmds.setAttr("index_02_r.rotateY", -2.481003482e-014)
        cmds.setAttr("index_02_r.rotateX", 5.153492507e-014)
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
        jointsToRemove = ["hip_pack_a", "hip_pack_b", "hip_pack_c", "hip_pack_d"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["trap_handle"]
        jointToTransferTo = "spine_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["sniper_can_arm_01", "sniper_can_arm_02", "sniper_can_arm_03", "pack_hose_out", "pack_hose_in"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shotgun", "shotgun_clip"]
        jointToTransferTo = "shotgun_base"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_l", "upperarm_twist_02_l", "arm_pouch_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upperarm_twist_01_r", "bot_uparm", "bot_uparm_pistonC_top", "upperarm_twist_02_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["lowerarm_twist_01_l", "lowerarm_twist_02_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["arm_hose_top", "bot_loarm", "bot_loarm_pistonB_top", "bot_loarm_pistonC_top", "bot_loarm_pistonD_top", "bot_loarm_twist", "bot_loarm_pistonC_bot", "bot_loarm_pistonD_bot", "bot_uparm_pistonC_bot", "lowerarm_twist_01_r", "arm_hose_bott", "wrist_cover"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["index_01_l", "index_02_l", "index_03_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "ring_01_l", "ring_02_l", "ring_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l"]
        jointToTransferTo = "hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["bot_hand", "gun", "gun_barrel", "gun_body_panel_l", "gun_body_panel_r", "gun_scope", "gun_stock_base", "gun_barrel_inner", "gun_stock_base_in_bott", "gun_stock_base_out_bott", "gun_stock_base_in_top", "gun_stock_base_out_top", "gun_stock_in_bott", "gun_stock_in_top", "gun_stock_bott", "gun_stock_panel_l", "gun_stock_panel_r", "bot_loarm_pistonB_bot", "index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "ring_01_r", "ring_02_r", "ring_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r"]
        jointToTransferTo = "hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["eyeball_l", "eyeball_r", "eyebrow_in_l", "eyebrow_in_r", "eyebrow_out_l", "eyebrow_out_r", "eyelid_bott_l", "eyelid_bott_r", "eyelid_top_l", "eyelid_top_r", "helmet_sight", "jaw", "mask"]
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

        jointsToRemove = ["ankle_plate_in_l", "ankle_plate_out_l", "calf_twist_01_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ankle_plate_in_r", "ankle_plate_out_r", "calf_twist_01_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ball_l", "heel_plate_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ball_r", "heel_plate_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("Price_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

PriceLOD4()


