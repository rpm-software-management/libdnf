%module common_types

%begin %{
    #define SWIG_PYTHON_2_UNICODE
%}

%include <stdint.i>
%include <std_map.i>
%include <std_pair.i>
%include <std_set.i>
%include <std_string.i>
%include <std_vector.i>

%template(SetString) std::set<std::string>;
%template(PairStringString) std::pair<std::string, std::string>;
%template(VectorPairStringString) std::vector<std::pair<std::string, std::string>>;
%template(MapStringString) std::map<std::string, std::string>;
%template(MapStringMapStringString) std::map<std::string, std::map<std::string, std::string>>;
%template(MapStringPairStringString) std::map<std::string, std::pair<std::string, std::string>>;

%{
    #include "libdnf/utils/PreserveOrderMap.hpp"
%}

%include <exception.i>
%exception {
    try {
        $action
    }
    catch (const std::out_of_range  & e)
    {
        SWIG_exception(SWIG_IndexError, e.what());
    }
    catch (const std::exception & e)
    {
       SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

%ignore libdnf::PreserveOrderMap::MyBidirIterator;
%ignore libdnf::PreserveOrderMap::MyBidirIterator::operator++;
%ignore libdnf::PreserveOrderMap::MyBidirIterator::operator--;
%ignore libdnf::PreserveOrderMap::begin;
%ignore libdnf::PreserveOrderMap::end;
%ignore libdnf::PreserveOrderMap::cbegin;
%ignore libdnf::PreserveOrderMap::cend;
%ignore libdnf::PreserveOrderMap::rbegin;
%ignore libdnf::PreserveOrderMap::rend;
%ignore libdnf::PreserveOrderMap::crbegin;
%ignore libdnf::PreserveOrderMap::crend;
%ignore libdnf::PreserveOrderMap::insert;
%ignore libdnf::PreserveOrderMap::erase(const_iterator pos);
%ignore libdnf::PreserveOrderMap::erase(const_iterator first, const_iterator last);
%ignore libdnf::PreserveOrderMap::count;
%ignore libdnf::PreserveOrderMap::find;
%ignore libdnf::PreserveOrderMap::operator[];
%ignore libdnf::PreserveOrderMap::at;
%include "libdnf/utils/PreserveOrderMap.hpp"

%inline %{
class StopIterator {};

template<class T>
class Iterator {
public:
    Iterator(typename T::iterator _cur, typename T::iterator _end) : cur(_cur), end(_end) {}
    Iterator* __iter__()
    {
      return this;
    }

    typename T::iterator cur;
    typename T::iterator end;
  };
%}

%define EXTEND_TEMPLATE_PreserveOrderMap(ReturnT, Key, T...)
    %extend libdnf::PreserveOrderMap<Key, T> {
        ReturnT __getitem__(const Key & key)
        {
            return $self->at(key);
        }

        void __setitem__(const Key & key, const T & value)
        {
            $self->operator[](key) = value;
        }

        void __delitem__(const Key & key)
        {
            if ($self->erase(key) == 0)
                throw std::out_of_range("PreserveOrderMap::__delitem__");
        }

        bool __contains__(const Key & key) const
        {
            return $self->count(key) > 0;
        }

        size_t __len__() const
        {
            return $self->size();
        }
    }
%enddef
EXTEND_TEMPLATE_PreserveOrderMap(T, std::string, std::string)
EXTEND_TEMPLATE_PreserveOrderMap(T &, std::string, libdnf::PreserveOrderMap<std::string, std::string>)

%template(PreserveOrderMapStringString) libdnf::PreserveOrderMap<std::string, std::string>;
%template(PreserveOrderMapStringStringIterator) Iterator<libdnf::PreserveOrderMap<std::string, std::string>>;
%template(PreserveOrderMapStringPreserveOrderMapStringString) libdnf::PreserveOrderMap<std::string, libdnf::PreserveOrderMap<std::string, std::string>>;
%template(PreserveOrderMapStringPreserveOrderMapStringStringIterator) Iterator<libdnf::PreserveOrderMap<std::string, libdnf::PreserveOrderMap<std::string, std::string>>>;


%exception __next__() {
    try
    {
        $action  // calls %extend function next() below
    }
    catch (StopIterator)
    {
        PyErr_SetString(PyExc_StopIteration, "End of iterator");
        return NULL;
    }
}

// For old Python
%exception next() {
    try
    {
        $action  // calls %extend function next() below
    }
    catch (StopIterator)
    {
        PyErr_SetString(PyExc_StopIteration, "End of iterator");
        return NULL;
    }
}

%define EXTEND_TEMPLATE_PreserveOrderMapIterator(Key, T...)
%extend Iterator<libdnf::PreserveOrderMap<Key, T>> {
    Key __next__()
    {
        if ($self->cur != $self->end) {
            return ($self->cur++)->first;
        }
        throw StopIterator();
    }
    Key next()
    {
        if ($self->cur != $self->end) {
            return ($self->cur++)->first;
        }
        throw StopIterator();
    }
}

%extend libdnf::PreserveOrderMap<Key, T> {
    Iterator<libdnf::PreserveOrderMap<Key, T>> __iter__()
    {
        return Iterator<libdnf::PreserveOrderMap<Key, T>>($self->begin(), $self->end());
    }
}
%enddef
EXTEND_TEMPLATE_PreserveOrderMapIterator(std::string, std::string)
EXTEND_TEMPLATE_PreserveOrderMapIterator(std::string, libdnf::PreserveOrderMap<std::string, std::string>)

%exception;
