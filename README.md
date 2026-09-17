# CampusTreeManager — 校园树木信息管理系统

使用 C11 编写的命令行练习项目，通过固定数组管理最多 100 条校园树木记录。

本仓库还包含独立的 Python 工具 [C Project Analyzer](c-project-analyzer/README.md)，可以统计 C 项目的代码、注释和空行，并生成 JSON 与 Markdown 报告。

## 项目背景

校园绿化信息可以按树木编号、树种、位置、胸径和健康状态整理。本项目把这些信息作为练习对象，将 C 语言中的结构体、数组、函数、字符串处理和文件读写组合成一个可运行的小程序。界面使用英文，文档使用中文。

## 功能清单

- 添加树木：检查正整数编号、编号唯一性、文本长度、正数胸径和健康状态。
- 显示记录：表格列出全部字段，健康状态显示英文名称。
- 查询：按编号精确查询；按树种进行区分大小写的精确匹配，显示全部结果。
- 排序：冒泡排序按胸径降序排列，交换完整结构体；相同胸径保持原有顺序。
- 统计：总数、三类健康状态的数量和比例、平均胸径、所有并列最大胸径的树木。
- 持久化：启动自动读取，菜单手动保存，退出或输入结束时自动保存。
- 错误处理：无数据、非法或过长输入、文件不存在、损坏 CSV 行、重复编号、容量溢出及文件读写失败。

## 技术和知识点

程序仅使用 C 标准库以及创建目录所需的系统接口：Windows 使用 `_mkdir`，其他平台使用 `mkdir`，没有第三方运行库依赖。所有源文件均为 `.c`，使用 C11 编译，不使用 C++。

使用 `Tree` 结构体和固定数组存储数据，以 `count` 管理有效记录数量；用顺序查找实现查询，用冒泡排序练习完整结构体交换。输入统一通过 `fgets()` 读取，使用 `strtol()` 和 `strtof()` 解析数值，检查范围、尾随字符和非有限浮点数；过长行会被清理，避免影响下一次输入。

## 项目目录

以下内容直接放在仓库根目录，无额外的 `CampusTreeManager` 子目录。

```text
.
├── src/
│   ├── main.c                # 主菜单、启动和退出流程
│   ├── tree_manager.c        # 输入验证、管理操作和文件读写
│   └── tree_manager.h        # 结构体、常量和函数声明
├── data/
│   └── trees.csv             # 5 条示例记录
├── screenshots/
│   └── .gitkeep
├── tests/
│   ├── test_input.txt        # 从空数据开始的重定向操作
│   └── run_tests.py          # 可选的自动集成测试，使用 Python 标准库
├── CMakeLists.txt
├── .gitignore
└── README.md
```

`build/`、`out/`、`work/`、`outputs/` 和常见编译产物均已忽略。`data/trees.csv` 是有意保留的示例数据，不应忽略。

## Visual Studio 2022 运行方法

先在 Visual Studio Installer 中安装“使用 C++ 的桌面开发”工作负载，包含 MSVC v143、Windows SDK 和 CMake 工具。该工作负载的名称包含 C++，但也提供 C 编译器；本项目仍按 C11 编译。

推荐通过 CMake 生成解决方案。在 **Developer PowerShell for VS 2022** 中进入仓库根目录，执行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
.\build\Debug\CampusTreeManager.exe
```

也可以打开生成的 `build/CampusTreeManager.sln`，右键 `CampusTreeManager` 项目设为启动项目，按 Ctrl+F5。CMake 已为生成的 Visual Studio 项目设置调试工作目录为仓库根目录。

如果手动建立 Visual Studio 空项目：添加 `src` 中的三个文件，在项目属性中设置“C/C++ → 高级 → 编译为”为 **编译为 C 代码 (/TC)**，“C 语言标准”为 **ISO C11 (/std:c11)**，预处理器定义添加 `_CRT_SECURE_NO_WARNINGS`，并将“调试 → 工作目录”设置为本仓库根目录。该宏只抑制 MSVC 对标准 `fopen` 的替代接口建议。

也可直接在 Developer PowerShell 中编译：

```powershell
New-Item -ItemType Directory -Force build | Out-Null
cl /nologo /TC /std:c11 /W4 /utf-8 /D_CRT_SECURE_NO_WARNINGS src/main.c src/tree_manager.c /Fo:build\ /Fe:build\CampusTreeManager.exe
.\build\CampusTreeManager.exe
```

## CMake、GCC 和 Clang 构建

CMake 最低版本为 3.16。较新的 MSVC 支持 C11；推荐使用 VS 2022 自带的 CMake。

在 Linux/macOS 或已有 GCC/Clang 的环境中：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/CampusTreeManager
```

