"""Build static review gallery from saved actual renders and measured manifest."""
from pathlib import Path
import json,html
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'TunaSweeper/SourceArt/Environment/ExtractionMarkers';m=json.loads((OUT/'model_manifest.json').read_text())
sections=[('Actual UE renders',list((OUT/'Previews').glob('UE_*.png'))),('Actual Blender / FBX renders',list((OUT/'Previews').glob('Blender_*.png'))+list((OUT/'Previews').glob('FBX_*.png'))),('ImageGen concept reference (not model render)',[OUT/'References/ImageGen_Concept.png']),('ImageGen texture atlas',[OUT/'Textures/T_EM_Atlas.png'])]
doc=['<!doctype html><html lang="en"><meta charset="utf-8"><title>Extraction markers review</title><style>body{font:16px system-ui;background:#172122;color:#e2e9e5;margin:32px}h1,h2{color:#a4dfa7}main{max-width:1400px;margin:auto}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(420px,1fr));gap:18px}figure{margin:0;background:#243232;padding:12px;border-radius:8px}img{width:100%;height:auto}figcaption{padding-top:9px}table{border-collapse:collapse}td,th{padding:9px 25px;border:1px solid #4e6260}a{color:#a4dfa7}</style><main><h1>Extraction marker props</h1><p>3 independent props · 204 triangles · 5 material slots · no prop light actors. Sample extraction radius 300 cm; static editor and separately labeled SIE previews; sample travel disabled.</p><table><tr><th>Mesh</th><th>Triangles</th><th>Slots</th><th>Bounds size cm</th></tr>']
for e in m['assets']:
 b=e['bounds_m'];doc.append(f"<tr><td>{e['name']}</td><td>{e['triangles']}</td><td>{e['material_slots']}</td><td>{' × '.join(f'{(b[i+3]-b[i])*100:.1f}' for i in range(3))}</td></tr>")
doc.append('</table><p>Arrow points +X (north). UE camera yaw 0: +Y is screen right. Source topdown applies an explicit horizontal view flip for the same display convention.</p>')
for title,paths in sections:
 if not paths:continue
 doc.append('<h2>'+title+'</h2><div class="grid">')
 for p in sorted(paths):
  ref=p.relative_to(OUT).as_posix();label=html.escape(p.stem.replace('_',' '));doc.append(f'<figure><a href="{ref}"><img loading="lazy" src="{ref}" alt="{label}"></a><figcaption>{label}</figcaption></figure>')
 doc.append('</div>')
doc.append('</main></html>');(OUT/'Review.html').write_text('\n'.join(doc),encoding='utf-8')
