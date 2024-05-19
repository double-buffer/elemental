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
// AppKit/NSApplication.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <Foundation/Foundation.hpp>
#include "GameControllerPrivate.hpp"
#include "GCPhysicalInputProfile.hpp"
#include <functional>

namespace GC
{
	_NS_ENUM(NS::UInteger, KeyCode)
	{
		KeyA = 4,
		KeyB,
		KeyC,
		KeyD,
		KeyE,
		KeyF,
		KeyG,
		KeyH,
		KeyI,
		KeyJ,
		KeyK,
		KeyL,
		KeyM,
		KeyN,
		KeyO,
		KeyP,
		KeyQ,
		KeyR,
		KeyS,
		KeyT,
		KeyU,
		KeyV,
		KeyW,
		KeyX,
		KeyY,
		KeyZ,
		One,
		Two,
		Three,
		Four,
		Five,
		Six,
		Seven,
		Eight,
		Nine,
		Zero,
		ReturnOrEnter,
		Escape,
		DeleteOrBackspace,
		Tab,
		Spacebar,
		Hyphen,
		EqualSign,
		OpenBracket,
		CloseBracket,
		Backslash,
		NonUSPound,
		Semicolon,
		Quote,
		GraveAccentAndTilde,
		Comma,
		Period,
		Slash,
		CapsLock,
		F1,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		F13,
		F14,
		F15,
		F16,
		F17,
		F18,
		F19,
		F20,
		PrintScreen,
		ScrollLock,
		Pause,
		Insert,
		Home,
		PageUp,
		DeleteForward,
		End,
		PageDown,
		RightArrow,
		LeftArrow,
		DownArrow,
		UpArrow,
		KeypadNumLock,
		KeypadSlash,
		KeypadAsterisk,
		KeypadHyphen,
		KeypadPlus,
		KeypadEnter,
		Keypad1,
		Keypad2,
		Keypad3,
		Keypad4,
		Keypad5,
		Keypad6,
		Keypad7,
		Keypad8,
		Keypad9,
		Keypad0,
		KeypadPeriod,
		KeypadEqualSign,
		NonUSBackslash,
		Application,
		Power,
		International1,
		International2,
		International3,
		International4,
		International5,
		International6,
		International7,
		International8,
		International9,
		Lang1,
		Lang2,
		Lang3,
		Lang4,
		Lang5,
		Lang6,
		Lang7,
		Lang8,
		Lang9,
		LeftControl,
		LeftShift,
		LeftAlt,
		LeftGui,
		RightControl,
		RightShift,
		RightAlt,
		RightGui,
	};

    class ControllerElement : public NS::Referencing<ControllerElement>
    {
    };

    class ControllerButtonInput : public NS::Referencing<ControllerButtonInput, ControllerElement>
    {
    };

    class KeyboardInput;

    using KeyChangedHandlerBlock = void(^)(KeyboardInput*, ControllerButtonInput*, long, bool);
    using KeyChangedHandlerFunction = std::function<void(KeyboardInput*, ControllerButtonInput*, KeyCode, bool)>;

	class KeyboardInput : public NS::Referencing<KeyboardInput, PhysicalInputProfile>
	{
		public:
            void setKeyChangedHandler(KeyChangedHandlerBlock handler);
            void setKeyChangedHandler(const KeyChangedHandlerFunction& handler);


            //void keyChangedHandler(GC::KeyChangedHandlerBlock handle);
            //void    keyChangedHandler(const KeyChangedHandlerFunction& handle);
	};

}

_NS_INLINE void GC::KeyboardInput::setKeyChangedHandler(KeyChangedHandlerBlock handler)
{
	Object::sendMessage<void>(this, _GC_PRIVATE_SEL(setKeyChangedHandler_), handler);
}

_NS_INLINE void GC::KeyboardInput::setKeyChangedHandler(const KeyChangedHandlerFunction& handler)
{    
    __block KeyChangedHandlerFunction blockHandler = handler;

	Object::sendMessage<void>(this, _GC_PRIVATE_SEL(setKeyChangedHandler_), ^(KeyboardInput* keyboardInput, GC::ControllerButtonInput* controllerButtonInput, GC::KeyCode keyCode, bool isPressed)
    {
        blockHandler(keyboardInput, controllerButtonInput, keyCode, isPressed);
    });
}
/*
_NS_INLINE void GC::KeyboardInput::keyChangedHandler( const KeyChangedHandlerFunction& handle )
{
    __block KeyChangedHandlerFunction blockFunction = handle;

    keyChangedHandler(^(class GC::KeyboardInput *keyboard, GC::ControllerButtonInput *key, GC::KeyCode keyCode, BOOL pressed) 
    { 
        printf("Test\n");
        blockFunction(keyboard, key, keyCode, pressed); 
    });
}*/
