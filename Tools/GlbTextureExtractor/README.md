# GLB Texture Extractor

UE용 텍스처를 GLB에서 외부 PNG로 언팩하는 .NET 10 WinForms 도구입니다.

입력 GLB를 선택하면 출력 폴더는 기본적으로 원본 GLB와 같은 위치로 설정됩니다.

## 동작

- Base Color와 기타 텍스처의 긴 변 목표 해상도를 각각 드롭다운에서 선택합니다. 기본 선택값은 권장값인 1024px와 512px입니다.
- 작은 원본 텍스처는 확대하지 않고 원본 크기를 유지합니다.
- 결과 텍스처는 `T_{입력파일명}_{슬롯}_{순번}.png` 규칙으로 `Textures` 폴더에 저장합니다.
- 추출 파일과 원본/결과 해상도, 머티리얼 슬롯, 건너뜀 사유는 도구의 실행 로그에서만 표시합니다. 별도의 manifest 파일은 저장하지 않습니다.
- 기본 옵션에서는 GLB의 머티리얼과 메시, 메시 노드 이름을 각각 `M_{입력파일명}` 및 `SM_{입력파일명}` 규칙으로 정리한 복사본을 선택한 출력 폴더 바로 아래에 저장합니다. 이 복사본은 임베딩 이미지 BIN 데이터를 제거하고, 머티리얼 텍스처를 `Textures/T_*.png` 상대 경로로 다시 연결합니다. 따라서 GLB와 `Textures` 폴더를 함께 유지해야 합니다.

KTX2/BasisU 및 GDI+가 해석하지 못하는 이미지 형식은 리샘플하지 않고 manifest에 기록합니다.

## 실행

`RunGlbTextureExtractor.bat`을 실행하거나 다음 명령을 사용합니다.

```powershell
dotnet run --configuration Release --project Tools/GlbTextureExtractor/GlbTextureExtractor.csproj
```
