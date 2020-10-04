
#include <po.h>

PO_INIT_MAIN_FILE_WITH_SUB_PROGRAM_SUPPORT();

static po::flag ask(po::LongName("ask"), po::ShortName('a'));
static po::argument<std::string> colors(po::LongName("colors"), po::ShortName('c'), po::Def<std::string>("auto"));
static po::flag complete_args(po::LongName("complete-args"));
static po::argument<std::string> escape(po::LongName("escape"), po::ShortName('e'), po::Def<std::string>("yes"));
static po::argument<std::string> fields(po::LongName("fields"), po::ShortName('f'), po::Def<std::string>("common"));
static po::optional_argument<std::string> get_values(po::LongName("get-values"), po::ShortName('g'));
static po::argument<std::string> mode(po::LongName("mode"), po::ShortName('m'), po::Def<std::string>("tabular"));
static po::flag pretty(po::LongName("pretty"), po::ShortName('p'));
static po::flag show_secrets(po::LongName("show-secrets"), po::ShortName('s'));
static po::flag terse(po::LongName("terse"), po::ShortName('t'));
//static po::flag version(po::LongName("version"), po::ShortName('s'));
static po::optional_argument<std::size_t> wait(po::LongName("wait"), po::ShortName('w'));

static po::group connection(po::LongName("connection"));

// TODO: insert positional_argument into seperate list in a group so that it is possible to keep order and skip optional arguments
// the aim of that is to make it possible to parse something like:
// connection up [ id | uuid | path ] ID [ifname ifname] [ap BSSID] [passwd-file file]

static po::group connection_show(po::ParentGroup(connection), po::LongName("show"));
static po::flag connection_show_active(po::ParentGroup(connection_show), po::LongName("active"));
static po::optional_argument<std::string> connection_show_pos{po::ParentGroup(connection_show), po::LongName("order")};
static po::positional_argument connection_show_id{po::ParentGroup(connection_show)};
static po::multi_positional_argument connection_show_id_id{po::After(connection_show_id)};

static po::group connection_up(po::ParentGroup(connection), po::LongName("up"));
static po::positional_argument connection_up_id_selector{po::After(connection_up)};
static po::positional_argument connection_up_id{po::After(connection_up_id_selector)};
static po::group connection_up_id_ifname{po::After(connection_up_id), po::LongName("ifname")};
static po::positional_argument connection_up_id_ifname_ifname{po::BindTo(connection_up_id_ifname)};
static po::group connection_up_ap{po::After(connection_up_id_ifname_ifname), po::LongName("ap")};
static po::positional_argument connection_up_ap_ap{po::BindTo(connection_up_ap)};
static po::group connection_up_passwd_file{po::After(connection_up_ap_ap), po::LongName("passwd-file")};
static po::positional_argument connection_up_passwd_file_file{po::BindTo(connection_up_passwd_file)};

static po::group connection_down(po::ParentGroup(connection), po::LongName("down"));
static po::positional_argument connection_down_id_selector{po::After(connection_down)};
static po::positional_argument connection_down_id{po::After(connection_down_id_selector)};

int main_connection_show()
{
    return 0;
}
int main_connection_up()
{
    return 0;
}

static po::sub_program sub_connection_show(connection_show, main_connection_show);
static po::sub_program sub_connection_up(connection_up, main_connection_up);