
#pragma once

#include <cstring>
#include <utility>
#include <vector>
#include <memory>
#include <string>
#include <tuple>
#include <sstream>
#include <optional>
#include <functional>
#include <type_traits>
#include <map>

namespace po
{
    enum class ParseStatus
    {
        NoMatch, Match, HelpParsed
    };
    namespace detail
    {
        namespace helper
        {
            template <class T>
            constexpr T lexical_cast(const std::string_view sv)
            {
                T result;
                if constexpr (std::is_arithmetic_v<T>)
                {
                    std::istringstream ss;
                    ss.str(std::string(sv));
                    ss >> result;
                }
                else
                {
                    result = static_cast<T>(sv);
                }
                return result;
            }

            template <class T, class Tag_t>
            class named_type
            {
            public:
                using type_t = T;

                named_type(T v)
                    : _value(v)
                {}
                operator T()
                {
                    return _value;
                }

            private:
                T _value;
            };

            template <class T, class Tuple>
            struct has_type;
            template <class T>
            struct has_type<T, std::tuple<>> : std::false_type {};
            template <class T, class U, class... Ts>
            struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>> {};
            template <class T, class... Ts>
            struct has_type<T, std::tuple<T, Ts...>> : std::true_type {};

            template<const char* ErrorMsg, class ParamT, class... Types>
            typename ParamT::type_t pick_option(Types&&... args)
            {
                auto t = std::make_tuple(std::forward<Types>(args)...);
                //static_assert(has_type<ParamT, decltype(t)>::value, ErrorMsg);
                return std::get<ParamT>(t);
            }
            template<class ParamT, class... Types>
            typename ParamT::type_t pick_option_with_default(const typename ParamT::type_t& def, Types&&... args)
            {
                auto t = std::make_tuple(std::forward<Types>(args)...);
                if constexpr (has_type<ParamT, decltype(t)>::value)
                {
                    return std::get<ParamT>(t);
                }
                return def;
            }
        }
        class base_sub_program
        {
        public:
            virtual int
                operator()() = 0;
            virtual bool
                parsed() const = 0;
        };
        class base_option
        {
        public:
            base_option(const base_option* parent, std::string_view long_name, char short_name, std::string_view pattern)
                : _parent(parent)
                , _long_name(long_name)
                , _short_name(short_name)
                , _pattern(pattern)
            {
                if (long_name == "" && short_name == 0)
                {
                    throw std::runtime_error("po error: no option name given");
                }
            }
            virtual ParseStatus
                try_parse_option(int* argc, const char*** argv)
            {
                ParseStatus result = ParseStatus::NoMatch;
                if (_pattern == "")
                {
                    const char* on = **argv;
                    if (on[0] == 0 || on[1] == 0)
                    {
                        throw std::runtime_error("po error: invalid option");
                    }
                    if (on[0] != '-')
                    {
                        if (on[1] == 0 && _short_name != 0 && on[0] == _short_name)
                        {
                            result = ParseStatus::Match;
                        }
                        else if (_long_name != "" && std::strcmp(on, _long_name.data()) == 0)
                        {
                            result = ParseStatus::Match;
                        }
                    }
                    else if (on[1] != '-' && _short_name != 0 && on[1] == _short_name)
                    {
                        result = ParseStatus::Match;
                    }
                    else if (_long_name != "")
                    {
                        auto iter = std::find(on + 2, on + std::strlen(on), '=');
                        if (std::strncmp(on + 2, _long_name.data(), iter - on - 2) == 0)
                        {
                            result = ParseStatus::Match;
                        }
                    }
                    if (result == ParseStatus::Match)
                    {
                        _parsed_argument = std::string_view(**argv);
                        (*argc)--;
                        (*argv)++;
                    }
                }
                else
                {
                    auto iter = std::find(_pattern.begin(), _pattern.end(), '*');
                    auto count_beg = std::distance(_pattern.begin(), iter);
                    auto count_end = std::distance(iter + 1, _pattern.end());
                    auto len = std::strlen(**argv);
                    bool cmp1 = std::strncmp(&_pattern[0], (**argv) + 2, count_beg) == 0;
                    bool cmp2 = true;
                    for (ptrdiff_t i = 0; i < count_end; i++)
                    {
                        if (*(iter + 1 + i) != *((**argv) + len - count_end + i))
                        {
                            cmp2 = false;
                            break;
                        }
                    }
                    if (cmp1 && cmp2)
                    {
                        auto begin = (**argv) + count_beg + 2;
                        auto end = (**argv) + len - count_end;
                        auto iter = std::find(begin, end, '=');
                        _parsed_argument = std::string_view(**argv);
                        _parsed_pattern_argument = std::string_view(begin, iter - begin);
                        (*argc)--;
                        (*argv)++;
                        result = ParseStatus::Match;
                    }
                }
                return result;
            }
            std::string_view
                long_name() const
            {
                return _long_name;
            }
            char
                short_name() const
            {
                return _short_name;
            }
            std::string_view
                name() const
            {
                return _long_name == "" ? std::string_view(&_short_name) : _long_name;
            }
            virtual bool
                notify() const = 0;
            bool
                parsed() const
            {
                return _parsed_argument != "";
            }
            bool
                parsed_as_group() const
            {
                return _parsed_argument.size() > 0 && _parsed_argument[0] != '-';
            }
            std::string_view
                parsed_argument() const
            {
                return _parsed_argument;
            }
            void
                set_parsed_argument(std::string_view parsed_argument)
            {
                _parsed_argument = parsed_argument;
            }
            std::string_view
                parsed_pattern_argument() const
            {
                return _parsed_pattern_argument;
            }
            std::string
                full_qualified_name() const
            {
                std::string result;
                std::vector<const base_option*> hist;
                const base_option* cur = _parent;
                hist.push_back(this);
                while (cur != nullptr)
                {
                    hist.push_back(cur);
                    cur = cur->_parent;
                }
                for (auto iter = hist.rbegin() + 1; iter != hist.rend() - 1; iter++)
                {
                    result += std::string((*iter)->name()) + "::";
                }
                result += std::string(name());
                return result;
            }

