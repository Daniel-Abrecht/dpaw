#ifndef XEV_RANDR_H
#define XEV_RANDR_H

#include <X11/extensions/Xrandr.h>

#define XEV_EVENTS \
  X(XEV_BaseEvent, ( \
    Y(RRScreenChangeNotify, XRRScreenChangeNotifyEvent, RRScreenChangeNotifyMask) \
    Y(RRNotify, XRRNotifyEvent, 0) \
  )) \
  X(RRNotify, ( \
    Y(RRNotify_CrtcChange, XRRCrtcChangeNotifyEvent, RRCrtcChangeNotifyMask) \
    Y(RRNotify_OutputChange, XRROutputChangeNotifyEvent, RROutputChangeNotifyMask) \
    Y(RRNotify_OutputProperty, XRROutputPropertyNotifyEvent, RROutputPropertyNotifyMask) \
    Y(RRNotify_ProviderChange, XRRProviderChangeNotifyEvent, RRProviderChangeNotifyMask) \
    Y(RRNotify_ProviderProperty, XRRProviderPropertyNotifyEvent, RRProviderPropertyNotifyMask) \
    Y(RRNotify_ResourceChange, XRRResourceChangeNotifyEvent, RRResourceChangeNotifyMask) \
  ))

#define XEV_EXT randr

#include <-dpaw/xev.template>

#endif
