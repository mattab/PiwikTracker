//
//  PiwikTracker.swift
//  PiwikTracker
//
//  Created by Cornelius Horstmann on 07.10.16.
//  Copyright © 2016 Mattias Levin. All rights reserved.
//

import Foundation

public enum CustomVariableScope: Int {
    case Visit
    case Screen
}

let PiwikSessionStartNotification: String = "PiwikSessionStartNotification"


public class PiwikTracker: NSObject {
    
    internal static var _sharedInstance: PiwikTracker?
    public let siteID: String
    public let dispatcher: PiwikDispatcher
    private let eventQueue: EventQueue
    
    // FIXME: handle those
    public var userID: String?
    public var prefixingEnabled: Bool = true
    public var debug: Bool = false
    public var optOut: Bool = false // FIXME: get this from the userdefaults
    public var sampleRate: UInt8 = PiwikConstants.DefaultSampleRate {
        didSet {
            if sampleRate > 100 {
                // FIXME: ad a log warning here?
                sampleRate = 100
            }
        }
    }
    public var includeDefaultCustomVariable: Bool = true
    public var sessionStart: Bool = true // I can see this matches the backend concept, but shouldn't it better be a function? "restartSession" or "finishCurrentSession" or such?
    public var sessionTimeout: TimeInterval = PiwikConstants.DefaultSessionTimeout
    public var dispatchInterval: TimeInterval = PiwikConstants.DefaultDispatchTimer
    public var maxNumberOfQueuedevents: UInt32 = PiwikConstants.DefaultMaxNumberOfStoredEvents
    public var eventsPerRequest: UInt8 = PiwikConstants.DefaultNumberOfEventsPerRequest
    
    
    internal var _clientID: String?
    internal var clientID: String {
        get {
            if let clientID = _clientID { return clientID }
            let userdefaultsKey = "\(siteID)_\(PiwikConstants.UserDefaultVisitorIDKey)"
            if let clientID = UserDefaults.standard.string(forKey: userdefaultsKey) { _clientID = clientID; return clientID }
            
            // none found, generate a key
            let uuid: String = UUID().uuidString.md5
            let clientID = uuid.substring(length: 16)
            UserDefaults.standard.setValue(clientID, forKey: userdefaultsKey)
            UserDefaults.standard.synchronize()
            return clientID
        }
    }
    
    
    init(siteId: String, dispatcher: PiwikDispatcher) {
        self.siteID = siteId
        self.dispatcher = dispatcher
        
        // FIXME: add a persistent EventQueue
        self.eventQueue = EventQueueVolatile()
        
        // start dispatch timer
        // observe UIApplicationDidBecomeActiveNotification and UIApplicationWillResignActiveNotification
    }
    
    func queue(event: [String:String]) -> Bool {
        guard !optOut else { return true }
        guard sampleRate == 100 || sampleRate < UInt8(arc4random_uniform(101)) else { return true }
        
        var parameters = event
        // FIXME: add those special parameters
        //        parameters = [self addPerRequestParameters:parameters];
        //        parameters = [self addSessionParameters:parameters];
        //        parameters = [self addStaticParameters:parameters];
        
        eventQueue.storeEvent(withParameters: parameters) {
            if dispatchInterval == 0 {
                DispatchQueue.main.async(execute: { [unowned self] in
                    let _ = self.dispatch()
                })
            }
        }
        return false
    }
    
    public func dispatch() -> Bool {
        guard !dispatcherRunning else { return true }
        dispatcherRunning = true
        dispatchNextBatch()
        return true
    }
    
    
    private func dispatchNextBatch() {
        eventQueue.events(withLimit: eventsPerRequest) { (entityIds, events, hasMore) in
            if events.count == 0 {
                dispatcherRunning = false
                startDispatchTimer()
            } else {
                let parameter = requestParameters(forEvents: events)
            }
        }
    }
    
    private func requestParameters(forEvents events: [[String:String]]) -> [String:String] {
        // FIXME: implement me proper
        return [:]
    }
    
    
    internal var totalNumberOfVisits: UInt32 = 0
    internal var firstVisit: Date = Date()
    internal var previousVisit: Date?
    internal var currentVisit: Date = Date()
    internal var appDidEnterBackground: Date?
    
    internal var dispatcherRunning = false
    internal var dispatchTimer: Timer?
}

// MARK: dispatcher
internal extension PiwikTracker {
    func startDispatchTimer() {
        DispatchQueue.main.async(execute: { [unowned self] in
            self.stopDispatchTimer()
            if self.dispatchInterval > 0 {
                self.dispatchTimer = Timer.scheduledTimer(timeInterval: self.dispatchInterval, target: self, selector: #selector(self.dispatch), userInfo: nil, repeats: false)
            }
            })
    }
    
    func stopDispatchTimer() {
        if let dispatchTimer = dispatchTimer {
            dispatchTimer.invalidate()
            self.dispatchTimer = nil
        }
    }
}

// MARK: sharedInstance
extension PiwikTracker {
    
    class func defaultDispatcher(withBaseUrl baseUrl: URL) -> PiwikDispatcher {
        // FIXME: return proper dispatcher
        return PiwikDebugDispatcher()
    }
    
    public static var sharedInstance: PiwikTracker? {
        get { return _sharedInstance }
    }
    
    public class func sharedInstance(withSiteId siteId: String, baseURL: URL) -> PiwikTracker? {
        let lastPathComponent = baseURL.lastPathComponent
        // FIXME: add url cleanup (remove last component?)
        let dispatcher = defaultDispatcher(withBaseUrl: baseURL)
        self._sharedInstance = PiwikTracker(siteId: siteId, dispatcher: dispatcher)
        return sharedInstance
    }
    
