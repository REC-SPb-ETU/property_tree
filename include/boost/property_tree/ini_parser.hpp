// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
// Copyright (C) 2009 Sebastian Redl
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_INI_PARSER_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_INI_PARSER_HPP_INCLUDED

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/detail/ptree_utils.hpp>
#include <boost/property_tree/detail/file_parser_error.hpp>
#include <boost/optional.hpp>

#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <locale>

namespace boost { namespace property_tree { namespace ini_parser
{

    /**
     * Determines whether the @c flags are valid for use with the ini_parser.
     * @param flags value to check for validity as flags to ini_parser.
     * @return true if the flags are valid, false otherwise.
     */
    inline bool validate_flags(int flags)
    {
        return flags == 0;
    }

    /** Indicates an error parsing INI formatted data. */
    class ini_parser_error: public file_parser_error
    {
    public:
        /**
         * Construct an @c ini_parser_error
         * @param message Message describing the parser error.
         * @param filename The name of the file being parsed containing the
         *                 error.
         * @param line The line in the given file where an error was
         *             encountered.
         */
        ini_parser_error(const std::string &message,
                         const std::string &filename,
                         unsigned long line)
            : file_parser_error(message, filename, line)
        {
        }
    };

    namespace detail
    {
        template<typename StringType>
        inline StringType make_string(const char* string)
        {
            std::basic_ostringstream<typename StringType::value_type> stm;
            stm << string;
            return stm.str();
        }

        template<typename StringType>
        inline StringType comment_key()
        {
            return make_string<StringType>("inicomment");
        }

        template<typename StringType>
        inline StringType section_comment_key()
        {
            return make_string<StringType>("inicomment_section");
        }

        template<typename CharType>
        inline CharType comment_start_character()
        {
            std::basic_ostringstream<CharType> stm;
            return stm.widen('#');
        }

        template<class Ptree>
        void check_dupes(const Ptree &pt)
        {
            if(pt.size() <= 1)
                return;
            const typename Ptree::key_type *lastkey = 0;
            typename Ptree::const_assoc_iterator it = pt.ordered_begin(),
                                                 end = pt.not_found();
            lastkey = &it->first;
            for(++it; it != end; ++it) {
                if(*lastkey == it->first)
                    BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                        "duplicate key", "", 0));
                lastkey = &it->first;
            }
        }

        template<class Ptree>
        void write_comment(std::basic_ostream<
                               typename Ptree::key_type::value_type
                           > &stream,
                           const typename Ptree::key_type& comment,
                           const typename Ptree::key_type::value_type& commentStart)
        {
            typedef typename Ptree::key_type::value_type Ch;
            if (comment.empty())
                return;
            typename Ptree::key_type line;
            bool hadNonEmptyLine = false;
            for (size_t i = 0; i < comment.size(); i++)
            {
                if (comment[i] == Ch('\n'))
                {
                    if (!line.empty() || hadNonEmptyLine)
                    {
                        // dump comment line
                        stream << commentStart << line << Ch('\n');
                    }
                    else
                    {
                        // preceding empty lines are output without the commentStart
                        // dump comment line
                        stream << Ch('\n');
                    }
                    line.clear();
                    hadNonEmptyLine |= (!line.empty());
                }
                else if (comment[i] != Ch('\r'))
                {
                    // Ignore '\r' (for Windows!)
                    line += comment[i];
                }
            }
            if (!line.empty() || hadNonEmptyLine)
            {
                // dump comment line
                stream << commentStart << line << Ch('\n');
            }
            else
            {
                // preceding empty lines are output without the commentStart
                // dump comment line
                stream << Ch('\n');
            }
        }

        template <typename Ptree>
        void write_keys(std::basic_ostream<
                                      typename Ptree::key_type::value_type
                                  > &stream,
                                  const Ptree& pt,
                                  bool throw_on_children,
                                  const typename Ptree::key_type& commentKey,
                                  const typename Ptree::key_type::value_type& commentStart,
                                  const typename boost::optional<typename Ptree::key_type>& sectionName = {})
        {
            typedef typename Ptree::key_type::value_type Ch;
            typedef typename Ptree::key_type Str;
            const Str sectionCommentKey = section_comment_key<Str>();
            for (typename Ptree::const_iterator it = pt.begin(), end = pt.end();
                 it != end; ++it)
            {
                // check for existence of comment node
                boost::optional<Str> comment = it->second.template get_optional<Str>(commentKey);
                boost::optional<Str> sectionComment = it->second.template get_optional<Str>(sectionCommentKey);

                const typename Ptree::size_type commentCount = bool(comment) + bool(sectionComment);
                if (it->second.size() != commentCount) {
                    //only two depth-levels are allowed in INI-files ... but we also have to filter out the additional .comment nodes
                    if (throw_on_children) {
                        BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                            "ptree is too deep (only two depth steps allowed in INI files)", "", 0));
                    }
                    continue;
                }

