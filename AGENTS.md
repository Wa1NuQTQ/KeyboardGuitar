# KeyboardGuitar 开发约定与架构

本文件约束当前项目中的开发工作。目标是让开发者能够顺着“输入 → 演奏 → 声音与人物反馈”理解功能，并逐步完成自由演奏、跟曲练习，最后加入 Guitar Pro 曲谱导入。

## 1. 项目入口与当前状态

- 项目根目录：本文件所在目录；入口为 `KeyboardGuitar.uproject`。
- 已核实环境（2026-10-13）：Windows、UE 5.8.3；项目关联版本为 `5.8`。
- 当前只有一个运行时模块 `Source/KeyboardGuitar`，源码仍为引擎模板；下文的业务类是待实现设计，不代表已存在。
- 构建目标：`KeyboardGuitar`（Game）、`KeyboardGuitarEditor`（Editor），定义在 `Source/*.Target.cs`。
- `KeyboardGuitar.Build.cs` 已包含 `Core`、`CoreUObject`、`Engine`、`InputCore`、`EnhancedInput`；输入配置已使用 Enhanced Input。
- 当前启动地图：`/Game/Art/Maps/L_Classroom`。教室资源在 `/Game/Art/Scene`；`/Game/Art/Character` 已建目录，尚无已保存的人物资源。
- 当前渲染配置采用 Lumen、Virtual Shadow Maps，并关闭静态光照。业务功能开发不要顺带重置已调整的灯光、曝光、材质或摄像机。
- 用户选择从原始下载包中的 optimized FBX 导入人物；资源名为 `cgtrader_optimized_Anime+Girl+Kawaii.fbx`。导入后验证比例、骨骼、蒙皮及表情，不自动替换成旧项目处理过的角色。
- 同级 `KeyboardGuitar_old` 是参考工程，只读参考；当前项目不依赖其代码、资产路径、生成文件或项目外脚本。
- 检查时当前目录不是 Git 仓库。修改前重新确认版本管理状态，不能假设可以通过 Git 回退。

## 2. 设计原则

- 单机、一个主要演奏场景，先做能演奏的一条完整功能链。暂不实现联网、多人、通用乐器框架或插件化业务。
- 保持一个运行时模块，按功能分目录。只有真实的编辑器专用功能出现时，才考虑增加 Editor 模块。
- C++ 实现音符规则、输入响应、会话状态和判定；蓝图负责资源配置、可视化动画图和界面布局；关卡负责场景摆放。
- 直接使用 UE 的 GameMode、PlayerController、Pawn、Component、DataAsset、AnimInstance 和 UMG。不要为一个局部功能建立自定义全局 Subsystem、事件总线、服务定位器或通用 Manager。
- 下文名称是首次实现的默认方案。创建类的同时完成其可验证的用途，不提前创建空目录、空接口和转发类。
- 调整架构时同步更新本文件，说明新的职责归属；不维护多份相互冲突的架构说明。

## 3. 源码和资源组织

源码按功能放置，同一功能的 `.h`、`.cpp` 相邻，沿用当前模块布局；有跨模块公开接口需求时再引入 Public/Private 分层。

```text
Source/KeyboardGuitar/
  KeyboardGuitar.Build.cs / KeyboardGuitar.h / KeyboardGuitar.cpp
  Game/          GameMode、PlayerController、输入与界面装配
  Instrument/    音符类型、吉他状态、拨弦规则、乐器配置
  Performer/     演奏者 Pawn、动画状态与人物适配
  Audio/         吉他发声、每弦声音生命周期
  Practice/      第二阶段：曲目、播放进度、匹配判定
  UI/            第二阶段按需：有实际逻辑的 Widget C++ 类
  Import/        第三阶段按需：外部谱格式转换
```

```text
Content/
  Art/
    Maps/                           保留现有 L_Classroom 位置
    Scene/{Meshes,Materials,Textures,FX}/
    Character/<角色名>/{Meshes,Materials,Textures,Animations}/
    Guitar/{Meshes,Materials,Textures}/
  Game/
    Core/                           GameMode、PlayerController 蓝图子类
    Input/                          IA_*、IMC_*
    Performer/                      人物 Pawn 蓝图、演奏动画蓝图/Control Rig
    Data/                           吉他配置、和弦表、第二阶段的曲目数据
    UI/                             WBP_* 界面
  Audio/Guitar/                     吉他采样、SoundWave、按需使用的 MetaSound
```

