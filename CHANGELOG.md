# Changelog
All notable changes to this project will be documented in this file.
The format is based on Keep a Changelog, and this project adheres to Semantic Versioning.

## [1.1.4] - 2026-03-27

### Added
- Added sound callback function triggered after playback ends.
- Added transform offset (for texture offset) and animation playback speed control.
- Added per-animation clip playback speed setting.
- Added animation clip callback function support.
- Added example usage for custom loading state.
- User can change SpriteSheet for SpriteAnimator

### Changed
- Changed material, shader, texture, and spritesheet ownership from `unique_ptr` to `shared_ptr`.
- Window size is now set only once during initialization.

### Fixed
- Fixed issue where loading nothing caused CPU usage to spike to 100% and prevented transition to next level.
- Fixed issue where animation callbacks were not properly updated.

## [1.1.3] - 2025-12-22

### Added
- Added custom loading state example.
- Added checker function that checks non-looping animation clip finished.
- User can add callback when resizing window for resizing texture.

### Changed
- Changed Engine name from SNAKE_Engine to JinEngine
- Now update loop is paused while dragging or resizing window

### Fixed
- If AddObject called in Init function, it will also init in the same frame.

## [1.1.2] - 2025-09-17

### Added
- Added asynchronous loader (usable via `LoadingState`).  
- Added asset checker functions in `RenderManager`.  
- Added a post-processing pass to `GameState`. This pass runs after all objects submitted in the `Draw` function have been rendered.  
- Added mouse dragging detection function in `InputManager`.  

### Changed
- Replaced audio backend from **FMOD** to **miniaudio**.  
- Removed redundant functions from the `Object` class.  
- Updated `Collider` to support offset values.  
- Because of limitations of the 2D quad mesh, GPU depth testing was difficult to apply consistently. Now objects are sorted by **layer first**, and within the same layer they are sorted by **depth** before rendering.  
- Replaced legacy OpenGL API calls with modern DSA (Direct State Access) functions.  

### Fixed
- Fixed crash issue when **miniaudio** and **GLFW** were released in conflicting order.  
- Fixed performance issue where large objects caused excessive cell insertions in the spatial hash grid.
- Fixed issue where objects were added to `rawPtrObjects` and `objectMap` too early; now they are only added once they actively participate in the update cycle.  


## [1.1.1] - 2025-08-10

### Added
- Added `unregister` functions in `RenderManager` for assets (Texture, Material, Shader, etc.).  
- If an asset has errors, the engine now falls back to built-in shaders and materials to visually indicate the error.  
- Added a function in `WindowManager` to set the background color.  
- Added the ability for users to explicitly control render layer ordering.  

### Changed
- `Camera` no longer stores an orthographic matrix; it only manages the view matrix. The orthographic projection is now handled by `RenderManager`.  

### Fixed
- Fixed a bug where the font texture atlas did not expand correctly.  
- Fixed an issue where UVs were not properly updated after the font texture atlas expanded.  
- Fixed incorrect world scale and world position calculations for objects that ignore the camera.  

## [1.1.0] - 2025-08-05


## [1.0.0] - 2025-07-17


[1.0.0]: https://github.com/Nero-TheThrill/JinEngine/compare/v_1.0.0...v_1.0.0
[1.1.0]: https://github.com/Nero-TheThrill/JinEngine/compare/v_1.0.0...v_1.1.0
[1.1.1]: https://github.com/Nero-TheThrill/JinEngine/compare/v_1.1.0...v_1.1.1
[1.1.2]: https://github.com/Nero-TheThrill/JinEngine/compare/v_1.1.1...v_1.1.2
[1.1.3]: https://github.com/Nero-TheThrill/JinEngine/compare/v_1.1.2...v_1.1.3
[1.1.4]: https://github.com/Nero-TheThrill/JinEngine/compare/v_1.1.3...v_1.1.4
