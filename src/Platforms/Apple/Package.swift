// swift-tools-version:5.7
import PackageDescription

let package = Package(
  name: "elemental-native",
  platforms: [
    .macOS(.v13)
  ],
  products: [
    .library(
      name: "Elemental.Native",
      type: .dynamic,
      targets: ["Elemental.Native"])
  ],
  dependencies: [
    .package(
      url: "https://github.com/apple/swift-atomics.git", 
      .upToNextMajor(from: "1.0.0")
    )
  ],
  targets: [
    .systemLibrary(name: "NativeElemental"),
    .target(
      name: "Elemental.Native",
      dependencies: [
        "NativeElemental",
        .product(name: "Atomics", package: "swift-atomics")
      ]
    )
  ]
)