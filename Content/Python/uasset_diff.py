"""스크린샷/에디터 UI 없이 uasset 변경사항을 텍스트로 비교하는 유틸리티.

프로젝트 종속 코드 없음 — 이 파일 하나만 아무 Unreal 프로젝트의
`Content/Python/`에 넣으면 그대로 동작한다(Python Editor Script Plugin
필요). Git 리포지토리 여부와 무관하게 diff_asset_against_file()만으로도
쓸 수 있고, git 리포라면 diff_against_git_revision()으로 git show 추출까지
한 번에 처리한다.

방식: git 등으로 확보한 이전 리비전의 .uasset 파일을 임시 패키지로 로드해
현재 버전과 CDO(Class Default Object) 프로퍼티·컴포넌트를 자동/전수
비교한다. UObject 참조는 경로로, 구조체는 to_dict()로 풀어서 실제 값을
비교하므로(메모리 주소·인스턴스 GUID 등에서 오는 거짓 diff 없음) 사람이
스크린샷을 보고 판독할 필요가 없다. 결과는 LogPython(Warning)에 찍히고
반환값으로도 받을 수 있다.

한계: 블루프린트 이벤트그래프(노드) 변경은 CDO 리플렉션만으로는 감지할 수
없다. 로직 변경이 의심되면 에디터 내장 "Diff Against Depot" 툴을
(unreal.AssetToolsHelpers.get_asset_tools().diff_against_depot) 별도로
써야 한다.

사용 예 (에디터 Python 콘솔, 또는 `py <스크립트>`):
    import uasset_diff

    # 이미 다른 방법으로 이전 버전 파일을 확보한 경우
    uasset_diff.diff_asset_against_file(
        "/Game/Blueprint/BP_Test", "C:/scratch/BP_Test_old.uasset")

    # git 리포에서 바로 비교 (리포 루트는 기본적으로 프로젝트 루트로 추정)
    uasset_diff.diff_against_git_revision(
        "/Game/Blueprint/BP_Test", "Content/Blueprint/BP_Test.uasset", revision="HEAD~1")
"""

import os
import shutil
import subprocess
import tempfile
import time

import unreal

_TEMP_PACKAGE_DIR = "/Game/_DiffTemp"


def _get_cdo(asset_path):
    obj = unreal.load_asset(asset_path)
    if obj is None:
        raise RuntimeError(f"에셋 로드 실패: {asset_path}")
    generated_class = getattr(obj, "generated_class", None)
    obj_class = generated_class() if generated_class else obj.get_class()
    return unreal.get_default_object(obj_class)


def _normalize_value(value, old_pkg, new_pkg):
    """비교 가능한 값으로 정규화.

    UObject 참조는 주소 대신 경로로, 구조체는 to_dict()로 풀어 실제 필드값을
    비교한다. old_pkg로 시작하는 경로는 new_pkg로 치환해, 임시 패키지 안의
    자기참조(컴포넌트 등)가 원본과 동일하게 비교되도록 한다.
    """
    if value is None:
        return None
    if hasattr(value, "get_path_name"):
        try:
            path = value.get_path_name()
        except Exception:
            return repr(value)
        if old_pkg and path.startswith(old_pkg):
            path = new_pkg + path[len(old_pkg):]
        return path
    if hasattr(value, "to_dict"):
        try:
            d = value.to_dict()
            return {k: _normalize_value(v, old_pkg, new_pkg) for k, v in d.items()}
        except Exception:
            pass
    if isinstance(value, (list, tuple, set)):
        return [_normalize_value(v, old_pkg, new_pkg) for v in value]
    return value


def _is_delegate(value):
    # 델리게이트 인스턴스의 타입명은 시그니처 이름(예: "OnAnimInitialized")이라
    # 이름만으로는 판별 불가 — MRO에 있는 델리게이트 베이스 클래스로 판별한다.
    mro = {c.__name__ for c in type(value).__mro__}
    return "MulticastDelegateBase" in mro or "DelegateBase" in mro


def _diff_object_properties(old_obj, new_obj, old_pkg, new_pkg, prefix=""):
    diffs = []
    names = sorted(set(dir(old_obj)) & set(dir(new_obj)))
    for name in names:
        if name.startswith("_"):
            continue
        try:
            old_value = old_obj.get_editor_property(name)
            new_value = new_obj.get_editor_property(name)
        except Exception:
            # 프로퍼티가 아니라 메서드거나, 두 클래스 간 존재하지 않는 속성
            continue
        # 델리게이트는 CDO 단계에서 항상 Unbound이고 str()에 주소가 섞여
        # 나오는 값 비교라 의미 있는 diff가 아니다 — 건너뛴다.
        if _is_delegate(old_value) or _is_delegate(new_value):
            continue
        old_norm = _normalize_value(old_value, old_pkg, new_pkg)
        new_norm = _normalize_value(new_value, old_pkg, new_pkg)
        if old_norm != new_norm:
            diffs.append(f"{prefix}{name}: {old_norm!r} -> {new_norm!r}")
    return diffs


