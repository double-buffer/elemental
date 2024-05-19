#pragma once

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"

namespace GC
{
	class Controller : public NS::Referencing<Controller>
	{
		public:
			static NS::Array* controllers();
	};

}

_NS_INLINE NS::Array* GC::Controller::controllers()
{
	return Object::sendMessage<NS::Array*>(_GC_PRIVATE_CLS(GCController), _GC_PRIVATE_SEL(controllers));
}
