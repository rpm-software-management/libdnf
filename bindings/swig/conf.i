%module conf

%include <exception.i>
%include <stdint.i>
%include <std_map.i>
%include <std_pair.i>

%import(module="libdnf.common_types") "common_types.i"

%begin %{
    #define SWIG_PYTHON_2_UNICODE
%}

%{
    // make SWIG wrap following headers
    #include "libdnf/conf/ConfigRepo.hpp"
    #include "libdnf/conf/ConfigParser.hpp"
    #include <iterator>
    #include <memory>
    #include <sstream>
%}

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

%template(OptionChildBool) libdnf::OptionChild<libdnf::OptionBool>;
%template(OptionChildString) libdnf::OptionChild<libdnf::OptionString>;
%template(OptionChildStringList) libdnf::OptionChild<libdnf::OptionStringList>;
%template(OptionChildNumberInt32) libdnf::OptionChild<libdnf::OptionNumber<std::int32_t>>;
%template(OptionChildNumberUInt32) libdnf::OptionChild<libdnf::OptionNumber<std::uint32_t>>;
%template(OptionChildNumberFloat) libdnf::OptionChild<libdnf::OptionNumber<float>>;
%template(OptionChildEnumString) libdnf::OptionChild<libdnf::OptionEnum<std::string>>;
%template(OptionChildSeconds) libdnf::OptionChild<libdnf::OptionSeconds>;

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
namespace libdnf {
struct ConfigParser {
public:
    typedef PreserveOrderMap<std::string, PreserveOrderMap<std::string, std::string>> Container;

    static void substitute(std::string & text,
        const std::map<std::string, std::string> & substitutions);
    void setSubstitutions(const std::map<std::string, std::string> & substitutions);
    const std::map<std::string, std::string> & getSubstitutions() const;
    void read(const std::string & filePath);
    void write(const std::string & filePath, bool append) const;
    void write(const std::string & filePath, bool append, const std::string & section) const;
    void write(std::ostream & outputStream, const std::string & section) const;
    void write(std::ostream & outputStream) const;
    bool addSection(const std::string & section, const std::string & rawLine);
    bool addSection(const std::string & section);
    bool hasSection(const std::string & section) const noexcept;
    bool hasOption(const std::string & section, const std::string & key) const noexcept;
    void setValue(const std::string & section, const std::string & key, const std::string & value, const std::string & rawItem);
    void setValue(const std::string & section, const std::string & key, const std::string & value);
    bool removeSection(const std::string & section);
    bool removeOption(const std::string & section, const std::string & key);
    void addCommentLine(const std::string & section, const std::string & comment);
    const std::string & getValue(const std::string & section, const std::string & key) const;
    std::string getSubstitutedValue(const std::string & section, const std::string & key) const;
    const std::string & getHeader() const noexcept;
    std::string & getHeader() noexcept;
    const Container & getData() const noexcept;
    Container & getData() noexcept;
};
}
%clear std::string & text;

%extend libdnf::ConfigParser {
    void readString(const std::string & content) {
        std::unique_ptr<std::istringstream> istream(new std::istringstream(content));
        self->read(std::move(istream));
    }
}

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
# Partial compatibility with Python ConfigParser
ConfigParser.readFileName = ConfigParser.read
def ConfigParser__newRead(self, filenames):
    parsedFNames = []
    try:
        if isinstance(filenames, str) or isinstance(filenames, unicode):
            filenames = [filenames]
    except NameError:
        pass
    for fname in filenames:
        try:
            self.readFileName(fname)
            parsedFNames.append(fname)
        except IOError:
            pass
        except Exception as e:
            raise RuntimeError("Parsing file '%s' failed: %s" % (fname, str(e)))
    return parsedFNames
ConfigParser.read = ConfigParser__newRead
del ConfigParser__newRead

def ConfigParser__read_string(self, string, source='<string>'):
    try:
        self.readString(string)
    except Exception as e:
        raise RuntimeError("Parsing source '%s' failed: %s" % (source, str(e)))
ConfigParser.read_string = ConfigParser__read_string
del ConfigParser__read_string

def ConfigParser__add_section(self, section):
    if not self.addSection(section):
        raise KeyError("Section '%s' already exists" % section)
ConfigParser.add_section = ConfigParser__add_section
del ConfigParser__add_section

ConfigParser.has_section = ConfigParser.hasSection
ConfigParser.has_option = ConfigParser.hasOption

def ConfigParser__get(self, section, option, raw=False):
    try:
        if raw:
            return self.getValue(section, option)
        else:
            return self.getSubstitutedValue(section, option)
    except IndexError as e:
        raise KeyError(str(e))

ConfigParser.get = ConfigParser__get
del ConfigParser__get

def ConfigParser__getint(self, section, option, raw=False):
    return int(self.get(section, option, raw=raw))
ConfigParser.getint = ConfigParser__getint
del ConfigParser__getint

def ConfigParser__getfloat(self, section, option, raw=False):
    return float(self.get(section, option, raw=raw))
ConfigParser.getfloat = ConfigParser__getfloat
del ConfigParser__getfloat

ConfigParser._boolean_states = {'1': True, 'yes': True, 'true': True, 'on': True,
                                '0': False, 'no': False, 'false': False, 'off': False}
def ConfigParser__getboolean(self, section, option, raw=False):
    v = self.get(section, option, raw=raw)
    if v.lower() not in self._boolean_states:
        raise ValueError('Not a boolean: %s' % v)
    return self._boolean_states[v.lower()]
ConfigParser.getboolean = ConfigParser__getboolean
del ConfigParser__getboolean

def ConfigParser__set(self, section, option, value):
    if not self.hasSection(section):
        raise KeyError("No section: '%s'" % section)
    self.setValue(section, option, value)
ConfigParser.set = ConfigParser__set
del ConfigParser__set

ConfigParser.remove_section = ConfigParser.removeSection

def ConfigParser__remove_option(self, section, option):
    if not self.hasSection(section):
        raise KeyError("No section: '%s'" % section)
    return self.removeOption(section, option)
ConfigParser.remove_option = ConfigParser__remove_option
del ConfigParser__remove_option

def ConfigParser__options(self, section):
    if not self.hasSection(section):
        raise KeyError("No section: '%s'" % section)
    sectObj = self.getData()[section]
    return [item for item in sectObj if not item.startswith('#')]
ConfigParser.options = ConfigParser__options
del ConfigParser__options

def ConfigParser__sections(self):
    return list(self.getData())
ConfigParser.sections = ConfigParser__sections
del ConfigParser__sections

# Compatible name aliases
ConfigMain.exclude = ConfigMain.excludepkgs
ConfigRepo.exclude = ConfigRepo.excludepkgs
%}
