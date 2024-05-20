#pragma once

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"
#include "GCControllerElement.hpp"
#include <functional>

namespace GC
{
    class ControllerButtonInput;

    using ControllerButtonValueChangedHandlerBlock = void(^)(ControllerButtonInput*, float, bool);
    using ControllerButtonValueChangedHandlerFunction = std::function<void(ControllerButtonInput*, float, bool)>;

    class ControllerButtonInput : public NS::Referencing<ControllerButtonInput, ControllerElement>
    {
    public:
        void setValueChangedHandler(ControllerButtonValueChangedHandlerBlock handler);
        void setValueChangedHandler(const ControllerButtonValueChangedHandlerFunction& handler);
    };
}

_NS_INLINE void GC::ControllerButtonInput::setValueChangedHandler(ControllerButtonValueChangedHandlerBlock handler)
{
	Object::sendMessage<void>(this, _GC_PRIVATE_SEL(setValueChangedHandler_), handler);
}

_NS_INLINE void GC::ControllerButtonInput::setValueChangedHandler(const ControllerButtonValueChangedHandlerFunction& handler)
{    
    __block ControllerButtonValueChangedHandlerFunction blockHandler = handler;

	Object::sendMessage<void>(this, _GC_PRIVATE_SEL(setValueChangedHandler_), ^(ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
    {
        blockHandler(controllerButtonInput, value, isPressed);
    });
}
