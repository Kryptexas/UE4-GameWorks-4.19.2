
set uebp_testfail=0

rem editor tests ***************************************************************

call runuattest.bat BuildCookRun -build -run -editortest -unattended -nullrhi -NoP4
call runuattest.bat BuildCookRun -build -run -editortest -unattended -nullrhi -NoP4 -project=QAGame
call runuattest.bat BuildCookRun -build -run -editortest -unattended -nullrhi -NoP4 -project=FortniteGame
call runuattest.bat BuildCookRun -build -run -editortest -unattended -nullrhi -NoP4 -project=OrionGame
call runuattest.bat BuildCookRun -build -run -editortest -unattended -nullrhi -NoP4 -project=Soul
call runuattest.bat BuildCookRun -build -run -editortest -unattended -nullrhi -NoP4 -project=ShooterGame
call runuattest.bat BuildCookRun -build -run -editortest -unattended -nullrhi -NoP4 -project=Shadow
call runuattest.bat BuildCookRun -build -run -editortest -unattended -nullrhi -NoP4 -project=StrategyGame
call runuattest.bat BuildCookRun -build -run -editortest -unattended -nullrhi -NoP4 -project=VehicleGame


rem buildcookrun tests (PC, uncooked) ***************************************************************

call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=Samples\Showcases\Infiltrator\Infiltrator.uproject -platform=win64 -map=/Game/Maps/TestMaps/Visuals/VIS_ArtDemo_P
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=Samples\Showcases\Elemental\Elemental.uproject -platform=win64 -map=/Game/Maps/GDC12_Ice/Elemental.umap

call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QAEntry
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-AmbientOcclusion
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-AntiAliasing
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Blueprints
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-ClickHUD
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-CullDistance
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Decals
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Destructible
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-DoF
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Foliage
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Landscape
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-LightsMovable
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-LightsStatic
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-LightsStationary
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Materials
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Matinee
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-MotionBlur
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-NavMesh
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Particles
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Physics
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-PostProcessing
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundAttenuation
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundCrossFade
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundDoppler
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundLoop
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundMix
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundMultichannel
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundReverb
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundReverbPriority
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-StreamLevels/QA-StreamLevels_P
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Surfaces
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/TestMaps/DM-Deck
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/TestMaps/tracetest
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/RenderTestMap

call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=Soul -platform=win64 -map=/Game/Maps/LV_Soul_Levels_P

call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=Shadow -platform=win64 -map=/Game/Maps/ShadowFullGameMap

call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=OrionGame -platform=win64 -server -serverplatform=win64 -map=/Game/PROTO/Gametypes/TDM/Maps/TDM_Arena

call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=ShooterGame -platform=win64 -server -serverplatform=win64 -map=/Game/Maps/Sanctuary
call runuattest.bat BuildCookRun -skipbuild -run -unattended -nullrhi -NoP4 -project=ShooterGame -platform=win64 -server -serverplatform=win64 -map=/Game/Maps/Highrise

rem buildcookrun tests (PC, cooked) ***************************************************************

call runuattest.bat BuildCookRun -build -cook -stage -pak -run -unattended -nullrhi -NoP4 -project=Samples\Showcases\Infiltrator\Infiltrator.uproject -platform=win64 -map=/Game/Maps/TestMaps/Visuals/VIS_ArtDemo_P
call runuattest.bat BuildCookRun -build -cook -stage -pak -run -unattended -nullrhi -NoP4 -project=Samples\Showcases\Elemental\Elemental.uproject -platform=win64 -map=/Game/Maps/GDC12_Ice/Elemental.umap

rem call runuattest.bat BuildCookRun -skipbuild -cook -unattended -nullrhi -NoP4 -project=QAGame -platform=win64+xboxone+ps4+ios -allmaps
call runuattest.bat BuildCookRun -skipbuild -cook -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -allmaps
call runuattest.bat BuildCookRun -skipbuild -cook -unattended -nullrhi -NoP4 -project=QAGame -platform=xboxone -allmaps
call runuattest.bat BuildCookRun -skipbuild -cook -unattended -nullrhi -NoP4 -project=QAGame -platform=ps4 -allmaps
call runuattest.bat BuildCookRun -skipbuild -cook -unattended -nullrhi -NoP4 -project=QAGame -platform=ios -allmaps

