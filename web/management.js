
let firmwareBusy=false;
const form=document.getElementById('settings');for(const [k,v] of Object.entries(config)){if(form.elements[k])form.elements[k].value=v;}
if(config.iface){form.elements.iface.add(new Option(config.iface,config.iface));form.elements.iface.value=config.iface;}
let lang=config.adminLanguage||'zh';const msg=key=>I18n.text(key,lang);
function translate(){document.documentElement.lang=lang==='en'?'en':'zh-CN';document.getElementById('language').value=lang;I18n.apply(document,lang);updateWifiLabels();document.getElementById('cityCodeLookup').title=msg("Search for a city on China Weather. The nine digits in its weather page URL are the city code.");document.getElementById('crop').setAttribute('aria-label',msg("Photo crop preview"));if(typeof photo!=='undefined')refreshAlbum();tick();}
document.getElementById('language').onchange=async e=>{const previous=lang;lang=e.target.value;translate();const r=await post('/api/language',{language:lang});if(!r.ok){lang=previous;translate();}};
document.querySelectorAll('.day').forEach(c=>{c.checked=!!((config.alarmDays??31)&+c.value);c.onchange=()=>{form.elements.alarmDays.value=[...document.querySelectorAll('.day:checked')].reduce((n,c)=>n|+c.value,0);};});
const originals=new Map([...form.querySelectorAll('input,select')].map(el=>[el,el.type==='checkbox'?el.checked:el.value]));
 function dirty(){for(const card of form.querySelectorAll('section'))card.classList.toggle('dirty',[...card.querySelectorAll('input,select')].some(el=>(el.type==='checkbox'?el.checked:el.value)!==originals.get(el)));}
 form.addEventListener('input',dirty);form.addEventListener('change',dirty);
 form.onsubmit=async e=>{e.preventDefault();const message=document.getElementById('message'),button=document.querySelector('#settingsActions button');if(!+form.elements.alarmDays.value){message.textContent=msg("Select at least one weekday");return;}button.disabled=true;message.textContent=msg("Saving…");try{const response=await fetch(form.action,{method:'POST',body:new URLSearchParams(new FormData(form))});const content=await response.text();message.textContent=(response.headers.get('Content-Type')||'').includes('text/html')?(response.ok?msg("Saved."):msg("Failed")):content;message.style.color=response.ok?'#147e87':'#b84b42';if(response.ok)setTimeout(()=>location.reload(),8000);else button.disabled=false;}catch(error){message.textContent=msg("Failed");message.style.color='#b84b42';button.disabled=false;}};
