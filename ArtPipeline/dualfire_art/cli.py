from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from .asset import create_asset
from .config import load_pipeline_config
from .doctor import doctor_succeeded, run_doctor
from .errors import PipelineError
from .operations import (
    approve_candidate,
    build_frames,
    collect_comfy_result,
    import_keyframe,
    extract_birefnet_mask,
    pack_frames,
    prepare_asset,
    submit_comfy,
)
from .validation import validate_outputs


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = ROOT / "config" / "pipeline.local.yaml"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="dualfire-art")
    subparsers = parser.add_subparsers(dest="command", required=True)

    doctor = subparsers.add_parser("doctor", help="Validate local ComfyUI and model setup")
    doctor.add_argument("--config", type=Path, default=DEFAULT_CONFIG)

    init_asset = subparsers.add_parser("init-asset", help="Create an image asset package")
    init_asset.add_argument("asset_dir", type=Path)
    init_asset.add_argument("--asset-id", required=True)
    init_asset.add_argument("--category", required=True, choices=("player", "enemy", "projectile"))
    init_asset.add_argument("--grade", default="normal")
    init_asset.add_argument(
        "--attribute",
        default="none",
        choices=("none", "air", "ground", "universal", "anti_air", "anti_ground"),
    )
    init_asset.add_argument("--profile", default="legacy", choices=("legacy", "pixel-sprite"))
    init_asset.add_argument("--force", action="store_true")

    prepare = subparsers.add_parser("prepare", help="Normalize and validate source image")
    prepare.add_argument("asset_dir", type=Path)
    prepare.add_argument("--force", action="store_true")

    mask = subparsers.add_parser("extract-mask", help="Extract a BiRefNet source mask through local ComfyUI")
    mask.add_argument("asset_dir", type=Path)
    mask.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    mask.add_argument("--threshold", type=float, default=0.5)
    mask.add_argument("--apply-source-mask", action="store_true")
    mask.add_argument("--force", action="store_true")

    submit = subparsers.add_parser("submit-comfy", help="Generate Qwen image candidates")
    submit.add_argument("asset_dir", type=Path)
    submit.add_argument("--state", required=True)
    submit.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    submit.add_argument("--candidates", type=int)
    submit.add_argument("--prompt-file", type=Path)
    submit.add_argument("--force", action="store_true")

    collect = subparsers.add_parser("collect-comfy", help="Recover a completed ComfyUI candidate by prompt ID")
    collect.add_argument("asset_dir", type=Path)
    collect.add_argument("--state", required=True)
    collect.add_argument("--prompt-id", required=True)
    collect.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    collect.add_argument("--candidate", type=int, default=0)
    collect.add_argument("--force", action="store_true")

    approve = subparsers.add_parser("approve", help="Approve one generated candidate")
    approve.add_argument("asset_dir", type=Path)
    approve.add_argument("--state", required=True)
    approve.add_argument("--candidate", type=int, required=True)
    approve.add_argument("--mask")
    approve.add_argument("--force", action="store_true")

    import_frame = subparsers.add_parser("import-keyframe", help="Register a user-approved external keyframe")
    import_frame.add_argument("asset_dir", type=Path)
    import_frame.add_argument("--state", required=True)
    import_frame.add_argument("--file", type=Path, required=True)
    import_frame.add_argument("--mask")
    import_frame.add_argument("--force", action="store_true")

    frames = subparsers.add_parser("build-frames", help="Build deterministic animation frames")
    frames.add_argument("asset_dir", type=Path)
    frames.add_argument("--state", required=True)
    frames.add_argument("--force", action="store_true")

    pack = subparsers.add_parser("pack", help="Pack frames, metadata, and preview")
    pack.add_argument("asset_dir", type=Path)
    pack.add_argument("--state", required=True)
    pack.add_argument("--force", action="store_true")

    validate = subparsers.add_parser("validate", help="Validate final output")
    validate.add_argument("asset_dir", type=Path)
    validate.add_argument("--state")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        return _dispatch(args)
    except PipelineError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


def _dispatch(args: argparse.Namespace) -> int:
    if args.command == "doctor":
        results = run_doctor(load_pipeline_config(args.config))
        for result in results:
            print(f"[{'OK' if result.ok else 'FAIL'}] {result.name}: {result.detail}")
        return 0 if doctor_succeeded(results) else 1
    if args.command == "init-asset":
        path = create_asset(
            args.asset_dir,
            args.asset_id,
            args.category,
            args.grade,
            args.attribute,
            profile=args.profile.replace("-", "_"),
            force=args.force,
        )
        print(path)
        return 0
    if args.command == "prepare":
        print(json.dumps(prepare_asset(args.asset_dir, force=args.force), indent=2))
        return 0
    if args.command == "extract-mask":
        print(
            extract_birefnet_mask(
                args.asset_dir,
                load_pipeline_config(args.config),
                threshold=args.threshold,
                apply_source_mask=args.apply_source_mask,
                force=args.force,
            )
        )
        return 0
    if args.command == "submit-comfy":
        run = submit_comfy(
            args.asset_dir,
            args.state,
            load_pipeline_config(args.config),
            candidates=args.candidates,
            force=args.force,
            prompt_file=args.prompt_file,
        )
        print(json.dumps(run, ensure_ascii=False, indent=2))
        return 0
    if args.command == "collect-comfy":
        run = collect_comfy_result(
            args.asset_dir,
            args.state,
            args.prompt_id,
            load_pipeline_config(args.config),
            candidate_index=args.candidate,
            force=args.force,
        )
        print(json.dumps(run, ensure_ascii=False, indent=2))
        return 0
    if args.command == "approve":
        print(approve_candidate(args.asset_dir, args.state, args.candidate, args.mask, args.force))
        return 0
    if args.command == "import-keyframe":
        print(import_keyframe(args.asset_dir, args.state, args.file, args.mask, args.force))
        return 0
    if args.command == "build-frames":
        for path in build_frames(args.asset_dir, args.state, force=args.force):
            print(path)
        return 0
    if args.command == "pack":
        for name, path in pack_frames(args.asset_dir, args.state, force=args.force).items():
            print(f"{name}: {path}")
        return 0
    if args.command == "validate":
        result = validate_outputs(args.asset_dir, args.state)
        for warning in result.warnings:
            print(f"WARNING: {warning}")
        for error in result.errors:
            print(f"ERROR: {error}", file=sys.stderr)
        print("Validation passed" if result.ok else "Validation failed")
        return 0 if result.ok else 1
    raise PipelineError(f"Unknown command: {args.command}")


if __name__ == "__main__":
    raise SystemExit(main())
