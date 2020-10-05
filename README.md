# po
## Aim
The aim of the library is to make it possible to build some more complex command line tools like for example [nmcli](https://developer.gnome.org/NetworkManager/stable/nmcli.html) while keeping everything as simple as possible.
## Example
```C++
#include <iostream>
#include <filesystem>

#include <po.h>

// Initialize the library
// This macro creates the single tone instance of the parser which holds references to the created options and provides a main, if you want write your own main the macro PO_INIT_MAIN_FILE instead can be used, 
PO_INIT_MAIN_FILE_WITH_SUB_PROGRAM_SUPPORT()

// The library introduces the concept of sub programs
// It possible (but not mandatory) to create groups and assign them to a sub program which is invoked if the group gets parsed
// Those relationships result in a tree as shown here in this example:

// Here we build the tree
static po::multi_pattern_flag<std::string> pflag1(po::LongName("pflag1"), po::Pattern("pflag1-*"));
static po::multi_pattern_argument<std::string, double> parg1(po::LongName("parg1"), po::Pattern("parg1-*"));
static po::group group1(po::LongName("group1"));
static po::group group2(po::ParentGroup(group1), po::LongName("group2"));
static po::group group3(po::ParentGroup(group2), po::LongName("group3"));
static po::optional_argument<int> arg1(po::ParentGroup(group1), po::LongName("arg1"));
static po::argument<int> arg2(po::ParentGroup(group1), po::LongName("arg2"));
static po::argument<int> arg3(po::ParentGroup(group2), po::LongName("arg3"));
static po::argument<int> arg4(po::ParentGroup(group3), po::LongName("arg4"), po::Def<int>(5));
static po::flag flag1(po::ParentGroup(group3), po::LongName("flag1"), po::ShortName('f'));
static po::multi_argument<std::filesystem::path> marg1(po::ParentGroup(group3), po::LongName("marg1"), po::Min(1), po::Max(10));
static po::help main_help({});
static po::help goup1_help(po::ParentGroup(group1), po::Header("group1 help"));

// Two main functions
int main_sub()
{
    std::cout << "=============== main_sub ===============" << std::endl;
    return 0;
}
int main_sub_group3(
      const std::map<std::string, double>& parg1
    , const std::optional<int>& arg1
    , const int& arg2
    , const int& arg3
    , const int& arg4
    , const bool& flag1
    , const std::vector<std::filesystem::path>& marg1)
{
    std::cout << "=============== main_sub_group3 ===============" << std::endl;
    for (const auto&[key, value] : parg1)
    {
        std::cout << "parg1-" << key << "=" << value << std::endl;
    }
    if (arg1) std::cout << "arg1=" << *arg1 << std::endl;
    std::cout << "arg2=" << arg2 << std::endl;
    std::cout << "arg3=" << arg3 << std::endl;
    std::cout << "arg4=" << arg4 << std::endl;
    std::cout << std::boolalpha << "flag1=" << flag1 << std::endl;
    std::size_t i = 1;
    for (const auto& p : marg1)
    {
        std::cout << "marg1." << i << "=" << p << std::endl;
        i++;
    }
    return 0;
}
// Register the main functions
// main_sub gets invoked if the rout_group gets parsed (so main_sub behaves like the classic main function)
static po::sub_program sp_default(main_sub);
// main_sub_group1 only gets invoekd if group1 gets parsed, the arguments passed to the function will be passed to main_sub_group1
static po::sub_program sp1(group3, main_sub_group3, parg1, arg1, arg2, arg3, arg4, flag1, marg1);
```
### Example program invocation
```
$ ./program --pflag1-can0 --parg1-asdf=1.1 --parg1-qwer=2.2 group1 --arg2=2 group2 --arg3=3 group3 --flag1 --marg1=file1.txt --marg1=file2.txt
=============== main_sub ===============
=============== main_sub_group3 ===============
parg1-asdf=1.1
parg1-qwer=2.2
arg2=2
arg3=3
arg4=5
flag1=true
marg1.1="file1.txt"
marg1.2="file2.txt"
```
