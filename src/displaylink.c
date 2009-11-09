/*

	Copyright 2009 Roberto De Ioris <roberto@unbit.it>

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * A copy of the General Public License is included with the source
 * distribution of this driver, as COPYING.


Special Thanks to Unbit (http://unbit.it) for letting me work on this stuff, and to
Marvell for sending me a wonderful sheevaplug (http://www.marvell.com/featured/plugcomputing.jsp) for testing
the driver on embedded systems.

---------------------

TODO:
- rotation
- better randr support


*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <fcntl.h>

#include "displaylink.h"

#include "xf86_OSproc.h"

#include "mipointer.h"
#include "mibstore.h"
#include "micmap.h"
#include "colormapst.h"
#include "xf86cmap.h"

#include "fb.h"

#include "xf86Resources.h"
#include "xf86RAC.h"

#include "fbdevhw.h"


// prototypes (as fbdev)
static const OptionInfoRec * DisplayLinkAvailableOptions(int chipid, int busid);
static void	DisplayLinkIdentify(int flags);
static Bool	DisplayLinkProbe(DriverPtr drv, int flags);
static Bool	DisplayLinkPreInit(ScrnInfoPtr pScrn, int flags);
static Bool	DisplayLinkScreenInit(int Index, ScreenPtr pScreen, int argc,
				char **argv);
static Bool	DisplayLinkCloseScreen(int scrnIndex, ScreenPtr pScreen);

static Bool	DisplayLinkDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op,
				pointer ptr);

// damage handler
static void DLBlockHandler(pointer data, OSTimePtr pTimeout, pointer pRead);
static void DLWakeupHandler(pointer data, int i, pointer LastSelectMask);



static int pix24bpp = 0;

#define DL_VERSION	3
#define DL_NAME		"DL"
#define DL_DRIVER_NAME	"displaylink"


_X_EXPORT DriverRec DL = {
	DL_VERSION,
	DL_DRIVER_NAME,
	DisplayLinkIdentify,
	DisplayLinkProbe,
	DisplayLinkAvailableOptions,
	NULL,
	0,
	DisplayLinkDriverFunc,

};

/* Supported "chipsets" */
static SymTabRec DisplayLinkChipsets[] = {
    { 0, "displaylink" },
    {-1, NULL }
};


/* Supported options */
typedef enum {
	OPTION_FBDEV,
	OPTION_DEBUG
} DisplayLinkOpts;

static const OptionInfoRec DisplayLinkOptions[] = {
	{ OPTION_FBDEV,		"fbdev",	OPTV_STRING,	{0},	FALSE },
	{ -1,			NULL,		OPTV_NONE,	{0},	FALSE }
};

/* -------------------------------------------------------------------- */

#ifdef XFree86LOADER

MODULESETUPPROTO(DisplayLinkSetup);

static XF86ModuleVersionInfo DisplayLinkVersRec =
{
	"displaylink",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR, PACKAGE_VERSION_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	NULL,
	{0,0,0,0}
};

_X_EXPORT XF86ModuleData displaylinkModuleData = { &DisplayLinkVersRec, DisplayLinkSetup, NULL };

pointer
DisplayLinkSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
	static Bool setupDone = FALSE;

	if (!setupDone) {
		setupDone = TRUE;
		xf86AddDriver(&DL, module, HaveDriverFuncs);
		return (pointer)1;
	} else {
		if (errmaj) *errmaj = LDR_ONCEONLY;
		return NULL;
	}
}

#endif /* XFree86LOADER */


static Bool
DisplayLinkGetRec(ScrnInfoPtr pScrn)
{
	if (pScrn->driverPrivate != NULL)
		return TRUE;
	
	pScrn->driverPrivate = xnfcalloc(sizeof(DisplayLinkRec), 1);
	return TRUE;
}

