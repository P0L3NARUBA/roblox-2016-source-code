# Cookbook for the Contribs

## Read This Before Reading the Cookbook
``<path-to-source>`` is the path to where your source is, e.g. C:\GitHub\Roblox-2016<br>

## Qt

**TIP:** You can download [Qt.7z](https://drive.google.com/file/d/10zhRv421d2DUdF7eV-dqR4cIDTZAhiDL/view?usp=drive_link) and extract the contents into the `<path-to-source>\Contribs` folder if you'd prefer to skip compiling it.

1. Open **VS2012 x86 Native Tools Command Prompt**
2. Change the Directory (using `cd`) to: `<path-to-source>\Contribs\Qt\4.8.5\win_VS2012` 
3. Run the following command:
```sh
configure -make nmake -platform win32-msvc2012 -prefix <path-to-source>\Contribs\Qt\4.8.5\win_VS2012 -opensource -confirm-license -opengl desktop -nomake examples -nomake tests -webkit -xmlpatterns
```
4. Type **nmake confclean** to make sure you're on the right track.
5. When everything completes, type **nmake** and do something else in the meantime as it will take **a long** time to compile.
 
## Boost
1. Navigate to **`<path-to-source>\Contribs\boost_1_56_0\`**
2. Run **bootstrap.bat**
3. After that is complete, run **build_boost.bat** and it will begin compiling.
    - If you get any errors about Python, **do not worry** as this is normal and will not affect compilation.

You should be greeted with these lines after compilation finishes:

```
...failed updating 56 targets...
...skipped 8 targets...
...updated 1095 targets...
```

## OpenSSL
1. Install [Strawberry Perl](https://strawberryperl.com/)
2. Open **Developer Command Prompt for VS2012**
3. Change the Directory to: **`<path-to-source>\Contribs\openssl`**
4. Run **`perl Configure VC-WIN32`**
    - If you get an error along the lines of `'perl' is not recognized as an internal or external command, operable program or batch file.`, run `set PATH=<path-to-perl>\bin;%PATH%` and then try running the above command again.
5. Then run this in the command prompt: **`ms\32all.bat`**
6. Create a new folder named **openssl** inside your source folder
7. When the build process completes, navigate to **`<path-to-source>\Contribs\openssl\out32dll`** and copy these 2 files from there to **`<path-to-source>\openssl`**: **`ssleay32.dll, libeay32.dll`**.

**IMPORTANT:** You must be in **Client_2016.sln** for these 2 dependencies below.

## SDL2
Locate the SDL2 Project at **3rd Party > SDL2**, right-click on it and click **Build**<br>
- You may need to build it as a .dll file instead. To do this, go into **Properties** and change **Target Extension** to **.dll**, **Configuration Type** to **Dynamic Library (.dll)**, and remove **HAVE_LIBC;** from **C/C++ > Preprocessor > Preprocessor Definitions**.<br>

## libcurl
Locate the libcurl Project at **3rd Party > libcurl**, right-click on it and click **Build**<br>
- You may need to build it as a .lib file instead. To do this, go into **Properties** and change **Target Extention** to **.lib** and **Configuration Type** to **Static Library (.lib)**.

**That's it, you've compiled the libraries!**<br>
**If you would like, you can exchange the library/dll files for ones you have yourself, though this is unrecommended.**