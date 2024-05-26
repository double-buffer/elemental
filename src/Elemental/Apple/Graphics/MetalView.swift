#if canImport(UIKit)
import UIKit
#else
import Cocoa
#endif

import QuartzCore

#if canImport(UIKit)
class CustomView: UIView {
    var metalDisplayLink: CAMetalDisplayLink!
    var elemWindow: UInt
    var touchHandler: TouchHandlerPointer

    init(_ frame: CGRect, frameLatency: Int, _ elemWindow: UInt, _ touchHandler: TouchHandlerPointer) {
        self.elemWindow = elemWindow
        self.touchHandler = touchHandler
        self.previousLoc = CGPoint(x: 0, y: 0)
        self.deltaX = 0
        self.deltaY = 0
        super.init(frame: frame)
        self.metalDisplayLink = CAMetalDisplayLink(metalLayer: (self.layer as? CAMetalLayer)!)
    
        let refreshRate = UIScreen.main.maximumFramesPerSecond;
        self.metalDisplayLink.preferredFrameRateRange = CAFrameRateRange(minimum: Float(refreshRate), maximum: Float(refreshRate), __preferred: Float(refreshRate))
        self.metalDisplayLink.preferredFrameLatency = Float(frameLatency)
        self.contentScaleFactor = CGFloat(UIScreen.main.nativeScale)
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override class var layerClass: AnyClass {
        return CAMetalLayer.self
    }

    override func didMoveToWindow() {
        super.didMoveToWindow()
      
        self.metalDisplayLink.add(to: RunLoop.current, forMode: RunLoop.Mode.commonModes)
    }

    // TODO: Pass funtion pointers
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        // TODO: We can get the id of the touch with the pointer ot the touch
        for touch in touches {
            sendTouchEvent(0, touch) 
            // TODO: Support pencils for ipad
        }
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            print("Touch moved to \(location)")
            sendTouchEvent(0, touch) 
        }
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            print("Touch ended at \(location)")
            sendTouchEvent(0, touch) 
        }
    }
    
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            print("Touch cancelled at \(location)")
            sendTouchEvent(0, touch) 
        }
    }

    // Those variables are touch id dependends
    var previousLoc: CGPoint
    var deltaX: CGFloat
    var deltaY: CGFloat

    private func sendTouchEvent(_ fingerIndex: UInt, _ touch: UITouch) {
  
            // TODO: Location function crash but it seems ok because the normalized position
            // Seems good to compute the deltas

        // TODO: Test resting
        // TODO: Review magic numbers
        let mul = 500.0

        let location = normalize(location: touch.location(in: self))

        var state = UInt(0)

        if (touch.phase == .began) {
            state = 0
            deltaX = 0
            deltaY = 0
        } else if (touch.phase == .moved || touch.phase == .stationary) {
            deltaX = (location.x - previousLoc.x) * mul
            deltaY = (location.y - previousLoc.y) * mul
            state = 1
        } else if (touch.phase == .ended) {
            state = 2
        }

        previousLoc = location
        touchHandler(elemWindow, fingerIndex, Float(location.x), Float(location.y), Float(deltaX), Float(-deltaY), state)
    }

    private func normalize(location: CGPoint) -> CGPoint {
        let maxDimension = max(self.bounds.width, self.bounds.height)
        let normalizedX = location.x / maxDimension
        let normalizedY = location.y / maxDimension
        return CGPoint(x: normalizedX, y: normalizedY)
    }
}
#else
class CustomView: NSView {
    var metalDisplayLink: CAMetalDisplayLink!
    var elemWindow: UInt
    var touchHandler: TouchHandlerPointer

    init(_ frame: CGRect, frameLatency: Int, _ elemWindow: UInt, _ touchHandler: TouchHandlerPointer) {
        self.elemWindow = elemWindow
        self.touchHandler = touchHandler
        self.previousLoc = NSPoint(x: 0, y: 0)
        self.deltaX = 0
        self.deltaY = 0
        super.init(frame: frame)

        self.allowedTouchTypes = [.direct, .indirect]
        //self.wantsRestingTouches = true

        self.wantsLayer = true
        let metalLayer = CAMetalLayer()

        self.layer = metalLayer
        self.metalDisplayLink = CAMetalDisplayLink(metalLayer: metalLayer)
        self.metalDisplayLink.preferredFrameLatency = Float(frameLatency)
        let refreshRate = NSScreen.main!.maximumFramesPerSecond;
        self.metalDisplayLink.preferredFrameRateRange = CAFrameRateRange(minimum: Float(refreshRate), maximum: Float(refreshRate), __preferred: Float(refreshRate))
    }
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func performKeyEquivalent(with event: NSEvent) -> Bool {
        if event.modifierFlags.contains(.command) {
            return false
        }

        return true
    }