static void
DisplayLinkFreeRec(ScrnInfoPtr pScrn)
{
	if (pScrn->driverPrivate == NULL)
		return;
	xfree(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
}

static const OptionInfoRec *
DisplayLinkAvailableOptions(int chipid, int busid)
{
	return DisplayLinkOptions;
}

static void
DisplayLinkIdentify(int flags)
{
	xf86PrintChipsets(DL_NAME, "driver for ", DisplayLinkChipsets);
}



static Bool
DisplayLinkProbe(DriverPtr drv, int flags)
{
	int i;
	ScrnInfoPtr pScrn ;
       	GDevPtr *devSections;
	int numDevSections;
	int bus,device,func;
	char *dev;
	int entity;
	Bool foundScreen = FALSE;

	if ((numDevSections = xf86MatchDevice(DL_DRIVER_NAME, &devSections)) <= 0) 
	    return FALSE;
	
	if (!xf86LoadDrvSubModule(drv, "fbdevhw"))
	    return FALSE;
	    
	for (i = 0; i < numDevSections; i++) {

	    pScrn = NULL;
	    dev = xf86FindOptionValue(devSections[i]->options,"fbdev");

	    if (fbdevHWProbe(NULL,dev,NULL)) {

		entity = xf86ClaimFbSlot(drv, 0,
                                              devSections[i], TRUE);		
		pScrn = xf86ConfigFbEntity(pScrn,0,entity,
					       NULL,NULL,NULL,NULL);
		
	    }

	    // hack for system with real cards		
	    fbSlotClaimed = FALSE;

	    if (pScrn) {
	    	foundScreen = TRUE;
	    
	    	pScrn->driverVersion = DL_VERSION;
	    	pScrn->driverName    = DL_DRIVER_NAME;
	    	pScrn->name          = DL_NAME;
	    	pScrn->Probe         = DisplayLinkProbe;
	    	pScrn->PreInit       = DisplayLinkPreInit;
	    	pScrn->ScreenInit    = DisplayLinkScreenInit;
	    	pScrn->SwitchMode    = fbdevHWSwitchModeWeak();
	    	pScrn->AdjustFrame   = fbdevHWAdjustFrameWeak();
	    	pScrn->EnterVT       = fbdevHWEnterVTWeak();
	    	pScrn->LeaveVT       = fbdevHWLeaveVTWeak();
	    	pScrn->ValidMode     = fbdevHWValidModeWeak();

	    	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "using %s\n", dev ? dev : "default device");
		}
	}
	xfree(devSections);
	return foundScreen;
}


displaylink_xf86crtc_resize (ScrnInfoPtr scrn, int width, int height)
{
    int         old_x = scrn->virtualX;
    int         old_y = scrn->virtualY;

    fprintf(stderr,"displaylink CRTC resize !!! %dx%d\n", width, height);

    if (old_x == width && old_y == height)
        return TRUE;

    scrn->virtualX = width;
    scrn->virtualY = height;
    return TRUE;
}

static const xf86CrtcConfigFuncsRec displaylink_xf86crtc_config_funcs = {
    displaylink_xf86crtc_resize
};

static Bool
DisplayLinkPreInit(ScrnInfoPtr pScrn, int flags)
{
	DisplayLinkPtr fPtr;
	int default_depth, fbbpp;
	const char *s;
	int type;
	char edid[128];
	xf86MonPtr DLMonPtr;

	if (flags & PROBE_DETECT) return FALSE;


	/* Check the number of entities, and fail if it isn't one. */
	if (pScrn->numEntities != 1)
		return FALSE;

	pScrn->monitor = pScrn->confScreen->monitor;

	DisplayLinkGetRec(pScrn);
	fPtr = DLPTR(pScrn);

	fPtr->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

	pScrn->racMemFlags = RAC_FB | RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;
	/* XXX Is this right?  Can probably remove RAC_FB */
	pScrn->racIoFlags = RAC_FB | RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;

	/* open device */
	if (!fbdevHWInit(pScrn,NULL,xf86FindOptionValue(fPtr->pEnt->device->options,"fbdev")))
		return FALSE;

	// Damn, better to use fbdevHWGetFD()...
        fPtr->fd = open(xf86FindOptionValue(fPtr->pEnt->device->options,"fbdev"),O_WRONLY);
	if (fPtr->fd < 0) {
		return FALSE;
	}

	ioctl(fPtr->fd,DL_IOCTL_EDID, edid);
	DLMonPtr = xf86InterpretEDID(pScrn->scrnIndex, (Uchar *) edid);
	xf86PrintEDID(DLMonPtr);


	default_depth = fbdevHWGetDepth(pScrn,&fbbpp);
	if (!xf86SetDepthBpp(pScrn, default_depth, default_depth, fbbpp,
			     Support24bppFb | Support32bppFb | SupportConvert32to24 | SupportConvert24to32))
		return FALSE;
	xf86PrintDepthBpp(pScrn);

	/* Get the depth24 pixmap format */
	if (pScrn->depth == 24 && pix24bpp == 0)
		pix24bpp = xf86GetBppFromDepth(pScrn, 24);

	/* color weight */
	if (pScrn->depth > 8) {
		rgb zeros = { 0, 0, 0 };
		if (!xf86SetWeight(pScrn, zeros, zeros))
			return FALSE;
	}

	/* visual init */
	if (!xf86SetDefaultVisual(pScrn, -1))
		return FALSE;

	/* We don't currently support DirectColor at > 8bpp */
	if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "requested default visual"
			   " (%s) is not supported at depth %d\n",
			   xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
		return FALSE;
	}

	{
		Gamma zeros = {0.0, 0.0, 0.0};

		if (!xf86SetGamma(pScrn,zeros)) {
			return FALSE;
		}
	}

	pScrn->progClock = TRUE;
	pScrn->rgbBits   = 8;
	pScrn->chipset   = "displaylink";
	pScrn->videoRam  = fbdevHWGetVidmem(pScrn);

	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "hardware: %s (video memory:"
		   " %dkB)\n", fbdevHWGetName(pScrn), pScrn->videoRam/1024);

	/* handle options */
	xf86CollectOptions(pScrn, NULL);
	if (!(fPtr->Options = xalloc(sizeof(DisplayLinkOptions))))
		return FALSE;
	memcpy(fPtr->Options, DisplayLinkOptions, sizeof(DisplayLinkOptions));
	xf86ProcessOptions(pScrn->scrnIndex, fPtr->pEnt->device->options, fPtr->Options);


	/* Load fb module */
	if (xf86LoadSubModule(pScrn, "fb") == NULL) {
		DisplayLinkFreeRec(pScrn);
		return FALSE;
	}


	xf86CrtcConfigInit(pScrn, &displaylink_xf86crtc_config_funcs);

	dl_crtc_init(pScrn);
        if (!fPtr->crtc) {
		DisplayLinkFreeRec(pScrn);
        	return FALSE;
	}

        displaylink_output_init(pScrn);

	if (!fPtr->output) {
		DisplayLinkFreeRec(pScrn);
        	return FALSE;
	}

	/* setup video modes */

	/* check for edid incongruences (as when udlfb is loaded before attaching a monitor) */
	pScrn->currentMode = fbdevHWGetBuildinMode(pScrn) ;
	
	pScrn->modes = fPtr->output->funcs->get_modes(fPtr->output);
	if (pScrn->modes->HDisplay <= pScrn->currentMode->HDisplay && pScrn->modes->VDisplay <= pScrn->currentMode->VDisplay) {
		pScrn->currentMode = pScrn->modes ;
	}
	pScrn->virtualX = pScrn->currentMode->HDisplay ;
	pScrn->virtualY = pScrn->currentMode->VDisplay ;

	xf86CrtcSetSizeRange (pScrn, 320, 200, pScrn->virtualX, pScrn->virtualY);

	xf86InitialConfiguration (pScrn, TRUE);

	xf86PrintModes(pScrn);

	pScrn->displayWidth = pScrn->virtualX;

	xf86SetDpi(pScrn, 0, 0);


	return TRUE;
}