document.querySelectorAll('[data-tab]').forEach(b=>b.onclick=()=>{document.querySelectorAll('[data-tab]').forEach(x=>x.classList.toggle('active',x===b));['screen','photos','maintenance'].forEach(n=>document.getElementById('tab-'+n).classList.toggle('hidden',b.dataset.tab!==n));form.classList.toggle('hidden',b.dataset.tab!=='settings');document.getElementById('settingsActions').classList.toggle('hidden',b.dataset.tab!=='settings');if(b.dataset.tab==='settings'&&!wifiScanned)scanManagementWifi();});
const format=(v,u)=>v<0||v==null?'--':v.toFixed(1)+u;const rate=v=>v<0?'--':v>=1048576?(v/1048576).toFixed(1)+' MB/s':(v/1024).toFixed(1)+' KB/s';
function weatherOptions(){const enabled=form.elements.weatherEnabled.value==='1';form.elements.home.querySelector('option[value="5"]').disabled=!enabled;document.getElementById('weatherPage').disabled=!enabled;if(!enabled&&form.elements.home.value==='5')form.elements.home.value='6';}form.elements.weatherEnabled.onchange=weatherOptions;weatherOptions();
async function tick(){if(firmwareBusy)return;try{const response=await fetch('/api/status',{cache:'no-store'});if(response.status===401){location.href='/manage';return;}const d=await response.json();updateInterfaces(d);updateCurrentWifi(d);document.getElementById('state').textContent=d.online?msg("Online"):msg("Offline");for(const k of ['cpu','memory'])document.getElementById(k).textContent=format(d[k],'%');document.getElementById('temperature').textContent=format(d.temperature,'°C');document.getElementById('traffic').textContent=rate(d.rx)+' / '+rate(d.tx);document.getElementById('traffic').style.fontSize='15px';document.getElementById('details').textContent=(d.model||'NAS')+' · '+(d.interface||'--')+' · '+(d.disks||[]).length+msg(" disks / ")+(d.volumes||[]).length+msg(" volumes");const w=d.weather;document.getElementById('weatherDetails').textContent=w&&w.valid?(lang==='en'?(w.cityEnglish||'Tianjin')+' · '+(w.conditionEnglish||''):(w.city||'')+' · '+(w.condition||''))+' · '+w.temperature+'°C · '+w.humidity+'%':msg("Waiting for weather");const a=d.alarm;if(a)document.getElementById('alarmDetails').textContent=[msg("Idle"),msg("Ringing"),msg("Snoozed"),msg("Dismissed today")][a.state]+(a.state===2?' · '+a.remaining+'s':'');}catch(e){document.getElementById('state').textContent=msg("Disconnected");}}
async function post(path,data={}){return fetch(path,{method:'POST',body:new URLSearchParams({...data,token:form.elements.token.value})});}
async function setPage(page){await post('/api/page',{page});tick();}
async function testBeep(){const r=await post('/api/beep');document.getElementById('message').textContent=r.ok?msg("Sound sent"):msg("Failed");}
async function alarmAction(a){const r=await post('/api/alarm/'+a,{tune:form.elements.alarmTune.value});if(!r.ok)document.getElementById('alarmDetails').textContent=msg("No active alarm");else tick();}
async function resetSettings(){const r=await post('/api/reset/request');if(!r.ok)return;const d=await r.json();if(!confirm(msg("Clear all settings and photos? This cannot be undone.")))return;const done=await post('/api/reset/confirm',{nonce:d.nonce});if(done.ok){alert(msg("Reset complete. Reconnect to the device Wi-Fi."));location.reload();}}
const crop=document.getElementById('crop'),cropCtx=crop.getContext('2d');let photo=null,angle=0,zoom=1,offsetX=0,offsetY=0,drag=null;
let albumEntries=[];
function drawCrop(){cropCtx.fillStyle='#0b1421';cropCtx.fillRect(0,0,240,240);if(!photo)return;let w=angle%180?photo.height:photo.width,h=angle%180?photo.width:photo.height,scale=Math.max(240/w,240/h)*zoom;offsetX=Math.max(-(w*scale-240)/2,Math.min((w*scale-240)/2,offsetX));offsetY=Math.max(-(h*scale-240)/2,Math.min((h*scale-240)/2,offsetY));cropCtx.save();cropCtx.translate(120+offsetX,120+offsetY);cropCtx.rotate(angle*Math.PI/180);cropCtx.scale(scale,scale);cropCtx.drawImage(photo,-photo.width/2,-photo.height/2);cropCtx.restore();}
function resetCrop(){zoom=1;offsetX=offsetY=0;document.getElementById('zoom').value=1;drawCrop();}
function rotatePhoto(){angle=(angle+90)%360;resetCrop();}
document.getElementById('zoom').oninput=e=>{zoom=+e.target.value;drawCrop();};
document.getElementById('photoFile').onchange=e=>{const file=e.target.files[0];if(!file)return;const url=URL.createObjectURL(file),img=new Image();img.onload=()=>{photo=img;angle=0;resetCrop();URL.revokeObjectURL(url);document.getElementById('uploadPhoto').disabled=!document.getElementById('photoSlot').value;};img.onerror=()=>{URL.revokeObjectURL(url);document.getElementById('photoMessage').textContent=msg("Cannot open photo");};img.src=url;};
crop.onpointerdown=e=>{if(!photo)return;drag=[e.clientX,e.clientY,offsetX,offsetY];crop.setPointerCapture(e.pointerId);};crop.onpointermove=e=>{if(!drag)return;offsetX=drag[2]+(e.clientX-drag[0])*240/crop.clientWidth;offsetY=drag[3]+(e.clientY-drag[1])*240/crop.clientHeight;drawCrop();};crop.onpointerup=crop.onpointercancel=()=>drag=null;
async function refreshAlbum(){try{const entries=await(await fetch('/api/album')).json();albumEntries=entries;const slots=document.getElementById('photoSlot'),previous=slots.value;slots.replaceChildren();
 document.getElementById('photoCount').textContent=msg('Photos')+' · '+entries.length+' / 7';
 if(entries.length<7){let o=document.createElement('option');o.value='add';o.textContent=msg("Add new photo (automatic slot)");slots.append(o);}
 for(const entry of entries){let o=document.createElement('option');o.value=entry.slot;o.textContent=msg("Replace photo ")+(entry.slot+1)+msg(" (occupied)");slots.append(o);}
 if(previous==='add'&&entries.length<7)slots.value='add';
 document.getElementById('uploadPhoto').disabled=!photo||!slots.value;
 const container=document.getElementById('album');container.replaceChildren();for(const entry of entries){let card=document.createElement('div'),img=document.createElement('img'),caption=document.createElement('small'),button=document.createElement('button');img.src=entry.url+'&v='+Date.now();caption.textContent=msg("Photo ")+(entry.slot+1);button.textContent=msg("Delete");button.onclick=async()=>{const r=await fetch('/api/album/delete',{method:'POST',body:new URLSearchParams({slot:entry.slot,token:form.elements.token.value})});if(r.ok)refreshAlbum();};card.append(img,caption,button);container.append(card);}}catch(e){document.getElementById('photoMessage').textContent=msg("Cannot load photos");}}
