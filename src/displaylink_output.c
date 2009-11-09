#include "displaylink.h"

static void displaylink_output_prepare(xf86OutputPtr output)
{
}

static void displaylink_output_commit(xf86OutputPtr output)
{
}

static void
displaylink_dpms(xf86OutputPtr output, int mode)
{
}

static void
displaylink_save (xf86OutputPtr output)
{
}

static void
displaylink_restore (xf86OutputPtr output)
{
}

static int
displaylink_mode_valid(xf86OutputPtr output, DisplayModePtr pMode)
{
    return MODE_OK;
}

static Bool
displaylink_mode_fixup(xf86OutputPtr output, DisplayModePtr mode,
		    DisplayModePtr adjusted_mode)
{
    return TRUE;
}

static void
displaylink_mode_set(xf86OutputPtr output, DisplayModePtr mode,
		  DisplayModePtr adjusted_mode)
{
	fprintf(stderr,"DisplayLink MODESET\n");
}

static xf86OutputStatus
displaylink_detect(xf86OutputPtr output)
{
	return XF86OutputStatusConnected;
}

static void
displaylink_destroy (xf86OutputPtr output)
{
}

#ifdef RANDR_GET_CRTC_INTERFACE
static xf86CrtcPtr
displaylink_get_crtc(xf86OutputPtr output)
{
    	return output->crtc;
}
#endif

static DisplayModePtr
displaylink_get_modes (xf86OutputPtr output)
{
	DisplayModePtr          modes;
	char edid[128];
	int ret;
        xf86MonPtr DLMonPtr;
	ScrnInfoPtr pScrn = output->scrn;
        DisplayLinkPtr fPtr = DLPTR(pScrn);

	ret = ioctl(fPtr->fd,DL_IOCTL_EDID, edid);
	DLMonPtr = xf86InterpretEDID(pScrn->scrnIndex, (Uchar *) edid);


	xf86OutputSetEDID (output, DLMonPtr);
	modes = xf86OutputGetEDIDModes (output);

	return modes;
}

static const xf86OutputFuncsRec displaylink_output_funcs = {
    .dpms = displaylink_dpms,
    .save = displaylink_save,
    .restore = displaylink_restore,
    .mode_valid = displaylink_mode_valid,
    .mode_fixup = displaylink_mode_fixup,
    .prepare = displaylink_output_prepare,
    .mode_set = displaylink_mode_set,
    .commit = displaylink_output_commit,
    .detect = displaylink_detect,
    .get_modes = displaylink_get_modes,
    .destroy = displaylink_destroy,
#ifdef RANDR_GET_CRTC_INTERFACE
    .get_crtc = displaylink_get_crtc,
#endif
};

void
displaylink_output_init(ScrnInfoPtr pScrn)
{
	xf86OutputPtr output;
	DisplayLinkPtr fPtr = DLPTR(pScrn);

    	fPtr->output = xf86OutputCreate (pScrn, &displaylink_output_funcs, (char *) fbdevHWGetName(pScrn));
	if (fPtr->output) {
		fPtr->output->possible_crtcs |= (1 << 0);
		fPtr->output->crtc = fPtr->crtc;
		fPtr->output->subpixel_order = SubPixelHorizontalRGB;
		fPtr->output->interlaceAllowed = FALSE;
    		fPtr->output->doubleScanAllowed = FALSE;
	}

	
}