static Bool
DisplayLinkCreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DisplayLinkPtr fPtr = DLPTR(pScrn);
    Bool ret;

    pScreen->CreateScreenResources = fPtr->CreateScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = DisplayLinkCreateScreenResources;


    if (!DamageSetup(pScreen))
        return FALSE;


    fPtr->pDamage = DamageCreate((DamageReportFunc)NULL,
                                 (DamageDestroyFunc)NULL,
                                 DamageReportNone,
                                 TRUE, pScreen, pScreen);

    if (!RegisterBlockAndWakeupHandlers(DLBlockHandler, DLWakeupHandler,
                                        (pointer)pScreen))
						return FALSE;

    fPtr->pPixmap = pScreen->GetScreenPixmap(pScreen);

    DamageRegister(&fPtr->pPixmap->drawable, fPtr->pDamage);


    return TRUE;
}

static Bool
DisplayLinkDamageInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DisplayLinkPtr fPtr = DLPTR(pScrn);
    

    fPtr->CreateScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = DisplayLinkCreateScreenResources;

    return TRUE;
}


static Bool
DisplayLinkScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	DisplayLinkPtr fPtr = DLPTR(pScrn);
	VisualPtr visual;
	int init_picture = 0;
	int ret, flags;
	int type;


	if (NULL == (fPtr->fbmem = fbdevHWMapVidmem(pScrn))) {
	        xf86DrvMsg(scrnIndex,X_ERROR,"mapping of video memory"
			   " failed\n");
		return FALSE;
	}
	fPtr->fboff = fbdevHWLinearOffset(pScrn);

	fbdevHWSave(pScrn);

	fbdevHWSaveScreen(pScreen, SCREEN_SAVER_ON);
	fbdevHWAdjustFrame(scrnIndex,0,0,0);

	miClearVisualTypes();
	if (!miSetVisualTypes(pScrn->depth, TrueColorMask, pScrn->rgbBits, TrueColor)) {
		xf86DrvMsg(scrnIndex,X_ERROR,"visual type setup failed"
			   " for %d bits per pixel [1]\n",
			   pScrn->bitsPerPixel);
		return FALSE;
	}

	if (!miSetPixmapDepths()) {
	  xf86DrvMsg(scrnIndex,X_ERROR,"pixmap depth setup failed\n");
	  return FALSE;
	}

	fPtr->fbstart = fPtr->fbmem + fPtr->fboff;

	ret = fbScreenInit(pScreen, fPtr->fbstart,
					   pScrn->virtualX,
					   pScrn->virtualY, pScrn->xDpi,
					   pScrn->yDpi, pScrn->displayWidth,
					   pScrn->bitsPerPixel);
	if (!ret)
		return FALSE;

	if (pScrn->bitsPerPixel > 8) {
		/* Fixup RGB ordering */
		visual = pScreen->visuals + pScreen->numVisuals;
		while (--visual >= pScreen->visuals) {
			if ((visual->class | DynamicClass) == DirectColor) {
				visual->offsetRed   = pScrn->offset.red;
				visual->offsetGreen = pScrn->offset.green;
				visual->offsetBlue  = pScrn->offset.blue;
				visual->redMask     = pScrn->mask.red;
				visual->greenMask   = pScrn->mask.green;
				visual->blueMask    = pScrn->mask.blue;
			}
		}
	}

	/* must be after RGB ordering fixed */
	if (!fbPictureInit(pScreen, NULL, 0))
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			   "Render extension initialisation failed\n");

	if (!DisplayLinkDamageInit(pScreen)) {
	    xf86DrvMsg(scrnIndex, X_ERROR,
		       "damage initialization failed\n");
	    return FALSE;
	}


	xf86SetBlackWhitePixels(pScreen);
	miInitializeBackingStore(pScreen);
	xf86SetBackingStore(pScreen);

	/* software cursor */
	miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

	/* colormap */
	if (!miCreateDefColormap(pScreen)) {
			xf86DrvMsg(scrnIndex, X_ERROR,
                                   "internal error: miCreateDefColormap failed "
				   "in DisplayLinkScreenInit()\n");
		return FALSE;
	}

	flags = CMAP_PALETTED_TRUECOLOR;
	if(!xf86HandleColormaps(pScreen, 256, 8, fbdevHWLoadPaletteWeak(), 
				NULL, flags))
		return FALSE;

	xf86DPMSInit(pScreen, fbdevHWDPMSSetWeak(), 0);

	xf86CrtcScreenInit (pScreen);

	pScreen->SaveScreen = fbdevHWSaveScreenWeak();

	/* Wrap the current CloseScreen function */
	fPtr->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = DisplayLinkCloseScreen;

	return TRUE;
}

