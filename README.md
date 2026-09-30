# CampusTreeManager v2 — 校园树木信息管理系统

C11 命令行管理程序，使用固定数组保存最多 100 条记录。提供完整 CRUD、稳定排序、统计、输入验证，以及同目录临时文件 + 原子替换保存。它是单进程学习项目，不是数据库。

## 项目目标与完整 CRUD

- Create：新增树木，验证编号唯一、文本长度、有限正数胸径和健康状态。
- Read：完整列表、按编号或树种精确查询、健康比例、平均及并列最大胸径。
- Update：按编号编辑，空输入保留原值，临时副本通过确认后整体提交。
- Delete：显示目标并二次确认，通过数组前移删除，保持剩余顺序。
- 胸径降序冒泡排序，相同胸径保持原顺序。
- 启动加载 CSV；显式保存或有未保存修改时退出保存。

```text
1. Add tree
2. Edit tree
3. Delete tree
4. List all trees
5. Search tree
6. Sort by diameter
7. Show statistics
8. Save data
0. Exit
```

## 项目目录

```text
.
├── src/
│   ├── main.c                 # 菜单和 dirty 状态
│   ├── tree_manager.c         # 输入、CRUD、排序统计、安全保存
│   └── tree_manager.h         # Tree、固定容量、公共接口
├── data/trees.csv             # 5 条示例记录
├── tests/
│   ├── run_tests.py           # 端到端集成检查
│   ├── test_input.txt         # 从空数据开始的旧功能回归输入
│   └── test_save_failures.c   # 仅测试程序使用的 I/O 故障注入
├── screenshots/              # 展示图片；旧图片可能仍显示 v1 菜单
├── CMakeLists.txt
├── .gitignore
└── README.md
```

所有命令均在本仓库根目录运行。`build/`、`work/`、`out/`、`outputs/` 和编译产物已忽略。示例 `data/trees.csv` 保留在版本管理中。

## 构建与运行

需要 C11 编译器和 CMake 3.16+，没有第三方运行库。Python 3 仅用于集成测试，运行 C 程序不需要 Python。

Windows：安装 Visual Studio 2022 的“使用 C++ 的桌面开发”工作负载（包含 C 编译器），在开发者终端中运行：

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Debug
ctest --test-dir build-vs -C Debug --output-on-failure
.\build-vs\Debug\CampusTreeManager.exe
```

上面是 VS 生成器复现命令；本次实际验证使用 x64 Native Tools Command Prompt、MSVC 19.44.35225 和 NMake Debug：

```bat
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS=/WX -DPython3_EXECUTABLE=C:/Users/wyh58/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure -V
build\CampusTreeManager.exe
```

`Python3_EXECUTABLE` 是这台测试机器实际使用的 Python 3.12.14 路径，其他机器请替换为自己的路径；若 CMake 能找到 Python，可以省略该参数。不要在已有的同一个 build 目录中切换生成器。

Linux/macOS 的复现命令如下，尚未在这些主机上执行验证：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/CampusTreeManager
```

MSVC 编译启用 `/W4 /utf-8`，本次额外以 `/WX` 将警告视作错误；其他编译器启用 `-Wall -Wextra -Wpedantic`。CMake 设置 C11，关闭编译器语言扩展。

**数据路径相对于进程工作目录。** 从仓库根目录启动会读写示例 CSV；做演示请使用单独工作目录，见下文。

## 修改与删除规则

修改菜单输入编号，`0` 取消。编号不存在时提示，编号本身不能修改。

1. 显示当前完整记录。
2. 按顺序输入树种、位置、胸径、健康状态；直接回车或只有空白表示保留原值。
3. 任一字段输入 `:cancel`，或输入结束（EOF），取消整个编辑。
4. 有实际变化时显示待保存记录，`Apply changes? (y/N):` 只有完整输入 `y` 或 `Y` 才应用。
5. 在最终确认时输入结束或其他内容，也不修改原记录。输入值与原值相同则显示 `No changes.`。

内部先执行 `Tree updated = trees[index]`，共用新增功能的文本及数值验证函数；全部有效并确认后才执行 `trees[index] = updated`。不对结构体填充字节做比较，只比较四个可编辑字段。

