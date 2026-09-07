"""Build a local, self-contained review index referencing preserved model renders."""
from pathlib import Path
import json,html
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'TunaSweeper/SourceArt/Environment/LabSupplyProps'
m=json.loads((OUT/'model_manifest.json').read_text())
labels=['금속 작업대','개방형 철제 선반','낮은 수납장','이동식 카트','작업용 의자','벽걸이 제어함','대형 분석 장비','실험 싱크대','시료 용기·트레이','팔레트','밀폐형 보급 상자','박스 적재 묶음']
def img(path,title):return f'<figure><a href="{path}"><img loading="lazy" src="{path}" alt="{html.escape(title)}"></a><figcaption>{html.escape(title)}</figcaption></figure>'
parts=['''<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Lab & Supply · 실제 모델 검토</title><style>body{margin:0;background:#141c23;color:#dde7eb;font:16px/1.6 system-ui}main{max-width:1400px;margin:auto;padding:36px}h1{font-size:36px;margin-bottom:6px}h2{margin-top:50px;color:#8fc3c5}p{max-width:960px;color:#b9c8ce}a{color:#99d9df}figure{margin:0;background:#202b34;border-radius:8px;overflow:hidden}img{display:block;width:100%}figcaption{padding:12px 16px}nav a{margin-right:20px}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:18px}.pair{display:grid;grid-template-columns:1fr 1fr;gap:18px}.wide{margin:20px 0}.chip{color:#8fc3c5}summary{cursor:pointer}table{border-collapse:collapse;width:100%}td,th{text-align:left;padding:8px;border-bottom:1px solid #35414b}@media(max-width:800px){.grid{grid-template-columns:1fr 1fr}.pair{grid-template-columns:1fr}main{padding:16px}}</style><main><span class="chip">TUNASWEEPER / LAB SUPPLY PROPS / UE 5.7</span><h1>연구소 · 보급 창고 12종</h1><p>아래는 Blender 메시와 공유 아틀라스로 렌더한 실제 모델입니다. ImageGen 컨셉 이미지는 맨 아래에 별도로 보관했습니다. 각 이미지를 누르면 원본 크기로 열립니다.</p><nav><a href="#models">모델 12종</a><a href="#rooms">두 공간</a><a href="#wear">오염 비교</a><a href="#reference">ImageGen 원본</a><a href="../../../../Tools/LabSupplyProps/README.md">조합 안내</a></nav><h2 id="models">실제 모델 · 개별 렌더</h2><div class="grid">''']
for i,(e,label) in enumerate(zip(m['assets'],labels)):
    size=' × '.join(f'{v*100:g}' for v in e['dimensions_m'])
    parts.append(img('Previews/Model_'+e['key']+'.png',f'{i+1:02d} {label} · {size} cm · {e["triangles"]} tris · 1 slot'))
parts.append('</div><h2>전체 모델 시트</h2>'+img('Previews/Models_12_Sheet.png','Blender actual geometry · 12 unique props · 2,368 triangles'))
parts.append('<h2 id="rooms">전용 공간 · 프로젝트 탑다운 카메라와 사선</h2><p>기본 실내 모듈을 재사용한 8×8m 공간입니다. 천장과 보가 없습니다. 탑다운은 arm 1500cm, pitch −88°, yaw 0°, FOV 70°에 맞췄습니다. 높은 선반의 상판은 아래 적재물을 가리므로 벽면에 배치했습니다. 중앙 동선은 별도 UE 레이 검사 대상입니다.</p><div class="pair">')
for group,label in [('Lab','연구소'),('Warehouse','보급 창고')]:
    for view,title in [('PlayCamera','Blender / 탑다운'),('Oblique','Blender / 사선')]:parts.append(img(f'Previews/{group}_{view}.png',label+' · '+title))
parts.append('</div><h2>UE 에디터 캡처</h2><p>저장된 전용 맵의 카메라 캡처입니다. PIE나 FPS 측정 결과가 아닙니다.</p><div class="pair">')
for group,label in [('Lab','연구소'),('Warehouse','보급 창고')]:
    for camera,title in [('GameCamera','프로젝트 카메라'),('ObliqueCamera','사선')]:parts.append(img(f'Previews/UE_LSP_{group}_{camera}.png',label+' · UE '+title))
parts.append('</div><h2 id="wear">기본 낡음 + 추가 오염</h2><p>좌측은 ImageGen 아틀라스에 들어 있는 기본 사용감만, 우측은 공용 오염 마스크 강도 1.8을 더한 모습입니다. 기본 재질 강도는 0.35입니다. 넓은 오염 데칼과 투명 재질을 쓰지 않습니다.</p><div class="pair">')
for view,title in [('Base','기본 낡음'),('Dust','추가 오염 1.8')]:parts.append(img('Previews/CloseWear_'+view+'.png',title))
for view,title in [('BaseOnly','기본 낡음 / 전체'),('AdditionalDirt','추가 오염 / 전체')]:parts.append(img('Previews/Wear_'+view+'.png',title))
parts.append('</div><h2>측정 합계</h2><table><tr><th>장면</th><th>배치</th><th>삼각형</th><th>가구/소품 삼각형</th></tr>')
for group in ('Lab','Warehouse'):
    t=m['sample_totals'][group];parts.append(f'<tr><td>{group}</td><td>{t["instances"]}</td><td>{t["triangles"]:,}</td><td>{t["prop_triangles"]:,}</td></tr>')
parts.append('</table><p>12종 합계 2,368삼각형, 재질 슬롯 12개, 단순 충돌 박스 140개. 공통 Opaque 재질 하나는 2048² 아틀라스와 기존 1024² 오염 마스크를 2회 읽습니다. 표의 배치 합계에는 기본 벽·바닥·문틀을 포함하고 조명·카메라·크기 프록시는 제외합니다. 상세 검사와 한계는 <a href="../../../../Tools/LabSupplyProps/VALIDATION.md">검증 보고서</a>를 확인하세요.</p>')
parts.append('<h2 id="reference">ImageGen 컨셉 레퍼런스 · 실제 모델 렌더와 구분</h2>'+img('References/ImageGen_Concept12.png','ImageGen concept reference — NOT a model render'))
parts.append('<details><summary>ImageGen 공유 아틀라스 / 원본과 프롬프트</summary><div class="pair">'+img('Textures/ImageGen_Atlas_Final.png','ImageGen atlas final')+img('Textures/ImageGen_Atlas_v1.png','ImageGen atlas before two-tile edit')+'</div><p><a href="References/concept.prompt.txt">컨셉 프롬프트</a> · <a href="Textures/atlas.prompt.txt">아틀라스 프롬프트</a> · <a href="Textures/atlas-edit.prompt.txt">두 칸 수정 프롬프트</a></p></details></main></html>')
(OUT/'Review.html').write_text(''.join(parts),encoding='utf-8')
print('LSP_REVIEW_WRITTEN')

