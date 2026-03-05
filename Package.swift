// swift-tools-version: 6.2
import PackageDescription

let package = Package(
    name: "SwiftFestivalSolver",
    platforms: [
        .macOS(.v13),
    ],
    products: [
        .library(
            name: "SwiftFestivalSolver",
            targets: ["SwiftFestivalSolver"]
        ),
    ],
    targets: [
        .target(
            name: "CFestival",
            path: "Sources/CFestival",
            publicHeadersPath: "include",
            cxxSettings: [
                .define("THREADS"),
                .define("LINUX"),
                .define("FESTIVAL_SUPPRESS_STDOUT_LOGS"),
                .headerSearchPath("compat"),
                .unsafeFlags(["-std=c++17"]),
            ]
        ),
        .target(
            name: "SwiftFestivalSolver",
            dependencies: ["CFestival"],
            path: "Sources/SwiftFestivalSolver"
        ),
        .testTarget(
            name: "SwiftFestivalSolverTests",
            dependencies: ["SwiftFestivalSolver"],
            path: "Tests/SwiftFestivalSolverTests"
        ),
    ]
)
