from __future__ import annotations

import argparse
import re
from pathlib import Path

from report_common import assert_outside_repo, load_json, validate_report


def validate_html(path: Path) -> list[str]:
    errors: list[str] = []
    text = path.read_text(encoding="utf-8")
    lower = text.lower()
    for marker in ("<!doctype html>", "<main", 'id="quiz-data"'):
        if marker not in lower:
            errors.append(f"HTML is missing {marker}")
    if "{{" in text or "}}" in text:
        errors.append("HTML contains an unresolved template marker")
    if re.search(r"<(script|img|link)[^>]+(?:src|href)\s*=\s*[\"']https?://", text, re.I):
        errors.append("HTML contains an external script, image, or stylesheet")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Explain Change JSON and HTML")
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--html", required=True, type=Path)
    parser.add_argument("--repo-root", required=True, type=Path)
    args = parser.parse_args()

    assert_outside_repo(args.html, args.repo_root)
    errors = validate_report(load_json(args.input))
    errors.extend(validate_html(args.html))
    if errors:
        print("Validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print(f"Validation passed: {args.html.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
