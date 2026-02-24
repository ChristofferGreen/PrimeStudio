# PrimeStudio

PrimeStudio is the Studio UI kit for PrimeStage. It provides themed roles, defaults, and composite widgets in the `PrimeStage::Studio` namespace.

## Building

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

Optional configuration:
- `-DPRIMESTUDIO_BUILD_TESTS=ON/OFF` toggles tests.
- `-DPRIMESTUDIO_BUILD_EXAMPLES=ON/OFF` toggles example binaries.

## Tests

From a build dir:

```sh
ctest --output-on-failure
```

Or run the test binary directly:

```sh
./PrimeStudio_tests
```
