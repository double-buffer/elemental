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
}
#else
import GameController
class CustomView: NSView {
    var metalDisplayLink: CAMetalDisplayLink!

    init(_ frame: CGRect, frameLatency: Int) {
        super.init(frame: frame)

        self.wantsLayer = true
        let metalLayer = CAMetalLayer()

        self.layer = metalLayer
        self.metalDisplayLink = CAMetalDisplayLink(metalLayer: metalLayer)
        self.metalDisplayLink.preferredFrameLatency = Float(frameLatency)
        
        let refreshRate = NSScreen.main!.maximumFramesPerSecond;
        self.metalDisplayLink.preferredFrameRateRange = CAFrameRateRange(minimum: Float(refreshRate), maximum: Float(refreshRate), __preferred: Float(refreshRate))
/*
        NotificationCenter.default.addObserver(forName: Notification.Name.init(rawValue: "GCKeyboardDidConnectNotification"), object: nil, queue: nil) {
            (note) in
            guard let _keyboard = note.object as? GCKeyboard else {
                return
            }

            print("swift keyboard")
            
            // Register callbacks
            _keyboard.keyboardInput?.keyChangedHandler = {
                (keyboardInput, controllerButton, key, isPressed) in
                    print("Key pressed \(isPressed)")
                }
        }*/
    }
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func viewDidMoveToWindow() {
        super.viewDidMoveToWindow()

        // TODO: Handle on MacOS the case where a window is closed or change screen
        // TODO: Handle minimized
        self.metalDisplayLink.add(to: RunLoop.current, forMode: RunLoop.Mode.common)
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
