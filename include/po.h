
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
#include <set>
#include <ostream>
#include <iomanip>
#include <filesystem>

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
            template <class T1, class T2>
            inline constexpr bool has_type_v = has_type<T1, T2>::value;

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
            
            void print_1_column(std::ostream& os, std::string_view text)
            {
            }
            void print_2_columns(std::ostream& os, std::string_view left, std::string_view right)
            {
                os << "  ";
                os << std::setw(26) << std::left << left;
                if (left.size() > 22 && right != "")
                {
                    os << "\n  " << std::setw(26) << "";
                }
                std::stringstream desc_{std::string(right)};
                std::size_t bytes_written = 0;
                std::string line;
                while (std::getline(desc_, line, '\n'))
                {
                    std::stringstream line_{line};
                    while (line_)
                    {
                        std::string word;
                        line_ >> word;
                        if (bytes_written > 0 && bytes_written + word.size() > 32)
                        {
                            bytes_written = 0;
                            os << "\n  " << std::setw(26) << "";
                        }
                        if (word == "")
                        {
                            break;
                        }
                        os << word << " ";
                        bytes_written += word.size() + 1;
                    }
                    bytes_written = 0;
                    os << "\n  " << std::setw(26) << "";
                }
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
        class base_group;
        class base_option
        {
        public:
            using parent_t = std::optional<std::reference_wrapper<base_group>>;
            using cparent_t = std::optional<std::reference_wrapper<const base_group>>;

            base_option(
                  parent_t parent
                , std::string_view long_name
                , char short_name
                , std::string_view pattern
                , std::string_view desc
                , std::string_view arg_name)

                : _parent(parent)
                , _long_name(long_name)
                , _short_name(short_name)
                , _pattern(pattern)
                , _desc(desc)
                , _arg_name(arg_name)
            {}
            virtual ParseStatus
                try_parse_option(int narg, int* argc, const char*** argv)
            {
                ParseStatus result = ParseStatus::NoMatch;
                if (*argc != 0)
                {
                    if (_pattern == "")
                    {
                        const char* on = **argv;
                        if (on[0] != 0)
                        {
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
                                _parsed_count++;
                            }
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
                            _parsed_count++;
                            result = ParseStatus::Match;
                        }
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
            template <class T>
            std::string
                get_print_name_argument(std::optional<T> def) const
            {
                std::string name_ = "";
                if (short_name() != 0)
                {
                    name_ += "-";
                    name_.push_back(short_name());
                    if (long_name() != "")
                    {
                        name_ += " | --" + std::string(long_name());
                    }
                }
                else
                {
                    name_ += "--" + std::string(long_name());
                }
                std::string argn(arg_name());
                if (argn == "")
                {
                    argn = "arg";
                }
                if (def)
                {
                    if constexpr (std::is_same_v<T, char>)
                    {
                        name_ += " <" + argn + "=";
                        name_.push_back(*def);
                        name_ += ">";
                    }
                    else if constexpr (std::is_same_v<T, std::string>)
                    {
                        name_ += " <" + argn + "=" + *def + ">";
                    }
                    else
                    {
                        name_ += " <" + argn + "=" + std::to_string(*def) + ">";
                    }
                }
                else
                {
                    name_ += " <" + argn + ">";
                }
                return name_;
            }
            std::string
                get_print_name_positional() const
            {
                return "<" + std::string(arg_name()) + ">...";
            }
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
            std::size_t
                parsed_count() const
            {
                return _parsed_count;
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
            void inc_parsed_count()
            {
                _parsed_count++;
            }
            std::string_view
                desc() const
            {
                return _desc;
            }
            std::string_view
                arg_name() const
            {
                return _arg_name;
            }
            parent_t
                parent()
            {
                return _parent;
            }
            const parent_t
                parent() const
            {
                return _parent;
            }
            virtual void
                notify() const = 0;
            virtual void
                print_help(std::ostream& os, int argc, const char** argv) const = 0;

        private:
            parent_t _parent;
            char _short_name;
            std::string_view _long_name;
            std::string_view _pattern;
            std::string_view _parsed_argument;
            std::string_view _parsed_pattern_argument;
            std::string_view _desc;
            std::string_view _arg_name;
            std::size_t _parsed_count{0};
        };
        class base_group
            : public base_option
        {
        public:
            using base1_t = base_option;

            base_group(
                  parent_t parent
                , std::string_view name
                , char short_name
                , bool optional
                , std::string_view desc)

                : base1_t(parent, name, short_name, "", desc, "")
                , _optional(optional)
            {}
            void
                register_option(base_option* bo)
            {
                _options.push_back(bo);
            }
            void
                register_group(base_group* bg)
            {
                _groups.push_back(bg);
            }
            void 
                set_multi_positional_argument(base_option* bo)
            {
                _multi_positional_argument = bo;
            }
            base_option*
                get_multi_positional_argument()
            {
                return _multi_positional_argument;
            }
            const base_option*
                get_multi_positional_argument() const
            {
                return _multi_positional_argument;
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
                set_after(base_option* bo)
            {
                _after = bo;
            }
            void
                set_bind_to(base_option* bo)
            {
                _bind_to = bo;
            }
            virtual ParseStatus
                try_parse_option(int narg, int* argc, const char*** argv) override
            {
                auto result = base1_t::try_parse_option(narg, argc, argv);
                if (result == ParseStatus::Match)
                {
                    if (parsed_as_group())
                    {
                        bool parsed = true;
                        bool group_parsed = false;
                        std::size_t parsed_counter = 0;
                        while (parsed)
                        {
                            parsed = false;
                            for (auto* op : options())
                            {
                                auto ret = op->try_parse_option(narg, argc, argv);
                                if (ret == ParseStatus::Match)
                                {
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
                            if (!parsed)
                            {
                                for (auto* op : groups())
                                {
                                    result = op->try_parse_option(narg, argc, argv);
                                    if (*argc == 0)
                                    {
                                        break;
                                    }
                                    if (result == ParseStatus::Match)
                                    {
                                        break;
                                    }
                                    else if (result == ParseStatus::HelpParsed)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        if (_bind_to != nullptr)
                        {
                            _bind_to->try_parse_option(narg, argc, argv);
                        }
                        if (_multi_positional_argument != nullptr)
                        {
                            _multi_positional_argument->try_parse_option(narg, argc, argv);
                        }
                    }
                    else
                    {
                        (*argc)++;
                        (*argv)--;
                        result = ParseStatus::NoMatch;
                    }
                }
                if (_after != nullptr)
                {
                    _after->try_parse_option(narg, argc, argv);
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
            const std::vector<base_group*>&
                groups() const
            {
                return _groups;
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
            base_option*
                after()
            {
                return _after;
            }
            const base_option*
                after() const
            {
                return _after;
            }
            base_option*
                bind_to()
            {
                return _bind_to;
            }
            const base_option*
                bind_to() const
            {
                return _bind_to;
            }
            virtual void
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
                        op->notify();
                    }
                    if (_bind_to != nullptr)
                    {
                        if (!_bind_to->parsed())
                        {
                            throw std::runtime_error("po error: if \"" + std::string(name()) + "\" is given, \"" +
                                std::string(_bind_to->name()) + "\" must follow");
                        }
                        _bind_to->notify();
                    }
                }
                if (_after != nullptr)
                {
                    if (!_after->parsed())
                    {
                        throw std::runtime_error("po error: could not find \"" + std::string(_after->name()) + "\"");
                    }
                    _after->notify();
                }
            }
            virtual void
                print_help(std::ostream& os, int argc, const char** argv) const override
            {
                // Synopsis
                os << "Synopsis:\n";
                os << "  ";
                std::filesystem::path path_to_exe(*argv);
                os << path_to_exe.filename().string() << " ";
                std::vector<cparent_t> hist;
                {
                    cparent_t cur = *this;
                    while (cur)
                    {
                        hist.push_back(cur);
                        cur = cur->get().parent();
                    }
                }
                for (auto iter = hist.rbegin(); iter != hist.rend() - 1; iter++)
                {
                    const base_group& bg = (**iter).get();
                    if (bg.name() != "")
                    {
                        os << bg.name() << " ";
                    }
                    if (bg.options().size() > 0)
                    {
                        os << "[Options...] ";
                    }
                }
                if (options().size() > 0)
                {
                    os << "[Options...] ";
                }
                if (groups().size() > 0)
                {
                    os << "[SubGroup] ";
                }
                base_option* cur = nullptr;
                bool b_to = false;
                if (_after != nullptr)
                {
                    cur = _after;
                }
                else if (_bind_to != nullptr)
                {
                    cur = _bind_to;
                }
                while (cur != nullptr)
                {
                    auto* bg_cur = dynamic_cast<base_group*>(cur);
                    if (bg_cur->optional() && !b_to)
                    {
                        os << "[";
                        b_to = true;
                    }
                    std::string name_{cur->long_name()};
                    os << name_ << " ";
                    if (bg_cur->bind_to() == nullptr && b_to)
                    {
                        os << "] ";
                        b_to = false;
                    }
                    if (bg_cur != nullptr)
                    {
                        cur = nullptr;
                        if (bg_cur->after() != nullptr)
                        {
                            cur = bg_cur->after();
                        }
                        else if (bg_cur->bind_to() != nullptr)
                        {
                            cur = bg_cur->bind_to();
                        }
                    }
                }
                if (get_multi_positional_argument() != nullptr)
                {
                    os << get_multi_positional_argument()->get_print_name_positional();
                }
                os << "\n\n";
                if (desc() != "")
                {
                    os << desc();
                    os << "\n\n";
                }
                if (groups().size() > 0)
                {
                    os << "Help:\n";
                    os << "Type program_name --help for a help overview\n";
                    os << "and program_name <SubGroup> --help for help on a specifiy SubGroup";
                    os << "\n\n";
                }
                // Synopsis end

                if (options().size() > 0)
                {
                    os << "Options:\n";
                    for (const auto* op : options())
                    {
                        op->print_help(os, argc, argv);
                    }
                }
                if (groups().size() > 0)
                {
                    os << "\nSubGroups:\n";
                    for (const auto* g : groups())
                    {
                        helper::print_2_columns(os, g->name(), g->desc());
                        os << "\n\n";
                    }
                }
                if (get_multi_positional_argument() != nullptr)
                {
                    get_multi_positional_argument()->print_help(os, argc, argv);
                }
            }

        private:
            std::vector<base_option*> _options;
            std::vector<base_group*> _groups;
            base_option* _help;
            base_sub_program* _sub_program{nullptr};
            base_option* _after{nullptr};
            base_option* _bind_to{nullptr};
            base_option* _multi_positional_argument{nullptr};
            bool _optional;
            bool _has_group{false};
        };
        class root_group
            : public base_group
        {
        public:
            using base1_t = base_group;

            root_group()
                : base_group(std::nullopt, "", 0, false, "")
            {
                set_parsed_argument("main_group");
            }
            virtual ParseStatus
                try_parse_option(int narg, int* argc, const char*** argv) override
            {
                ParseStatus result = ParseStatus::NoMatch;
                if (*argc != 0)
                {
                    bool parsed = true;
                    while (parsed)
                    {
                        parsed = false;
                        for (auto* op : options())
                        {
                            result = op->try_parse_option(narg, argc, argv);
                            if (*argc == 0)
                            {
                                break;
                            }
                            if (result == ParseStatus::Match)
                            {
                                parsed = true;
                                break;
                            }
                            else if (result == ParseStatus::HelpParsed)
                            {
                                break;
                            }
                        }
                        if (!parsed)
                        {
                            for (auto* op : groups())
                            {
                                result = op->try_parse_option(narg, argc, argv);
                                if (*argc == 0)
                                {
                                    break;
                                }
                                if (result == ParseStatus::Match)
                                {
                                    break;
                                }
                                else if (result == ParseStatus::HelpParsed)
                                {
                                    break;
                                }
                            }
                        }
                    }
                    if (get_multi_positional_argument() != nullptr)
                    {
                        result = get_multi_positional_argument()->try_parse_option(narg, argc, argv);
                    }
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
                _argc = argc;
                _argv = argv;
                ParseStatus result = ParseStatus::NoMatch;
                argc--;
                argv++;
                if (argc != 0)
                {
                    result = _main_group->try_parse_option(argc, &argc, &argv);
                    if (result != ParseStatus::Match &&
                        result != ParseStatus::HelpParsed)
                    {
                        throw std::runtime_error("po error: unkown argument \"" + std::string(*argv) + "\"");
                    }
                }
                return result;
            }
            void
                notify() const
            {
                _main_group->notify();
            }
            operator base_group&()
            {
                return *_main_group;
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
            int get_argc() const
            {
                return _argc;
            }
            const char** get_argv() const
            {
                return _argv;
            }

        private:
            int _argc{0};
            const char** _argv{nullptr};
            static parser _instance;
            std::unique_ptr<root_group> _main_group;
            std::vector<base_sub_program*> _sub_programs;
        };
        template <class T>
        class base_argument
            : public detail::base_option
        {
        public:
            using type_t = T;
            using base1_t = detail::base_option;

            base_argument(
                  parent_t bg
                , std::string_view long_name
                , char short_name
                , std::size_t min
                , std::size_t max
                , std::string_view pattern
                , std::string_view desc
                , std::string_view arg_name)

                : detail::base_option(bg, long_name, short_name, pattern, desc, arg_name)
                , _min(min)
                , _max(max)
            {
                if (_max < _min)
                {
                    throw std::runtime_error("po error: max < min for option \"" + std::string(name()) + "\"");
                }
                bg->get().register_option(this);
            }
            std::optional<T>
                try_parse_option_argument(int narg, int* argc, const char*** argv)
            {
                std::optional<T> result = std::nullopt;
                auto ret = base1_t::try_parse_option(narg, argc, argv);
                if (ret == ParseStatus::Match)
                {
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
                        if (*argc != 0)
                        {
                            str_value = std::string_view(**argv);
                            (*argc)--;
                            (*argv)++;
                        }
                    }
                    else
                    {
                        str_value = std::string_view(pa.data() + 2, std::distance(pa.begin() + 2, pa.end()));
                    }
                    result = helper::lexical_cast<T>(str_value);
                }
                return result;
            }
            virtual void
                notify() const override
            {
                if (_min > parsed_count())
                {
                    throw std::runtime_error("po error: too less \"" + std::string(name()) +
                        "\" arguments given (min=" + std::to_string(_min) + ")");
                }
                else if (_max < parsed_count())
                {
                    throw std::runtime_error("po error: too many \"" + std::string(name()) +
                        "\" arguments given (min=" + std::to_string(_min) + ")");
                }
            }

        private:
            std::size_t _min{1}, _max{1};
            std::size_t _parse_counter{0};
        };

        struct TagParentGroup {};
        struct TagLongName {};
        struct TagShortName {};
        struct TagDesc {};
        struct TagOptional {};
        struct TagMin {};
        struct TagMax {};
        struct TagPattern {};
        template <class T>
        struct TagDef {};
        struct TagHeader {};
        struct TagMessage {};
        struct TagBindTo {};
        struct TagAfter {};
        struct TagArgName {};
    }

    using ParentGroup = detail::helper::named_type<detail::base_group::parent_t, detail::TagParentGroup>;
    using LongName = detail::helper::named_type<std::string_view, detail::TagLongName>;
    using ShortName = detail::helper::named_type<char, detail::TagShortName>;
    using Desc = detail::helper::named_type<std::string_view, detail::TagDesc>;
    using Optional = detail::helper::named_type<bool, detail::TagOptional>;
    using Min = detail::helper::named_type<std::size_t, detail::TagMin>;
    using Max = detail::helper::named_type<std::size_t, detail::TagMax>;
    using Pattern = detail::helper::named_type<std::string_view, detail::TagPattern>;
    template <class T>
    using Def = detail::helper::named_type<std::optional<T>, detail::TagDef<T>>;
    using Header = detail::helper::named_type<std::string_view, detail::TagHeader>;
    using Message = detail::helper::named_type<std::string_view, detail::TagMessage>;
    using BindTo = detail::helper::named_type<detail::base_group::parent_t, detail::TagBindTo>;
    using After = detail::helper::named_type<detail::base_group::parent_t, detail::TagAfter>;
    using ArgName = detail::helper::named_type<std::string_view, detail::TagArgName>;

    class flag
        : public detail::base_option
    {
    public:
        using type_t = bool;
        using base1_t = detail::base_option;
        using valid_options_t = std::tuple<ParentGroup, LongName, ShortName, Pattern, Desc>;
        
        template <class... Args>
        flag(Args&&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , detail::helper::pick_option_with_default<Pattern>("", args...)
                , detail::helper::pick_option_with_default<Desc>("", args...)
                , "")
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for flag");
            static_assert(
                  detail::helper::has_type_v<LongName, std::tuple<Args...>> ||
                  detail::helper::has_type_v<ShortName, std::tuple<Args...>>
                , "po error static_assert: flag requires ShortName or LongName option");
            auto pg = detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...);
            pg->get().register_option(this);
        }
        operator bool() const
        {
            return parsed();
        }
        virtual void
            notify() const override
        {
            if (parsed_count() > 1)
            {
                throw std::runtime_error("po error: flag \"" + std::string(name()) + "\" is specified more than once");
            }
        }
        virtual void
            print_help(std::ostream& os, int argc, const char** argv) const override
        {
            std::string name_ = "";
            if (short_name() != 0)
            {
                name_ += "-";
                name_.push_back(short_name());
                if (long_name() != "")
                {
                    name_ += " | --" + std::string(long_name());
                }
            }
            else
            {
                name_ += "--" + std::string(long_name());
            }
            os << "  ";
            os << std::setw(26) << std::left << name_;
            if (name_.size() > 22 && desc() != "")
            {
                os << "\n";
            }
            std::stringstream desc_{std::string(desc())};
            std::size_t bytes_written = 0;
            while (desc_)
            {
                std::string word;
                desc_ >> word;
                if (bytes_written > 0 && bytes_written + word.size() > 32)
                {
                    bytes_written = 0;
                    os << "\n  " << std::setw(26) << "";
                }
                if (word == "")
                {
                    break;
                }
                os << word << " ";
                bytes_written += word.size() + 1;
            }
            os << "\n\n";
        }
    };
    class multi_flag
        : public detail::base_option
    {
    public:
        using type_t = bool;
        using base1_t = detail::base_option;
        using valid_options_t = std::tuple<ParentGroup, LongName, ShortName, Min, Max, Pattern, Desc>;

        template <class... Args>
        multi_flag(Args&&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , ""
                , detail::helper::pick_option_with_default<Desc>("", args...)
                , "")
            , _min(detail::helper::pick_option_with_default<Min>(1, args...))
            , _max(detail::helper::pick_option_with_default<Max>(std::size_t(-1), args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for multi_flag");
            auto po = detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...);
            po->get().register_option(this);
        }
        virtual void
            notify() const override
        {
            if (parsed_count() < _min)
            {
                throw std::runtime_error("po error: flag \"" + std::string(name()) +
                    "\" appeared too less (min=" + std::to_string(_min) + ")");
            }
            else if (parsed_count() > _max)
            {
                throw std::runtime_error("po error: flag \"" + std::string(name()) +
                    "\" appeared too often (max=" + std::to_string(_max) + ")");
            }
        }
        virtual void
            print_help(std::ostream& os, int argc, const char** argv) const override
        {
        }

    private:
        std::size_t _min, _max;
    };
    template <class T>
    class multi_pattern_flag
        : public flag
    {
    public:
        using type_t = std::vector<T>;
        using base1_t = flag;
        using valid_options_t = std::tuple<ParentGroup, LongName, ShortName, Min, Max, Pattern>;

        template <class... Args>
        multi_pattern_flag(Args&&... args)
            : base1_t(
                  ParentGroup(detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...))
                , LongName(detail::helper::pick_option_with_default<LongName>("", args...))
                , ShortName(detail::helper::pick_option_with_default<ShortName>(0, args...))
                , Pattern(detail::helper::pick_option_with_default<Pattern>("", args...)))
            , _min(detail::helper::pick_option_with_default<Min>(1, args...))
            , _max(detail::helper::pick_option_with_default<Max>(1, args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for multi_pattern_flag");
            static_assert(detail::helper::has_type<Pattern, std::tuple<Args...>>::value, "po error static_assert: missing option \"pattern\" for multi_pattern_flag");
        }
        virtual ParseStatus
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option(argc, argv);
            if (ret == ParseStatus::Match)
            {
                auto pa = base1_t::parsed_pattern_argument();
                auto value = detail::helper::lexical_cast<T>(pa);
                _arguments.push_back(value);
            }
            return ret;
        }
        operator type_t() const
        {
            return _arguments;
        }
        virtual void
            print_help(std::ostream& os) const override
        {
        }

    private:
        type_t _arguments;
        std::size_t _min, _max;
    };
    class help
        : public flag
    {
    public:
        using base1_t = flag;
        using valid_options_t = std::tuple<ParentGroup, Header, Message>;

        template <class... Args>
        help(Args&&... args)
            : base1_t(
                  ParentGroup(detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...))
                , LongName("help")
                , ShortName('h'))
            , _header(detail::helper::pick_option_with_default<Header>("", args...))
            , _message(detail::helper::pick_option_with_default<Message>("", args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for help");
        }
        virtual ParseStatus
            try_parse_option(int narg, int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option(narg, argc, argv);
            if (ret == ParseStatus::Match)
            {
                std::stringstream ss;
                parent()->get().print_help(ss, *argc + (narg - *argc + 1), *argv - (narg - *argc + 1));
                throw std::runtime_error(ss.str());
            }
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
        using valid_options_t = std::tuple<ParentGroup, LongName, ShortName, Desc, Min, Max, Pattern, Def<T>, ArgName>;

        template <class... Args>
        argument(Args&&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , 1
                , 1
                , ""
                , detail::helper::pick_option_with_default<Desc>("", args...)
                , detail::helper::pick_option_with_default<ArgName>("", args...))
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
            try_parse_option(int narg, int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(narg, argc, argv);
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
        virtual void
            notify() const override
        {
            if (!_def && parsed())
            {
                base1_t::notify();
            }
        }
        virtual void
            print_help(std::ostream& os, int argc, const char** argv) const override
        {
            std::string name_ = get_print_name_argument(_def);
            detail::helper::print_2_columns(os, name_, desc());
            os << "\n\n";
        }

    private:
        std::optional<T> _def;
        T _argument;
    };
    template <class T>
    class optional_argument
        : public detail::base_argument<T>
    {
    public:
        using type_t = std::optional<T>;
        using base1_t = detail::base_argument<T>;
        using valid_options_t = std::tuple<ParentGroup, LongName, ShortName, Desc, ArgName>;

        template <class... Args>
        optional_argument(Args&&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , 0
                , 1
                , ""
                , detail::helper::pick_option_with_default<Desc>("", args...)
                , detail::helper::pick_option_with_default<ArgName>("", args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for optional_argument");
        }
        virtual ParseStatus
            try_parse_option(int narg, int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(narg, argc, argv);
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
        virtual void
            print_help(std::ostream& os, int argc, const char** argv) const override
        {
            std::string name_ = "[";
            if (short_name() != 0)
            {
                name_ += "-";
                name_.push_back(short_name());
                if (long_name() != "")
                {
                    name_ += " | --" + std::string(long_name());
                }
            }
            else
            {
                name_ += "--" + std::string(long_name());
            }
            if (arg_name() != "")
            {
                name_ += " <" + std::string(arg_name()) + ">]";
            }
            else
            {
                name_ += " <arg>]";
            }
            os << "  ";
            os << std::setw(26) << std::left << name_;
            if (name_.size() > 22 && desc() != "")
            {
                os << "\n";
            }
            std::stringstream desc_{std::string(desc())};
            std::size_t bytes_written = 0;
            while (desc_)
            {
                std::string word;
                desc_ >> word;
                if (bytes_written > 0 && bytes_written + word.size() > 32)
                {
                    bytes_written = 0;
                    os << "\n  " << std::setw(26) << "";
                }
                if (word == "")
                {
                    break;
                }
                os << word << " ";
                bytes_written += word.size() + 1;
            }
            os << "\n\n";
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
        using valid_options_t = std::tuple<ParentGroup, LongName, ShortName, Min, Max>;

        template <class... Args>
        multi_argument(detail::base_group& parent_group, Args&&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...)
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
        virtual void
            print_help(std::ostream& os) const override
        {
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
        using valid_options_t = std::tuple<ParentGroup, LongName, ShortName, Min, Max, Pattern>;

        template <class... Args>
        multi_pattern_argument(Args&&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , detail::helper::pick_option_with_default<Min>(1, args...)
                , detail::helper::pick_option_with_default<Max>(std::size_t(-1), args...)
                , detail::helper::pick_option_with_default<Pattern>("", args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for multi_pattern_argument");
            static_assert(detail::helper::has_type<Pattern, std::tuple<Args...>>::value, "po error static_assert: missing option \"pattern\" for multi_pattern_argument");
        }
        virtual ParseStatus
            try_parse_option(int* argc, const char*** argv) override
        {
            auto ret = base1_t::try_parse_option_argument(argc, argv);
            if (ret)
            {
                auto pa = base1_t::parsed_pattern_argument();
                auto key = detail::helper::lexical_cast<KeyT>(pa);
                _arguments[key] = *ret;
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
    class positional_argument
        : public detail::base_group
    {
    public:
        using type_t = std::string_view;
        using base1_t = detail::base_group;
        using valid_options_t = std::tuple<ParentGroup, LongName, ShortName, After, BindTo, Desc>;
        
        template <class... Args>
        positional_argument(Args&&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<ParentGroup>(
                     detail::helper::pick_option_with_default<After>(
                         detail::helper::pick_option_with_default<BindTo>(std::nullopt, args...), args...) , args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , false
                , detail::helper::pick_option_with_default<Desc>("", args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for positional_argument");
            if constexpr (detail::helper::has_type<After, std::tuple<Args...>>::value)
            {
                auto pg = detail::helper::pick_option_with_default<After>(std::nullopt, args...);
                pg->get().set_after(this);
            }
            else if constexpr (detail::helper::has_type<BindTo, std::tuple<Args...>>::value)
            {
                auto pg = detail::helper::pick_option_with_default<BindTo>(std::nullopt, args...);
                pg->get().set_bind_to(this);
            }
            else
            {
                auto pg = detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...);
                pg->get().set_multi_positional_argument(this);
            }
        }

        virtual ParseStatus
            try_parse_option(int narg, int* argc, const char*** argv) override
        {
            ParseStatus result = ParseStatus::NoMatch;
            if (*argc != 0 && (**argv)[0] == '-')
            {
                _argument = **argv;
                set_parsed_argument(_argument);
                (*argc)--;
                (*argv)++;
                if (bind_to() != nullptr)
                {
                    bind_to()->try_parse_option(narg, argc, argv);
                }
                result = ParseStatus::Match;
            }
            if (after() != nullptr)
            {
                after()->try_parse_option(narg, argc, argv);
            }
            return result;
        }
        operator type_t() const
        {
            return _argument;
        }
        void virtual
            notify() const override
        {
            if (parsed())
            {
                if (bind_to() != nullptr)
                {
                    bind_to()->notify();
                }
            }
            if (after() != nullptr)
            {
                after()->notify();
            }
        }

    private:
        type_t _argument;
    };
    class multi_positional_argument
        : public detail::base_option
    {
    public:
        using type_t = std::set<std::string_view>;
        using base1_t = detail::base_option;
        using valid_options_t = std::tuple<ParentGroup, Min, Max, After, BindTo, Desc, ArgName>;
        
        template <class... Args>
        multi_positional_argument(Args&&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<ParentGroup>(
                     detail::helper::pick_option_with_default<After>(
                         detail::helper::pick_option_with_default<BindTo>(std::nullopt, args...), args...) , args...)
                , ""
                , 0
                , ""
                , detail::helper::pick_option_with_default<Desc>("", args...)
                , detail::helper::pick_option_with_default<ArgName>("", args...))
            , _min(detail::helper::pick_option_with_default<Min>(1, args...))
            , _max(detail::helper::pick_option_with_default<Max>(std::size_t(-1), args...))
        {
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: unkown option given for positional_argument");
            static_assert((detail::helper::has_type<Args, valid_options_t>::value && ...)
                , "po error static_assert: argument ArgName required for positional_argument");
            if constexpr (detail::helper::has_type<After, std::tuple<Args...>>::value)
            {
                auto pg = detail::helper::pick_option_with_default<After>(std::nullopt, args...);
                pg->get().set_after(this);
            }
            else if constexpr (detail::helper::has_type<BindTo, std::tuple<Args...>>::value)
            {
                auto pg = detail::helper::pick_option_with_default<BindTo>(std::nullopt, args...);
                pg->get().set_bind_to(this);
            }
            else
            {
                auto pg = detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...);
                pg->get().set_multi_positional_argument(this);
            }
        }

        virtual ParseStatus
            try_parse_option(int narg, int* argc, const char*** argv) override
        {
            ParseStatus result = ParseStatus::NoMatch;
            if (*argc != 0)
            {
                while (*argc)
                {
                    std::string_view value = **argv;
                    set_parsed_argument(value);
                    _arguments.insert(value);
                    inc_parsed_count();
                    (*argc)--;
                    (*argv)++;
                }
                result = ParseStatus::Match;
            }
            return result;
        }
        operator type_t() const
        {
            return _arguments;
        }
        void virtual
            notify() const override
        {
            if (_min > parsed_count())
            {
                throw std::runtime_error("po error: too less arguments for positional arguments (min=" +
                    std::to_string(_min) + ")");
            }
            else if (_max < parsed_count())
            {
                throw std::runtime_error("po error: too many arguments for \"" +
                    std::string(name()) + "\" (max=" + std::to_string(_max) + ")");
            }
        }
        virtual void
            print_help(std::ostream& os, int argc, const char** argv) const override
        {
            std::string name_ = get_print_name_positional();
            detail::helper::print_2_columns(os, name_, desc());
            os << "\n\n";
        }

    private:
        type_t _arguments;
        std::size_t _min, _max;
    };
    class group
        : public detail::base_group
    {
    public:
        using base1_t = detail::base_group;
        using valid_options_t = std::tuple<ParentGroup, LongName, ShortName, Optional, After, BindTo, Desc>;

        template <class... Args>
        group(Args&&... args)
            : base1_t(
                  detail::helper::pick_option_with_default<ParentGroup>(
                     detail::helper::pick_option_with_default<After>(
                         detail::helper::pick_option_with_default<BindTo>(std::nullopt, args...), args...) , args...)
                , detail::helper::pick_option_with_default<LongName>("", args...)
                , detail::helper::pick_option_with_default<ShortName>(0, args...)
                , detail::helper::pick_option_with_default<Optional>(true, args...)
                , detail::helper::pick_option_with_default<Desc>("", args...))
        {
            static_assert((detail::helper::has_type_v<Args, valid_options_t> && ...)
                , "po error static_assert: unkown option given for group");
            if constexpr (detail::helper::has_type<After, std::tuple<Args...>>::value)
            {
                auto pg = detail::helper::pick_option_with_default<After>(std::nullopt, args...);
                pg->get().set_after(this);
            }
            else if constexpr (detail::helper::has_type<BindTo, std::tuple<Args...>>::value)
            {
                auto pg = detail::helper::pick_option_with_default<BindTo>(std::nullopt, args...);
                pg->get().set_bind_to(this);
            }
            else
            {
                auto pg = detail::helper::pick_option_with_default<ParentGroup>(std::nullopt, args...);
                pg->get().register_group(this);
            }
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

        sub_program(detail::parser& p, detail::base_group& bg, std::function<int(const typename Args::type_t&...)>&& program, const Args&... args)
            : _program(program)
            , _member{std::forward_as_tuple(args...)}
        {
            p.register_sub_program(this);
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
}

#define PO_INIT_MAIN_FILE_WITH_SUB_PROGRAM_SUPPORT(PARSER)      \
    int                                                         \
        main(int argc, const char** argv)                       \
    {                                                           \
        try                                                     \
        {                                                       \
            PARSER.parse_command_line(argc, argv);              \
        }                                                       \
        catch (const std::runtime_error& err)                   \
        {                                                       \
            std::cout << err.what();                            \
        }                                                       \
        PARSER.notify();                                        \
        return *PARSER.execute_main();                          \
    }
