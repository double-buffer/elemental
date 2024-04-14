//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// Foundation/NSPrivate.hpp
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

namespace NS
{
namespace Private
{
    namespace Class
    {
        _NS_PRIVATE_DEF_CLS(NSRunLoop);
    } // Class
} // Private
} // MTL

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace NS
{
namespace Private
{
    namespace Protocol
    {

    } // Protocol
} // Private
} // NS

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace NS
{
namespace Private
{
    namespace Selector
    {
      	_NS_PRIVATE_DEF_SEL(bytes,
			"bytes");
                _NS_PRIVATE_DEF_SEL(currentRunLoop,
                                    "currentRunLoop");
                _NS_PRIVATE_DEF_SEL(mainRunLoop,
                                    "mainRunLoop");
                _NS_PRIVATE_DEF_SEL(runMode_beforeDate_,
                    "runMode:beforeDate:");
    } // Class
} // Private
} // MTL

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
