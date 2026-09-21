# SimRobot中查看和修改配置参数指南

## 一、查看配置参数的方法

### 方法1：使用 `get` 命令查看模块参数

在SimRobot控制台中输入：

```bash
# 查看球衣分类器的所有参数
get module:JerseyClassifierProvider2020For2023

# 或者查看特定参数
get module:JerseyClassifierProvider2020For2023:grayRange
get module:JerseyClassifierProvider2020For2023:scanWidthRatio
get module:JerseyClassifierProvider2020For2023:satThreshold
```

**输出示例**：
```
module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.35; max = 0.67; }
module:JerseyClassifierProvider2020For2023:scanWidthRatio = 0.7
module:JerseyClassifierProvider2020For2023:satThreshold = 140
```

---

### 方法2：使用 `mr` 命令查看模块请求

```bash
# 查看模块请求信息
mr module:JerseyClassifierProvider2020For2023
```

这会显示模块的状态、提供的表示（representation）等信息。

---

### 方法3：通过视图面板查看

#### 步骤1：打开模块视图

在SimRobot菜单中：
```
View → Module → JerseyClassifierProvider2020For2023
```

或者在左侧面板中展开：
```
robot1 (或其他机器人)
  └─ modules
      └─ JerseyClassifierProvider2020For2023
```

#### 步骤2：查看参数

双击模块名称，会打开一个窗口显示所有参数。

---

### 方法4：查看配置文件路径

```bash
# 查看当前加载的配置文件
get representation:RobotInfo

# 查看场景配置
get representation:ScenarioInfo
```

这会告诉你当前使用的是哪个Location的配置（如Default, TeenSizeField等）。

---

## 二、修改配置参数的方法

### 方法1：使用 `set` 命令临时修改（推荐用于测试）

在SimRobot控制台中：

```bash
# 修改单个参数
set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.45; max = 0.67; }
set module:JerseyClassifierProvider2020For2023:scanWidthRatio = 0.8
set module:JerseyClassifierProvider2020For2023:satThreshold = 150

# 修改后立即生效，无需重启
```

**特点**：
- ✅ 立即生效
- ✅ 适合快速测试
- ❌ 重启SimRobot后失效
- ❌ 不会保存到配置文件

---

### 方法2：修改配置文件（推荐用于永久修改）

#### 步骤1：找到配置文件

配置文件位置：
```
Config/Locations/Default/jerseyClassifierProvider2020For2023.cfg
```

或者根据你的场景：
```
Config/Locations/TeenSizeField/jerseyClassifierProvider2020For2023.cfg
Config/Locations/SimulatorAutomaticCalibration/jerseyClassifierProvider2020For2023.cfg
```

#### 步骤2：编辑配置文件

使用文本编辑器打开并修改：

```cfg
// 示例：调整黑色识别阈值
grayRange = { min = 0.45; max = 0.67; };  // 原值 0.35

// 示例：调整扫描宽度
scanWidthRatio = 0.8;  // 原值 0.7

// 示例：调整饱和度阈值
satThreshold = 150;  // 原值 140
```

#### 步骤3：重新加载配置

在SimRobot中：

**方法A：重启SimRobot**
```bash
# 关闭SimRobot
# 重新启动
./SimRobot
```

**方法B：重新加载场景**
```
File → Open → 重新选择场景文件
```

**方法C：使用reload命令（如果支持）**
```bash
reload module:JerseyClassifierProvider2020For2023
```

---

### 方法3：使用 `save` 命令保存当前参数

如果你在SimRobot中用 `set` 命令修改了参数，可以保存到文件：

```bash
# 保存当前模块的所有参数
save module:JerseyClassifierProvider2020For2023
```

这会将当前参数保存到配置文件中。

---

## 三、实时查看参数效果

### 启用调试绘制

```bash
# 启用球衣分类调试绘制
vid upper module:JerseyClassifierProvider2020For2023:jerseyClassification
vid upper module:JerseyClassifierProvider2020For2023:jersey
vid upper module:JerseyClassifierProvider2020For2023:jerseyWeights
vid upper module:JerseyClassifierProvider2020For2023:branch4Stage
```

