# Adapter catalog

Select only adapters relevant to the scoped change. Each adapter returns observations, sources, and limitations using the adapter evidence contract.

## Core adapters

| Adapter | Use for | Evidence sources | Important limits |
| --- | --- | --- | --- |
| `git-text` | C++, headers, Config, Build.cs, scripts, metadata | Git name status, numstat, bounded patch, callers and tests | Binary `.uasset` content is opaque |
| `unreal-asset` | Any `.uasset` or `.umap` | Asset class, package metadata, dependencies, referencers, properties | Metadata does not prove runtime behavior |
| `blueprint` | Actor, component and object Blueprints | Components, variables, functions, events, graph/node/pin structure, class defaults | CDO comparison does not reveal event-graph logic |
| `umg` | Widget Blueprints | Widget Tree, slots, named slots, bindings, animations, events and compile state | Widget Animation editing may need MovieScene-level access |
| `mvvm` | UI ViewModels and bindings | ViewModels, field paths, conversion functions and binding mode | A valid binding does not prove focus/input behavior |
| `data` | DataTable, DataAsset and settings | Row/schema/property changes, soft references and validation | Consumers must be inspected separately |
| `world` | Levels and placed Actors | Actor/component hierarchy, transforms, overrides and streaming references | Avoid loading or saving a shared active level merely for explanation |
| `rendering` | Material, Niagara and animation graphs | Graph topology, parameters, assets and runtime bindings | Visual correctness normally requires a preview or runtime capture |
| `runtime` | Behavior verification | Build, Blueprint compile, tests, PIE logs, ensures and screenshots | Never report `passed` when the command was not run |

## DualFire tool routing

### Git and C++/Config

1. Run `scripts/collect_git_evidence.py` for deterministic file facts.
2. Read affected callers, data structures, config consumers and tests.
3. Explain execution order instead of file order.

### General Unreal assets

1. Query asset class, dependencies, and referencers through available AssetTools/Asset Registry tools.
2. Use ObjectTools only after listing exact property names.
3. For a previous revision, reuse `Content/Python/uasset_diff.py` and `UUassetDiffLibrary` for CDO/component properties.
4. Record its limitation: it does not establish Blueprint event-graph changes.

### Blueprint

1. Use the live EditorToolset registry to find Blueprint graph, function, event, node, pin, variable, dispatcher, and component inspection tools.
2. Capture graph structure as read-only evidence.
3. For old/new graph comparison, require structured evidence for both sides; otherwise describe only the current graph and mark the delta unknown.

### Widget Blueprint and MVVM

1. Use live UMGToolSet descriptions for Widget Tree, slots, components, bindings, and compile state.
2. Use Blueprint tools for event/function graphs that are outside Designer Tree coverage.
3. Use MVVMToolset for ViewModels and field bindings.
4. Inspect animations through UWidgetAnimation/MovieScene-capable tools when available; otherwise record the gap.
5. Do not save, compile, or PIE during read-only collection.

### Data and runtime

1. Compare row or property values and then inspect every material consumer affected by the changed field.
2. Keep static inspection and runtime verification in separate report entries.
3. Before compilation or PIE, check whether the user is editing the same Blueprint or Map in the shared Editor.

## Adding an adapter

Add a new row and a short routing section only when a new asset family requires a different evidence API. Preserve this contract:

1. Declare a stable adapter name and domain.
2. Accept an explicit scope.
3. Perform read-only collection by default.
4. Emit normalized observations with source references.
5. Declare blind spots in `limitations`.
6. Never synthesize the final narrative inside the adapter.

The renderer and report schema must remain independent of Unreal asset type.
