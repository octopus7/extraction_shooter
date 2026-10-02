import test from 'node:test';
import assert from 'node:assert/strict';
import worker from '../src/worker.js';
import {createDatabase,makeEvent,eventRequest} from './support/database.js';
test('accepts a later milestone without earlier events', async t=>{
 const DB=createDatabase(); t.after(()=>DB.close());
 const response=await worker.fetch(eventRequest(makeEvent()),{DB});
 assert.equal(response.status,201);
 assert.equal(DB.sqlite.prepare('SELECT COUNT(*) AS n FROM events').get().n,1);
});
test('exact duplicate is idempotent and conflicting event ID cannot overwrite',async t=>{
 const DB=createDatabase(); t.after(()=>DB.close()); const event=makeEvent();
 assert.equal((await worker.fetch(eventRequest(event),{DB})).status,201);
 assert.equal((await worker.fetch(eventRequest(event),{DB})).status,200);
 assert.equal((await worker.fetch(eventRequest({...event,playtimeSeconds:999}),{DB})).status,409);
 assert.equal(DB.sqlite.prepare('SELECT playtime_seconds AS n FROM events').get().n,125);
});
test('validates identities, categories, fields and cumulative time', async t=>{
 const DB=createDatabase(); t.after(()=>DB.close());
 for(const patch of [{playerId:'bad'},{dataset:'main'},{category:'start'},{playtimeSeconds:-1},
 {playtimeSeconds:Infinity},{checkpointId:'x'.repeat(129)},{buildId:'<script>'},{extra:'value'},
 {checkpointId:'demo.complete',category:'quest'}]){
  assert.equal((await worker.fetch(eventRequest(makeEvent(patch)),{DB})).status,400,JSON.stringify(patch));
 }
 assert.equal((await worker.fetch(eventRequest(makeEvent({checkpointId:'location.new_area',category:'location'})),{DB})).status,201);
});
test('bounds body bytes without trusting Content-Length',async()=>{
 const request=new Request('https://tracker.test/api/events',{method:'POST',headers:{'Content-Type':'application/json'},body:' '.repeat(16385)});
 assert.equal((await worker.fetch(request,{})).status,413);
});
test('rejects invalid JSON and non-JSON payloads',async()=>{
 for(const [body,type,status] of [['{','application/json',400],['{}','text/plain',415]]){
  assert.equal((await worker.fetch(new Request('https://tracker.test/api/events',{method:'POST',headers:{'Content-Type':type},body}),{})).status,status);
 }
});
test('database error never acknowledges event success',async()=>{
 const DB={prepare(){throw new Error('private backend detail');}};
 const response=await worker.fetch(eventRequest(makeEvent()),{DB});
 assert.equal(response.status,503);
 assert.equal((await response.text()).includes('private backend'),false);
});
