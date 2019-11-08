pushd v8jsi

MSBuild.exe v8jsi.vcxproj /p:Platform=x64 /p:Configuration=Release -verbosity:detailed
MSBuild.exe v8jsi.vcxproj /p:Platform=x64 /p:Configuration=Debug -verbosity:detailed

MSBuild.exe v8jsi.vcxproj /p:Platform=Win32 /p:Configuration=Release -verbosity:detailed
MSBuild.exe v8jsi.vcxproj /p:Platform=Win32 /p:Configuration=Debug -verbosity:detailed

popd