删除时显示完整目标记录，再问 `Delete this tree? (y/N):`。只有精确的 `y` 或 `Y` 删除；空行、`n`、`yes`、带空白的 ` y`、过长输入或 EOF 均取消。删除后逐项前移并减小 count，不改变其他记录相对顺序。空数组、未找到、重复删除均不会让 count 变成负数。

## 数据修改状态

`dirty` 表示自上次成功保存后是否发生有效修改：

- 成功新增、实际编辑、删除、排序发生交换：设为 1。
- 查询、列表、统计、取消、相同值编辑、已排好序：不设置 dirty。
- 手动保存成功：清为 0；失败：保留原状态。
- 退出或 EOF：dirty 为 0 直接退出；为 1 时保存，保存失败返回非零退出码。

这是保守的修改标记，不实现撤销或全量快照比较：将数值改回原值仍可能需要一次保存。
手动选择 `8` 是明确的写入请求，即使 dirty 为 0 也会保存。

## CSV 格式和验证

无表头，每行五列：`id,species,location,diameter,health`。

```csv
1,Ginkgo,Library Square,32.50,1
2,Chinese scholar tree,Teaching Building,25.80,2
```

| 字段 | 规则 |
| --- | --- |
| id | 1 至 INT_MAX，唯一 |
| species | 去除首尾空白后 1–49 字节 |
| location | 去除首尾空白后 1–99 字节 |
| diameter | float 可表示的有限正数，厘米 |
| health | 1 Healthy；2 Average；3 Poor |

文本可含内部空格，不支持英文逗号、控制字符或通用 CSV 引号转义。推荐无 BOM、英文名称以便表格对齐。数值解析检查范围、尾随字符、溢出和非有限值。
保存胸径使用 9 位有效数字保留 binary32 float 的重读精度，界面显示两位小数；例如 25.8 保存为 25.7999992 是浮点表示结果。

启动自动创建 data 目录；不存在的 CSV 以空数组开始，只查看退出不会创建 CSV。损坏行、重复编号、超出容量的行会提示行号并跳过。读取失败或 data 不是目录时终止启动，不尝试保存。

## 安全保存设计

```text
内存数据
  → 同目录 trees.csv.tmp（独占创建）
  → 逐条 fprintf 并检查返回值
  → fflush 并检查
  → fclose 并检查
  → 替换 trees.csv
```

- 正式文件在临时文件完整关闭前不改变。
- Windows 使用 `MoveFileExA(..., MOVEFILE_REPLACE_EXISTING)`；其他平台使用 `rename()`。
- 写入、刷新或关闭失败：尝试删除本次创建的临时文件，返回失败。
- 替换失败：保留正式文件，尝试删除本次临时文件，返回失败。
- 成功替换后临时路径消失。
- 使用 C11 `wx` 模式，已有 `.tmp` 时拒绝保存，不截断或删除不属于本次保存的文件。
- 临时文件清理本身失败会明确提示，不虚称临时文件已删除。

这是**同目录临时文件 + 原子替换**，面向普通本地文件系统，不能等同于数据库事务或异常断电保障：没有磁盘/目录 fsync、备份、日志、并发写入锁和权限元数据复制机制。

## 失败场景与数据保护

| 场景 | 行为 |
| --- | --- |
| 编辑途中 EOF / 取消 | 丢弃临时记录，原记录不变 |
| 查看损坏 CSV 后退出 | 不自动重写，损坏行原样保留 |
| 有效修改后保存，或明确手动保存 | 只保存内存有效记录，已跳过的坏行/超容量行不保留 |
| 临时文件已存在、目录不可写 | 保存失败，正式文件保持不变 |
| 写入、刷新、关闭失败 | 返回失败，清理本次临时文件 |
| 正式文件被禁止删除的句柄占用 | Windows 替换失败，旧 CSV 保留 |
| 退出保存失败 | 非零退出码；未保存的内存修改不会恢复 |

若发现旧 `.tmp`，先关闭本程序其他实例并备份、检查正式 CSV 与临时文件内容，再决定如何恢复；程序不会自动覆盖它。保存重要数据前仍需备份。

