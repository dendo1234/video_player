# Video Player (Pitchfork Layout)

This is a C++ project using the Pitchfork layout, CMake, SDL3, and FFmpeg.

## Structure
- `src/` — Source files
- `include/` — Public headers
- `extern/` — External dependencies
- `test/` — Tests

## Build Instructions
1. Install SDL3 and FFmpeg (with pkg-config support).
2. Run:
   ```
   cmake -B build -S .
   cmake --build build
   ```
3. Run the executable from `build/`.

## References
- [Pitchfork Spec](https://github.com/vector-of-bool/pitchfork)
- [SDL3](https://github.com/libsdl-org/SDL)
- [FFmpeg](https://ffmpeg.org/)
