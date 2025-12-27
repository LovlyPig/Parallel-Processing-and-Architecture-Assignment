#pragma once

#include <atomic>
#include <iostream>

// 管理指针的标记
// 指针的最低位用来作为标记
namespace PointerUtils {
    template<typename T>
    T* get_pointer(T* p) {
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(p) & ~uintptr_t(0x1));
    }

    template<typename T>
    bool is_marked(T* p) {
        return (reinterpret_cast<uintptr_t>(p) & 0x1) != 0;
    }

    template<typename T>
    T* get_marked(T* p) {
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(p) | 0x1);
    }

    template<typename T>
    T* get_unmarked(T* p) {
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(p) & ~uintptr_t(0x1));
    }
}

// 有序升序链表
template<typename T>
class LockFreeList {
private:
    struct Node {
        T value;
        std::atomic<Node*> next;

        Node(const T& val) : value(val) {
            next.store(nullptr);
        }
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;

    // 查找值所在的位置，返回前驱节点和当前节点
    // 物理删除被标记的节点
    void find(const T& value, Node*& pre, Node*& cur) {
    retry:
        pre = head.load();
        cur = pre->next.load();

        while (true) {
            // cur为空，或cur的值大于等于value，停止
            if (!cur || PointerUtils::get_pointer(cur)->value >= value) {
                break;
            }
            pre = cur;
            cur = PointerUtils::get_pointer(cur)->next.load();
        }

        // 检查cur是否被标记，如果是，则物理删除它
        while (cur && PointerUtils::is_marked(cur)) {
            Node* unmarked_cur = PointerUtils::get_pointer(cur);
            Node* next_cur = unmarked_cur->next.load();

            if (!pre->next.compare_exchange_strong(cur, next_cur)) {
                goto retry;
            }
            //delete unmarked_cur;
            cur = next_cur;
        }
    }

public:
    LockFreeList() {
        head = new Node(T{});
        tail = new Node(T{});
        head.load()->next.store(tail.load());
    }

    ~LockFreeList() {
        Node *node = PointerUtils::get_pointer(head.load());
        while (node) {
            Node* next = PointerUtils::get_pointer(node->next.load());
            delete node;
            node = next;
        }
    }

    bool add(const T& value) {
        Node* new_node = new Node(value);
        Node* pre = nullptr;
        Node* cur = nullptr;

        while (true) {
            find(value, pre, cur);

            if (cur && PointerUtils::get_pointer(cur)->value == value && !PointerUtils::is_marked(cur)) {
                delete new_node;
                return false; 
            }

            new_node->next.store(cur);
            if (pre->next.compare_exchange_strong(cur, new_node)) {
                return true;   
            }
        }
    }

    bool remove(const T& value) {
        Node* pre = nullptr;
        Node* cur = nullptr;

        while (true) {
            find(value, pre, cur);

            if (!cur || PointerUtils::get_pointer(cur)->value != value) {
                return false; 
            }

            Node* unmarked_cur = PointerUtils::get_pointer(cur);
            Node* next_cur = unmarked_cur->next.load();
            Node* marked_next = PointerUtils::get_marked(next_cur);

            if (unmarked_cur->next.compare_exchange_strong(next_cur, marked_next)) {
                Node *cur_marked_node = PointerUtils::get_marked(unmarked_cur);
                if (pre->next.compare_exchange_strong(cur, next_cur)) {
                    delete unmarked_cur;
                } else {}

                return true;
            }

        }
    }

    bool contains(const T& value) {
        Node* cur = head.load();
        while (cur && PointerUtils::get_pointer(cur) != tail.load() && PointerUtils::get_pointer(cur)->value < value) {
            cur = cur->next.load();
        }

        Node* p_cur = PointerUtils::get_pointer(cur);
        return p_cur && p_cur != tail.load() &&
                p_cur->value == value &&
                !PointerUtils::is_marked(p_cur->next.load());
    }
};