        private:
            const base_option* _parent;
            char _short_name{0};
            std::string_view _long_name{0};
            std::string_view _pattern;
            std::string_view _parsed_argument;
            std::string_view _parsed_pattern_argument;
        };
        class base_group
            : public base_option
        {
        public:
            using base1_t = base_option;

            base_group(const base_option* parent, std::string_view name, char short_name, bool optional)
                : base1_t(parent, name, short_name, "")
                , _optional(optional)
            {}
            void
                register_option(base_option* bo)
            {
                if (dynamic_cast<base_group*>(bo))
                {
                    if (_positional_argument != nullptr)
                    {
                        throw std::runtime_error("po error: can not add group \"" + std::string(bo->name()) +
                            "\" to group \"" + std::string(name()) + "\" which already holds a positional argument");
                    }
                    _has_group = true;
                }
                _options.push_back(bo);
            }
            void
                set_help_option(base_option* help)
            {
                if (_help != nullptr)
                {
                    throw std::runtime_error("po error: tryed to set help for \"" + std::string(name()) + "\" twice");
                }
                _help = help;
            }
            void
                set_sub_program(base_sub_program* sp)
            {
                if (_sub_program != nullptr)
                {
                    throw std::runtime_error("po error: tryed to set sub program for \"" + std::string(name()) + "\" twice");
                }
                _sub_program = sp;
            }
            void
                set_positional_argument(base_option* bo)
            {
                if (_positional_argument != nullptr)
                {
                    throw std::runtime_error("po error: tryed to set positional for \"" + std::string(name()) + "\" twice");
                }
                if (_has_group)
                {
                    throw std::runtime_error("po error: can not add positional argument \"" + std::string(bo->name()) +
                        "\" to group \"" + std::string(name()) + "\" which already holds a group");
                }
                _positional_argument = bo;
            }
            virtual ParseStatus
                try_parse_option(int* argc, const char*** argv) override
            {
                auto result = base1_t::try_parse_option(argc, argv);
                if (result == ParseStatus::Match)
                {
                    if (!parsed_as_group())
                    {
                        throw std::runtime_error("po error: ambigious argument \"" + std::string(name()) + "\"");
                    }
                    if (argc == 0)
                    {
                        throw std::runtime_error("po error: parsing program option failed, no more arguments given");
                    }
                    bool parsed = true;
                    bool group_parsed = false;
                    bool matched = false;
                    std::size_t parsed_counter = 0;
                    while (parsed)
                    {
                        parsed = false;
                        for (auto* op : options())
                        {
                            auto ret = op->try_parse_option(argc, argv);
                            if (ret == ParseStatus::Match)
                            {
                                matched = true;
                                if (!op->parsed_as_group() && *argc != 0)
                                {
                                    parsed = true;
                                }
                                parsed_counter++;
                                break;
                            }
                            else if (ret == ParseStatus::HelpParsed)
                            {
                                parsed= false;
                                result = ret;
                                break;
                            }
                        }
                    }
                    if (result != ParseStatus::HelpParsed && !group_parsed && parsed_counter != 0)
                    {
                        result = parse_positional(argc, argv);
                    }
                }
                return result;
            }
            std::optional<int>
                execute_main()
            {
                std::optional<int> result = std::nullopt;
                if (_sub_program != nullptr)
                {
                    result = (*_sub_program)();
                }
                return result;
            }
            const std::vector<base_option*>&
                options() const
            {
                return _options;
            }
            bool
                optional() const
            {
                return _optional;
            }
            bool
                help_parsed() const
            {
                bool result = false;
                if (_help != nullptr)
                {
                    result = _help->parsed();
                }
                return result;
            }
            virtual bool
                notify() const override
            {
                bool result = false;
                if (!optional() && !parsed())
                {
                    throw std::runtime_error("po error: could not find option \"" + std::string(name()) + "\"");
                }
                if (parsed())
                {
                    for (const auto* op : options())
                    {
                        result = op->notify();
                        if (result)
                        {
                            break;
                        }
                    }
                }
                return result;
            }

         protected:
            ParseStatus
                parse_positional(int* argc, const char*** argv)
            {
                ParseStatus result = ParseStatus::Match;
                if (_positional_argument != nullptr)
                {
                    result = _positional_argument->try_parse_option(argc, argv);
                }
                return result;
            }

        private:
            std::vector<base_option*> _options;
            base_option* _help;
            base_sub_program* _sub_program{nullptr};
            base_option* _positional_argument{nullptr};
            bool _optional;
            bool _has_group{false};
        };
        class root_group
            : public base_group
        {
        public:
            root_group()
                : base_group(nullptr, "main_group", 0, false)
            {
                set_parsed_argument("main_group");
            }
            virtual ParseStatus
                try_parse_option(int* argc, const char*** argv) override
            {
                ParseStatus result = ParseStatus::NoMatch;
                if (argc == 0)
                {
                    throw std::runtime_error("po error: parsing program option failed, no more arguments given");
                }
                bool parsed = true;
                bool group_parsed = false;
                bool matched = false;
                std::size_t parsed_counter = 0;
                while (parsed)
                {
                    parsed = false;
                    for (auto* op : options())
                    {
                        result = op->try_parse_option(argc, argv);
                        if (result == ParseStatus::Match)
                        {
                            parsed_counter++;
                            matched = true;
                            if (op->parsed_as_group())
                            {
                                if (group_parsed)
                                {
                                    throw std::runtime_error(
                                        "po error: two groups (first: \"" + std::string(name()) + "\" second: \"" +
                                        std::string(op->name()) +  "\") in the same stage are not possible");
                                }
                                group_parsed = true;
                            }
                            if (*argc != 0)
                            {
                                parsed = true;
                            }
                            break;
                        }
                        else if (result == ParseStatus::HelpParsed)
                        {
                            parsed = false;
                            break;
                        }
                    }
                }
                if (result != ParseStatus::HelpParsed && !group_parsed && parsed_counter != 0)
                {
                    result = parse_positional(argc, argv);
                }
                return result;
            }
        };
        class parser
        {
        public:
            static parser& instance()
            {
                return _instance;
            }

