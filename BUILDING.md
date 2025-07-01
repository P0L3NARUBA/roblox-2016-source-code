# Cookbook for the Source

## Read This Before Reading the Cookbook
``<path-to-source>`` is the path to where your source is, e.g. C:\GitHub\Roblox-2016<br>
You must have some prior technical knowledge to this, as we cannot help you with every problem you have.

1. Open a Command Prompt with Administrator Rights, then clone the repository with **[Git](https://git-scm.com/)** like this:
```
git config --system core.longpaths true && cd <path-to-source> && git clone https://github.com/P0L3NARUBA/roblox-2016-source-code
```
  - Do this if you don't want to download the source code over and over again. **(Recommended)**
2. You will need to use **[Visual Studio 2012](https://drive.google.com/file/d/1XoA5Av_6OedTwGi_ebTb_XsQ7-RmEKSd/view?usp=sharing)**, **[Visual Studio 2012 Update 5](https://drive.google.com/file/d/1_rrwnITjCl-kcqEKTQWUDJgEegAcKAM6/view?usp=sharing)** and **[Visual Studio 2022](https://visualstudio.microsoft.com/tr/vs/)** for viewing the solution
  - Uncheck all the optional components in the Visual Studio 2012 installer except for **"Microsoft Foundation Classes for C++"** to save space, as none of them are needed.
3. Create a system environment variable called **CONTRIB_PATH** and set the path to: **`<path-to-source>\Contribs`**
4. Now the libraries must be built. To do so, head over to: **[BUILDING_CONTRIBS.md](/BUILDING_CONTRIBS.md)**
5. When all set, open the **Client_2016.sln** Solution inside your source folder with **Visual Studio 2022**
  - Press **Cancel** if you're prompted with a **Review Solution Actions** window.
6. Change the Solution Configuration from the top to:
  - **Release** if you're building **WindowsClient**
  - **ReleaseStudio** if you're building **RobloxStudio**
  - **ReleaseRCC** if you're building **RCCService**
7. Before building **RCCService**, **RobloxStudio** or **WindowsClient**, you should build these projects first:
  - **3rd Party > boost.static**
  - **3rd Party > zlib**
  - **3rd Party > libcurl**
     - If you already built this while following **BUILDING_CONTRIBS.md**, you can skip this.
  - **3rd Party > SDL2**
     - If you already built this while following **BUILDING_CONTRIBS.md**, you can skip this.
  - **gSOAP > soapcpp2**
  - **gSOAP > wsdl2h**
  - **Shaders > ShaderCompiler**
  - **Rendering > LibOVR**
  - **qtnribbon**
  - **sgCore**
  - **CoreScriptConverter2** (Only if you're building **WindowsClient**)
8. Right-click on the project you would like to build **(RCCService, RobloxStudio or either WindowsClient)** and click **Build**

**Thats it, you successfully built from the source!**