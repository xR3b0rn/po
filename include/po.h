
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

#include <boost/lexical_cast.hpp>

namespace po
{
    namespace detail
    {
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
            base_option(const std::string& name)
            {
                if (name.size() == 0)
                {
                    throw std::runtime_error("po error: no option name given");
                }
                if (name.size() == 1)
                {
                    _short_name = name;
                }
                else
                {
                    auto iter = std::find(name.begin(), name.end(), ',');
                    if (iter == name.end())
                    {
                        _name = name;
                    }
                    else
                    {
                        if (std::distance(name.begin(), iter) != 1)
                        {
                            throw std::runtime_error("po error: short option name \"" + std::string(name.begin(), iter) + "\" is too long");
                        }
                        _short_name = std::string(name.begin(), name.begin() + 1);
                        if (std::distance(name.begin() + 2, name.end()) < 2)
                        {
                            throw std::runtime_error("po error: option name \"" + std::string(name.begin() + 2, name.end()) + "\" is too short");
                        }
                        _name = std::string(name.begin() + 1, name.end());
                    }
                }
            }
            virtual bool
                try_parse_option(int* argc, const char*** argv) = 0;
            const std::string&
                name() const
            {
                return _name;
            }
            const std::string&
                short_name() const
            {
                return _short_name;
            }
            const std::string&
                name_or_short() const
            {
                return _name == "" ? _short_name : _name;
            }
            virtual void notify() const = 0;

        protected:
            bool
                is_option_name_equal(const char* on) const
            {
                bool result = false;
                if (on[0] == 0 || on[1] == 0)
                {
                    throw std::runtime_error("po error: \"" + std::string(on) + "\" is an invalid option");
                }
                if (on[1] != '-')
                {
                    if (on[1] == _short_name[0])
                    {
                        result = true;
                    }
                }
                else
                {
                    auto iter = std::find(on + 2, on + std::strlen(on), '=');
                    result = std::strncmp(on + 2, _name.c_str(), iter - on - 2) == 0;
                }
                return result;
            }

