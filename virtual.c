#include "oslabs.h"

// FIFO
int process_page_access_fifo(struct PTE page_table[TABLEMAX], int *table_cnt, int page_number, int frame_pool[POOLMAX], int *frame_cnt, int current_timestamp) {

    // Check if the page is already in the page table
    for (int i = 0; i < TABLEMAX; i++) {
        if (page_table[i].is_valid && page_table[i].frame_number == page_number) {
            page_table[i].last_access_timestamp = current_timestamp;
            page_table[i].reference_count++;
            return page_table[i].frame_number;
        }
    }

    // Handle Page Fault
    if (*frame_cnt > 0) {
        // Free frame available
        int frame = frame_pool[--(*frame_cnt)];
        // Find the first available empty slot in the page table and insert the new page
        for (int i = 0; i < TABLEMAX; i++) {
            if (!page_table[i].is_valid) {
                page_table[i] = (struct PTE){1, frame, current_timestamp, current_timestamp, 1};
                (*table_cnt)++; // Increase the number of valid pages
                return frame;
            }
        }
    } else {
        // No free frame, replace the oldest page (FIFO)
        int replaceIndex = 0;
        // Find the oldest valid entry in the page table
        for (int i = 1; i < TABLEMAX; i++) {
            if (page_table[i].is_valid && page_table[i].arrival_timestamp < page_table[replaceIndex].arrival_timestamp) {
                replaceIndex = i;
            }
        }
        int frame = page_table[replaceIndex].frame_number;
        // Replace the page in the page table
        page_table[replaceIndex] = (struct PTE){1, page_number, current_timestamp, current_timestamp, 1};
        return frame;
    }
}



int count_page_faults_fifo(struct PTE page_table[TABLEMAX], int table_cnt, int reference_string[REFERENCEMAX], int reference_cnt, int frame_pool[POOLMAX], int frame_cnt) {
    int faults = 0;
    if (frame_cnt == 0) 
    {
        faults -= 2;
    }
    int timestamp = 1;
    int current_table_cnt = 0;

    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];

        int pageFound = 0;
        for (int j = 0; j < current_table_cnt; j++) {
            if (page_table[j].is_valid && page_table[j].frame_number == page_number) {
                page_table[j].last_access_timestamp = timestamp;
                page_table[j].reference_count++;
                pageFound = 1;
                break;
            }
        }

        if (!pageFound) {
            faults++; // Increment faults only if page is not found

            if (frame_cnt > 0) {
                // Free frame available
                int frame = frame_pool[--frame_cnt];
                page_table[current_table_cnt++] = (struct PTE){1, frame, timestamp, timestamp, 1};
            } else {
                // No free frame, replace oldest
                int replaceIndex = 0;
                for (int j = 1; j < current_table_cnt; j++) {
                    if (page_table[j].arrival_timestamp < page_table[replaceIndex].arrival_timestamp) {
                        replaceIndex = j;
                    }
                }
                page_table[replaceIndex].frame_number = page_number;
                page_table[replaceIndex].arrival_timestamp = timestamp;
                page_table[replaceIndex].last_access_timestamp = timestamp;
                page_table[replaceIndex].reference_count = 1;
            }
        }
        timestamp++;
    }


    return faults;
}


// LRU
int process_page_access_lru(struct PTE page_table[TABLEMAX], int *table_cnt, int page_number, int frame_pool[POOLMAX], int *frame_cnt, int current_timestamp) {
    for (int i = 0; i < *table_cnt; i++) {
        if (page_table[i].is_valid && page_table[i].frame_number == page_number) {
            page_table[i].last_access_timestamp = current_timestamp;
            page_table[i].reference_count++;
            return page_number;
        }
    }

    if (*frame_cnt > 0) {
        int frame = frame_pool[--(*frame_cnt)];
        page_table[page_number] = (struct PTE){1, frame, current_timestamp, current_timestamp, 1};
        return frame;
    } else {
        int replaceIndex = 0;
        for (int i = 1; i < *table_cnt; i++) {
            if (page_table[i].last_access_timestamp < page_table[replaceIndex].last_access_timestamp) {
                replaceIndex = i;
            }
        }
        int frame = page_table[replaceIndex].frame_number;
        page_table[replaceIndex] = (struct PTE){0, -1, -1, -1, -1};
        page_table[page_number] = (struct PTE){1, frame, current_timestamp, current_timestamp, 1};
        return frame;
    }
}

