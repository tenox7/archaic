/* archaic.r - the Mac's resources for Archaic (mkmac). SIZE: the memory
 * that MultiFinder and System 7 give Archaic. The pictures of the slices
 * take the most: in 256 colors 4 slices of a 640 x 480 window are about
 * 1.2 MB. Its flags: Archaic runs in the background too, gets suspend
 * and resume events (it deactivates its window itself), and Apple events
 * (the Finder's Quit). */
#include "Processes.r"

resource 'SIZE' (-1) {
	reserved,
	acceptSuspendResumeEvents,
	reserved,
	canBackground,
	doesActivateOnFGSwitch,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreChildDiedEvents,
	is32BitCompatible,
	isHighLevelEventAware,
	onlyLocalHLEvents,
	notStationeryAware,
	dontUseTextEditServices,
	notDisplayManagerAware,
	reserved,
	reserved,
	3072 * 1024,
	1536 * 1024
};
