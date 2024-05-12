#pragma once

#include "Elemental.h"

class MetalDisplayLinkHandler : public CA::MetalDisplayLinkDelegate
{
    public:
        MetalDisplayLinkHandler(ElemSwapChain swapChain, ElemSwapChainUpdateHandlerPtr updateHandler, void* updatePayload);
        void metalDisplayLinkNeedsUpdate(CA::MetalDisplayLink* displayLink, CA::MetalDisplayLinkUpdate* update) override;

    private:
        ElemSwapChain _swapChain;
        ElemSwapChainUpdateHandlerPtr _updateHandler;
        void* _updatePayload;
};

// TODO: Review data
struct MetalSwapChainData
{
    NS::SharedPtr<CA::MetalLayer> DeviceObject;
    NS::SharedPtr<CA::MetalDrawable> BackBufferDrawable;
    ElemWindow Window;
    ElemCommandQueue CommandQueue;
    ElemGraphicsDevice GraphicsDevice;
    bool PresentCalled;
    CFTimeInterval CreationTimestamp;
    CFTimeInterval PreviousTargetPresentationTimestamp;
    uint32_t Width;
    uint32_t Height;
    float AspectRatio;
    ElemTextureFormat Format;
};

struct MetalSwapChainDataFull
{
    NS::SharedPtr<CA::MetalDisplayLink> MetalDisplayLink;
    MetalDisplayLinkHandler* MetalDisplayLinkHandler;
};

MetalSwapChainData* GetMetalSwapChainData(ElemSwapChain swapChain);
MetalSwapChainDataFull* GetMetalSwapChainDataFull(ElemSwapChain swapChain);

ElemSwapChain MetalCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options);
void MetalFreeSwapChain(ElemSwapChain swapChain);
ElemSwapChainInfo MetalGetSwapChainInfo(ElemSwapChain swapChain);
void MetalSetSwapChainTiming(ElemSwapChain swapChain, uint32_t frameLatency, uint32_t targetFPS);
void MetalPresentSwapChain(ElemSwapChain swapChain);
