# MOS2 电视造型 3D 打印外壳

[English](3D_PRINT_CASE.en.md)

项目提供一套适用于 MOS2 主板与 240×240 屏幕的电视造型外壳。模型由前壳、后盖、天线、底座、盖板和两个旋钮组成，可分别打印并组装。

[购买适用的 MOS2 主板（淘宝）](https://item.taobao.com/item.htm?id=854776370120)

![MOS2 电视造型外壳渲染图](../hardware/MOS2-TV-3D-PRINT-CASE/render-preview.png)

## 下载模型

- [完整 STL 压缩包](../hardware/MOS2-TV-3D-PRINT-CASE/MOS2-TV-3D-PRINT-CASE.zip)

压缩包包含 11 个 STL 模型。后盖、天线、盖板和两个旋钮为各方案共用部件；前壳与底座按以下方式选择。

## 前壳与底座方案

### 标准前壳搭配独立底座

打印 `1-Front-Case.stl`，再选择一种底座：

- `4-Base.stl`：简易底座。
- `4-Base-Retro-Four-Leg-10deg.stl`：复古电视四脚大底座，固定仰角 10°。
- `4-Base-Retro-Four-Leg-15deg.stl`：复古电视四脚大底座，固定仰角 15°。

![复古电视四脚大底座 10° 与 15°](../hardware/MOS2-TV-3D-PRINT-CASE/retro-four-leg-base-preview.png)

### 前壳自带四脚

无需另打印底座，直接选择一种带四脚的前壳：

- `1-Front-Case_Four-Leg-5deg.stl`：一体超短脚，仰角 5°。
- `1-Front-Case_Four-Leg-8deg.stl`：一体超短脚，仰角 8°。

![前壳自带四脚 5° 与 8°](../hardware/MOS2-TV-3D-PRINT-CASE/integrated-four-leg-preview.png)

### 共用部件

- `2-Rear-Cover.stl`：后盖。
- `3-Antenna.stl`：天线。
- `5-Cover-Plate.stl`：盖板。
- `6-Knob-1.stl`：上旋钮。
- `7-Knob-2.stl`：下旋钮。

## 打印说明

- 根据选定方案导入对应前壳、底座和共用部件，根据打印机和材料调整摆放方向、支撑、层高与填充率。
- 外壳、卡片和旋钮可使用不同颜色耗材；渲染图中的六套颜色仅作为配色参考。
- 打印前先在切片软件中核对尺寸，避免自动缩放模型。
- 安装前清理支撑和毛边，先试装屏幕、主板和按键，再固定后盖。

模型仅包含外壳结构，MOS2 主板、屏幕、蜂鸣器及连接线需另行准备。
