Unreal Engine with VXGI and HBAO+
=================================

**Note**: for up-to-date information on Unreal Engine in general, please refer to [README by Epic Games](https://github.com/EpicGames/UnrealEngine/blob/release/README.md).

Introduction
------------

VXGI stands for [Voxel Global Illumination](http://www.geforce.com/hardware/technology/vxgi), and it's an advanced rendering technique for real-time indirect illumination.

Global illumination (GI) is a way of computing lighting in the scene that includes indirect illumination, i.e. simulating objects that are lit by other objects as well as ideal light sources. Adding GI to the scene greatly improves the realism of the rendered images. Modern real-time rendering engines simulate indirect illumination using different approaches, which include precomputed light maps (offline GI), local light sources placed by artists, and simple ambient light.

HBAO+ stands for [Horizon-Based Ambient Occlusion Plus](http://www.geforce.com/hardware/technology/hbao-plus), and it's a fast and relatively stable screen-space ambient occlusion solution.

How to Build
------------

First, you need to download the source code. For beginners, it's easier to download a .zip file (look for the "Download ZIP" button on top of this page). Alternatively, you can use a Git client and clone the repository from GitHub onto your machine, then switch to the right branch - this way, keeping your copy up to date will be much easier. 

When you have the source code on your local hard drive, follow these steps to build it:

1.  Run Setup.bat.

2.	Run GenerateProjectFiles.bat
	
3.	Open UE4.sln, build the UE4 project.

4.	Run UE4 Editor.

How to Use
----------

Please see the [overview document](UE4_VXGI_Overview.pdf).

FAQ
---

**Q:** Is VXGI a plug-in for Unreal Engine? Can it be combined with other GameWorks techs, such as WaveWorks or HairWorks?
**A:** No, it's a separate branch of the engine. The UE plug-in interface is very limiting, and complicated technologies like VXGI cannot work through it. In order to combine several GameWorks technologies, you have to merge the corresponding branches. Alternatively, you can use a third-party merged branch.

**Q:** What are the minimum and recommended PC system requirements to run VXGI?
**A:** Minimum: a 64-bit Windows 7 system with any DirectX 11 class GPU. Recommended: a fast 4+ core processor (more is better because VXGI needs to compile a few heavy shaders for every material), 16 GB of system memory, and an NVIDIA GeForce GTX 9xx series GPU (GM20x or newer architecture).

**Q:** Do I have to build the engine from source to use VXGI, or there is a binary distribution available somewhere?
**A:** Currently there are no binary distributions, so you have to build it. It's not that hard, all you need is a free edition of Microsoft Visual Studio 2015 or 2017.

**Q:** I loaded a map but there is no indirect lighting.
**A:** Please make sure that...

- Console variable r.VXGI.DiffuseTracingEnable is set to 1
- Console variable r.VXGI.AmbientOcclusionMode is set to 0
- Directly lit or emissive materials have "Used with VXGI Voxelization" box checked
- Direct lights are Movable and have "VXGI Indirect Lighting" box checked
- There is an active PostProcessVolume and the camera is inside it (or it's unbounded)
- In the PostProcessVolume, the "Settings/VXGI Diffuse/Enable Diffuse Tracing" box is checked

It is also useful to switch the View mode to "VXGI Opacity Voxels" or "VXGI Emittance Voxels" to make sure that the objects you need are represented as voxels and emit (or reflect) light.

**Q:** Shadows from area lights are way too soft or missing.
**A:** Make sure that the voxels are not too large, ideally they should be comparable in size to the features that you'd like too see in shadows. Use r.VXGI.VoxelSize to adjust that.

**Q:** Some object doesn't cast a shadow or the shadow is not dark enough.
**A:** Make sure that the shadow casting object is represented with opacity voxels at all, and if not, check "Used with VXGI Voxelization" on its material. If the shadow is too weak, increase the "Opacity Scale" parameter in the VXGI group of material properties.

**Q:** Are area lights just static meshes? How does the Plane mesh and its material relate to the area light properties?
**A:** Yes, area lights are a subtype of static meshes. They add some properties like intensity or texture, and those properties are picked up by VXGI. Plane position, orientation, and scale are also used by VXGI, but nothing else is. So the material used to render the plane into viewports is entirely separate from the light characteristics, and has to be edited separately to match. Alternatively, you can hide the plane by unchecking its Visible property, and it will still emit light. Also note that if the Plane's material has an emissive component, and VXGI diffuse or specular tracing is enabled, by default the plane will be voxelized and will also emit light through the voxels. It's recommended to disable voxelization of area light planes completely, or to make the Emissive output of their material dependent on the VXGI Voxelization material input (i.e. zero it when voxelization is performed).

**Q:** If I only want to use area lights and/or VXAO, is there a way to improve performance?
**A:** Yes, in this case, set r.VXGI.AmbientOcclusionMode to 1. In that mode, VXGI will only process opacity voxels and not emittance, which makes it significantly faster.

**Q:** There was a way to add ambient light to diffuse tracing results before, where did that go?
**A:** VXGI 2 always produces a separate ambient occlusion channel and no longer has an option to add that ambient light. You can use the VXAO / SSAO channel in combination with a Sky Light or Ambient Cubemap feature in the Post Process Volume to add ambient lighting.

**Q:** There are no specular reflections on translucent objects, how do I add them?
**A:** You need to modify the translucent material and make it trace specular cones. See [this forum post](https://forums.unrealengine.com/showthread.php?53735-NVIDIA-GameWorks-Integration&p=423841&highlight=vxgi#post423841) for an example.

**Q:** Can specular reflections be less blurry?
**A:** Usually yes, but there is a limit. The quality of reflections is determined by the size of voxels representing the reflected object(s), so you need to reduce that size. There are several ways to do that:

- Place a "VXGI Anchor" actor near the reflected objects. VXGI's scene representation has a region where it is most detailed, and this actor controls the location of that region.
- Reduce r.VXGI.VoxelSize, which will make all voxels smaller, but also obviously reduce the range of VXGI effects.
- Increase r.VXGI.MapSize{X,Y,Z}, but there are only 3 options for these parameters: 64, 128 and 256, and the latter is extremely expensive.

**Q:** Is it possible to pre-compute lighting with VXGI to use on low-end PCs or mobile devices?
**A:** No, as VXGI was designed as a fully dynamic solution. It is theoretically possible to use VXGI cone tracing to bake light maps, but such feature is not implemented, and it doesn't add enough value compared to traditional light map solutions like Lightmass: the only advantage is that baking will be faster.

**Q:** Which graphics APIs does VXGI support?
**A:** It supports Direct3D 11 and 12.

**Q:** Can I use VXGI in my own rendering engine, without Unreal Engine?
**A:** Yes. The SDK package is available on [NVIDIA GameWorks Developer website](https://developer.nvidia.com/vxgi).


Tech Support
------------

This branch of UE4 is primarily discussed on the Unreal Engine forums: [NVIDIA GameWorks Integration](https://forums.unrealengine.com/showthread.php?53735-NVIDIA-GameWorks-Integration). That forum thread contains many questions and answers, and some NVIDIA engineers also sometimes participate in the discussion. You can also post an issue on this repository on GitHub.

Additional Resources
--------------------

- [Interactive Indirect Illumination Using Voxel Cone Tracing](http://maverick.inria.fr/Publications/2011/CNSGE11b/GIVoxels-pg2011-authors.pdf) - the original paper on voxel cone tracing.
- [NVIDIA VXGI: Dynamic Global Illumination for Games](http://on-demand.gputechconf.com/gtc/2015/presentation/S5670-Alexey-Panteleev.pdf) - a presentation about VXGI basics and its use in UE4.
- [Practical Real-Time Voxel-Based Global Illumination for Current GPUs](http://on-demand.gputechconf.com/gtc/2014/presentations/S4552-rt-voxel-based-global-illumination-gpus.pdf) - a technical presentation about VXGI while it was still work-in-progress.
- [GDC 2018 Vault](http://www.gdcvault.com/browse/gdc-18) - look for a talk entitled "Advances in Real-Time Voxel-Based GI".

Licensing
---------

This branch of UE4 is covered by the general [Unreal Engine End User License Agreement](LICENSE.pdf). There is no additional registration or fees associated with the use of VXGI or HBAO+ within Unreal Engine.