        private:
            std::string _short_name;
            std::string _name;
        };
        class base_group
            : public base_option
        {
        public:
            struct user_option
            {
                std::string name;
                bool optional;
            };

            base_group(const std::string& name, bool optional)
                : base_option(name)
                , _optional(optional)
            {}
            void
                register_option(base_option* bo)
            {
                _options.push_back(bo);
            }
            void
                set_sub_program(base_sub_program* sp)
            {
                _sub_programs = sp;
            }
            virtual bool
                try_parse_option(int* argc, const char*** argv) = 0
            {
                if (argc == 0)
                {
                    throw std::runtime_error("po error: parsing program option failed");
                }
                bool parsed = true;
                bool group_parsed = false;
                while (parsed)
                {
                    parsed = false;
                    for (auto* op : options())
                    {
                        if (op->try_parse_option(argc, argv))
                        {
                            if (dynamic_cast<base_group*>(op))
                            {
                                if (group_parsed)
                                {
                                    throw std::runtime_error(
                                        "po error: two groups (first: \"" + name_or_short() + "\" second: \"" + op->name_or_short() +  "\")"
                                        "in the same stage are not possible");
                                }
                                group_parsed = true;
                            }
                            if (*argc != 0)
                            {
                                parsed = true;
                            }
                            break;
                        }
                    }
                }
                return true;
            }
            bool
                parsed() const
            {
                return _parsed;
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
            bool optional() const
            {
                return _optional;
            }

        protected:
            void
                set_parsed(bool parsed)
            {
                _parsed = parsed;
            }

        private:
            std::vector<base_option*> _options;
            base_sub_program* _sub_programs{nullptr};
            bool _parsed{false};
            bool _optional;
        };
        class root_group
            : public base_group
        {
        public:
            root_group()
                : base_group("main_group", false)
            {
                set_parsed(true);
            }
            virtual bool
                try_parse_option(int* argc, const char*** argv) override
            {
                return base_group::try_parse_option(argc, argv);
            }
            virtual void
                notify() const override {}
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
                _main_group->try_parse_option(&argc, &argv);
                if (argc != 0)
                {
                    throw std::runtime_error("po error: unkown argument \"" + std::string(*argv) + "\"");
                }
            }
            void
                notify() const
            {
                _main_group->notify();
                for (auto* op : _main_group->options())
                {
                    op->notify();
                }
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
            using type_t = T;

            argument(const std::string& name, std::size_t min, std::size_t max)
                : detail::base_option(name)
                , _min(min)
                , _max(max)
            {
                if (_max < _min)
                {
                    throw std::runtime_error("po error: max < min for option \"" + name_or_short() + "\"");
                }
                po::detail::parser::instance().register_main_group_option(this);
            }
            argument(detail::base_group& bg, const std::string& name, std::size_t min, std::size_t max)
                : detail::base_option(name)
                , _min(min)
                , _max(max)
            {
                if (_max < _min)
                {
                    throw std::runtime_error("po error: max < min for option \"" + name_or_short() + "\"");
                }
                bg.register_option(this);
            }
            std::optional<T>
                try_parse_option_argument(int* argc, const char*** argv)
            {
                std::optional<T> result = std::nullopt;
                bool ret = is_option_name_equal(**argv);
                if (ret)
                {
                    _parse_counter++;
                    if (_parse_counter > _max)
                    {
                        throw std::runtime_error("po error: too many \"" + name_or_short() + "\" arguments given, max is " + std::to_string(_max));
                    }
                    auto iter = std::strrchr(**argv, '=');
                    std::string str_value;
                    if (iter != 0)
                    {
                        str_value = std::string(iter + 1);
                    }
                    else
                    {
                        (*argc)--;
                        (*argv)++;
                        if (*argc == 0)
                        {
                            throw std::runtime_error("po error: no parameter given for argument option \"" + name_or_short() +  "\"");
                        }
                        str_value = std::string(**argv);
                    }
                    (*argc)--;
                    (*argv)++;
                    if constexpr (std::is_arithmetic_v<T>)
                    {
                        result = boost::lexical_cast<T>(str_value);
                    }
                    else
                    {
                        result = static_cast<const T&>(str_value);
                    }
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
            virtual void
                notify() const override
            {
                if (_min > _parse_counter)
                {
                    throw std::runtime_error("po error: too less \"" + name_or_short() + "\" arguments given, min is " + std::to_string(_min));
                }
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
        using type_t = bool;

        struct option
        {
            std::string name;
            std::string desc{""};
        };

        flag(const option& op)
            : detail::base_option(op.name)
        {
            po::detail::parser::instance().register_main_group_option(this);
        }
        flag(detail::base_group& bg, const option& op)
            : detail::base_option(op.name)
        {
            bg.register_option(this);
        }
        virtual bool
            try_parse_option(int* argc, const char*** argv) override
        {
            bool ret = is_option_name_equal(**argv);
            if (ret && _given)
            {
                throw std::runtime_error("po error: flag \"" + std::string(**argv) + "\" appeared more than once");
            }
            else if (ret)
            {
                (*argc)--;
                (*argv)++;
                _given = true;
            }
            return ret;
        }
        operator bool() const
        {
            return _given;
        }
        virtual void
            notify() const override {}

    private:
        bool _given{false};
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
            std::string name;
            bool optional{false};
            std::optional<T> def;
        };

        single_argument(const option& op)
            : base1_t(op.name, op.optional ? 0 : 1, 1)
            , _def(op.def)
        {
            if (op.def)
            {
                _argument = *op.def;
            }
        }
        single_argument(detail::base_group& bg, const option& op)
            : base1_t(bg, op.name, op.optional ? 0 : 1, 1)
            , _def(op.def)
        {
            if (op.def)
            {
                _argument = *op.def;
            }
        }
        virtual bool
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(argc, argv);
            if (ret)
            {
                _argument = *ret;
            }
            return bool(ret);
        }
        operator T() const
        {
            return _argument;
        }
        virtual void
            notify() const override
        {
            if (!_def)
            {
                base1_t::notify();
            }
        }

    private:
        bool _def;
        T _argument;
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
            std::string name;
            std::size_t min{1}, max{std::size_t(-1)};
        };

        multi_argument(const option& op)
            : base1_t(op.name, op.min, op.max)
        {}
        multi_argument(detail::base_group& bg, const option& op)
            : base1_t(bg, op.name, op.min, op.max)
        {}
        virtual bool
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(argc, argv);
            if (ret)
            {
                _arguments.push_back(*ret);
            }
            return bool(ret);
        }
        operator std::vector<T>() const
        {
            return _arguments;
        }

    private:
        std::vector<T> _arguments;
    };
    class group
        : public detail::base_group
    {
    public:
        struct option
        {
            std::string name;
            bool optional{true};
        };

        group(const option& op)
            : detail::base_group(op.name, op.optional)
        {
            detail::parser::instance().register_main_group_option(this);
        }
        group(detail::base_group& bg, const option& op)
            : detail::base_group(op.name, op.optional)
        {
            bg.register_option(this);
        }
        virtual bool
            try_parse_option(int* argc, const char*** argv) override
        {
            bool result = false;
            std::string arg(**argv);
            if (arg == name())
            {
                set_parsed(true);
                (*argc)--;
                (*argv)++;
                result = base_group::try_parse_option(argc, argv);
            }
            return result;
        }
        operator bool() const
        {
            return parsed();
        }
        virtual void
            notify() const override
        {
            if (!optional() && !parsed())
            {
                throw std::runtime_error("po error: could not find option \"" + name_or_short() + "\"");
            }
            if (parsed())
            {
                for (const auto* op : options())
                {
                    op->notify();
                }
            }
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
        static void
            notify()
        {
            detail::parser::instance().notify();
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
        po::command_line::process(argc, argv);          \
        po::command_line::notify();                     \
        return po::command_line::execute_main();        \
    }
