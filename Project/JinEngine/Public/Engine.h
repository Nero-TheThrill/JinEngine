/**
 * @file Engine.h
 * @brief Master include header that exposes the main public engine interface.
 *
 * @details
 * This header acts as a single entry point for the JinEngine framework.
 * It aggregates the engine core, state system, object system, rendering system,
 * audio system, camera system, resource system, collision system, animation system,
 * debugging utilities, and commonly used GLM math headers.
 *
 * Including this file allows engine code and gameplay-side code to access most of
 * the engine's public types without including each header individually.
 *
 * In the current engine structure, many implementation files include this header as a
 * central dependency so they can access shared engine types and systems from one place.
 *
 * @note
 * This file is primarily an umbrella header for convenience and interface exposure.
 * It does not define runtime behavior by itself, but it determines which engine modules
 * become visible to translation units that include it.
 *
 * @warning
 * Because this header includes many other headers, including it widely may increase
 * compile times. For large-scale projects, prefer including only the specific headers
 * that are actually required in each file when build time becomes important.
 */
#pragma once

 /**
  * @name Engine Core and High-Level Systems
  * @brief Core engine entry point and primary runtime systems.
  *
  * @details
  * These headers expose the main engine object, the game state abstraction,
  * the shared EngineContext, input handling, window management, state transitions,
  * object lifecycle management, rendering, audio playback, camera management,
  * render layer registration, and frame timing utilities.
  *
  * Together, these types form the backbone of the runtime loop used by JinEngine.
  */
  //@{
#include "JinEngine.h"
#include "GameState.h"
#include "EngineContext.h"
#include "InputManager.h"
#include "WindowManager.h"
#include "StateManager.h"
#include "ObjectManager.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "CameraManager.h"
#include "RenderLayerManager.h"
#include "EngineTimer.h"
//@}

/**
 * @name Object Types
 * @brief Base and derived object types managed by the engine.
 *
 * @details
 * These headers expose the generic Object base type, text-rendered objects,
 * and gameplay-oriented GameObject types.
 *
 * Objects are updated, drawn, and removed through ObjectManager, and may also
 * participate in rendering, collision, animation, and other engine systems.
 */
 //@{
#include "Object.h"
#include "TextObject.h"
#include "GameObject.h"
//@}

/**
 * @name Rendering Resources and Spatial Types
 * @brief Rendering resources, transform data, fonts, cameras, and collision types.
 *
 * @details
 * These headers expose the major rendering resource types such as Texture, Shader,
 * Material, and Mesh, along with Transform data, Font-based text rendering support,
 * the 2D camera class, collider types, and animation-related structures.
 *
 * These components work together inside RenderManager and related systems to produce
 * visible output, perform culling, and support sprite or text rendering workflows.
 */
 //@{
#include "Texture.h"
#include "Shader.h"
#include "Transform.h"
#include "Material.h"
#include "Mesh.h"
#include "Font.h"
#include "Camera2D.h"
#include "Collider.h"
#include "Animation.h"
//@}

/**
 * @name Resource Loading and Transitional States
 * @brief Asynchronous resource loading support and loading-state workflow.
 *
 * @details
 * These headers provide background resource loading support and a dedicated loading
 * state used to queue assets, upload completed resources to the GPU, and transition
 * into the next gameplay state once loading has finished.
 */
 //@{
#include "AsyncResourceLoader.h"
#include "LoadingState.h"
//@}

/**
 * @name Debug Utilities
 * @brief Logging and debug support.
 *
 * @details
 * Exposes engine logging macros and the DebugLogger utility used throughout the
 * engine for diagnostics, warnings, and errors. :contentReference[oaicite:8]{index=8}
 */
#include "Debug.h"

 /**
  * @name External Math Dependencies
  * @brief Common GLM math headers used throughout the engine.
  *
  * @details
  * These headers expose vector and matrix types and common transform helpers used by
  * the engine's rendering, camera, transform, and collision systems.
  */
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"