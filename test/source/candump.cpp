
#include <po.h>

PO_INIT_MAIN_FILE();
static po::argument<char> timestamp(po::ShortName('t'), po::Def<char>('a'));
static po::flag hardware_timestamp(po::ShortName('H'));
static po::multi_flag increment_color_level(po::ShortName('c'));
static po::flag binary_output(po::ShortName('i'));
static po::flag ascii_output(po::ShortName('a'));
static po::flag swap_byte_order(po::ShortName('S'));
static po::argument<char> silent_mode(po::ShortName('s'), po::Def<char>('0'));
static po::optional_argument<std::string> bridge(po::ShortName('b'));
static po::optional_argument<std::string> bridge_without_loop_back(po::ShortName('B'));
static po::argument<std::size_t> bridge_delay(po::ShortName('u'), po::Def<std::size_t>(10));
static po::flag log_to_file(po::ShortName('l'));
static po::flag log_to_stdout(po::ShortName('L'));
static po::optional_argument<std::size_t> terminate_after(po::ShortName('n'));
static po::optional_argument<std::size_t> socket_receive_buffer_size(po::ShortName('r'));
static po::flag do_not_exit_on_device_down(po::ShortName('D'));
static po::flag monitor_dropped_frames(po::ShortName('d'));
static po::flag dump_can_errors_human_readable(po::ShortName('e'));
static po::flag print_extra_msg_info(po::ShortName('x'));
static po::flag terminate_after_msescs(po::ShortName('T'));
static po::multi_positional_argument can_interfaces;

int main(int argc, const char** argv)
{
    int result = 0;
    po::command_line::parse(argc, argv);
    po::command_line::notify();
    return result;
}
