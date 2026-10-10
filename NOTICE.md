# 素材与依赖说明

- 硬件与部分时钟、天气、动画素材来自用户提供的 JSRUN MOS2 和 SmallDesktopDisplay 参考源码；NAS 界面设计参考 routermonitor。
- 屏幕字库包含腾讯 TencentSans W7 派生的点阵子集；字体原文件不在本仓库分发。网页优先引用用户本机已安装的字体。
- 字体生成使用 FreeType / freetype-py / Pillow，固件编译依赖由 PlatformIO 管理。
- TFT_eSPI、QRCode、ArduinoJson、TJpg_Decoder 等依赖适用各自的许可。

参考项目素材和腾讯字体的完整授权范围尚未逐项核验，本仓库未对这些第三方内容另行授予开源许可。转载、商业分发或再发布时，请分别确认相关权利人的许可。
