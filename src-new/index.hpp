//
// Helper class for constructing indexed sets.
//


#ifndef INDEX_H_
#define INDEX_H_

#include <cereal/types/memory.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/string.hpp>

using namespace std;

struct Index {
    Index() : cur_index(0) {}

    int get_or_add(string item) {
        if (fmap.find(item) != fmap.end()) {
            return fmap[item];
        } else {
            fmap[item] = cur_index;
            rmap[cur_index] = item;
            cur_index++;
            return cur_index - 1;
        }
    }

    int index(string item) const {
        return fmap.at(item);
    }

    string get_string(int index) const {
        return rmap.at(index);
    }

    int size() const {
        return cur_index;
    }

    template <class Archive>
    void serialize(Archive &ar) {
        ar(fmap, rmap, cur_index);
    }

  private:
    int cur_index;
    unordered_map<string, int> fmap;
    unordered_map<int, string> rmap;
};

#endif  // INDEX_H_
