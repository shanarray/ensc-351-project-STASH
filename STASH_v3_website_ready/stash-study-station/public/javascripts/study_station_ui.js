'use strict';
const byId = id => document.getElementById(id);
const pad = n => String(Math.floor(n)).padStart(2,'0');
function showTime(total) {
  total=Math.max(0,Math.floor(Number(total)||0));
  return `${pad(total/3600)}:${pad((total%3600)/60)}:${pad(total%60)}`;
}
async function loadStatus(){
  try{
    const response=await fetch('/api/status',{cache:'no-store'});
    if(!response.ok) throw new Error('No live data');
    const status=await response.json();
    const mode=status.state.toUpperCase();
    byId('state').textContent=mode;
    byId('state').className='state '+(mode==='OFF'?'off':mode==='PAUSED'?'paused':'');
    byId('timer').textContent=showTime(status.remaining_seconds);
    byId('motion').textContent=status.motion==null?'--':(status.motion?'Detected':'Not detected');
    byId('volume').textContent=status.music_volume==null?'--':status.music_volume;
    byId('connection').textContent='Connected';
    byId('connection').className='connection online';
  }catch{
    byId('connection').textContent='Waiting for timer data';
    byId('connection').className='connection offline';
    byId('state').textContent='UNAVAILABLE';
    byId('timer').textContent='--:--:--';
    byId('motion').textContent='--';byId('volume').textContent='--';
  }
}
async function loadHistory(){
  try{
    const response=await fetch('/api/history',{cache:'no-store'});
    if(!response.ok) throw new Error('History unavailable');
    const rows=await response.json();
    const body=byId('history');
    body.replaceChildren();
    if(rows.length===0){const tr=document.createElement('tr');const td=document.createElement('td');td.colSpan=3;td.textContent='No sessions logged yet.';tr.append(td);body.append(tr);return;}
    for(const item of rows){
      const tr=document.createElement('tr');
      for(const value of [item.date||'',item.mode||'',showTime(item.duration_seconds)]){
        const td=document.createElement('td');td.textContent=value;tr.append(td);
      }
      body.append(tr);
    }
  }catch{ /* Keep previous history if the server is unavailable. */ }
}
loadStatus();loadHistory();
setInterval(loadStatus,1000);
setInterval(loadHistory,10000);
