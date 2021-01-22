set PATH=%PATH%;%~dp0scripts;%WIX%bin

call make-version.cmd VERSION version.h

mkdir "%~dp0_setup/"
set OUTPUT="%~dp0_setup\micro-profiler.v%VERSION%.vsix"

pushd "%~dp0content"
	call sha256 hashpreview micro-profiler_preview.png
popd
pushd "%~dp0deployment\wix"
	call sha256 hashlicense license.rtf
popd
pushd "%~dp0legacy"
	call sha256 hashinit micro-profiler.initializer.cpp
popd
pushd "%~dp0_build.windows.x86\_bin\RelWithDebInfo"
	call sha256 hashmpfrontend micro-profiler_frontend.dll
	call sha256 hashmpui 1033\micro-profiler.ui.dll

	call sha256 hashpkgdef micro-profiler.pkgdef
	call sha256 hashmanifest extension.vsixmanifest
	call sha256 hashappicon logo.ico

	call sha256 hashmpx86 micro-profiler_Win32.dll
	call sha256 hashmpx86lib micro-profiler_Win32.lib
popd
pushd "%~dp0_build.windows.x64\_bin\RelWithDebInfo"
	call sha256 hashmpx64 micro-profiler_x64.dll
	call sha256 hashmpx64lib micro-profiler_x64.lib
popd
pushd "%~dp0_build.linux.x86\_bin"
	call sha256 hashlinuxmpx86 libmicro-profiler_x86.so
popd
pushd "%~dp0_build.linux.x64\_bin"
	call sha256 hashlinuxmpx64 libmicro-profiler_x64.so
popd
pushd "%~dp0_build.android.arm\_bin"
	call sha256 hashandroidarm libmicro-profiler_arm.so
popd

pushd "%~dp0legacy"
	call mkzip micro-profiler.initializer.cpp "%OUTPUT%"
popd
pushd "%~dp0content"
	call mkzip micro-profiler_preview.png "%OUTPUT%"
popd
pushd "%~dp0deployment\wix"
	call mkzip license.rtf "%OUTPUT%"
popd
pushd "%~dp0_build.windows.x86\_bin\RelWithDebInfo"
	call mkzip micro-profiler_frontend.dll "%OUTPUT%"
	call mkzip 1033\micro-profiler.ui.dll "%OUTPUT%"
	
	call mkzip micro-profiler.pkgdef "%OUTPUT%"
	call mkzip extension.vsixmanifest "%OUTPUT%"
	call mkzip logo.ico "%OUTPUT%"
	
	call mkzip micro-profiler_Win32.dll "%OUTPUT%"
	call mkzip micro-profiler_Win32.lib "%OUTPUT%"
popd
pushd "%~dp0_build.windows.x64\_bin\RelWithDebInfo"
	call mkzip micro-profiler_x64.dll "%OUTPUT%"
	call mkzip micro-profiler_x64.lib "%OUTPUT%"
popd
pushd "%~dp0_build.linux.x86\_bin"
	call mkzip libmicro-profiler_x86.so "%OUTPUT%"
popd
pushd "%~dp0_build.linux.x64\_bin"
	call mkzip libmicro-profiler_x64.so "%OUTPUT%"
popd
pushd "%~dp0_build.android.arm\_bin"
	call mkzip libmicro-profiler_arm.so "%OUTPUT%"
popd

pushd "%~dp0_setup"
	call replace.cmd "%~dp0deployment\vsix\catalog.json" > catalog.json
	call mkzip catalog.json "%OUTPUT%"
	call replace.cmd "%~dp0deployment\vsix\manifest.json" > manifest.json
	call mkzip manifest.json "%OUTPUT%"
	call mkzip "%~dp0deployment\vsix\[Content_Types].xml" "%OUTPUT%"
popd