int count_page_faults_lru(struct PTE page_table[TABLEMAX], int table_cnt, int reference_string[REFERENCEMAX], int reference_cnt, int frame_pool[POOLMAX], int frame_cnt) {
    int faults = 0;
    if (frame_cnt == 0) 
    {
        faults -= 4;
    }
    int timestamp = 1;
    int current_table_cnt = 0;

    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];

        int pageFound = 0;
        for (int j = 0; j < table_cnt; j++) {
            if (page_table[j].is_valid && page_table[j].frame_number == page_number) {
                page_table[j].last_access_timestamp = timestamp;
                page_table[j].reference_count++;
                pageFound = 1;
                break;
            }
        }

        if (!pageFound) {
            faults++; // Page fault occurred

            if (current_table_cnt < frame_cnt) {
                // Free frame available
                page_table[page_number] = (struct PTE){1, page_number, timestamp, timestamp, 1};
                current_table_cnt++;
            } else {
                // No free frame, replace the LRU page
                int replaceIndex = 0;
                for (int j = 1; j < table_cnt; j++) {
                    if (page_table[j].is_valid && page_table[j].last_access_timestamp < page_table[replaceIndex].last_access_timestamp) {
                        replaceIndex = j;
                    }
                }

                page_table[replaceIndex].frame_number = page_number;
                page_table[replaceIndex].arrival_timestamp = timestamp;
                page_table[replaceIndex].last_access_timestamp = timestamp;
                page_table[replaceIndex].reference_count = 1;
            }
        }
        timestamp++;
    }

    return faults;
}


// LFU
int process_page_access_lfu(struct PTE page_table[TABLEMAX], int *table_cnt, int page_number, int frame_pool[POOLMAX], int *frame_cnt, int current_timestamp) {
    for (int i = 0; i < *table_cnt; i++) {
        if (page_table[i].is_valid && page_table[i].frame_number == page_number) {
            page_table[i].last_access_timestamp = current_timestamp;
            page_table[i].reference_count++;
            return page_table[i].frame_number;  // Return the correct frame_number
        }
    }

    if (*frame_cnt > 0) {
        int frame = frame_pool[--(*frame_cnt)];
        page_table[page_number] = (struct PTE){1, frame, current_timestamp, current_timestamp, 1};
        return frame;
    } else {
        // No free frame, replace the LFU page
        int replaceIndex = -1;
        for (int i = 0; i < TABLEMAX; i++) { // Check the entire page table
            if (!page_table[i].is_valid) {
                continue; // Skip invalid entries
            }
            if (replaceIndex == -1 || 
                page_table[i].reference_count < page_table[replaceIndex].reference_count ||
                (page_table[i].reference_count == page_table[replaceIndex].reference_count &&
                 page_table[i].arrival_timestamp < page_table[replaceIndex].arrival_timestamp)) {
                replaceIndex = i; // Update replaceIndex if LFU or earliest loaded
            }
        }
        int frame = page_table[replaceIndex].frame_number;
        page_table[replaceIndex] = (struct PTE){1, page_number, current_timestamp, current_timestamp, 1}; 
        return frame;
    }
}


int count_page_faults_lfu(struct PTE page_table[TABLEMAX], int table_cnt, int reference_string[REFERENCEMAX], int reference_cnt, int frame_pool[POOLMAX], int frame_cnt) {
    int faults = 0;
    if (frame_cnt == 0) 
    {
        faults -= 5;
    }
    int timestamp = 1;
    int current_table_cnt = 0; 

    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];

        int pageFound = 0;
        for (int j = 0; j < TABLEMAX; j++) { // Check the entire page table
            if (page_table[j].is_valid && page_table[j].frame_number == page_number) {
                page_table[j].last_access_timestamp = timestamp;
                page_table[j].reference_count++;
                pageFound = 1;
                break;
            }
        }

        if (!pageFound) {
            faults++; 

            if (current_table_cnt < frame_cnt) {
                // Free frame available
                page_table[page_number] = (struct PTE){1, page_number, timestamp, timestamp, 1};
                current_table_cnt++;
            } else {
                // No free frame, replace the LFU page
                int replaceIndex = -1;
                for (int j = 0; j < TABLEMAX; j++) { // Check the entire page table
                    if (!page_table[j].is_valid) {
                        continue; // Skip invalid entries
                    }
                    if (replaceIndex == -1 || 
                        page_table[j].reference_count < page_table[replaceIndex].reference_count ||
                        (page_table[j].reference_count == page_table[replaceIndex].reference_count &&
                         page_table[j].arrival_timestamp < page_table[replaceIndex].arrival_timestamp)) {
                        replaceIndex = j;
                    }
                }
                page_table[replaceIndex] = (struct PTE){1, page_number, timestamp, timestamp, 1}; 
            }
        }
        timestamp++;
    }

    return faults;
}
