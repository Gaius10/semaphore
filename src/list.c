#include <stdlib.h>
#include <stdio.h>

#include "../lib/list.h"
#include "../lib/car.h"

void list_init(list_t* list) {
    list->size = 0;
}

void list_append(list_t* list, car_t* car) {
    if (list->size >= LIST_MAX_SIZE) {
        printf("list_t is full. Cannot append new car.\n");
        return;
    }

    list->items[list->size++] = car;
}

void list_remove(list_t* list, int index) {
    if (index < 0 || index >= list->size) {
        printf("Index out of bounds. Cannot remove car.\n");
        return;
    }

    for (int i = index; i < list->size - 1; i++) {
        list->items[i] = list->items[i + 1];
    }

    list->size--;
}

void list_destroy(list_t* list) {
    for (int i = 0; i < list->size; i++) {
        destroy_car(list->items[i]);
    }

    free(list);
}

car_t* list_get(list_t* list, int index) {
    if (index < 0 || index >= list->size) {
        printf("Index out of bounds. Cannot get car.\n");
        return NULL;
    }

    return list->items[index];
}
