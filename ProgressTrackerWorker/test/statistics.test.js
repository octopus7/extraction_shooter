import test from 'node:test';
import assert from 'node:assert/strict';
import worker from '../src/worker.js';
import {createDatabase,makeEvent,eventRequest} from './support/database.js';
const token='test-admin-secret-at-least-32-characters';
const q1='quest.demo_q1_water_intake_check',q2='quest.demo_q2_clear_water_screen';
const q3a='quest.demo_q3a_repair_valve',q3b='quest.demo_q3b_repair_bunker_pipe',q4='quest.demo_q4_todays_reward';
async function read(env,path){const r=await worker.fetch(new Request('https://tracker.test'+path,{headers:{Authorization:'Bearer '+token}}),env);assert.equal(r.status,200);return r.json();}
async function put(env,patch={}){assert.equal((await worker.fetch(eventRequest(makeEvent(patch)),env)).status,201);}
function setup(t){const DB=createDatabase();t.after(()=>DB.close());return {DB,ADMIN_TOKEN:token};}
test('later step contributes and missing earlier record is reported without inference',async t=>{
 const env=setup(t);await put(env);
 let summary=await read(env,'/api/admin/summary');
 assert.equal(summary.totals.players,1);assert.equal(summary.totals.startedPlayers,0);
 assert.equal(summary.totals.missingRuns,1);
 assert.equal(summary.stages.find(x=>x.id===q2).rate,100);
 assert.equal(summary.stages.find(x=>x.id===q1).players,0);
 const runs=await read(env,'/api/admin/runs');
 const params=new URLSearchParams(Object.fromEntries(['playerId','runId','buildId','dataset'].map(k=>[k,runs.items[0][k]])));
 const detail=await read(env,'/api/admin/run?'+params);
 assert.deepEqual(detail.missing,[{checkpointId:q1,requiredBy:[q2]}]);
 await put(env,{checkpointId:q1});
 summary=await read(env,'/api/admin/summary');
 assert.equal(summary.totals.missingRuns,0);
});
test('branch requires both prerequisite quests, optional unknown locations add no missing flags',async t=>{
 const env=setup(t);
 for(const id of [q1,q2,q3a,q4]) await put(env,{checkpointId:id});
 let summary=await read(env,'/api/admin/summary');
 assert.equal(summary.stages.find(x=>x.id===q4).missingRuns,1);
 await put(env,{checkpointId:q3b});
 await put(env,{checkpointId:'location.optional',category:'location'});
 summary=await read(env,'/api/admin/summary');
 assert.equal(summary.totals.missingRuns,0);
 assert.equal(summary.stages.find(x=>x.id==='location.optional').registered,false);
});
test('different runs or builds cannot fill prerequisite gaps',async t=>{
 const env=setup(t);await put(env);
 await put(env,{checkpointId:q1,runId:crypto.randomUUID()});
 await put(env,{checkpointId:q1,buildId:'demo-2'});
 const summary=await read(env,'/api/admin/summary?buildId=demo-1');
 assert.equal(summary.totals.runs,2);
 assert.equal(summary.totals.missingRuns,1);
 assert.deepEqual(summary.builds,['demo-1','demo-2']);
});
test('summary counts distinct players, per-run earliest seconds and completion without predecessors',async t=>{
 const env=setup(t);await put(env);await put(env,{playtimeSeconds:100});
 await put(env,{runId:crypto.randomUUID(),playtimeSeconds:200});
 await put(env,{checkpointId:'demo.complete',category:'complete',playtimeSeconds:300});
 const s=await read(env,'/api/admin/summary');
 assert.equal(s.totals.players,1);assert.equal(s.totals.runs,2);assert.equal(s.totals.completedPlayers,1);
 assert.equal(s.stages.find(x=>x.id===q2).runs,2);
 assert.equal(s.stages.find(x=>x.id===q2).averageSeconds,150);
});
test('pagination is bounded and empty results are valid',async t=>{
 const env=setup(t);
 let result=await read(env,'/api/admin/runs');assert.equal(result.total,0);assert.deepEqual(result.items,[]);
 for(let i=0;i<27;i++) await put(env,{runId:crypto.randomUUID()});
 result=await read(env,'/api/admin/runs?page=2');
 assert.equal(result.pageSize,25);assert.equal(result.total,27);assert.equal(result.items.length,2);
});
