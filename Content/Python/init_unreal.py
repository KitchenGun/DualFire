"""DualFire 프로젝트 전용 초기화.

범용 MCP Toolset(level, blueprint, asset)은
ue-mcp-editor-tools repo 의 init_unreal.py 가 별도로 로드한다.
여기에는 DualFire 프로젝트에만 해당하는 초기화를 작성한다.
"""

import unreal

unreal.log("[DualFire] Project-specific init complete.")
