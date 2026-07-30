# Report model

Use UTF-8 JSON. The renderer accepts schema version `1.0`.

## Required shape

```json
{
  "schema_version": "1.0",
  "mode": "full",
  "title": "Common UI input routing",
  "project": "DualFire",
  "generated_at": "2026-07-28T12:00:00+09:00",
  "scope": {
    "kind": "commit_range",
    "label": "abc123..def456",
    "base": "abc123",
    "target": "def456",
    "paths": []
  },
  "executive_summary": ["One concise outcome."],
  "background": [
    {"title": "Previous behavior", "body": "What existed before the change."}
  ],
  "intuition": [
    {"title": "Mental model", "body": "Explain the central idea before code."}
  ],
  "flows": [
    {"title": "Runtime flow", "steps": ["Input arrives", "Controller routes it", "Widget handles it"]}
  ],
  "changes": [
    {
      "title": "Route menu input",
      "domain": "ui",
      "adapter": "git-text",
      "confidence": "observed",
      "summary": "The controller now forwards the action to the active layout.",
      "details": ["Explain behavior, not merely the edited lines."],
      "sources": [
        {"path": "Source/DualFire/UI/Example.cpp", "line": 42, "note": "Forwarding call"}
      ]
    }
  ],
  "risks": [
    {
      "severity": "medium",
      "title": "Focus ownership",
      "detail": "Input may reach the wrong layer when focus is stale.",
      "mitigation": "Exercise navigation in PIE."
    }
  ],
  "verification": [
    {"name": "Editor build", "status": "not_run", "evidence": "Not run in the shared Editor session."}
  ],
  "limitations": ["Old Blueprint graph was not available as structured data."],
  "quiz": [
    {
      "question": "Which object owns routing?",
      "options": ["Controller", "Texture", "DataTable"],
      "answer_index": 0,
      "explanation": "The observed call originates in the controller."
    },
    {
      "question": "What remains unverified?",
      "options": ["PIE behavior", "File name", "Commit hash"],
      "answer_index": 0,
      "explanation": "Only static evidence was collected."
    },
    {
      "question": "Why is focus a risk?",
      "options": ["It affects routing", "It changes texture size", "It edits Git history"],
      "answer_index": 0,
      "explanation": "The active focus path determines which UI layer receives input."
    }
  ]
}
```

## Validation rules

- `mode` is `compact` or `full`.
- `scope.kind` and `scope.label` are non-empty.
- `confidence` is `observed`, `inferred`, or `unknown`.
- Every `observed` change has at least one source.
- Verification status is `passed`, `failed`, or `not_run`.
- Risk severity is `low`, `medium`, or `high`.
- A full report contains three to five quiz questions; a compact report contains zero to three.
- Each quiz has three or four options and a valid zero-based `answer_index`.

## Adapter evidence contract

Adapter output is not rendered directly. It supplies normalized facts for constructing the report:

```json
{
  "schema_version": "adapter-evidence/1.0",
  "adapter": "git-text",
  "scope": {"kind": "commit_range", "label": "abc123..def456"},
  "collected_at": "2026-07-28T12:00:00+09:00",
  "observations": [
    {
      "subject": "Source/DualFire/UI/Example.cpp",
      "change_type": "modified",
      "summary": "12 additions, 3 deletions",
      "details": [],
      "sources": [{"path": "Source/DualFire/UI/Example.cpp", "note": "Git diff"}]
    }
  ],
  "limitations": [],
  "raw_patch_excerpt": "optional bounded text"
}
```

Keep raw excerpts bounded. The final report must present a synthesized explanation rather than dumping adapter output.
