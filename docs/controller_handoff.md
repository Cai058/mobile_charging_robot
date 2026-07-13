# 移动充电机器人 Controller 故障分析交接文档

## 下一会话目标

继续分析并修复 011 机器人三点直轨上的自动导航问题：机器人错过目标点光电门后应安全搜索、反向，并且不能越过 1/3 号物理端点冲出轨道。

## 已阅读范围

已完整阅读 Controller 相关文件：

- `Controller/inc/ControllerRC.h`
- `Controller/src/ControllerRC.c`
- `Controller/inc/ControllerDummy.h`
- `Controller/src/ControllerDummy.c`
- `Controller/inc/Chassiscontroller.h`
- `Controller/src/Chassiscontroller.c`

为理解直接配置，额外阅读：

- `User/Inc/Robot_Config.h`
- `User/Src/Robot_Config.c`

尚未修改任何代码。

## 控制架构概览

`Chassiscontroller.c::Update()` 每周期：

1. `Sensor_t_Update()` 更新 RFID、光电门、限位开关、服务器、电池和遥控器数据。
2. 根据 `RC_GetMode()` 选择自动或遥控。
3. 自动模式执行 `Dummy_Update()` 和 `SwitchState()`。
4. 目标速度经过一阶滤波和 PID 后，通过 CAN 下发电机。

自动状态机：

```text
IDLE → RUNNING → ADJUST → WORKING → PUSH/PULL → IDLE
                          └→ CHARGING → IDLE/RUNNING
```

RFID 更新 `location_id` 时只在识别到非零卡号时更新，因此 `location_id` 表示“最近一次识别的点位”，不是连续实时位置。

## 当前生效的 011 配置

```c
#define Robot_ID "011"
#define Total_Points 3
#define Direction_Point 18
#define Charge_ID 2
#define Move_Direction 1
#define slow_speed 4000
#define fast_speed 5000
#define working_speed 5000
```

011 场地实际上是 `1—2—3` 三点直轨，但实际寻路函数仍按 `Direction_Point == 18` 的环形轨道计算方向。

## 用户报告的现场故障

自动模式下，机器人初始停在 1 号和 2 号之间：

```text
location_id = 1
pg_state = 0
```

收到“去 1 号取桩”指令后，机器人经过 2 号、3 号，最终冲出 3 号端点。

## 已确认的逐周期执行路径

### 初始周期：`location_id == target_id == 1`

在 `ControllerDummy.c` 的 `STATE_RUNNING` 中：

```c
if (m_ctrl.location_id == m_ctrl.current_target_id)
{
    SetxSpeed(500, 2);
    arrive_flag = 1;
    if (m_ctrl.pg_state == 1) {
        /* 进入 STATE_ADJUST */
    }
}
```

因为 `pg_state == 0`，机器人不会进入 `STATE_ADJUST`，而是以 500 的目标速度沿旧的 `move_direction` 继续运动。`move_direction` 初始化/重置为 1，所以会朝 2、3 号方向，而不是返回寻找 1 号光电门。

代码确实设置了低速 500；问题是低速方向没有依据目标光电门相对机器人位于前方还是后方来决定。

### 到达 2 号后

此时 `location_id != target_id`，方向函数会执行：

```c
m_ctrl.move_direction = get_direction(2, 1);
```

按 `Direction_Point == 18` 计算，结果为 `-1`，理论上应该返回 1 号。

但常规慢速分支中真正调用 `SetxSpeed()` 的条件是：

```c
if (arrive_flag == 0) {
    SetxSpeed(...);
}
```

此前 `arrive_flag` 已被设为 1，所以虽然 `move_direction` 变为 `-1`，新的负方向速度没有写入 `m_motor_speed[]`。改变方向变量不会自动改变已经保存的电机速度符号，电机可能继续保持原来的正向 500。

### 到达 3 号后

状态机没有“到达 1/3 号物理端点就强制停车或折返”的逻辑，因此仍可能保持旧速度冲出轨道。