不使用 CMake 时也可以直接编译：

```sh
mkdir -p build
gcc -std=c11 -Wall -Wextra -Wpedantic src/main.c src/tree_manager.c -o build/CampusTreeManager
./build/CampusTreeManager
# 或将 gcc 替换为 clang
```

**数据路径相对于进程工作目录，而不是可执行文件所在目录。** 从仓库根目录启动会读取仓库的 `data/trees.csv`；从其他目录启动会在那里创建独立的 `data` 目录。

## CSV 数据格式

无标题行，每行恰好五列，顺序为 `id,species,location,diameter,health`：

```csv
1,Ginkgo,Library Square,32.50,1
2,Chinese scholar tree,Teaching Building,25.80,2
```

| 字段 | 规则 |
| --- | --- |
| id | 1 到当前平台 `INT_MAX`，不允许重复 |
| species | 1–49 字节，可以包含内部空格 |
| location | 1–99 字节，可以包含内部空格 |
| diameter | 可表示为 `float` 的有限正数，单位厘米 |
| health | 1 = Healthy，2 = Average，3 = Poor |

文本首尾空白会被去除，空文本和控制字符会被拒绝。树种和位置暂不支持英文逗号，不实现带引号的通用 CSV 转义；请使用无 BOM 的文本文件。推荐使用英文名称，以便控制台显示对齐。

保存胸径时使用 9 位有效数字，读取后尽量保持原始 `float` 精度，界面显示两位小数。因此保存的数字格式可能与示例不同，例如 `32.50` 保存为 `32.5`。

启动时自动创建 `data` 目录。文件不存在时以零条记录开始；坏行、重复编号和超过 100 条的记录会被跳过，并输出行号及原因，继续读取后续行。无法正常打开或读取文件时终止启动，避免随后自动保存覆盖原文件。

**保存会重写当前数据文件。** 被跳过的损坏行和超过容量的行不会保留；手动修改 CSV 前请先备份。保存失败会提示，退出时保存失败会返回非零状态码。本版本没有原子替换或备份机制，写入过程中发生磁盘故障可能留下不完整文件。

## 示例运行过程

仓库自带 5 条示例记录。启动后可以依次选择 `2`（显示）、`3 → 2 → Ginkgo → 0`（查询）、`4`（排序）、`5`（统计）、`6`（保存）、`0`（退出）。

```text
Loaded 5 tree(s).

===== Campus Tree Manager =====
1. Add tree
2. List all trees
3. Search tree
4. Sort by diameter
5. Show statistics
6. Save data
0. Exit
Select an option: 5
Total trees: 5
Healthy: 2 (40.00%)
Average: 2 (40.00%)
Poor: 1 (20.00%)
Average diameter: 29.00 cm
Largest diameter tree(s):
```

最大胸径记录为编号 3，Ginkgo，East Gate，40.00 cm，Healthy。降序排列后的编号为 `3, 1, 5, 2, 4`。实际表格保留完整文本列，建议增大终端宽度以减少自动换行。

## 测试方法及结果

`tests/test_input.txt` 必须在空数据环境运行。它先检查空数据，然后连续添加 5 条记录，过程中包含非法编号、空树种、非法胸径、非法健康状态和重复编号，之后执行查询、排序、统计及保存。

无需删除示例 CSV，可以在独立目录测试。在仓库根目录的 Windows **cmd.exe** 中执行（假设已按上述 VS CMake 步骤构建）：

