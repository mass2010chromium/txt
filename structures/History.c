#include "History.h"

#include <assert.h>
#include <stdlib.h>

/**
 * Make a new history object, with the specified capcity.
 */
History* make_History(size_t size, destructor_t destructor) {
    History* ret = malloc(sizeof(History));
    inplace_make_History(ret, size, destructor);
    return ret;
}
void inplace_make_History(History* history, size_t size, destructor_t destructor) {
    history->unlimited = (size == 0);
    history->destructor_handle = destructor;
    if (size == 0) {
        size = 10;
        history->cursor = 0;
    }
    else {
        inplace_make_Deque(&history->deque, size);
    }
    inplace_make_Vector(&history->stack, size);
}

/**
 * Push a new item into the history.
 */
void History_push(History* this, void* item) {
    if (this->unlimited) {
        // Clear the rest, add this.
        for (size_t i = this->cursor; i < this->stack.size; ++i) {
            this->destructor_handle(this->stack.elements[i]);
            free(this->stack.elements[i]);
        }
        this->stack.size = this->cursor;
        Vector_push(&this->stack, item);
        this->cursor = this->stack.size;
        return;
    }
    if (Deque_full(&this->deque)) {
        void* old_item = Deque_pop(&this->deque);
        this->destructor_handle(old_item);
        free(old_item);
    }
    assert(Deque_push(&this->deque, item) == 0);
    for (size_t i = 0; i < this->stack.size; ++i) {
        this->destructor_handle(this->stack.elements[i]);
        free(this->stack.elements[i]);
    }
    this->stack.size = 0;
}

/**
 * Scroll the history buffer.
 * Positive means back in time.
 * Returns a pointer to the new "now".
 * Can return NULL if you scrolled to the end!
 */
void** History_scroll(History* this, ssize_t amount) {
    if (this->unlimited) {
        this->cursor -= amount;
        if (this->cursor == 0) {
            return NULL;
        }
        return &this->stack.elements[this->cursor - 1];
    }
    if (amount > 0) {
        for (size_t i = 0; i < amount; ++i) {
            void* item = Deque_pop_r(&this->deque);
            Vector_push(&this->stack, item);
        }
    }
    else {
        for (size_t i = 0; i < amount; ++i) {
            void* item = Vector_pop(&this->stack);
            assert(Deque_push(&this->deque, item) == 0);
        }
    }
    if (Deque_size(&this->deque) == 0) {
        return NULL;
    }
    return Deque_peek_r(&this->deque);
}

/**
 * Equivalent to History.scroll(0).
 * Get the item at the end of the deque.
 * Returns a pointer to "now".
 */
void** History_peek(History* this) {
    if (this->unlimited) {
        if (this->cursor == 0) {
            return NULL;
        }
        return &this->stack.elements[this->cursor - 1];
    }
    if (Deque_size(&this->deque) == 0) {
        return NULL;
    }
    return Deque_peek_r(&this->deque);
}

/**
 * Get the current depth.
 * Equal to the amount you can scroll back.
 * Depth is at most (size), and at least 0.
 * 0 means empty history.
 */
size_t History_get_depth(History* this) {
    if (this->unlimited) {
        return this->cursor;
    }
    return Deque_size(&this->deque);
}

/**
 * NOTE: The History object has ownership of all its items.
 * They will be destroyed here!
 */
void History_destroy(History* this) {
    for (size_t i = 0; i < this->stack.size; ++i) {
        this->destructor_handle(this->stack.elements[i]);
        free(this->stack.elements[i]);
    }
    Vector_destroy(&this->stack);
    if (!this->unlimited) {
        size_t deque_idx = this->deque.head;
        for (size_t i = 0; i < this->deque.size; ++i) {
            this->destructor_handle(this->deque.elements[deque_idx]);
            free(this->deque.elements[deque_idx]);
            ++deque_idx;
            if (deque_idx == this->deque.max_size) {
                deque_idx = 0;
            }
        }
        Deque_destroy(&this->deque);
    }
}