- `Art` 保存模型、贴图、材质和原始动画；`Game` 保存本游戏的行为与配置资产。引入完整资源包时保留其必要依赖。
- 常用命名前缀：`L_` 地图、`BP_` 蓝图、`SK_` 骨骼网格、`SM_` 静态网格、`M_` 材质、`MI_` 材质实例、`T_` 贴图、`ABP_` 动画蓝图、`DA_` 数据资产、`WBP_` 界面。
- 新建资产采用这些约定；不为统一名称批量重命名已有第三方资源。
- `.uasset/.umap` 只通过 UE 编辑器或引擎脚本编辑、移动和修复引用。不要用资源管理器移动已引用资产，不手改二进制。
- `Binaries`、`Intermediate`、`Saved`、`DerivedDataCache`、`.vs`、生成的解决方案不是源码。不要因“整理架构”删除整个生成目录或清空用户的备份。

## 4. 第一阶段：自由演奏的职责

| 类型（待实现） | 拥有的状态与职责 | 边界 |
|---|---|---|
| `AKeyboardGuitarGameMode : AGameModeBase` | 指定 Controller 和默认 Pawn 类，使用引擎出生流程 | 不保存和弦、声音、手指姿态 |
| `AKeyboardGuitarPlayerController` | Enhanced Input 绑定、界面输入模式、创建本地 UI；第二阶段拥有练习组件 | 将按键变成演奏命令，不计算音高或骨骼变换 |
| `AGuitarPerformerPawn` | 人物 SkeletalMesh、吉他 StaticMesh、演奏 CameraComponent；拥有下方乐器与声音组件，连接表现反馈 | 初期坐姿演奏用 Pawn，不额外引入 CharacterMovement；不负责判分 |
| `UGuitarInstrumentComponent` | 六弦当前品位/制音状态、和弦切换、拨弦/扫弦规则，产生确定的发声音符 | 不认识键盘键位、Widget、人物骨名或导入文件 |
| `UGuitarAudioComponent` | 从有效音符触发声音，管理重拨、停止、衰减和每弦发声实例 | 使用现有 UE 音频能力；不推断和弦、不驱动谱面进度 |
| `UGuitarDefinition : UDataAsset` | 调弦、可用品位、和弦指法和音色引用等静态配置 | 不存本局按键状态、分数和音频播放实例 |

- `GameMode` 在 `PlayerStart` 生成 `BP_GuitarPerformer` 并由 Controller 占有。场景不要同时放置另一个自动占有的人物；需要预览时与运行时实例明确区分。
- 可调的骨骼网格、吉他、动画类、镜头相对位置和组件配置通过蓝图默认值/反射属性指定。先使用 Pawn 的相机；只有出现多个镜头切换需求时再扩展。
- 乐器组件是弦/品位状态的唯一写入者；动画和界面读取该状态，不各自维护另一份和弦规则。
- Pawn 绑定乐器的有效演奏事件，将同一份结果交给音频组件和动画状态。第二阶段练习组件也订阅此事件。命令用直接调用，多接收方通知用组件委托，不广播全局事件。
- 对象依赖在构造、初始化或占有时显式连接；不每帧搜索 Actor，不把必需引用缺失默默隐藏。错误指出具体缺失资产或属性。

数据流：

```text
Enhanced Input → PlayerController → InstrumentComponent
                                      ├─ Pawn → AudioComponent 发声
                                      ├─ Pawn/AnimInstance → 动画与手部反馈
                                      └─ 第二阶段 PracticeSession → 判定 → UI
```

### 音符与输入约定

- 内部 `StringIndex` 固定为 `0..5`，分别对应低音 E2 到高音 E4；标准调弦 MIDI 值为 `[40,45,50,55,59,64]`。谱面通常从上到下显示高音到低音，由 UI 显式转换。
- `Fret=0` 表示空弦；制音用明确的布尔字段或状态表达，不与负品位混用。音高由调弦 MIDI 值加品位得到；上限读取乐器配置。
- 有效演奏记录至少包含弦、品位、MIDI 音高、`Velocity01`、输入发生时间。时间字段带 `Seconds`/`Milliseconds` 后缀，力度明确为 `0..1`。
- 生成后将本次演奏记录作为快照传给声音、动画和判定；后续和弦变化不能重新解释已经触发的音符。
- 多弦动作作为一组命令处理，保留每弦发声时序；扫弦间隔是有单位的参数。不能因为和弦切换而让本应静止的弦自动发声。
- 单次拨弦使用输入边沿事件，避免 `Triggered` 持续调用造成每帧重复拨弦。释放和取消事件负责解除保持状态；切菜单、失焦时清理按键状态。
- 第一版每根弦保持一个主要发声实例，并确定重拨时旧声音的停止/淡出规则。音色变化先用采样与现有 AudioComponent 实现，确有听感需求再扩展 MetaSound 或合成。

### 人物和动画

