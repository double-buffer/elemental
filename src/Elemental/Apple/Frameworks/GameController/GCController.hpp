#pragma once

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"
#include "GCExtendedGamepad.hpp"
#include "GCPhysicalInputProfile.hpp"

namespace GC
{
	class Controller : public NS::Referencing<Controller>
	{
		public:
			static NS::Array* controllers();
            ExtendedGamepad* extendedGamepad();
			PhysicalInputProfile* physicalInputProfile();
	};

}

_NS_INLINE NS::Array* GC::Controller::controllers()
{
	return Object::sendMessage<NS::Array*>(_GC_PRIVATE_CLS(GCController), _GC_PRIVATE_SEL(controllers));
}

_NS_INLINE GC::ExtendedGamepad* GC::Controller::extendedGamepad()
{
	return Object::sendMessage<GC::ExtendedGamepad*>(this, _GC_PRIVATE_SEL(extendedGamepad));
}

_NS_INLINE GC::PhysicalInputProfile* GC::Controller::physicalInputProfile()
{
	return Object::sendMessage<GC::PhysicalInputProfile*>(this, _GC_PRIVATE_SEL(physicalInputProfile));
}
