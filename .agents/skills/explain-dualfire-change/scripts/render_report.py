from __future__ import annotations

import argparse
import html
import json
from pathlib import Path
from typing import Any

from report_common import assert_outside_repo, load_json, validate_report


def esc(value: Any) -> str:
    return html.escape(str(value), quote=True)


def paragraphs(items: list[str]) -> str:
    return "".join(f"<p>{esc(item)}</p>" for item in items)


def titled_sections(items: list[dict[str, Any]]) -> str:
    return "".join(
        f'<article class="panel"><h3>{esc(item["title"])}</h3><p>{esc(item["body"])}</p></article>'
        for item in items
    )


def flow_sections(items: list[dict[str, Any]]) -> str:
    blocks: list[str] = []
    for item in items:
        steps = "".join(
            f'<li><span class="step-index">{index}</span><span>{esc(step)}</span></li>'
            for index, step in enumerate(item["steps"], start=1)
        )
        blocks.append(
            f'<article class="panel"><h3>{esc(item["title"])}</h3><ol class="flow">{steps}</ol></article>'
        )
    return "".join(blocks)


def change_sections(items: list[dict[str, Any]]) -> str:
    blocks: list[str] = []
    for item in items:
        details = "".join(f"<li>{esc(detail)}</li>" for detail in item["details"])
        sources = []
        for source in item["sources"]:
            location = source["path"]
            if source.get("line"):
                location += f':{source["line"]}'
            note = f' — {source["note"]}' if source.get("note") else ""
            sources.append(f"<li><code>{esc(location)}</code>{esc(note)}</li>")
        source_html = "".join(sources) or "<li>No direct source recorded.</li>"
        blocks.append(
            "".join(
                [
                    '<article class="change-card">',
                    '<div class="change-meta">',
                    f'<span class="badge confidence-{esc(item["confidence"])}">{esc(item["confidence"])}</span>',
                    f'<span class="badge">{esc(item["domain"])}</span>',
                    f'<span class="adapter">{esc(item["adapter"])}</span>',
                    "</div>",
                    f'<h3>{esc(item["title"])}</h3>',
                    f'<p>{esc(item["summary"])}</p>',
                    f'<ul>{details}</ul>' if details else "",
                    '<details><summary>Sources</summary>',
                    f'<ul class="sources">{source_html}</ul>',
                    "</details></article>",
                ]
            )
        )
    return "".join(blocks)


def risk_sections(items: list[dict[str, Any]]) -> str:
    return "".join(
        "".join(
            [
                f'<article class="risk risk-{esc(item["severity"])}">',
                f'<div><span class="badge">{esc(item["severity"])}</span><h3>{esc(item["title"])}</h3></div>',
                f'<p>{esc(item["detail"])}</p>',
                f'<p><strong>Mitigation:</strong> {esc(item["mitigation"])}</p>',
                "</article>",
            ]
        )
        for item in items
    )


def verification_rows(items: list[dict[str, Any]]) -> str:
    return "".join(
        f'<tr><td>{esc(item["name"])}</td><td><span class="status status-{esc(item["status"])}">{esc(item["status"])}</span></td><td>{esc(item["evidence"])}</td></tr>'
        for item in items
    )


def section(section_id: str, title: str, content: str) -> str:
    if not content:
        return ""
    return f'<section id="{esc(section_id)}"><h2>{esc(title)}</h2>{content}</section>'


def render(data: dict[str, Any], template: str) -> str:
    scope_rows = "".join(
        f'<div><dt>{esc(key)}</dt><dd>{esc(value if not isinstance(value, list) else ", ".join(map(str, value)))}</dd></div>'
        for key, value in data["scope"].items()
        if value not in (None, "", [])
    )
    limitations = "".join(f"<li>{esc(item)}</li>" for item in data["limitations"])
    verification = verification_rows(data["verification"])

    content = "".join(
        [
            section("summary", "Executive summary", paragraphs(data["executive_summary"])),
            section("background", "Background", titled_sections(data["background"])),
            section("intuition", "Intuition", titled_sections(data["intuition"])),
            section("flows", "Runtime and data flow", flow_sections(data["flows"])),
            section("changes", "Literate change walkthrough", change_sections(data["changes"])),
            section("risks", "Risks", risk_sections(data["risks"])),
            section(
                "verification",
                "Verification",
                '<div class="table-wrap"><table><thead><tr><th>Check</th><th>Status</th><th>Evidence</th></tr></thead>'
                f"<tbody>{verification}</tbody></table></div>" if verification else "<p>No verification recorded.</p>",
            ),
            section("limitations", "Limitations", f"<ul>{limitations}</ul>" if limitations else "<p>None recorded.</p>"),
            section("quiz", "Understanding check", '<div id="quiz-root"></div>' if data["quiz"] else "<p>No quiz for this compact report.</p>"),
        ]
    )

    quiz_json = json.dumps(data["quiz"], ensure_ascii=False).replace("<", "\\u003c").replace(">", "\\u003e").replace("&", "\\u0026")
    replacements = {
        "{{TITLE}}": esc(data["title"]),
        "{{PROJECT}}": esc(data["project"]),
        "{{MODE}}": esc(data["mode"]),
        "{{GENERATED_AT}}": esc(data["generated_at"]),
        "{{SCOPE}}": scope_rows,
        "{{CONTENT}}": content,
        "{{QUIZ_JSON}}": quiz_json,
    }
    rendered = template
    for marker, value in replacements.items():
        rendered = rendered.replace(marker, value)
    return rendered


def main() -> int:
    parser = argparse.ArgumentParser(description="Render a self-contained Explain Change report")
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--repo-root", required=True, type=Path)
    args = parser.parse_args()

    assert_outside_repo(args.output, args.repo_root)
    data = load_json(args.input)
    errors = validate_report(data)
    if errors:
        raise ValueError("invalid report:\n- " + "\n- ".join(errors))

    template_path = Path(__file__).resolve().parent.parent / "assets" / "report-template.html"
    template = template_path.read_text(encoding="utf-8")
    output = render(data, template)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(output, encoding="utf-8", newline="\n")
    print(f"Wrote {args.output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
