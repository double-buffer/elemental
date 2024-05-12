//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// QuartzCore/CAPrivate.hpp
//
// Copyright 2020-2023 Apple Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <objc/runtime.h>
#include "QuartzCore/CADefines.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#define _CA_PRIVATE_CLS(symbol) (Private::Class::s_k##symbol)
#define _CA_PRIVATE_SEL(accessor) (Private::Selector::s_k##accessor)

namespace CA
{
    namespace Private
    {
        namespace Class
        {
            _CA_PRIVATE_DEF_CLS(CAMetalDisplayLink);
        } // Class
    } // Private
} // CA

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace CA
{
    namespace Private
    {
        namespace Protocol
        {
            
            _CA_PRIVATE_DEF_PRO(CAMetalDisplayLinkDelegate);
            
        } // Protocol
    } // Private
} // CA

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace CA
{
    namespace Private
    {
        namespace Selector
        {
            _CA_PRIVATE_DEF_SEL(initWithMetalLayer,
                                "initWithMetalLayer:");
            _CA_PRIVATE_DEF_SEL(delegate,
                                "delegate");
            _CA_PRIVATE_DEF_SEL(drawable,
                                "drawable");
            _CA_PRIVATE_DEF_SEL(targetPresentationTimestamp, "targetPresentationTimestamp");
            _CA_PRIVATE_DEF_SEL(targetTimestamp, "targetTimestamp");
            _CA_PRIVATE_DEF_SEL(addToRunLoop_,
                                "addToRunLoop:forMode:");
            _CA_PRIVATE_DEF_SEL(removeFromRunLoop_, "removeFromRunLoop:forMode:");
			_CA_PRIVATE_DEF_SEL(contentsScale, "contentsScale");
        } // Class
    } // Private
} // CA

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
