# 타이틀 배경 상하 확장

- 입력: `chatgpt/title_matte_lake.png`
- 출력: `chatgpt/title_matte_lake_extended.png`
- 도구: 내장 imagegen 이미지 편집. 원본은 보존했다.
- UE 텍스처는 정사각 비율을 유지하는 2의 거듭제곱 크기와 밉맵을 사용하며, 가장자리 흐림은 투영 머티리얼에서 추가한다.

사용 프롬프트:

> Edit target: supplied lakeside matte painting. Outpaint vertically to a SQUARE 1:1 image. Preserve the original wide 1670x942 scene as the exact central horizontal band, at its existing composition, perspective, relative object positions, painterly brushwork, palette and detail. Add substantial new scenery ABOVE and BELOW, so the original band takes approximately the central 56 percent of the square canvas height and full width. Top extension: continue blue sky, cloud wisps and the existing large tree canopy naturally. Bottom extension: continue the sunlit dirt path, grass, foreground leaves and ground in the existing perspective. Keep the original fish-marked crate, lake, tree, barrel, lantern and distant mountains unchanged in appearance and placement within the original band. This will be a game title matte background, with a separate 3D character added later: NO characters, no text, no logo, no borders. Preserve sharp central scenery; gradually soften only the far outermost edges with natural defocus. Do not stretch, warp, crop away or redesign the source scene. Deliver a high-resolution square bitmap with seamless vertical outpainting.