                // first parameter of section, write section
                if (it == pt.begin() && sectionName)
                {
                    // empty lines in front of a new section to better separate it from other sections
                    if (stream.tellp() != 0)
                        stream  << Ch('\n');

                    if (sectionComment)
                        write_comment<Ptree>(stream, *sectionComment, commentStart);

                    stream << Ch('[') << *sectionName << Ch(']') << Ch('\n');
                }
                // write parameter
                if (comment)
                    write_comment<Ptree>(stream, *comment, commentStart);

                stream << it->first << Ch(' ') << Ch('=') << Ch(' ')
                       << it->second.template get_value<
                          std::basic_string<Ch> >()
                       << Ch('\n');
            }
        }

        template <typename Ptree>
        void write_top_level_keys(std::basic_ostream<
                                      typename Ptree::key_type::value_type
                                  > &stream,
                                  const Ptree& pt,
                                  const typename Ptree::key_type& commentKey,
                                  const typename Ptree::key_type::value_type& commentStart)
        {
            write_keys(stream, pt, false, commentKey, commentStart);
        }

        template <typename Ptree>
        void write_sections(std::basic_ostream<
                                typename Ptree::key_type::value_type
                            > &stream,
                            const Ptree& pt,
                            const typename Ptree::key_type& commentKey,
                            const typename Ptree::key_type::value_type& commentStart)
        {
            typedef typename Ptree::key_type::value_type Ch;
            typedef typename Ptree::key_type Str;
            for (typename Ptree::const_iterator it = pt.begin(), end = pt.end();
                 it != end; ++it)
            {
                boost::optional<Str> comment = it->second.template get_optional<Str>(commentKey);
                if (!it->second.empty()) {
                    check_dupes(it->second);
                    if(!it->second.data().empty() && comment) // top_level_key with comment
                        continue;
                    if (!it->second.data().empty())
                        BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                            "mixed data and children", "", 0));

                    write_keys(stream, it->second, true, commentKey, commentStart, it->first);
                }
            }
        }
    }

    /**
     * Read INI from a the given stream and translate it to a property tree.
     * @note Clears existing contents of property tree. In case of error
     *       the property tree is not modified.
     * @throw ini_parser_error If a format violation is found.
     * @param stream Stream from which to read in the property tree.
     * @param[out] pt The property tree to populate.
     *
     * Key comments will be stored in the propertyTree as a child of actual property
     * in an additional entry \c key.inicomment (wie von comment_key() returned)
     * Section comments will be stored in the propertyTree as a child of section first property
     * in an additional entry \c key.inicomment_section (wie von section_comment_key() returned)
     */
    template<class Ptree>
    void read_ini(std::basic_istream<
                    typename Ptree::key_type::value_type> &stream,
                  Ptree &pt)
    {
        typedef typename Ptree::key_type Str;
        typedef typename Str::value_type Ch;
        const Ch semicolon = stream.widen(';');
        const Ch hash = detail::comment_start_character<Ch>();
        const Ch lbracket = stream.widen('[');
        const Ch rbracket = stream.widen(']');
        const Str commentKey = detail::comment_key<Str>();
        const Str sectionCommentKey = detail::section_comment_key<Str>();

        Ptree local;
        unsigned long line_no = 0;
        Ptree *section = 0;
        Str line;
        Str lastComment;
        Str sectionComment;

        // For all lines
        while (stream.good())
        {

            // Get line from stream
            ++line_no;
            std::getline(stream, line);
            if (!stream.good() && !stream.eof())
                BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                    "read error", "", line_no));

            // If line is non-empty
            line = property_tree::detail::trim(line, stream.getloc());
            if (!line.empty())
            {
                // Comment, section or key?
                if (line[0] == semicolon || line[0] == hash)
                {
                    // Save comments to intermediate storage
                    if (!lastComment.empty())
                        lastComment += Ch('\n');
                    lastComment += line.substr(1);
                }
                else if (line[0] == lbracket)
                {
                    // If the previous section was empty, drop it again.
                    if (section && section->empty())
                        local.pop_back();
                    typename Str::size_type end = line.find(rbracket);
                    if (end == Str::npos)
                        BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                            "unmatched '['", "", line_no));
                    Str key = property_tree::detail::trim(
                        line.substr(1, end - 1), stream.getloc());
                    if (local.find(key) != local.not_found())
                        BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                            "duplicate section name", "", line_no));
                    section = &local.push_back(
                        std::make_pair(key, Ptree()))->second;
                    if (!lastComment.empty())
                    {
                        sectionComment = lastComment;
                        lastComment.clear();
                    }
                }
                else
                {
                    Ptree &container = section ? *section : local;
                    typename Str::size_type eqpos = line.find(Ch('='));
                    if (eqpos == Str::npos)
                        BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                            "'=' character not found in line", "", line_no));
                    if (eqpos == 0)
                        BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                            "key expected", "", line_no));
                    Str key = property_tree::detail::trim(
                        line.substr(0, eqpos), stream.getloc());
                    Str data = property_tree::detail::trim(
                        line.substr(eqpos + 1, Str::npos), stream.getloc());
                    if (container.find(key) != container.not_found())
                        BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                            "duplicate key name", "", line_no));
                    Ptree* keyTree = &container.push_back(std::make_pair(key, Ptree(data)))->second;
                    if (!lastComment.empty())
                    {
                        keyTree->put(commentKey, lastComment);
                        lastComment.clear();
                    }
                    if(!sectionComment.empty())
                    {
                        keyTree->put(sectionCommentKey, sectionComment);
                        sectionComment.clear();
                    }
                }
            }
        }
        // If the last section was empty, drop it again.
        if (section && section->empty())
            local.pop_back();

        // Swap local ptree with result ptree
        pt.swap(local);

    }

    /**
     * Read INI from a the given file and translate it to a property tree.
     * @note Clears existing contents of property tree.  In case of error the
     *       property tree unmodified.
     * @throw ini_parser_error In case of error deserializing the property tree.
     * @param filename Name of file from which to read in the property tree.
     * @param[out] pt The property tree to populate.
     * @param loc The locale to use when reading in the file contents.
     */
    template<class Ptree>
    void read_ini(const std::string &filename,
                  Ptree &pt,
                  const std::locale &loc = std::locale())
    {
        std::basic_ifstream<typename Ptree::key_type::value_type>
            stream(filename.c_str());
        if (!stream)
            BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                "cannot open file", filename, 0));
        stream.imbue(loc);
        try {
            read_ini(stream, pt);
        }
        catch (ini_parser_error &e) {
            BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                e.message(), filename, e.line()));
        }
    }

    /**
     * Translates the property tree to INI and writes it the given output
     * stream.
     * @pre @e pt cannot have data in its root.
     * @pre @e pt cannot have keys both data and children.
     * @pre @e pt cannot be deeper than two levels.
     * @pre There cannot be duplicate keys on any given level of @e pt.
     * @throw ini_parser_error In case of error translating the property tree to
     *                         INI or writing to the output stream.
     * @param stream The stream to which to write the INI representation of the 
     *               property tree.
     * @param pt The property tree to tranlsate to INI and output.
     * @param flags The flags to use when writing the INI file.
     *              No flags are currently supported.
     */
    template<class Ptree>
    void write_ini(std::basic_ostream<
                       typename Ptree::key_type::value_type
                   > &stream,
                   const Ptree &pt,
                   int flags = 0)
    {
        BOOST_ASSERT(validate_flags(flags));
        (void)flags;

        const typename Ptree::key_type commentKey = detail::comment_key<typename Ptree::key_type>();
        const typename Ptree::key_type::value_type commentStart = detail::comment_start_character<typename Ptree::key_type::value_type>();

        if (!pt.data().empty())
            BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                "ptree has data on root", "", 0));
        detail::check_dupes(pt);

        detail::write_top_level_keys(stream, pt, commentKey, commentStart);
        detail::write_sections(stream, pt, commentKey, commentStart);
    }

    /**
     * Translates the property tree to INI and writes it the given file.
     * @pre @e pt cannot have data in its root.
     * @pre @e pt cannot have keys both data and children.
     * @pre @e pt cannot be deeper than two levels.
     * @pre There cannot be duplicate keys on any given level of @e pt.
     * @throw info_parser_error In case of error translating the property tree
     *                          to INI or writing to the file.
     * @param filename The name of the file to which to write the INI
     *                 representation of the property tree.
     * @param pt The property tree to tranlsate to INI and output.
     * @param flags The flags to use when writing the INI file.
     *              The following flags are supported:
     * @li @c skip_ini_validity_check -- Skip check if ptree is a valid ini. The
     *     validity check covers the preconditions but takes <tt>O(n log n)</tt>
     *     time.
     * @param loc The locale to use when writing the file.
     */
    template<class Ptree>
    void write_ini(const std::string &filename,
                   const Ptree &pt,
                   int flags = 0,
                   const std::locale &loc = std::locale())
    {
        std::basic_ofstream<typename Ptree::key_type::value_type>
            stream(filename.c_str());
        if (!stream)
            BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                "cannot open file", filename, 0));
        stream.imbue(loc);
        try {
            write_ini(stream, pt, flags);
        }
        catch (ini_parser_error &e) {
            BOOST_PROPERTY_TREE_THROW(ini_parser_error(
                e.message(), filename, e.line()));
        }
    }

} } }

namespace boost { namespace property_tree
{
    using ini_parser::ini_parser_error;
    using ini_parser::read_ini;
    using ini_parser::write_ini;
} }

#endif
