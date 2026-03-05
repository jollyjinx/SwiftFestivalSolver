// swift-tools-version: 6.2
import PackageDescription

let package = Package(
    name: "Festival",
    platforms: [
        .macOS(.v13),
    ],
    products: [
        .library(
            name: "Festival",
            targets: ["Festival"]
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
                .headerSearchPath("compat"),
                .unsafeFlags(["-std=c++17"]),
            ]
        ),
        .target(
            name: "Festival",
            dependencies: ["CFestival"]
        ),
        .testTarget(
            name: "FestivalTests",
            dependencies: ["Festival"]
        ),
    ]
)
