
#include <po.h>
#include <iostream>

static po::detail::parser parser;
static po::flag ask{parser, po::LongName("ask"), po::ShortName('a')};
static po::argument<std::string> colors{parser, po::LongName("colors"), po::ShortName('c'), po::Def<std::string>("auto")};
static po::flag complete_args{parser, po::LongName("complete-args")};
static po::argument<std::string> escape{parser, po::LongName("escape"), po::ShortName('e'), po::Def<std::string>("yes")};
static po::argument<std::string> fields{parser, po::LongName("fields"), po::ShortName('f'), po::Def<std::string>("common")};
static po::optional_argument<std::string> get_values{parser, po::LongName("get-values"), po::ShortName('g')};
static po::argument<std::string> mode{parser, po::LongName("mode"), po::ShortName('m'), po::Def<std::string>("tabular")};
static po::flag pretty{parser, po::LongName("pretty"), po::ShortName('p')};
static po::flag show_secrets{parser, po::LongName("show-secrets"), po::ShortName('s')};
static po::flag terse{parser, po::LongName("terse"), po::ShortName('t')};
static po::flag version{parser, po::LongName("version"), po::ShortName('s')};
static po::optional_argument<std::size_t> wait{parser, po::LongName("wait"), po::ShortName('w')};

static po::group general{parser, po::LongName("general"), po::Desc(
    "Use this command to show NetworkManager status and permissions. "
    "You can also get and change system hostname, as well as NetworkManager logging level and domains.")};
static po::group networking{parser, po::LongName("networking"), po::Desc(
    "Query NetworkManager networking status, enable and disable networking.")};
static po::group radio{parser, po::LongName("radio"), po::Desc("Show radio switches status, or enable and disable the switches.")};
static po::group connection{parser, po::LongName("connection"), po::Desc(
    "NetworkManager stores all network configuration as \"connections\", "
    "which are collections of data (Layer2 details, IP addressing, etc.) "
    "that describe how to create or connect to a network. A connection is "
    "\"active\" when a device uses that connection's configuration to create "
    "or connect to a network. There may be multiple connections that apply "
    "to a device, but only one of them can be active on that device at any "
    "given time. The additional connections can be used to allow quick "
    "switching between different networks and configurations.\n\n"
    "Consider a machine which is usually connected to a DHCP-enabled network, "
    "but sometimes connected to a testing network which uses static IP "
    "addressing. Instead of manually reconfiguring eth0 each time the network "
    "is changed, the settings can be saved as two connections which both "
    "apply to eth0, one for DHCP (called default) and one with the static "
    "addressing details (called testing). When connected to the DHCP-enabled "
    "network the user would run nmcli con up default , and when connected to "
    "the static network the user would run nmcli con up testing.")};
static po::group device{parser, po::LongName("device"), po::Desc("Show and manage network interfaces.")};
static po::group agent{parser, po::LongName("agent"), po::Desc("Run nmcli as a NetworkManager secret agent, or polkit agent.")};
static po::group monitor{parser, po::LongName("monitor"), po::Desc(
    "Observe NetworkManager activity. Watches for changes in connectivity state, devices or connection profiles.\n\n"
    "See also nmcli connection monitor and nmcli device monitor to watch for changes in certain devices or connections.")};

// TODO: insert positional_argument into seperate list in a group so that it is possible to keep order and skip optional arguments
// the aim of that is to make it possible to parse something like:
// connection up [ id | uuid | path ] ID [ifname ifname] [ap BSSID] [passwd-file file]

static po::group connection_show{connection, po::LongName("show")};
static po::flag connection_show_active{connection_show, po::LongName("active")};
static po::optional_argument<std::string> connection_show_pos{connection_show, po::LongName("order")};
static po::positional_argument connection_show_id{connection_show};
static po::multi_positional_argument connection_show_id_id{po::After(connection_show_id)};

static po::group connection_up{connection, po::LongName("up")};
static po::positional_argument connection_up_id_selector{po::After(connection_up)};
static po::positional_argument connection_up_id{po::After(connection_up_id_selector)};
static po::group connection_up_id_ifname{po::After(connection_up_id), po::LongName("ifname")};
static po::positional_argument connection_up_id_ifname_ifname{po::BindTo(connection_up_id_ifname)};
static po::group connection_up_ap{po::After(connection_up_id_ifname_ifname), po::LongName("ap")};
static po::positional_argument connection_up_ap_ap{po::BindTo(connection_up_ap)};
static po::group connection_up_passwd_file{po::After(connection_up_ap_ap), po::LongName("passwd-file")};
static po::positional_argument connection_up_passwd_file_file{po::BindTo(connection_up_passwd_file)};

static po::group connection_down{connection, po::LongName("down")};
static po::positional_argument connection_down_id_selector{po::After(connection_down)};
static po::positional_argument connection_down_id{po::After(connection_down_id_selector)};

int main_connection_show()
{
    return 0;
}
int main_connection_up()
{
    parser.get_main_group()->print_help(std::cout, parser.get_argc(), parser.get_argv());
    return 0;
}

static po::sub_program sub_connection_show(parser, connection_show, main_connection_show);
static po::sub_program sub_connection_up(parser, connection_up, main_connection_up);

PO_INIT_MAIN_FILE_WITH_SUB_PROGRAM_SUPPORT(parser);
