#pragma once
namespace WebMessages {
inline String text(const String& key,bool english){if(english)return key;
 if(key=="Invalid settings. Check addresses and values.")return "设置无效，请检查地址和数值范围。";
 if(key=="Saved. Restarting device.")return "已保存，设备正在重启。";
 if(key=="Use a 240x240 baseline JPEG, max 48KB. Original photo kept.")return "需要 240×240 基线 JPEG，最大 48KB。原相片已保留。";
 if(key=="Album is full (7 photos). Select a photo to replace.")return "相册已满（7 张），请选择要替换的照片。";
 return key;
}
}