## 自动化测试

```sh
python tests/run_tests.py /absolute/path/to/CampusTreeManager
```

或统一通过 CTest，使用前文命令。`BUILD_TESTING=OFF` 可关闭测试目标。找不到 Python 时 CMake 会警告并不注册集成检查，此时仅通过 CTest 中的 C 故障测试不能宣称完成全部验证。

本次 Windows x64 / MSVC 19.44.35225 / CMake 3.31.6-msvc6 / Python 3.12.14 实测：

```text
All 61 integration checks passed.
All 4 safe-save fault checks passed.
100% tests passed, 0 tests failed out of 2
```

CTest 的 2 个测试入口分别执行 61 项端到端检查和 4 项保存故障检查，不是只有两条断言。
原有 19 项检查继续保留其场景，并按新菜单编号及 dirty 语义调整：空启动不再期待创建 CSV，容量测试改为显式保存有效的 100 条记录。新增 42 项集成检查。

覆盖修改不存在/成功/空输入/相同值/中途取消/最终拒绝、非法新值、删除首中尾/全部/重复、删除顺序和 ID 重用、文件内容及时间戳不变、手动保存清除 dirty、再次修改重新标记、损坏行只查看不清除、成功无临时文件、占用临时路径失败、Windows 真实文件共享锁引起的替换失败。

`test_save_failures.c` 只在测试构建中包装 stdio，模拟 fprintf、fflush、fclose 失败并验证旧内容与临时文件清理，再验证失败后正常保存。生产程序没有故障开关。Windows 句柄测试为平台专属，其他平台明确跳过两项，不能沿用 Windows 的通过数量。

测试在 work 或构建目录下的专用夹具内执行，不修改仓库示例 CSV。`tests/test_input.txt` 是从空数据开始的可重定向输入，旧查询子菜单仍为 1 按 ID、2 按树种、0 返回。

## 实际演示

在 Windows cmd 的仓库根目录，首次准备独立演示目录：

```bat
mkdir work\manual-v2\data
copy data\trees.csv work\manual-v2\data\trees.csv
cd work\manual-v2
..\..\build\CampusTreeManager.exe
```

NMake 可执行文件位于 build；如果使用前面的 VS 生成器，改为 `..\..\build-vs\Debug\CampusTreeManager.exe`。

本次实际运行的操作：

1. `2` → 编号 `1` → 树种回车 → 位置回车 → 胸径 `35.5` → 健康 `2` → `y`。
2. `3` → 编号 `4` → `n`，取消删除。
3. `3` → 编号 `4` → `y`，删除记录。
4. `8` 保存，`0` 退出。

实际输出节选：

```text
Apply changes? (y/N): Tree updated.
Delete this tree? (y/N): Delete cancelled.
Delete this tree? (y/N): Tree deleted.
Select an option: Saved 4 tree(s).
Select an option: No unsaved changes. Goodbye.
```

保存后的实际 CSV：

```csv
1,Ginkgo,Library Square,35.5,2
2,Chinese scholar tree,Teaching Building,25.7999992,2
3,Ginkgo,East Gate,40,1
5,Maple,Dormitory Garden,28.5,2
```

## 当前限制

- 固定容量 100，不引入动态数组、链表、数据库、GUI 或第三方测试框架。
- 查询是大小写敏感的精确匹配；编号不能编辑，删除后的编号可以重新使用。
- `:cancel` 是编辑输入保留词，不能通过编辑界面设置为字段值。
- CSV 无 BOM/引号/逗号字段支持，文本限制按字节计算。
- 没有并发控制或多进程数据合并；临时文件独占创建仅防止相互截断，不防止先后保存导致旧快照覆盖新数据。
- 没有备份、撤销、崩溃恢复或断电持久性承诺，文件权限等元数据可能随替换改变。
- Windows 路径使用窄字符 API，未验证非本地文件系统和复杂路径编码；POSIX 分支未在 Linux/macOS 主机实际运行。
- dirty 为保守标记，编辑后改回、删除后重新添加仍可能触发保存。
