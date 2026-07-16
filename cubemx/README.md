# 隔离的 STM32CubeMX 工程

本目录用于逐外设重建硬件配置。`mobile_charging_robot.ioc` 是唯一有效的 CubeMX 配置源，生成代码先进入 `cubemx/generated/`，审查通过后再按白名单同步到根工程。

## 重新生成代码

在仓库根目录运行：

```powershell
& 'E:/download/STM32CubeMX/jre/bin/java.exe' `
  -jar 'E:/download/STM32CubeMX/STM32CubeMX.exe' `
  -q 'cubemx/generate-stage1.txt'
```

固件包固定为 `STM32Cube FW_F4 V1.26.2`，本地路径为 `C:/Users/Lexi/STM32Cube/Repository/STM32Cube_FW_F4_V1.26.2`。

CubeMX 会更新整个隔离生成工程，但根工程不得直接复制整个 `Src/` 或 `Inc/`。每次只审查和同步当前迁移外设。

隔离工程中的 `Drivers/`、`build/`、生成副本 `.ioc` 和 `.mxproject` 都可以由 CubeMX/固件包重新创建，已被 Git 忽略，可在目录清理时安全删除。正式配置源始终是 `cubemx/mobile_charging_robot.ioc`。

## 白名单同步

CAN：

```powershell
powershell -ExecutionPolicy Bypass -File cubemx/sync-generated.ps1 -Peripheral can
```

该白名单会同时同步 `can.c` 和 `can.h`，避免生成端与正式端的句柄或初始化声明不一致。脚本还会自动把 CubeMX 在 `can.h` 文件末尾生成的多个空行规范为一个换行，保证重复同步和 `git diff --check` 稳定通过。

GPIO：

```powershell
powershell -ExecutionPolicy Bypass -File cubemx/sync-generated.ps1 -Peripheral gpio
```

USART1/RC DMA：

```powershell
powershell -ExecutionPolicy Bypass -File cubemx/sync-generated.ps1 -Peripheral usart1_rc
```

TIM2 系统调度：

```powershell
powershell -ExecutionPolicy Bypass -File cubemx/sync-generated.ps1 -Peripheral tim2
```

UART8/RFID DMA：

```powershell
powershell -ExecutionPolicy Bypass -File cubemx/sync-generated.ps1 -Peripheral rfid_uart8
```

USART6/Battery DMA：

```powershell
powershell -ExecutionPolicy Bypass -File cubemx/sync-generated.ps1 -Peripheral battery_usart6
```

UART7/Server DMA：

```powershell
powershell -ExecutionPolicy Bypass -File cubemx/sync-generated.ps1 -Peripheral server_uart7
```

同步目标保持为根工程的稳定路径 `User/Src/<外设>.c`。迁移前的旧版本保存在 `User/archive/`，不参与 CMake 编译。

## 根工程编译

普通 Debug 编译：

```powershell
cmake --build --preset debug
```

迁移切片验收时执行全量编译：

```powershell
cmake --build --preset debug --clean-first
```

最终 ELF 位于 `build/debug/mobile_charging_robot.elf`。
