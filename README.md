# NotePad

基于 Qt 6 的多功能笔记与开发工具，集代码编辑、笔记管理、数据分析、图片查看于一体。

## 功能

- 代码编辑：基于 QScintilla，支持多标签页、行号、当前行高亮、查找、字体配置。
- 笔记管理：本地 SQLite 存储，支持笔记本分组、搜索、排序、新建、重命名、删除。
- 编译与终端：支持调用本机 C++ 编译器、解析编译错误、内置命令输出窗口。
- 数据分析：支持 CSV / JSON 预览、统计摘要、折线图、柱状图、散点图、饼图和 Python 脚本分析。
- 图片查看：支持常见图片格式、缩放、平移、旋转、缩略图、增强预览和导出。

## 工程结构

```text
.
|-- CMakeLists.txt          # 推荐的命令行构建入口
|-- NotePad.pro             # Qt Creator / qmake 兼容入口
|-- cmake/                  # 第三方库探测与运行时复制
|-- docs/                   # 工程说明
|-- scripts/                # 构建和清理脚本
|-- lib/                    # 已随项目放置的第三方库
|-- Dev-Cpp/                # 可选的外部编辑器集成目录
|-- *.cpp, *.h, *.ui        # 应用源码
```

更多维护说明见 [docs/ENGINEERING.md](docs/ENGINEERING.md)。

## 环境要求

| 依赖 | 建议版本 |
| --- | --- |
| Qt | 6.5 或以上 |
| 编译器 | MinGW 64-bit |
| CMake | 3.21 或以上 |
| QScintilla | Qt 6 版本 |
| ElaWidgetTools | 与当前 Qt/编译器匹配的版本 |

编译功能需要本机可用的 `g++`，数据分析脚本功能需要本机可用的 Python 3。

## 构建

推荐使用 CMake 脚本：

```powershell
.\scripts\build.ps1 -Configuration Debug
```

也可以继续使用 Qt Creator 打开 `NotePad.pro`，选择 Desktop Qt 6.x MinGW 64-bit 套件后构建。

命令行 qmake 构建：

```powershell
qmake NotePad.pro
mingw32-make -j4
```

## 第三方库

- QScintilla：代码编辑器组件。
- ElaWidgetTools：UI 组件库。
- Qt Charts：图表模块。
- SQLite：通过 Qt SQL 模块进行本地笔记存储。

第三方库默认从 `lib/` 读取。CMake 下可通过 `-DNOTEPAD_VENDOR_DIR=...` 指定其他位置。

## 开发约定

- 生成物不要提交：`build/`、`moc_*.cpp`、对象文件、运行时数据库等已在 `.gitignore` 中排除。
- qmake 和 CMake 两个入口都保留，新增源码时需要同步更新 `NotePad.pro` 和 `CMakeLists.txt`。
- 图片查看和图片文件生命周期相关逻辑风险较高，调整前请单独验证。

## 版本

当前版本：`0.8.6`
