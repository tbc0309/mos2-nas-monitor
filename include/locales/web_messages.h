#pragma once
// Generated from locales/backend/*.json.
namespace WebMessages {
inline String text(const String& key,bool english){if(english)return key;
 if(key=="Invalid settings. Check addresses and values.")return "设置无效，请检查地址和数值范围。";
 if(key=="Saved. Restarting device.")return "已保存，设备正在重启。";
 if(key=="Use a 240x240 baseline JPEG, max 48KB. Original photo kept.")return "需要 240×240 基线 JPEG，最大 48KB。原相片已保留。";
 if(key=="Album is full (7 photos). Select a photo to replace.")return "相册已满（7 张），请选择要替换的照片。";
 if(key=="Choose a new password of at least 8 bytes.")return "请设置至少 8 字节的新密码。";
 if(key=="Encryption requires a privacy password (8–64 bytes).")return "隐私密码须为 8–64 字节。";
 if(key=="SNMP v3 requires a username and an authentication password (8–64 bytes).")return "SNMP v3 需要用户名和认证密码（8–64 字节）。";
 if(key=="Reload the settings page.")return "请刷新设置页面。";
 if(key=="Invalid background color")return "底色无效。";
 if(key=="Invalid display/alarm settings")return "显示或闹钟设置无效。";
 if(key=="Invalid display settings")return "显示设置无效。";
 if(key=="Invalid seconds setting")return "秒数设置无效。";
 if(key=="Configuration write failed")return "配置保存失败。";
 if(key=="Reload page")return "请刷新页面。";
 if(key=="Storage error")return "存储失败。";
 if(key=="Invalid language")return "语言设置无效。";
 if(key=="Invalid slot")return "照片位置无效。";
 if(key=="No photo")return "没有照片。";
 if(key=="No active alarm")return "没有活动闹钟。";
 if(key=="Invalid click count")return "点击次数无效。";
 if(key=="Reset confirmation expired")return "重置确认已过期。";
 if(key=="Connect to Wi-Fi")return "连接 Wi-Fi";
 if(key=="Sign in")return "管理登录";
 if(key=="Save & connect")return "保存并连接";
 if(key=="Saved.")return "已保存。";
 if(key=="Verify the city code before saving.")return "请先校验城市代码再保存。";
 if(key=="Saved. Applied immediately.")return "已保存，已立即生效。";
 if(key=="Saved. Sign in with the new password.")return "已保存，请使用新密码重新登录。";
 return key;
}
}
