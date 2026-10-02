import {HttpError} from './http.js';
const columns=['event_id','player_id','run_id','build_id','dataset','checkpoint_id','category','playtime_seconds'];
function values(e){return [e.eventId,e.playerId,e.runId,e.buildId,e.dataset,e.checkpointId,e.category,e.playtimeSeconds];}
export async function storeEvent(db,event){
 const args=values(event);
 const result=await db.prepare('INSERT INTO events ('+columns.join(',')+') VALUES (?,?,?,?,?,?,?,?) ON CONFLICT(event_id) DO NOTHING').bind(...args).run();
 if(!result.success) throw new Error('storage');
 if(result.meta.changes>0) return {accepted:true,duplicate:false};
 const prior=await db.prepare('SELECT '+columns.join(',')+' FROM events WHERE event_id=?').bind(event.eventId).first();
 if(!prior || columns.some((c,i)=>prior[c]!==args[i])) throw new HttpError(409,'event_conflict');
 return {accepted:true,duplicate:true};
}
