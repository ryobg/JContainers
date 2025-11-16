#pragma once

#include <atomic>

namespace boost {
    namespace serialization {

        template<class Archive, class T>
        inline void save_atomic(Archive& ar, const std::atomic<T>& v) {
            T refCnt = v;
            ar & refCnt;
        }

        template<class Archive, class T>
        inline void load_atomic(Archive& ar, std::atomic<T> & v) {
            T refCnt = (T)0;
            ar & refCnt;
            v = refCnt;
        }
    }
}
