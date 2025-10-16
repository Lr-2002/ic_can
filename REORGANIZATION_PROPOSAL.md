# Path Reorganization Proposal

## 🎯 Current Problems
1. **Scattered dependencies**: regressor libs in `tools/regressor/lib/`, dm-tools in root, source in `src/`
2. **Confusing paths**: `tools/../src/`, `tools/../include/`, `tools/../dm-tools/`
3. **Mixed levels**: Some tools at root level, some in tools/, source files scattered
4. **Deep nesting**: `tools/regressor/lib/M_mtrx_fcn/` - too deep
5. **Built binaries mixed with source**: Built files in same directory as source

## 🏗️ Proposed Clean Structure

```
ic_can/
├── lib/                          # All external dependencies
│   ├── dm-tools/                 # -> dm-tools/USB2FDCAN/SDK/
│   │   ├── include/
│   │   └── lib/
│   └── matlab/                   # -> tools/regressor/lib/
│       ├── include/
│       └── lib/
├── src/                          # All source code
│   ├── core/                     # -> src/ic_can/core/
│   ├── motors/                   # -> src/ic_can/motors/
│   ├── protocols/                # -> src/ic_can/protocols/
│   ├── safety/                   # -> src/ic_can/safety/
│   └── tools/                    # -> tools/*.cpp
├── include/                      # All headers (already good)
│   └── ic_can/
├── examples/                     # Example code (already good)
├── tests/                        # Tests (already good)
├── python/                       # Python bindings (already good)
├── build/                        # Build output only
├── docs/                         # Documentation
├── cmake/                        # CMake modules (already good)
└── CMakeLists.txt                # Root CMake
```

## 🔄 Migration Steps

### 1. Move External Libraries
```bash
mkdir -p lib
mv dm-tools/USB2FDCAN/SDK lib/dm-tools
mv tools/regressor lib/matlab
```

### 2. Move Tools Source
```bash
mkdir -p src/tools
mv tools/*.cpp src/tools/
```

### 3. Clean Build System
- Update CMakeLists.txt paths:
  - `lib/dm-tools/include` instead of `dm-tools/USB2FDCAN/SDK/C++/ubuntu/include`
  - `lib/matlab/lib` instead of `tools/regressor/lib`
- No more `../` paths needed!

## 📊 Benefits

### Before (Current):
- ❌ `tools/../src/ic_can/core/` (confusing)
- ❌ `tools/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include` (very long)
- ❌ `tools/regressor/lib/M_mtrx_fcn/` (deep nesting)
- ❌ Mixed source/built files

### After (Proposed):
- ✅ `src/core/` (clean)
- ✅ `lib/dm-tools/include` (short)
- ✅ `lib/matlab/lib` (simple)
- ✅ Clear separation of concerns

## 🎯 Updated CMake Paths

### Include Paths:
```cmake
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/matlab/include
)
```

### Library Paths:
```cmake
find_library(DM_TOOLS_LIB
    NAMES u2canfd
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools/lib
)

file(GLOB MATLAB_LIBS
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/matlab/lib/*.o"
)
```

### Source Files:
```cmake
add_subdirectory(src)  # Includes src/tools automatically
```

## 🚀 This Solves:
1. ✅ Clear dependency management
2. ✅ Simple, predictable paths
3. ✅ No more `../` confusion
4. ✅ Proper separation of concerns
5. ✅ Easy to understand structure
6. ✅ Better maintainability