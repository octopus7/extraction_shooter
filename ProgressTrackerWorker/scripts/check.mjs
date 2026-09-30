import {readdir,readFile} from 'node:fs/promises';
import {spawnSync} from 'node:child_process';
import {resolve,relative} from 'node:path';
import {fileURLToPath} from 'node:url';
import {catalog,validateCatalog} from '../src/catalog.js';
const root=fileURLToPath(new URL('..',import.meta.url));
async function checkDirectory(dir){
 for(const entry of await readdir(dir,{withFileTypes:true})){
  const path=resolve(dir,entry.name);
  if(entry.isDirectory()) await checkDirectory(path);
  else if(/\.(mjs|js)$/.test(entry.name)){
   const r=spawnSync(process.execPath,['--check',path],{encoding:'utf8'});
   if(r.status!==0) throw new Error(r.stderr||relative(root,path));
  }
 }
}
for(const dir of ['src','public','scripts','test']) await checkDirectory(resolve(root,dir));
validateCatalog(catalog);
const strings=JSON.parse(await readFile(resolve(root,'public/strings/ko.json'),'utf8'));
for(const entry of catalog) if(!Object.hasOwn(strings,entry.labelKey)) throw new Error('Missing string: '+entry.labelKey);
const config=JSON.parse(await readFile(resolve(root,'wrangler.jsonc'),'utf8'));
if(config.assets.directory!=='./public'||config.d1_databases[0].binding!=='DB') throw new Error('Invalid bindings');
console.log('JavaScript syntax, catalog, strings and binding configuration checked.');