async function uploadPhoto(){if(!photo)return;const destination=document.getElementById('photoSlot').value;if(!destination)return;
 if(destination!=='add'&&!confirm(msg("Replace this photo?")))return;let button=document.getElementById('uploadPhoto');button.disabled=true;try{let blob;for(let q=.9;q>=.3;q-=.1){blob=await new Promise(resolve=>crop.toBlob(resolve,'image/jpeg',q));if(blob&&blob.size<=49152)break;}if(!blob||blob.size>49152)throw Error(msg("Photo exceeds 48KB"));let data=new FormData();data.append('file',blob,'photo.jpg');let r=await fetch('/api/album/upload?'+(destination==='add'?'mode=add':'slot='+destination),{method:'POST',headers:{'X-CSRF-Token':form.elements.token.value},body:data});if(!r.ok)throw Error(await r.text());document.getElementById('photoMessage').textContent=msg("Photo saved");refreshAlbum();}catch(e){document.getElementById('photoMessage').textContent=e.message;}finally{button.disabled=!photo||!document.getElementById('photoSlot').value;}}document.getElementById('photoSlot').onchange=()=>document.getElementById('uploadPhoto').disabled=!photo;drawCrop();refreshAlbum();

let wifiNetworks=[],wifiScanning=false,wifiScanned=false,wifiScanMessage='',currentWifi=null;
function updateWifiLabels(){
 const select=document.getElementById('manageNetworks'),selected=select.value;
 select.replaceChildren(new Option(msg('Choose a network'),''),...wifiNetworks.map(n=>new Option(n.ssid+' · '+n.rssi+' dBm',n.ssid)));
 select.value=selected;document.getElementById('manageScanStatus').textContent=wifiScanMessage?msg(wifiScanMessage):'';
 if(currentWifi)document.getElementById('wifiCurrent').textContent=currentWifi.wifiConnected?msg('Current Wi-Fi')+' · '+currentWifi.wifiSsid:msg('Not connected');
}
function updateCurrentWifi(d){currentWifi=d;updateWifiLabels();}
async function scanManagementWifi(refresh=false){
 if(wifiScanning||firmwareBusy)return;wifiScanning=true;wifiScanned=true;const button=document.getElementById('manageRescan');button.disabled=true;wifiScanMessage='Scanning…';updateWifiLabels();
 try{const r=await fetch('/api/wifi/scan'+(refresh?'?refresh=1':''),{cache:'no-store'});if(r.status===401){location.replace('/manage');return;}if(!r.ok)throw Error();const d=await r.json();if(d.scanning){setTimeout(()=>scanManagementWifi(),1000);return;}wifiNetworks=d.networks||[];wifiScanMessage=wifiNetworks.length?'':'No networks found';}
 catch(_){wifiScanMessage='Wi-Fi scan failed';}finally{wifiScanning=false;button.disabled=false;updateWifiLabels();}
}
document.getElementById('manageNetworks').onchange=e=>{if(e.target.value){form.elements.ssid.value=e.target.value;dirty();}};
document.getElementById('manageRescan').onclick=()=>scanManagementWifi(true);

translate();setInterval(tick,3000);

function resetColors(group){const defaults=group==='clock'?{clockColor:'#ffffff',minuteColor:'#ffa500'}:{nasBackground:'#21455c'};for(const [name,value] of Object.entries(defaults))form.elements[name].value=value;dirty();}

