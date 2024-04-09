#pragma once

#include "Elemental.h"

class MetalViewDelegate : public MTK::ViewDelegate
{
    public:
        MetalViewDelegate(ElemSwapChainUpdateHandlerPtr updateHandler, void* updatePayload);
        virtual void drawInMTKView(MTK::View* pView) override;

    private:
        ElemSwapChainUpdateHandlerPtr _updateHandler;
        void* _updatePayload;
};

// TODO: Review members
struct MetalSwapChainData
{
    NS::SharedPtr<MTK::View> DeviceObject;
    NS::SharedPtr<CA::MetalDrawable> BackBufferDrawable;
    ElemTexture BackBufferTexture;
    dispatch_semaphore_t WaitSemaphore;
    ElemCommandQueue CommandQueue;
    ElemGraphicsDevice GraphicsDevice;
};

struct MetalSwapChainDataFull
{
    MetalViewDelegate* MetalViewDelegate;
};

MetalSwapChainData* GetMetalSwapChainData(ElemSwapChain swapChain);
MetalSwapChainDataFull* GetMetalSwapChainDataFull(ElemSwapChain swapChain);

ElemSwapChain MetalCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, const ElemSwapChainOptions* options);
ElemSwapChain MetalCreateSwapChain2(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options);
void MetalFreeSwapChain(ElemSwapChain swapChain);
ElemSwapChainInfo MetalGetSwapChainInfo(ElemSwapChain swapChain);
void MetalResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height);
void MetalPresentSwapChain(ElemSwapChain swapChain);
void MetalWaitForSwapChainOnCpu(ElemSwapChain swapChain);
