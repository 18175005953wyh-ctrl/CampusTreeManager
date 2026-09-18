# Campus Route Planner — 校园路径规划器

使用 C11、邻接矩阵和 Dijkstra 算法实现的命令行校园路径规划工具，输出完整路线和总距离。

## 项目背景与选择原因

环境设计关注建筑、道路和公共空间之间的联系。校园中的图书馆、广场、宿舍等地点可以抽象为图的顶点，道路可以抽象为边，道路长度就是边的权重。最短路径算法为这些空间联系提供了一种可计算的表达方式。

本项目把环境设计与数据结构学习结合起来：以一个虚构的校园为例，将空间关系录入文件，用程序回答“从某个地点到另一个地点，怎样走总距离更短”。它是算法练习与作品展示项目，示例距离不是实地测量数据，也不是实际导航服务。

该工具独立放在仓库的 `campus-route-planner/` 中，与原有两个项目并列。以下构建命令都从本子目录执行，除非特别标明。

## 主要功能

- 显示全部地点的编号、名称和类型。
- 显示所有双向直接道路，每条道路只显示一次。
- 根据起终点编号计算最短路径，显示完整地点顺序与总距离。
- 起终点相同时返回该地点和 0 米；不连通时显示无路径提示。
- 使用不区分大小写的英文名称关键词搜索，返回全部匹配地点。
- 汇总地点数、道路数、道路总长、平均长度、最大度数地点、孤立地点及类型数量。
- 从 CSV 自动读取地图，对错误行输出带行号的警告并跳过。
- 校验空输入、非法整数、过长输入和不存在的编号；输入结束时正常退出。
- 支持 `--data` 显式指定地图目录，以及构建后随可执行文件附带示例数据。

## 数据结构与算法

| 图概念 | 在程序中的对应关系 |
| --- | --- |
| 顶点 | 一个 `Location`，包含正整数编号、名称和类型 |
| 边 | 两个地点间的一条双向道路 |
| 权重 | 道路长度，正整数，单位米 |
| 邻接矩阵 | `Graph.adjacency[i][j]`，下标对应地点数组的位置 |
| 无直接道路 | `INF_DISTANCE`，值为 1,000,000,000 |
| 自身到自身 | 矩阵对角线为 0 |

地点编号不要求连续，也不等于数组下标。程序通过 `find_location_index_by_id()` 映射，例如测试中的编号 30 对应下标 2。图最多保存 50 个地点，不使用动态内存。

Dijkstra 的处理过程：

1. 起点距离设为 0，其他地点距离设为无穷大。
2. 从尚未确定的地点中选出当前距离最小的一个。
3. 检查它的相邻地点；如果经过它可以走得更短，就更新 `distance[]` 和 `previous[]`。
4. 重复直到终点距离确定，或已经没有可到达的未处理地点。
5. 从终点沿 `previous[]` 回到起点，将所得序列反转，得到完整路线。

`visited[]` 表示已确定最短距离的地点。数组实现的时间复杂度为 O(V²)，邻接矩阵空间复杂度为 O(V²)。非负权是 Dijkstra 的前提；本项目的 CSV 进一步要求道路距离严格大于 0。

相同总距离的候选路线保留先找到的一条，按照地点在 CSV 中的顺序进行选择，因此同一份输入的结果稳定。修改 CSV 中地点的顺序可能改变等长路线的选择，但不会改变最短距离。

为避免“有效路径距离达到无穷大标记”和整数溢出，每条道路最大长度为 `(INF_DISTANCE - 1) / (MAX_LOCATIONS - 1)`，即 **20,408,163 米**。正权图的最短路径无需重复顶点，最多 49 条边，所以合法最短路径始终小于无穷大标记。松弛操作在加法前也检查边界。所有道路总长度使用 `long long` 累加，因为它可能大于单条最短路径距离。

## 项目结构

```text
campus-route-planner/
├── src/
│   ├── main.c                # 菜单、命令行参数和运行流程
│   ├── graph.c               # 图、CSV、显示、搜索和摘要
│   ├── pathfinder.c          # Dijkstra、路径回溯和路线输出
│   └── input.c               # 行输入和整数解析
├── include/
│   ├── graph.h
│   ├── pathfinder.h
│   └── input.h
├── data/
│   ├── locations.csv
│   └── roads.csv
├── tests/
│   ├── test_runner.c         # 独立 C 核心测试程序
│   ├── test_cli.cmake        # 菜单和启动参数的自动化测试
│   └── fixtures/
│       ├── test_locations.csv
│       ├── test_roads.csv
│       └── disconnected_roads.csv
├── screenshots/
│   └── .gitkeep
├── CMakeLists.txt
├── .gitignore
├── LICENSE
└── README.md
```

使用的知识点包括结构体、固定数组、指针参数、邻接矩阵、最短路径、字符串处理、文件读写、模块化编程，以及 CMake、CTest 和 Git。源码为 `.c` 文件，CMake 声明 `LANGUAGES C`，不使用 C++ 或第三方库。

