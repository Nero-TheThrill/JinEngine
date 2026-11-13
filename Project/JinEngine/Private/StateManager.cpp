#include "Engine.h"

GameState* StateManager::GetCurrentState() const
{
	return currentState ? currentState.get() : nullptr;
}

void StateManager::ChangeState(std::unique_ptr<GameState> newState)
{
	nextState = std::move(newState);
}

void StateManager::Update(float dt, const EngineContext& engineContext)
{
	if (nextState != nullptr)
	{
		if (currentState != nullptr)
		{
			currentState->SystemFree(engineContext);
			currentState->SystemUnload(engineContext);
			engineContext.soundManager->ControlAll(SoundManager::SoundControlType::Stop);
		}
		currentState = std::move(nextState);
		currentState->SystemLoad(engineContext);
		currentState->SystemInit(engineContext);
		currentState->GetCameraManager().SetScreenSizeForAll(engineContext.windowManager->GetWidth(), engineContext.windowManager->GetHeight());
	}
	if (currentState != nullptr)
	{
		currentState->SystemUpdate(dt, engineContext);
	}
}

void StateManager::Draw(const EngineContext& engineContext)
{
    if (currentState != nullptr)
    {
	engineContext.renderManager->BeginFrame(engineContext);

	currentState->Draw(engineContext);
	engineContext.renderManager->FlushDrawCommands(engineContext);
	if (engineContext.engine->ShouldRenderDebugDraws())
	{
	    currentState->DrawColliderDebug(engineContext);
	    engineContext.renderManager->FlushDebugLineDrawCommands(engineContext);
	}

	engineContext.renderManager->EndFrame(engineContext);

	currentState->PostProcessing(engineContext);
	engineContext.renderManager->FlushDrawCommands(engineContext);
    }
}

void StateManager::Free(const EngineContext& engineContext)
{
	if (currentState != nullptr)
	{
		currentState->SystemFree(engineContext);
		currentState->SystemUnload(engineContext);
		currentState.reset();
	}
}
