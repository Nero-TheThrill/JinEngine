[![Latest release](https://img.shields.io/github/v/release/Nero-TheThrill/SNAKE_ENGINE)](https://github.com/Nero-TheThrill/SNAKE_ENGINE/releases)
[![Changelog](https://img.shields.io/badge/Changelog-keep%20a%20changelog-blue)](./CHANGELOG.md)


# SNAKE_ENGINE

A lightweight C++ game engine (sample framework) that bundles 2D rendering, input, sound, text, collision, and state management in one place. Built on OpenGL 4.6, it supports batched rendering and instancing, and handles text via runtime glyph baking (FreeType). It provides a dependency/tag–based resource system so you can pull in only the modules your project needs.

<img width="605" height="410" alt="sn" src="https://github.com/user-attachments/assets/e3324563-7210-484a-85d3-f3ff0d2b710e" />

---

## Key Features

* **State transition framework:** Separates the lifecycle into Load/Init/Update/Draw/Free/Unload per `GameState`.
* **Camera & frustum culling:** `Camera2D` coordinate system + automatic exclusion of off–screen objects.
* **Batching & instancing:** Groups identical Mesh/Material pairs; instancing based on the `i_Model` attribute.
* **Runtime text engine:** Bakes glyphs on demand via FreeType; supports alignment and multi-line text (font size range 4–64).
* **Input utilities:** Compare current/previous key/mouse states, convert between screen/world coordinates, read scroll delta.
* **Collision & broad phase:** AABB/Circle colliders + spatial hash grid to minimize pair comparisons.
* **Collision groups/masks:** Tag-based collision filtering for selective checks.
* **Sound:** miniaudio-based play/pause/stop, instance ID management, tag-level controls.
* **Async resource loading:** Background loading with progress tracking via `LoadingState`.
* **Compute shader support:** Dispatch compute shaders for post-processing (e.g., water ripple, glitch).
* **Window utilities:** Fullscreen toggle, resize restriction, cursor visibility control.
* **Debug draw:** Line-drawing API (with camera/projection) to visualize colliders, etc.
* **Built-in logger:** Unified `Log`, `Warn`, `Error` system with configurable log levels.
* **Resource registry:** Tag-based register/lookup/release for Shader/Texture/Mesh/Material/Font/SpriteSheet.

---

## Engine Architecture Overview

```
SNAKE_Engine
 ├─ WindowManager : GLFW/GLAD init, event callbacks, swap/clear
 ├─ InputManager  : Key/mouse state, scroll, world-coordinate conversion
 ├─ SoundManager  : miniaudio system, sound loading/playback/control
 ├─ RenderManager : Resource registry, batching/instancing, debug draw
 │   ├─ RenderLayerManager : Manages 0–16 layer tags
 │   └─ FrustumCuller      : Camera-based visibility filtering
 ├─ StateManager  : Switch/update/draw for GameState
 └─ (GameState)   : Owns ObjectManager + CameraManager
      └─ ObjectManager : Object creation/lifetime/collision/draw submit
          ├─ Object    : Transform2D, Mesh/Material/Animator/Collider
          ├─ TextObject: Font + text mesh generation
          └─ Collider  : Circle/AABB + SpatialHashGrid
```

---

## Requirements

* C++20 or later
* OpenGL 4.6 driver
* Platform libraries: **GLFW**, **GLAD**, **GLM**, **stb_image**, **FreeType**, **miniaudio**

---

## Usage by System

### 1) Rendering / Resources

* **Tag register/lookup:** Use `Register*` / `Get*ByTag`. Re-registering the same tag warns and is ignored.

* **Material uniforms/textures:**

  ```cpp
  Material* m = engineContext.renderManager->GetMaterialByTag("mat.brick");
  m->SetUniform("u_Color", glm::vec4(1,1,1,1));
  m->SetTexture("u_Texture", engineContext.renderManager->GetTextureByTag("brick"));
  ```

* **Instancing:** Your shader must expose the `i_Model` attribute. After enabling, same Mesh/Material pairs are drawn in one call.

  ```cpp
  Material* m = engineContext.renderManager->GetMaterialByTag("mat.instanced");
  Mesh* mesh = engineContext.renderManager->GetMeshByTag("[EngineMesh]default");
  m->EnableInstancing(true, mesh);
  ```

### 2) Text / Fonts

```cpp
engineContext.renderManager->RegisterFont("NotoSans16", "fonts/NotoSans-Regular.ttf", 16);
Font* font = engineContext.renderManager->GetFontByTag("NotoSans16");
auto text = std::make_unique<TextObject>(font, "Hello, World!", TextAlignH::Center, TextAlignV::Middle);
text->SetColor({0.4,0.7,1.0,0.7});
text->SetIgnoreCamera(true, engineContext.stateManager->GetCurrentState()->GetActiveCamera());
objects.AddObject(std::move(text), "title");
```

* Text bakes only the needed glyphs into the atlas at runtime, and supports alignment/multi-line.

### 3) Animation (Sprite Sheets)

```cpp
engineContext.renderManager->RegisterTexture("hero", "Textures/hero.jpg");
engineContext.renderManager->RegisterSpriteSheet("heroSpriteSheet", "hero", 32, 32);
SpriteSheet* sheet = engineContext.renderManager->GetSpriteSheetByTag("heroSpriteSheet");
sheet->AddClip("sidewalk",  { 0,1,2,3,4,5,6,7,8 }, 0.08f, true);
sheet->AddClip("frontwalk", { 86,87,88,89,90,91 }, 0.08f, true);
sheet->AddClip("idle",      { 9 },                 0.08f, false);
AttachAnimator(sheet, 0.08f);
GetSpriteAnimator()->PlayClip("idle");
```

### 4) Input

```cpp
if (engineContext.inputManager->IsKeyPressed(GLFW_KEY_SPACE)) { /* ... */ }
```

### 5) Camera / Layers

```cpp
engineContext.renderManager->RegisterRenderLayer("UI", 2);

Camera2D* cam = cameraManager.GetCamera("main"); // default camera
cam->SetZoom(1.25f);
cam->SetPosition({100,50});

obj->SetRenderLayer("UI");
```

### 6) Collision

```cpp
obj->SetCollider(std::make_unique<AABBCollider>(obj, glm::vec2(1.f, 1.f)));
obj->GetCollider()->SetUseTransformScale(true);
obj->SetCollision(engineContext.stateManager->GetCurrentState()->GetObjectManager(),
                  "button", { "player" });
```

* Supports circle/box colliders, point tests, collision groups/masks, and debug rendering.

### 7) Sound

```cpp
engineContext.soundManager->LoadSound("bgm", "audio/bgm.ogg", /*loop=*/true);
auto id = engineContext.soundManager->Play("bgm", 0.7f);
engineContext.soundManager->ControlByID(SoundManager::SoundControlType::Pause, id);
engineContext.soundManager->ControlByTag(SoundManager::SoundControlType::Resume, "bgm");
```

### 8) Async Loading

```cpp
// In LoadingState
loading->QueueTexture(engineContext, "tex.bg", "Textures/bg.png");
loading->QueueFont(engineContext, "font.ui", "Fonts/NotoSans.ttf", 32);
loading->QueueSound("bgm", "audio/bgm.ogg", true);
```

### 9) Compute Shaders

```cpp
ComputeMaterial* mat = engineContext.renderManager->GetMaterialByTag("waterDrop");
engineContext.renderManager->DispatchCompute(mat); // assumes 8x8 workgroup size
```

### 10) Debug Draw

```cpp
engineContext.renderManager->DrawDebugLine({-50,-50}, {50,50}, GetActiveCamera(), {1,0,0,1}, 2.0f);
// StateManager flushes every frame; shown when the engine allows debug draws
```

---

## Performance Tips

* Maximize identical **Mesh/Material** combinations to reduce draw calls.
* Use **instancing** whenever possible.
* For large maps, **spatial hashing** and **frustum culling** provide significant gains in collision/visibility.
* Unload unused textures/shaders/materials via `Unregister*` to free GPU memory.

---

## Troubleshooting

* Seeing a black/yellow checker texture? Your material doesn’t have a texture bound.
* Garbled fonts? Check the TTF path/pixel size range (4–64) and logs to ensure the atlas expands dynamically.
* State transition stops BGM? Engine automatically stops all sounds when switching states.

---

## Third-Party

* **GLFW** — window/input
* **GLAD** — OpenGL loader
* **OpenGL 4.6** — rendering
* **GLM** — math
* **stb_image** — image loader
* **FreeType** — fonts/glyphs
* **miniaudio** — audio

Please comply with each library’s license.

---

## License

This project is licensed under the **MIT License**.  
You are free to use, modify, and distribute this engine in both personal and commercial projects, as long as you include the original license notice.

---

## Building as a Library

If you want to build **SNAKE_ENGINE** as a separate library and link it to your project:

1. **Build the Engine**  
   - Use Visual Studio to compile the engine as a static library (`.lib`) with the **EngineOnly** configuration.  
   - The build will produce:
     - `SNAKE_Engine.lib`
     - `Headers/` folder (contains engine-facing headers)
     - `Thirdparty/` folder (external dependencies: GLFW, GLM, stb_image, FreeType, miniaudio)


2. **Setup in Your Project (Visual Studio example)**  
   - Go to **Project Properties → C/C++ → General → Additional Include Directories**
     - Add `$(SnakeEngineRoot)/public`
     - Add `$(SnakeEngineRoot)/thirdparty/include`
   - Go to **Project Properties → Linker → General → Additional Library Directories**
     - Add the path to your built `snake_engine.lib`
   - Go to **Project Properties → Linker → Input → Additional Dependencies**
     - Add `snake_engine.lib`

4. **Notes**  
   - When distributing, provide the license notices for all included third-party libraries.

---
