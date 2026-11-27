/* auxrsrc.r - the resources of Archaic for A/UX (mkaux): rez makes them,
 * into the file %archaic next to the program. The A/UX Toolbox does not
 * start a program without its resource file. SIZE: the Mac heap.
 * Archaic's pages and pictures are in Unix memory (malloc), so it is
 * small. Archaic runs in the background too, gets suspend and resume
 * events, and the click that brings it to the front is not a click on
 * the page. */
#include "types.r"

resource 'SIZE' (-1) {
	dontSaveScreen,
	acceptSuspendResumeEvents,
	enableOptionSwitch,
	canBackground,
	multiFinderAware,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreChildDiedEvents,
	is32BitCompatible,
	reserved,
	reserved,
	reserved,
	reserved,
	reserved,
	reserved,
	reserved,
	512 * 1024,
	256 * 1024
};
