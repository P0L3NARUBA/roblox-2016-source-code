@ECHO OFF
REM Written by yungDoom
REM LOGIC: It copies the necessary files from various place to folder you've selected.

:: Remove useless libraries (still don't know why i even added them)
if exist "C:\Trunk2016\Contribs\hlsl2glslfork" ( del /q /s C:\Trunk2016\Contribs\hlsl2glslfork && rd /s /q C:\Trunk2016\Contribs\hlsl2glslfork )
if exist "C:\Trunk2016\Contribs\glsl-optimizer" ( del /q /s C:\Trunk2016\Contribs\glsl-optimizer && rd /s /q C:\Trunk2016\Contribs\glsl-optimizer ) 
if exist "C:\Trunk2016\Contribs\xulrunner" ( del /q /s C:\Trunk2016\Contribs\xulrunner && rd /s /q C:\Trunk2016\Contribs\xulrunner ) 
if exist "C:\Trunk2016\Contribs\libwebm" ( del /q /s C:\Trunk2016\Contribs\libwebm && rd /s /q C:\Trunk2016\Contribs\libwebm ) 
if exist "C:\Trunk2016\Contribs\cabsdk" ( del /q /s C:\Trunk2016\Contribs\cabsdk && rd /s /q C:\Trunk2016\Contribs\cabsdk ) 
if exist "C:\Trunk2016\Contribs\curl-7.50.2" ( del /q /s C:\Trunk2016\Contribs\curl-7.50.2 && rd /s /q C:\Trunk2016\Contribs\curl-7.50.2 ) 
if exist "C:\Trunk2016\Contribs\SDL2.0.18" ( del /q /s C:\Trunk2016\Contribs\SDL2.0.18 && rd /s /q C:\Trunk2016\Contribs\SDL2.0.18 ) 


ECHO 1. RobloxStudio
ECHO 2. RCCService
ECHO 3. WindowsClient
Echo 4. Custom Location
Echo 5. Clear all the libraries
Echo 6. Clear all the libraries from Custom Location
Echo 7. Help
Echo 8. Exit
ECHO.

if exist ".git\" (
tools\cecho\cecho {0A}You're using Git Version of the source, cool!{#}
) else (
tools\cecho\cecho {0C}You're using LOCAL Git Version of the source, not cool!{#}
)

ECHO.

CHOICE /C 12345678 /M "Enter your choice 1-8:"

:: This is where it gets your input
IF ERRORLEVEL 8 GOTO Exit
IF ERRORLEVEL 7 GOTO Help
IF ERRORLEVEL 6 GOTO ClearLocation
IF ERRORLEVEL 5 GOTO Clear
IF ERRORLEVEL 4 GOTO CustomPath
IF ERRORLEVEL 3 GOTO WindowsClient
IF ERRORLEVEL 2 GOTO RCCService
IF ERRORLEVEL 1 GOTO RobloxStudio

:Exit :: the command line closes.
exit
GOTO End

:Help
cls
ECHO This batch file helps you copying the libraries
ECHO Its an important process to build the game so make sure you do it
ECHO Open this bat file again to start selecting.
TIMEOUT /T 10
GOTO End