            parser()
                : _main_group(std::make_unique<root_group>())
            {}
            void
                register_main_group_option(base_option* bo)
            {
                _main_group->register_option(bo);
            }
            void set_main_group_help_option(base_option* help)
            {
                _main_group->set_help_option(help);
            }
            void
                register_sub_program(base_sub_program* sp)
            {
                _sub_programs.push_back(sp);
            }
            base_group*
                get_main_group()
            {
                return _main_group.get();
            }
            ParseStatus
                parse_command_line(int argc, const char** argv)
            {
                auto ret = _main_group->try_parse_option(&argc, &argv);
                if (ret != ParseStatus::Match &&
                    ret != ParseStatus::HelpParsed)
                {
                    throw std::runtime_error("po error: unkown argument \"" + std::string(*argv) + "\"");
                }
                return ret;
            }
            bool
                notify() const
            {
                return _main_group->notify();
            }
            std::optional<int>
                execute_main()
            {
                std::optional<int> result;
                for (auto* sp : _sub_programs)
                {
                    if (sp->parsed())
                    {
                        result = (*sp)();
                        if (*result != 0)
                        {
                            break;
                        }
                    }
                }
                return result;
            }

        private:
            static parser _instance;
            std::unique_ptr<root_group> _main_group;
            std::vector<base_sub_program*> _sub_programs;
        };
        template <class T>
        class base_argument
            : public detail::base_option
        {
        public:
            using base1_t = detail::base_option;
            using type_t = T;

            base_argument(std::string_view long_name, char short_name, std::size_t min, std::size_t max, std::string_view pattern)
                : base_argument(*detail::parser::instance().get_main_group(), long_name, short_name, min, max, pattern)
            {}
            base_argument(detail::base_group& bg, std::string_view long_name, char short_name, std::size_t min, std::size_t max, std::string_view pattern)
                : detail::base_option(&bg, long_name, short_name, pattern)
                , _min(min)
                , _max(max)
            {
                if (_max < _min)
                {
                    throw std::runtime_error("po error: max < min for option \"" + std::string(name()) + "\"");
                }
                bg.register_option(this);
            }
            std::optional<T>
                try_parse_option_argument(int* argc, const char*** argv)
            {
                std::optional<T> result = std::nullopt;
                auto ret = base1_t::try_parse_option(argc, argv);
                if (ret == ParseStatus::Match)
                {
                    _parse_counter++;
                    if (_parse_counter > _max)
                    {
                        throw std::runtime_error("po error: too many \"" + std::string(name()) +
                            "\" arguments given, max is " + std::to_string(_max));
                    }
                    auto pa = parsed_argument();
                    auto iter = std::find(pa.begin(), pa.end(), '=');
                    std::string_view str_value;
                    if (iter != pa.end())
                    {
                        auto start = std::distance(pa.begin(), iter) + 1;
                        auto count = std::distance(iter + 1, pa.end());
                        str_value = std::string_view(&pa[start], count);
                    }
                    else if (pa[1] == '-' || (pa[1] != '-' && pa[2] == 0))
                    {
                        if (*argc == 0)
                        {
                            throw std::runtime_error("po error: no parameter given for argument option \""
                                + std::string(name()) +  "\"");
                        }
                        str_value = std::string_view(**argv);
                        (*argc)--;
                        (*argv)++;
                    }
                    else
                    {
                        str_value = std::string_view(pa.data() + 2, std::distance(pa.begin() + 2, pa.end()));
                    }
                    result = helper::lexical_cast<T>(str_value);
                }
                return result;
            }
            bool
                parsed() const
            {
                return _parse_counter > 0;
            }
            std::size_t
                parse_counter() const
            {
                return _parse_counter;
            }
            virtual bool
                notify() const override
            {
                if (_min > _parse_counter)
                {
                    throw std::runtime_error("po error: too less \"" + std::string(name()) +
                        "\" arguments given, min is " + std::to_string(_min));
                }
                return false;
            }

        private:
            std::size_t _min{1}, _max{1};
            std::size_t _parse_counter{0};
        };

