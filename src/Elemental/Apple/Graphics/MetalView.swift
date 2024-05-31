#if canImport(UIKit)
import UIKit
typealias Touch = UITouch
typealias View = UIView
#else
import Cocoa
typealias Touch = NSTouch
typealias View = NSView
#endif

import QuartzCore
struct TouchData {
    var previousLoc: CGPoint
    var deltaLocation: CGPoint
    var index: Int
}

public class TouchEventManager {
    let view: View
    let touchHandler: TouchHandlerPointer
    let elemWindow: UInt
    var touchData: [AnyHashable: TouchData] = [:]
    var touchOrder: [UInt64] = []

    init(_ view: View, _ touchHandler: TouchHandlerPointer, _ elemWindow : UInt) {
        self.view = view
        self.elemWindow = elemWindow
        self.touchHandler = touchHandler
    }

    func sendTouchEvent(_ touch: Touch) {
        // TODO: Support pencils for ipad
        let touchHash: AnyHashable
        #if canImport(UIKit)
        touchHash = touch.hash
        #else
         touchHash = touch.identity as! AnyHashable
        #endif
        
        let touchKey = UInt64(bitPattern: Int64(touchHash.hashValue))

        let location = normalizeTouchPosition(touch, true)
        var state = UInt(0)
        var previousLoc = CGPoint(x: 0, y: 0)
        var deltaLocation = CGPoint(x: 0, y: 0)

        if let data = touchData[touchKey] {
            previousLoc = data.previousLoc
            deltaLocation = data.deltaLocation
        }

        var fingerIndex = UInt(0)

        if (touch.phase == .began) {
            state = 0
            deltaLocation = CGPoint(x: 0, y: 0)

            if touchData[touchKey] == nil {
                touchOrder.append(touchKey)
                let index = touchOrder.count - 1
                fingerIndex = UInt(index)
                touchData[touchKey] = TouchData(previousLoc: location, deltaLocation: deltaLocation, index: index)
            }
        } else if (touch.phase == .moved || touch.phase == .stationary) {
            fingerIndex = UInt(touchData[touchKey]?.index ?? 0)
            deltaLocation.x = (location.x - previousLoc.x) * 500
            deltaLocation.y = (location.y - previousLoc.y) * 500
            state = 1
        } else {
            state = 2
            fingerIndex = UInt(touchData[touchKey]?.index ?? 0)

            if (fingerIndex == 0) {
                touchOrder.removeAll()
            }
                
            touchData.removeValue(forKey: touchKey)
        }
        
        // Update the dictionary with the new locations
        if touchData[touchKey] != nil {
            touchData[touchKey]?.previousLoc = location
            touchData[touchKey]?.deltaLocation = deltaLocation
        }

        let absoluteLocation = normalizeTouchPosition(touch, false)

        // TODO: Device Id should represent the screen or trackpad not the touch!
        touchHandler(elemWindow, touchKey, fingerIndex, Float(absoluteLocation.x), Float(absoluteLocation.y), Float(deltaLocation.x), Float(deltaLocation.y), state)
    }

    private func normalizeTouchPosition(_ touch: Touch, _ applyRatio: Bool) -> CGPoint {
        #if canImport(UIKit)
        let location = touch.location(in: self.view)
        let maxDimension = max(self.view.bounds.width, self.view.bounds.height)
        let normalizedX = location.x / (applyRatio ? maxDimension : self.view.bounds.width)
        let normalizedY = location.y / (applyRatio ? maxDimension : self.view.bounds.height)
        return CGPoint(x: normalizedX, y: -normalizedY)
        #else
        return CGPoint(x: touch.normalizedPosition.x, y: touch.normalizedPosition.y)
        #endif
    }
}

#if canImport(UIKit)
class CustomView: UIView {
    var metalDisplayLink: CAMetalDisplayLink!
    var touchEventManager: TouchEventManager!

    init(_ frame: CGRect, frameLatency: Int, _ elemWindow: UInt, _ touchHandler: TouchHandlerPointer) {
        super.init(frame: frame)
        isMultipleTouchEnabled = true
        self.touchEventManager = TouchEventManager(self, touchHandler, elemWindow)
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

    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            touchEventManager.sendTouchEvent(touch) 
        }
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            touchEventManager.sendTouchEvent(touch) 
        }
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            touchEventManager.sendTouchEvent(touch) 
        }
    }
    
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            touchEventManager.sendTouchEvent(touch) 
        }
    }
}
#else
class CustomView: NSView {
    var metalDisplayLink: CAMetalDisplayLink!
    var touchEventManager: TouchEventManager!

    init(_ frame: CGRect, frameLatency: Int, _ elemWindow: UInt, _ touchHandler: TouchHandlerPointer) {
        super.init(frame: frame)
        self.touchEventManager = TouchEventManager(self, touchHandler, elemWindow)

        self.allowedTouchTypes = [.indirect]

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
        let touches = event.touches(matchingPhase: .began, in: self)
        for touch in touches {
            touchEventManager.sendTouchEvent(touch) 
        }
    }
    
    override func touchesMoved(with event: NSEvent) {
        let touches = event.touches(matchingPhase: .moved, in: self)
        for touch in touches {
            touchEventManager.sendTouchEvent(touch) 
        }
    }
    
    override func touchesEnded(with event: NSEvent) {
        let touches = event.touches(matchingPhase: .ended, in: self)
        for touch in touches {
            touchEventManager.sendTouchEvent(touch) 
        }
    }
    
    override func touchesCancelled(with event: NSEvent) {
        let touches = event.touches(matchingPhase: .cancelled, in: self)
        for touch in touches {
            touchEventManager.sendTouchEvent(touch) 
        }
    }

    override var acceptsFirstResponder: Bool {
        return true
    }
}
#endif

public struct MetalViewResult {
    var View: UnsafeMutableRawPointer
    var MetalLayer: UnsafeMutableRawPointer
    var MetalDisplayLink: UnsafeMutableRawPointer
}

public typealias TouchHandlerPointer = @convention(c) (UInt, UInt64, UInt, Float, Float, Float, Float, UInt) -> Void

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
