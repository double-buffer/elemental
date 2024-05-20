#pragma once

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"
#include "GCPhysicalInputProfile.hpp"
#include "GCControllerButtonInput.hpp"
#include "GCControllerDirectionPad.hpp"

namespace GC
{
	class ExtendedGamepad : public NS::Referencing<ExtendedGamepad, PhysicalInputProfile>
	{
		public:
            ControllerButtonInput* buttonA();
            ControllerButtonInput* buttonB();
            ControllerButtonInput* buttonX();
            ControllerButtonInput* buttonY();
            ControllerDirectionPad* leftThumbstick();
	};
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::buttonA()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(buttonA));
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::buttonB()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(buttonB));
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::buttonX()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(buttonX));
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::buttonY()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(buttonY));
}

_NS_INLINE GC::ControllerDirectionPad* GC::ExtendedGamepad::leftThumbstick()
{
	return Object::sendMessage<GC::ControllerDirectionPad*>(this, _GC_PRIVATE_SEL(leftThumbstick));
}
