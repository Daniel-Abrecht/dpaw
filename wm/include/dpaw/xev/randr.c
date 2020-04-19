#ifndef XEV_RANDR_H
#define XEV_RANDR_H

#include <X11/extensions/Xrandr.h>

#define RRScreenChangeNotifyMask  (1L << 0)
/* V1.2 additions */
#define RRCrtcChangeNotifyMask      (1L << 1)
#define RROutputChangeNotifyMask    (1L << 2)
#define RROutputPropertyNotifyMask  (1L << 3)
/* V1.4 additions */
#define RRProviderChangeNotifyMask   (1L << 4)
#define RRProviderPropertyNotifyMask (1L << 5)
#define RRResourceChangeNotifyMask   (1L << 6)
/* V1.6 additions */
#define RRLeaseNotifyMask            (1L << 7)

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

#include <dpaw/xev.template>

#endif
