import maya.cmds as cmds
import customMayaMenu as menu
import Tools.ART_ImportMultipleWeights as impw
import Tools.ART_TransferWeights as transfer
import Tools.ART_ExportLODtoFBX as export
reload(export)

################################################## HYPERBREACH #########################################################################
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



# HyperBreach LOD1 -------------------------------------------------------------------------------------------------------------
def HyperBreachLOD1():
    if saveResult == "Yes":

        cmds.file(save=True)

        meshes = ["backlauncher_l_geo_LOD1", "backlauncher_r_geo_LOD1", "backspine_geo_LOD1", "bicep_l_geo_LOD1", "bicep_l_hatch_geo_LOD1", "bicep_r_geo_LOD1", "bicep_r_hatch_geo_LOD1", "bodyback_geo_LOD1", "bodypanel_l_geo_LOD1", "bodyshell_geo_LOD1", "calfhatch_l_geo_LOD1", "calfhatch_r_geo_LOD1", "calfjet_l_geo_LOD1", "calfjet_r_geo_LOD1", "calfs_geo_LOD1", "clav_l_geo_LOD1", "clav_r_geo_LOD1", "controlarm_l1_geo_LOD1", "controlarm_l2_geo_LOD1", "controlarm_r1_geo_LOD1", "controlarm_r2_geo_LOD1", "controlgrip_l_geo_LOD1", "controlgrip_r_geo_LOD1", "foot_l_geo_LOD1", "foot_r_geo_LOD1", "footflap_l_geo_LOD1", "footflap_r_geo_LOD1", "grenade1_geo_LOD1", "grenade2_geo_LOD1", "grenade3_geo_LOD1", "grenadefinger_1a_geo_LOD1", "grenadefinger_1b_geo_LOD1", "grenadefinger_1c_geo_LOD1", "grenadefinger_2a_geo_LOD1", "grenadefinger_2b_geo_LOD1", "grenadefinger_2c_geo_LOD1", "grenadefinger_3a_geo_LOD1", "grenadefinger_3b_geo_LOD1", "grenadefinger_3c_geo_LOD1", "grenadelauncherr_geo_LOD1", "grenadepiston_geo_LOD1", "hipcenter_geo_LOD1", "hipplate_geo_LOD1", "hose_geo_LOD1", "jetarm_l_geo_LOD1", "jetarm_r_geo_LOD1", "jetfoot_l_geo_LOD1", "jetfoot_r_geo_LOD1", "leg_l_geo_LOD1", "leg_r_geo_LOD1", "lowerspine_geo_LOD1", "mine_geo_LOD1", "minebase_geo_LOD1", "minedeploy_geo_LOD1", "minepivot_geo_LOD1", "missle_geo_LOD1", "missledoor1_geo_LOD1", "missledoor2_geo_LOD1", "misslelatch1_geo_LOD1", "misslelatch2_geo_LOD1", "misslelatch3_geo_LOD1", "misslelauncher_geo_LOD1", "misslepiston_geo_LOD1", "pedal_l_geo_LOD1", "pedal_r_geo_LOD1", "pedalarm_l_geo_LOD1", "pedalarm_r_geo_LOD1", "seat_geo_LOD1", "seatbase_geo_LOD1", "seatpivot_geo_LOD1", "shoulder_l_geo_LOD1", "shoulder_r_geo_LOD1", "shoulderbone_l_geo_LOD1", "shoulderbone_r_geo_LOD1", "shoulderpad_l_geo_LOD1", "shoulderpad_r_geo_LOD1", "tailplate_geo_LOD1", "toe_l_geo_LOD1", "toe_r_geo_LOD1", "turbine_geo_LOD1", "Rider:cigar_geo_LOD1", "Rider:cuffs_geo_LOD1", "Rider:eye_balls_geo_LOD1", "Rider:eye_cover_geo_LOD1", "Rider:feet_geo_LOD1", "Rider:gun_barrel_geo_LOD1", "Rider:gun_clip_geo_LOD1", "Rider:gun_flaps_geo_LOD1", "Rider:gun_geo_LOD1", "Rider:hands_geo_LOD1", "Rider:head_geo_LOD1", "Rider:holster_geo_LOD1", "Rider:HyperBreach_Rabbit_Eyelashes Rider:HyperBreach_Rabbit_Hair Rider:insignia_geo_LOD1", "Rider:leggings_geo_LOD1", "Rider:legs_geo_LOD1", "Rider:lower_belt_geo_LOD1", "Rider:lower_teeth_geo_LOD1", "Rider:shoulders_geo_LOD1", "Rider:tongue_geo_LOD1", "Rider:torso_geo_LOD1", "Rider:upper_teeth_geo_LOD1"]
        
        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("rider_thumb_02_l.rotateZ", 22.03618142)
        cmds.setAttr("rider_thumb_02_l.rotateY", 5.675048056)
        cmds.setAttr("rider_thumb_02_l.rotateX", 22.94177697)
        cmds.setAttr("rider_ring_02_l.rotateZ", 54.79433274)
        cmds.setAttr("rider_pinky_02_l.rotateZ", 54.79433274)
        cmds.setAttr("rider_middle_02_l.rotateZ", 59.79453692)
        cmds.setAttr("rider_index_02_l.rotateZ", 64.20477863)

        cmds.setAttr("rider_thumb_02_r.rotateZ", 22.03618142)
        cmds.setAttr("rider_thumb_02_r.rotateY", 5.675048056)
        cmds.setAttr("rider_thumb_02_r.rotateX", 22.94177697)
        cmds.setAttr("rider_ring_02_r.rotateZ", 54.79433274)
        cmds.setAttr("rider_pinky_02_r.rotateZ", 54.79433274)
        cmds.setAttr("rider_middle_02_r.rotateZ", 59.79453692)
        cmds.setAttr("rider_index_02_r.rotateZ", 64.20477863)

       # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["lower_fin_end"]
        jointToTransferTo = "lower_fin"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["cockpit_foot_pedal_r"]
        jointToTransferTo = "cockpit_pedal_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["cockpit_foot_pedal_l"]
        jointToTransferTo = "cockpit_pedal_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["controlArm_r_03"]
        jointToTransferTo = "controlArm_r_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["controlArm_l_03"]
        jointToTransferTo = "controlArm_l_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["neck_01", "head"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hand_l"]
        jointToTransferTo = "lowerarm_l"
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

        jointsToRemove = ["ring_03_r"]
        jointToTransferTo = "ring_02_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["foot_spike_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["foot_spike_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_index_02_l"]
        jointToTransferTo = "rider_index_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_middle_02_l"]
        jointToTransferTo = "rider_middle_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_pinky_02_l"]
        jointToTransferTo = "rider_pinky_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_ring_02_l"]
        jointToTransferTo = "rider_ring_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_thumb_02_l"]
        jointToTransferTo = "rider_thumb_01_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_index_02_r"]
        jointToTransferTo = "rider_index_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_middle_02_r"]
        jointToTransferTo = "rider_middle_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_pinky_02_r"]
        jointToTransferTo = "rider_pinky_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_ring_02_r"]
        jointToTransferTo = "rider_ring_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_thumb_02_r"]
        jointToTransferTo = "rider_thumb_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_heel_l"]
        jointToTransferTo = "rider_calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_heel_r"]
        jointToTransferTo = "rider_calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("HyperBreach_LOD1.fbx", "LOD1")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD1 Exported Successfully!!!!!!!!!!!!!!!"

