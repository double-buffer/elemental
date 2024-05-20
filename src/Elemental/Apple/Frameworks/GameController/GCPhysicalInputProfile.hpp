#pragma once

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"
#include "GCControllerElement.hpp"

namespace GC
{
    class PhysicalInputProfile;

    using InputValueDidChangeHandlerBlock = void(^)(PhysicalInputProfile*, ControllerElement*);
    using InputValueDidChangeHandlerFunction = std::function<void(PhysicalInputProfile*, ControllerElement*)>;

	class PhysicalInputProfile : public NS::Referencing<PhysicalInputProfile>
	{
		public:
            GC::Device* device();
            
            void setValueDidChangeHandler(InputValueDidChangeHandlerBlock handler);
            void setValueDidChangeHandler(const InputValueDidChangeHandlerFunction& handler);
	};
}

_NS_INLINE GC::Device* GC::PhysicalInputProfile::device()
{
	return Object::sendMessage<GC::Device*>(this, _GC_PRIVATE_SEL(device));
}

_NS_INLINE void GC::PhysicalInputProfile::setValueDidChangeHandler(InputValueDidChangeHandlerBlock handler)
{
	Object::sendMessage<void>(this, _GC_PRIVATE_SEL(setValueDidChangeHandler_), handler);
}

_NS_INLINE void GC::PhysicalInputProfile::setValueDidChangeHandler(const InputValueDidChangeHandlerFunction& handler)
{    
    __block InputValueDidChangeHandlerFunction blockHandler = handler;

	Object::sendMessage<void>(this, _GC_PRIVATE_SEL(setValueDidChangeHandler_), ^(PhysicalInputProfile* profile, ControllerElement* element)
    {
        blockHandler(profile, element);
    });
}
