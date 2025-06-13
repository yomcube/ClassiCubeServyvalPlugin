#ifdef _WIN32
	#define CC_API __declspec(dllimport)
	#define CC_VAR __declspec(dllimport)
	#define EXPORT __declspec(dllexport)
#else
	#define CC_API
	#define CC_VAR
	#define EXPORT __attribute__((visibility("default")))
#endif

#include "ServyvalPlugin.hpp"

extern "C" {
	#include <Block.h>
	#include <BlockID.h>
	#include <BlockPhysics.h>
	#include <Chat.h>
	#include <Constants.h>
	#include <Entity.h>
	#include <Event.h>
	#include <ExtMath.h>
	#include <Game.h>
	#include <Inventory.h>
	#include <Platform.h>
	#include <Server.h>
	#include <String.h>
	#include <World.h>
}

namespace ServyvalPlugin {

	static cc_string emptystr;
	static RNGState rnd;

	static int Counts[BLOCK_COUNT];
	static PhysicsHandler PhysHandlers[512];

	void Able(BlockID block, cc_bool enabled) {
		Blocks.CanPlace[block] = enabled;
		Inventory.Map[block] = enabled ? block : 0;
	}

	void OnBlockChange(BlockID block, cc_bool del, int type) {
		// Don't count water or lava
		if (block > 7 && block < 12)
			return;

		Chat_AddOf(&emptystr, Message_AlternateType);

		int count = 1;

		if (del) {
			Counts[block] += count;
			Able(block, true);

			int i = INVENTORY_BLOCKS_PER_HOTBAR;
			int j = -1;
			for (; i > -1; i--) {
				if (Inventory_Get(i) == block) {
					j = -1;
					break;
				}
				if (Inventory_Get(i) == 0) j = i;
			}
			if (j != -1) Inventory_Set(j, block);
		}
		else {
			Counts[block] -= count;
			if (Counts[block] < 1) {
				Counts[block] = 0;
				Able(block, false);
				Inventory_Set(Inventory.SelectedIndex, 0);
			}
		}

		char strbuf[STRING_SIZE];
		cc_string str = String_FromArray(strbuf);
		cc_string tmp = Block_UNSAFE_GetName(block);
		String_Format3(&str, del ? Format_Positive : Format_Negative, &count, &tmp, &Counts[block]);
		Chat_AddOf(&str, type);
	}
	void PlaceHandler(int index, BlockID block) {
		OnBlockChange(block, false, Message_Type);

		if (PhysHandlers[block] != NULL) {
			PhysicsHandler handler = PhysHandlers[block];
			handler(index, block);
		}
	}
	void DeleteHandler(int index, BlockID block) {
		OnBlockChange(block, true, Message_Type);

		if (block == BLOCK_LEAVES) {
			int r = Random_Next(&rnd, 31);
			if (!r) OnBlockChange(BLOCK_SAPLING, true, Message_AlternateType);
		}

		if (PhysHandlers[block + 256] != NULL) {
			PhysicsHandler handler = PhysHandlers[block + 256];
			handler(index, block);
		}
	}
	void OnHeldBlockChanged(void* obj) {
		Chat_AddOf(&emptystr, Message_AlternateType);

		if (Inventory_SelectedBlock == 0) {
			Chat_AddOf(&emptystr, Message_Type);
			return;
		}
		char strbuf[STRING_SIZE];
		cc_string str = String_FromArray(strbuf);
		cc_string tmp = Block_UNSAFE_GetName(Inventory_SelectedBlock);
		String_Format2(&str, Format_Neutral, &tmp, &Counts[Inventory_SelectedBlock]);
		Chat_AddOf(&str, Message_Type);
	}
#define CheckLeaves(b,x,y,z) tmp = World_GetBlock(x,y,z); b = b || tmp == BLOCK_LEAVES || tmp == BLOCK_LOG;
	void LeavesRandomTick(int index, BlockID block) {
		// TODO: Use packed index instead of coordinates
		int x, y, z, tmp;
		World_Unpack(index, x, y, z);

		cc_bool b = false;
		CheckLeaves(b, x + 1, y, z);
		CheckLeaves(b, x - 1, y, z);
		CheckLeaves(b, x, y, z + 1);
		CheckLeaves(b, x, y, z - 1);
		CheckLeaves(b, x, y + 1, z);
		CheckLeaves(b, x, y - 1, z);
		if (!b) Game_ChangeBlock(x, y, z, BLOCK_AIR);
	}

	static void Init(void) {
		if (!Server.IsSinglePlayer)
			return;

		emptystr = String_FromReadonly("");

		TimeMS t = DateTime_CurrentUTC();
		Random_Seed(&rnd, (int)t);

		Physics.Enabled = true;

		struct LocalPlayer* p = (struct LocalPlayer*)Entities.List[ENTITIES_SELF_ID];
		p->Hacks.CanFly = false;
		p->Hacks.CanNoclip = false;
		p->Hacks.CanSpeed = false;
		p->Hacks.CanRespawn = false;
		p->Hacks.FullBlockStep = false;
		p->Hacks.MaxJumps = 1;
		p->ReachDistance = 5.0f;

		int i = 0;
		for (; i < BLOCK_COUNT; i++) {
			Inventory.Map[i] = 0;
			Counts[i] = 0;
			Blocks.CanPlace[i] = false;
		}
		for (i = 0; i < INVENTORY_HOTBARS * INVENTORY_BLOCKS_PER_HOTBAR; i++)
			Inventory.Table[i] = 0;
		for (i = 1; i < 256; i++) {
			PhysHandlers[i] = Physics.OnPlace[i];
			PhysHandlers[i + 256] = Physics.OnDelete[i];
			Physics.OnPlace[i] = PlaceHandler;
			Physics.OnDelete[i] = DeleteHandler;
		}
		Physics.OnRandomTick[BLOCK_LEAVES] = LeavesRandomTick;

		Event_Register(&UserEvents.HeldBlockChanged, NULL, OnHeldBlockChanged);
	}

}

extern "C" {
	EXPORT int Plugin_ApiVersion = 1;
	EXPORT struct IGameComponent Plugin_Component = { ServyvalPlugin::Init };
}
