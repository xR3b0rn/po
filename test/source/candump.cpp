
#include <po.h>
#include <iostream>

PO_INIT_MAIN_FILE();
static po::detail::parser parser;
static po::argument<char> timestamp{parser, po::ArgName("timestamp"), po::ShortName('t'), po::Def<char>('a'), po::Desc("(timestamp: (a)bsolute/(d)elta/(z)ero/(A)bsolute w date)")};
static po::flag hardware_timestamp{parser, po::ShortName('H'), po::Desc("(read hardware timestamps instead of system timestamps)")};
static po::multi_flag increment_color_level{parser, po::ShortName('c'), po::Min(0), po::Desc("(increment color mode level)")};
static po::flag binary_output{parser, po::ShortName('i'), po::Desc("(binary output - may exceed 80 chars/line)")};
static po::flag ascii_output{parser, po::ShortName('a'), po::Desc("(enable additional ASCII output)")};
static po::flag swap_byte_order{parser, po::ShortName('S'), po::Desc("(swap byte order in printed CAN data[] - marked with '`' )")};
static po::argument<char> silent_mode{parser, po::ShortName('s'), po::Def<char>('0'), po::Desc("(silent mode - 0: off (default) 1: animation 2: silent)")};
static po::optional_argument<std::string> bridge{parser, po::ShortName('b'), po::Desc("(bridge mode - send received frames to <can>)")};
static po::optional_argument<std::string> bridge_without_loop_back{parser, po::ShortName('B'), po::Desc("(bridge mode - like '-b' with disabled loopback)")};
static po::argument<std::size_t> bridge_delay{parser, po::ShortName('u'), po::Def<std::size_t>(10), po::Desc("(delay bridge forwarding by <usecs> microseconds)")};
static po::flag log_to_file{parser, po::ShortName('l'), po::Desc("(log CAN-frames into file. Sets '-s 2' by default)")};
static po::flag log_to_stdout{parser, po::ShortName('L'), po::Desc("(use log file format on stdout)")};
static po::optional_argument<std::size_t> terminate_after{parser, po::ShortName('n'), po::Desc("(terminate after receiption of <count> CAN frames)")};
static po::optional_argument<std::size_t> socket_receive_buffer_size{parser, po::ShortName('r'), po::Desc("(set socket receive buffer to <size>)")};
static po::flag do_not_exit_on_device_down{parser, po::ShortName('D'), po::Desc("(Don't exit if a \"detected\" can device goes down.)")};
static po::flag monitor_dropped_frames{parser, po::ShortName('d'), po::Desc("(monitor dropped CAN frames)")};
static po::flag dump_can_errors_human_readable{parser, po::ShortName('e'), po::Desc("(dump CAN error frames in human-readable format)")};
static po::flag print_extra_msg_info{parser, po::ShortName('x'), po::Desc("(print extra message infos, rx/tx brs esi)")};
static po::optional_argument<std::size_t> terminate_after_msescs{parser, po::ShortName('T'), po::Desc("(terminate after <msecs> without any reception)")};
static po::multi_positional_argument can_interfaces{parser, po::Desc("Up to 16 CAN interfaces with optional filter sets can be specified on the commandline in the form: <ifname>[,<filter>]*"), po::ArgName("ifname[,<filter>*]")};

int main(int argc, const char** argv)
{
    parser.get_main_group()->print_help(std::cout, argc, argv);
    int result = 0;
    parser.parse_command_line(argc, argv);
    parser.notify();
    return result;
}
