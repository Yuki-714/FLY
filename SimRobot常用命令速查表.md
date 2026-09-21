# SimRobot常用命令速查表

## 🎯 球衣分类器相关命令

### 查看参数

```bash
# 查看所有参数
get module:JerseyClassifierProvider2020For2023

# 查看特定参数
get module:JerseyClassifierProvider2020For2023:grayRange
get module:JerseyClassifierProvider2020For2023:scanWidthRatio
get module:JerseyClassifierProvider2020For2023:satThreshold
get module:JerseyClassifierProvider2020For2023:hueSimilarityThreshold
```

### 修改参数（临时，重启后失效）

```bash
# 调整黑色识别阈值
set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.45; max = 0.67; }

# 调整扫描宽度（排除手臂）
set module:JerseyClassifierProvider2020For2023:scanWidthRatio = 0.6

# 调整饱和度阈值
set module:JerseyClassifierProvider2020For2023:satThreshold = 150

# 调整色调相似度
set module:JerseyClassifierProvider2020For2023:hueSimilarityThreshold = 35

# 调整最小球衣比例
set module:JerseyClassifierProvider2020For2023:minJerseyRatio = 0.4
```

### 保存参数

```bash
# 保存当前参数到配置文件
save module:JerseyClassifierProvider2020For2023
```

### 启用调试绘制

```bash
# 显示识别结果和统计数据
vid upper module:JerseyClassifierProvider2020For2023:jerseyClassification

# 显示采样点颜色
vid upper module:JerseyClassifierProvider2020For2023:jersey

# 显示像素权重
vid upper module:JerseyClassifierProvider2020For2023:jerseyWeights

# 显示分支4的决策过程（彩色vs黑色）
vid upper module:JerseyClassifierProvider2020For2023:branch4Stage

# 显示球门扇区
vid upper option:KickAtGoal:goalSector

# 显示障碍物扇区轮
vid upper option:KickAtGoal:wheel
```

---

## 🎮 游戏控制命令

### 基本控制

```bash
# 游戏状态控制
gc initial          # 初始状态
gc ready            # 准备状态（READY）
gc set              # 设置状态（SET）
gc playing          # 比赛状态（PLAYING）
gc finished         # 结束状态

# 开球控制
gc kickOffFirstTeam   # 第一队开球
gc kickOffSecondTeam  # 第二队开球

# 进球
gc goalByFirstTeam    # 第一队进球
gc goalBySecondTeam   # 第二队进球
```

### 机器人控制

```bash
# 惩罚机器人
pr <robot_number>     # 惩罚指定机器人
ur <robot_number>     # 取消惩罚

# 示例
pr 1                  # 惩罚1号机器人
ur 1                  # 取消1号机器人的惩罚
```

---

## 📊 查看表示（Representation）

### 感知相关

```bash
# 查看障碍物识别结果
get representation:ObstaclesFieldPercept

# 查看球的位置
get representation:FieldBall

# 查看全局对手模型
get representation:GlobalOpponentsModel

# 查看机器人姿态
get representation:RobotPose

# 查看图像
get representation:Image
get representation:ECImage
```

### 行为相关

```bash
# 查看策略状态
get representation:StrategyStatus

# 查看技能请求
get representation:SkillRequest

# 查看行为状态
get representation:BehaviorStatus

# 查看游戏状态
get representation:GameState
```

### 运动相关

```bash
# 查看运动请求
get representation:MotionRequest

# 查看运动信息
get representation:MotionInfo

# 查看步态输出
get representation:WalkingEngineOutput
```

---

## 🔧 模块控制

### 查看模块信息

```bash
# 查看模块请求
mr module:JerseyClassifierProvider2020For2023

# 查看所有模块
mr list

# 查看特定模块的提供者
mr representation:ObstaclesFieldPercept
```

### 启用/禁用模块

```bash
# 禁用模块
mr off module:JerseyClassifierProvider2020For2023

# 启用模块
mr on module:JerseyClassifierProvider2020For2023

# 使用默认提供者
mr default representation:ObstaclesFieldPercept
```

---

## 📹 视图控制

### 打开视图

```bash
# 打开图像视图
v image upper         # 上摄像头图像
v image lower         # 下摄像头图像

# 打开场地视图
v field               # 场地俯视图

# 打开控制台
v console             # 控制台视图
```

### 调试绘制

```bash
# 启用绘制（vid = view drawing）
vid <view> <drawing_name>

# 示例
vid upper module:JerseyClassifierProvider2020For2023:jerseyClassification
vid field representation:ObstaclesFieldPercept
vid field module:StrategyBehaviorControl:setPlay

# 禁用绘制
vd <view> <drawing_name>

# 示例
vd upper module:JerseyClassifierProvider2020For2023:jerseyClassification
```

---

## 📝 日志控制

### 记录日志

```bash
# 开始记录
log start

# 停止记录
log stop

# 保存日志
log save <filename>

# 示例
log save test_jersey_recognition
```

### 回放日志