**显示内容**：
- 每个检测到的机器人旁边会显示：
  ```
  Havg=XX   (平均色调)
  Savg=XX   (平均饱和度)
  Iavg=XX   (平均亮度)
  Hcol=XX   (彩色像素的平均色调)
  sat=XX/YY (彩色像素数/总像素数)
  
  own: XX           (识别为队友的像素数)
  opponent: XX      (识别为对手的像素数)
  class: own field player  (最终分类结果)
  ```

### 实时调整参数并观察

```bash
# 1. 启用调试绘制
vid upper module:JerseyClassifierProvider2020For2023:jerseyClassification

# 2. 修改参数
set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.40; max = 0.67; }

# 3. 观察图像上的分类结果变化

# 4. 继续调整
set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.45; max = 0.67; }

# 5. 找到最佳值后保存
save module:JerseyClassifierProvider2020For2023
```

---

## 四、常用参数调整场景

### 场景1：黑色球衣识别不到

**症状**：黑色球衣被识别为 `unknown` 或误识别为对手

**解决方法**：
```bash
# 提高黑色亮度阈值
set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.45; max = 0.67; }

# 或者提高饱和度阈值
set module:JerseyClassifierProvider2020For2023:satThreshold = 150
```

**原理**：
- `grayRange.min` 越高，越多的暗像素被识别为黑色
- `satThreshold` 越高，越多的"高伪饱和度"暗像素被接受为黑色

---

### 场景2：彩色球衣被误识别为黑色

**症状**：黄色、蓝色等彩色球衣被错误识别为黑色

**解决方法**：
```bash
# 降低黑色亮度阈值
set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.30; max = 0.67; }

# 或者降低饱和度阈值
set module:JerseyClassifierProvider2020For2023:satThreshold = 120
```

---

### 场景3：手臂颜色干扰识别

**症状**：白色或彩色手臂影响球衣识别

**解决方法**：
```bash
# 缩小扫描宽度，排除手臂
set module:JerseyClassifierProvider2020For2023:scanWidthRatio = 0.6

# 或者更激进
set module:JerseyClassifierProvider2020For2023:scanWidthRatio = 0.5
```

---

### 场景4：识别不稳定，频繁切换

**症状**：同一个机器人一会儿识别为队友，一会儿识别为对手

**解决方法**：
```bash
# 提高最小球衣比例要求
set module:JerseyClassifierProvider2020For2023:minJerseyRatio = 0.5

# 提高最小像素占比
set module:JerseyClassifierProvider2020For2023:minJerseyWeightRatio = 0.15
```

---

## 五、查看实时识别数据

### 方法1：查看ObstaclesFieldPercept

```bash
# 查看场上障碍物识别结果
get representation:ObstaclesFieldPercept
```

**输出示例**：
```
obstacles = [
  {
    center = {x = 1200; y = 300;};
    type = ownPlayer;        // 识别为队友
    confidence = 0.85;
  },
  {
    center = {x = 2000; y = -500;};
    type = opponentPlayer;   // 识别为对手
    confidence = 0.92;
  },
  {
    center = {x = 1500; y = 0;};
    type = unknown;          // 未识别
    confidence = 0.0;
  }
];
```

---

### 方法2：查看GlobalOpponentsModel

```bash
# 查看全局对手模型
get representation:GlobalOpponentsModel
```

这会显示所有识别到的对手位置和状态。

---

### 方法3：使用日志记录

```bash
# 开始记录日志
log start

# 进行测试...

# 停止记录
log stop

# 日志会保存在 Config/Logs/ 目录下
```

然后可以用之前创建的 `analyze_jersey_log.py` 分析日志。

---

## 六、完整的调试工作流

### 步骤1：启动SimRobot并加载场景

```bash
cd Build/Linux/SimRobot/Develop
./SimRobot

# 加载场景
File → Open → Config/Scenes/Game.ros3
```

### 步骤2：启用调试绘制

