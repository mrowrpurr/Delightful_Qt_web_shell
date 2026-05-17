// app_shell::UrlProtocol — macOS implementation.
//
// macOS URL protocols are a two-part contract:
//   1. The bundle's Info.plist declares which schemes it CAN handle
//      via CFBundleURLTypes. Without this entry, the bundle is
//      invisible to LaunchServices regardless of any runtime calls
//      below — the OS will never route a <slug>:// URL to this app.
//      That plist injection happens in app/desktop/xmake.lua's
//      after_build hook on macOS, keyed off APP_SLUG.
//
//   2. LaunchServices tracks which bundle is the DEFAULT handler for
//      each scheme. registerProtocol() makes this bundle the default;
//      isRegistered() checks whether it currently is.
//
// "Unregister" doesn't have a clean equivalent on macOS — bundles
// declare their handleable schemes statically in Info.plist, and
// LaunchServices has no API to clear the default handler back to
// "none". The closest we can do is set another bundle as default,
// which requires finding one. For now unregisterProtocol() is a
// best-effort no-op with a logged note.

#include "url_protocol.hpp"

#include <QApplication>
#include <QDebug>

#include <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include "app.hpp"

namespace app_shell {

namespace {
    NSString* schemeNSString(const QString& s) {
        return [NSString stringWithUTF8String:s.toUtf8().constData()];
    }

    NSString* myBundleIdentifier() {
        return [[NSBundle mainBundle] bundleIdentifier];
    }
}

bool UrlProtocol::isRegistered() const {
    NSString* myId = myBundleIdentifier();
    if (!myId) return false;

    CFStringRef defaultBundle = LSCopyDefaultHandlerForURLScheme(
        (__bridge CFStringRef)schemeNSString(name()));
    if (!defaultBundle) return false;

    bool matches = CFStringCompare(defaultBundle, (__bridge CFStringRef)myId,
                                   kCFCompareCaseInsensitive) == kCFCompareEqualTo;
    CFRelease(defaultBundle);
    return matches;
}

void UrlProtocol::registerProtocol() {
    NSString* myId = myBundleIdentifier();
    if (!myId) {
        qWarning() << "[UrlProtocol] No bundle identifier — cannot register.";
        return;
    }

    OSStatus status = LSSetDefaultHandlerForURLScheme(
        (__bridge CFStringRef)schemeNSString(name()),
        (__bridge CFStringRef)myId);

    if (status != noErr)
        qWarning() << "[UrlProtocol] LSSetDefaultHandlerForURLScheme failed: " << status;
}

void UrlProtocol::unregisterProtocol() {
    // macOS has no API to clear the default handler. The bundle still
    // declares the scheme via Info.plist; LaunchServices keeps a default
    // assigned. Leaving this as a documented no-op rather than fake-success.
    qInfo() << "[UrlProtocol] unregisterProtocol() is a no-op on macOS — "
               "the scheme is declared in Info.plist and cannot be retracted at runtime.";
}

}  // namespace app_shell
