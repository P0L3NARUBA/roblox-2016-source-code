#pragma once

namespace RBX {

	enum TonemapperMode {
		LEGACY,
		REINHARD,
		ACES,
		PBR_NEUTRAL,
		AGX,
	};

	enum FilterMode {
		BOX,
		GAUSSIAN,
		DIRECTIONAL,
		RADIAL,
	};

}