HyperBreachLOD1()

# HyperBreach LOD2 -------------------------------------------------------------------------------------------------------------
def HyperBreachLOD2():
    if saveResult == "Yes":

        cmds.file(save=True)
        
        meshes = ["backlauncher_l_geo_LOD2", "backlauncher_r_geo_LOD2", "backspine_geo_LOD2", "bicep_l_geo_LOD2", "bicep_l_hatch_geo_LOD2", "bicep_r_geo_LOD2", "bicep_r_hatch_geo_LOD2", "bodyback_geo_LOD2", "bodypanel_l_geo_LOD2", "bodyshell_geo_LOD2", "calfhatch_l_geo_LOD2", "calfhatch_r_geo_LOD2", "calfjet_l_geo_LOD2", "calfjet_r_geo_LOD2", "calfs_geo_LOD2", "clav_l_geo_LOD2", "clav_r_geo_LOD2", "controlarm_l1_geo_LOD2", "controlarm_l2_geo_LOD2", "controlarm_r1_geo_LOD2", "controlarm_r2_geo_LOD2", "controlgrip_l_geo_LOD2", "controlgrip_r_geo_LOD2", "foot_l_geo_LOD2", "foot_r_geo_LOD2", "footflap_l_geo_LOD2", "footflap_r_geo_LOD2", "grenade1_geo_LOD2", "grenade2_geo_LOD2", "grenade3_geo_LOD2", "grenadefinger_1a_geo_LOD2", "grenadefinger_1b_geo_LOD2", "grenadefinger_1c_geo_LOD2", "grenadefinger_2a_geo_LOD2", "grenadefinger_2b_geo_LOD2", "grenadefinger_2c_geo_LOD2", "grenadefinger_3a_geo_LOD2", "grenadefinger_3b_geo_LOD2", "grenadefinger_3c_geo_LOD2", "grenadelauncherr_geo_LOD2", "grenadepiston_geo_LOD2", "hipcenter_geo_LOD2", "hipplate_geo_LOD2", "hose_geo_LOD2", "jetarm_l_geo_LOD2", "jetarm_r_geo_LOD2", "jetfoot_l_geo_LOD2", "jetfoot_r_geo_LOD2", "leg_l_geo_LOD2", "leg_r_geo_LOD2", "lowerspine_geo_LOD2", "mine_geo_LOD2", "minebase_geo_LOD2", "minedeploy_geo_LOD2", "minepivot_geo_LOD2", "missle_geo_LOD2", "missledoor1_geo_LOD2", "missledoor2_geo_LOD2", "misslelatch1_geo_LOD2", "misslelatch2_geo_LOD2", "misslelatch3_geo_LOD2", "misslelauncher_geo_LOD2", "misslepiston_geo_LOD2", "pedal_l_geo_LOD2", "pedal_r_geo_LOD2", "pedalarm_l_geo_LOD2", "pedalarm_r_geo_LOD2", "seat_geo_LOD2", "seatbase_geo_LOD2", "seatpivot_geo_LOD2", "shoulder_l_geo_LOD2", "shoulder_r_geo_LOD2", "shoulderbone_l_geo_LOD2", "shoulderbone_r_geo_LOD2", "shoulderpad_l_geo_LOD2", "shoulderpad_r_geo_LOD2", "tailplate_geo_LOD2", "toe_l_geo_LOD2", "toe_r_geo_LOD2", "turbine_geo_LOD2", "Rider:cigar_geo_LOD2", "Rider:cuffs_geo_LOD2", "Rider:eye_balls_geo_LOD2", "Rider:eye_cover_geo_LOD2", "Rider:feet_geo_LOD2", "Rider:gun_barrel_geo_LOD2", "Rider:gun_clip_geo_LOD2", "Rider:gun_flaps_geo_LOD2", "Rider:gun_geo_LOD2", "Rider:hands_geo_LOD2", "Rider:head_geo_LOD2", "Rider:holster_geo_LOD2", "Rider:HyperBreach_Rabbit_Eyelashes Rider:HyperBreach_Rabbit_Hair Rider:insignia_geo_LOD2", "Rider:leggings_geo_LOD2", "Rider:legs_geo_LOD2", "Rider:lower_belt_geo_LOD2", "Rider:lower_teeth_geo_LOD2", "Rider:shoulders_geo_LOD2", "Rider:tongue_geo_LOD2", "Rider:torso_geo_LOD2", "Rider:upper_teeth_geo_LOD2"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("rider_thumb_02_l.rotateZ", 22.03618142)
        cmds.setAttr("rider_thumb_02_l.rotateY", 5.675048056)
        cmds.setAttr("rider_thumb_02_l.rotateX", 22.94177697)
        cmds.setAttr("rider_thumb_01_l.rotateZ", 4.168007795)
        cmds.setAttr("rider_thumb_01_l.rotateY", -1.675941069)
        cmds.setAttr("rider_thumb_01_l.rotateX", 9.067155995)
        cmds.setAttr("rider_ring_02_l.rotateZ", 54.79433274)
        cmds.setAttr("rider_ring_01_l.rotateZ", 54.76549897)
        cmds.setAttr("rider_pinky_02_l.rotateZ", 54.79433274)
        cmds.setAttr("rider_pinky_01_l.rotateZ", 53.17737673)
        cmds.setAttr("rider_middle_02_l.rotateZ", 59.79453692)
        cmds.setAttr("rider_middle_01_l.rotateZ", 51.05024479)
        cmds.setAttr("rider_index_02_l.rotateZ", 64.20477863)
        cmds.setAttr("rider_index_01_l.rotateZ", 46.94060635)
        cmds.setAttr("rider_index_01_l.rotateY", 3.692762633)
        cmds.setAttr("rider_index_01_l.rotateX", -8.969261741)

        cmds.setAttr("rider_thumb_02_r.rotateZ", 22.03618142)
        cmds.setAttr("rider_thumb_02_r.rotateY", 5.675048056)
        cmds.setAttr("rider_thumb_02_r.rotateX", 22.94177697)
        cmds.setAttr("rider_thumb_01_r.rotateZ", 4.168007795)
        cmds.setAttr("rider_thumb_01_r.rotateY", -1.675941069)
        cmds.setAttr("rider_thumb_01_r.rotateX", 9.067155995)
        cmds.setAttr("rider_ring_02_r.rotateZ", 54.79433274)
        cmds.setAttr("rider_ring_01_r.rotateZ", 54.76549897)
        cmds.setAttr("rider_pinky_02_r.rotateZ", 54.79433274)
        cmds.setAttr("rider_pinky_01_r.rotateZ", 53.17737673)
        cmds.setAttr("rider_middle_02_r.rotateZ", 59.79453692)
        cmds.setAttr("rider_middle_01_r.rotateZ", 51.05024479)
        cmds.setAttr("rider_index_02_r.rotateZ", 64.20477863)
        cmds.setAttr("rider_index_01_r.rotateZ", 46.94060635)
        cmds.setAttr("rider_index_01_r.rotateY", 3.692762633)
        cmds.setAttr("rider_index_01_r.rotateX", -8.969261741)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)

        jointsToDelete = []

         # remove unneeded joints
        jointsToRemove = ["lower_fin", "lower_fin_end"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["turbine_hinge", "turbine", "turbine_root", "cockpit_foot_pedal_r", "cockpit_pedal_r", "cockpit_foot_pedal_l", "cockpit_pedal_l"]
        jointToTransferTo = "spine_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["controlArm_r_03", "controlArm_r_02"]
        jointToTransferTo = "controlArm_r_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["controlArm_l_03", "controlArm_l_02"]
        jointToTransferTo = "controlArm_l_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rocket_l"]
        jointToTransferTo = "rocket_root_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rocket_r"]
        jointToTransferTo = "rocket_root_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upper_fin", "neck_01", "head"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tube_lats_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["arm_jet_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tube_arm_r", "arm_jet_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["bay_door_a", "bay_door_b", "hand_l", "recoil_l", "tube_arm_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rocket_arm_b"]
        jointToTransferTo = "rocket_arm"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hand_r", "recoil_r", "shell_1", "shell_2", "shell_3"]
        jointToTransferTo = "lowerarm_r"
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

        jointsToRemove = ["ring_02_r", "ring_03_r"]
        jointToTransferTo = "ring_01_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["leg_socket_l"]
        jointToTransferTo = "leg_socket_root_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["leg_socket_r"]
        jointToTransferTo = "leg_socket_root_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["leg_jet_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["leg_jet_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["foot_spike_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["foot_spike_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_index_01_l", "rider_index_02_l", "rider_middle_01_l", "rider_middle_02_l", "rider_pinky_01_l", "rider_pinky_02_l", "rider_ring_01_l", "rider_ring_02_l", "rider_thumb_01_l", "rider_thumb_02_l"]
        jointToTransferTo = "rider_hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_index_01_r", "rider_index_02_r", "rider_middle_01_r", "rider_middle_02_r", "rider_pinky_01_r", "rider_pinky_02_r", "rider_ring_01_r", "rider_ring_02_r", "rider_thumb_01_r", "rider_thumb_02_r"]
        jointToTransferTo = "rider_hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_earMain_l_01", "rider_earMain_l_02", "rider_earMain_l_03", "rider_earMain_r_01", "rider_earMain_r_02", "rider_earMain_r_03", "rider_eyeball_l", "rider_eyeball_r", "rider_eyelid_bott_l", "rider_eyelid_bott_r", "rider_eyelid_top_l", "rider_eyelid_top_r", "rider_jaw"]
        jointToTransferTo = "rider_head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_heel_l"]
        jointToTransferTo = "rider_calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_heel_r"]
        jointToTransferTo = "rider_calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_ball_l"]
        jointToTransferTo = "rider_foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_ball_r"]
        jointToTransferTo = "rider_foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("HyperBreach_LOD2.fbx", "LOD2")

        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD2 Exported Successfully!!!!!!!!!!!!!!!"

