template<class E> class LL {
    //static LL<E>   l;
    static LL<E>& l();

};

class A {
};

//template<typename A>  
//typename <class A>LL LL<A>::l;
template <typename E>
LL<E>& LL<E>::l() { static LL<E> rc; return rc; }
