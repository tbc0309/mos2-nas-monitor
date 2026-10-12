# 安全设计与验证

管理接口需要登录会话；写入还需 CSRF 令牌。默认密码必须先修改。会话空闲 30 分钟失效，注销、重置清除会话。第五次密码错误等待 30 秒，后续失败逐步增加到 5 分钟。设备校验 Host 与 Origin，模拟器只监听回环地址并限制 Host、Origin、请求大小、连接等待和工作线程数。

配置不会通过公开配置响应返回管理、Wi-Fi 或 SNMP 密码。照片只接受受限尺寸的 JPEG；升级检查长度、布局与 CRC。机器索引、动作与配置字段均验证，保存失败回滚，删除目标后清空相关采集缓存和会话。

## 自动验证

本地回归覆盖访问策略、协议解析、SNMPv3、升级、触摸、资源和模拟器回归。访问安全测试验证匿名写入、Host/Origin 伪造、CSRF、爆破退避、配置保密、删除失败回滚、异常 HTTP 和重置持久化。BER 回归包含 3000 个确定性异常报文以及 OID 溢出、截断与编码边界。

这些是定向回归，不等同于完整渗透测试。测试不需要把真实密码写进仓库。

## ESP8266 与依赖核对

固件固定 Arduino ESP8266 3.1.2，显式选择 NONOS SDK 3.0.5，避免 PlatformIO 默认使用旧的 2.2 系列 SDK。相关依据：

- [Espressif Zero PMK / beacon crash 公告](https://www.espressif.com/en/Security_advisories_about_Zero_PMK_installation_and_beacon_crash)：列出旧 SDK 的无线漏洞及修复版本。
- [NONOS SDK 3.0.5](https://github.com/espressif/ESP8266_NONOS_SDK/releases/tag/v3.0.5)：本项目选择的无线 SDK。
- [Arduino ESP8266 发布记录](https://github.com/esp8266/Arduino/releases)：框架版本依据。
- [Espressif FragAttacks 公告](https://documentation.espressif.com/AR2023-008%20Security%20Advisory%20for%20WLAN%20FragAttacks%20EN.pdf)：公告列出 RTOS 分支修复，不能据此声称 NONOS 3.0.5 已修复全部相关漏洞。
- [Pillow ImageCms 安全公告](https://github.com/python-pillow/Pillow/security/advisories/GHSA-9hw9-ch79-4vh6)：模拟器依赖最低提高到 Pillow 12.3；照片入口限定 JPEG，不调用 ImageCms。

## 安全边界

管理页面使用 HTTP，局域网窃听仍可能截获密码和会话；不要映射到公网。Host/Origin 校验与退避不能抵御全部流量攻击。配置使用独立随机安装密钥，以 AES-256-CTR 加 HMAC-SHA256 加密并验证完整性；每次写入使用随机 nonce。管理密码保存 PBKDF2-HMAC-SHA256 的加盐校验值（4000 轮），不保存可恢复的登录密码。Wi-Fi、SNMP 密码和 v2c 社群需要用于连接，因此加密保存，启动后仅在 RAM 解密使用。

加密不是 ESP8266 的硬件 Flash 加密。密钥与配置同在文件系统中，能够读取完整 Flash 的攻击者仍可提取密钥；4000 轮是受限设备性能折中，不等同于服务器密码库的高成本 KDF。HTTP 传输仍未加密。SDK 使用 `WiFi.persistent(false)`，防止连接操作额外写入 SDK 明文凭据。系统仅接受新加密格式，不兼容旧明文配置。

CRC 是完整性校验，不是数字签名。已登录管理员仍能上传恶意构造的有效镜像，本项目没有安全启动或固件签名信任链。无线 SDK 含闭源代码，本轮没有进行无线射频攻击验证，也不声称全部 ESP 漏洞已消除。


## 登录与配网入口

响应采用 `Referrer-Policy: same-origin`。不能使用 `no-referrer` 后又要求表单 Origin 非空：浏览器可能将 POST Origin 置为 `null`，导致正常登录被拒绝。来源、会话和 CSRF 校验仍全部保留。

首次没有 Wi-Fi 配置或用户六击才开启配网；普通断网进行重连，不在 30 秒后自动开放配网。`/manage` 始终是管理登录入口。模拟器的 `/setup` 也遵循初始化/六击入口限制。

配置加解密只发生在读取/保存时，密码 KDF 只在修改与登录校验时执行，并定期喂 watchdog。页面显示、会话检查和 SNMP 采集不重复执行 KDF。

实现依据：[BearSSL AES CTR API](https://bearssl.org/apidoc/bearssl__block_8h.html)、[BearSSL HMAC API](https://bearssl.org/apidoc/bearssl__hmac_8h.html)、[ESP8266 Wi-Fi 持久化文档](https://arduino-esp8266.readthedocs.io/en/stable/esp8266wifi/generic-class.html)。
