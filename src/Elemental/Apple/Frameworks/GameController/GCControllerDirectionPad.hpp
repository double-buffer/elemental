#pragma once

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"
#include "GCControllerElement.hpp"
#include <functional>

namespace GC
{
    class ControllerDirectionPad;

    using ControllerDirectionPadValueChangedHandlerBlock = void(^)(ControllerDirectionPad*, float, float);
    using ControllerDirectionPadValueChangedHandlerFunction = std::function<void(ControllerDirectionPad*, float, float)>;

    class ControllerDirectionPad : public NS::Referencing<ControllerDirectionPad, ControllerElement>
    {
    public:
        void setValueChangedHandler(ControllerDirectionPadValueChangedHandlerBlock handler);
        void setValueChangedHandler(const ControllerDirectionPadValueChangedHandlerFunction& handler);
    };
}

_NS_INLINE void GC::ControllerDirectionPad::setValueChangedHandler(ControllerDirectionPadValueChangedHandlerBlock handler)
{
	Object::sendMessage<void>(this, _GC_PRIVATE_SEL(setValueChangedHandler_), handler);
}

_NS_INLINE void GC::ControllerDirectionPad::setValueChangedHandler(const ControllerDirectionPadValueChangedHandlerFunction& handler)
{    
    __block ControllerDirectionPadValueChangedHandlerFunction blockHandler = handler;

	Object::sendMessage<void>(this, _GC_PRIVATE_SEL(setValueChangedHandler_), ^(ControllerDirectionPad* directionPad, float xValue, float yValue)
    {
        blockHandler(directionPad, xValue, yValue);
    });
}
