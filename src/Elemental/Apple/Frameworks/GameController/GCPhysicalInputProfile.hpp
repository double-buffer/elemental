#pragma once

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"

namespace GC
{
	class PhysicalInputProfile : public NS::Referencing<PhysicalInputProfile>
	{
		public:
            GC::Device* device();
	};
}

_NS_INLINE GC::Device* GC::PhysicalInputProfile::device()
{
	return Object::sendMessage<GC::Device*>(this, _GC_PRIVATE_SEL(device));
}
