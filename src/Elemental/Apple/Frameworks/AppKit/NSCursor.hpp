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
// AppKit/NSEvent.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <Foundation/NSPrivate.hpp>
#include "AppKitPrivate.hpp"

namespace NS 
{
    class Cursor : public Referencing<Cursor> 
    {
    public:
        static void hide();
        static void unhide();
    };
        
    _NS_INLINE void Cursor::hide()
    {
        Object::sendMessage<void>(_APPKIT_PRIVATE_CLS(NSCursor), _APPKIT_PRIVATE_SEL(hide));
    }

    _NS_INLINE void Cursor::unhide()
    {
        Object::sendMessage<void>(_APPKIT_PRIVATE_CLS(NSCursor), _APPKIT_PRIVATE_SEL(unhide));
    }
}