## 示例地图

示例包含 **12 个地点、18 条道路**：Main Gate、Library、Central Square、Teaching Building、Laboratory、Dormitory、Cafeteria、Sports Field、Art Center、Garden、Bus Stop 和 Administration Building。

一个可以验证算法的例子：

```text
Main Gate ---------------------------- Library
          direct road: 600 m

Main Gate --200 m--> Central Square --180 m--> Library
                     total: 380 m
```

图中的道路均可双向通行。程序应选择经过 Central Square 的 380 米路线，而非直接的 600 米道路。示例地图全部连通；孤立点和不连通图由测试数据覆盖。

## CSV 格式

`data/locations.csv` 无表头，每行三列：

```csv
1,Main Gate,Entrance
2,Library,Study
3,Central Square,Public Space
```

`data/roads.csv` 无表头，每行三列为“起点编号、终点编号、距离”：

```csv
1,2,600
1,3,200
3,2,180
```

规则：

- 编号为 `1` 到 `INT_MAX` 的整数，地点编号必须唯一；两端编号必须已经加载。
- 名称和类型为 1–63 字节的非空文本，支持内部空格；首尾空白去除，不支持控制字符。
- 暂不支持字段中的英文逗号，也不实现通用 CSV 引号转义；名称和类型建议使用英文。
- 空行跳过；接受常见 LF/CRLF 换行和首行 UTF-8 BOM。请保存为 UTF-8，程序不做通用编码转换。
- 超长行、格式错误、重复地点编号、未知道路端点、非正或超范围距离、自环均警告并跳过。
- 重复双向道路只保留最小距离，输出警告；不会将两条相同端点的道路计算两次。
- 超过 50 个地点的行警告并跳过；之后依赖这些未加载地点的道路也会跳过。
- 两个文件必须都能打开和读取，否则启动失败并返回非零状态码。文件存在但为空时可以进入菜单。
- 部分坏行不导致整个程序退出：继续使用有效地图数据，警告提示数据可能不完整。
- 程序只读取地图，不修改或保存地图文件。

## Visual Studio 2022 运行方法

需要安装“使用 C++ 的桌面开发”工作负载，其中包含 C 编译器、Windows SDK 和适用于 Windows 的 CMake 工具。工作负载名称中的 C++ 不代表项目使用 C++。

**打开文件夹方式：**

1. 选择“打开本地文件夹”，打开本项目的 `campus-route-planner` 子目录，注意不是整个父仓库。
2. 等待 CMake 配置完成，选择 `x64-Debug`。
3. 在启动项中选择 **`campus_route_planner.exe`**，不要选测试程序 `route_tests.exe`。
4. 按 Ctrl+F5 编译运行，出现英文菜单后输入数字。

CMake 构建后会把示例 `data` 复制到可执行文件旁边。更新原始 CSV 后，如果没有触发可执行文件重新链接，可以执行“重新生成”，或运行时使用 `--data` 指向原始数据目录。

**生成解决方案方式：**在 Developer PowerShell for VS 2022 中进入本子目录，执行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DSTRICT_WARNINGS=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\campus_route_planner.exe --data data
```

也可以打开 `build/CampusRoutePlanner.sln`，将 `campus_route_planner` 设为启动项目后运行。测试程序是独立目标，不会影响正常程序。

## CMake 构建方法

要求 CMake 3.20+ 和支持 C11 的编译器。MSVC 使用 `/W4`；GCC/Clang 使用 `-Wall -Wextra -Wpedantic`。启用 `STRICT_WARNINGS` 会将警告视为错误。

Linux/macOS 中，进入本项目子目录后执行：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTRICT_WARNINGS=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/campus_route_planner --data data
```

在已加载 MSVC 环境的 Developer PowerShell 中，也可以使用 NMake：

```powershell
cmake -S . -B build-nmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DSTRICT_WARNINGS=ON
cmake --build build-nmake
ctest --test-dir build-nmake --output-on-failure
.\build-nmake\campus_route_planner.exe --data data
```

同一个构建目录不要混用不同生成器。NMake 示例的 `build-nmake/` 同样被忽略。

## 数据目录与启动参数

```text
campus_route_planner
campus_route_planner --data data
campus_route_planner --data "path with spaces"
campus_route_planner --help
```

显式使用 `--data` 时，只读取该目录；相对目录根据当前终端工作目录解析。参数不正确或目录路径太长会返回非零状态码。

不指定目录时，先尝试当前工作目录下的 `data/locations.csv`。找不到时，通过启动命令中的可执行文件路径寻找旁边的 `data`。同一张地图的两个文件始终从同一目录读取，不混用不同目录的文件。

通过 `PATH` 从其他目录仅输入裸命令名启动时，标准 C 无法可靠确定可执行文件的真实位置，建议显式使用 `--data`。数据目录和文件路径缓冲区为 1024 字节，不支持任意长度的路径。源码不包含任何开发机器绝对路径。

## 操作与真实示例

启动后菜单为：

