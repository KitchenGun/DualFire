# Windows Release Build

## Release sequence

1. Update the single release value, `ProjectVersion`, in `Config/DefaultGame.ini` using semantic versioning (`major.minor.patch`).
2. Commit the intended release on `main`, push it, then create and push the matching annotated tag `v<version>`. The packaging script never commits, tags, or pushes.
3. From a clean, synchronized `main`, run the preflight check:

   ```powershell
   .\Scripts\PackageWindows.ps1 -PreflightOnly
   ```

4. Package with UE 5.8:

   ```powershell
   .\Scripts\PackageWindows.ps1
   ```

   Use `-EngineRoot <UE_5.8 root>` only when Unreal is not installed at `D:\UE_5.8`.

5. Verify the generated `Saved\Releases\v<version>\DualFire-v<version>-dev.<UTC>+<shortSHA>-Win64.zip` and its packaged `BUILD_INFO.txt`. The build ID is `<version>-dev.<UTC>+<shortSHA>`; its metadata records ProductVersion, `Channel=dev`, commit, release tag, Unreal Engine 5.8, Development configuration, and build UTC. The archive contains Development Win64 output, cooked `LV_Start`, `LV_Test`, and `LV_Result`, pak/IoStore containers, and UE prerequisites. `-clean` removes previous cook output before this build, and the script then verifies all three `.umap` files under `Saved\Cooked`.
6. Replace only the latest release ZIP in the target Google Drive folder after manual verification. Do not upload, delete, or modify Drive contents from this script.

## Preflight gates

The script requires all of the following before UAT is allowed to run:

- clean working tree;
- current branch is `main`;
- local `main` equals fetched `origin/main`;
- `ProjectVersion` is valid semantic versioning;
- `v<version>` is an existing local annotated tag, its commit is an ancestor of `HEAD`, and the peeled commit from `origin` exactly matches the local tag commit.

On a newly bumped but untagged version, `-PreflightOnly` failing with a missing tag is expected. A lightweight tag, inaccessible remote tag, missing peeled remote commit, or local/remote commit mismatch also blocks the build. Create and push the matching annotated release tag first, then rerun it. A UAT failure or a missing required package artifact removes any ZIP created by this invocation, so an incomplete package is not distributed.
