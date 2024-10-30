#ifndef G_MISC_STD_EXTENSIONS_UTILITY_H
#define G_MISC_STD_EXTENSIONS_UTILITY_H

#include <iostream>
#include <vector>


template<class ElementType, class Allocator>
std::ostream& operator<< (std::ostream& os, const std::vector<ElementType, Allocator> v) {
    os << "[";
    if (v.size() >= 1) os << " " << v[0];
    for (int i = 1 ; i < v.size() ; ++i) {
        os << ", " << v[i];
    }
    os << " ]";
    return os;
}


template<typename T>
std::ostream& operator<<(std::ostream& os, OneLink<T> *link){

    os << "{";
    if (link) {
        os << link->value ;
        while (link = link->next) {
            os << ", " << link->value;
        }
    }
    os << " }";
    return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, OneLink<T>& link){

    os << "{";

    os << link.value ;
    OneLink<T> *cur = &link;
    while (cur = cur->next) {
        os << ", " << cur->value;
    }

    os << " }";
    return os;
}

template<class T, int N>
std::ostream& operator<< (std::ostream& os, T v[N]) {
    os << "[";
    if (N >= 1) os << " " << v[0];
    for (int i = 1 ; i < N ; ++i) {
        os << ", " << v[i];
    }
    os << " ]";
    return os;
}

#endif //#ifdef G_MISC_STD_EXTENSIONS_UTILITY_H
