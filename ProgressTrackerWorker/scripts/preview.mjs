import {createServer} from 'node:http';
import {readFile} from 'node:fs/promises';
import {fileURLToPath} from 'node:url';
import {resolve,sep,extname} from 'node:path';
import worker from '../src/worker.js';
import {createDatabase,makeEvent,eventRequest} from '../test/support/database.js';
const publicRoot=fileURLToPath(new URL('../public/',import.meta.url));
const DB=createDatabase();
const env={DB,ADMIN_TOKEN:'local-preview-token-32-characters',PREVIEW:'true',
 ASSETS:{async fetch(request){
  const pathname=decodeURIComponent(new URL(request.url).pathname);
  const filename=resolve(publicRoot,'.'+(pathname==='/'?'/index.html':pathname));
  if(!filename.startsWith(publicRoot.endsWith(sep)?publicRoot:publicRoot+sep)) return new Response(null,{status:404});
  try{
   const data=await readFile(filename);
   const type={'.html':'text/html; charset=utf-8','.css':'text/css; charset=utf-8','.js':'text/javascript; charset=utf-8','.json':'application/json; charset=utf-8'}[extname(filename)];
   return new Response(data,{headers:{'Content-Type':type??'application/octet-stream','Cache-Control':'no-store','X-Content-Type-Options':'nosniff',
   'Content-Security-Policy':"default-src 'none'; script-src 'self'; style-src 'self'; connect-src 'self'; img-src 'self' data:; base-uri 'none'; form-action 'self'; frame-ancestors 'none'"}});
  }catch{return new Response(null,{status:404});}
 }}
};
if(!process.argv.includes('--empty')){
 const stages=['game.start','quest.demo_q1_water_intake_check','quest.demo_q2_clear_water_screen','quest.demo_q3a_repair_valve','quest.demo_q3b_repair_bunker_pipe','quest.demo_q4_todays_reward','demo.complete'];
 for(let i=0;i<38;i++){
  const playerId=crypto.randomUUID(),runId=crypto.randomUUID(),buildId=i%3?'demo-2026.09.30':'demo-2026.09.29';
  for(let j=0;j<=i%7;j++){
   if(j===2&&i%4===0) continue;
   const checkpointId=stages[j],category=j===0?'start':j===6?'complete':'quest';
   await worker.fetch(eventRequest(makeEvent({playerId,runId,buildId,checkpointId,category,playtimeSeconds:j*180+i*5})),env);
  }
 }
}
const port=Number(process.env.PORT||8788);
const server=createServer(async(req,res)=>{
 try{
  const request=new Request('http://127.0.0.1:'+port+req.url,{method:req.method,headers:req.headers,
   ...(['GET','HEAD'].includes(req.method)?{}:{body:req,duplex:'half'})});
  const response=await worker.fetch(request,env);
  res.writeHead(response.status,Object.fromEntries(response.headers));
  res.end(Buffer.from(await response.arrayBuffer()));
 }catch{res.writeHead(500);res.end();}
});
server.listen(port,'127.0.0.1',()=>console.log('Preview only: http://127.0.0.1:'+port+' | token: '+env.ADMIN_TOKEN));
function close(){server.close(()=>{DB.close();process.exit(0);});}
process.on('SIGINT',close);process.on('SIGTERM',close);
