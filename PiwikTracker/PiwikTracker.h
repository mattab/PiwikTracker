//
//  PiwikTracker.h
//  PiwikTracker
//
//  Created by Mattias Levin on 3/12/13.
//  Copyright 2013 Mattias Levin. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "AFNetworking.h"


@class PiwikTransaction;


/**
 
 The PiwikTracker is an Objective-C framework (iOS and OSX) for sending analytics to a Piwik server.
 
 Piwik server is a downloadable, Free/Libre (GPLv3 licensed) real time web analytics software, [http://piwik.org](http://piwik.org).
 This framework implements the Piwik tracking REST API [http://piwik.org/docs/tracking-api/reference.](http://piwik.org/docs/tracking-api/reference/)
 
 ###How does it work
 
 1. Create and configure the tracker
 2. Track screen views, events, errors, social interaction, search and goals
 3. Let the dispatch timer dispatch pending events to the Piwik server or start the dispatch manually


 All events are persisted locally in Core Data until they are dispatched and successfully received by the Piwik server.
 
 All methods are asynchronous and will return immediately.
 */
@interface PiwikTracker : AFHTTPClient


/**
 @name Creating a Piwik tracker
 */

/**
 Create and configure a shared Piwik tracker.
 
 @param baseURL The base URL of the Piwik server. The URL should not include the tracking endpoint path component (/piwik.php)
 @param siteID The unique site id generated by the the Piwik server when the tracked application is created
 @param authenticationToken The unique authentication token generated by the the Piwik server when the tracked application is created
 @return The newly created PiwikTracker
 */
+ (instancetype)sharedInstanceWithBaseURL:(NSURL*)baseURL siteID:(NSString*)siteID authenticationToken:(NSString*)authenticationToken;

/**
 Return the shared Piwik tracker.
 
 The Piwik tracker must have been created and configured for this method to return the tracker.
 
 @return The existing PiwikTracker object
 @see sharedInstanceWithBaseURL:siteID:authenticationToken:
 */
+ (instancetype)sharedInstance;

/**
 Piwik site id generated by the Piwik server.
 
 The value can be found in the Piwik server -> Settings -> Websites.
 */
@property (nonatomic, readonly) NSString *siteID;

/**
 Piwik authentication token generated by the Piwik server. 
 
 The authentication token must be set in order to enabled some of the Piwik features. The value can be found in the Piwik server -> API. 
 
 @warning If you change your user password on Piwik the authentication token value will also change.
 */
@property (nonatomic, readonly) NSString *authenticationToken;

/**
 Unique client id, used to identify unique visitors.
 
 This id is generated the first time the app is installed. The value will be retained across app restart and upgrades. If the application is uninstalled and installed again, a new client id will be generated. 
 
 Requires the authentication token to be set.
 */
@property (nonatomic, readonly) NSString *clientID;


/**
 @name Tracker configuration
 */

/**
 Views, events, exceptions and social tracking will be prefixed based on type. 
 
 This will allow for logical separation and grouping of statistics in the Piwik web interface.
 Screen views will prefixed with "screen".
 Events will be prefixed with "event".
 Exceptions will be prefixed with "exception".
 Social interaction will be prefixed with "social".
 
 Default value is YES which would be the preferred option for most developers. Set to NO to avoid prefixing or implement a custom prefixing schema.
 */
@property (nonatomic) BOOL isPrefixingEnabled;

/**
 Run the tracker in debug mode.
 
 Instead of sending events to the Piwik server, events will be printed to the console. Can be useful during development.
 */
@property(nonatomic) BOOL debug;

/**
 Opt out of tracking.
 
 No events will be sent to the Piwik server. This feature can be used to allow the user to opt out of tracking due to privacy. The value will be retained across app restart and upgrades.
 */
@property(nonatomic) BOOL optOut;

/**
 The probability of an event actually being sampled and sent to the Piwik server. Value 1-100, default 100.
 
 Use the sample rate to only send a sample of all events generated by the app. This can be useful for applications that generate a lot of events.
 */
@property (nonatomic) NSUInteger sampleRate;

/**
 Events sent to the Piwik server will include the users current position when the event was generated. This can be used to improve showing visitors location. Default NO. This value must be set before the tracker is used the first time.
 
 Please note that the position will only be used when showing the location in a users profile. It will not affect the visitor map.
 
 The user will be asked for permission to use current location when the first event is sent. The user can also disable the location service from the Settings location.
 
 Think about users privacy. Provide information why their location is tracked and give the user the option to opt out.
 
 Turning this ON will potentially use more battery power. The tracker will only react to significant location changes to reduce battery impact. Location changes will not be tracked when the app is terminated or running in the background.
 Please note that users can decided to not allow the app to access location information from the general Settings app.
 */
