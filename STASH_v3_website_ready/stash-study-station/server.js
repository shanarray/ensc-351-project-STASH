'use strict';
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');

const PORT = Number(process.env.PORT || 8088);
const ROOT = path.join(__dirname, 'public');
const DATA = path.join(__dirname, 'data');
const types = {'.html':'text/html; charset=utf-8','.css':'text/css; charset=utf-8','.js':'application/javascript; charset=utf-8'};

function reply(res, status, data, contentType) {
  res.writeHead(status, {'Content-Type':contentType, 'Cache-Control':'no-store'});
  res.end(data);
}
function json(res, status, value) {
  reply(res, status, JSON.stringify(value), 'application/json; charset=utf-8');
}
function getStatus(res) {
  fs.readFile(path.join(DATA,'status.json'),'utf8',(err,raw)=>{
    if(err) return json(res,503,{error:'No live status file yet. Start the C exporter.'});
    try {
      const data=JSON.parse(raw);
      if(typeof data.state!=='string' || !Number.isFinite(Number(data.remaining_seconds)))
        return json(res,503,{error:'status.json is missing state or remaining_seconds'});
      json(res,200,data);
    } catch(e) {json(res,503,{error:'Cannot read status.json'});}
  });
}
function parseCSV(raw) {
  const rows=raw.trim().split(/\r?\n/);
  if(rows.length<2) return [];
  const heads=rows.shift().split(',').map(s=>s.trim());
  return rows.filter(Boolean).slice(-100).reverse().map(line=>{
    const values=line.split(',');
    return Object.fromEntries(heads.map((k,i)=>[k,(values[i]||'').trim()]));
  });
}
function getHistory(res) {
  fs.readFile(path.join(DATA,'history.csv'),'utf8',(err,raw)=>{
    if(err) return json(res,200,[]);
    json(res,200,parseCSV(raw));
  });
}
const server=http.createServer((req,res)=>{
  if(req.method!=='GET') return json(res,405,{error:'Read-only server'});
  let pathname;
  try {pathname=decodeURIComponent(new URL(req.url,'http://localhost').pathname);} catch {return json(res,400,{error:'Bad URL'});}
  if(pathname==='/api/status') return getStatus(res);
  if(pathname==='/api/history') return getHistory(res);
  if(pathname==='/') pathname='/index.html';
  const filename=path.resolve(ROOT, '.' + pathname);
  if(!filename.startsWith(ROOT+path.sep)) return reply(res,403,'Forbidden','text/plain');
  const type=types[path.extname(filename)];
  if(!type) return reply(res,404,'Not found','text/plain');
  fs.readFile(filename,(err,buf)=>err?reply(res,404,'Not found','text/plain'):reply(res,200,buf,type));
});
server.listen(PORT,'0.0.0.0',()=>console.log(`STASH dashboard on http://0.0.0.0:${PORT}`));
