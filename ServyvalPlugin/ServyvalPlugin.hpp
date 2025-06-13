#pragma once

extern "C" {
#include <Core.h>
}

namespace ServyvalPlugin {

	const char* Format_Positive = "&a+%i &f%s &a[%i]";
	const char* Format_Negative = "&c-%i &f%s &a[%i]";
	const char* Format_Neutral = "&f%s &a[%i]";
	constexpr int Message_Type = 13;
	constexpr int Message_AlternateType = 12;

	void Able(BlockID block, cc_bool enabled);
	void OnBlockChange(BlockID block, cc_bool del, int type);
	void PlaceHandler(int index, BlockID block);
	void DeleteHandler(int index, BlockID block);
	void OnHeldBlockChanged(void* obj);
	void LeavesRandomTick(int index, BlockID block);

	static void Init(void);

} // namespace ServyvalPlugin
