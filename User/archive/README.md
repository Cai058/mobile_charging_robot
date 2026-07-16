# 旧初始化代码归档

本目录保存 CubeMX 逐外设迁移前、已经通过整机验证的旧初始化代码，用于源码对比和紧急回退。

规则：

- 本目录不参与根 CMake 编译。
- 每个旧文件只在首次迁移时归档一次，后续不跟随新实现修改。
- 当前归档基线为提交 `440b4c1`，标签 `v2.0.0-cmake-vscode`。
- 正式运行源码仍位于 `User/Src/`。

当前文件：

- `can.c`：迁移前的 CAN1/CAN2 初始化实现。
- `gpio.c`：迁移前的 GPIO 初始化实现。
- `bsp_rc.c`：迁移前的 USART1、DMA 和 RC 接收初始化实现。
- `time.c`：迁移前的 TIM2 系统调度初始化实现。
- `bsp_485_rfid.c`：迁移前的 UART8、DMA1 和 RFID 收发初始化实现。
- `bsp_485_battery.c`：迁移前的 USART6、DMA2 和 Battery 收发初始化实现。
- `bsp_485_server.c`：迁移前的 UART7、DMA1 和 Server 收发初始化实现。
