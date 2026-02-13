#include "inventory.h"
#include <string.h>

static const ItemMetadata ITEM_METADATA[ITEM_COUNT] = {
    [ITEM_NONE]    = { "Empty",   "" },
    [ITEM_POTION]  = { "Potion",  "Restores 10 HP." },
    [ITEM_KEY]     = { "Key",     "Opens a locked door." },
    [ITEM_SWORD]   = { "Sword",   "A sturdy blade. +3 ATK." },
    [ITEM_SHIELD]  = { "Shield",  "Blocks incoming attacks. +2 DEF." },
    [ITEM_ROPE]    = { "Rope",    "50 feet of hempen rope." },
    [ITEM_TORCH]   = { "Torch",   "Lights up dark areas." },
    [ITEM_MAP]     = { "Map",     "Shows the surrounding area." },
    [ITEM_COMPASS] = { "Compass", "Always points north." },
};

void inventory_init(Inventory *inv) {
    memset(inv, 0, sizeof(*inv));
    inv->money = 100;

    // Test items so the grid isn't empty
    inventory_add_item(inv, ITEM_POTION, 3);
    inventory_add_item(inv, ITEM_KEY, 1);
    inventory_add_item(inv, ITEM_SWORD, 1);
    inventory_add_item(inv, ITEM_SHIELD, 1);
    inventory_add_item(inv, ITEM_TORCH, 5);
    inventory_add_item(inv, ITEM_ROPE, 1);
}

bool inventory_add_item(Inventory *inv, ItemID id, int quantity) {
    if (id <= ITEM_NONE || id >= ITEM_COUNT || quantity <= 0) return false;

    // Stack onto existing slot first
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (inv->slots[i].id == id) {
            inv->slots[i].quantity += quantity;
            return true;
        }
    }

    // Find first empty slot
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (inv->slots[i].id == ITEM_NONE) {
            inv->slots[i].id = id;
            inv->slots[i].quantity = quantity;
            return true;
        }
    }

    return false; // inventory full
}

bool inventory_remove_item(Inventory *inv, ItemID id, int quantity) {
    if (id <= ITEM_NONE || id >= ITEM_COUNT || quantity <= 0) return false;

    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (inv->slots[i].id == id) {
            if (inv->slots[i].quantity < quantity) return false;
            inv->slots[i].quantity -= quantity;
            if (inv->slots[i].quantity <= 0) {
                inv->slots[i].id = ITEM_NONE;
                inv->slots[i].quantity = 0;
            }
            return true;
        }
    }

    return false; // item not found
}

const ItemMetadata *inventory_get_metadata(ItemID id) {
    if (id < 0 || id >= ITEM_COUNT) return &ITEM_METADATA[ITEM_NONE];
    return &ITEM_METADATA[id];
}