HyperBreachLOD2()

# HyperBreach LOD3 -------------------------------------------------------------------------------------------------------------
def HyperBreachLOD3():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["backlauncher_l_geo_LOD3", "backlauncher_r_geo_LOD3", "backspine_geo_LOD3", "bicep_l_geo_LOD3", "bicep_l_hatch_geo_LOD3", "bicep_r_geo_LOD3", "bicep_r_hatch_geo_LOD3", "bodyback_geo_LOD3", "bodypanel_l_geo_LOD3", "bodyshell_geo_LOD3", "calfhatch_l_geo_LOD3", "calfhatch_r_geo_LOD3", "calfjet_l_geo_LOD3", "calfjet_r_geo_LOD3", "calfs_geo_LOD3", "clav_l_geo_LOD3", "clav_r_geo_LOD3", "controlarm_l1_geo_LOD3", "controlarm_l2_geo_LOD3", "controlarm_r1_geo_LOD3", "controlarm_r2_geo_LOD3", "controlgrip_l_geo_LOD3", "controlgrip_r_geo_LOD3", "foot_l_geo_LOD3", "foot_r_geo_LOD3", "footflap_l_geo_LOD3", "footflap_r_geo_LOD3", "grenade1_geo_LOD3", "grenade2_geo_LOD3", "grenade3_geo_LOD3", "grenadefinger_1a_geo_LOD3", "grenadefinger_1b_geo_LOD3", "grenadefinger_1c_geo_LOD3", "grenadefinger_2a_geo_LOD3", "grenadefinger_2b_geo_LOD3", "grenadefinger_2c_geo_LOD3", "grenadefinger_3a_geo_LOD3", "grenadefinger_3b_geo_LOD3", "grenadefinger_3c_geo_LOD3", "grenadelauncherr_geo_LOD3", "grenadepiston_geo_LOD3", "hipcenter_geo_LOD3", "hipplate_geo_LOD3", "hose_geo_LOD3", "jetarm_l_geo_LOD3", "jetarm_r_geo_LOD3", "jetfoot_l_geo_LOD3", "jetfoot_r_geo_LOD3", "leg_l_geo_LOD3", "leg_r_geo_LOD3", "lowerspine_geo_LOD3", "mine_geo_LOD3", "minebase_geo_LOD3", "minedeploy_geo_LOD3", "minepivot_geo_LOD3", "missle_geo_LOD3", "missledoor1_geo_LOD3", "missledoor2_geo_LOD3", "misslelatch1_geo_LOD3", "misslelatch2_geo_LOD3", "misslelatch3_geo_LOD3", "misslelauncher_geo_LOD3", "misslepiston_geo_LOD3", "pedal_l_geo_LOD3", "pedal_r_geo_LOD3", "pedalarm_l_geo_LOD3", "pedalarm_r_geo_LOD3", "seat_geo_LOD3", "seatbase_geo_LOD3", "seatpivot_geo_LOD3", "shoulder_l_geo_LOD3", "shoulder_r_geo_LOD3", "shoulderbone_l_geo_LOD3", "shoulderbone_r_geo_LOD3", "shoulderpad_l_geo_LOD3", "shoulderpad_r_geo_LOD3", "tailplate_geo_LOD3", "toe_l_geo_LOD3", "toe_r_geo_LOD3", "turbine_geo_LOD3", "Rider:cigar_geo_LOD3", "Rider:cuffs_geo_LOD3", "Rider:eye_balls_geo_LOD3", "Rider:eye_cover_geo_LOD3", "Rider:feet_geo_LOD3", "Rider:gun_barrel_geo_LOD3", "Rider:gun_clip_geo_LOD3", "Rider:gun_flaps_geo_LOD3", "Rider:gun_geo_LOD3", "Rider:hands_geo_LOD3", "Rider:head_geo_LOD3", "Rider:holster_geo_LOD3", "Rider:HyperBreach_Rabbit_Eyelashes Rider:HyperBreach_Rabbit_Hair Rider:insignia_geo_LOD3", "Rider:leggings_geo_LOD3", "Rider:legs_geo_LOD3", "Rider:lower_belt_geo_LOD3", "Rider:lower_teeth_geo_LOD3", "Rider:shoulders_geo_LOD3", "Rider:tongue_geo_LOD3", "Rider:torso_geo_LOD3", "Rider:upper_teeth_geo_LOD3"]
    
        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("rider_thumb_02_l.rotateZ", 22.03618142)
        cmds.setAttr("rider_thumb_02_l.rotateY", 5.675048056)
        cmds.setAttr("rider_thumb_02_l.rotateX", 22.94177697)
        cmds.setAttr("rider_thumb_01_l.rotateZ", 4.168007795)
        cmds.setAttr("rider_thumb_01_l.rotateY", -1.675941069)
        cmds.setAttr("rider_thumb_01_l.rotateX", 9.067155995)
        cmds.setAttr("rider_ring_02_l.rotateZ", 54.79433274)
        cmds.setAttr("rider_ring_01_l.rotateZ", 54.76549897)
        cmds.setAttr("rider_pinky_02_l.rotateZ", 54.79433274)
        cmds.setAttr("rider_pinky_01_l.rotateZ", 53.17737673)
        cmds.setAttr("rider_middle_02_l.rotateZ", 59.79453692)
        cmds.setAttr("rider_middle_01_l.rotateZ", 51.05024479)
        cmds.setAttr("rider_index_02_l.rotateZ", 64.20477863)
        cmds.setAttr("rider_index_01_l.rotateZ", 46.94060635)
        cmds.setAttr("rider_index_01_l.rotateY", 3.692762633)
        cmds.setAttr("rider_index_01_l.rotateX", -8.969261741)

        cmds.setAttr("rider_thumb_02_r.rotateZ", 22.03618142)
        cmds.setAttr("rider_thumb_02_r.rotateY", 5.675048056)
        cmds.setAttr("rider_thumb_02_r.rotateX", 22.94177697)
        cmds.setAttr("rider_thumb_01_r.rotateZ", 4.168007795)
        cmds.setAttr("rider_thumb_01_r.rotateY", -1.675941069)
        cmds.setAttr("rider_thumb_01_r.rotateX", 9.067155995)
        cmds.setAttr("rider_ring_02_r.rotateZ", 54.79433274)
        cmds.setAttr("rider_ring_01_r.rotateZ", 54.76549897)
        cmds.setAttr("rider_pinky_02_r.rotateZ", 54.79433274)
        cmds.setAttr("rider_pinky_01_r.rotateZ", 53.17737673)
        cmds.setAttr("rider_middle_02_r.rotateZ", 59.79453692)
        cmds.setAttr("rider_middle_01_r.rotateZ", 51.05024479)
        cmds.setAttr("rider_index_02_r.rotateZ", 64.20477863)
        cmds.setAttr("rider_index_01_r.rotateZ", 46.94060635)
        cmds.setAttr("rider_index_01_r.rotateY", 3.692762633)
        cmds.setAttr("rider_index_01_r.rotateX", -8.969261741)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["hip_l", "hip_r", "lower_fin", "lower_fin_end"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["turbine_hinge", "turbine", "turbine_root", "controlArm_r_03", "controlArm_r_02", "controlArm_r_01", "controlArm_l_03", "controlArm_l_02", "controlArm_l_01", "cockpit_seat", "cockpit_seat_base", "cockpit_foot_pedal_r", "cockpit_pedal_r", "cockpit_foot_pedal_l", "cockpit_pedal_l"]
        jointToTransferTo = "spine_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rocket_transform", "rocket_root_l", "rocket_l", "rocket_root_r", "rocket_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["upper_fin", "shoulder_pad_r", "shoulder_pad_l", "neck_01", "head"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["cannon", "mine"]
        jointToTransferTo = "cannon_arm_2"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulder_mid_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tube_lats_r", "shoulder_mid_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["arm_jet_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tube_arm_r", "arm_jet_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["bay_door_a", "bay_door_b", "hand_l", "recoil_l", "tube_arm_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rocket_arm_b"]
        jointToTransferTo = "rocket_arm"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hand_r", "index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "ring_01_r", "ring_02_r", "ring_03_r", "recoil_r", "shell_1", "shell_2", "shell_3"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["leg_socket_root_l", "leg_socket_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["leg_socket_root_r", "leg_socket_r"]
        jointToTransferTo = "thigh_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_l", "leg_jet_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["leg_jet_r", "calf_twist_01_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ball_l", "foot_spike_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["foot_spike_r", "ball_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_index_01_l", "rider_index_02_l", "rider_middle_01_l", "rider_middle_02_l", "rider_pinky_01_l", "rider_pinky_02_l", "rider_ring_01_l", "rider_ring_02_l", "rider_thumb_01_l", "rider_thumb_02_l"]
        jointToTransferTo = "rider_hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_index_01_r", "rider_index_02_r", "rider_middle_01_r", "rider_middle_02_r", "rider_pinky_01_r", "rider_pinky_02_r", "rider_ring_01_r", "rider_ring_02_r", "rider_thumb_01_r", "rider_thumb_02_r"]
        jointToTransferTo = "rider_hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_earMain_l_01", "rider_earMain_l_02", "rider_earMain_l_03", "rider_earMain_r_01", "rider_earMain_r_02", "rider_earMain_r_03", "rider_eyeball_l", "rider_eyeball_r", "rider_eyelid_bott_l", "rider_eyelid_bott_r", "rider_eyelid_top_l", "rider_eyelid_top_r", "rider_jaw"]
        jointToTransferTo = "rider_head"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_heel_l"]
        jointToTransferTo = "rider_calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_heel_r"]
        jointToTransferTo = "rider_calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_ball_l"]
        jointToTransferTo = "rider_foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_ball_r"]
        jointToTransferTo = "rider_foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("HyperBreach_LOD3.fbx", "LOD3")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD3 Exported Successfully!!!!!!!!!!!!!!!"

