
#include <po.h>
#include <iostream>

static po::detail::parser parser;
static po::flag ask{po::ParentGroup(parser), po::LongName("ask"), po::ShortName('a')};
static po::argument<std::string> colors{po::ParentGroup(parser), po::LongName("colors"), po::ShortName('c'), po::Def<std::string>("auto")};
static po::flag complete_args{po::ParentGroup(parser), po::LongName("complete-args")};
static po::argument<std::string> escape{po::ParentGroup(parser), po::LongName("escape"), po::ShortName('e'), po::Def<std::string>("yes")};
static po::argument<std::string> fields{po::ParentGroup(parser), po::LongName("fields"), po::ShortName('f'), po::Def<std::string>("common")};
static po::optional_argument<std::string> get_values{po::ParentGroup(parser), po::LongName("get-values"), po::ShortName('g')};
static po::argument<std::string> mode{po::ParentGroup(parser), po::LongName("mode"), po::ShortName('m'), po::Def<std::string>("tabular")};
static po::flag pretty{po::ParentGroup(parser), po::LongName("pretty"), po::ShortName('p')};
static po::flag show_secrets{po::ParentGroup(parser), po::LongName("show-secrets"), po::ShortName('s')};
static po::flag terse{po::ParentGroup(parser), po::LongName("terse"), po::ShortName('t')};
static po::flag version{po::ParentGroup(parser), po::LongName("version"), po::ShortName('s')};
static po::optional_argument<std::size_t> wait{po::ParentGroup(parser), po::LongName("wait"), po::ShortName('w')};

static po::group general{po::ParentGroup(parser), po::LongName("general"), po::Desc(
    "Use this command to show NetworkManager status and permissions. "
    "You can also get and change system hostname, as well as NetworkManager logging level and domains.")};
static po::group networking{po::ParentGroup(parser), po::LongName("networking"), po::Desc(
    "Query NetworkManager networking status, enable and disable networking.")};
static po::group radio{po::ParentGroup(parser), po::LongName("radio"), po::Desc("Show radio switches status, or enable and disable the switches.")};
static po::group connection{po::ParentGroup(parser), po::LongName("connection"), po::Desc(
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
static po::group device{po::ParentGroup(parser), po::LongName("device"), po::Desc("Show and manage network interfaces.")};
static po::group agent{po::ParentGroup(parser), po::LongName("agent"), po::Desc("Run nmcli as a NetworkManager secret agent, or polkit agent.")};
static po::group monitor{po::ParentGroup(parser), po::LongName("monitor"), po::Desc(
    "Observe NetworkManager activity. Watches for changes in connectivity state, devices or connection profiles.\n\n"
    "See also nmcli connection monitor and nmcli device monitor to watch for changes in certain devices or connections.")};

// TODO: insert positional_argument into seperate list in a group so that it is possible to keep order and skip optional arguments
// the aim of that is to make it possible to parse something like:
// connection up [ id | uuid | path ] ID [ifname ifname] [ap BSSID] [passwd-file file]

static po::group connection_show{po::ParentGroup(connection), po::LongName("show")};
static po::flag connection_show_active{po::ParentGroup(connection_show), po::LongName("active")};
static po::optional_argument<std::string> connection_show_pos{po::ParentGroup(connection_show), po::LongName("order")};
static po::positional_argument connection_show_id{po::ParentGroup(connection_show)};
static po::multi_positional_argument connection_show_id_id{po::After(connection_show_id)};

static po::group connection_up{po::ParentGroup(connection), po::LongName("up")};
static po::positional_argument connection_up_id_selector{po::LongName("{id | uuid | path}"), po::After(connection_up)};
static po::positional_argument connection_up_id{po::LongName("<ID>"), po::After(connection_up_id_selector)};
static po::group connection_up_id_ifname{po::After(connection_up_id), po::LongName("ifname")};
static po::positional_argument connection_up_id_ifname_ifname{po::LongName("<ifname>"), po::BindTo(connection_up_id_ifname)};
static po::group connection_up_ap{po::After(connection_up_id_ifname_ifname), po::LongName("ap")};
static po::positional_argument connection_up_ap_ap{po::LongName("<BSSID>"), po::BindTo(connection_up_ap)};
static po::group connection_up_passwd_file{po::After(connection_up_ap_ap), po::LongName("passwd-file")};
static po::positional_argument connection_up_passwd_file_file{po::LongName("<file>"), po::BindTo(connection_up_passwd_file)};

static po::group connection_down{po::ParentGroup(connection), po::LongName("down")};
static po::positional_argument connection_down_id_selector{po::After(connection_down)};
static po::positional_argument connection_down_id{po::After(connection_down_id_selector)};

static po::help help{po::ParentGroup(parser)};
static po::help help_connection_up{po::ParentGroup(connection_up)};

int main_connection_show()
{
    return 0;
}
int main_connection_up()
{
    return 0;
}

static po::sub_program sub_connection_show(parser, connection_show, main_connection_show);
static po::sub_program sub_connection_up(parser, connection_up, main_connection_up);

PO_INIT_MAIN_FILE_WITH_SUB_PROGRAM_SUPPORT(parser);
