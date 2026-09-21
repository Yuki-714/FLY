# SimpleStandEngine FSR 支撑脚检测技术规划书

## 1. 文档信息

- 目标模块：`SimpleStandEngine`
- 开发分支：`motion-control-dev`
- 当前阶段：双脚着地时的压力测量与重心转移验证
- 后续阶段：确认支撑稳定后，尝试将非支撑脚抬高 5 mm
- 适用对象：NAO 仿真，之后再迁移到真机

## 2. 背景

`SimpleStandEngine` 当前已经能够完成：

1. 从零刚度安全进入站立；
2. 使用逆运动学生成双脚固定的站姿；
3. 使用 IMU 和脚踝 PD 修正前后、左右倾斜；
4. 在双脚不抬起的条件下，使躯干按“中心 → 左 → 中心 → 右 → 中心”移动。

现在只能知道“已经给出了横向移动命令”，还不能证明机器人的重量确实转移到了目标支撑脚。因此，在抬脚之前必须引入 FSR（Foot Sole Resistor，脚底压力传感器）反馈。

本阶段只建立压力测量链路，不使用 FSR 修改关节角，也不抬脚。这样可以先区分“感知错误”和“控制错误”。

## 3. 目标与非目标

### 3.1 本阶段目标

- 读取左脚四个 FSR：`fl`、`fr`、`bl`、`br`；
- 读取右脚四个 FSR：`fl`、`fr`、`bl`、`br`；
- 独立计算左右脚原始总压力；
- 对左右脚总压力进行一阶低通滤波；
- 计算左右脚支撑比例 `supportRatio`；
- 输出可在 SimRobot 中查看的调试曲线；
- 验证躯干左右移动与压力变化方向一致；
- 为下一阶段“安全抬脚”提供可信的支撑状态。

### 3.2 本阶段不做

- 不改变 `standHeight`；
- 不改变当前横向重心移动轨迹；
- 不使用 FSR 闭环修改躯干位置；
- 不抬起任何一只脚；
- 不切换单脚支撑状态；
- 不接入强化学习；
- 不在真机上直接试验未经仿真验证的阈值。

## 4. FSR 数据定义

项目中的 `FsrSensorData` 已提供：

```cpp
pressures[leg][sensor]
totals[leg]
```

压力单位为 kg。每只脚包含四个传感器：

| 名称 | 含义 |
| --- | --- |
| `fl` | 前左 Front Left |
| `fr` | 前右 Front Right |
| `bl` | 后左 Back Left |
| `br` | 后右 Back Right |

虽然数据结构已经提供 `totals`，本阶段仍显式读取并累加四个传感器，以便确认每个输入均已接入，并为以后计算脚底压力中心保留基础。

```cpp
leftRaw = left.fl + left.fr + left.bl + left.br;
rightRaw = right.fl + right.fr + right.bl + right.br;
```

## 5. 数据流

```text
SimRobot 脚底接触力
        │
        ▼
NaoProvider
        │  FsrSensorData：左右脚各四个压力值
        ▼
SimpleStandEngine::updateFootPressure()
        │
        ├─ 有效性检查
        ├─ 四点压力求和
        ├─ 一阶低通滤波
        └─ supportRatio 计算
        │
        ▼
SimRobot 调试曲线
leftPressure / rightPressure / supportRatio
```

FSR 数据在本阶段只流向调试输出，不流向 `JointRequest`。

## 6. 计算方法

### 6.1 传感器有效性

每个压力值使用前必须检查：

- 不是 `SensorData::off`；
- 是有限数值；
- 不小于 0。

仿真阶段要求八个传感器均有效。如果存在无效数据：

- 将本帧标记为 `fsrValid = false`；
- 不用本帧数据更新滤波器；
- `supportRatio` 不参与任何状态判断；
- 输出一次受控的错误或注释，避免每帧刷屏。

### 6.2 低通滤波

FSR 接触力会发生高频跳动，不能直接用于抬脚判定。左右脚分别采用一阶低通滤波：

```cpp
filtered = keep * previous + (1.f - keep) * raw;
```

建议初始参数：

```text
fsrLowPassRatio = 0.9
```

含义：90% 使用上一帧结果，10% 使用当前测量。`keep` 越大，曲线越平稳，但响应越慢。

收到第一帧有效数据时直接令：

```cpp
filtered = raw;
```

这样可以避免滤波值从 0 缓慢爬升。

### 6.3 支撑比例

```cpp
supportRatio =
  (filteredLeftPressure - filteredRightPressure) /
  (filteredLeftPressure + filteredRightPressure);
```

理论范围为 `[-1, 1]`：

| `supportRatio` | 含义 |
| --- | --- |
| 接近 `+1` | 重量主要在左脚 |
| 大于 `0` | 左脚压力大于右脚 |
| 接近 `0` | 两脚压力接近 |
| 小于 `0` | 右脚压力大于左脚 |
| 接近 `-1` | 重量主要在右脚 |

为避免双脚离地或数据失效时除以接近 0 的数，需要先检查：

