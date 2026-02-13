#ifndef INVENTORY_H
#define INVENTORY_H

#include <stdbool.h>

typedef enum ItemID {
    ITEM_NONE = 0,
    ITEM_POTION,
    ITEM_KEY,
    ITEM_SWORD,
    ITEM_SHIELD,
    ITEM_ROPE,
    ITEM_TORCH,
    ITEM_MAP,
    ITEM_COMPASS,
    ITEM_COUNT
} ItemID;

typedef struct ItemMetadata {
    const char *name;
    const char *description;
} ItemMetadata;

#define MAX_INVENTORY_SLOTS 32

typedef struct InventorySlot {
    ItemID id;
    int quantity;
} InventorySlot;

typedef struct Inventory {
    InventorySlot slots[MAX_INVENTORY_SLOTS];
    int money;
} Inventory;

void inventory_init(Inventory *inv);
bool inventory_add_item(Inventory *inv, ItemID id, int quantity);
bool inventory_remove_item(Inventory *inv, ItemID id, int quantity);
const ItemMetadata *inventory_get_metadata(ItemID id);

#endif
