# SwiftFestivalSolver

This directory contains a Swift Package port/integration of the **Festival 3.1** Sokoban solver.

- Original solver core: Festival 3.1 C/C++ source by **Yaron Shoham**
- Swift package port and integration: **ported by Patrick Stein** 

This is not a clean-room rewrite of the solving algorithms. The original Festival core is embedded and wrapped with a Swift-facing API.

## What Is Included

- `Sources/CFestival/festival_3.1__src`: original Festival solver source code
- `Sources/CFestival/FestivalBridge.cpp`: C bridge for invoking Festival
- `Sources/Festival/Festival.swift`: Swift API (`FestivalEngine`)
- `Sources/CFestival/compat`: portability headers used for package builds

## Build And Test

```bash
cd Festival
swift build -c release
swift test -c release
```

## Using From Another Swift Package

Add as a local dependency:

```
.package(url: "https://github.com/jollyjinx/SwiftFestivalSolver", from: "0.0.1")
```

Then depend on product `Festival` and call `FestivalEngine.solve(levelText:configuration:)`.

## License

SwiftFestivalSolver is distributed under the MIT License.

See [LICENSE](LICENSE) for the full text and attribution to:

- Yaron Shoham (original Festival solver)
- Patrick Stein (Swift package port/integration work)
