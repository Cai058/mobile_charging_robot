# Isolated STM32CubeMX project

This directory contains the stage 3 hardware-configuration reconstruction. It is intentionally isolated from the verified root firmware.

Generate the project with:

```powershell
& 'E:/download/STM32CubeMX/jre/bin/java.exe' `
  -jar 'E:/download/STM32CubeMX/STM32CubeMX.exe' `
  -q 'cubemx/generate-stage1.txt'
```

Generated source is written to `cubemx/generated/`. Do not copy generated initialization into the root firmware until the current migration slice has passed review, compilation and on-board testing.

The firmware package is pinned to `STM32Cube FW_F4 V1.26.2`. CubeMX copies its `Drivers/` directory and a duplicate IOC into the output project during generation; those reproducible copies, `.mxproject` metadata and all build directories are intentionally ignored by Git. `cubemx/mobile_charging_robot.ioc` is the only authoritative IOC.

Build the isolated generated project with GNU Arm GCC 14.2 and Ninja:

```powershell
$env:PATH='C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin;C:/Users/Lexi/AppData/Local/Microsoft/WinGet/Packages/Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe;' + $env:PATH
Set-Location 'cubemx/generated/mobile_charging_robot_cubemx'
cmake --preset Debug --fresh
cmake --build --preset Debug
```

CAN automatic bus-off recovery is represented by `ABOM=ENABLE` in the IOC. The explicit `CAN_IT_BUSOFF` enables are intentionally kept in the generated `can.c` USER CODE regions and survive regeneration.
