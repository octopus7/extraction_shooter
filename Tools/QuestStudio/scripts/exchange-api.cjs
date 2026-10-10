const { latest, t } = require('./exchange.cjs');

function createClient(origin, fetcher = fetch) {
  const url = new URL(origin);
  if (url.protocol !== 'https:' || url.username || url.password || url.pathname !== '/' || url.search || url.hash) throw new Error(t('exchange.origin'));
  let cookie = '';
  async function request(route, body) {
    let response;
    try {
      response = await fetcher(new URL(route, url), { method: body === undefined ? 'GET' : 'POST', redirect: 'error',
        headers: { origin: url.origin, ...(body === undefined ? {} : { 'content-type': 'application/json' }), ...(cookie ? { cookie } : {}) },
        ...(body === undefined ? {} : { body: JSON.stringify(body) }), signal: AbortSignal.timeout(30000) });
    } catch { throw new Error(t('exchange.network')); }
    if (!response.ok) {
      // Never print a response body: a proxy/server could echo credentials or cookies.
      if (response.status === 429) throw new Error(t('exchange.rateLimit', { seconds: Number(response.headers.get('retry-after')) || 900 }));
      throw new Error(t('exchange.http', { status: response.status }));
    }
    if (route === '/api/login') {
      const token = /(?:^|,\s*)(__Host-quest_session=[A-Za-z0-9_-]{43})(?:;|$)/.exec(response.headers.get('set-cookie') ?? '');
      if (!token) throw new Error(t('exchange.response'));
      cookie = token[1];
    }
    try { return await response.json(); } catch { throw new Error(t('exchange.response')); }
  }
  return {
    login: password => request('/api/login', { password }),
    upload: body => request('/api/snapshots', body),
    async downloadLatest() {
      for (let attempt = 0; attempt < 3; attempt++) {
        const selected = latest((await request('/api/snapshots')).snapshots);
        const loaded = await request('/api/snapshots/' + encodeURIComponent(selected.id));
        const check = latest((await request('/api/snapshots')).snapshots);
        if (loaded.snapshot?.id === selected.id && loaded.snapshot.id === check.id && loaded.snapshot.revision === check.revision && loaded.snapshot.updatedAt === check.updatedAt) return loaded;
      }
      throw new Error(t('exchange.remoteChanged'));
    },
    async logout() { if (cookie) { try { await request('/api/logout', {}); } finally { cookie = ''; } } },
  };
}
module.exports = { createClient };
