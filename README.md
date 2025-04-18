# Roblox 2016 Source Code

This source originates from robloxsrc.zip that was spinning around but its rare to find these days.<br>
After a long effort, this repository has been brought to you on github with handful of changes!<br>

**To see how you can build from the source, refer to [BUILDING.md](/BUILDING.md)**<br>
**Having any problems? you can get help at [our discord server](https://www.discord.gg/rVrYHdrbsp)**<br>

# Table of Contents
1. [Features](#features--additions)
2. [Libraries Used](#-libraries-used)
3. [Tools Used](#-tools-used)
4. [Contributors](#%EF%B8%8F-contributors)

---

## Features / Additions
- Fixed a lot of issues that breaks the compilation and the codebase
  - I messed with a lot of things to fix all the projects though, i should get props for that.
- ColorProperty, from.RGB, from.HSV and from.Hex implementation
- [Asset Proxy and the other server-side stuff](https://www.github.com/P0L3NARUBA/rocknet-rblx)
- Changed Splash Screen and Copyright Date(s)
- Reverse Engineered some libraries and executables to make them editable
- Cleaned up the whole source to make things easier and not complicated
- All the necessary libraries are included in the repository.
   - Except **Qt**, [see here to download it](/BUILDING_CONTRIBS.md)

## Goals
- Add more helpful documentation.
- Backport Hitius and Graphictoria Features.
- Make it able to build all the projects in Visual Studio 2022
  - [ ] App
  - [ ] App.BulletPhysics
  - [ ] Base
  - [ ] CoreScriptConverter2
  - [ ] CSG
  - [ ] Log
  - [ ] Network
  - [ ] qtnribbon
    - It gives "Designtime build failed for project" error, seems like it has an easy fix though.
  - [ ] RCCService
  - [ ] RobloxStudio
  - [ ] WindowsClient
   - [ ] AppDraw
   - [ ] GfxBase
   - [ ] GfxCore
   - [ ] GfxRender
   - [ ] graphics3D
   - [ ] LibOVR
   - [ ] RbxG3D
   - [ ] boost.static
     - Needs a new Boost version.
   - [x] DSBaseClasses
   - [ ] libcurl
	 - OpenSSL libraries gives unresolved external symbols so these should get updated too.
   - [ ] SDL2
     - Windows SDK gives "negative subscript" errors.
   - [x] zlib
   - [ ] ShaderCompiler
   - [ ] Bootstrapper
   - [ ] BootstrapperClient
   - [ ] RobloxProxy
   - [ ] NPRobloxProxy
   - [ ] BootstrapperQTStudio
   - [ ] BootstrapperRCCService

## 📚 Libraries Used
- [Boost](/Contribs/boost_1_56_0) = 1.56.0
- [cpp-netlib](/Contribs/cpp-netlib-0.11.0-final) = 0.11.0-final
- [curl](/Contribs/windows/x86/curl/curl-7.43.0) = 7.43.0
- [DSBaseClasses](/Contribs/DSBaseClasses) = *unknown*
- [SDL2](/Contribs/SDL2) = 2.0.4
- [Roblox SDK](/Contribs/SDK) = *unknown*
- [OpenSSL](/Contribs/openssl) = 1.0.0c
- [Qt](BUILDING_CONTRIBS.md) = 4.8.5
- [glsl-optimizer](/Rendering/ShaderCompiler/glsl-optimizer) = *unknown*
- [hlsl2glslfork](/Rendering/ShaderCompiler/hlsl2glslfork) = *unknown*
- [mojoshader](/Rendering/ShaderCompiler/mojoshader) = *unknown*
- [w3c-libwww](/Contribs/w3c-libwww-5.4.0) = 5.4.0
- [VMProtect](/Contribs/VMProtectWin_2.13) = 2.13
- [zlib](/Contribs/windows/x86/zlib/zlib-1.2.8) = 1.2.8

### 🔨 Tools Used
- [HxD](/Tools/HxD) = 2.5.0.0
- [cecho](/Tools/cecho) = *unknown*
- [ILSpy](/Tools/ILSpy) = 9.0
- [rbxsigner](/Tools/rbxsigner) = *unknown*

---

## ❤️ Contributors
[@xspyy](https://github.com/xspyy)
* fromHSV and fromHex
* Asset Proxy **(index_online.php)** and Trustcheck Fixes!
* Character and BodyColors Fetching
   * Also he gave his database, what a kind of him :)
* Avatar Renderer

[@cetcat](https://github.com/cetcat)
* Helped Compilation of Bootstrappers

[@bpr1ch3](https://github.com/bpr1ch3)
* New Files and FFlags for Rocknet.

Be a contributor by doing a helpful of changes to the source code or to the [RockNet](https://github.com/P0L3NARUBA/rocknet-rblx).