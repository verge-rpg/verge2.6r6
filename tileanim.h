
#ifndef TILEANIM_H
#define TILEANIM_H

#include "vtypes.h"
#include <vector>

// Proto
namespace VSP
{
	class VSP;
};

class TileAnim
{
	struct StrandState
	{
		bool flip;
		int count;

		StrandState() : flip(false), count(0) {}
	};

	VSP::VSP* pVsp;

private:
	std::vector<StrandState> strand;
	std::vector<int>  tileidx;

public:

	TileAnim() : pVsp(0) {}

    int count;

	void Reset(VSP::VSP* vsp);
	void Update();

	inline int GetTileIdx(u32 idx)
	{
		if (idx<0 || idx>=tileidx.size() )
			return 0;
		return tileidx[idx];
	}
};

#endif