function updateInterfaces(d){
 const select=form.elements.iface,selected=select.value;
 const options=[{index:'',name:msg("Automatic")+(d.interface?' · '+d.interface:'')}];
 for(const item of d.interfaces||[])options.push({index:String(item.index),name:item.name+' · '+msg(item.up?"Connected":"Not connected")});
 if(selected&&!options.some(x=>x.index===selected))options.push({index:selected,name:selected});
 const key=JSON.stringify(options);
 if(select.dataset.options!==key){select.replaceChildren(...options.map(x=>new Option(x.name,x.index)));select.value=selected;select.dataset.options=key;}
}


function firmwareCRC(bytes){let crc=0xffffffff;for(const b of bytes){crc^=b;for(let i=0;i<8;i++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}return ((crc^0xffffffff)>>>0).toString(16).padStart(8,'0');}
async function upgradeFirmware(){
  if(firmwareBusy)return;
  const input=document.getElementById('firmwareFile'),keep=document.getElementById('firmwareKeep'),button=document.getElementById('firmwareUpgrade'),progress=document.getElementById('firmwareProgress'),message=document.getElementById('firmwareMessage'),file=input.files[0];
  const feedback=(key,color='#147e87')=>{message.dataset.i18n=key;message.textContent=msg(key);message.style.color=color;};
  if(!file||!file.name.toLowerCase().endsWith('.bin')||file.size<4096||file.size>0x3fa000){feedback("Choose a valid program .bin (up to 1 MiB).",'#b84b42');return;}
  if(!confirm(keep.checked?msg("Upgrade firmware and keep settings and photos?"):msg("Upgrade firmware and delete all settings and photos?")))return;
  firmwareBusy=true;input.disabled=keep.disabled=button.disabled=true;progress.value=0;progress.classList.remove('hidden');feedback("Uploading… Do not disconnect power.");
  try{
    const bytes=new Uint8Array(await file.arrayBuffer()),body=new FormData();const program=FirmwareBundle.program(bytes);body.append('firmware',new Blob([program]),file.name);
    if(bytes[0]!==0xe9)throw new Error('invalid');
    const result=await new Promise((resolve,reject)=>{
      const request=new XMLHttpRequest();request.open('POST','/api/firmware?'+new URLSearchParams({size:program.length,crc:firmwareCRC(program),keep:keep.checked?'1':'0'}));request.setRequestHeader('X-CSRF-Token',form.elements.token.value);request.timeout=180000;
      request.upload.onprogress=e=>{if(e.lengthComputable)progress.value=Math.round(e.loaded/e.total*100);};
      request.upload.onload=()=>{feedback("Checking firmware… Do not disconnect power.");};
      request.onload=()=>{let d;try{d=JSON.parse(request.responseText);}catch(e){return reject(new Error('write'));}if(request.status===200&&d.updated)resolve(d);else reject(new Error(d.error||'write'));};
      request.onerror=request.ontimeout=()=>reject(new Error('network'));request.send(body);
    });
    progress.value=100;feedback(result.simulated?"Simulation passed. No firmware was flashed.":result.preserved?"Upgrade complete. 1. Wait for restart. 2. Reopen management at the same address.":"Upgrade complete. 1. Wait for restart. 2. Connect to the device hotspot. 3. Set up Wi-Fi again.");
    if(!result.simulated)return;
  }catch(error){feedback(error.message==='network'?"Connection interrupted. Check the device before retrying.":['login_required','csrf'].includes(error.message)?"Session expired. Reload and sign in again.":error.message==='size'?"Firmware is too large for the program partition.":"Upgrade failed: invalid, incompatible or incomplete firmware. Data was not cleared.",'#b84b42');}
  firmwareBusy=false;input.disabled=keep.disabled=button.disabled=false;
}

function snmpOptions(){const v=form.elements.snmpVersion.value;document.querySelectorAll('[data-snmp]').forEach(e=>e.classList.toggle('hidden',e.dataset.snmp!==v));form.elements.snmpUser.required=v==='3';form.elements.snmpPassword.minLength=v==='3'?8:0;const encrypted=v==='3'&&form.elements.snmpPrivacy.value!=='none';document.getElementById('privacyPasswordField').classList.toggle('hidden',!encrypted);form.elements.snmpPrivacyPassword.minLength=encrypted?8:0;}
form.elements.snmpPrivacy.addEventListener('change',snmpOptions);form.elements.snmpVersion.addEventListener('change',snmpOptions);snmpOptions();

async function signOut(event){event.preventDefault();try{const response=await post('/logout');if(response.ok||response.status===401)location.replace('/manage');else throw Error();}catch(_){document.getElementById('message').textContent=msg('Failed');}}