```text
===== Campus Route Planner =====
1. List all locations
2. Show direct roads
3. Find shortest route
4. Search location
5. Show map summary
0. Exit
Select an option:
```

依次输入 `3`、`1`、`2`（每次按回车），查询校门到图书馆，实际输出：

```text
Shortest route:
Main Gate -> Central Square -> Library

Total distance: 380 m
```

输入 `4`，再输入 `building`，会找到 Teaching Building 和 Administration Building。输入 `5` 的实际摘要核心数据为：

```text
Locations: 12
Roads: 18
Total road length: 3690 m
Average road length: 205.00 m
Isolated locations: 0
Most connected location(s):
  Central Square (6 roads)
```

随后还会显示每种地点类型的数量。类型按完整字符串区分大小写统计，名称搜索则忽略英文大小写。没有道路时，平均长度为 0，最大度数地点显示 None；多个地点并列最大度数时全部显示。表格保留完整文本列，终端较窄时可能换行。

输入 `0` 退出，不会修改任何地图数据。

## 自动化测试与实际结果

CTest 包含两个测试组：

- `core_tests`：独立的 `tests/test_runner.c`，共 29 项测试。
- `cli_tests`：CMake 驱动主程序的 11 个真实子进程场景，不依赖 Python 或第三方测试框架。

除 CTest 外，可在本项目目录直接运行核心测试（NMake 或单配置构建）：

```powershell
.\build-nmake\route_tests.exe tests/fixtures
```

Visual Studio 多配置构建则使用：

```powershell
.\build\Debug\route_tests.exe tests/fixtures
```

核心测试覆盖：地点和道路加载、编号映射、直接和多中间点最短路径、完整路径起终点与边距离验证、相同起终点、不连通与孤立点、搜索、非法 CSV、重复道路、容量上限、超长输入恢复、BOM、等长路径稳定性、最大合法边权与 50 顶点回溯、64 位道路总长，以及与独立 Floyd-Warshall 参考结果比较的 64 对起终点。

命令行测试覆盖：实际菜单、样例 380 米路线、空输入和非法/超长输入、未知编号、名称搜索、输入中断、缺失文件、空地图、不可达路线、损坏记录和可执行文件旁数据读取。

**2026-09-18 实际验证环境与结果：**

- Windows、MSVC 19.44.35225、CMake/CTest 3.31.6、NMake 生成器。
- C11 编译，启用 `/W4 /WX`；编译成功，无编译警告。
- CTest **2/2 测试组通过**：核心 **29/29**、命令行 **11/11**，无跳过、无失败。
- 单独执行核心测试和主程序演示，得到上述路径与地图摘要结果。
- 最初使用 Ninja 时，本地环境中的编译器探测阶段未完成，改用 NMake 后完成了全部构建测试。没有把 Ninja 的尝试记为通过。
- 未在 GCC、Clang 或 Linux/macOS 实际构建；代码使用标准 C11，但这些环境仍需复验。

测试故意输入坏行，因此测试日志里的 `Warning:` 和缺失测试文件提示属于预期场景，不是编译警告。失败的检查会令测试程序返回非零状态，Release 构建下也不会禁用检查。

核心测试在当前工作目录生成一个临时 CSV，结束时删除；CTest 在构建目录运行，命令行测试的输入及地图副本也位于构建目录。生成的缓存、构建文件和程序不应提交 Git。

## 截图位置

`screenshots/.gitkeep` 用于保留目录，目前未添加截图。可运行后保存 `shortest-route.png`、`map-summary.png` 和 `tests.png`，再把实际截图加入 README。这里的终端文本是实际输出的摘录，不是截图。

## 当前限制与改进方向

当前最多 50 个地点，只支持双向、正整数权重道路；地图只读，无编辑和保存功能。不支持带逗号的名称或通用 CSV 引号规则，不处理实时路况、通行时间或实际地理坐标。默认数据路径发现依赖工作目录或启动命令提供的程序路径，复杂启动环境建议使用 `--data`。

邻接矩阵适合这一规模的学习示例，但对大量稀疏地点会浪费空间。后续可以改用邻接表、加入带合理启发函数的 A*、区分步行和骑行道路、支持临时封路和无障碍路线，再考虑真实地图或 Web 界面。以上均为计划，不属于当前功能。

## 我通过项目练习到的能力

- 将校园空间关系抽象为图，并区分地点编号与数组下标。
- 用结构体和固定数组组织数据，维护双向矩阵的一致性。
- 理解 Dijkstra 的距离更新与前驱回溯。
- 处理输入、CSV 解析、文件错误和数值边界。
- 将界面与算法分离，利用独立测试验证结果。
- 使用 CMake/CTest 构建与测试，并整理 Git 分支、文档和演示数据。

阅读建议：先读 `include/graph.h` 理解数据，再读 `src/pathfinder.c` 理解算法，最后读 `src/main.c` 理解整个流程。

本子项目使用 [MIT License](LICENSE)，不改变仓库其他项目的许可状态。
