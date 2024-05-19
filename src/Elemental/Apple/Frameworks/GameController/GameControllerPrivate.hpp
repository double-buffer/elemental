#pragma once

#include <objc/runtime.h>

#define _GC_PRIVATE_CLS( symbol )				   ( Private::Class::s_k ## symbol )
#define _GC_PRIVATE_SEL( accessor )				 ( Private::Selector::s_k ## accessor )

#if defined( GC_PRIVATE_IMPLEMENTATION )

#define _GC_PRIVATE_VISIBILITY						__attribute__( ( visibility( "default" ) ) )
#define _GC_PRIVATE_IMPORT						  __attribute__( ( weak_import ) )

#if __OBJC__
#define  _GC_PRIVATE_OBJC_LOOKUP_CLASS( symbol  )   ( ( __bridge void* ) objc_lookUpClass( # symbol ) )
#else
#define  _GC_PRIVATE_OBJC_LOOKUP_CLASS( symbol  )   objc_lookUpClass( # symbol ) 
#endif // __OBJC__

#define _GC_PRIVATE_DEF_CLS( symbol )				void*				   s_k ## symbol 	_GC_PRIVATE_VISIBILITY = _GC_PRIVATE_OBJC_LOOKUP_CLASS( symbol );
#define _GC_PRIVATE_DEF_SEL( accessor, symbol )	 SEL					 s_k ## accessor	_GC_PRIVATE_VISIBILITY = sel_registerName( symbol );
#define _GC_PRIVATE_DEF_CONST( type, symbol )	   _GC_EXTERN type const   GC ## symbol   _GC_PRIVATE_IMPORT; \
													type const			  GC::symbol	 = ( nullptr != &GC ## symbol ) ? GC ## symbol : nullptr;


#else

#define _GC_PRIVATE_DEF_CLS( symbol )				extern void*			s_k ## symbol;
#define _GC_PRIVATE_DEF_SEL( accessor, symbol )	 extern SEL			  s_k ## accessor;
#define _GC_PRIVATE_DEF_CONST( type, symbol )

#endif


namespace GC::Private::Class 
{
    _GC_PRIVATE_DEF_CLS( GCController );
    _GC_PRIVATE_DEF_CLS( GCKeyboard );
}

namespace GC::Private::Selector
{
    _GC_PRIVATE_DEF_SEL( controllers,
                            "controllers" );

    _GC_PRIVATE_DEF_SEL( coalescedKeyboard,
                            "coalescedKeyboard" );

    _GC_PRIVATE_DEF_SEL( keyboardInput,
                            "keyboardInput" );

    _GC_PRIVATE_DEF_SEL( setKeyChangedHandler_,
                            "setKeyChangedHandler:" );
    
    _GC_PRIVATE_DEF_SEL( productCategory,
                                "productCategory" );

    _GC_PRIVATE_DEF_SEL( device,
                                "device" );
}
