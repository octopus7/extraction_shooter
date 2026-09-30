import {HttpError,json} from './http.js';
import {readEvent} from './events.js';
import {storeEvent} from './database.js';
import {requireAdmin} from './auth.js';
import {parseFilter,parsePage,parseIdentity,getSummary,getRuns,getRun} from './statistics.js';
export default {
 async fetch(request,env){
  try{
   const url=new URL(request.url);
   if(url.pathname==='/api/events'){
    if(request.method!=='POST') return json({error:'method_not_allowed'},405);
    const event=await readEvent(request);
    const result=await storeEvent(env.DB,event);
    return json(result,result.duplicate?200:201);
   }
   if(url.pathname==='/api/health' && request.method==='GET') return json({ok:true,preview:env.PREVIEW==='true'});
   if(url.pathname.startsWith('/api/admin/')){
    await requireAdmin(request,env);
    if(request.method!=='GET') return json({error:'method_not_allowed'},405);
    if(url.pathname==='/api/admin/summary') return json(await getSummary(env.DB,parseFilter(url)));
    if(url.pathname==='/api/admin/runs') return json(await getRuns(env.DB,parseFilter(url),parsePage(url)));
    if(url.pathname==='/api/admin/run') return json(await getRun(env.DB,parseIdentity(url)));
   }
   if(url.pathname.startsWith('/api/')) return json({error:'not_found'},404);
   if(env.ASSETS && ['GET','HEAD'].includes(request.method)) return env.ASSETS.fetch(request);
   return json({error:'not_found'},404);
  }catch(error){
   if(error instanceof HttpError) return json({error:error.code},error.status);
   return json({error:'unavailable'},503);
  }
 }
};
