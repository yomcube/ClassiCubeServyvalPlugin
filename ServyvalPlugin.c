#ifdef _WIN32
    #define CC_API __declspec(dllimport)
    #define CC_VAR __declspec(dllimport)
    #define EXPORT __declspec(dllexport)
#else
    #define CC_API
    #define CC_VAR
    #define EXPORT __attribute__((visibility("default")))
#endif

#include "src/Block.h"
#include "src/BlockID.h"
#include "src/BlockPhysics.h"
#include "src/Chat.h"
#include "src/Constants.h"
#include "src/Entity.h"
#include "src/Event.h"
#include "src/ExtMath.h"
#include "src/Game.h"
#include "src/Inventory.h"
#include "src/Platform.h"
#include "src/Server.h"
#include "src/String.h"
#include "src/World.h"

#define POSFORMAT "&a+1 &f%s &a[%i]"
#define NEGFORMAT "&c-1 &f%s &a[%i]"
#define NEUFORMAT      "&f%s &a[%i]"
#define MSGTYPE 13
#define ALTMSGTYPE 12

static cc_string emptystr;
static RNGState rnd;

static int Counts[BLOCK_COUNT];
static PhysicsHandler PhysHandlers[512];

static void Able(BlockID block, cc_bool enabled) {
	Blocks.CanPlace[block] = enabled;
	Inventory.Map[block] = enabled ? block : 0;
}

static void OnBlockChange(BlockID block, cc_bool del, int type) {
	// Don't count water or lava
	if (block > 7 && block < 12)
		return;
		
	Chat_AddOf(&emptystr, ALTMSGTYPE);
	
	if (del) {
		Counts[block] += 1;
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
		Counts[block] -= 1;
		if (Counts[block] < 1) {
			Counts[block] = 0;
			Able(block, false);
			Inventory_Set(Inventory.SelectedIndex, 0);
		}
	}
	
	char strbuf[STRING_SIZE];
	cc_string str = String_FromArray(strbuf);
	cc_string tmp = Block_UNSAFE_GetName(block);
	String_Format2(&str, del ? POSFORMAT : NEGFORMAT, &tmp, &Counts[block]);
	Chat_AddOf(&str, type);
}
static void PlaceHandler(int index, BlockID block) {
	OnBlockChange(block, false, MSGTYPE);
	
	if (PhysHandlers[block] != NULL) {
		PhysicsHandler handler = PhysHandlers[block];
		handler(index, block);
	}
}
static void DeleteHandler(int index, BlockID block) {
	OnBlockChange(block, true, MSGTYPE);
	
	if (block == BLOCK_LEAVES) {
		int r = Random_Next(&rnd, 31);
		if (!r) OnBlockChange(BLOCK_SAPLING, true, ALTMSGTYPE);
	}
	
	if (PhysHandlers[block + 256] != NULL) {
		PhysicsHandler handler = PhysHandlers[block + 256];
		handler(index, block);
	}
}
static void OnHeldBlockChanged(void* obj) {
	Chat_AddOf(&emptystr, ALTMSGTYPE);
	
	if (Inventory_SelectedBlock == 0) {
		Chat_AddOf(&emptystr, MSGTYPE);
		return;
	}
	char strbuf[STRING_SIZE];
	cc_string str = String_FromArray(strbuf);
	cc_string tmp = Block_UNSAFE_GetName(Inventory_SelectedBlock);
	String_Format2(&str, NEUFORMAT, &tmp, &Counts[Inventory_SelectedBlock]);
	Chat_AddOf(&str, MSGTYPE);
}
#define CheckLeaves(b,x,y,z) tmp = World_GetBlock(x,y,z); b = b || tmp == BLOCK_LEAVES || tmp == BLOCK_LOG;
static void LeavesRandomTick(int index, BlockID block) {
	// TODO: Use packed index instead of coordinates
	int x,y,z, tmp;
	World_Unpack(index, x,y,z);
	
	cc_bool b = false;
	CheckLeaves(b, x + 1, y,     z    );
	CheckLeaves(b, x - 1, y,     z    );
	CheckLeaves(b, x,     y,     z + 1);
	CheckLeaves(b, x,     y,     z - 1);
	CheckLeaves(b, x,     y + 1, z    );
	CheckLeaves(b, x,     y - 1, z    );
	if (!b) Game_ChangeBlock(x,y,z, BLOCK_AIR);
}

static void ServyvalPlugin_Init(void) {
	if (!Server.IsSinglePlayer)
		return;
	
	emptystr = String_FromReadonly("");
	
	TimeMS t = DateTime_CurrentUTC_MS();
	Random_Seed(&rnd, (int)t);
	
	Physics.Enabled = true;
	
	struct LocalPlayer* p = (struct LocalPlayer*)Entities.List[ENTITIES_SELF_ID];
	p->Hacks.CanFly        = false;
	p->Hacks.CanNoclip     = false;
	p->Hacks.CanSpeed      = false;
	p->Hacks.CanRespawn    = false;
	p->Hacks.FullBlockStep = false;
	p->Hacks.MaxJumps      = 1;
	p->ReachDistance       = 5.0f;
	
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

EXPORT int Plugin_ApiVersion = 1;
EXPORT struct IGameComponent Plugin_Component = { ServyvalPlugin_Init };