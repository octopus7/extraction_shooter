"""Generate the review index and measured model table from the final manifest."""
from pathlib import Path
import json,html
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion'
m=json.loads((OUT/'model_manifest.json').read_text())
names=['절반 폭 바닥판','배수 그레이팅 바닥판','바닥 경계 마감','절반 폭 벽판','창 개구부 벽','낮은 칸막이','바깥쪽 코너','벽 끝 마감','독립 문틀','관찰창 삽입 모듈','직선 배관','90도 배관','배관 끝단','직선 덕트','덕트 코너','환기구','벽 부착 비상등','구역 표지판']
cards=[];rows=[]
for i,(e,label) in enumerate(zip(m['assets'],names),1):
    dims=[round((e['bounds_m'][j+3]-e['bounds_m'][j])*100,2) for j in range(3)]
    rows.append(f"| {i}. {label} / {e['key']} | {' × '.join(map(str,dims))} | {e['triangles']} | {e['material_slots']} | {len(e['collision_boxes'])} |")
    cards.append(f'<article><a href="Models/{e["name"]}.fbx"><img src="Previews/Module_{e["key"]}.png" loading="lazy"></a><h3>{i:02} · {label}</h3><p>{e["key"]} · {e["triangles"]} tris · {e["material_slots"]} slots · {len(e["collision_boxes"])} boxes</p></article>')
def picture(name,label):return f'<figure><a href="Previews/{name}.png"><img src="Previews/{name}.png" loading="lazy"></a><figcaption>{label}</figcaption></figure>'
page='''<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>실내 모듈 확장 · 18종</title><style>body{margin:0;background:#131a1e;color:#e4e9eb;font:16px system-ui;line-height:1.55}main{max-width:1400px;margin:auto;padding:40px}h1{font-size:42px}h2{margin-top:55px;color:#b6d7d9}p{color:#acb9bf}a{color:#a2d8dd}img{width:100%;display:block;border-radius:8px}figure{margin:18px 0}figcaption{padding:10px 0;color:#bccacf}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(310px,1fr));gap:18px}.modules{grid-template-columns:repeat(auto-fit,minmax(270px,1fr))}article{background:#202a30;padding:12px;border-radius:10px}article h3{font-size:17px}article p{font-size:14px}.stats{font-size:21px;color:#8ed3cc}strong{color:#eef4f6}</style><main>'''
page+=f'<h1>실내 모듈 확장 · 18종</h1><p>기본 6종과 함께 조립한 천장 없는 산업 시설 검토 장면</p><p class="stats">확장 {m["unique_triangles"]} tris · 전체 고유 {m["combined_unique_triangles"]} tris · {m["sample_instances"]} 배치 / {m["sample_triangles"]} tris</p>'
page+='<p><a href="ModularInteriorExpansion.blend">Blender 원본</a> · <a href="model_manifest.json">치수·배치 명세</a> · <a href="source_validation.json">Blender/FBX 검증</a> · <a href="unreal_reload_validation.json">UE 재로드 검증</a> · <a href="unreal_map_reload_validation.json">UE 맵·충돌 검증</a></p>'
page+='<p>바닥 그레이팅·환기구의 홈은 불투명 텍스처 표현입니다. 관통구가 아닙니다. 유리는 별도 반투명 슬롯이며 굴절은 없습니다. ZONE 01은 시각 검토용 타이포그래피로 게임 문자열을 변경하지 않습니다.</p>'
page+='<h2>전체 조립 · Blender</h2>'+picture('Overview','기본 6종 + 확장 18종을 함께 배치한 전체 장면')+'<div class="grid">'+picture('Annex','확장 구역')+picture('PlayCamera','프로젝트 설정과 같은 탑다운 카메라: 1500cm / −88° / FOV70')+'</div>'
page+='<h2>저장된 UE 샘플 맵</h2><p>UE 에디터 카메라 캡처입니다. PIE 플레이 및 FPS 측정은 수행하지 않았습니다.</p><div class="grid">'
for n,t in [('UE_BasePlayCamera','기본 세트 탑다운'),('UE_ExpansionPlayCamera','확장 세트 탑다운'),('UE_WindowServices','창·벽면 설비'),('UE_DoorFrame','분리 문틀과 기본 문짝')]:page+=picture(n,t)
page+='</div><h2>접합부 · Blender</h2><div class="grid">'
for n,t in [('Window_Services','관찰창, 창 하부 배관, 벽 부착 덕트'),('Door_Frame','기존 문짝 호환 독립 문틀'),('Outside_Corner','바깥쪽 코너와 끝 마감')]:page+=picture(n,t)
page+='</div><h2>공용 오염 마스크 비교</h2><p>Clean 0 / Weak 0.65 / Strong 1.8. 강도를 0으로 설정해도 마스크 텍스처 읽기는 남습니다.</p><div class="grid">'
for n in ['Clean','Weak','Strong']:page+=picture('Dirt_'+n,n)
page+='</div><h2>18종 모듈 시트</h2><div class="grid modules">'+''.join(cards)+'</div></main></html>'
(OUT/'Review.html').write_text(page,encoding='utf-8')
(Path(__file__).parent/'MODEL_TABLE.md').write_text('| 모델 | 크기 X × Y × Z (cm) | 삼각형 | 슬롯 | 단순 박스 |\n|---|---:|---:|---:|---:|\n'+'\n'.join(rows)+'\n',encoding='utf-8')
