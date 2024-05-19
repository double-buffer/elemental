#pragma once

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"
#include "GCKeyboardInput.hpp"

namespace GC
{
	class Keyboard : public NS::Referencing<Keyboard>
	{
		public:
			static Keyboard* coalescedKeyboard();

            KeyboardInput* keyboardInput();
	};
}

_NS_INLINE GC::Keyboard* GC::Keyboard::coalescedKeyboard()
{
	return Object::sendMessage<GC::Keyboard*>(_GC_PRIVATE_CLS(GCKeyboard), _GC_PRIVATE_SEL(coalescedKeyboard));
}

_NS_INLINE GC::KeyboardInput* GC::Keyboard::keyboardInput()
{
	return Object::sendMessage<GC::KeyboardInput*>(this, _GC_PRIVATE_SEL(keyboardInput));
}