HyperBreachLOD3()

# HyperBreach LOD4 -------------------------------------------------------------------------------------------------------------
def HyperBreachLOD4():
    if saveResult == "Yes":

        cmds.file(save=True)
    
        meshes = ["backlauncher_l_geo_LOD4", "backlauncher_r_geo_LOD4", "backspine_geo_LOD4", "bicep_l_geo_LOD4", "bicep_l_hatch_geo_LOD4", "bicep_r_geo_LOD4", "bicep_r_hatch_geo_LOD4", "bodyback_geo_LOD4", "bodypanel_l_geo_LOD4", "bodyshell_geo_LOD4", "calfhatch_l_geo_LOD4", "calfhatch_r_geo_LOD4", "calfjet_l_geo_LOD4", "calfjet_r_geo_LOD4", "calfs_geo_LOD4", "clav_l_geo_LOD4", "clav_r_geo_LOD4", "controlarm_l1_geo_LOD4", "controlarm_l2_geo_LOD4", "controlarm_r1_geo_LOD4", "controlarm_r2_geo_LOD4", "controlgrip_l_geo_LOD4", "controlgrip_r_geo_LOD4", "foot_l_geo_LOD4", "foot_r_geo_LOD4", "footflap_l_geo_LOD4", "footflap_r_geo_LOD4", "grenade1_geo_LOD4", "grenade2_geo_LOD4", "grenade3_geo_LOD4", "grenadefinger_1a_geo_LOD4", "grenadefinger_1b_geo_LOD4", "grenadefinger_1c_geo_LOD4", "grenadefinger_2a_geo_LOD4", "grenadefinger_2b_geo_LOD4", "grenadefinger_2c_geo_LOD4", "grenadefinger_3a_geo_LOD4", "grenadefinger_3b_geo_LOD4", "grenadefinger_3c_geo_LOD4", "grenadelauncherr_geo_LOD4", "grenadepiston_geo_LOD4", "hipcenter_geo_LOD4", "hipplate_geo_LOD4", "hose_geo_LOD4", "jetarm_l_geo_LOD4", "jetarm_r_geo_LOD4", "jetfoot_l_geo_LOD4", "jetfoot_r_geo_LOD4", "leg_l_geo_LOD4", "leg_r_geo_LOD4", "lowerspine_geo_LOD4", "mine_geo_LOD4", "minebase_geo_LOD4", "minedeploy_geo_LOD4", "minepivot_geo_LOD4", "missle_geo_LOD4", "missledoor1_geo_LOD4", "missledoor2_geo_LOD4", "misslelatch1_geo_LOD4", "misslelatch2_geo_LOD4", "misslelatch3_geo_LOD4", "misslelauncher_geo_LOD4", "misslepiston_geo_LOD4", "pedal_l_geo_LOD4", "pedal_r_geo_LOD4", "pedalarm_l_geo_LOD4", "pedalarm_r_geo_LOD4", "seat_geo_LOD4", "seatbase_geo_LOD4", "seatpivot_geo_LOD4", "shoulder_l_geo_LOD4", "shoulder_r_geo_LOD4", "shoulderbone_l_geo_LOD4", "shoulderbone_r_geo_LOD4", "shoulderpad_l_geo_LOD4", "shoulderpad_r_geo_LOD4", "tailplate_geo_LOD4", "toe_l_geo_LOD4", "toe_r_geo_LOD4", "turbine_geo_LOD4", "Rider:cigar_geo_LOD4", "Rider:cuffs_geo_LOD4", "Rider:eye_balls_geo_LOD4", "Rider:eye_cover_geo_LOD4", "Rider:feet_geo_LOD4", "Rider:gun_barrel_geo_LOD4", "Rider:gun_clip_geo_LOD4", "Rider:gun_flaps_geo_LOD4", "Rider:gun_geo_LOD4", "Rider:hands_geo_LOD4", "Rider:head_geo_LOD4", "Rider:holster_geo_LOD4", "Rider:HyperBreach_Rabbit_Eyelashes Rider:HyperBreach_Rabbit_Hair Rider:insignia_geo_LOD4", "Rider:leggings_geo_LOD4", "Rider:legs_geo_LOD4", "Rider:lower_belt_geo_LOD4", "Rider:lower_teeth_geo_LOD4", "Rider:shoulders_geo_LOD4", "Rider:tongue_geo_LOD4", "Rider:torso_geo_LOD4", "Rider:upper_teeth_geo_LOD4"]

        # Export a skin weights file for each mesh we plan to operate on.
        cmds.select(meshes)
        menu.exportSelectedSkinWeights(True)

        # Set the pose if needed
        cmds.setAttr("rider_thumb_02_l.rotateZ", 22.03618142)
        cmds.setAttr("rider_thumb_02_l.rotateY", 5.675048056)
        cmds.setAttr("rider_thumb_02_l.rotateX", 22.94177697)
        cmds.setAttr("rider_thumb_01_l.rotateZ", 4.168007795)
        cmds.setAttr("rider_thumb_01_l.rotateY", -1.675941069)
        cmds.setAttr("rider_thumb_01_l.rotateX", 9.067155995)
        cmds.setAttr("rider_ring_02_l.rotateZ", 54.79433274)
        cmds.setAttr("rider_ring_01_l.rotateZ", 54.76549897)
        cmds.setAttr("rider_pinky_02_l.rotateZ", 54.79433274)
        cmds.setAttr("rider_pinky_01_l.rotateZ", 53.17737673)
        cmds.setAttr("rider_middle_02_l.rotateZ", 59.79453692)
        cmds.setAttr("rider_middle_01_l.rotateZ", 51.05024479)
        cmds.setAttr("rider_index_02_l.rotateZ", 64.20477863)
        cmds.setAttr("rider_index_01_l.rotateZ", 46.94060635)
        cmds.setAttr("rider_index_01_l.rotateY", 3.692762633)
        cmds.setAttr("rider_index_01_l.rotateX", -8.969261741)

        cmds.setAttr("rider_thumb_02_r.rotateZ", 22.03618142)
        cmds.setAttr("rider_thumb_02_r.rotateY", 5.675048056)
        cmds.setAttr("rider_thumb_02_r.rotateX", 22.94177697)
        cmds.setAttr("rider_thumb_01_r.rotateZ", 4.168007795)
        cmds.setAttr("rider_thumb_01_r.rotateY", -1.675941069)
        cmds.setAttr("rider_thumb_01_r.rotateX", 9.067155995)
        cmds.setAttr("rider_ring_02_r.rotateZ", 54.79433274)
        cmds.setAttr("rider_ring_01_r.rotateZ", 54.76549897)
        cmds.setAttr("rider_pinky_02_r.rotateZ", 54.79433274)
        cmds.setAttr("rider_pinky_01_r.rotateZ", 53.17737673)
        cmds.setAttr("rider_middle_02_r.rotateZ", 59.79453692)
        cmds.setAttr("rider_middle_01_r.rotateZ", 51.05024479)
        cmds.setAttr("rider_index_02_r.rotateZ", 64.20477863)
        cmds.setAttr("rider_index_01_r.rotateZ", 46.94060635)
        cmds.setAttr("rider_index_01_r.rotateY", 3.692762633)
        cmds.setAttr("rider_index_01_r.rotateX", -8.969261741)

        # Delete the history on the meshes so that they get baked down into their posed positions
        cmds.delete(meshes, ch=True)

        # Move the skeleton back to its bindpose (This script REQUIRES the bind pose to be the pose that we want to export the skeleton to.  This is the case for all known characters created with A.R.T.)
        cmds.dagPose("root", bp=True, r=True) 

        # Re-import the skinning we exported before onto the mesh so thats its bound in its posed position.
        cmds.select(meshes)
        impw.importMultipleSkinWeights("", True)
        
        jointsToDelete = []

        # remove unneeded joints
        jointsToRemove = ["hip_l", "hip_r", "lower_fin", "lower_fin_end"]
        jointToTransferTo = "pelvis"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["turbine_hinge", "turbine", "turbine_root", "controlArm_r_03", "controlArm_r_02", "controlArm_r_01", "controlArm_l_03", "controlArm_l_02", "controlArm_l_01", "cockpit_seat", "cockpit_seat_base", "cockpit_foot_pedal_r", "cockpit_pedal_r", "cockpit_foot_pedal_l", "cockpit_pedal_l"]
        jointToTransferTo = "spine_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rocket_transform", "rocket_root_l", "rocket_l", "rocket_root_r", "rocket_r"]
        jointToTransferTo = "spine_02"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["cannon_root", "cannon_arm_1", "cannon_arm_2", "cannon", "mine", "upper_fin", "shoulder_pad_r", "shoulder_pad_l", "neck_01", "head"]
        jointToTransferTo = "spine_03"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["shoulder_mid_l"]
        jointToTransferTo = "clavicle_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tube_lats_r", "shoulder_mid_r"]
        jointToTransferTo = "clavicle_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["arm_jet_l"]
        jointToTransferTo = "upperarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["tube_arm_r", "arm_jet_r"]
        jointToTransferTo = "upperarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["bay_door_a", "bay_door_b", "hand_l", "recoil_l", "rocket_arm_root", "rocket_arm", "rocket_arm_b", "rocket", "tube_arm_l"]
        jointToTransferTo = "lowerarm_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["hand_r", "index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "ring_01_r", "ring_02_r", "ring_03_r", "recoil_r", "shell_1", "shell_2", "shell_3"]
        jointToTransferTo = "lowerarm_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["leg_socket_root_l", "leg_socket_l"]
        jointToTransferTo = "thigh_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["leg_socket_root_r", "leg_socket_r"]
        jointToTransferTo = "thigh_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["calf_twist_01_l", "leg_jet_l"]
        jointToTransferTo = "calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["leg_jet_r", "calf_twist_01_r"]
        jointToTransferTo = "calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["ball_l", "foot_spike_l"]
        jointToTransferTo = "foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["foot_spike_r", "ball_r"]
        jointToTransferTo = "foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_index_01_l", "rider_index_02_l", "rider_middle_01_l", "rider_middle_02_l", "rider_pinky_01_l", "rider_pinky_02_l", "rider_ring_01_l", "rider_ring_02_l", "rider_thumb_01_l", "rider_thumb_02_l"]
        jointToTransferTo = "rider_hand_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_index_01_r", "rider_index_02_r", "rider_middle_01_r", "rider_middle_02_r", "rider_pinky_01_r", "rider_pinky_02_r", "rider_ring_01_r", "rider_ring_02_r", "rider_thumb_01_r", "rider_thumb_02_r"]
        jointToTransferTo = "rider_hand_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_neck_02", "rider_head", "rider_earMain_l_01", "rider_earMain_l_02", "rider_earMain_l_03", "rider_earMain_r_01", "rider_earMain_r_02", "rider_earMain_r_03", "rider_eyeball_l", "rider_eyeball_r", "rider_eyelid_bott_l", "rider_eyelid_bott_r", "rider_eyelid_top_l", "rider_eyelid_top_r", "rider_jaw"]
        jointToTransferTo = "rider_neck_01"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_heel_l"]
        jointToTransferTo = "rider_calf_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_heel_r"]
        jointToTransferTo = "rider_calf_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_ball_l"]
        jointToTransferTo = "rider_foot_l"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)

        jointsToRemove = ["rider_ball_r"]
        jointToTransferTo = "rider_foot_r"
        transfer.LOD_transferWeights(meshes, jointsToRemove, jointToTransferTo)
        jointsToDelete.extend(jointsToRemove)
        
        # Delete the joints we no longer want.
        for one in jointsToDelete:
            if cmds.objExists(one):
                cmds.delete(one)
                
        # Export the fbx file.
        export.exportLOD("HyperBreach_LOD4.fbx", "LOD4")
        
        # Re-open the file without saving.
        fullPath = cmds.file(q = True, sceneName = True)
        cmds.file(fullPath, open=True, f=True)

        print "LOD4 Exported Successfully!!!!!!!!!!!!!!!"

HyperBreachLOD4()


