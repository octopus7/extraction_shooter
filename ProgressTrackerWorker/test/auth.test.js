import test from 'node:test';
import assert from 'node:assert/strict';
import worker from '../src/worker.js';
import {createDatabase} from './support/database.js';
const token='test-admin-secret-at-least-32-characters';
test('admin endpoints fail closed without configuration or correct token',async t=>{
 const DB=createDatabase();t.after(()=>DB.close());
 for(const route of ['/api/admin/summary','/api/admin/runs','/api/admin/run']){
  for(const [env,auth,status] of [[{DB},token,503],[{DB,ADMIN_TOKEN:token},'',401],[{DB,ADMIN_TOKEN:token},'wrong',401]]){
   const r=await worker.fetch(new Request('https://tracker.test'+route,{headers:{Authorization:'Bearer '+auth}}),env);
   assert.equal(r.status,status,route);assert.equal(r.headers.get('Cache-Control'),'no-store');
  }
 }
});
test('authorized requests return data and reject invalid filters',async t=>{
 const DB=createDatabase();t.after(()=>DB.close());
 const env={DB,ADMIN_TOKEN:token};
 const get=path=>worker.fetch(new Request('https://tracker.test'+path,{headers:{Authorization:'Bearer '+token}}),env);
 assert.equal((await get('/api/admin/summary')).status,200);
 for(const path of ['/api/admin/summary?buildId=%27OR%201=1','/api/admin/runs?page=-1','/api/admin/runs?page=1.5','/api/admin/run?runId=oops'])
  assert.equal((await get(path)).status,400,path);
});
test('health reveals no credentials and API routes do not fall through to HTML',async()=>{
 const r=await worker.fetch(new Request('https://tracker.test/api/health'),{ADMIN_TOKEN:token});
 assert.equal(r.status,200);assert.equal((await r.text()).includes(token),false);
 assert.equal((await worker.fetch(new Request('https://tracker.test/api/unknown'),{})).status,404);
});