def _diff_components(old_cdo, new_cdo, old_pkg, new_pkg):
    lines = []
    old_comps = {c.get_name(): c for c in old_cdo.get_components_by_class(unreal.ActorComponent)}
    new_comps = {c.get_name(): c for c in new_cdo.get_components_by_class(unreal.ActorComponent)}

    for name in sorted(set(new_comps) - set(old_comps)):
        lines.append(f"[+] 컴포넌트 추가: {name} ({new_comps[name].get_class().get_name()})")
    for name in sorted(set(old_comps) - set(new_comps)):
        lines.append(f"[-] 컴포넌트 삭제: {name} ({old_comps[name].get_class().get_name()})")

    for name in sorted(set(old_comps) & set(new_comps)):
        lines.extend(_diff_object_properties(
            old_comps[name], new_comps[name], old_pkg, new_pkg, prefix=f"{name}."))

    return lines


def diff_asset_against_file(current_asset_path, old_uasset_disk_path, log_prefix="ASSETDIFF"):
    """current_asset_path(예: /Game/Blueprint/BP_Test)와 old_uasset_disk_path
    (이전 버전 .uasset 파일의 절대경로 — 출처는 git이든 백업이든 무관)를 비교한다.

    반환값: diff 라인 리스트(빈 리스트면 프로퍼티/컴포넌트 차이 없음).
    같은 내용을 LogPython(Warning)으로도 출력한다(log_prefix로 필터링 가능).
    """
    # 패키지 내부에 저장된 오브젝트 이름(예: "BP_Test")은 파일을 복사해도 그대로
    # 유지된다. load_asset()은 "패키지 경로의 마지막 세그먼트 == 오브젝트 이름"을
    # 기대하므로, 이름은 그대로 두고 상위 폴더만 바꿔 경로 충돌을 피한다.
    # 폴더 자체도 호출마다 고유하게 만든다 — 이전 호출의 정리(delete_asset)가
    # 파일 잠금 등으로 완전히 끝나지 않았어도 다음 호출과 절대 충돌하지 않도록.
    run_id = str(int(time.time() * 1000))
    asset_name = current_asset_path.rsplit("/", 1)[-1]
    temp_asset_path = f"{_TEMP_PACKAGE_DIR}/{run_id}/{asset_name}"
    temp_disk_path = unreal.Paths.project_content_dir() + f"_DiffTemp/{run_id}/{asset_name}.uasset"
    temp_disk_path = unreal.Paths.convert_relative_path_to_full(temp_disk_path)

    os.makedirs(os.path.dirname(temp_disk_path), exist_ok=True)
    shutil.copyfile(old_uasset_disk_path, temp_disk_path)

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous([f"{_TEMP_PACKAGE_DIR}/{run_id}"], True)

    try:
        old_cdo = _get_cdo(temp_asset_path)
        new_cdo = _get_cdo(current_asset_path)

        lines = []
        lines.extend(_diff_object_properties(old_cdo, new_cdo, temp_asset_path, current_asset_path))
        lines.extend(_diff_components(old_cdo, new_cdo, temp_asset_path, current_asset_path))

        if not lines:
            unreal.log_warning(
                f"{log_prefix} {current_asset_path}: 프로퍼티/컴포넌트 변경 없음 "
                "(이벤트그래프 로직 변경은 이 방식으로 감지되지 않음)"
            )
        else:
            for line in lines:
                unreal.log_warning(f"{log_prefix} {line}")
        unreal.log_warning(f"{log_prefix}_COUNT {len(lines)}")
        return lines
    finally:
        # 실패해도(파일 잠금 등) 다음 호출은 run_id가 달라 절대 충돌하지 않는다.
        # 여기서는 최선 노력으로만 정리한다.
        try:
            if unreal.EditorAssetLibrary.does_asset_exist(temp_asset_path):
                unreal.EditorAssetLibrary.delete_asset(temp_asset_path)
            unreal.SystemLibrary.collect_garbage()
        except Exception as cleanup_err:
            unreal.log_warning(f"{log_prefix} cleanup 실패(무시 가능): {cleanup_err}")


def diff_against_git_revision(current_asset_path, repo_relative_path, revision="HEAD", repo_root=None):
    """git 리포에서 바로 비교하는 편의 함수 — git show/임시파일/cleanup을 전부 대신 처리한다.

    current_asset_path: 비교할 현재 에셋의 패키지 경로 (예: "/Game/Blueprint/BP_Test")
    repo_relative_path: 리포 루트 기준 상대 경로 (예: "Content/Blueprint/BP_Test.uasset")
    revision: git show에 넘길 리비전 (기본 HEAD = 마지막 커밋 시점과 비교)
    repo_root: 리포 루트 절대경로. 생략하면 프로젝트 루트(.uproject가 있는 폴더)로 추정 —
               대부분의 Unreal 프로젝트는 프로젝트 루트가 곧 git 리포 루트이므로 기본값으로 충분.

    반환값: diff_asset_against_file()과 동일.
    """
    if repo_root is None:
        repo_root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())

    result = subprocess.run(
        ["git", "show", f"{revision}:{repo_relative_path}"],
        cwd=repo_root, capture_output=True, check=True,
    )

    fd, tmp_path = tempfile.mkstemp(suffix=os.path.splitext(repo_relative_path)[1] or ".uasset")
    try:
        with os.fdopen(fd, "wb") as f:
            f.write(result.stdout)
        return diff_asset_against_file(current_asset_path, tmp_path)
    finally:
        os.remove(tmp_path)
