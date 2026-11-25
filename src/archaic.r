/* archaic.r - the Mac's resources for Archaic (mkmac). SIZE: the memory
 * that MultiFinder gives Archaic. The pictures of the slices take the
 * most: in 256 colors 4 slices of a 640 x 480 window are about 1.2 MB. */
#include "Processes.r"

resource 'SIZE' (-1) {
	reserved,
	ignoreSuspendResumeEvents,
	reserved,
	canBackground,
	doesActivateOnFGSwitch,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreChildDiedEvents,
	is32BitCompatible,
	notHighLevelEventAware,
	onlyLocalHLEvents,
	notStationeryAware,
	dontUseTextEditServices,
	notDisplayManagerAware,
	reserved,
	reserved,
	3072 * 1024,
	1536 * 1024
};
