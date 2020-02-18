/** @file Simple command line argument parsing library */

#include <string>
#include <sstream>
#include <map>

namespace program_options {

    template <typename T>
    class optional {
        T val_;
        bool valid_;
        
        public:
        explicit optional(const T& val): val_(val), valid_(true) {}
        optional(): val_(), valid_(false) {}
        
        explicit operator bool() const
        {
            return valid_;
        }

        bool operator !() const
        {
            return !bool(*this);
        }
        
        const T& operator*() const
        {
            if (!valid_) throw std::runtime_error("dereferencing an empty optional");
            return val_;
        }

        const T* operator->() const
        {
            if (!valid_) throw std::runtime_error("dereferencing an empty optional");
            return &val_;
        }
    };

    namespace detail {
        template <typename T>
        optional<T> make_optional(const T& val)
        {
            return optional<T>(val);
        }

        template <typename T>
        optional<T> try_lexical_cast(const std::string& s) {
            std::istringstream strm(s);
            noskipws(strm);
            T val;
            strm >> val;
            if (strm && strm.eof()) return make_optional(val);
            return optional<T>();
        }

        template <>
        optional<bool> try_lexical_cast(const std::string& s) {
            for (const auto* cval: {"false","FALSE","off","OFF","no","NO","0"}) {
                if (cval==s) return detail::make_optional(false);
            }
            for (const auto* cval: {"true","TRUE","on","ON","yes","YES","1"}) {
                if (cval==s) return detail::make_optional(true);
            }
            return optional<bool>();
        }
        
    }
    
    
    class params_map {
        using map_type = std::map<std::string,std::string>;
        map_type map_;
        
      public:
        // using parsed_type=optional<params_map>;
        
        template <typename T>
        optional<T> get(const std::string& key) const
        {
            auto it = map_.find(key);
            if (it == map_.end()) return optional<T>();
            auto maybe_val = detail::try_lexical_cast<T>(it->second);
            return maybe_val;
        }

        template <typename T>
        optional<T> get_or(const std::string& key, const T& deflt) const
        {
            auto it = map_.find(key);
            if (it == map_.end()) return detail::make_optional(deflt);
            auto maybe_val = detail::try_lexical_cast<T>(it->second);
            return maybe_val;
        }

        optional<std::string> get_or(const std::string& key, const char* deflt) const
        {
            return get_or(key, std::string(deflt));
        }

        friend optional<params_map> parse(int argc, const char* const* argv);
    };

    
    inline optional<params_map> parse(int argc, const char* const* argv) {
        using std::string;
        using return_type = optional<params_map>;
        params_map p;
        for (int i=1; i<argc; ++i) {
            std::string arg{argv[i]};
            auto eq_pos=arg.find('=');
            if (eq_pos==string::npos || eq_pos<1) {
                return return_type();
            }
            p.map_[arg.substr(0, eq_pos)] = arg.substr(eq_pos+1);
        }
        return return_type(p);
    }

}
