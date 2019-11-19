#ifndef XEV_RANDR_H
#define XEV_RANDR_H

#include <X11/extensions/Xrandr.h>

#define XEV_EVENTS \
  X(XEV_BaseEvent, ( \
    Y(RRScreenChangeNotify, XRRScreenChangeNotifyEvent) \
    Y(RRNotify, XRRNotifyEvent) \
  )) \
  X(RRNotify, ( \
    Y(RRNotify_CrtcChange, XRRCrtcChangeNotifyEvent) \
    Y(RRNotify_OutputChange, XRROutputChangeNotifyEvent) \
    Y(RRNotify_OutputProperty, XRROutputPropertyNotifyEvent) \
    Y(RRNotify_ProviderChange, XRRProviderChangeNotifyEvent) \
    Y(RRNotify_ProviderProperty, XRRProviderPropertyNotifyEvent) \
    Y(RRNotify_ResourceChange, XRRResourceChangeNotifyEvent) \
  ))

#define XEV_EXT randr
#define XEV_OPTIONAL

#include <xev.template>

#endif
