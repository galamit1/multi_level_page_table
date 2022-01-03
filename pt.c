//
// Created by gal amit on 21/10/2021.
//

#include <stdio.h>
#include <stdlib.h>
#include "os.h"
#include <stdint.h>


#define OFFSET_SIZE 12
#define LEVELS 5
#define TABLE_ENTRY_POINTER_SIZE 9

#define VALID_MASK 1


void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn);
uint64_t page_table_query(uint64_t pt, uint64_t vpn);
uint64_t walk_page_table_update(uint64_t * pt_pointer, uint64_t vpn, uint64_t new_ppn, int update, int create);
uint64_t ppn_to_pte(uint64_t ppn);
int is_valid(uint64_t pte);
uint64_t remove_valid_bit(uint64_t pte);


uint64_t get_level_vpn_pointer(uint64_t vpn, int level) {
    int move_bits= (LEVELS - (level + 1)) * TABLE_ENTRY_POINTER_SIZE;
    uint64_t pointer = vpn >> move_bits;
    uint64_t bit_mask = (1 << TABLE_ENTRY_POINTER_SIZE) - 1;
    return pointer & bit_mask;
}

int is_valid(uint64_t pte) {
    if ((VALID_MASK & pte) == 0u) {
        return 0;
    }
    return 1;
}

uint64_t ppn_to_pte(uint64_t ppn) {
    ppn = ppn << OFFSET_SIZE;
    ppn = ppn | VALID_MASK; // add valid bit
    return ppn;
}

uint64_t remove_valid_bit(uint64_t pte) {
    return pte % 2 == 1 ? pte - 1 : pte;
}

uint64_t walk_page_table_update(uint64_t * pt_pointer, uint64_t vpn, uint64_t new_ppn, int update, int create) {
    uint64_t * root = pt_pointer;
    uint64_t pte;
    uint64_t vpn_pointer;
    uint64_t ppn;
    int level;
    for (level = 0; level < LEVELS; level++) {
        vpn_pointer = get_level_vpn_pointer(vpn, level);
        pte = root[vpn_pointer]; // gets to vpn_pointer row in the page table the root is pointing on
        if (!is_valid(pte)) {
            if (update && create) { // creates the next level
                ppn = alloc_page_frame();
                root[vpn_pointer] = ppn_to_pte(ppn); // update pte
            } else {
                return NO_MAPPING;
            }
        }
        root = (uint64_t *)phys_to_virt(remove_valid_bit(root[vpn_pointer])); // pointer to the next level
    }
    vpn_pointer = get_level_vpn_pointer(vpn, level);
    pte = root[vpn_pointer];
    if (!update) { // its check
        return is_valid(pte) == 0 ? NO_MAPPING : pte >> OFFSET_SIZE; // get the ppn
    }
    if (create) {
        root[vpn_pointer] = ppn_to_pte(new_ppn);
    } else { // destroy
        root[vpn_pointer] = remove_valid_bit(pte);
    }
    return NO_MAPPING;
}


void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn) {
    uint64_t * pt_pointer = phys_to_virt(pt << 12); //remove the offset
    if (ppn == NO_MAPPING) { //destroy
        walk_page_table_update(pt_pointer, vpn, ppn, 1, 0);
    }
    else { // create mapping
        walk_page_table_update(pt_pointer, vpn, ppn, 1, 1);
    }
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn) { // check for mapping
    uint64_t * pt_pointer = phys_to_virt(pt << 12);
    return walk_page_table_update(pt_pointer, vpn, NO_MAPPING, 0, 0);
}