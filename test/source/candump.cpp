
#include <po.h>
#include <iostream>

static po::detail::parser parser;
static po::argument<char> timestamp{po::ParentGroup(parser), po::ArgName("timestamp"), po::ShortName('t'), po::Def<char>('a'), po::Desc("(timestamp: (a)bsolute/(d)elta/(z)ero/(A)bsolute w date)")};
static po::flag hardware_timestamp{po::ParentGroup(parser), po::ShortName('H'), po::Desc("(read hardware timestamps instead of system timestamps)")};
static po::multi_flag increment_color_level{po::ParentGroup(parser), po::ShortName('c'), po::Min(0), po::Desc("(increment color mode level)")};
static po::flag binary_output{po::ParentGroup(parser), po::ShortName('i'), po::Desc("(binary output - may exceed 80 chars/line)")};
static po::flag ascii_output{po::ParentGroup(parser), po::ShortName('a'), po::Desc("(enable additional ASCII output)")};
static po::flag swap_byte_order{po::ParentGroup(parser), po::ShortName('S'), po::Desc("(swap byte order in printed CAN data[] - marked with '`' )")};
static po::argument<char> silent_mode{po::ParentGroup(parser), po::ShortName('s'), po::Def<char>('0'), po::Desc("(silent mode - 0: off (default) 1: animation 2: silent)")};
static po::optional_argument<std::string> bridge{po::ParentGroup(parser), po::ShortName('b'), po::Desc("(bridge mode - send received frames to <can>)")};
static po::optional_argument<std::string> bridge_without_loop_back{po::ParentGroup(parser), po::ShortName('B'), po::Desc("(bridge mode - like '-b' with disabled loopback)")};
static po::argument<std::size_t> bridge_delay{po::ParentGroup(parser), po::ShortName('u'), po::Def<std::size_t>(10), po::Desc("(delay bridge forwarding by <usecs> microseconds)")};
static po::flag log_to_file{po::ParentGroup(parser), po::ShortName('l'), po::Desc("(log CAN-frames into file. Sets '-s 2' by default)")};
static po::flag log_to_stdout{po::ParentGroup(parser), po::ShortName('L'), po::Desc("(use log file format on stdout)")};
static po::optional_argument<std::size_t> terminate_after{po::ParentGroup(parser), po::ShortName('n'), po::Desc("(terminate after receiption of <count> CAN frames)")};
static po::optional_argument<std::size_t> socket_receive_buffer_size{po::ParentGroup(parser), po::ShortName('r'), po::Desc("(set socket receive buffer to <size>)")};
static po::flag do_not_exit_on_device_down{po::ParentGroup(parser), po::ShortName('D'), po::Desc("(Don't exit if a \"detected\" can device goes down.)")};
static po::flag monitor_dropped_frames{po::ParentGroup(parser), po::ShortName('d'), po::Desc("(monitor dropped CAN frames)")};
static po::flag dump_can_errors_human_readable{po::ParentGroup(parser), po::ShortName('e'), po::Desc("(dump CAN error frames in human-readable format)")};
static po::flag print_extra_msg_info{po::ParentGroup(parser), po::ShortName('x'), po::Desc("(print extra message infos, rx/tx brs esi)")};
static po::optional_argument<std::size_t> terminate_after_msescs{po::ParentGroup(parser), po::ShortName('T'), po::Desc("(terminate after <msecs> without any reception)")};
static po::multi_positional_argument can_interfaces{
      po::ParentGroup(parser)
    , po::Desc(
        "Up to 16 CAN interfaces with optional filter sets can be specified "
        "on the commandline in the form: <ifname>[,<filter>]*"
        "\n\n"
        "Filters:\n"
        "Comma separated filters can be specified for each given CAN interface.\n\n"
        "<can_id>:<can_mask>\n\n"
        "(matches when <received_can_id> & mask == can_id & mask)\n\n"
        "<can_id>~<can_mask>\n\n"
        "(matches when <received_can_id> & mask != can_id & mask)\n\n"
        "#<error_mask>\n\n"
        "(set error frame filter, see include/linux/can/error.h)\n\n"
        "[j|J]\n\n"
        "(join the given CAN filters - logical AND semantic)\n\n"
        "CAN IDs, masks and data content are given and expected in hexadecimal values.  When can_id"
        "and  can_mask  are  both  8  digits, they are assumed to be 29 bit EFF.  Without any given"
        "filter all data frames are received ('0:0' default filter).\n\n"
        "Use interface name 'any' to receive from all CAN interfaces.")
    , po::ArgName("ifname[,<filter>*]")};

int main(int argc, const char** argv)
{
    parser.get_main_group()->print_help(std::cout, argc, argv);
    int result = 0;
    parser.parse_command_line(argc, argv);
    parser.notify();
    return result;
}