```bash
vid upper module:JerseyClassifierProvider2020For2023:jerseyClassification
vid upper module:JerseyClassifierProvider2020For2023:jersey
vid field representation:ObstaclesFieldPercept
```

### 步骤3：查看当前参数

```bash
get module:JerseyClassifierProvider2020For2023
```

### 步骤4：开始比赛并观察

```bash
gc ready
gc set
gc playing
```

观察图像上的识别结果和统计数据。

### 步骤5：调整参数

根据观察到的问题，使用 `set` 命令调整参数：

```bash
# 示例：黑色识别不到
set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.45; max = 0.67; }

# 观察效果...

# 继续调整
set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.50; max = 0.67; }
```

### 步骤6：保存最佳参数

找到最佳参数后：

```bash
# 方法1：使用save命令
save module:JerseyClassifierProvider2020For2023

# 方法2：手动编辑配置文件
# 编辑 Config/Locations/Default/jerseyClassifierProvider2020For2023.cfg
```

---

## 七、快速参考表

### 常用命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `get module:XXX` | 查看模块参数 | `get module:JerseyClassifierProvider2020For2023` |
| `set module:XXX:param = value` | 修改参数 | `set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.45; max = 0.67; }` |
| `save module:XXX` | 保存参数到文件 | `save module:JerseyClassifierProvider2020For2023` |
| `vid upper module:XXX:drawing` | 启用调试绘制 | `vid upper module:JerseyClassifierProvider2020For2023:jerseyClassification` |
| `get representation:XXX` | 查看表示数据 | `get representation:ObstaclesFieldPercept` |

### 关键参数速查

| 参数 | 默认值 | 作用 | 调整建议 |
|------|--------|------|----------|
| `grayRange.min` | 0.35 | 黑色亮度上限 | 识别不到黑色→调高；误判彩色为黑色→调低 |
| `satThreshold` | 140 | 黑色饱和度上限 | 识别不到黑色→调高；误判彩色为黑色→调低 |
| `scanWidthRatio` | 0.7 | 扫描宽度比例 | 手臂干扰→调低；采样不足→调高 |
| `hueSimilarityThreshold` | 30 | 色调相似度 | 识别率低→调高；误判多→调低 |
| `minJerseyRatio` | 0.35 | 最小优势比例 | 识别不稳定→调高；识别率低→调低 |

---

## 八、故障排查

### 问题1：`set` 命令不生效

**可能原因**：
- 参数名称错误
- 语法错误

**解决方法**：
```bash
# 先用 get 确认参数名称
get module:JerseyClassifierProvider2020For2023

# 确保语法正确（注意空格和分号）
set module:JerseyClassifierProvider2020For2023:grayRange = { min = 0.45; max = 0.67; }
```

### 问题2：修改配置文件后不生效

**可能原因**：
- 配置文件路径错误
- 没有重新加载

**解决方法**：
```bash
# 确认当前使用的Location
get representation:RobotInfo

# 修改对应Location的配置文件
# 例如：Config/Locations/Default/jerseyClassifierProvider2020For2023.cfg

# 重新加载场景
File → Open → 重新选择场景
```

### 问题3：调试绘制不显示

**可能原因**：
- 绘制层未启用
- 图像视图未打开

**解决方法**：
```bash
# 确保图像视图已打开
View → Image → Upper (或 Lower)

# 启用调试绘制
vid upper module:JerseyClassifierProvider2020For2023:jerseyClassification

# 检查绘制层是否启用
# 在图像视图右键 → Drawings → 确认勾选
```

---

## 九、推荐的调试流程

```
1. 启动SimRobot
   ↓
2. 加载场景
   ↓
3. 启用调试绘制
   ↓
4. 查看当前参数
   ↓
5. 开始比赛
   ↓
6. 观察识别结果
   ↓
7. 使用set命令调整参数
   ↓
8. 观察效果
   ↓
9. 重复6-8直到满意
   ↓
10. 保存参数到配置文件
```

---

**提示**：在调整参数时，建议每次只改一个参数，观察效果后再调整下一个，这样更容易找到最佳配置。
