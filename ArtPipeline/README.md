# DualFire Art Pipeline

이미지 입력만 사용하는 DualFire 도트 스프라이트 제작 파이프라인이다. Blender, Ollama, Paper2D, Unreal 런타임 변경은 포함하지 않는다.

## 환경

- Python 3.11~3.13
- ComfyUI Desktop 설치 필요
- ComfyUI 데이터 경로: `config/pipeline.local.yaml`에서만 설정
- API: `http://127.0.0.1:8001`
- GPU 기준: RTX 4090 24GB

RTX 4090 24GB 환경은 BF16 본체 대신 공식 FP8 Mixed 변형을 사용한다. 모델은 자동으로 받지 않는다. ComfyUI 데이터 경로 아래에 직접 배치한다.

```text
models/diffusion_models/qwen_image_edit_2511_fp8mixed.safetensors
models/text_encoders/qwen_2.5_vl_7b_fp8_scaled.safetensors
models/vae/qwen_image_vae.safetensors
models/loras/Qwen-Image-Edit-2511-Lightning-4steps-V1.0-bf16.safetensors
```

## 설치

```powershell
cd ArtPipeline
uv sync --extra dev
uv run dualfire-art doctor
```

## 기본 흐름

```powershell
uv run dualfire-art init-asset assets\enemies\normal\air\DF_EN_AIR_SCOUT_01 --asset-id DF_EN_AIR_SCOUT_01 --category enemy --grade normal --attribute air
# input/source.png 배치 후
uv run dualfire-art prepare assets\enemies\normal\air\DF_EN_AIR_SCOUT_01
uv run dualfire-art submit-comfy assets\enemies\normal\air\DF_EN_AIR_SCOUT_01 --state idle
uv run dualfire-art approve assets\enemies\normal\air\DF_EN_AIR_SCOUT_01 --state idle --candidate 0
uv run dualfire-art build-frames assets\enemies\normal\air\DF_EN_AIR_SCOUT_01 --state idle
uv run dualfire-art pack assets\enemies\normal\air\DF_EN_AIR_SCOUT_01 --state idle
uv run dualfire-art validate assets\enemies\normal\air\DF_EN_AIR_SCOUT_01
```

불투명 PNG, JPG, WEBP는 `source.mask_file`에 같은 크기의 이진 마스크가 필요하다. 승인 전 Qwen 결과는 최종 출력으로 이동하지 않는다.

## 출력

- `output/frames/`: 개별 RGBA 프레임
- `output/sheets/`: 가로 1행 스프라이트 시트
- `output/metadata/`: 프레임, FPS, 피벗, 앵커, 해시
- `output/preview/`: 최근접 8배 확대 검수 이미지

`input/`, `work/`, 미리보기, 모델, 로컬 절대경로 설정은 Git에서 제외된다.

## Green-Screen Sprite Input

For a pixel-art reference on a flat green background, use the pipeline instead of creating a separate mask or crop script:

```yaml
source:
  file: input/front.png
  mask_file: null
  chroma_key:
    color: [0, 255, 0]
    tolerance: 48
  frame_fit:
    padding: 4
animations:
  idle:
    source_frames:
      - work/normalized/fitted.png
```

`prepare` removes the chroma key, writes a binary alpha mask, crops to visible pixels, and fits the result to the target cell with nearest-neighbor scaling. `mask_file` and `chroma_key` cannot be used together.

For sparse VFX pixels such as afterburner cores, reserve exact output colors without increasing the palette limit:

```yaml
target:
  palette_limit: 16
  reserved_palette_colors:
    - [255, 242, 200]
    - [255, 188, 74]
    - [255, 112, 36]
```

## Codex MCP

- 서버: `comfyui-mcp@0.30.0`
- 설정: 저장소의 `.codex/config.toml`
- 로컬 URL/데이터 경로: Git에서 제외된 `config/pipeline.local.yaml`
- 현재 API: `http://127.0.0.1:8001` 응답 정상
- 현재 런타임: ComfyUI `0.27.0`, PyTorch `2.10.0+cu130`, RTX 4090 CUDA 정상
- 워크플로 검증: 로컬 GPU 실행, 13개 노드와 필수 노드 타입 인식 정상
- 실행 대기: Qwen 모델 3종과 전용 입력 이미지가 아직 없음

Codex는 프로젝트 설정을 새 세션에서 로드한다. MCP 실행 도구는 허용 목록으로 제한되며 모델·노드 설치, ComfyUI 프로세스 제어, 클라우드 생성 도구는 노출하지 않는다.

## 기준 출처

API 그래프는 Comfy-Org의 `image_qwen_image_edit_2511.json`을 기준으로 작성했다. 기본 워크플로우는 최종 40-step 설정이고, 빠른 확인은 `qwen_image_edit_2511_lightning_4step_api.json`과 4 steps, CFG 1.0을 사용한다.

- https://github.com/Comfy-Org/workflow_templates/blob/main/templates/image_qwen_image_edit_2511.json
- https://docs.comfy.org/tutorials/image/qwen/qwen-image-edit-2511

## Opaque Image To Pixel Sprite

For a supplied JPG, PNG, or WEBP with an opaque background, create a pixel-sprite package first. The profile preserves the supplied silhouette, removes the background through local BiRefNet, then uses deterministic downscale, palette quantization, and nearest-neighbor upscaling. It does not call Qwen unless `submit-comfy` is explicitly requested.

```powershell
uv run dualfire-art init-asset assets\player_fighter --asset-id DF_PL_FIGHTER_01 --category player --profile pixel-sprite
# Place the source at assets\player_fighter\input\source.png
uv run dualfire-art extract-mask assets\player_fighter --apply-source-mask
uv run dualfire-art prepare assets\player_fighter
uv run dualfire-art import-keyframe assets\player_fighter --state idle --file assets\player_fighter\input\source.png --mask work/masks/birefnet.png
uv run dualfire-art build-frames assets\player_fighter --state idle
uv run dualfire-art pack assets\player_fighter --state idle
uv run dualfire-art validate assets\player_fighter
```

The default profile outputs a `1024x1024` transparent RGBA frame from a `128x128` working image with a maximum 48-color palette. To export a pure green background instead, set `animations.idle.pixel_art.background` to `mode: chroma_key` and `color: [0, 255, 0]` before `build-frames`.