:ClearLocation
cls
tools\cecho\cecho {0E}WARNING: Your input should start with: C:\Trunk2016{#}
set /p loco=Enter the path: 
if exist "%loco%" (
    cd /d %loco%
    del *.lib
    cd /d C:\Trunk2016\
    ECHO.

    tools\cecho\cecho {0A}All the necessary files from the folder you've choosed has been cleared.{#}

    ECHO.
    TIMEOUT /T 3 
) else (
  cls
  tools\cecho\cecho {0C}Failed to do the task{#}
  ECHO.
  tools\cecho\cecho {0C}Make sure you've extracted the Source and Contribs correctly.{#}
  TIMEOUT /T 5
)
GOTO End

:Clear
cls
if exist "C:\Trunk2016\" (
  if exist C:\Trunk2016\WindowsClient (
    cd /d C:\Trunk2016\WindowsClient
    del *.lib )
  if exist C:\Trunk2016\RCCService (
    cd /d C:\Trunk2016\RCCService
    del *.lib )
 if exist C:\Trunk2016\RobloxStudio (
    cd /d C:\Trunk2016\RobloxStudio
    del *.lib )
    cd /d C:\Trunk2016\
    tools\cecho\cecho {0A}All the necessary folders have been cleared.{#}
    TIMEOUT /T 3 
) else ( 
  cls
  tools\cecho\cecho {0C}Failed to do the task{#}
  ECHO.
  tools\cecho\cecho {0C}Make sure you've extracted the Source and Contribs correctly.{#}
  TIMEOUT /T 5
)
GOTO End

:CustomPath
cls
tools\cecho\cecho {0E}WARNING: Your input should start with: C:\Trunk2016{#}
set /p in=Enter the path where you want the files to go: 
if exist "%in%" (
    if not exist "%in%\libboost_locale-vc110-mt-1_56.lib" (
    xcopy C:\Trunk2016\Contribs\boost_1_56_0\stage\lib\*.* %in%
    )
    if not exist "%in%\VMProtectSDK32.lib" (
    xcopy C:\Trunk2016\Contribs\VMProtectWin_2.13\lib\*.lib %in%
    )
    if not exist "%in%\libcurl_a.lib" (
    xcopy "C:\Trunk2016\Contribs\windows\x86\curl\curl-7.43.0\build\Win32\VC11\DLL Release - DLL OpenSSL\libcurl_a.lib" %in%
    )
    if not exist "%in%\VMProtectSDK32.lib" (
    xcopy C:\Trunk2016\zlib\win\bin\Release\zlib.lib %in%
    )
    ECHO.

    tools\cecho\cecho {0A}All the folders has been copied, have a good luck!{#}
    
    ECHO.
    TIMEOUT /T 3 
) else ( 
  cls
  tools\cecho\cecho {0C}Failed to do the task{#}
  ECHO.
  tools\cecho\cecho {0C}Make sure you've extracted the Source and Contribs correctly.{#}
  TIMEOUT /T 5
)
GOTO End

:WindowsClient
cls
if exist "C:\Trunk2016\WindowsClient" (
    if not exist "C:\Trunk2016\WindowsClient\libboost_locale-vc110-mt-1_56.lib" (
    xcopy C:\Trunk2016\Contribs\boost_1_56_0\stage\lib\*.* C:\Trunk2016\WindowsClient\
    )
    if not exist "C:\Trunk2016\WindowsClient\VMProtectSDK32.lib" (
    xcopy C:\Trunk2016\Contribs\VMProtectWin_2.13\lib\*.lib C:\Trunk2016\WindowsClient\
    )
    if not exist "C:\Trunk2016\WindowsClient\libcurl_a.lib" (
    xcopy "C:\Trunk2016\Contribs\windows\x86\curl\curl-7.43.0\build\Win32\VC11\DLL Release - DLL OpenSSL\libcurl_a.lib" C:\Trunk2016\WindowsClient\
    )
    if not exist "C:\Trunk2016\WindowsClient\zlib.lib" (
    xcopy C:\Trunk2016\zlib\win\bin\Release\zlib.lib C:\Trunk2016\WindowsClient\
    )
    tools\cecho\cecho {0A}All the folders has been copied, have a good luck!{#}
    TIMEOUT /T 3
) else (
  cls 
  tools\cecho\cecho {0C}Failed to do the task{#}
  ECHO.
  tools\cecho\cecho {0C}Make sure you've extracted the Source and Contribs correctly.{#}
  TIMEOUT /T 5
)
GOTO End


:RCCService
cls
if exist "C:\Trunk2016\RCCService" (
    if not exist "C:\Trunk2016\RCCService\libboost_locale-vc110-mt-1_56.lib" (
    xcopy C:\Trunk2016\Contribs\boost_1_56_0\stage\lib\*.* C:\Trunk2016\RCCService\
    )
    if not exist "C:\Trunk2016\RCCService\VMProtectSDK32.lib" (
    xcopy C:\Trunk2016\Contribs\VMProtectWin_2.13\lib\*.lib C:\Trunk2016\RCCService\
    )
    if not exist "C:\Trunk2016\RCCService\libcurl_a.lib" (
    xcopy "C:\Trunk2016\Contribs\windows\x86\curl\curl-7.43.0\build\Win32\VC11\DLL Release - DLL OpenSSL\libcurl_a.lib" C:\Trunk2016\RCCService\
    )
    if not exist "C:\Trunk2016\RCCService\zlib.lib" (
    xcopy C:\Trunk2016\zlib\win\bin\Release\zlib.lib C:\Trunk2016\RCCService\
    )
    tools\cecho\cecho {0A}All the folders has been copied, have a good luck!{#}
    TIMEOUT /T 3
) else ( 
  cls

  tools\cecho\cecho {0C}Failed to do the task{#}
  ECHO.
  tools\cecho\cecho {0C}Make sure you've extracted the Source and Contribs correctly.{#}
  TIMEOUT /T 5
)
GOTO End

:RobloxStudio
cls
if exist "C:\Trunk2016\RobloxStudio" (
    if not exist "C:\Trunk2016\RobloxStudio\libboost_locale-vc110-mt-1_56.lib" (
    xcopy C:\Trunk2016\Contribs\boost_1_56_0\stage\lib\*.* C:\Trunk2016\RobloxStudio\
    )
    if not exist "C:\Trunk2016\RobloxStudio\VMProtectSDK32.lib" (
    xcopy C:\Trunk2016\Contribs\VMProtectWin_2.13\lib\*.lib C:\Trunk2016\RobloxStudio\
    )
    if not exist "C:\Trunk2016\RobloxStudio\libcurl_a.lib" (
    xcopy "C:\Trunk2016\Contribs\windows\x86\curl\curl-7.43.0\build\Win32\VC11\DLL Release - DLL OpenSSL\libcurl_a.lib" C:\Trunk2016\RobloxStudio\
    )
    if not exist "C:\Trunk2016\RobloxStudio\zlib.lib" (
    xcopy C:\Trunk2016\zlib\win\bin\Release\zlib.lib C:\Trunk2016\RobloxStudio\
    )
    tools\cecho\cecho {0A}All the folders has been copied, have a good luck!{#}
    TIMEOUT /T 3
) else ( 
  cls
  tools\cecho\cecho {0C}Failed to do the task{#}
  ECHO.
  tools\cecho\cecho {0C}Make sure you've extracted the Source and Contribs correctly.{#}
  TIMEOUT /T 5
)
GOTO End

:End
