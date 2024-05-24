#pragma once

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"

namespace GC
{
    using ProductCategory = class String*;
    
    //_NS_CONST(ProductCategory, Keyboard);

	class Device : public NS::Referencing<Device>
	{
		public:
            NS::String* productCategory();
	};
}

//_GC_PRIVATE_DEF_CONST(GC::ProductCategory, Keyboard);

_NS_INLINE NS::String* GC::Device::productCategory()
{
	return Object::sendMessage<NS::String*>(this, _GC_PRIVATE_SEL(productCategory));
}
