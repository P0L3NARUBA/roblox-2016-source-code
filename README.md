![My *handmade* Roblox Logo](https://github.com/user-attachments/assets/ced623cd-6692-4759-8e46-e9453f5454fc)

<p align="center">
<img alt="GitHub Repo Size" src="https://img.shields.io/github/repo-size/P0L3NARUBA/roblox-2016-source-code">
<img alt="GitHub Release" src="https://img.shields.io/github/v/release/P0L3NARUBA/roblox-2016-source-code?color=violet">
<img alt="GitHub Last Commit" src="https://img.shields.io/github/last-commit/P0L3NARUBA/roblox-2016-source-code/pixel-lighting">
</p>

# Roblox 2016 Source Code
This is a branch of this source, modified to include a modern lighting engine equivalent to or better than modern Roblox.

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
- All master-branch features and additions.

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

## Tools Used
- [HxD](/Tools/HxD) = 2.5.0.0
- [ILSpy](/Tools/ILSpy) = 9.1

---

## Contributors / Credits
See **[CONTRIBUTORS.md](/CONTRIBUTORS.md)**

---

## Current Goals
### Rendering
- [x] HDR color buffer
- [ ] Proper tonemapping
   - [ ] Reinhard
   - [x] ACES
   - [ ] AgX
- [x] Local pixel lights
- [ ] Directional light shadow-mapping
   - [ ] Tinting
- [ ] Local light shadow-mapping
   - [ ] Tinting
- [ ] Glass material with real-time refraction
- [ ] PBR lighting model
   - [ ] Metalness texture workflow
   - [x] GGX specular model
   - [x] Oren-Nayar-Fujii diffuse model
   - [ ] Environment diffuse and specular
   - [ ] Clearcoat
   - [ ] Parallax occlusion mapping
   - [ ] Subsurface scattering
   - [ ] Automatic shader LODs
- [ ] Unified light instance
- [ ] Clustered light culling
- [ ] Voxel imitation mode
- [ ] Ambient Occlusion
- [ ] Advanced post-processing effects
   - [ ] Depth of Field
   - [ ] Blur
   - [ ] Color Correction
   - [ ] Lens Flare
   - [ ] Bloom
      - [ ] Dirt mask
   - [ ] Volumetric Lighting
      - [ ] Texture mask
   - [ ] Chromatic Abberation
   - [ ] Film Grain
   - [ ] Edge Detection
- [ ] Clouds
- [ ] HDR skybox support
- [ ] Grass
- [ ] Global illumination
- [ ] Reflections
   - [ ] Placeable environment maps
   - [ ] Screen-space
- [ ] Reference path tracer
- [ ] Exposure
   - [ ] Adaptive
- [ ] Highlights
- [ ] GPU Particles
   - [ ] Physics simulation
- [ ] Vulkan support

**Ones below will be updated when master branch commits are applied to this branch.**
### General
- [ ] Color3uint8
   - [ ] Color3.fromRGB()
- [ ] R15
- [x] :Connect() and :Wait()
- [ ] Fix keyboard shortcuts
   - [ ] Reset character keybind
   - [ ] Chat keybind
      - Works in Studio but not in client 
   - [ ] Windows key on WindowsClient
- [ ] Adding Cyrillic & Non-Latin language support
   - [ ] UTF/Unicode support
- [ ] New Lua version
- [ ] Improving profanity filter
- [x] New fonts
- [x] Support for newer mesh versions
- [ ] Dark Theme for Studio
- [x] Change the location of unrelated files inside **content\fonts** folder.
- [ ] Making bootstrappers functional as intended 
- [ ] Fixing broken in-game recorder
- [ ] 64-bit support 
- [x] Uploading Qt to the Github as issue-free

### Building all projects with Visual Studio 2022 **[46/60]**
   #### Main
   - [x] App
   - [x] App.BulletPhysics
   - [x] Base
   - [ ] CoreScriptConverter2
   - [x] CSG
   - [x] Log
   - [x] Network
   - [x] qtnribbon
   - [ ] RCCService
   - [x] RobloxStudio
   - [x] sgCore
   - [ ] WindowsClient

   #### 3rd Party / Contribs
   - [x] boost.static
   - [ ] Boost
      - Needs a newer Boost version.
   - [ ] cpp-netlib
   - [x] DSBaseClasses
   - [x] libcurl
      - OpenSSL libraries spits out unresolved external symbols so these should get updated too.
   - [ ] Qt
   - [ ] OpenSSL
   - [x] SDL2
   - [x] zlib
   - [ ] w3c-libwww

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
   - [x] PrepAllForUpload
   - [x] BootstrapperClient.PrepForUpload
   - [x] BootstrapperRccService.PrepForUpload
   - [x] BootstrapperQTStudio.PrepForUpload
   - [x] RobloxProxy.PrepForUpload

   #### Other
   - [x] IncludeChecker
   - [x] RbxTestHooks
   - [x] ScriptSigner
   - [x] Emcaster
   - [x] EmcasterTest
   - [x] EmReciever
   - [x] Roblox.Common
   - [x] Roblox.Common.Web
   - [x] Roblox.Configuration
   - [x] Roblox.Diagnostics
   - [x] Roblox.Grid.Arbiter.Common
   - [x] Roblox.Grid.Client
   - [x] Roblox.Grid.Common
   - [x] Roblox.Ssh
   - [x] Roblox.System
   - [x] Roblox.WebsiteSettings
   - [x] Roblox.RccServiceArbiter

### Being able to build on listed platforms **[1/3]**
- [x] Windows
- [ ] Android
- [ ] MacOS / Unix

## Current Issues
- The Roblox in-game video recorder is pixelated and has no game sound; just some cracking noises.
- Certain transparent renderables are either black or not rendered entirely.
- Tonemapping is only applied when an active ColorCorrection instance is present.
- Blur instance results in broken visuals.
- Undo and redo does not respect proper Color3 values on instances and will instead snap to the nearest BrickColor value.

---
