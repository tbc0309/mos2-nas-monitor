# 配置参数与命名约定

内部配置定义集中在 `src/app/configuration_types.h`，机器档案最多 4 个。`tools/config_parameters.json` 记录所有配置成员与表单/JSON 字段的对应关系，测试检查漏项。短协议键与有含义的内部成员显式映射，不添加旧版配置迁移或别名。

- C++ 类型使用 PascalCase；函数和成员使用 lowerCamelCase；Python 使用 snake_case。
- 时间成员写明 `Seconds` / `Minutes`；调度常量统一毫秒；容量原始值保留单位，展示再转换。
- OID 使用具名常量，页面文案只进入语言源文件，禁止混入协议识别。
- 密码、社群值不进入公共配置响应。管理员持久值是校验器，不是可解密的登录密码；设备凭据随整个配置加密保存。

| 分类 | 协议字段 | 内部成员 | 默认值 | 约束 | 应用 |
| --- | --- | --- | --- | --- | --- |
| Management | `adminDefault` | `adminDefault` | True | 由字段验证 | restart |
| Management | `ssid` | `wifiSsid` | 空 | ≤32 bytes | restart |
| Management | `pass` | `wifiPassword` | 空 | ≤63 bytes；敏感；留空保留（设置接口） | restart |
| SNMP | `nas` | `snmpHost` | 192.168.1.100 | 由字段验证 | restart |
| SNMP | `community` | `snmpCommunity` | public | ≤64 bytes；敏感；留空保留（设置接口） | restart |
| Management | `admin` | `adminPasswordVerifier` | PBKDF2 verifier (initial password: 12345678) | ≤64 bytes；敏感；留空保留（设置接口） | restart |
| SNMP | `storage` | `storageVolumeIndex` | 空 | 由字段验证 | restart |
| SNMP | `iface` | `networkInterfaceIndex` | 空 | 由字段验证 | restart |
| SNMP | `snmpVersion` | `snmpVersion` | 2c | 2c/3 | restart |
| SNMP | `snmpUser` | `snmpUsername` | 空 | ≤32 bytes | restart |
| SNMP | `snmpPassword` | `snmpAuthPassword` | 空 | ≤64 bytes；敏感；留空保留（设置接口） | restart |
| SNMP | `snmpAuth` | `snmpAuthProtocol` | MD5 | MD5/SHA | restart |
| SNMP | `snmpPrivacy` | `snmpPrivacyProtocol` | none | none/DES/AES | restart |
| SNMP | `snmpPrivacyPassword` | `snmpPrivacyPassword` | 空 | ≤64 bytes；敏感；留空保留（设置接口） | restart |
| Weather | `cityCode` | `weatherCityCode` | 101030100 | 由字段验证 | restart |
| Weather | `ntpServer` | `timeServer` | ntp.aliyun.com | ≤63 bytes | restart |
| Weather | `weatherEnabled` | `weatherEnabled` | 1 | 由字段验证 | restart |
| Weather | `weatherInterval` | `weatherRefreshMinutes` | 20 | 由字段验证 | restart |
| Display | `rotation` | `displayRotation` | 0 | 由字段验证 | restart |
| Display | `sound` | `touchSoundEnabled` | 1 | 由字段验证 | restart |
| Alarm | `alarmTime` | `alarmTime` | 07:00 | 由字段验证 | restart |
| Alarm | `alarmEnabled` | `alarmEnabled` | 0 | 由字段验证 | restart |
| Alarm | `snooze` | `alarmSnoozeMinutes` | 5 | 由字段验证 | restart |
| Photos | `albumInterval` | `photoIntervalSeconds` | 10 | 由字段验证 | restart |
| Display | `language` | `displayLanguage` | zh | zh/en | restart |
| Display | `clockColor` | `clockHourColor` | #ffffff | 由字段验证 | restart |
| SNMP | `nasBackground` | `monitorBackground` | #21455c | 由字段验证 | restart |
| SNMP | `cardBackground` | `monitorCardBackground` | #103040 | 由字段验证 | restart |
| Display | `minuteColor` | `clockMinuteColor` | #ffa500 | 由字段验证 | restart |
| Management | `adminLanguage` | `adminLanguage` | zh | zh/en | live |
| Photos | `albumFixed` | `photoFixedMode` | 0 | 由字段验证 | restart |
| Display | `showSeconds` | `clockSecondsEnabled` | 1 | 由字段验证 | restart |
| Alarm | `alarmDays` | `alarmWeekdayMask` | 31 | 由字段验证 | restart |
| Alarm | `alarmTune` | `alarmMelody` | 0 | 由字段验证 | restart |
| Display | `home` | `startupPage` | 6 | 由字段验证 | restart |
| SNMP | `port` | `snmpPort` | 161 | 由字段验证 | restart |
| SNMP | `interval` | `snmpPollIntervalSeconds` | 5 | 由字段验证 | restart |
| Display | `brightness` | `displayBrightnessPercent` | 70 | 由字段验证 | restart |

设置表单统一使用“保存并重启”，包括天气城市、语言、配色与密码；成功写入后才安排重启，校验或写入失败不会重启。设备列表的添加、删除、排序可以连续操作，完成后统一保存并重启。`adminLanguage` 是管理网页自身的语言偏好，切换网页显示不属于设置表单提交。

## 机器与运行参数

`machines` 是最多 6 个档案的数组；档案包含名称、SNMP 目标、认证、接口、卷索引、周期和配色。`activeMachine` 指前端正在监控的目标，`selectedMachine` 指后台正在编辑的目标，二者不能混用。`machineName` 最长 48 字节，允许留空；留空时屏幕使用 SNMP 自动名称（群晖使用型号），填写后使用自定义名称。自动获取结果不会写回自定义字段。编辑非活动档案不切换屏幕或采集器。

页面值：0 总览、1 网络、2 CPU/内存、4 设备状态、5 天气、6 大时钟、7 相册、8 硬盘、9 存储；3 不使用。`alarmDays` 为周一最低位至周日最高位，默认 31（周一至周五）。`rotation` 为 0/1/2/3，对应 0°/90°/180°/270°。

接口及卷索引留空表示自动。城市代码为 9 位，保存前进行天气服务存在性校验。颜色格式为 `#RRGGBB`。管理员新密码为 8–64 字节且不能是出厂默认值。运行快照、重试和有效期不持久化，具体周期见 [采集设计](TELEMETRY.md)。
