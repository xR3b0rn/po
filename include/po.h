
#pragma once

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
            enum class ParseAction
            {
                NoMatch, Match, Finish
            };

            base_option(std::string_view name, std::string_view short_name, std::string_view pattern)
                : _name(name)
                , _short_name(short_name)
                , _pattern(pattern)
            {
                if (name == "" && short_name == "")
                {
                    throw std::runtime_error("po error: no option name given");
                }
            }
            virtual ParseAction
                try_parse_option(int* argc, const char*** argv) = 0
            {
                ParseAction result = ParseAction::NoMatch;
                if (_pattern == "")
                {
                    const char* on = **argv;
                    if (on[0] == 0 || on[1] == 0)
                    {
                        throw std::runtime_error("po error: invalid option");
                    }
                    if (on[0] != '-')
                    {
                        if (on[1] == 0)
                        {
                            if (on[0] == _short_name[0])
                            {
                                result = ParseAction::Match;
                            }
                        }
                        else if (std::strcmp(on, _name.data()) == 0)
                        {
                            result = ParseAction::Match;
                        }
                    }
                    else if (on[1] != '-')
                    {
                        if (on[1] == _short_name[0])
                        {
                            result = ParseAction::Match;
                        }
                    }
                    else
                    {
                        auto iter = std::find(on + 2, on + std::strlen(on), '=');
                        if (std::strncmp(on + 2, _name.data(), iter - on - 2) == 0)
                        {
                            result = ParseAction::Match;
                        }
                    }
                    if (result == ParseAction::Match)
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
                        result = ParseAction::Match;
                    }
                }
                return result;
            }
            std::string_view
                name() const
            {
                return _name;
            }
            std::string_view
                short_name() const
            {
                return _short_name;
            }
            std::string_view
                name_or_short() const
            {
                return _name == "" ? _short_name : _name;
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

        private:
            std::string_view _short_name;
            std::string_view _name;
            std::string_view _pattern;
            std::string_view _parsed_argument;
            std::string_view _parsed_pattern_argument;
        };
        class base_group
            : public base_option
        {
        public:
            using base1_t = base_option;

            struct user_option
            {
                std::string_view name;
                bool optional;
            };

            base_group(std::string_view name, std::string_view short_name, bool optional)
                : base1_t(name, short_name, "")
                , _optional(optional)
            {}
            void
                register_option(base_option* bo)
            {
                _options.push_back(bo);
            }
            void
                set_help_option(base_option* help)
            {
                _help = help;
            }
            void
                set_sub_program(base_sub_program* sp)
            {
                _sub_programs = sp;
            }
            virtual ParseAction
                try_parse_option(int* argc, const char*** argv) override
            {
                auto result = base1_t::try_parse_option(argc, argv);
                if (result == ParseAction::Match)
                {
                    if (!parsed_as_group())
                    {
                        throw std::runtime_error("po error: ambigious argument \"" + std::string(name_or_short()) + "\"");
                    }
                    if (argc == 0)
                    {
                        throw std::runtime_error("po error: parsing program option failed, no more arguments given");
                    }
                    bool parsed = true;
                    bool group_parsed = false;
                    bool matched = false;
                    while (parsed)
                    {
                        parsed = false;
                        for (auto* op : options())
                        {
                            auto ret = op->try_parse_option(argc, argv);
                            if (ret == ParseAction::Match)
                            {
                                matched = true;
                                if (!op->parsed_as_group() && *argc != 0)
                                {
                                    parsed = true;
                                }
                                break;
                            }
                            else if (ret == ParseAction::Finish)
                            {
                                parsed= false;
                                result = ret;
                                break;
                            }
                        }
                    }
                }
                return result;
            }
            std::optional<int>
                execute_main()
            {
                std::optional<int> result = std::nullopt;
                if (_sub_programs != nullptr)
                {
                    result = (*_sub_programs)();
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
                    throw std::runtime_error("po error: could not find option \"" + std::string(name_or_short()) + "\"");
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

        private:
            std::vector<base_option*> _options;
            base_option* _help;
            base_sub_program* _sub_programs{nullptr};
            bool _optional;
        };
        class root_group
            : public base_group
        {
        public:
            root_group()
                : base_group("main_group", "", false)
            {
                set_parsed_argument("main_group");
            }
            virtual ParseAction
                try_parse_option(int* argc, const char*** argv) override
            {
                ParseAction result = ParseAction::NoMatch;
                if (argc == 0)
                {
                    throw std::runtime_error("po error: parsing program option failed, no more arguments given");
                }
                bool parsed = true;
                bool group_parsed = false;
                bool matched = false;
                while (parsed)
                {
                    parsed = false;
                    for (auto* op : options())
                    {
                        result = op->try_parse_option(argc, argv);
                        if (result == ParseAction::Match)
                        {
                            matched = true;
                            if (op->parsed_as_group())
                            {
                                if (group_parsed)
                                {
                                    throw std::runtime_error(
                                        "po error: two groups (first: \"" + std::string(name_or_short()) + "\" second: \"" +
                                        std::string(op->name_or_short()) +  "\") in the same stage are not possible");
                                }
                                group_parsed = true;
                            }
                            if (*argc != 0)
                            {
                                parsed = true;
                            }
                            break;
                        }
                        else if (result == ParseAction::Finish)
                        {
                            parsed = false;
                            break;
                        }
                    }
                }
                if (*argc == 0 || result == ParseAction::Finish)
                {
                    result = ParseAction::Finish;
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
            void
                parse_command_line(int argc, const char** argv)
            {
                auto ret = _main_group->try_parse_option(&argc, &argv);
                if (ret != detail::base_option::ParseAction::Finish)
                {
                    throw std::runtime_error("po error: unkown argument \"" + std::string(*argv) + "\"");
                }
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
        class argument
            : public detail::base_option
        {
        public:
            using base1_t = detail::base_option;
            using type_t = T;

            argument(const std::string_view name, std::string_view short_name, std::size_t min, std::size_t max, std::string_view pattern)
                : argument(*detail::parser::instance().get_main_group(), name, short_name, min, max, pattern)
            {}
            argument(detail::base_group& bg, std::string_view name, std::string_view short_name, std::size_t min, std::size_t max, std::string_view pattern)
                : detail::base_option(name, short_name, pattern)
                , _min(min)
                , _max(max)
            {
                if (_max < _min)
                {
                    throw std::runtime_error("po error: max < min for option \"" + std::string(name_or_short()) + "\"");
                }
                bg.register_option(this);
            }
            std::optional<T>
                try_parse_option_argument(int* argc, const char*** argv)
            {
                std::optional<T> result = std::nullopt;
                auto ret = base1_t::try_parse_option(argc, argv);
                if (ret == base_option::ParseAction::Match)
                {
                    _parse_counter++;
                    if (_parse_counter > _max)
                    {
                        throw std::runtime_error("po error: too many \"" + std::string(name_or_short()) +
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
                    else
                    {
                        if (*argc == 0)
                        {
                            throw std::runtime_error("po error: no parameter given for argument option \""
                                + std::string(name_or_short()) +  "\"");
                        }
                        str_value = std::string_view(**argv);
                        (*argc)--;
                        (*argv)++;
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
                    throw std::runtime_error("po error: too less \"" + std::string(name_or_short()) +
                        "\" arguments given, min is " + std::to_string(_min));
                }
                return false;
            }

        private:
            std::size_t _min{1}, _max{1};
            std::size_t _parse_counter{0};
        };
    }
    class flag
        : public detail::base_option
    {
    public:
        using base1_t = detail::base_option;
        using type_t = bool;

        struct option
        {
            std::string_view name;
            std::string_view short_name;
            std::string_view desc;
        };

        flag(const option& op)
            : flag(*detail::parser::instance().get_main_group(), op)
        {}
        flag(detail::base_group& bg, const option& op)
            : detail::base_option(op.name, op.short_name, "")
        {
            bg.register_option(this);
        }
        virtual ParseAction
            try_parse_option(int* argc, const char*** argv) override
        {
            auto was_parsed = parsed();
            auto ret = base1_t::try_parse_option(argc, argv);
            if (ret == base_option::ParseAction::Match && was_parsed)
            {
                throw std::runtime_error("po error: flag \"" + std::string(name_or_short()) + "\" appeared more than once");
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
        flag(const option& op, std::string_view pattern)
            : flag(*detail::parser::instance().get_main_group(), op, pattern)
        {}
        flag(detail::base_group& bg, const option& op, std::string_view pattern)
            : detail::base_option(op.name, op.short_name, pattern)
        {
            bg.register_option(this);
        }
    };
    template <class T>
    class multi_pattern_flag
        : public flag
    {
    public:
        using base1_t = flag;
        using type_t = std::vector<T>;

        struct option
        {
            std::string_view name;
            std::string_view desc;
            std::string_view pattern;
        };
        multi_pattern_flag(const option& op)
            : base1_t({ op.name, op.pattern, op.desc }, op.pattern)
        {}
        multi_pattern_flag(detail::base_group& bg, const option& op)
            : base1_t(bg, { op.name, op.pattern, op.desc }, op.pattern)
        {}
        virtual ParseAction
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option(argc, argv);
            if (ret == ParseAction::Match)
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

        struct option
        {
            std::string_view header;
            std::string_view message;
        };

        help(const option& op)
            : help(*detail::parser::instance().get_main_group(), op)
        {}
        help(detail::base_group& bg, const option& op)
            : flag(bg, { .name = "h,help" } )
            , _header(op.header)
            , _message(op.message)
        {
            bg.set_help_option(this);
        }
        virtual ParseAction
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option(argc, argv);
            return ret == ParseAction::Match ? ParseAction::Finish : ParseAction::NoMatch;
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
    class single_argument
        : public detail::argument<T>
    {
    public:
        using base1_t = detail::argument<T>;
        using type_t = T;

        struct option
        {
            std::string_view name;
            std::string_view short_name;
            std::optional<T> def;
        };

        single_argument(const option& op)
            : single_argument(*detail::parser::instance().get_main_group(), op)
        {}
        single_argument(detail::base_group& bg, const option& op)
            : base1_t(bg, op.name, op.short_name, 1, 1, "")
            , _def(op.def)
        {
            if (op.def)
            {
                _argument = *op.def;
            }
        }
        virtual detail::base_option::ParseAction
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(argc, argv);
            if (ret)
            {
                _argument = *ret;
            }
            return ret ? detail::base_option::ParseAction::Match : detail::base_option::ParseAction::NoMatch;
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
        : public detail::argument<T>
    {
    public:
        using base1_t = detail::argument<T>;
        using type_t = std::optional<T>;

        struct option
        {
            std::string_view name;
            std::string_view short_name;
            bool optional{false};
        };

        optional_argument(const option& op)
            : optional_argument(*detail::parser::instance().get_main_group(), op)
        {}
        optional_argument(detail::base_group& bg, const option& op)
            : base1_t(bg, op.name, op.short_name, 0, 1, "")
        {}
        virtual detail::base_option::ParseAction
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(argc, argv);
            _argument = ret;
            return ret ? detail::base_option::ParseAction::Match : detail::base_option::ParseAction::NoMatch;
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
        : public detail::argument<T>
    {
    public:
        using base1_t = detail::argument<T>;
        using type_t = std::vector<T>;

        struct option
        {
            std::string_view name;
            std::string_view short_name;
            std::size_t min{1}, max{std::size_t(-1)};
        };

        multi_argument(const option& op)
            : multi_argument(*detail::parser::instance().get_main_group(), op)
        {}
        multi_argument(detail::base_group& bg, const option& op)
            : base1_t(bg, op.name, op.short_name, op.min, op.max, "")
        {}
        virtual detail::base_option::ParseAction
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(argc, argv);
            if (ret)
            {
                _arguments.push_back(*ret);
            }
            return ret ? detail::base_option::ParseAction::Match : detail::base_option::ParseAction::NoMatch;
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
        : public detail::argument<ValueT>
    {
    public:
        using base1_t = detail::argument<ValueT>;
        using type_t = std::map<KeyT, ValueT>;

        struct option
        {
            std::string_view name;
            std::string_view short_name;
            std::size_t min{1}, max{std::size_t(-1)};
            std::string_view pattern;
        };
        multi_pattern_argument(const option& op)
            : base1_t(op.name, op.short_name, op.min, op.max, op.pattern)
        {}
        multi_pattern_argument(detail::base_group& bg, const option& op)
            : base1_t(bg, op.name, op.short_name,op.min, op.max, op.pattern)
        {}
        virtual detail::base_option::ParseAction
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
            return ret ? detail::base_option::ParseAction::Match : detail::base_option::ParseAction::NoMatch;
        }
        operator type_t() const
        {
            return _arguments;
        }

    private:
        type_t _arguments;
    };
    class group
        : public detail::base_group
    {
    public:
        struct option
        {
            std::string_view name;
            std::string_view short_name;
            bool optional{true};
        };

        group(const option& op)
            : group(*detail::parser::instance().get_main_group(), op)
        {}
        group(detail::base_group& bg, const option& op)
            : detail::base_group(op.name, op.short_name, op.optional)
        {
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

        sub_program(std::function<int(const typename Args::type_t&...)>&& program, Args&... args)
            : _program(program)
            , _member{std::forward_as_tuple(args...)}
        {
            detail::parser::instance().register_sub_program(this);
            _base_group = detail::parser::instance().get_main_group();
        }
        sub_program(detail::base_group& bg, std::function<int(const typename Args::type_t&...)>&& program, Args&... args)
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
        std::tuple<Args&...> _member;
        detail::base_group* _base_group;
    };
    class command_line
    {
    public:
        static void
            process(int argc, const char** argv)
        {
            detail::parser::instance().parse_command_line(argc - 1, argv + 1);
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
