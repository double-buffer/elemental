#pragma once

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"
#include "GCPhysicalInputProfile.hpp"
#include <functional>

namespace GC
{
    class MouseInput;

    using MouseMovedHandlerBlock = void(^)(MouseInput*, float, float);
    using MouseMovedHandlerFunction = std::function<void(MouseInput*, float, float)>;

	class MouseInput : public NS::Referencing<MouseInput, PhysicalInputProfile>
	{
		public:
            ControllerButtonInput* leftButton();
            ControllerButtonInput* middleButton();
            ControllerButtonInput* rightButton();
            NS::Array* auxiliaryButtons();
            ControllerDirectionPad* scroll();

            void setMouseMovedHandler(MouseMovedHandlerBlock handler);
            void setMouseMovedHandler(const MouseMovedHandlerFunction& handler);
	};

}

_NS_INLINE GC::ControllerButtonInput* GC::MouseInput::leftButton()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(leftButton));
}

_NS_INLINE GC::ControllerButtonInput* GC::MouseInput::middleButton()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(middleButton));
}

_NS_INLINE GC::ControllerButtonInput* GC::MouseInput::rightButton()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(rightButton));
}

_NS_INLINE NS::Array* GC::MouseInput::auxiliaryButtons()
{
	return Object::sendMessage<NS::Array*>(this, _GC_PRIVATE_SEL(auxiliaryButtons));
}

_NS_INLINE GC::ControllerDirectionPad* GC::MouseInput::scroll()
{
	return Object::sendMessage<GC::ControllerDirectionPad*>(this, _GC_PRIVATE_SEL(scroll));
}

_NS_INLINE void GC::MouseInput::setMouseMovedHandler(MouseMovedHandlerBlock handler)
{
	Object::sendMessage<void>(this, _GC_PRIVATE_SEL(setMouseMovedHandler_), handler);
}

_NS_INLINE void GC::MouseInput::setMouseMovedHandler(const MouseMovedHandlerFunction& handler)
{    
    __block MouseMovedHandlerFunction blockHandler = handler;

	Object::sendMessage<void>(this, _GC_PRIVATE_SEL(setMouseMovedHandler_), ^(MouseInput* mouse, float deltaX, float deltaY)
    {
        blockHandler(mouse, deltaX, deltaY);
    });
}
