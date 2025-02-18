// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

#include "test_utils.hpp"
#include <boost/property_tree/ini_parser.hpp>
#include <sstream>

using namespace boost::property_tree;

///////////////////////////////////////////////////////////////////////////////
// Test data

// Correct data
const char *ok_data_1 = 
    "\n"
    "; Comment section\n"
    "[Section1]\n"
    "\t   \t; Comment\n"
    "  Key1=Data1\n"
    "            \n"
    "   Key2   =   Data2\n"
    "Key 3     =      Data 3  \n"
    "Key4=Data4 #comment\n"
    "[Section2] ;Comment\n"
    "\t   \tKey1=Data4\n";

// Correct data
const char *ok_data_2 = 
    "[Section1]\n"
    "Key1=Data1";              // No eol

// Correct data
const char *ok_data_3 = 
    "";

// Correct data
const char *ok_data_4 = 
    ";Comment";

// Correct data
const char *ok_data_5 = 
    "# Comment\n"
    "Key1=Data1\n"             // No section
    "Key2=Data2\n";

// Treat # as section comment.
const char *ok_data_6 =
    "# Comment\n"
    "[Section1]\n"
    "Key1=Data1\n";

// Treat # as key comment.
const char *ok_data_7 =
    "[Section1]\n"
    "# Comment\n"
    "Key1=Data1\n";

// Multiline comments
const char *ok_data_8 =
    "# Comment section string1\n"
    "# Comment section string2\n"
    "[Section1]\n"
    "# Comment key string1\n"
    "# Comment key string2\n"
    "Key1=Data1\n";

// Erroneous data
const char *error_data_1 =
    "[Section1]\n"
    "Key1\n"                   // No equals sign
    "Key2=Data2";

// Erroneous data
const char *error_data_2 = 
    "[Section1]\n"
    "Key1=Data1\n"
    "=Data2\n";                // No key

// Erroneous data
const char *error_data_3 =
    "# This is a comment\n"
    "This isn't a comment\n"    //No # or ;
    "[Section1]\n"
    "Key1=Data1\n";

struct ReadFunc
{
    template<class Ptree>
    void operator()(const std::string &filename, Ptree &pt) const
    {
        read_ini(filename, pt);
    }
};

struct WriteFunc
{
    template<class Ptree>
    void operator()(const std::string &filename, const Ptree &pt) const
    {
        write_ini(filename, pt);
    }
};

void test_erroneous_write(const boost::property_tree::ptree &pt)
{
    std::stringstream stream;
    try
    {
        write_ini(stream, pt);
        BOOST_ERROR("No required exception thrown");
    }
    catch (ini_parser_error &e)
    {
        (void)e;
    }
    catch (...)
    {
        BOOST_ERROR("Wrong exception type thrown");
    }
}

template<class Ptree>
void test_ini_parser()
{
    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_1, NULL,
        "testok1.ini", NULL, "testok1out.ini", 10, 59, 65
    );
    
    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_2, NULL,
        "testok2.ini", NULL, "testok2out.ini", 3, 5, 12
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_3, NULL,
        "testok3.ini", NULL, "testok3out.ini", 1, 0, 0
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_4, NULL,
        "testok4.ini", NULL, "testok4out.ini", 1, 0, 0
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_5, NULL,
        "testok5.ini", NULL, "testok5out.ini", 4, 18, 18
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_6, NULL,
        "testok6.ini", NULL, "testok6out.ini", 4, 13, 30
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_7, NULL,
        "testok7.ini", NULL, "testok7out.ini", 4, 13, 22
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_8, NULL,
        "testok8.ini", NULL, "testok8out.ini", 5, 95, 40
    );

    generic_parser_test_error<Ptree, ReadFunc, WriteFunc, ini_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_1, NULL,
        "testerr1.ini", NULL, "testerr1out.ini", 2
    );

    generic_parser_test_error<Ptree, ReadFunc, WriteFunc, ini_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_2, NULL,
        "testerr2.ini", NULL, "testerr2out.ini", 3
    );

    generic_parser_test_error<Ptree, ReadFunc, WriteFunc, ini_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_3, NULL,
        "testerr3.ini", NULL, "testerr3out.ini", 2
    );
}

void test_unmappable_trees()
{
    // Test too deep ptrees
    {
        ptree pt;
        pt.put_child("section.key.bogus", ptree());
        test_erroneous_write(pt);
    }

    // Test duplicate sections
    {
        ptree pt;
        pt.push_back(std::make_pair("section", ptree()));
        pt.push_back(std::make_pair("section", ptree()));
        test_erroneous_write(pt);
    }

    // Test duplicate keys
    {
        ptree pt;
        ptree &child = pt.put_child("section", ptree());
        child.push_back(std::make_pair("key", ptree()));
        child.push_back(std::make_pair("key", ptree()));
        test_erroneous_write(pt);
    }

    // Test mixed data and children.
    {
        ptree pt;
        ptree &child = pt.put_child("section", ptree("value"));
        child.push_back(std::make_pair("key", ptree()));
        child.push_back(std::make_pair("key", ptree()));
        test_erroneous_write(pt);
    }
}

void test_other_trees()
{
    // Top-level keys must be written before any section.
    {
        ptree pt;
        pt.put("section.innerkey", "v1");
        pt.put("nosection", "v2");
        std::stringstream s;
        write_ini(s, pt);
        s.clear();
        s.seekg(0, std::ios_base::beg);
        ptree result;
        read_ini(s, result);
        BOOST_TEST(result.get("section.innerkey", "bad") == "v1");
        BOOST_TEST(result.get("nosection", "bad") == "v2");
    }
}

void test_empty_name_section()
{
  std::stringstream stringStreamInput;
  stringStreamInput << "[]" << std::endl;
  stringStreamInput << "empty_section_key = 1" << std::endl;
  stringStreamInput <<  std::endl;
  stringStreamInput << "[section]" << std::endl;
  stringStreamInput << "section_key = 1" << std::endl;
  stringStreamInput.clear();
  stringStreamInput.seekg(0, std::ios_base::beg);
  ptree result;
  read_ini(stringStreamInput, result);
  BOOST_TEST(result.get(".empty_section_key", "bad") == "1");
  BOOST_TEST(result.get("section.section_key", "bad") == "1");
  std::stringstream stringStreamOutput;
  write_ini(stringStreamOutput, result);
  std::string input = stringStreamInput.str();
  std::string output = stringStreamOutput.str();
  BOOST_TEST(output == input);
}

int main()
{
    test_ini_parser<ptree>();
    test_ini_parser<iptree>();
#ifndef BOOST_NO_CWCHAR
    test_ini_parser<wptree>();
    test_ini_parser<wiptree>();
#endif

    test_unmappable_trees();
    test_other_trees();
    test_empty_name_section();
    return boost::report_errors();

}
