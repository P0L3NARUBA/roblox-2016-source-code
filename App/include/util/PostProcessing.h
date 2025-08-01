#pragma once

namespace RBX {

	enum TonemapperMode {
		LEGACY = 0,
		REINHARD = 1,
		ACES = 2,
		PBR_NEUTRAL = 3,
		AGX = 4,
	};

	enum FilterMode {
		BOX = 0,
		GAUSSIAN = 1,
		DIRECTIONAL = 2,
		RADIAL = 3,
	};

}