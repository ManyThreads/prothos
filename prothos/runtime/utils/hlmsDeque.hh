#pragma once

#include <cstddef>
#include <array>
//#include <assert.h>

//#include "atomic.h"
#include "utils/shim.hh"
#include <atomic>
//using namespace std;

namespace hlms{


      template<class T, size_t NODE_SIZE = 1u << 12>
      class deque {
      public:

        typedef T* pointer_type;

        inline deque();

        inline void push_bottom(pointer_type ptr);
        inline std::pair<bool, pointer_type> pop_top();
        inline pointer_type pop_bottom();

      private:

        struct deque_node;

        struct deque_node_header {
          deque_node_header() : next(nullptr), prev(nullptr) {}
          deque_node* next;// = nullptr;
          deque_node* prev;// = nullptr;
        };

        struct deque_node : public deque_node_header {

          deque_node()
          {
            //assert(reinterpret_cast<size_t>(this) % NODE_SIZE == 0);
#ifndef NDEBUG
            for (auto& el : data) {
              el.store(reinterpret_cast<pointer_type>(0xDEADBEEF));
            }
#endif
          }

          static constexpr size_t node_size = NODE_SIZE;
          static constexpr size_t array_size = (node_size - sizeof(deque_node_header)) / sizeof(pointer_type);

          pointer_type load(size_t index)
          {
            return data[index].load();
          }

          void store(size_t index, pointer_type val)
          {
            data[index].store(val);
          }

          void* operator new(size_t size)
          {
            void* storage = aligned_alloc(size, node_size);
            //if (!storage) {
              //throw std::bad_alloc();
            //}
            return storage;
          }

          //void operator delete(void* ptr)
          //{
            //free(ptr);
          //}

          protected:
            std::array<std::atomic<pointer_type>, array_size> data;
        };

        //static_assert(sizeof(deque_node) == deque_node::node_size, "Invalid node size!");

        struct cas_struct {

          static constexpr size_t index_mask = deque_node::node_size-1;
          static constexpr size_t index_offset = 3;
          //static_assert(1 << index_offset == sizeof(pointer_type), "Incorrect index offset.");
          static constexpr size_t tag_mask = (1 << index_offset)-1;

          cas_struct() : node(nullptr), index(0), tag(0) {}

          deque_node* node;// = nullptr;
          size_t index;// = 0;
          size_t tag;// = 0;

          size_t encode();
          static cas_struct decode(size_t);

          void print() const
          {
            //std::cerr
              //<< node << '\t'
              //<< index << '\t'
              //<< tag << '\n';
          }


        };

		std::atomic<size_t> top_code;
		std::atomic<size_t> bottom_code;

        inline bool indicateEmpty(const cas_struct& top);

      };

      template<class T, size_t S>
      deque<T,S>::deque()
      {
        cas_struct top, bottom;
        auto nodeA = new deque_node();
        auto nodeB = new deque_node();
        nodeA->next = nodeB;
        nodeB->prev = nodeA;
        cas_struct cas;
        cas.node = nodeA;
        cas.index = deque_node::array_size-1;
        const auto code = cas.encode();
        top_code.store(code);
        bottom_code.store(code);
      }

      template<class T, size_t S>
      void deque<T,S>::push_bottom(pointer_type ptr)
      {
        auto bottom = cas_struct::decode(bottom_code.load());
        bottom.node->store(bottom.index, ptr);
        if (bottom.index != 0) {
          bottom.index--;
        } else {
          auto new_node = new deque_node();
          new_node->next = bottom.node;
          bottom.node->prev = new_node;
          bottom.node = new_node;
          bottom.index = deque_node::array_size-1;
        }
        bottom_code.store(bottom.encode());
      }

