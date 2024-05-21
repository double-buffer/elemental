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

            ControllerButtonInput* buttonMenu();
            ControllerButtonInput* buttonOptions();
            ControllerButtonInput* buttonHome();
    
            ControllerButtonInput* leftShoulder();
            ControllerButtonInput* rightShoulder();
            ControllerButtonInput* leftTrigger();
            ControllerButtonInput* rightTrigger();

            ControllerDirectionPad* leftThumbstick();
            ControllerButtonInput* leftThumbstickButton();

            ControllerDirectionPad* rightThumbstick();
            ControllerButtonInput* rightThumbstickButton();
            
            ControllerDirectionPad* dpad();
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

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::buttonOptions()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(buttonOptions));
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::buttonHome()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(buttonHome));
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::buttonMenu()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(buttonMenu));
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::leftShoulder()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(leftShoulder));
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::rightShoulder()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(rightShoulder));
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::leftTrigger()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(leftTrigger));
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::rightTrigger()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(rightTrigger));
}

_NS_INLINE GC::ControllerDirectionPad* GC::ExtendedGamepad::leftThumbstick()
{
	return Object::sendMessage<GC::ControllerDirectionPad*>(this, _GC_PRIVATE_SEL(leftThumbstick));
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::leftThumbstickButton()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(leftThumbstickButton));
}

_NS_INLINE GC::ControllerDirectionPad* GC::ExtendedGamepad::rightThumbstick()
{
	return Object::sendMessage<GC::ControllerDirectionPad*>(this, _GC_PRIVATE_SEL(rightThumbstick));
}

_NS_INLINE GC::ControllerButtonInput* GC::ExtendedGamepad::rightThumbstickButton()
{
	return Object::sendMessage<GC::ControllerButtonInput*>(this, _GC_PRIVATE_SEL(rightThumbstickButton));
}

_NS_INLINE GC::ControllerDirectionPad* GC::ExtendedGamepad::dpad()
{
	return Object::sendMessage<GC::ControllerDirectionPad*>(this, _GC_PRIVATE_SEL(dpad));
}
