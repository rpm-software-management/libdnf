%module conf

%include <stdint.i>
%include <std_map.i>
%include <std_pair.i>
%include <std_vector.i>
%include <std_string.i>

%begin %{
    #define SWIG_PYTHON_2_UNICODE
%}

%{
    // make SWIG wrap following headers
    #include <iterator>
    #include "libdnf/conf/ConfigRepo.hpp"
    #include "libdnf/conf/ConfigParser.hpp"
    using namespace libdnf;
%}

%include <exception.i>
%exception {
    try {
        $action
    }
    catch (const std::exception & e)
    {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

#define final

// make SWIG look into following headers
%ignore libdnf::Option::Exception;
%ignore libdnf::Option::InvalidValue;
%ignore libdnf::Option::ValueNotSet;
%include "libdnf/conf/Option.hpp"
%include "libdnf/conf/OptionChild.hpp"
%include "libdnf/conf/OptionBool.hpp"
%include "libdnf/conf/OptionEnum.hpp"
%template(OptionEnumString) libdnf::OptionEnum<std::string>;
%include "libdnf/conf/OptionNumber.hpp"
%template(OptionNumberInt32) libdnf::OptionNumber<std::int32_t>;
%template(OptionNumberUInt32) libdnf::OptionNumber<std::uint32_t>;
%template(OptionNumberInt64) libdnf::OptionNumber<std::int64_t>;
%template(OptionNumberUInt64) libdnf::OptionNumber<std::uint64_t>;
%template(OptionNumberFloat) libdnf::OptionNumber<float>;
%include "libdnf/conf/OptionSeconds.hpp"
%include "libdnf/conf/OptionString.hpp"
%include "libdnf/conf/OptionStringList.hpp"
%include "libdnf/conf/OptionPath.hpp"

%template(OptionChildBool) libdnf::OptionChild<OptionBool>;
%template(OptionChildString) libdnf::OptionChild<OptionString>;
%template(OptionChildStringList) libdnf::OptionChild<OptionStringList>;
%template(OptionChildNumberInt32) libdnf::OptionChild< OptionNumber<std::int32_t> >;
%template(OptionChildNumberUInt32) libdnf::OptionChild< OptionNumber<std::uint32_t> >;
%template(OptionChildNumberFloat) libdnf::OptionChild< OptionNumber<float> >;
%template(OptionChildEnumString) libdnf::OptionChild< OptionEnum<std::string> >;
%template(OptionChildSeconds) libdnf::OptionChild<OptionSeconds>;
%template(PairStringString) std::pair<std::string, std::string>;
%template(MapStringString) std::map<std::string, std::string>;

%include <std_vector_ext.i>

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

%feature ("flatnested", "1");
%rename (OptionBinds_Item) libdnf::OptionBinds::Item;
%ignore libdnf::OptionBinds::Exception;
%ignore libdnf::OptionBinds::OutOfRange;
%ignore libdnf::OptionBinds::AlreadyExists;
%ignore libdnf::OptionBinds::add(const std::string & id, Option & option,
    const Item::NewStringFunc & newString, const Item::GetValueStringFunc & getValueString, bool addValue);
%ignore libdnf::OptionBinds::add(const std::string & id, Option & option,
    Item::NewStringFunc && newString, Item::GetValueStringFunc && getValueString, bool addValue);
%ignore libdnf::OptionBinds::begin;
%ignore libdnf::OptionBinds::cbegin;
%ignore libdnf::OptionBinds::end;
%ignore libdnf::OptionBinds::cend;
%ignore libdnf::OptionBinds::find;
%include "libdnf/conf/OptionBinds.hpp"
%feature ("flatnested", "0");

%include "libdnf/conf/Config.hpp"
%include "libdnf/conf/ConfigMain.hpp"
%include "libdnf/conf/ConfigRepo.hpp"

%template(OptionBindsIterator) Iterator<libdnf::OptionBinds>;

%exception libdnf::ConfigParser::read {
    try
    {
        $action
    }
    catch (const libdnf::ConfigParser::CantOpenFile & e)
    {
        SWIG_exception(SWIG_IOError, e.what());
    }
    catch (const libdnf::ConfigParser::Exception & e)
    {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
    catch (const std::exception & e)
    {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

%exception libdnf::ConfigParser::getValue {
    try
    {
        $action
    }
    catch (const libdnf::ConfigParser::MissingSection & e)
    {
        SWIG_exception(SWIG_IndexError, e.what());
    }
    catch (const libdnf::ConfigParser::MissingOption & e)
    {
        SWIG_exception(SWIG_IndexError, e.what());
    }
    catch (const libdnf::ConfigParser::Exception & e)
    {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
    catch (const std::exception & e)
    {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

%exception libdnf::ConfigParser::getSubstitutedValue {
    try
    {
        $action
    }
    catch (const libdnf::ConfigParser::MissingSection & e)
    {
        SWIG_exception(SWIG_IndexError, e.what());
    }
    catch (const libdnf::ConfigParser::MissingOption & e)
    {
        SWIG_exception(SWIG_IndexError, e.what());
    }
    catch (const libdnf::ConfigParser::Exception & e)
    {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
    catch (const std::exception & e)
    {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

%apply std::string & INOUT { std::string & text }
%ignore libdnf::ConfigParser::Exception;
%ignore libdnf::ConfigParser::CantOpenFile;
%ignore libdnf::ConfigParser::ParsingError;
%ignore libdnf::ConfigParser::MissingSection;
%ignore libdnf::ConfigParser::MissingOption;
%include "libdnf/conf/ConfigParser.hpp"
%clear std::string & text;
%template(MapStringMapStringString) std::map<std::string, std::map<std::string, std::string>>;

%exception __next__() {
    try
    {
        $action  // calls %extend function next() below
    }
    catch (const StopIterator &)
    {
        PyErr_SetString(PyExc_StopIteration, "End of iterator");
        return NULL;
    }
    catch (const std::exception & e)
    {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

// For old Python
%exception next() {
    try
    {
        $action  // calls %extend function next() below
    }
    catch (const StopIterator &)
    {
        PyErr_SetString(PyExc_StopIteration, "End of iterator");
        return NULL;
    }
    catch (const std::exception & e)
    {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

%template(PairStringOptionBindsItem) std::pair<std::string, libdnf::OptionBinds::Item *>;
%extend Iterator<libdnf::OptionBinds> {
    std::pair<std::string, libdnf::OptionBinds::Item *> __next__()
    {
        if ($self->cur != $self->end) {
            auto & id = $self->cur->first;
            auto pValue = &($self->cur++)->second;
            return {id, pValue};
        }
        throw StopIterator();
    }
    std::pair<std::string, libdnf::OptionBinds::Item *> next()
    {
        if ($self->cur != $self->end) {
            auto & id = $self->cur->first;
            auto pValue = &($self->cur++)->second;
            return {id, pValue};
        }
        throw StopIterator();
    }
}

%exception libdnf::OptionBinds::__getitem__ {
    try
    {
        $action
    }
    catch (const libdnf::OptionBinds::OutOfRange & e)
    {
        SWIG_exception(SWIG_IndexError, e.what());
    }
    catch (const std::exception & e)
    {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

%extend libdnf::OptionBinds {
    Item & __getitem__(const std::string & id)
    {
        return $self->at(id);
    }

    size_t __len__()
    {
        return $self->size();
    }

    Iterator<libdnf::OptionBinds> __iter__()
    {
        return Iterator<libdnf::OptionBinds>($self->begin(), $self->end());
    }
}

%pythoncode %{
# Compatible name aliases
ConfigMain.exclude = ConfigMain.excludepkgs
ConfigRepo.exclude = ConfigRepo.excludepkgs
%}