```bat
mkdir work\manual-test
cd work\manual-test
..\..\build\Debug\CampusTreeManager.exe < ..\..\tests\test_input.txt
..\..\build\Debug\CampusTreeManager.exe
```

第二次启动选择 `2` 可查看恢复的数据。重复运行整套输入前需使用新的空测试目录。PowerShell 不支持相同的 `<` 语法，建议使用 cmd.exe，或使用下面的自动测试脚本。

可选的集成测试需要 Python 3，仅使用标准库，不是运行 C 程序的必需依赖。在仓库根目录执行：

```powershell
python tests/run_tests.py "$((Get-Location).Path)\build\Debug\CampusTreeManager.exe"
```

Linux/macOS 对应命令：

```sh
python3 tests/run_tests.py "$PWD/build/CampusTreeManager"
```

脚本在 `work/` 下建立临时测试目录并自动清理，不修改示例 CSV。检查内容包括所要求的全部 10 类场景，以及查询无结果、大小写匹配、数值溢出、超长输入恢复、CSV 坏字段、输入结束取消半条记录、100 条容量限制、并列最大值和打开失败。

本次环境中的实际验证（2026-09-16）：

- 使用 Unity 随附的 Clang 14 / Emscripten，以 `-std=c11 -Wall -Wextra -Wpedantic -Werror` 成功编译，无警告；生成 WebAssembly 并通过 Node.js 运行，使用 `NODERAWFS=1` 访问真实文件系统。
- `tests/run_tests.py` 的 **19 项集成检查全部通过**，包含保存后启动独立进程恢复数据。
- Windows 条件分支使用 Clang 和已安装的 Windows SDK UCRT 头文件进行 C11 语法检查，通过且无警告。
- 当前环境未找到 MSVC 编译器和 CMake，因此 **未实际验证 MSVC 链接执行、CMake 构建或 GCC 构建**。上述命令供对应环境复现；WebAssembly 测试不等同于 Windows 原生运行测试。

当已有 Emscripten 环境时，可复现此次运行方式（需设置该环境所要求的变量）：

```sh
emcc -std=c11 -Wall -Wextra -Wpedantic -Werror src/main.c src/tree_manager.c -sNODERAWFS=1 -sENVIRONMENT=node -sEXIT_RUNTIME=1 -o work/CampusTreeManager.js
python3 tests/run_tests.py /absolute/path/to/node "$PWD/work/CampusTreeManager.js"
```

## 截图占位

`screenshots/.gitkeep` 用于保留空目录，目前没有实际截图。运行后可自行保存主菜单、查询和统计画面，例如 `screenshots/menu.png`、`screenshots/search.png`、`screenshots/statistics.png`，再在 README 中添加图片引用。上面的文本演示不是截图。

## 当前限制

- 最多 100 条记录，使用固定数组，尚无删除和修改功能。
- 树种仅支持区分大小写的精确查询，未实现模糊查询。
- CSV 不支持字段中的英文逗号、换行、BOM 或完整引号转义。
- 文本限制按字节计算；多字节字符的表格对齐取决于终端，推荐英文输入。
- 表格较宽，超大数值也可能使对齐效果下降；浮点计算及显示有舍入误差。
- 保存直接覆盖文件，未实现事务、并发访问控制或异常断电恢复。

## 后续改进方向

可继续增加按编号修改和删除、模糊查询、文件备份与原子保存、CSV 转义支持。在理解当前数组实现之后，再尝试动态数组或链表，并比较查找和排序算法。

## 我在这个项目中练习到的能力

- 用结构体描述对象，用数组和计数器维护一组数据。
- 将主程序流程、业务功能和公共声明拆分成多个文件。
- 安全读取一整行输入，验证数值、边界条件和字符串长度。
- 实现顺序查找、冒泡排序、比例统计和最大值查找。
- 处理文件打开、解析、读写失败和持久化恢复。
- 使用编译器警告、可重定向输入和自动集成检查验证程序。
- 编写构建说明、示例数据和 Git 忽略规则，整理可供别人运行的项目。
