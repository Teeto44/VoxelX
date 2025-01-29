# VoxelX

A small voxel engine written in C.

## Building

The project uses CMake, which should make it easy to build on any platform. However, right now ImGui, cimgui, and rlImGui are all built into a single `.lib` file, which is exclusive to Windows systems. I do plan to include a Unix-compatible build of the libraries if this project continues. Until then, Unix isn't expressly supported, but if you wish, you are welcome to build the libraries yourself; it should work just fine.

## Dependencies

All dependencies are either included in the project or will be downloaded when it builds.

Here are all of the libraries used:

- [raylib](https://github.com/raysan5/raylib)
- [rlImGui](https://github.com/raylib-extras/rlImGui)
- [cimgui](https://github.com/cimgui/cimgui)
- [imgui](https://github.com/ocornut/imgui)

## License

[MIT](https://choosealicense.com/licenses/mit/)