## “错过目标后反向”代码为何没生效

第一处在 `STATE_RUNNING`：

```c
if (arrive_flag == 1 && location_id == current_target_id) {
    arrive_cnt++;
    if (arrive_cnt > 14000) {
        arrive_cnt = 0;
        // m_ctrl.move_direction = -m_ctrl.move_direction;
    }
}
```

真正反向的语句已被注释；并且离开目标 RFID、识别到 2 号后，`location_id == target_id` 不再成立，计时也停止。

第二处在 `STATE_ADJUST`：

```c
if (pg_state == 1 && location_id == target_id) {
    /* 确认到达 */
} else {
    move_direction = -move_direction;
    lastState = STATE_ADJUST;
    currentState = STATE_RUNNING;
}
```

但进入 `STATE_ADJUST` 的前提本身就是在 `STATE_RUNNING` 中同时满足 `location_id == target_id && pg_state == 1`。本故障中光电门始终为 0，因此根本进不了该状态，也执行不到反向分支。

## 关于 `Total_Points == 3` 的关键澄清

用户记得的端点折返逻辑确实存在于 `Get_test_index()`：

```c
if (_next == 1) m_test_direction = 1;
else if (_next == Total_Points) m_test_direction = 2;
```

它在测试模式下生成目标序列：

```text
1 → 2 → 3 → 2 → 1 → ...
```

但它只修改 `m_test_direction` 和 `test_index`，即“下一测试目标”，不会修改实际电机导航使用的 `m_ctrl.move_direction`。

因此过去观察到的“到 3 号折返”，很可能是完成 3 号任务后，下一个测试目标变成 2 号，从而看起来像端点自动折返；它不是物理端点保护。

`Total_Points` 当前主要用于校验任务 ID 和生成测试目标序列；实际导航方向使用 `Direction_Point`，当前仍为 18。

不要简单把 `Direction_Point` 改成 3：环形最短路会认为 3→1 和 1→3 可以跨越闭环边界，而真实直轨不存在 3—1 直接相连，同样危险。

## 当前根因结论

1. `location_id` 是最近 RFID 点位，却被当成实时精确位置。
2. `location_id == target && pg_state == 0` 时沿旧方向盲搜，没有明确搜索方向或搜索阶段。
3. `arrive_flag == 1` 阻止之后的速度更新，造成方向变量改变但电机速度未反向。
4. 011 是直轨，但导航/恢复逻辑沿用了环形场地假设，且没有端点硬保护。

## 建议下一步

先确认期望的异常搜索策略，再修改代码。建议重点讨论：

1. 011 直轨使用单独的方向规则；当前位置等于目标但光电门未触发时，不能仅凭 RFID 判断搜索方向。
2. 引入明确的目标光电门搜索状态/阶段，记录进入目标 RFID 区域时的运动方向，不要复用含义混杂的 `arrive_flag`。
3. 一旦 `move_direction` 改变，立即调用 `SetxSpeed()` 写入新方向速度。
4. 在 1、3 号端点增加独立安全保护：若运动方向指向轨道外，立即停车或反向；最好使用物理限位或额外端点传感器，不能只依赖 RFID。
5. 检查 `slow_cnt`、`arrive_cnt`、`adjust_cnt` 的重置位置，避免跨任务残留。
6. 建立纯 C 或脚本状态机回放，覆盖：
   - 初始位于 1—2 之间，`location_id=1, pg=0`，目标 1；
   - 正常从 2 去 1；
   - 正常从 2 去 3；
   - 错过 1 号光电门；
   - 错过 3 号光电门；
   - 方向变量变化后电机目标速度符号同步变化。

## 建议技能

- `diagnose`：继续用可复现状态序列验证根因和修复。
- `tdd`：若要求实施修复，先建立最小状态机回放测试，再修改代码。

## 注意事项

- 当前只完成诊断，没有实施修复。
- 若需确认传感器语义，可继续阅读 RFID、光电门和主循环调用频率相关实现。