```cpp
totalPressure = filteredLeftPressure + filteredRightPressure;
```

只有在 `totalPressure >= minTotalPressure` 时才计算有效的 `supportRatio`。建议仿真初值：

```text
minTotalPressure = 0.5 kg
```

该参数只用于防止无接触时误判，不是未来抬脚的支撑阈值。

## 7. 代码改动规划

### 7.1 `SimpleStandEngine.h`

增加头文件：

```cpp
#include "Representations/Infrastructure/SensorData/FsrSensorData.h"
```

在模块依赖中增加：

```cpp
REQUIRES(FsrSensorData),
```

增加配置参数：

```cpp
(float) fsrLowPassRatio,
(float) minTotalPressure,
```

增加成员变量：

```cpp
float filteredLeftPressure = 0.f;
float filteredRightPressure = 0.f;
float supportRatio = 0.f;
bool fsrInitialized = false;
bool fsrValid = false;
bool fsrErrorReported = false;
```

增加方法：

```cpp
void updateFootPressure();
```

### 7.2 `SimpleStandEngine.cpp`

在 `update()` 中声明三个必要曲线：

```cpp
DECLARE_PLOT("module:SimpleStandEngine:fsr:leftPressure");
DECLARE_PLOT("module:SimpleStandEngine:fsr:rightPressure");
DECLARE_PLOT("module:SimpleStandEngine:fsr:supportRatio");
```

每个控制周期调用：

```cpp
updateFootPressure();
```

推荐调用位置是在完成模块初始化之后、状态机处理之前。这样即使机器人处于 `relaxed`、`movingToStand` 或 `standing`，都能持续观察脚底压力；但只有 `standing` 状态才允许后续使用该结果控制抬脚。

`updateFootPressure()` 的处理顺序：

1. 读取左右脚各四个压力值；
2. 检查八个值是否有效；
3. 分别计算左右脚原始总压力；
4. 初始化或更新低通滤波；
5. 检查双脚总压力是否足够；
6. 计算 `supportRatio`；
7. 输出三条调试曲线。

### 7.3 `simpleStandEngine.cfg`

增加初始参数：

```text
fsrLowPassRatio = 0.9;
minTotalPressure = 0.5;
```

第一轮验证只调 `fsrLowPassRatio`，不要同时调整重心移动幅度、周期、站高和 PD 参数，否则无法判断曲线变化由哪个参数造成。

### 7.4 `threads.cfg`

在 `Motion` 线程的 `representationProviders` 中增加：

```text
{representation = FsrSensorData; provider = NaoProvider;},
```

如果没有这项，`SimpleStandEngine` 声明 `REQUIRES(FsrSensorData)` 后，场景无法完成模块依赖解析。

### 7.5 调试曲线配置

可在 SimRobot 控制台中创建两个绘图窗口：

```text
vp FSRPressure 2500 0 6
vpd FSRPressure module:SimpleStandEngine:fsr:leftPressure blue left
vpd FSRPressure module:SimpleStandEngine:fsr:rightPressure red right

vp FSRSupportRatio 2500 -0.1 0.1
vpd FSRSupportRatio module:SimpleStandEngine:fsr:supportRatio green ratio
```

项目同时提供了可直接加载的曲线配置：

```text
call Plots/SimpleStandFSR.con
```

压力图纵轴上限 `6` 只是 NAO 仿真的初始显示范围，可根据实际曲线调整；不能把绘图范围当作控制阈值。

## 8. 预期时序

当前横向轨迹每个阶段持续 `weightShiftPhaseDuration = 2000 ms`：

| 阶段 | 躯干目标 | 压力预期 | 比例预期 |
| --- | --- | --- | --- |
| 0 | 中心 → 左 | 左脚上升，右脚下降 | 向正值变化 |
| 1 | 左 → 中心 | 两脚逐渐接近 | 回到 0 附近 |
| 2 | 中心 → 右 | 右脚上升，左脚下降 | 向负值变化 |
| 3 | 右 → 中心 | 两脚逐渐接近 | 回到 0 附近 |

低通滤波会带来一定时间延迟，因此曲线极值不一定与躯干位置命令同时出现。验收重点是方向正确、周期一致、数值稳定，而不是要求零延迟。

## 9. 实施顺序

### 阶段 A：接入数据

- 为模块增加 `FsrSensorData` 依赖；
- 为 SimpleStand 场景增加数据提供者；
- 编译并确认模块依赖无错误。

完成标志：SimRobot 能正常启动 `SimpleStand` 场景。

### 阶段 B：原始求和与有效性检查

- 读取八个传感器；
- 计算左右脚原始总压力；
- 暂时观察原始值是否非零、是否随接触变化。

完成标志：站立时左右脚都有合理的正压力。

### 阶段 C：滤波与支撑比例

- 加入低通滤波；
- 加入最小总压力保护；
- 计算 `supportRatio`；
- 输出三条正式调试曲线。

完成标志：曲线连续，没有 NaN、无穷大或明显的逐帧尖峰。

### 阶段 D：左右重心转移验收

