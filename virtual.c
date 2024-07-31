#include "oslabs.h"

// FIFO
int process_page_access_fifo(struct PTE page_table[TABLEMAX], int *table_cnt, int page_number, int frame_pool[POOLMAX], int *frame_cnt, int current_timestamp) {

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
        // Find the page with the earliest arrival time for replacement (not the current page)
        int replaceIndex = -1;
        for (int i = 0; i < *table_cnt; i++) {
            if (page_table[i].is_valid && (replaceIndex == -1 || page_table[i].arrival_timestamp < page_table[replaceIndex].arrival_timestamp)) {
                replaceIndex = i;
            }
        }
        int frame = page_table[replaceIndex].frame_number;
        page_table[replaceIndex] = (struct PTE){0, -1, -1, -1, -1};
        page_table[page_number] = (struct PTE){1, frame, current_timestamp, current_timestamp, 1};
        return frame;
    }
}


int count_page_faults_fifo(struct PTE page_table[TABLEMAX], int table_cnt, int reference_string[REFERENCEMAX], int reference_cnt, int frame_pool[POOLMAX], int frame_cnt) {
    int faults = 0;
    int timestamp = 1;
    int current_table_cnt = 0; 

    // Queue to track the order of pages loaded into frames
    int frameQueue[POOLMAX]; 
    int front = 0, rear = -1;

    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];

        int pageFound = 0;
        for (int j = 0; j < table_cnt; j++) { // Search the whole table
            if (page_table[j].is_valid && page_table[j].frame_number == page_number) {
                page_table[j].last_access_timestamp = timestamp;
                page_table[j].reference_count++;
                pageFound = 1;
                break;
            }
        }

        if (!pageFound) {
            faults++; 

            if (frame_cnt > 0 || current_table_cnt < table_cnt) { 
                // Free frame available or still empty slots in the page table
                
                // Load page into the next available frame or page table slot
                if (frame_cnt > 0) {
                    int frame = frame_pool[--frame_cnt];
                    page_table[current_table_cnt].is_valid = 1;
                    page_table[current_table_cnt].frame_number = frame;
                    frameQueue[++rear] = current_table_cnt; // Add to frame queue using index
                } else {
                    // No frame, but there's an empty slot in the page table
                    page_table[current_table_cnt].is_valid = 1;
                    page_table[current_table_cnt].frame_number = page_number; // Assume page_number is also the frame_number
                }
                page_table[current_table_cnt].arrival_timestamp = timestamp;
                page_table[current_table_cnt].last_access_timestamp = timestamp;
                page_table[current_table_cnt].reference_count = 1;
                current_table_cnt++; // Increment only after filling a slot
            } else {
                // No free frame or empty slots, replace the oldest page (FIFO)
                int replaceIndex = frameQueue[front];
                front = (front + 1) % table_cnt; // Use table_cnt as the queue size

                page_table[replaceIndex].frame_number = page_number;
                page_table[replaceIndex].arrival_timestamp = timestamp;
                page_table[replaceIndex].last_access_timestamp = timestamp;
                page_table[replaceIndex].reference_count = 1;
                frameQueue[rear] = replaceIndex; // Update queue with the replaced page's index
                rear = (rear + 1) % table_cnt;
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
    int timestamp = 1;
    int current_table_cnt = 0;

    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];
        int frame = process_page_access_lru(page_table, &current_table_cnt, page_number, frame_pool, &frame_cnt, timestamp++);
        if (frame != page_number) {
            faults++;
        }
    }
    return faults;
}

// LFU
int process_page_access_lfu(struct PTE page_table[TABLEMAX], int *table_cnt, int page_number, int frame_pool[POOLMAX], int *frame_cnt, int current_timestamp) {
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
            if (page_table[i].reference_count < page_table[replaceIndex].reference_count ||
                (page_table[i].reference_count == page_table[replaceIndex].reference_count &&
                 page_table[i].arrival_timestamp < page_table[replaceIndex].arrival_timestamp)) {
                replaceIndex = i;
            }
        }
        int frame = page_table[replaceIndex].frame_number;
        page_table[replaceIndex] = (struct PTE){0, -1, -1, -1, -1};
        page_table[page_number] = (struct PTE){1, frame, current_timestamp, current_timestamp, 1};
        return frame;
    }
}

int count_page_faults_lfu(struct PTE page_table[TABLEMAX], int table_cnt, int reference_string[REFERENCEMAX], int reference_cnt, int frame_pool[POOLMAX], int frame_cnt) {
    int faults = 0;
    int timestamp = 1;
    int current_table_cnt = 0;

    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];
        int frame = process_page_access_lfu(page_table, &current_table_cnt, page_number, frame_pool, &frame_cnt, timestamp++);
        if (frame != page_number) {
            faults++;
        }
    }
    return faults;
}
