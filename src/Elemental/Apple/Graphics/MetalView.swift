#if canImport(UIKit)
import UIKit
#else
import Cocoa
#endif

import QuartzCore

#if canImport(UIKit)
class CustomView: UIView {
    var metalDisplayLink: CAMetalDisplayLink!

    init(_ frame: CGRect, frameLatency: Int) {
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
        for touch in touches {
            let location = touch.location(in: self)
            print("Touch began at \(location)")
            // Handle touch began
        }
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            print("Touch moved to \(location)")
            // Handle touch moved
        }
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            print("Touch ended at \(location)")
            // Handle touch ended
        }
    }
    
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            print("Touch cancelled at \(location)")
            // Handle touch cancelled
        }
    }
}
#else
class CustomView: NSView {
    var metalDisplayLink: CAMetalDisplayLink!

    init(_ frame: CGRect, frameLatency: Int) {
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
        let touches = event.touches(matchingPhase: .began, in: self)
        for touch in touches {
  
            // TODO: Location function crash but it seems ok because the normalized position
            // Seems good to compute the deltas

            //let location = touch.location(in: self)
            let location = touch.normalizedPosition
        print("ok")
            print("Touch began at \(location)")
            // Handle touch began
        }
    }
    
    override func touchesMoved(with event: NSEvent) {
        let touches = event.touches(matchingPhase: .moved, in: self)
        for touch in touches {
            //let location = touch.location(in: self)
            let location = touch.normalizedPosition
            print("Touch moved to \(location)")
            // Handle touch moved
        }
    }
    
    override func touchesEnded(with event: NSEvent) {
        let touches = event.touches(matchingPhase: .ended, in: self)
        for touch in touches {
            //let location = touch.location(in: self)
            let location = touch.normalizedPosition
            print("Touch ended at \(location)")
            // Handle touch ended
        }
    }
    
    override func touchesCancelled(with event: NSEvent) {
        let touches = event.touches(matchingPhase: .cancelled, in: self)
        for touch in touches {
            //let location = touch.location(in: self)
            let location = touch.normalizedPosition
            print("Touch cancelled at \(location)")
            // Handle touch cancelled
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

public func createMetalView(frameLatency: Int) -> MetalViewResult {
#if canImport(UIKit)
    let customView = CustomView(UIScreen.main.bounds, frameLatency: frameLatency)
    let metalLayer = customView.layer
#else
    let frame = NSScreen.main!.frame
    let scale = NSScreen.main!.backingScaleFactor
    let customView = CustomView(CGRect(x: 0, y: 0, width: frame.width * scale, height: frame.height * scale), frameLatency: frameLatency)
    let metalLayer = customView.layer!
#endif
    let metalDisplayLink = customView.metalDisplayLink!

    return MetalViewResult(View: Unmanaged.passRetained(customView).toOpaque(), 
                           MetalLayer: Unmanaged.passRetained(metalLayer).toOpaque(), 
                           MetalDisplayLink: Unmanaged.passRetained(metalDisplayLink).toOpaque())
}
