let networkItems=[], scanState='select';
function updateNetworkLabels(){
 const networks=document.getElementById('networks'), selected=networks.value;
 networks.replaceChildren(new Option(accessText('select'),''));
 for(const item of networkItems)networks.add(new Option(item.ssid+' · '+item.rssi+' dBm'+(item.open?' · '+accessText('open'):''),item.ssid));
 networks.value=selected;
 document.getElementById('scanStatus').textContent=scanState==='select'?'':accessText(scanState);
}
if(setupMode){
 const networks=document.getElementById('networks'),button=document.getElementById('rescan'),ssid=document.getElementById('ssid');let scanning=false,setupSaved=false;
 async function scan(refresh=false){
  if(scanning||setupSaved)return;scanning=true;button.disabled=true;scanState='scanning';updateNetworkLabels();
  try{const response=await fetch('/api/wifi/scan'+(refresh?'?refresh=1':''));if(!response.ok)throw Error();const data=await response.json();
   if(data.scanning){setTimeout(()=>scan(),1000);return;}networkItems=data.networks;scanState=networkItems.length?'select':'empty';
  }catch(_){scanState='scanFailed';}finally{scanning=false;button.disabled=false;updateNetworkLabels();}
 }
 networks.onchange=()=>{ssid.value=networks.value;};button.onclick=()=>scan(true);scan();
 accessForm.onsubmit=async event=>{
  event.preventDefault();const submit=document.getElementById('submitButton'),feedback=document.getElementById('feedback');submit.disabled=true;submit.textContent=accessText('saving');feedback.dataset.error='';feedback.textContent='';
  try{const response=await fetch('/wifi/save',{method:'POST',body:new URLSearchParams(new FormData(accessForm))});const data=await response.json();if(!response.ok)throw Error(data.error||'failed');
   setupSaved=true;document.activeElement?.blur();document.getElementById('savedNetwork').dataset.ssid=ssid.value;accessForm.classList.add('hidden');document.getElementById('hint').classList.add('hidden');document.getElementById('saved').classList.remove('hidden');translateAccess();window.scrollTo(0,0);
  }catch(error){feedback.dataset.error=accessMessages.zh[error.message]?error.message:'failed';feedback.textContent=accessText(feedback.dataset.error);submit.disabled=false;submit.textContent=accessText('submit');}
 };
}