        struct TagBaseGroup {};
        struct TagLongName {};
        struct TagShortName {};
        struct TagOptional {};
        struct TagMin {};
        struct TagMax {};
        struct TagPattern {};
        template <class T>
        struct TagDef {};
        struct TagHeader {};
        struct TagMessage {};
    }

    using BaseGroup = detail::helper::named_type<detail::base_group&, detail::TagBaseGroup>;
    using LongName = detail::helper::named_type<std::string_view, detail::TagLongName>;
    using ShortName = detail::helper::named_type<char, detail::TagShortName>;
    using Optional = detail::helper::named_type<bool, detail::TagOptional>;
    using Min = detail::helper::named_type<std::size_t, detail::TagMin>;
    using Max = detail::helper::named_type<std::size_t, detail::TagMax>;
    using Pattern = detail::helper::named_type<std::string_view, detail::TagPattern>;
    template <class T>
    using Def = detail::helper::named_type<std::optional<T>, detail::TagDef<T>>;
    using Header = detail::helper::named_type<std::string_view, detail::TagHeader>;
    using Message = detail::helper::named_type<std::string_view, detail::TagMessage>;

    class flag
        : public detail::base_option
    {
    public:
        using base1_t = detail::base_option;
        using type_t = bool;
        
        template <class... Args>
        flag(const Args&... args)
            : base1_t(
                  &detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , "")
        {
            auto& bg = detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...);
            bg.register_option(this);
        }
        virtual ParseStatus
            try_parse_option(int* argc, const char*** argv) override
        {
            auto was_parsed = parsed();
            auto ret = base1_t::try_parse_option(argc, argv);
            if (ret == ParseStatus::Match && was_parsed)
            {
                throw std::runtime_error("po error: flag \"" + std::string(name()) + "\" appeared more than once");
            }
            return ret;
        }
        operator bool() const
        {
            return parsed();
        }
        virtual bool
            notify() const override
        {
            return false;
        }

