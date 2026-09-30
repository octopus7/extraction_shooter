import { DatabaseSync } from 'node:sqlite';
import { readFileSync, existsSync } from 'node:fs';
export function createDatabase() {
  const sqlite = new DatabaseSync(':memory:');
  const migration = new URL('../../migrations/0001_initial.sql', import.meta.url);
  if (existsSync(migration)) sqlite.exec(readFileSync(migration, 'utf8'));
  const db = {
    prepare(sql) {
      const statement = sqlite.prepare(sql);
      let args = [];
      const query = {
        bind(...values) { args = values; return query; },
        async run() { const r = statement.run(...args); return {success:true, meta:{changes:Number(r.changes)}}; },
        async first(column) { const r = statement.get(...args); return r ? (column ? r[column] : {...r}) : null; },
        async all() { return {success:true, results:statement.all(...args).map(r=>({...r}))}; }
      };
      return query;
    },
    close(){sqlite.close();}, sqlite
  };
  return db;
}
export function makeEvent(overrides={}) {
 return {eventId:crypto.randomUUID(), playerId:'10000000-0000-4000-8000-000000000001',
 runId:'20000000-0000-4000-8000-000000000001',buildId:'demo-1',dataset:'demo',
 checkpointId:'quest.demo_q2_clear_water_screen', category:'quest',playtimeSeconds:125,...overrides};
}
export function eventRequest(event) {
 return new Request('https://tracker.test/api/events',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(event)});
}
