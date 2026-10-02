export class HttpError extends Error {
 constructor(status,code){ super(code); this.status=status; this.code=code; }
}
export function json(value,status=200){
 return Response.json(value,{status,headers:{'Cache-Control':'no-store','X-Content-Type-Options':'nosniff'}});
}
