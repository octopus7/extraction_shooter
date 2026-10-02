import {HttpError} from './http.js';
export async function requireAdmin(request,env){
 const expected=env.ADMIN_TOKEN;
 if(typeof expected!=='string' || expected.length<24 || expected.length>256) throw new HttpError(503,'not_configured');
 const match=/^Bearer ([\x21-\x7e]{1,256})$/.exec(request.headers.get('Authorization')??'');
 if(!match) throw new HttpError(401,'unauthorized');
 const bytes=new TextEncoder();
 const algorithm={name:'HMAC',hash:'SHA-256'};
 const key=await crypto.subtle.importKey('raw',bytes.encode('progress-tracker-admin-comparison'),algorithm,false,['sign','verify']);
 const signature=await crypto.subtle.sign('HMAC',key,bytes.encode(expected));
 const equal=await crypto.subtle.verify('HMAC',key,signature,bytes.encode(match[1]));
 if(!equal) throw new HttpError(401,'unauthorized');
}
