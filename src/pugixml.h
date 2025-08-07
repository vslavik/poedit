/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2018-2025 Vaclav Slavik
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef Poedit_pugixml_h
#define Poedit_pugixml_h

#include "str_helpers.h"

#ifdef HAVE_PUGIXML
    #include <pugixml.hpp>
#else
    // #define PUGIXML_COMPACT ?
    #define PUGIXML_HEADER_ONLY
    #include "../deps/pugixml/src/pugixml.hpp"
#endif

namespace pugi
{

// Flags required for correct parsing of XML files with no loss of information
// FIXME: This includes parse_eol, which is undesirable: it converts files to Unix
//        line endings on save. OTOH without it, we'd have to do the conversion
//        manually both ways when extracting _and_ editing text.
constexpr auto PUGI_PARSE_FLAGS = parse_full | parse_ws_pcdata | parse_fragment;


/// Helper function to set an attribute on a node, creating it if it doesn't exist.
inline xml_attribute attribute(xml_node node, const char *name)
{
    auto a = node.attribute(name);
    return a ? a : node.append_attribute(name);
}

/// Check if a node contains only whitespace text
inline bool is_whitespace_only(xml_node node)
{
    if (!node || node.type() != node_pcdata)
        return false;

    const char* text = node.value();
    while (*text)
    {
        if (!std::isspace(static_cast<unsigned char>(*text)))
            return false;
        text++;
    }
    return true;
}


/// Does the node have any <elements> as children?
inline bool has_child_elements(xml_node node)
{
    return node.find_child([](xml_node n){ return n.type() == node_element; });
}


inline void remove_all_children(xml_node node)
{
    while (auto last = node.last_child())
        node.remove_child(last);
}


inline bool has_multiple_text_children(xml_node node)
{
    bool alreadyFoundOne = false;
    for (auto child = node.first_child(); child; child = child.next_sibling())
    {
        if (child.type() == node_pcdata || child.type() == node_cdata)
        {
            if (alreadyFoundOne)
                return true;
            else
                alreadyFoundOne = true;
        }
    }
    return false;
}


inline std::string get_node_text(xml_node node)
{
    // xml_node::text() returns the first text child, but that's not enough,
    // because some (weird) files in the wild mix text and CDATA content
    if (has_multiple_text_children(node))
    {
        std::string s;
        for (auto child = node.first_child(); child; child = child.next_sibling())
            if (child.type() == node_pcdata || child.type() == node_cdata)
                s.append(child.text().get());
        return s;
    }
    else
    {
        return node.text().get();
    }
}


inline void set_node_text(xml_node node, const std::string& text)
{
    // see get_node_text() for explanation
    if (has_multiple_text_children(node))
        remove_all_children(node);

    node.text() = text.c_str();
}


} // namespace pugi

#endif // Poedit_pugixml_h
