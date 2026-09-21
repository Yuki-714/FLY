# 运动控制模块预留区

此目录有意不从 `FLY-913` 复制运动控制实现，供后续自行搭建。

当前从最小可验证目标开始实现：

- `SimpleStandEngine`：零刚度、保持当前姿态、平滑进入固定站姿、稳定保持。
- `Config/Scenarios/SimpleStand`：只加载站立所需模块的独立场景。
- `Config/Scenes/SimpleStand.ros3`：NAO 的站立验证仿真。

已保留的集成边界：

- `Src/Representations/MotionControl`：运动控制与其他模块共享的数据表示和接口。
- `Src/Tools/Motion`：通用运动工具。
- `Config` 中的运动相关配置：用于后续实现接入和调试。
- `Make/CMake/B-Human.cmake` 中的原有构建接入点。

在补齐实现前，依赖运动控制模块的完整构建或运行配置可能无法通过，这是预期状态。

## 当前验证方式

```bash
./Make/Linux/generate
CCACHE_DIR=/tmp/fly-ccache ./Make/Linux/compile Develop SimRobot
./Build/Linux/SimRobot/Develop/SimRobot Config/Scenes/SimpleStand.ros3
```

`SimpleStand` 场景会依次执行：零刚度等待 1 秒、保持当前姿态并提升刚度 1 秒、
用 3 秒平滑插值到固定站姿，然后保持。胸口按钮可以在站立流程和零刚度状态之间切换。

此版本只允许 NAO 使用；T1/K1 会保持零刚度。真机使用前必须将 `autoStart` 改为
`false`，并在保护架和急停条件下重新验证刚度与关节限制。
