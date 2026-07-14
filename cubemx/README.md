# 隔离的 STM32CubeMX 工程

本目录用于阶段 3 的硬件配置重建，并与已经通过整机验证的根目录固件保持隔离。

使用以下命令重新生成工程：

```powershell
& 'E:/download/STM32CubeMX/jre/bin/java.exe' `
  -jar 'E:/download/STM32CubeMX/STM32CubeMX.exe' `
  -q 'cubemx/generate-stage1.txt'
```

生成的源码位于 `cubemx/generated/`。当前迁移切片完成源码审查、编译和上机测试之前，不得把生成的初始化代码复制到根目录固件中。

固件包固定为 `STM32Cube FW_F4 V1.26.2`。生成时 CubeMX 会把 `Drivers/` 目录和一份重复的 IOC 复制到输出工程中。由于这些文件都可以从固定版本固件包重复生成，Git 会忽略复制的驱动库、重复 IOC、`.mxproject` 元数据以及所有构建目录。`cubemx/mobile_charging_robot.ioc` 是唯一有效的 IOC 配置源。

使用 GNU Arm GCC 14.2 和 Ninja 编译隔离工程：

```powershell
$env:PATH='C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin;C:/Users/Lexi/AppData/Local/Microsoft/WinGet/Packages/Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe;' + $env:PATH
Set-Location 'cubemx/generated/mobile_charging_robot_cubemx'
cmake --preset Debug --fresh
cmake --build --preset Debug
```

CAN 自动 Bus-Off 恢复在 IOC 中使用 `ABOM=ENABLE` 表示。显式启用 `CAN_IT_BUSOFF` 的代码保存在生成文件 `can.c` 的 USER CODE 区中，重新生成工程时不会丢失。
