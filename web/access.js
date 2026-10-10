const accessMode = document.querySelector('main').dataset.mode;
const setupMode = accessMode === 'setup';
const accessForm = document.getElementById('accessForm');
let accessLanguage;
try { accessLanguage = localStorage.getItem('mos2-access-language') || 'zh'; } catch (_) { accessLanguage = 'zh'; }
if(accessLanguage!=='en')accessLanguage='zh';
const accessMessages = {
 zh:{title:setupMode?'连接 Wi-Fi':'管理登录',network:'Wi-Fi 网络',ssid:'网络名称',password:'密码',scan:'刷新',submit:setupMode?'保存并连接':'登录',select:'选择网络',scanning:'正在搜索附近网络…',empty:'未找到网络，可手动输入名称',scanFailed:'搜索失败，请刷新或手动输入',hint:'仅支持 2.4GHz，隐藏网络可手动填写。',saving:'正在保存…',saved:'网络已保存',networkSaved:'即将连接：',step1:'手机连接刚设置的 Wi-Fi。',step2:'联网完成后，连击小电视五次。',step3:'扫描屏幕二维码，打开管理页。',retry:'若连接失败，约30秒后重新加入设备热点重试。',invalid:'请检查网络名称和密码',saveFailed:'保存失败，请重试',wrong:'密码错误',wait:'请稍后重试',expired:'页面已过期，请刷新',failed:'保存未完成，请重试',open:'开放网络'},
 en:{title:setupMode?'Connect to Wi-Fi':'Sign in',network:'Wi-Fi network',ssid:'Network name',password:'Password',scan:'Refresh',submit:setupMode?'Save & connect':'Sign in',select:'Select network',scanning:'Searching for networks…',empty:'No networks found. Enter the name manually.',scanFailed:'Scan failed. Refresh or enter the name.',hint:'2.4GHz only. Enter hidden networks manually.',saving:'Saving…',saved:'Network saved',networkSaved:'Connecting to: ',step1:'Join the selected Wi-Fi on your phone.',step2:'Once connected, tap the device five times.',step3:'Scan the screen QR code to open management.',retry:'If connection fails, rejoin the device hotspot after about 30 seconds.',invalid:'Check network name and password',saveFailed:'Save failed. Try again.',wrong:'Incorrect password',wait:'Try again shortly',expired:'Page expired. Refresh to continue.',failed:'Save did not complete. Please retry.',open:'Open'}
};
const accessText = key => accessMessages[accessLanguage][key] || key;
function translateAccess() {
 document.documentElement.lang=accessLanguage;
 document.getElementById('accessLanguage').value=accessLanguage;
 document.getElementById('accessLang').value=accessLanguage;
 document.getElementById('title').textContent=accessText('title');
 document.querySelectorAll('[data-text]').forEach(el=>el.textContent=accessText(el.dataset.text));
 const feedback=document.getElementById('feedback'); feedback.textContent=feedback.dataset.error?accessText(feedback.dataset.error):'';
 document.getElementById('hint').textContent=setupMode?accessText('hint'):'';
 if(typeof updateNetworkLabels==='function')updateNetworkLabels();
 const savedNetwork=document.getElementById('savedNetwork');if(savedNetwork.dataset.ssid)savedNetwork.textContent=accessText('networkSaved')+savedNetwork.dataset.ssid;
}
document.getElementById('wifiFields').classList.toggle('hidden',!setupMode);
document.getElementById('ssid').disabled=!setupMode;document.getElementById('ssid').required=setupMode;
document.getElementById('accessLanguage').onchange=event=>{accessLanguage=event.target.value;try{localStorage.setItem('mos2-access-language',accessLanguage);}catch(_){}translateAccess();};
translateAccess();