static Bool
DisplayLinkCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	DisplayLinkPtr fPtr = DLPTR(pScrn);
	
	DamageUnregister(&fPtr->pPixmap->drawable, fPtr->pDamage);
	RemoveBlockAndWakeupHandlers(DLBlockHandler, DLWakeupHandler,
                                 (pointer) pScreen);
	fbdevHWRestore(pScrn);
	fbdevHWUnmapVidmem(pScrn);
	//close(fPtr->fd);
	pScrn->vtSema = FALSE;

	pScreen->CreateScreenResources = fPtr->CreateScreenResources;
	pScreen->CloseScreen = fPtr->CloseScreen;
	return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}




static Bool
DisplayLinkDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op, pointer ptr)
{
    xorgHWFlags *flag;
    
    switch (op) {
	case GET_REQUIRED_HW_INTERFACES:
	    flag = (CARD32*)ptr;
	    (*flag) = 0;
	    return TRUE;
	default:
	    return FALSE;
    }
}


// real drawing function

static void
DLBlockHandler(pointer data, OSTimePtr pTimeout, pointer pRead)
{
    ScreenPtr pScreen = (ScreenPtr) data;
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
        DisplayLinkPtr fPtr = DLPTR(pScrn);
	RegionPtr pRegion;
	int coords[4];

	if (!fPtr->pDamage)
		return;

	pRegion = DamageRegion(fPtr->pDamage);
        if (REGION_NOTEMPTY(pScreen, pRegion)) {
		int         nbox = REGION_NUM_RECTS (pRegion);
        	BoxPtr      pbox = REGION_RECTS (pRegion);

		coords[0] = pScrn->virtualX ;
        	coords[1] = pScrn->virtualY ;
        	coords[2] = 0 ;
        	coords[3] = 0 ;

		while(nbox--) {
                	if (pbox->x1 < coords[0])
                        	coords[0] = pbox->x1 ;

                	if (pbox->y1 < coords[1])
                        	coords[1] = pbox->y1 ;

                	if ( pbox->x2  > coords[2])
                        	coords[2] = pbox->x2 ;

                	if ( pbox->y2 > coords[3])
                        	coords[3] = pbox->y2 ;

                	pbox++;
        	}

        	coords[2] = coords[2] - coords[0] ;
        	coords[3] = coords[3] - coords[1] ;

        	ioctl(fPtr->fd, DL_IOCTL_BLIT, &coords);

        	DamageEmpty(fPtr->pDamage);
    	}


}

static void
DLWakeupHandler(pointer data, int i, pointer LastSelectMask)
{
}

