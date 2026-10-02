#import <Foundation/Foundation.h>

/// Delegate protocol for hotkey events
@protocol SPHotkeyMonitorDelegate <NSObject>
/// Fired immediately on trigger-down, before tap/hold classification.
- (void)hotkeyMonitorDidBeginTrigger;
/// Fired when an unconfirmed trigger is discarded (for example, a short tap
/// while configured for hold mode).
- (void)hotkeyMonitorDidCancelTrigger;
- (void)hotkeyMonitorDidDetectHoldStart;
- (void)hotkeyMonitorDidDetectHoldEnd;
- (void)hotkeyMonitorDidDetectTapStart;
- (void)hotkeyMonitorDidDetectTapEnd;
@end

typedef NS_ENUM(uint8_t, SPHotkeyMatchKind) {
    SPHotkeyMatchKindModifierOnly = 0,
    SPHotkeyMatchKindKeyDown = 1,
};

typedef NS_ENUM(uint8_t, SPHotkeyTriggerMode) {
    SPHotkeyTriggerModeHold = 0,
    SPHotkeyTriggerModeToggle = 1,
    SPHotkeyTriggerModeDoubleTap = 2,
};

/// One trigger hotkey. Immutable value object; the monitor accepts several of
/// them and any one can start/stop voice input.
@interface SPHotkeyTrigger : NSObject
@property (nonatomic, assign, readonly) NSInteger keyCode;
/// Alternative key code for the same physical key (e.g. Globe = 179), 0 if none.
@property (nonatomic, assign, readonly) NSInteger altKeyCode;
/// Modifier-only: the key-state flag observed on flagsChanged.
/// KeyDown: the exact modifier mask required alongside keyCode.
@property (nonatomic, assign, readonly) NSUInteger modifierFlag;
@property (nonatomic, assign, readonly) uint8_t matchKind;
@property (nonatomic, assign, readonly, getter=isModifierOnly) BOOL modifierOnly;
- (instancetype)initWithKeyCode:(NSInteger)keyCode
                     altKeyCode:(NSInteger)altKeyCode
                   modifierFlag:(NSUInteger)modifierFlag
                      matchKind:(uint8_t)matchKind;
- (BOOL)matchesKeyCode:(NSInteger)keyCode;
@end

@interface SPHotkeyMonitor : NSObject

/// Threshold in milliseconds to distinguish tap from hold. Default 180ms.
@property (nonatomic, assign) NSTimeInterval holdThresholdMs;

@property (nonatomic, assign) SPHotkeyTriggerMode triggerMode;

/// Primary key code to monitor (default: 63 = Fn/Globe)
@property (nonatomic, assign) NSInteger targetKeyCode;

/// Alternative key code to monitor (default: 179 = Globe on newer keyboards), 0 to disable
@property (nonatomic, assign) NSInteger altKeyCode;

/// Modifier flag to check for key state (default: 0x800000 = NSEventModifierFlagFunction)
@property (nonatomic, assign) NSUInteger targetModifierFlag;

/// How the trigger hotkey should be matched.
@property (nonatomic, assign) uint8_t targetMatchKind;

/// Additional trigger hotkeys that behave exactly like the primary one
/// (targetKeyCode & co.) and share its triggerMode. Only one trigger is
/// active at a time: while one is held, presses of the others are ignored.
@property (nonatomic, copy) NSArray<SPHotkeyTrigger *> *extraTriggers;

/// Maximum interval from the first press to the second press in double-tap
/// mode. Defaults to the user's macOS double-click interval.
@property (nonatomic, assign) NSTimeInterval doubleTapThresholdMs;

- (instancetype)initWithDelegate:(id<SPHotkeyMonitorDelegate>)delegate;
- (void)start;
- (void)stop;

/// Temporarily suppress hotkey detection (e.g. while a menu is open).
@property (nonatomic, assign) BOOL suspended;

/// Reset the state machine to idle. Call when an external event (e.g. audio error)
/// terminates a recording session outside the normal hotkey flow.
- (void)resetToIdle;

/// Arm the state machine as a confirmed hands-free (toggle) recording. Call when
/// an external source (e.g. the status bar menu) starts a session outside the
/// hotkey flow, so the next trigger press ends that session instead of starting
/// a second one. Pair with `resetToIdle` when the same source ends the session.
- (void)markExternalToggleRecording;

/// Whether the number/Enter handler keys can actually be swallowed globally
/// right now. For modifier-only triggers this reflects the Carbon hotkey
/// capture (the tap stays listen-only for its whole life); for non-modifier
/// triggers it reflects the consuming tap.
@property (nonatomic, assign, readonly) BOOL canConsumeHandlerKeyEvents;

/// How many number shortcuts (1..limit) the capture should swallow while
/// numberKeyHandler is set. Set BEFORE assigning numberKeyHandler so digits
/// without a template are never consumed. Default 9.
@property (assign) NSInteger numberKeyCaptureLimit;

/// Optional block called when a number key (1-9) is pressed.
/// Return YES to consume the key event so it does not continue to the target app.
/// Atomic: read from the event-tap thread while set/cleared on the main thread.
@property (copy) BOOL (^numberKeyHandler)(NSInteger number);

/// Optional block called when any non-template key is pressed (any key except 1-9).
/// The key event is NOT consumed — it always passes through to the target app.
/// Used to dismiss the overlay when the user resumes typing after text insertion.
/// Atomic: read from the event-tap thread while set/cleared on the main thread.
@property (copy) void (^anyKeyDismissHandler)(void);

/// Optional block called when Return/Enter is pressed.
/// Return YES to consume the key event. It is only honoured when the active
/// CGEventTap can suppress the event globally.
/// Atomic: read from the event-tap thread while set/cleared on the main thread.
@property (copy) BOOL (^enterKeyHandler)(void);

@end