      template<class T, size_t S>
      auto deque<T,S>::pop_top() -> std::pair<bool, pointer_type>
      {
        auto cur_top_code = top_code.load();
        const auto top = cas_struct::decode(cur_top_code);
        if (indicateEmpty(top)) {
          if (cur_top_code == top_code.load()) {
            return {true, nullptr}; //EMPTY
          } else {
            return {false, nullptr};//ABORT;
          }
        } else { // not empty
          cas_struct new_top;
          deque_node* node_to_free = nullptr;
          if (top.index != 0 ) {
            new_top = top;
            new_top.index--;
          } else {
            node_to_free = top.node->next;
            new_top.tag = top.tag+1;
            new_top.node = top.node->prev;
            new_top.index = deque_node::array_size-1;
          }
          auto result = top.node->load(top.index);
          //assert(new_top.encode() != cur_top_code);
          if (top_code.compare_exchange_strong(cur_top_code, new_top.encode())) {
            delete node_to_free;
            if ((size_t) result == 0xDEADBEEF) {
              //std::cerr << __func__ << deque_node::array_size << '\t' << cas_struct::tag_mask << "t\n";
              top.print();
              new_top.print();
            }
			while ((size_t) result == 0xDEADBEEF);
            return {true, result};
          } else {
            return {false, nullptr};//ABORT;
          }
        }
      }

      template<class T, size_t S>
      auto deque<T,S>::pop_bottom() -> pointer_type
      {
        const auto cur_bottom_code = bottom_code.load();
        const auto bottom = cas_struct::decode(cur_bottom_code);
        cas_struct new_bottom;
        if (bottom.index != deque_node::array_size-1)
        {
          new_bottom.node = bottom.node;
          new_bottom.index = bottom.index+1;
        } else {
          new_bottom.node = bottom.node->next;
          new_bottom.index = 0;
        }
        const auto new_bottom_code = new_bottom.encode();
        bottom_code.store(new_bottom_code);
        auto cur_top_code = top_code.load();
        const auto top = cas_struct::decode(cur_top_code);
        auto result = new_bottom.node->load(new_bottom.index);
        if (   bottom.node == top.node
            && bottom.index == top.index)
        {
          bottom_code.store(cur_bottom_code);
          return nullptr;//EMPTY;
        }
        if (   new_bottom.node == top.node
            && new_bottom.index == top.index) {
          auto new_top = top;
          new_top.tag++;
          const auto new_top_code = new_top.encode();
          if (!top_code.compare_exchange_strong(cur_top_code, new_top_code)) {
            bottom_code.store(cur_bottom_code);
            return nullptr;//EMPTY;
          }
        }
        if (bottom.node != new_bottom.node) {
          delete bottom.node;
        }
        if ((size_t) result == 0xDEADBEEF) {
          //std::cerr << __func__ << deque_node::array_size << "t\n";
          top.print();
          bottom.print();
          new_bottom.print();
        }
        while ((size_t) result == 0xDEADBEEF);
        return result;
      }

      template<class T, size_t S>
      inline bool deque<T,S>::indicateEmpty(const cas_struct& top)
      {
        auto bottom = cas_struct::decode(bottom_code.load());
        if (  (bottom.node == top.node)
           && (  bottom.index == top.index
              || bottom.index == top.index+1)) {
          return true;
        }
        if ( (bottom.node == top.node->next)
           && (bottom.index == 0)
           && (top.index == deque_node::array_size-1)) {
          return true;
        }
        return false;
      }



      template<class T, size_t S>
      size_t deque<T,S>::cas_struct::encode()
      {
        auto result = reinterpret_cast<size_t>(this->node);
        result |= this->index << index_offset;
        result |= this->tag & tag_mask;
#ifndef NDEBUG
        //auto rev = cas_struct::decode(result);
        //assert(this->index == rev.index);
        //assert(this->node == rev.node);
        //assert((this->tag & tag_mask) == rev.tag);
#endif
        return result;
      }

      template<class T, size_t S>
      auto deque<T,S>::cas_struct::decode(size_t code) -> cas_struct
      {
        cas_struct result;
        result.node = reinterpret_cast<deque_node*>(~index_mask & code);
        result.index = (code & index_mask) >> index_offset;
        result.tag = code & tag_mask;
        //assert(result.encode() == code);
        return result;
      }


} //hlms
