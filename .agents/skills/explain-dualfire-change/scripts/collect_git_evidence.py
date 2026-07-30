from __future__ import annotations

import argparse
import json
import subprocess
from datetime import datetime
from pathlib import Path
from typing import Any

from report_common import assert_outside_repo


CHANGE_NAMES = {
    "A": "added",
    "C": "copied",
    "D": "deleted",
    "M": "modified",
    "R": "renamed",
    "T": "type_changed",
    "U": "unmerged",
    "X": "unknown",
}


def run_git(repo: Path, arguments: list[str]) -> str:
    result = subprocess.run(
        ["git", *arguments],
        cwd=repo,
        check=True,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return result.stdout


def scope_arguments(args: argparse.Namespace) -> tuple[dict[str, Any], list[str]]:
    if args.commit_range:
        if ".." not in args.commit_range:
            raise ValueError("--range must use <base>..<target>")
        base, target = args.commit_range.split("..", 1)
        if not base or not target:
            raise ValueError("--range must include both base and target")
        return (
            {
                "kind": "commit_range",
                "label": args.commit_range,
                "base": base,
                "target": target,
            },
            [args.commit_range],
        )
    if args.staged:
        return ({"kind": "staged", "label": "staged changes"}, ["--cached"])
    return ({"kind": "worktree", "label": "unstaged worktree changes"}, [])


def parse_name_status(text: str) -> list[dict[str, str]]:
    changes: list[dict[str, str]] = []
    for line in text.splitlines():
        if not line.strip():
            continue
        fields = line.split("\t")
        status = fields[0]
        code = status[:1]
        if code in {"R", "C"} and len(fields) >= 3:
            changes.append(
                {
                    "path": fields[2],
                    "previous_path": fields[1],
                    "change_type": CHANGE_NAMES.get(code, "unknown"),
                }
            )
        elif len(fields) >= 2:
            changes.append(
                {
                    "path": fields[1],
                    "change_type": CHANGE_NAMES.get(code, "unknown"),
                }
            )
    return changes


def parse_numstat(text: str) -> dict[str, str]:
    stats: dict[str, str] = {}
    for line in text.splitlines():
        fields = line.split("\t")
        if len(fields) < 3:
            continue
        additions, deletions = fields[0], fields[1]
        path = fields[-1]
        if additions == "-" or deletions == "-":
            stats[path] = "binary change"
        else:
            stats[path] = f"{additions} additions, {deletions} deletions"
    return stats


def main() -> int:
    parser = argparse.ArgumentParser(description="Collect bounded Git evidence for Explain Change")
    parser.add_argument("--repo", required=True, type=Path)
    scope = parser.add_mutually_exclusive_group(required=True)
    scope.add_argument("--range", dest="commit_range")
    scope.add_argument("--staged", action="store_true")
    scope.add_argument("--worktree", action="store_true")
    parser.add_argument("--path", dest="paths", action="append", default=[])
    parser.add_argument("--max-patch-chars", type=int, default=50000)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    repo = args.repo.resolve()
    top_level = Path(run_git(repo, ["rev-parse", "--show-toplevel"]).strip()).resolve()
    if top_level != repo:
        raise ValueError(f"--repo must be the Git root: {top_level}")
    assert_outside_repo(args.output, repo)

    scope_model, diff_scope = scope_arguments(args)
    path_args = ["--", *args.paths] if args.paths else []
    name_status = run_git(repo, ["diff", *diff_scope, "--name-status", *path_args])
    numstat = run_git(repo, ["diff", *diff_scope, "--numstat", *path_args])
    patch = run_git(
        repo,
        ["diff", *diff_scope, "--no-ext-diff", "--no-color", "--unified=3", *path_args],
    )

    changes = parse_name_status(name_status)
    stats = parse_numstat(numstat)
    observations: list[dict[str, Any]] = []
    for change in changes:
        path = change["path"]
        observations.append(
            {
                "subject": path,
                "change_type": change["change_type"],
                "summary": stats.get(path, "change detected"),
                "details": (
                    [f"previous path: {change['previous_path']}"]
                    if "previous_path" in change
                    else []
                ),
                "sources": [{"path": path, "note": "Git diff"}],
            }
        )

    if args.worktree:
        untracked = run_git(repo, ["ls-files", "--others", "--exclude-standard", *path_args])
        for path in (line for line in untracked.splitlines() if line.strip()):
            observations.append(
                {
                    "subject": path,
                    "change_type": "untracked",
                    "summary": "untracked file; content not included in Git diff",
                    "details": [],
                    "sources": [{"path": path, "note": "git ls-files --others"}],
                }
            )

    truncated = len(patch) > args.max_patch_chars
    model = {
        "schema_version": "adapter-evidence/1.0",
        "adapter": "git-text",
        "scope": scope_model,
        "collected_at": datetime.now().astimezone().isoformat(timespec="seconds"),
        "observations": observations,
        "limitations": (["raw patch excerpt was truncated"] if truncated else []),
        "raw_patch_excerpt": patch[: args.max_patch_chars],
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="\n") as handle:
        json.dump(model, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    print(f"Wrote {args.output.resolve()} ({len(observations)} observations)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
