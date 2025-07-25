![My *handmade* Roblox Logo](https://github.com/user-attachments/assets/ced623cd-6692-4759-8e46-e9453f5454fc)

<p align="center">
<img alt="GitHub Repo Size" src="https://img.shields.io/github/repo-size/P0L3NARUBA/roblox-2016-source-code">
<img alt="GitHub Release" src="https://img.shields.io/github/v/release/P0L3NARUBA/roblox-2016-source-code?color=violet">
<img alt="GitHub Last Commit" src="https://img.shields.io/github/last-commit/P0L3NARUBA/roblox-2016-source-code/pixel-lighting">
</p>

# Roblox 2016 Source Code
This is a branch of this source, modified to include a modern lighting engine equivalent to or better than modern Roblox.
Only Studio is verified functioning and the one recommended to use.

**RENDERING PIPELINE REVAMP IN PROGRESS, THIS DOES NOT CURRENTLY FUNCTION.**

Supported graphics APIs are:
- DirectX 11

DirectX 9 and OpenGL is unsupported.

**To build from the source, refer to [BUILDING.md](/BUILDING.md)**<br>
   - Make sure to read them properly so you don't face any issues.

**Having any problems? You can get help at [our discord server](https://www.discord.gg/rVrYHdrbsp) or at the [issues page](https://github.com/P0L3NARUBA/roblox-2016-source-code/issues)**<br>

**Want to play the game in no time? Check out [Releases](https://github.com/P0L3NARUBA/roblox-2016-source-code/releases/)**<br>
**NOTE:** You may need [Rocknet](https://github.com/P0L3NARUBA/Rocknet/tree/main) to launch the game.

# Table of Contents
1. [Features / Additions](#-features--additions)
2. [Libraries Used](#-libraries-used)
3. [Tools Used](#-tools-used)
4. [Contributors / Credits](#%EF%B8%8F-contributors--credits)
5. [Current Goals](#-current-goals)
6. [Current Issues](#%EF%B8%8F-current-issues)

---

## Features / Additions
- A modern lighting engine, aimed to be greater than even Roblox's modern lighting engine.

## Libraries Used
- [Boost](/Contribs/boost_1_56_0) = 1.56.0
- [cpp-netlib](/Contribs/cpp-netlib-0.11.0-final) = 0.11.0-final
- [DSBaseClasses](/Contribs/DSBaseClasses) = *unknown*
- [OpenSSL](/Contribs/openssl) = 1.0.0c
- [Qt](/BUILDING_CONTRIBS.md) = 4.8.5
- [Roblox SDK](/Contribs/SDK) = *unknown*
- [SDL2](/Contribs/SDL2) = 2.0.4
- [VMProtectWin](/Contribs/VMProtectWin_2.13) = 2.13
- [w3c-libwww](/Contribs/w3c-libwww-5.4.0) = 5.4.0
- [curl](/Contribs/windows/x86/curl/curl-7.43.0) = 7.43.0
- [zlib](/Contribs/windows/x86/zlib/zlib-1.2.8) = 1.2.8
- [glsl-optimizer](/Rendering/ShaderCompiler/glsl-optimizer) = *unknown*
- [hlsl2glslfork](/Rendering/ShaderCompiler/hlsl2glslfork) = *unknown*
- [mojoshader](/Rendering/ShaderCompiler/mojoshader) = *unknown*
- [gSOAP](/RCCService/gSOAP/gsoap-2.7) = 2.7.10
- [RakNet](/Network/raknet) = 5 
- [Mesa](/RCCService/Mesa-7.8.1) = 7.8.1
- [TBB](/TBB_4_1) = 4.1

## Tools Used
- [HxD](https://mh-nexus.de/en/downloads.php?product=HxD20)
- [ILSpy](https://github.com/icsharpcode/ILSpy/releases)
- [rbxsigner](/Tools/rbxsigner) = *unknown*

---

## Contributors / Credits
See **[CONTRIBUTORS.md](/CONTRIBUTORS.md)**

---

## Current Goals
### Rendering
- [x] HDR color buffer
- [ ] Tonemapping
   - [ ] Legacy
   - [ ] Reinhard Simple
   - [x] ACES
   - [ ] [Khronos PBR Neutral](https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/README.md#pbr-neutral-specification)
   - [ ] AgX
- [x] Local pixel lights
- [ ] Directional light shadow-mapping
   - [ ] Cascades
   - [ ] Tinting
- [ ] Local light shadow-mapping
   - [ ] Tinting
- [ ] [Moment shadow map filtering](https://github.com/TheRealMJP/Shadows/blob/master/Shadows/MSM.hlsl)
- [ ] Glass material with real-time refraction
- [ ] PBR lighting model
   - [x] Metalness texture workflow
   - [x] GGX specular model
   - [x] [Oren-Nayar-Fujii diffuse model](https://mimosa-pudica.net/improved-oren-nayar.html)
   - [ ] Environment diffuse and specular
   - [ ] Clearcoat
   - [ ] Parallax occlusion mapping
   - [ ] Subsurface scattering
   - [ ] Automatic shader LODs
- [ ] Unified light instance
   - [ ] Point
   - [ ] Sphere
   - [ ] Line
   - [ ] Tube
   - [ ] Disk
   - [ ] Plane
- [ ] [IES light profiles](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-ies-light-profiles-in-unreal-engine)
- [ ] [Clustered Light Culling](https://www.aortiz.me/2018/12/21/CG.html)
- [ ] GPU Voxels
   - [ ] Cascades
   - [ ] Local lights
- [ ] Ambient Occlusion
   - [ ] [Screen-space (XeGTAO)](https://github.com/GameTechDev/XeGTAO)
   - [ ] [Distance fields](https://dev.epicgames.com/documentation/en-us/unreal-engine/distance-field-ambient-occlusion-in-unreal-engine)
- [ ] Advanced post-processing effects
   - [ ] Depth of Field
   - [ ] Blur
      - [x] Box
      - [ ] Gaussian
      - [ ] Directional
      - [ ] Radial
   - [ ] Motion Blur
   - [x] Color Correction
   - [ ] Lens Flare
   - [x] Bloom
      - [ ] Dirt mask
   - [ ] Volumetric Lighting
      - [ ] Texture mask
   - [ ] Posterize
   - [ ] Chromatic Abberation
   - [ ] Film Grain
   - [ ] Edge Detection
- [ ] Clouds
- [ ] Modern skybox system
   - [x] HDRI support
   - [x] Highlight reconstruction
   - [ ] Decoupled celestial bodies
   - [ ] Custom time-based color shifting
   - [ ] Rotation
- [ ] Dynamic Grass
- [ ] Global illumination
- [ ] Reflections
   - [ ] Placeable environment maps
      - [ ] Parallax correction
   - [ ] Screen-space
   - [ ] Planar
- [ ] Reference path tracer
- [ ] Exposure
   - [ ] Adaptive
- [ ] [Highlights](https://bgolus.medium.com/the-quest-for-very-wide-outlines-ba82ed442cd9)
   - [ ] Variable thickness
   - [ ] Anti-aliased
- [ ] Occlusion Culling
- [ ] GPU Particles
   - [ ] Physics simulation
- [ ] [Signed distance field generation](https://github.com/InteractiveComputerGraphics/TriangleMeshDistance)
- [ ] [Fog](https://iquilezles.org/articles/fog/)
   - [ ] Sun influence
   - [ ] Skybox
   - [ ] Height-based
- [ ] Revamped terrain
- [ ] Projected decals
- [ ] Material variants
   - [ ] [Alpha mode](https://create.roblox.com/docs/art/modeling/surface-appearance#alpha-modes)
      - [ ] Transparency
      - [ ] Overlay
      - [ ] Tinting
   - [ ] Resampling mode
      - [ ] Closest
      - [ ] Linear
      - [ ] Cubic
   - [ ] Tiling size
   - [ ] Tiling mode
      - [ ] Regular
      - [ ] Organic
      - [ ] Rot. Organic
   - [ ] Layering
- [ ] Rendering modes
   - [ ] Forward+
      - [ ] Early depth pass
   - [ ] Deferred
      - [ ] [MSAA support](https://diglib.eg.org/server/api/core/bitstreams/6839f5a2-c94c-4c18-9945-680663ccb097/content)
- [ ] Proper VR Support
- [ ] [New physics engine](https://graphics.cs.utah.edu/research/projects/avbd/)
- [ ] API support
   - [x] DirectX 11
   - [ ] Vulkan
   - [ ] OpenGL ES 3.1+

### General
- [x] Color3uint8
   - [x] Color3.fromRGB()
- [ ] R15
- [x] :Connect() and :Wait()
- [ ] Fix keyboard shortcuts
   - [ ] Reset character keybind
   - [ ] Chat keybind
   - [ ] Windows key on WindowsClient
- [x] New fonts
- [ ] Adding Cyrillic & Non-Latin language support
   - [ ] UTF/Unicode support
- [ ] New Lua version
- [ ] Improving profanity filter
- [x] New fonts
- [x] Support for newer mesh versions
- [ ] Dark Mode for Studio
- [x] Change the location of unrelated files inside **content\fonts** folder.
- [ ] Making bootstrappers functional as intended
- [ ] 64-bit support

### Projects that successfully build with Visual Studio 2022 **VERIFIED: [26/59]**
   #### Main
   - [x] App
   - [x] App.BulletPhysics
   - [x] Base
   - [x] CoreScriptConverter2
   - [x] CSG
   - [x] Log
   - [x] Network
   - [x] qtnribbon
   - [ ] RCCService
   - [x] RobloxStudio
   - [x] sgCore
   - [x] WindowsClient

   #### 3rd Party / Contribs
   - [x] boost.static
   - [ ] Boost
      - Needs a newer Boost version.
   - [ ] cpp-netlib
   - [x] DSBaseClasses
   - [x] libcurl
   - [ ] Qt
   - [ ] OpenSSL
   - [x] SDL2
   - [x] zlib
   - [ ] w3c-libwww
   - [ ] mesa

   #### gSOAP
   - [x] soapcpp2
   - [x] wsdl2h

   #### Rendering
   - [x] AppDraw
   - [x] GfxBase
   - [x] GfxCore
   - [x] GfxRender
   - [x] graphics3D
   - [x] LibOVR
   - [x] RbxG3D

   #### Shaders
   - [x] ShaderCompiler

   #### Installer
   - [ ] Bootstrapper
   - [ ] BootstrapperClient
   - [ ] BootstrapperQTStudio
   - [ ] RobloxProxy
   - [ ] PrepAllForUpload
   - [ ] BootstrapperClient.PrepForUpload
   - [ ] BootstrapperRccService.PrepForUpload
   - [ ] BootstrapperQTStudio.PrepForUpload
   - [ ] RobloxProxy.PrepForUpload

   #### Other
   - [ ] IncludeChecker
   - [ ] RbxTestHooks
   - [ ] ScriptSigner
   - [ ] Emcaster
   - [ ] EmcasterTest
   - [ ] EmReciever
   - [ ] Roblox.Common
   - [ ] Roblox.Common.Web
   - [ ] Roblox.Configuration
   - [ ] Roblox.Diagnostics
   - [ ] Roblox.Grid.Arbiter.Common
   - [ ] Roblox.Grid.Client
   - [ ] Roblox.Grid.Common
   - [ ] Roblox.Ssh
   - [ ] Roblox.System
   - [ ] Roblox.WebsiteSettings
   - [ ] Roblox.RccServiceArbiter

### Builds on listed platforms **VERIFIED: [1/3]**
- [x] Windows
- [ ] Android
- [ ] MacOS / Unix
- [ ] Linux

## Current Issues
- Outdoor Ambient does nothing.
- Occasional lights consisting of garbage data and getting past all legitimacy checks may result in buggy visuals.
- Outdoor environment map generation is broken, as well as lacking highlight reconstruction.
- Undo and redo does not respect proper Color3 values on instances and will instead snap to the nearest BrickColor value.

---
