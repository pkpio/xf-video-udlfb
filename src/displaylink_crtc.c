#include "displaylink.h"


static void
dl_crtc_dpms(xf86CrtcPtr crtc, int mode)
{
}

static Bool
dl_crtc_lock (xf86CrtcPtr crtc)
{
    	return FALSE;
}

static void
dl_crtc_unlock (xf86CrtcPtr crtc)
{
}

static void
dl_crtc_prepare (xf86CrtcPtr crtc)
{
}

static void
dl_crtc_commit (xf86CrtcPtr crtc)
{
}

static Bool
dl_crtc_mode_fixup(xf86CrtcPtr crtc, DisplayModePtr mode,
		     DisplayModePtr adjusted_mode)
{
    return TRUE;
}

static void
dl_crtc_mode_set(xf86CrtcPtr crtc, DisplayModePtr mode,
		   DisplayModePtr adjusted_mode,
		   int x, int y)
{

	fprintf(stderr,"DisplayLINK CRTC MODE SET %d %d\n", x ,y);
}


static void
dl_crtc_gamma_set(xf86CrtcPtr crtc, CARD16 *red, CARD16 *green, CARD16 *blue,
		    int size)
{
}

#if RANDR_13_INTERFACE
static void
dl_crtc_set_origin(xf86CrtcPtr crtc, int x, int y)
{

	fprintf(stderr,"Set DISPLAYLINK position to %d %d\n", x, y);
}
#endif

static const xf86CrtcFuncsRec dl_crtc_funcs = {
    .dpms = dl_crtc_dpms,
    .lock = dl_crtc_lock,
    .unlock = dl_crtc_unlock,
    .mode_fixup = dl_crtc_mode_fixup,
    .prepare = dl_crtc_prepare,
    .mode_set = dl_crtc_mode_set,
    .commit = dl_crtc_commit,
    .gamma_set = dl_crtc_gamma_set,
#if RANDR_13_INTERFACE
    .set_origin = dl_crtc_set_origin,
#endif
};

void
dl_crtc_init(ScrnInfoPtr pScrn)
{
	DisplayLinkPtr fPtr = DLPTR(pScrn);

    	fPtr->crtc = xf86CrtcCreate (pScrn, &dl_crtc_funcs);
	if (fPtr->crtc) {
		fPtr->crtc->enabled = TRUE;
		// rotation is still not supported
		fPtr->crtc->rotation = RR_Rotate_0 ;
	}

}

