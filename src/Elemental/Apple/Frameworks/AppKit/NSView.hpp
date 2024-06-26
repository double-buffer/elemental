/*
 *
 * Copyright 2020-2021 Apple Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// AppKit/NSView.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "AppKitPrivate.hpp"
#include <Foundation/NSObject.hpp>
#include <CoreGraphics/CGGeometry.h>
#include <QuartzCore/CAMetalLayer.hpp>

namespace NS
{
	class View : public NS::Referencing< View >
	{
		public:
			View*		init( CGRect frame );
			CGRect		frame();
			void 		setWantsLayer(bool wants_layer);
            void 		setLayer(CA::MetalLayer *layer);

            CGPoint     convertPointFromView(CGPoint point, View* view);
	};
}


_NS_INLINE NS::View* NS::View::init( CGRect frame )
{
	return Object::sendMessage< View* >( _APPKIT_PRIVATE_CLS( NSView ), _APPKIT_PRIVATE_SEL( initWithFrame_ ), frame );
}

_NS_INLINE CGRect NS::View::frame()
{
	return Object::sendMessage<CGRect>(this, _APPKIT_PRIVATE_SEL(frame));
}

_NS_INLINE void NS::View::setWantsLayer(bool wants_layer)
{
    Object::sendMessage<void>(this, _APPKIT_PRIVATE_SEL(setWantsLayer_), wants_layer);
}

_NS_INLINE void NS::View::setLayer(CA::MetalLayer *layer)
{
    Object::sendMessage<void>(this, _APPKIT_PRIVATE_SEL(setLayer_), layer);
}
            
_NS_INLINE CGPoint NS::View::convertPointFromView(CGPoint point, NS::View* view)
{
	return Object::sendMessage<CGPoint>(this, _APPKIT_PRIVATE_SEL(convertPoint_fromView_), point, view);
}