    public class func sharedInstance(withSiteId siteId: String, dispatcher: PiwikDispatcher) -> PiwikTracker? {
        self._sharedInstance = PiwikTracker(siteId: siteId, dispatcher: dispatcher)
        return sharedInstance
    }
}

// MARK: sending events
extension PiwikTracker {
    public func send(view: String) -> Bool {
        return send(views: [view])
    }
    
    public func send(views: [String]) -> Bool {
        if prefixingEnabled {
            let prefixed = [PiwikConstants.PrefixView] + views
            return send(components: prefixed)
        }
        return send(components: views)
    }
    
    public func send(outlink url: String) -> Bool {
        let event = [
            PiwikConstants.ParameterLink: url,
            PiwikConstants.ParameterURL: url]
        return queue(event: event)
    }
    
    public func send(download url: String) -> Bool {
        let event = [
            PiwikConstants.ParameterDownload: url,
            PiwikConstants.ParameterURL: url]
        return queue(event: event)
    }
    
    public func sendEvent(withCategory category: String, action: String, name: String? = nil, value: String? = nil) -> Bool {
        let event = [
            PiwikConstants.ParameterEventCategory: category,
            PiwikConstants.ParameterEventAction: action,
            PiwikConstants.ParameterEventName: name,
            PiwikConstants.ParameterEventValue: value,
            // FIXME: generate page url
            //PiwikConstants.ParameterURL: [self generatePageURL:nil]
        ]
        // FIXME: remove optionals
        //return queue(event: event)
        return false
    }
    
    public func sendException(withDescription description: String, fatal: Bool) -> Bool {
        let limitedDescription = UInt(description.lengthOfBytes(using: .utf8)) > PiwikConstants.ExceptionDescriptionMaximumLength ? description.substring(length: PiwikConstants.ExceptionDescriptionMaximumLength) : description
        let components: [String?] = [
            prefixingEnabled ? PiwikConstants.PrefixException : nil,
            fatal ? PiwikConstants.PrefixExceptionFatal : PiwikConstants.PrefixExceptionCaught,
            description
        ]
        // FIXME: remove optionals
//        return send(components: components)
        return false
    }
    
    public func sendSocial(action: String, forNetwork network: String, target: String? = nil) -> Bool {
        let components: [String?] = [
            prefixingEnabled ? PiwikConstants.PrefixSocial : nil,
            network,
            action,
            target
        ]
        // FIXME: remove optionals
        //        return send(components: components)
        return false
    }
    
    
    internal func send(components: [String]) -> Bool {
        // FIXME: generatePageURL
        let event = [
            PiwikConstants.ParameterActionName: components.joined(separator: "/"),
            PiwikConstants.ParameterURL: "" // [self generatePageURL:components];
        ]
        return queue(event: event)
    }
    
    public func sendGoal(withId id: UInt, revenue: UInt) -> Bool {
        // FIXME: generatePageURL
        let event: [String : Any] = [
            PiwikConstants.ParameterGoalID: id,
            PiwikConstants.ParameterRevenue: revenue,
            PiwikConstants.ParameterURL: "" // [self generatePageURL:components];
        ]
        // FIXME: event is a String:String ?
//        return queue(event: event)
        return false
    }
    
    public func sendSearch(withKeyword keyword: String, category: String?, hitcount: UInt?) -> Bool {
        // FIXME: generatePageURL
        let event: [String : Any] = [
            PiwikConstants.ParameterSearchKeyword: keyword,
            PiwikConstants.ParameterSearchCategory: category,
            PiwikConstants.ParameterSearchNumberOfHits: (hitcount != nil && hitcount! > 0) ? hitcount : nil,
            PiwikConstants.ParameterURL: "" // [self generatePageURL:components];
        ]
        // FIXME: remove optionals
//        return queue(event: event)
        return false
    }
    
    // FIXME: implement Transactions
//    public func sendTransaction()
    
    public func sendCampaign(_ campaign: Campaign) -> Bool {
        let campaignParameters = [
            PiwikConstants.ParameterCampaignName: campaign.name,
            PiwikConstants.ParameterCampaignKeyword: campaign.keyword,
            PiwikConstants.ParameterReferrer: campaign.url.absoluteString
        ]
        // FIXME: handle these campaignParameters
        // self.campaignParameters = [NSDictionary dictionaryWithDictionary:parameters];
        return false
    }
    
    public func sendContentImpression(withName name: String, piece: String?, target: String?) -> Bool {
        let event = [
            PiwikConstants.ParameterContentName: name,
            PiwikConstants.ParameterContentPiece: piece,
            PiwikConstants.ParameterContentTarget: target
        ]
        // FIXME: remove optionals
        //        return queue(event: event)
        return false
    }
    
    public func sendContentInteraction(withName name: String, piece: String?, target: String?) -> Bool {
        let event = [
            PiwikConstants.ParameterContentName: name,
            PiwikConstants.ParameterContentPiece: piece,
            PiwikConstants.ParameterContentTarget: target
        ]
        // FIXME: remove optionals
        //        return queue(event: event)
        return false
    }
    
    public func setCustomVariable(forIndex index: UInt, name: String, value: String, scope: CustomVariableScope) -> Bool {
        if includeDefaultCustomVariable && scope == .Visit && index <= 3 {
            debugPrint("Custom variable index conflicting with default indexes used by the SDK. Change index or turn off default default variables")
            return false
        }
        
        let customVariable = CustomVariable(index: index, name: name, value: value)
//        switch scope {
//        case .Screen:
//        case .Visit:
//        }
        // FIXME: add the variable to the visit/screen customVariables
        // return true
        return false
    }
    
    public func send(screen: String) -> Bool {
        return false
    }
}
