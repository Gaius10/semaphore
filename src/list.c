#include <stdlib.h>
#include <stdio.h>

#include "../lib/list.h"
#include "../lib/car.h"

List* create_list() {
    List* list = (List*)malloc(sizeof(List));

    if (list == NULL) {
        printf("Error allocating memory for List.\n");
        printf("Exiting...\n");
        exit(EXIT_FAILURE);
    }

    list->size = 0;
    return list;
}

void list_append(List* list, Car* car) {
    if (list->size >= LIST_MAX_SIZE) {
        printf("List is full. Cannot append new car.\n");
        return;
    }

    list->items[list->size++] = car;
}

void list_remove(List* list, int index) {
    if (index < 0 || index >= list->size) {
        printf("Index out of bounds. Cannot remove car.\n");
        return;
    }

    for (int i = index; i < list->size - 1; i++) {
        list->items[i] = list->items[i + 1];
    }

    list->size--;
}

void list_destroy(List* list) {
    for (int i = 0; i < list->size; i++) {
        destroy_car(list->items[i]);
    }

    free(list);
}

Car* list_get(List* list, int index) {
    if (index < 0 || index >= list->size) {
        printf("Index out of bounds. Cannot get car.\n");
        return NULL;
    }

    return list->items[index];
}