- 先完成比例、朝向、材质、骨架和静态坐姿，再绑定吉他，最后实现手臂、手指和拨弦反馈。
- AnimBlueprint/Control Rig 用于可视化调整姿态与 IK；C++ 提供演奏状态和必要计算。先采用引擎动画流程，不直接复制旧项目整套自定义骨骼求值代理。
- 游戏语义（第几弦、第几品）不包含模型骨名。骨名、手掌轴向、局部偏移放在人物配置中；第二个角色出现前不设计通用重定向框架。
- 吉他默认固定在 Pawn/躯干的稳定挂点，手部追踪吉他上的目标；避免吉他追踪手、手又追踪吉他的循环约束。
- 明确每个目标是吉他局部、网格组件还是世界空间。姿态只影响显示，动画通知不能成为拨弦发声或判定的唯一触发源。
- 动画并行求值只读取已准备好的状态快照，通过引擎支持的线程安全数据传递方式更新；工作线程不直接遍历关卡或读写 Actor/组件。

## 5. 第二阶段：跟曲练习

- 先使用手工创建的短曲目，再做长曲和外部导入。新增 `UGuitarSong : UDataAsset`，保存已排序的音符事件与可选伴奏引用。
- 第一版事件包含 `StartTimeSeconds`、`DurationSeconds`、`StringIndex`、`Fret`、`Velocity01`；同一时间可有多个不同弦的音符。数组下标在一次会话内可作为稳定匹配标识。
- 外部格式的拍号、变速等信息最终转换到该时间轴；运行时和 UI 不直接读取 GP 文件。第一版只支持明确实现的单轨品位音符，滑音、连音等技巧不假装已支持。
- `UPracticeSessionComponent` 由 PlayerController 拥有，统一管理当前曲目、`Stopped/Playing/Paused/Finished` 状态、播放位置、命中记录和结果。自由演奏不依赖它存在。
- 会话连接当前 Pawn 的乐器事件；重新占有、重开关卡或结束会话时先解绑旧对象，避免重复判定。
- 只有会话维护对外的曲目时间，谱面读取 `GetSongTimeSeconds()` 一类接口。先无伴奏时可采用单调时钟减去起点与暂停时间；不由每个 Widget 累加 DeltaSeconds。
- 加入伴奏/节拍器时，先实现并验证统一音频传输时钟与调度，优先使用引擎 Quartz 等现有机制。不要假设普通 Tick、Timer、动画通知或 Play 调用具有音频采样精度。Quartz 回调送达游戏线程的时间也不能直接当作音符实际播放时间。
- 实时拨弦立即响应，不为“同步”而强制等到下一拍。输入捕获时间映射到同一曲目时间轴，音频延迟校准与谱面视觉偏移使用不同参数。
- 判定从相同弦/品位且未处理的候选事件中，选时间误差最小的一个；每个事件最多命中一次，超过窗口统一标记 Miss。窗口使用毫秒，评分规则与 UI 文案分离。
- 暂停冻结曲目进度并暂停相关音频；重新开始清空判定、声音和动画瞬态。跳转/循环到实际需求阶段再实现，并同步重置游标和匹配记录。
- 切换曲目前先验证新曲目与所需资源；验证通过后统一切换时间轴、音频和谱面，避免部分切换。失败时保留可理解的会话状态并显示具体原因。
- UMG 负责显示谱面和结果、提交开始/暂停请求。可按帧绘制可见音符窗口，不复制整份曲谱、不每帧创建全部音符 Widget；首版只做一个清晰的谱面控件。

## 6. 第三阶段：曲谱导入

- 先明确一个支持的 GP 格式版本和一个吉他轨道，不一次支持所有变体。
- 转换器接收外部文件，输出与手写曲目同一结构的内部数据及可读诊断；文件选择与解析、游戏演奏各有边界。
- 在文件边界限制大小、事件数量并检查弦/品位、时间和排序。不支持的技巧或损坏文件必须报告，不能静默生成错误曲谱。
- 编辑器内生成资产的工具放入 Editor 模块；打包游戏运行时导入只使用运行时依赖，不引用 UnrealEd、AssetTools 或 Python 编辑器插件。
- 引入解析库前检查许可、现有能力、维护和打包支持；开发顺序未到此阶段时不添加依赖或解析框架。

## 7. 实现顺序与验收

每一步交付可运行结果，再进入下一步。用户以自己编写为主：请求指导时解释当前一步，请求实现时只实现授权范围，不自动补完后续所有阶段。

