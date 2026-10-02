import {HttpError} from './http.js';
export const MAX_BODY_BYTES=16384;
export const ID_PATTERN=/^[A-Za-z0-9][A-Za-z0-9_.:-]{0,127}$/;
export const UUID_PATTERN=/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;
const fields=['eventId','playerId','runId','buildId','dataset','checkpointId','category','playtimeSeconds'];
export function validateEvent(input){
 const fail=()=>{throw new HttpError(400,'invalid_event');};
 if(!input || Array.isArray(input) || typeof input!=='object' || Object.keys(input).length!==fields.length || fields.some(k=>!Object.hasOwn(input,k))) fail();
 for(const key of ['eventId','playerId','runId']) if(typeof input[key]!=='string' || !UUID_PATTERN.test(input[key]) || /^0+-0+-0+-0+-0+$/.test(input[key])) fail();
 for(const key of ['buildId','checkpointId']) if(typeof input[key]!=='string' || !ID_PATTERN.test(input[key])) fail();
 if(input.dataset!=='demo' || typeof input.playtimeSeconds!=='number' || !Number.isFinite(input.playtimeSeconds) || input.playtimeSeconds<0 || input.playtimeSeconds>315576000) fail();
 const categoryMatches = (input.category==='start' && input.checkpointId==='game.start') ||
 (input.category==='complete' && input.checkpointId==='demo.complete') ||
 (input.category==='quest' && input.checkpointId.startsWith('quest.') && input.checkpointId.length>6) ||
 (input.category==='location' && input.checkpointId.startsWith('location.') && input.checkpointId.length>9);
 if(!categoryMatches) fail();
 return {...input,eventId:input.eventId.toLowerCase(),playerId:input.playerId.toLowerCase(),runId:input.runId.toLowerCase()};
}
export async function readEvent(request){
 if(request.headers.get('Content-Type')?.split(';')[0].trim().toLowerCase()!=='application/json') throw new HttpError(415,'unsupported_media_type');
 if(Number(request.headers.get('Content-Length'))>MAX_BODY_BYTES) throw new HttpError(413,'payload_too_large');
 if(!request.body) throw new HttpError(400,'invalid_event');
 const reader=request.body.getReader(); const chunks=[]; let length=0;
 try{
  while(true){
   const {value,done}=await reader.read(); if(done) break;
   length+=value.byteLength;
   if(length>MAX_BODY_BYTES){await reader.cancel();throw new HttpError(413,'payload_too_large');}
   chunks.push(value);
  }
 }finally{reader.releaseLock();}
 const bytes=new Uint8Array(length);let offset=0;
 for(const chunk of chunks){bytes.set(chunk,offset);offset+=chunk.byteLength;}
 let parsed;
 try{parsed=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(bytes));}
 catch{throw new HttpError(400,'invalid_event');}
 return validateEvent(parsed);
}