@property (nonatomic) BOOL includeLocationInformation;


/**
 @name Session control
 */

/**
 Set this value to YES to force a new session start when the next event is sent to the Piwik server.
 
 By default a new session is started each time the application in launched.
 */
@property (nonatomic) BOOL sessionStart;

/**
 A new session will be generated if the application spent longer time in the background then the session timeout value. Default value 120 seconds.
 
 The Piwik server will also create a new session if the event is recorded 30 minutes after the previous received event. 
 
 Requires the authentication token to be set.
 */
@property (nonatomic) NSTimeInterval sessionTimeout;


/**
 @name Track screen views, events, goals and more
 */

/**
 Track a single screen view.
 
 Screen views are prefixed with "screen" by default unless prefixing scheme is turned off.
 
 @param screen The name of the screen to track.
 @return YES if the event was queued for dispatching.
 @see isPrefixingEnabled
 */
- (BOOL)sendView:(NSString*)screen;

/**
 Track a single hierarchical screen view.
 
 Piwik support hierarchical screen names, e.g. screen/settings/register. Use this to create a hierarchical and logical grouping of screen views in the Piwik web interface.
 
 Screen views are prefixed with "screen" by default unless prefixing scheme is turned off.
 
 @param screen A list of names of the screen to track.
 @param ... A nil terminated list of names of the screen to track.
 @return YES if the event was queued for dispatching.
 @see isPrefixingEnabled
 */
- (BOOL)sendViews:(NSString*)screen, ...;

/**
 Legacy event tracking.
 @warning This method is deprecated.
 @see sendEventWithCategory:action:name:value:
 */
- (BOOL)sendEventWithCategory:(NSString*)category action:(NSString*)action label:(NSString*)label __deprecated_msg("Use sendEventWithCategory:action:name:value: instead.");


/**
 Track an event (as oppose a screen view).
 
 @warning As of Piwik server 2.3 events are presented in a separate section and support sending a numeric value (float or integer). The Piwik tracker support this out of the box. If your app is connecting to an older Piwik server (<2.3) you can enable the legacy event encoding by defining the macro `PIWIK_LEGACY_EVENT_ENCODING` in your .pch file (`#define PIWIK_LEGACY_EVENT_ENCODING`).
 
 The legacy event encoding will track events as hierarchical screen names, category/action/label. Events are prefixed with "event" by default unless prefixing scheme is turned off.

 @param category The category of the event
 @param action The name of the action, e.g Play, Pause, Download
 @param name Event name, e.g. song name, file name. Optional.
 @param value A numeric value, float or integer. Optional.
 @return YES if the event was queued for dispatching.
 */
- (BOOL)sendEventWithCategory:(NSString*)category action:(NSString*)action name:(NSString*)name value:(NSNumber*)value;

/**
 Track a caught exception or error.
 
 Exception are prefixed with "exception" by default unless prefixing scheme is turned off.
 
 @param description A description of the exception. Maximum 50 characters
 @param isFatal YES if the exception will lead to a fatal application crash
 @return YES if the event was queued for dispatching.
 @see isPrefixingEnabled
 */
- (BOOL)sendExceptionWithDescription:(NSString*)description isFatal:(BOOL)isFatal;

/**
 Track a users interaction with social networks.
 
 Exception are prefixed with "social" by default unless prefixing scheme is turned off.
 
 @param action The action taken by the user, e.g. like, tweet
 @param target The target of the action, e.g. a comment, picture or video (often an unique id or name)
 @param network The social network the user is interacting with, e.g. Facebook
 @return YES if the event was queued for dispatching. 
 @see isPrefixingEnabled
 */
- (BOOL)sendSocialInteraction:(NSString*)action target:(NSString*)target forNetwork:(NSString*)network;

/**
 Track a goal conversion.
 
 @param goalID The unique goal ID as configured in the Piwik server.
 @param revenue The monetary value of the conversion.
 @return YES if the event was queued for dispatching.
 */
- (BOOL)sendGoalWithID:(NSUInteger)goalID revenue:(NSUInteger)revenue;

/**
 Track a search performed in the application. The search could be local or towards a server.
 
 Searches will be presented as Site Search requests in the Piwik web interface.
 
 @param keyword The search keyword entered by the user.
 @param category An optional search category.
 @param numberOfHits The number of results found (optional).
 @return YES if the event was queued for dispatching.
 */
- (BOOL)sendSearchWithKeyword:(NSString*)keyword category:(NSString*)category numberOfHits:(NSNumber*)numberOfHits;


