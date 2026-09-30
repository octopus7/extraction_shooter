const q=id=>'quest.'+id;
const q1=q('demo_q1_water_intake_check'),q2=q('demo_q2_clear_water_screen');
const q3a=q('demo_q3a_repair_valve'),q3b=q('demo_q3b_repair_bunker_pipe'),q4=q('demo_q4_todays_reward');
export const catalog=[
 {id:'game.start',category:'start',labelKey:'stage.start',requires:[]},
 {id:q1,category:'quest',labelKey:'stage.quest1',requires:[]},
 {id:q2,category:'quest',labelKey:'stage.quest2',requires:[q1]},
 {id:q3a,category:'quest',labelKey:'stage.quest3a',requires:[q2]},
 {id:q3b,category:'quest',labelKey:'stage.quest3b',requires:[q2]},
 {id:q4,category:'quest',labelKey:'stage.quest4',requires:[q3a,q3b]},
 {id:'demo.complete',category:'complete',labelKey:'stage.complete',requires:[q4]}
];
export function validateCatalog(entries){
 const byId=new Map(entries.map(e=>[e.id,e]));
 if(byId.size!==entries.length) throw new Error('duplicate checkpoint');
 for(const e of entries){
  if(!/^[A-Za-z0-9][A-Za-z0-9_.:-]{0,127}$/.test(e.id) || !Array.isArray(e.requires) || !e.labelKey) throw new Error('invalid checkpoint');
  for(const id of e.requires) if(!byId.has(id)) throw new Error('unknown prerequisite');
 }
 const visited=new Set(),visiting=new Set();
 function visit(id){
  if(visiting.has(id)) throw new Error('cyclic prerequisite');
  if(visited.has(id)) return;
  visiting.add(id);for(const parent of byId.get(id).requires) visit(parent);
  visiting.delete(id);visited.add(id);
 }
 for(const e of entries) visit(e.id);
}
validateCatalog(catalog);
export function describeCheckpoint(id,category){
 const entry=catalog.find(e=>e.id===id);
 return {id,category:entry?.category??category,labelKey:entry?.labelKey??null,registered:!!entry};
}
