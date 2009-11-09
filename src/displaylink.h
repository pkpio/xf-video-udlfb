#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86Modes.h"

#include "xf86Crtc.h"

#include "xf86DDC.h"

#include "edid.h"

#include "damage.h"

#define DL_IOCTL_BLIT           0xAA
#define DL_IOCTL_COPY           0xAB
#define DL_IOCTL_BLIT_DB        0xAC
#define DL_IOCTL_EDID           0xAD

// private structure (still based on fbdev)
typedef struct {
        int                             fd;
        unsigned char*                  fbstart;
        unsigned char*                  fbmem;
        int                             fboff;
        int                             lineLength;
        CloseScreenProcPtr              CloseScreen;
        CreateScreenResourcesProcPtr    CreateScreenResources;
        EntityInfoPtr                   pEnt;
        OptionInfoPtr                   Options;
        DamagePtr                       pDamage;
        PixmapPtr                       pPixmap;
        xf86CrtcPtr			crtc;
	xf86OutputPtr			output;
} DisplayLinkRec, *DisplayLinkPtr;

// macro from fbdev
#define DLPTR(p) ((DisplayLinkPtr)((p)->driverPrivate))

void dl_crtc_init(ScrnInfoPtr pScrn);
void displaylink_output_init(ScrnInfoPtr pScrn);