call runuattest.bat BuildCookRun -build -skipcook -stage -pak -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QAEntry
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-AmbientOcclusion
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-AntiAliasing
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Blueprints
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-ClickHUD
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-CullDistance
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Decals
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Destructible
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-DoF
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Foliage
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Landscape
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-LightsMovable
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-LightsStatic
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-LightsStationary
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Materials
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Matinee
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-MotionBlur
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-NavMesh
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Particles
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Physics
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-PostProcessing
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundAttenuation
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundCrossFade
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundDoppler
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundLoop
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundMix
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundMultichannel
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundReverb
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-SoundReverbPriority
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-StreamLevels/QA-StreamLevels_P
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Surfaces
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/TestMaps/DM-Deck
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/TestMaps/tracetest
call runuattest.bat BuildCookRun -skipbuild -skipcook -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/RenderTestMap

call runuattest.bat BuildCookRun -build -cook -stage -pak -run -unattended -nullrhi -NoP4 -project=Soul -platform=win64 -map=/Game/Maps/LV_Soul_Levels_P

call runuattest.bat BuildCookRun -build -cook -stage -pak -run -unattended -nullrhi -NoP4 -project=Shadow -platform=win64 -map=/Game/Maps/ShadowFullGameMap

call runuattest.bat BuildCookRun -skipbuild -cook -unattended -nullrhi -NoP4 -project=OrionGame -platform=win32+xboxone+ps4 -server -serverplatform=win64 -allmaps
call runuattest.bat BuildCookRun -build -skipcook -stage -pak -run -unattended -nullrhi -NoP4 -project=OrionGame -platform=win32 -server -serverplatform=win64 -map=/Game/PROTO/Gametypes/TDM/Maps/TDM_Arena

call runuattest.bat BuildCookRun -skipbuild -cook -unattended -nullrhi -NoP4 -project=ShooterGame -platform=win32+xboxone+ps4 -server -serverplatform=win64 -allmaps
call runuattest.bat BuildCookRun -build -skipcook -stage -pak -run -unattended -nullrhi -NoP4 -project=ShooterGame -platform=win32 -server -serverplatform=win64 -map=/Game/Maps/Sanctuary
call runuattest.bat BuildCookRun -skipbuild -skipcook -stage -pak -run -unattended -nullrhi -NoP4 -project=ShooterGame -platform=win32 -server -serverplatform=win64 -map=/Game/Maps/Highrise


rem buildcookrun tests (PC, cook on the fly) ***************************************************************

call runuattest.bat BuildCookRun -build -cookonthefly -stage -run -unattended -nullrhi -NoP4 -project=Samples\Showcases\Infiltrator\Infiltrator.uproject -platform=win64 -map=/Game/Maps/TestMaps/Visuals/VIS_ArtDemo_P
call runuattest.bat BuildCookRun -skipbuild -cookonthefly -stage -run -unattended -nullrhi -NoP4 -project=Samples\Showcases\Elemental\Elemental.uproject -platform=win64 -map=/Game/Maps/GDC12_Ice/Elemental.umap

call runuattest.bat BuildCookRun -skipbuild -cookonthefly -stage -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QAEntry
call runuattest.bat BuildCookRun -skipbuild -cookonthefly -stage -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/QA/QA-Physics
call runuattest.bat BuildCookRun -skipbuild -cookonthefly -stage -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/TestMaps/DM-Deck
call runuattest.bat BuildCookRun -skipbuild -cookonthefly -stage -run -unattended -nullrhi -NoP4 -project=QAGame -platform=win64 -map=/Game/Maps/RenderTestMap

call runuattest.bat BuildCookRun -skipbuild -cookonthefly -stage -run -unattended -nullrhi -NoP4 -project=Soul -platform=win64 -map=/Game/Maps/LV_Soul_Levels_P

call runuattest.bat BuildCookRun -skipbuild -cookonthefly -stage -run -unattended -nullrhi -NoP4 -project=Shadow -platform=win64 -map=/Game/Maps/ShadowFullGameMap


if "%uebp_testfail%" == "1" goto fail

ECHO ****************** All tests Succeeded
exit /B 0

:fail
ECHO ****************** A test failed
exit /B 1