    protected:
        //flag(const option& op, std::string_view pattern)
        //    : flag(*detail::parser::instance().get_main_group(), op, pattern)
        //{}
        //flag(detail::base_group& bg, const option& op, std::string_view pattern)
        //    : detail::base_option(&bg, op.long_name, op.short_name, pattern)
        //{
        //    bg.register_option(this);
        //}
    };
    class multi_flag
        : public detail::base_option
    {
    public:
        using type_t = bool;
        using base1_t = detail::base_option;
        using valid_options_t = std::tuple<BaseGroup, LongName, ShortName, Min, Max, Pattern>;

        struct option
        {
            std::string_view long_name;
            char short_name{0};
            std::string_view desc;
            std::size_t min{0}, max{std::size_t(-1)};
        };

        template <class... Args>
        multi_flag(const Args&... args)
            : base1_t(
                  &detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , "")
            , _min(detail::helper::pick_option_with_default<Min>(1, args...))
            , _max(detail::helper::pick_option_with_default<Max>(std::size_t(-1), args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for multi_flag");
            auto& bg = detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...);
            bg.register_option(this);
        }
        virtual ParseStatus
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option(argc, argv);
            if (ret == ParseStatus::Match)
            {
                _count++;
                if (_count > _max)
                {
                    throw std::runtime_error("po error: flag \"" + std::string(name()) +
                        "\" appeared too often (max=" + std::to_string(_max) + ")");
                }
            }
            return ret;
        }
        operator std::size_t() const
        {
            return _count;
        }
        virtual bool
            notify() const override
        {
            if (_count < _min)
            {
                throw std::runtime_error("po error: flag \"" + std::string(name()) +
                    "\" appeared too less (min=" + std::to_string(_min) + ")");
            }
            return false;
        }

    private:
        std::size_t _count{0};
        std::size_t _min, _max;
    };
    template <class T>
    class multi_pattern_flag
        : public flag
    {
    public:
        using type_t = std::vector<T>;
        using base1_t = flag;
        using valid_options_t = std::tuple<BaseGroup, LongName, ShortName, Min, Max, Pattern>;

        template <class... Args>
        multi_pattern_flag(const Args&... args)
            : base1_t(
                  &detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , detail::helper::pick_option_with_default<Min>(1, args...)
                , detail::helper::pick_option_with_default<Max>(1, args...)
                , detail::helper::pick_option<"po error static_assert: missing option \"pattern\" for multi_pattern_flag", Pattern>(args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for multi_pattern_flag");
        }
        virtual ParseStatus
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option(argc, argv);
            if (ret == ParseStatus::Match)
            {
                auto pa = base1_t::parsed_pattern_argument();
                if (pa == "")
                {
                    throw std::runtime_error("po error: pattern variable missing for \"" + std::string(base1_t::parsed_argument()) + "\"");
                }
                auto value = detail::helper::lexical_cast<T>(pa);
                _arguments.push_back(value);
            }
            return ret;
        }
        operator type_t() const
        {
            return _arguments;
        }

    private:
        type_t _arguments;
    };
    class help
        : public flag
    {
    public:
        using base1_t = flag;
        using valid_options_t = std::tuple<BaseGroup, Header, Message>;

        template <class... Args>
        help(const Args&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...)
                , LongName("help")
                , ShortName('h'))
            , _header(detail::helper::pick_option_with_default<Header>("", args...))
            , _message(detail::helper::pick_option_with_default<Message>("", args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for help");
        }
        virtual ParseStatus
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option(argc, argv);
            return ret == ParseStatus::Match ? ParseStatus::HelpParsed : ParseStatus::NoMatch;
        }
        const std::string&
            header() const
        {
            return _header;
        }
        const std::string&
            message() const
        {
            return _message;
        }

    private:
        std::string _header;
        std::string _message;
    };
    template <class T>
    class argument
        : public detail::base_argument<T>
    {
    public:
        using type_t = T;
        using base1_t = detail::base_argument<T>;
        using valid_options_t = std::tuple<BaseGroup, LongName, ShortName, Min, Max, Pattern, Def<T>>;

        template <class... Args>
        argument(const Args&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , 1
                , 1
                , "")
            , _def(detail::helper::pick_option_with_default<Def<T>>(std::nullopt, args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for argument");
            auto def = detail::helper::pick_option_with_default<Def<T>>(std::nullopt, args...);
            if (def)
            {
                _argument = *detail::helper::pick_option_with_default<Def<T>>(std::nullopt, args...);
            }
        }
        virtual ParseStatus
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(argc, argv);
            if (ret)
            {
                _argument = *ret;
            }
            return ret ? ParseStatus::Match : ParseStatus::NoMatch;
        }
        operator T() const
        {
            return _argument;
        }
        virtual bool
            notify() const override
        {
            if (!_def)
            {
                return base1_t::notify();
            }
            return false;
        }

    private:
        bool _def;
        T _argument;
    };
    template <class T>
    class optional_argument
        : public detail::base_argument<T>
    {
    public:
        using type_t = std::optional<T>;
        using base1_t = detail::base_argument<T>;
        using valid_options_t = std::tuple<BaseGroup, LongName, ShortName>;

        template <class... Args>
        optional_argument(const Args&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , 0
                , 1
                , "")
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for optional_argument");
        }
        virtual ParseStatus
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(argc, argv);
            if (ret)
            {
                _argument = ret;
            }
            return ret ? ParseStatus::Match : ParseStatus::NoMatch;
        }
        operator std::optional<T>() const
        {
            return _argument;
        }

    private:
        std::optional<T> _argument;
    };
    template <class T>
    class multi_argument
        : public detail::base_argument<T>
    {
    public:
        using type_t = std::vector<T>;
        using base1_t = detail::base_argument<T>;
        using valid_options_t = std::tuple<BaseGroup, LongName, ShortName, Min, Max>;

        template <class... Args>
        multi_argument(const Args&... args)
            : base1_t(
                  &detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , detail::helper::pick_option_with_default<Min>(1, args...)
                , detail::helper::pick_option_with_default<Max>(1, args...)
                , "")
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for multi_argument");
        }

        virtual ParseStatus
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(argc, argv);
            if (ret)
            {
                _arguments.push_back(*ret);
            }
            return ret ? ParseStatus::Match : ParseStatus::NoMatch;
        }
        operator std::vector<T>() const
        {
            return _arguments;
        }

    private:
        std::vector<T> _arguments;
    };
    template <class KeyT, class ValueT>
    class multi_pattern_argument
        : public detail::base_argument<ValueT>
    {
    public:
        using base1_t = detail::base_argument<ValueT>;
        using type_t = std::map<KeyT, ValueT>;
        using valid_options_t = std::tuple<BaseGroup, LongName, ShortName, Min, Max, Pattern>;

        template <class... Args>
        multi_pattern_argument(const Args&... args)
            : base1_t(
                  &detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , detail::helper::pick_option_with_default<Min>(1, args...)
                , detail::helper::pick_option_with_default<Max>(1, args...)
                , detail::helper::pick_option<"po error static_assert: missing option \"pattern\" for multi_pattern_argument", Pattern>(args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for multi_pattern_argument");
        }
        virtual ParseStatus
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(argc, argv);
            if (ret)
            {
                auto pa = base1_t::parsed_pattern_argument();
                if (pa == "")
                {
                    throw std::runtime_error("po error: pattern variable missing for \"" + std::string(base1_t::parsed_argument()) + "\"");
                }
                auto key = detail::helper::lexical_cast<KeyT>(pa);
                _arguments.insert(std::make_pair(key, *ret));
            }
            return ret ? ParseStatus::Match : ParseStatus::NoMatch;
        }
        operator type_t() const
        {
            return _arguments;
        }

    private:
        type_t _arguments;
    };
    template <class T>
    class positional_argument
        : public detail::base_option
    {
    public:
        using base1_t = detail::base_option;
        using type_t = std::vector<T>;
        using valid_options_t = std::tuple<BaseGroup, LongName, ShortName, Min, Max>;
        
        template <class... Args>
        positional_argument(const Args&... args)
            : base1_t(
                  &detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , "")
            , _min(detail::helper::pick_option_with_default<Min>(1, args...))
            , _max(detail::helper::pick_option_with_default<Max>(std::size_t(-1), args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for positional_argument");
            auto& bg = detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...);
            bg.register_option(this);
        }

        virtual ParseStatus
            try_parse_option(int* argc, const char*** argv) override
        {
            while (*argc)
            {
                if ((**argv)[0] == '-')
                {
                    throw std::runtime_error("po error: in positional arguments list arguments no loneger allowed (\"" +
                        std::string(**argv) + "\"");
                }
                auto value = detail::helper::lexical_cast<T>((**argv));
                _arguments.push_back(value);
                (*argc)--;
                (*argv)++;
            }
            return ParseStatus::Match;
        }
        operator std::optional<T>() const
        {
            return _arguments;
        }
        bool virtual
            notify() const override
        {
            return false;
        }

    private:
        std::vector<T> _arguments;
        std::size_t _min, _max;
    };
    class group
        : public detail::base_group
    {
    public:
        using base1_t = detail::base_group;
        using valid_options_t = std::tuple<BaseGroup, LongName, ShortName, Optional>;

        template <class... Args>
        group(const Args&... args)
            : base1_t(
                  &detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , detail::helper::pick_option_with_default<Optional>(true, args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for group");
            auto& bg = detail::helper::pick_option_with_default<BaseGroup>(*detail::parser::instance().get_main_group(), args...);
            bg.register_option(this);
        }
        operator bool() const
        {
            return parsed();
        }
    };
    template <class... Args>
    class sub_program
        : public detail::base_sub_program
    {
    public:
        using base1_t = detail::base_sub_program;

        sub_program(std::function<int(const typename Args::type_t&...)>&& program, const Args&... args)
            : _program(program)
            , _member{std::forward_as_tuple(args...)}
        {
            detail::parser::instance().register_sub_program(this);
            _base_group = detail::parser::instance().get_main_group();
        }
        sub_program(detail::base_group& bg, std::function<int(const typename Args::type_t&...)>&& program, const Args&... args)
            : _program(program)
            , _member{std::forward_as_tuple(args...)}
        {
            detail::parser::instance().register_sub_program(this);
            _base_group = &bg;
        }
        virtual int
            operator()() override
        {
            return std::apply(_program, _member);
        }
        virtual bool
            parsed() const override
        {
            return _base_group->parsed();
        }

    private:
        std::function<int(const typename Args::type_t&...)> _program;
        std::tuple<const Args&...> _member;
        detail::base_group* _base_group;
    };
    class command_line
    {
    public:
        static ParseStatus
            process(int argc, const char** argv)
        {
            return detail::parser::instance().parse_command_line(argc - 1, argv + 1);
        }
        static bool
            print_help_if_parsed()
        {
        }
        static bool
            notify()
        {
            return detail::parser::instance().notify();
        }
        static int
            execute_main()
        {
            auto result = detail::parser::instance().execute_main();
            if (!result)
            {
                throw std::runtime_error("po error: could not find a matching sub program, invoke --help for information");
            }
            return *result;
        }
    };
}

#define PO_INIT_MAIN_FILE() \
    po::detail::parser po::detail::parser::_instance;

#define PO_INIT_MAIN_FILE_WITH_SUB_PROGRAM_SUPPORT()    \
    PO_INIT_MAIN_FILE()                                 \
    int                                                 \
        main(int argc, const char** argv)               \
    {                                                   \
        int result = 0;                                 \
        po::command_line::process(argc, argv);          \
        if (!po::command_line::notify())                \
        {                                               \
            result = po::command_line::execute_main();  \
        }                                               \
        return result;                                  \
    }
