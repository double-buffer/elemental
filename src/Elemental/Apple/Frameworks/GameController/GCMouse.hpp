#pragma once

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"
#include "GCMouseInput.hpp"

namespace GC
{
	class Mouse : public NS::Referencing<Mouse>
	{
		public:
            MouseInput* mouseInput();
	};
}

_NS_INLINE GC::MouseInput* GC::Mouse::mouseInput()
{
	return Object::sendMessage<GC::MouseInput*>(this, _GC_PRIVATE_SEL(mouseInput));
}