1. **人物落地**：导入所选角色，确认尺寸、骨架、材质；放到教室中，保存坐姿与镜头配置。
2. **一根弦**：建立 GameMode、Controller、Pawn；按一次键只发一个音，松开/失焦没有卡键。
3. **完整自由演奏**：六弦、品位/和弦、重拨/制音、扫弦；声音、界面提示和手部目标读取同一乐器状态。
4. **表现完善**：角色持琴、左右手姿态、呼吸/眨眼；从最终镜头检查穿模和关节，不以骨骼数字正确代替视觉检查。
5. **短曲练习**：手写一段包含单音、同拍多弦、休止的曲目；验证开始、暂停、重开、命中和 Miss，再接伴奏同步。
6. **导入**：用真实小文件与手写曲谱对比，同时验证不支持/损坏输入及打包后的运行。

## 8. 编码、资源与验证规则

- 遵循 Epic C++ 风格与现有格式：`U/A/F/E` 类型前缀、`b` 布尔前缀、`generated.h` 最后包含；名称说明职责和单位。
- UObject 的强持有使用反射属性及适用的 `TObjectPtr`，非拥有缓存按生命周期选弱引用。事件/定时器在销毁或重新绑定时清理；异步完成前核对目标仍有效。
- 静态配置用 DataAsset/蓝图默认值；本局状态放实际拥有它的对象。不要将运行时状态写回资产、全局静态变量或 Level Blueprint。
- 数据类型优先小型 `USTRUCT`/普通函数。只有已出现的重复或多实现需求才引入接口/继承层次，不为“架构完整”拆出一串转发类。
- 默认关闭无用途的 Tick；手部插值、谱面滚动等持续表现才开启。输入回调中不做磁盘读取、文件解析或首次同步加载大资源。
- 新模块依赖在功能需要时添加：例如实现 UMG 时增加相应依赖，实现音频调度时核对其所属模块。仅实现文件需要的依赖放 Private，公开头文件确需的依赖才放 Public。
- 借鉴旧工程时先读相关逻辑，复用已理解的公式或数据。姿态偏移、导入比例、灯光、镜头、音频延迟都必须在新工程验证，不照搬旧机器绝对路径和补丁代码。
- 原始下载文件可保留在项目外；导入后的游戏运行必须只依赖项目/引擎已打包资源。源包或第三方素材的再分发遵守其许可，不随源码公开原始文件。
- 下载优先使用缓存，首次请求显式直连，失败后才使用已有代理；不修改系统代理。本地已有的引擎与资源不重复下载。
- 新测试代码、验证脚本、样例和报告使用系统临时目录下唯一的 `codex-tests-<任务名>-<唯一标识>` 子目录，不放主工程或提交。UE 正常构建所需生成目录保持工具默认约定。
- 缺陷验证关注正常、边界及失败行为。音符转换可在临时独立入口检查；输入/音频/动画必须在编辑器或游戏中验证，低帧率、暂停和重复进入 PIE 时仍应一致。
- 改动前检查工作区状态并保护用户已有修改。需要离线改资产时确认用户已保存并关闭编辑器；只读检查磁盘资产不能代表未保存的编辑器状态。
- 完成后报告修改、实际验证和未覆盖项。NullRHI 加载成功只能证明加载/部分引用检查，不能证明视觉、音频效果或帧率。

## 9. 本机常用命令

以下入口和目标名称已核对；不代表每次文档修改都需要构建。引擎路径是本机示例，其他机器先确认安装位置。普通资源调整不需要重新编译全部 C++；仅文档调整检查内容与链接即可。

PowerShell，在项目根目录执行：

```powershell
$projectFile = (Resolve-Path '.\KeyboardGuitar.uproject').Path
$engineRoot = 'C:\Program Files\Epic Games\UE_5.8'
& "$engineRoot\Engine\Build\BatchFiles\Build.bat" KeyboardGuitarEditor Win64 Development "-Project=$projectFile" -WaitMutex
```

打包前，涉及运行时模块/依赖时另构建 Game 目标：

```powershell
& "$engineRoot\Engine\Build\BatchFiles\Build.bat" KeyboardGuitar Win64 Development "-Project=$projectFile" -WaitMutex
```

构建前确认编辑器和 Live Coding 状态；修改反射声明/类布局后使用完整构建并重新打开编辑器验证。编辑器启动入口为 `$engineRoot\Engine\Binaries\Win64\UnrealEditor.exe`，参数为 `$projectFile`。构建与测试看实际退出码及日志，不以命令已发出判断成功。

打包、自动化测试和外部曲谱解析命令尚未建立；实现相应阶段后记录已实际验证的命令，不编造当前不存在的测试入口。

## 10. 按需查阅的官方资料

- [Gameplay Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine)：理解 GameMode、Controller、Pawn 的职责。
- [Enhanced Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)：配置输入动作与映射。
- [Epic C++ Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)：命名、反射与代码风格。
- [Quartz Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-quartz-in-unreal-engine)：第二阶段理解音频时序与调度。

资料用于解决当前步骤的问题，不将大型示例工程的分层、联网或插件体系整体搬入本项目。