```bash
# 加载日志
log load <filename>

# 播放控制
log play              # 播放
log pause             # 暂停
log stop              # 停止
log step              # 单步前进
log step -1           # 单步后退
```

---

## 🎨 常用调试组合

### 调试球衣识别

```bash
# 1. 启用所有相关绘制
vid upper module:JerseyClassifierProvider2020For2023:jerseyClassification
vid upper module:JerseyClassifierProvider2020For2023:jersey
vid field representation:ObstaclesFieldPercept

# 2. 查看当前参数
get module:JerseyClassifierProvider2020For2023

# 3. 开始比赛
gc ready
gc set
gc playing

# 4. 观察并调整参数
set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.45; max = 0.67; }

# 5. 保存参数
save module:JerseyClassifierProvider2020For2023
```

### 调试开球策略

```bash
# 1. 查看当前策略
get representation:StrategyStatus

# 2. 启用站位绘制
vid field module:StrategyBehaviorControl:setPlay

# 3. 查看传球评估
vid upper module:PassEvaluationProvider:heatmap

# 4. 开始开球
gc ready
gc set
gc playing
```

### 调试射门

```bash
# 1. 启用射门相关绘制
vid upper option:KickAtGoal:goalSector
vid upper option:KickAtGoal:wheel
vid upper option:KickAtGoal:goalLineIntersection

# 2. 查看期望进球评估
vid field module:ExpectedGoalsProvider:heatmap

# 3. 开始比赛
gc playing
```

---

## 🔍 故障排查命令

### 检查系统状态

```bash
# 查看机器人信息
get representation:RobotInfo

# 查看场景信息
get representation:ScenarioInfo

# 查看帧信息
get representation:FrameInfo

# 查看相机信息
get representation:CameraInfo
```

### 检查通信

```bash
# 查看团队数据
get representation:TeamData

# 查看游戏控制器数据
get representation:GameControllerData
```

### 检查感知

```bash
# 查看球感知
get representation:BallPercept

# 查看线感知
get representation:LinesPercept

# 查看场地边界
get representation:FieldBoundary
```

---

## 💡 快捷键

### SimRobot窗口

| 快捷键 | 功能 |
|--------|------|
| `Ctrl + O` | 打开场景 |
| `Ctrl + S` | 保存 |
| `Ctrl + Q` | 退出 |
| `Space` | 暂停/继续 |
| `Ctrl + R` | 重置场景 |

### 图像视图

| 快捷键 | 功能 |
|--------|------|
| `+` / `-` | 放大/缩小 |
| `鼠标滚轮` | 缩放 |
| `鼠标拖拽` | 平移 |
| `右键` | 显示菜单 |

---

## 📋 参数调整速查

### 黑色球衣识别问题

| 问题 | 参数 | 调整方向 | 命令示例 |
|------|------|----------|----------|
| 识别不到黑色 | `grayRange.min` | 调高 | `set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.45; max = 0.67; }` |
| 识别不到黑色 | `satThreshold` | 调高 | `set module:JerseyClassifierProvider2020For2023:satThreshold = 150` |
| 彩色误判为黑色 | `grayRange.min` | 调低 | `set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.30; max = 0.67; }` |
| 彩色误判为黑色 | `satThreshold` | 调低 | `set module:JerseyClassifierProvider2020For2023:satThreshold = 120` |

### 手臂干扰问题

| 问题 | 参数 | 调整方向 | 命令示例 |
|------|------|----------|----------|
| 白色手臂干扰 | `scanWidthRatio` | 调低 | `set module:JerseyClassifierProvider2020For2023:scanWidthRatio = 0.6` |
| 采样不足 | `scanWidthRatio` | 调高 | `set module:JerseyClassifierProvider2020For2023:scanWidthRatio = 0.8` |

### 识别稳定性问题

| 问题 | 参数 | 调整方向 | 命令示例 |
|------|------|----------|----------|
| 频繁切换 | `minJerseyRatio` | 调高 | `set module:JerseyClassifierProvider2020For2023:minJerseyRatio = 0.5` |
| 识别率低 | `minJerseyRatio` | 调低 | `set module:JerseyClassifierProvider2020For2023:minJerseyRatio = 0.3` |

---

## 🎓 使用技巧

### 技巧1：批量执行命令

创建一个 `.con` 文件（如 `debug_jersey.con`）：

```bash
# debug_jersey.con
vid upper module:JerseyClassifierProvider2020For2023:jerseyClassification
vid upper module:JerseyClassifierProvider2020For2023:jersey
vid field representation:ObstaclesFieldPercept
get module:JerseyClassifierProvider2020For2023
```

在SimRobot中执行：
```bash
call debug_jersey.con
```

### 技巧2：使用别名

在控制台中定义别名（如果支持）：
```bash
alias jc "get module:JerseyClassifierProvider2020For2023"
alias jcd "vid upper module:JerseyClassifierProvider2020For2023:jerseyClassification"
```

### 技巧3：保存常用配置

创建场景配置文件，包含常用的调试设置。

---

**提示**：将这个速查表保存为书签，随时查阅！
