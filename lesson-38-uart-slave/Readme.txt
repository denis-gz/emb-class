
HOW TO CREATE ESP32 PROJECT FROM TEMPLATE:

1. Copy cmake-template folder with a new name
2. Search and replace occurrences of "cmake-template" with a new project's name
3. Review main\Kconfig.projbuild, add/update hardware config params as needed
4. Open CMakeLists.txt as project in QtCreator, select "extensa-esp32s3" as a kit for the project
5. Set build type to "Debug" both in the Build type input field and CMake configuration
6. Correct path in CMake / "Build directory" field (this folder should be just "build", no subfolders)
7. If asked, click Apply
8. CMake should automaticall run configuration. If not, click on Run CMake
9. Under Build Steps, select "app" (build only) or "flash" (build & flash) target, unselect "all".
10. In "CMake arguments" filed, add "--preset default"
11. Build the project, check for any errors
12. For debugging, add these "Additional startup commands" in the project's Run Settings:

=== Working ===
mon reset halt
flushregs
set remote hardware-watchpoint-limit 2
mon arm semihosting enable
add-symbol-file {ActiveProject:RunConfig:Executable:NativeFilePath}
thbreak app_main
continue
===

set remotetimeout 20	

mon reset halt
flushregs
set remote hardware-watchpoint-limit 2
mon arm semihosting enable
add-symbol-file {ActiveProject:RunConfig:Executable:NativeFilePath}

mon reset halt
thbreak app_main
continue

=== CMake config ===
-DCMAKE_C_COMPILER:FILEPATH=%{Compiler:Executable:C}
-DCMAKE_CXX_COMPILER:FILEPATH=%{Compiler:Executable:Cxx}
-DCMAKE_BUILD_TYPE:UNINITIALIZED=Build
-DPYTHON_DEPS_CHECKED:UNINITIALIZED=1
-DPYTHON:UNINITIALIZED=C:\Espressif\tools\python\v5.5.2\venv\Scripts\python.exe
-DESP_PLATFORM:UNINITIALIZED=1
-DCCACHE_ENABLE:UNINITIALIZED=0
-DQT_CREATOR_SKIP_MAINTENANCE_TOOL_PROVIDER:UNINITIALIZED=ON
===
