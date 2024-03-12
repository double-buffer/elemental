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
// AppKit/NSWindow.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "AppKitPrivate.hpp"
#include <Foundation/NSObject.hpp>

#include <CoreGraphics/CGGeometry.h>


namespace NS
{
	class Screen : public Referencing<Screen>
	{
		public:
			CGRect		frame();
			CGRect		visibleFrame();
			CGFloat     backingScaleFactor();
	};

}

_NS_INLINE CGRect NS::Screen::frame()
{
	return Object::sendMessage<CGRect>(this, _APPKIT_PRIVATE_SEL(frame));
}

_NS_INLINE CGRect NS::Screen::visibleFrame()
{
	return Object::sendMessage<CGRect>(this, _APPKIT_PRIVATE_SEL(visibleFrame));
}

_NS_INLINE CGFloat NS::Screen::backingScaleFactor()
{
	return Object::sendMessage<CGFloat>(this, _APPKIT_PRIVATE_SEL(backingScaleFactor));
}