    override func viewDidMoveToWindow() {
        super.viewDidMoveToWindow()

        // TODO: Handle on MacOS the case where a window is closed or change screen
        // TODO: Handle minimized
        self.metalDisplayLink.add(to: RunLoop.current, forMode: RunLoop.Mode.common)
    }

    override func touchesBegan(with event: NSEvent) {
        // TODO: We can get the id of the touch with the identity property
        let touches = event.touches(matchingPhase: .began, in: self)
        for touch in touches {
            sendTouchEvent(0, touch) 
        }
    }
    
    override func touchesMoved(with event: NSEvent) {
        let touches = event.touches(matchingPhase: .moved, in: self)
        for touch in touches {
            sendTouchEvent(0, touch) 
        }
    }
    
    override func touchesEnded(with event: NSEvent) {
        let touches = event.touches(matchingPhase: .ended, in: self)
        for touch in touches {
            sendTouchEvent(0, touch) 
        }
    }
    
    override func touchesCancelled(with event: NSEvent) {
        let touches = event.touches(matchingPhase: .cancelled, in: self)
        for touch in touches {
            sendTouchEvent(0, touch) 
        }
    }

    override var acceptsFirstResponder: Bool {
        return true
    }

    // Those variables are touch id dependends
    var previousLoc: NSPoint
    var deltaX: CGFloat
    var deltaY: CGFloat
    private func sendTouchEvent(_ fingerIndex: UInt, _ touch: NSTouch) {
  
            // TODO: Location function crash but it seems ok because the normalized position
            // Seems good to compute the deltas

        // TODO: Test resting
        let mul = 500.0

        let location = touch.normalizedPosition
        var deltaX = (location.x - previousLoc.x) * mul
        var deltaY = (location.y - previousLoc.y) * mul

        previousLoc = location
        var state = UInt(0)

        if (touch.phase == .began) {
            state = 0
            deltaX = 0
            deltaY = 0
        } else if (touch.phase == .moved || touch.phase == .stationary) {
            deltaX = (location.x - previousLoc.x) * mul
            deltaY = (location.y - previousLoc.y) * mul
            state = 1
        } else if (touch.phase == .ended) {
            state = 2
        }

        touchHandler(elemWindow, fingerIndex, Float(location.x), Float(location.y), Float(deltaX), Float(deltaY), state)
    }
}
#endif

public struct MetalViewResult {
    var View: UnsafeMutableRawPointer
    var MetalLayer: UnsafeMutableRawPointer
    var MetalDisplayLink: UnsafeMutableRawPointer
}

public typealias TouchHandlerPointer = @convention(c) (UInt, UInt, Float, Float, Float, Float, UInt) -> Void

public func createMetalView(frameLatency: Int, window: UInt, touchHandlerPtr: UnsafeMutableRawPointer) -> MetalViewResult {
    let touchHandler = unsafeBitCast(touchHandlerPtr, to: TouchHandlerPointer.self)
#if canImport(UIKit)
    let customView = CustomView(UIScreen.main.bounds, frameLatency: frameLatency, window, touchHandler)
    let metalLayer = customView.layer
#else
    let frame = NSScreen.main!.frame
    let scale = NSScreen.main!.backingScaleFactor
    let customView = CustomView(CGRect(x: 0, y: 0, width: frame.width * scale, height: frame.height * scale), frameLatency: frameLatency, window, touchHandler)
    let metalLayer = customView.layer!
#endif
    let metalDisplayLink = customView.metalDisplayLink!

    return MetalViewResult(View: Unmanaged.passRetained(customView).toOpaque(), 
                           MetalLayer: Unmanaged.passRetained(metalLayer).toOpaque(), 
                           MetalDisplayLink: Unmanaged.passRetained(metalDisplayLink).toOpaque())
}
