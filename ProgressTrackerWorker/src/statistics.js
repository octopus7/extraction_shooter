import {catalog,describeCheckpoint} from './catalog.js';
import {HttpError} from './http.js';
import {ID_PATTERN,UUID_PATTERN} from './events.js';
const identity=['player_id','run_id','build_id','dataset'];
const key=identity.join(',');
const same=(a,b)=>identity.map(c=>a+'.'+c+'='+b+'.'+c).join(' AND ');
const rules=catalog.flatMap(c=>c.requires.map(parent=>[c.id,parent]));
function cte(buildId){
 const values=rules.map(()=>'(?,?)').join(',');
 return {
 sql:'WITH rules(child,parent) AS (VALUES '+values+'), arrivals AS (SELECT '+key+',checkpoint_id,category,MIN(playtime_seconds) AS seconds,MIN(received_at) AS first_received,MAX(received_at) AS last_received FROM events '+(buildId?'WHERE build_id=? ':'')+'GROUP BY '+key+',checkpoint_id,category), missing AS (SELECT DISTINCT '+identity.map(c=>'a.'+c).join(',')+',r.child,r.parent FROM arrivals a JOIN rules r ON a.checkpoint_id=r.child WHERE NOT EXISTS (SELECT 1 FROM arrivals p WHERE '+same('a','p')+' AND p.checkpoint_id=r.parent)) ',
 args:[...rules.flat(),...(buildId?[buildId]:[])]
 };
}
async function all(db,sql,args=[]){const result=await db.prepare(sql).bind(...args).all();if(!result.success)throw new Error('query');return result.results;}
export function parseFilter(url){
 const buildId=url.searchParams.get('buildId')||null;
 if(buildId && !ID_PATTERN.test(buildId)) throw new HttpError(400,'invalid_filter');
 return {buildId};
}
export async function getSummary(db,{buildId}){
 const c=cte(buildId);
 const [totals,stageRows,missingRows,builds]=await Promise.all([
  db.prepare(c.sql+'SELECT (SELECT COUNT(DISTINCT player_id) FROM arrivals) AS players,(SELECT COUNT(*) FROM (SELECT '+key+' FROM arrivals GROUP BY '+key+')) AS runs,(SELECT COUNT(DISTINCT player_id) FROM arrivals WHERE category=\'complete\') AS completedPlayers,(SELECT COUNT(DISTINCT player_id) FROM arrivals WHERE category=\'start\') AS startedPlayers,(SELECT COUNT(*) FROM (SELECT '+key+' FROM missing GROUP BY '+key+')) AS missingRuns').bind(...c.args).first(),
  all(db,c.sql+'SELECT checkpoint_id AS id,category,COUNT(DISTINCT player_id) AS players,COUNT(*) AS runs,AVG(seconds) AS averageSeconds FROM arrivals GROUP BY checkpoint_id,category',c.args),
  all(db,c.sql+'SELECT child AS id,COUNT(*) AS missingRuns FROM (SELECT DISTINCT '+key+',child FROM missing) GROUP BY child',c.args),
  all(db,'SELECT DISTINCT build_id AS id FROM events ORDER BY build_id LIMIT 200')
 ]);
 const ids=[...catalog.map(e=>e.id),...stageRows.filter(e=>!catalog.some(x=>x.id===e.id)).map(e=>e.id).sort()];
 return {builds:builds.map(e=>e.id),totals,stages:ids.map(id=>{
  const row=stageRows.find(e=>e.id===id);
  return {...describeCheckpoint(id,row?.category),players:row?.players??0,runs:row?.runs??0,
   rate:totals.players?100*(row?.players??0)/totals.players:0,averageSeconds:row?.averageSeconds??null,
   missingRuns:missingRows.find(e=>e.id===id)?.missingRuns??0};
 })};
}
export function parsePage(url){
 const value=url.searchParams.get('page')??'1';
 if(!/^[1-9][0-9]{0,5}$/.test(value)) throw new HttpError(400,'invalid_filter');
 return Number(value);
}
export async function getRuns(db,{buildId},page=1){
 const c=cte(buildId),pageSize=25;
 const total=await db.prepare(c.sql+'SELECT COUNT(*) AS total FROM (SELECT '+key+' FROM arrivals GROUP BY '+key+')').bind(...c.args).first('total');
 const rows=await all(db,c.sql+'SELECT a.player_id AS playerId,a.run_id AS runId,a.build_id AS buildId,a.dataset,MAX(a.last_received) AS lastReceivedAt,COUNT(*) AS checkpointCount,MAX(a.category=\'complete\') AS completed,(SELECT COUNT(DISTINCT m.parent) FROM missing m WHERE '+same('a','m')+') AS missingCount FROM arrivals a GROUP BY '+identity.map(c=>'a.'+c).join(',')+' ORDER BY lastReceivedAt DESC,playerId,runId,buildId,dataset LIMIT ? OFFSET ?',[...c.args,pageSize,(page-1)*pageSize]);
 return {page,pageSize,total,items:rows.map(r=>({...r,completed:!!r.completed}))};
}
export function parseIdentity(url){
 const result={};
 for(const key of ['playerId','runId','buildId','dataset']) result[key]=url.searchParams.get(key);
 if(!UUID_PATTERN.test(result.playerId??'') || !UUID_PATTERN.test(result.runId??'') || !ID_PATTERN.test(result.buildId??'') || result.dataset!=='demo') throw new HttpError(400,'invalid_filter');
 result.playerId=result.playerId.toLowerCase();result.runId=result.runId.toLowerCase();
 return result;
}
export async function getRun(db,run){
 const rows=await all(db,'SELECT checkpoint_id AS id,category,MIN(playtime_seconds) AS playtimeSeconds,MIN(received_at) AS receivedAt FROM events WHERE player_id=? AND run_id=? AND build_id=? AND dataset=? GROUP BY checkpoint_id,category ORDER BY playtimeSeconds,id',[run.playerId,run.runId,run.buildId,run.dataset]);
 if(!rows.length) throw new HttpError(404,'not_found');
 const present=new Set(rows.map(r=>r.id)),missing=new Map();
 for(const entry of catalog) if(present.has(entry.id)) for(const parent of entry.requires) if(!present.has(parent)){
  if(!missing.has(parent)) missing.set(parent,[]);missing.get(parent).push(entry.id);
 }
 return {identity:run,checkpoints:rows.map(r=>({...r,...describeCheckpoint(r.id,r.category)})),missing:[...missing].map(([checkpointId,requiredBy])=>({checkpointId,requiredBy}))};
}