/**
 Track an ecommerce transaction.
 
 A transaction contains transaction information as well as an optional list of items included in the transaction.
 
 Use the transaction builder to create the transaction object.
 
 @param transaction The transaction.
 @return YES if the event was queued for dispatching.
 @see PiwikTransactionBuilder
 @see PiwikTransaction
 @see PiwikTransactionItem
 */
- (BOOL)sendTransaction:(PiwikTransaction*)transaction;


/**
 Track that the app was launched from a Piwik campaign URL.
 The campaign information will be sent to the server with the next Piwik event.
 
 A Piwik campaign URL contains one or two special parameter for tracking campaigns.
 * pk_campaign - The name of the campaign
 * pk_keyword - A specific call to action within a campaign
 Example URL http://example.org/landing.html?pk_campaign=Email-Nov2011&pk_kwd=LearnMore
 
 Use the [Piwik URL builder tool](http://piwik.org/docs/tracking-campaigns-url-builder/) to safely generate Piwik campaign URLs.
 
 If no Piwik campaign parameters are detected in the URL will be ignored and no tracking performed.
 
 @param campaignURLString A custom app URL containing campaign parameters.
 @return YES if URL was detected to contain Piwik campaign parameter.
 */
- (BOOL)sendCampaign:(NSString*)campaignURLString;


/**
 @name Dispatch pending events
 */

/**
 The tracker will automatically dispatch all pending events on a timer. Default value 120 seconds.
 
 If a negative value is set the dispatch timer will never run and manual dispatch must be used. If 0 is set the event is dispatched as as quick as possible after it has been queued.
 
 @see dispatch
 */
@property(nonatomic) NSTimeInterval dispatchInterval;

/**
 Specifies the maximum number of events queued in core date. Default 500.
 
 If the number of queued events exceed this value events will no longer be queued.
 */
@property (nonatomic) NSUInteger maxNumberOfQueuedEvents;

/**
 Specifies how many events should be sent to the Piwik server in each request. Default 20 events per request.
 
 The Piwik server support sending one event at the time or in bulk mode. 
 
 Using bulk mode requires the authentication token to be set.
 
 @warning The bulk request encoding changed in Piwik 2.0. The Piwik tracker support 2.0 encoding out of the box. If you app is connecting to a Piwik 1.X server you can enable 1.x bulk encoding by defining the macro `PIWIK1_X_BULK_ENCODING` in your .pch file (`#define PIWIK1_X_BULK_ENCODING`). The encoding does not affect sending single events. 
 */
@property (nonatomic) NSUInteger eventsPerRequest;

/**
 Manually start a dispatch of all pending events.
 
 @return YES if the dispatch process was started.
 */
- (BOOL)dispatch;

/**
 Delete all pending events.
 */
- (void)deleteQueuedEvents;


/**
 @name Custom visit variables
 */

/**
 The application name.
 
 The application name will be sent as a custom variable (index 2). By default the application name stored in CFBundleDisplayName will be used.
 */
@property (nonatomic, strong) NSString *appName;

/**
 The application version.
 
 The application version will be sent as a custom variable (index 3). By default the application version stored in CFBundleVersion will be used.
 */
@property (nonatomic, strong) NSString *appVersion;

/**
 Set the extra custom visit variable at specific index.
 
 Extra custom variables are permitted to be set as 4th or 5th custom variables.
 
 @param index The index of the custom variable. Currently, only 4th and 5th indexes are permitted, the first three ones are reserved by iOS PiwikTracker.
 @param name The name of the custom variable.
 @param value The value of the custom variable as string.
 @return YES if the extra custom variable is set, NO if the extra custom variable is not permitted to be set at that specific index.
 */
- (BOOL)setExtraCustomVariableAtIndex:(NSUInteger)index name:(NSString *)name value:(NSString *)value;

/**
 Get the extra custom visit variable at specific index.
 
 Extra custom variables are permitted to be set as 4th or 5th custom variables.
 
 @param index The index of the custom variable. Currently, only 4th and 5th indexes are permitted, the first three ones are reserved by iOS PiwikTracker.
 @return The extra custom variable as CustomVariable object which is set at the specific index, nil if no custom variable has been set or index is not permitted (points to reserved custom variable).
 */
- (CustomVariable *)getExtraCustomVariableAtIndex:(NSUInteger)index;

/**
 Remove the extra custom visit variable at specific index.
 
 Extra custom variables are permitted to be set as 4th or 5th custom variables.
 
 @param index The index of the custom variable. Currently, only 4th and 5th indexes are permitted, the first three ones are reserved by iOS PiwikTracker.
 @return The extra custom variable as CustomVariable object which is removed from the specific index, nil if no custom variable has been set or index is not permitted (points to reserved custom variable).
 */
- (CustomVariable *)removeExtraCustomVariableAtIndex:(NSUInteger)index;

@end