- 保持双脚固定；
- 连续观察至少三个完整左右移动周期；
- 比较 `currentTorsoShiftY` 与 `supportRatio`；
- 保存曲线截图或日志作为验收记录。

完成标志：左右移动时支撑比例方向正确且重复性良好。

## 10. 验收标准

### 10.1 构建与运行

- `SimpleStandEngine` 编译通过；
- `SimpleStand` 场景能加载；
- 不出现 representation provider 缺失；
- 原有站立、IMU PD 和左右移动行为不退化。

### 10.2 数据正确性

- 左脚总压力等于左脚四个有效传感器之和；
- 右脚总压力等于右脚四个有效传感器之和；
- 两个滤波压力均为有限非负数；
- 有地面接触时 `supportRatio` 始终位于 `[-1, 1]`；
- 无有效接触时不产生除零、NaN 或错误支撑判断。

### 10.3 动态表现

- 身体向左移动：左脚压力上升、右脚压力下降、`supportRatio > 0`；
- 身体处于中心：两脚压力接近、`supportRatio` 接近 0；
- 身体向右移动：右脚压力上升、左脚压力下降、`supportRatio < 0`；
- 连续三个周期中的变化方向一致；
- 滤波后曲线不会因单帧噪声频繁跨过 0。

本阶段不强制规定“接近 0”的数值范围和未来单脚支撑阈值。先采集仿真数据，再根据曲线分布确定阈值，避免凭感觉填写常数。

## 11. 风险与处理

| 风险 | 表现 | 处理方式 |
| --- | --- | --- |
| 未配置数据提供者 | 场景启动时依赖解析失败 | 在 `threads.cfg` 注册 `FsrSensorData` |
| 传感器值无效 | 压力为 off、NaN 或负数 | 本帧标记无效，不更新滤波和控制判断 |
| 分母接近 0 | 比例突然变大或出现 NaN | 使用 `minTotalPressure` 做接触保护 |
| 滤波太弱 | 曲线尖峰多 | 增大 `fsrLowPassRatio` |
| 滤波太强 | 曲线严重滞后 | 减小 `fsrLowPassRatio` |
| 左右符号相反 | 向左时比例为负 | 先核对腿枚举和位移坐标，不直接颠倒公式 |
| 改动参数过多 | 无法定位结果变化原因 | 第一轮只接入 FSR，不调整站姿和 PD |

## 12. 后续阶段接口

只有本规划书的验收标准全部通过，才进入“抬脚 5 mm”阶段。后续判定不能只看单帧 `supportRatio`，应至少包含：

```text
fsrValid == true
总压力有效
supportRatio 超过目标阈值
条件持续一段确认时间
IMU 倾角仍在安全范围内
```

例如准备抬右脚时，需要先确认左脚已经稳定承受大部分重量。具体阈值和确认时间必须根据本阶段保存的曲线确定，不能在尚未测量时提前固定。

进入下一阶段后仍应保留以下安全限制：

- 首次只抬高 5 mm；
- 抬脚过程使用平滑插值；
- 支撑比例不足立即停止继续抬高；
- IMU 倾斜超过安全值时落回双脚支撑；
- 不改变现有脚踝修正上限 `±3°`。

## 13. 完成清单

- [x] `SimpleStandEngine` 引入并依赖 `FsrSensorData`
- [x] `threads.cfg` 注册 FSR 数据提供者
- [x] 读取左脚四个压力传感器
- [x] 读取右脚四个压力传感器
- [x] 检查八个输入值有效性
- [x] 计算左右脚原始总压力
- [x] 实现左右脚压力低通滤波
- [x] 实现最小总压力保护
- [x] 计算 `supportRatio`
- [x] 输出 `leftPressure`
- [x] 输出 `rightPressure`
- [x] 输出 `supportRatio`
- [x] 构建通过
- [x] 场景启动通过
- [ ] 连续观察至少三个左右移动周期
- [ ] 压力变化方向通过验收
- [ ] 保存曲线证据
- [x] 本阶段不抬脚、不接入 RL

## 14. 阶段 D 试验记录

### 14.1 横向位移 5 mm

- 左右脚压力均约为 2 kg，总压力稳定；
- 左右压力变化方向相反，滤波后曲线平滑；
- `supportRatio` 约在 `-0.03` 到 `+0.025` 之间变化；
- 正负方向和回零表现正确，证明 FSR 测量链路有效；
- 最大承重脚占比只有约 51.5%，不足以进入抬脚阶段。

### 14.2 下一轮：横向位移 10 mm

- 将 `weightShiftAmplitude` 从 5 mm 提高到 10 mm；
- 双脚继续固定，不改变 `standHeight`、PD 参数和移动周期；
- 曲线缓冲区提高到 2500 个采样点，可覆盖至少三个完整周期；
- `supportRatio` 显示范围缩小到 `[-0.1, 0.1]`；
- 如果机器人出现明显倾斜、脚底滑动或曲线振荡，应立即停止并恢复到 5 mm；
- 本轮仍只采集数据，不允许抬脚。
